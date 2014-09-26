/*
 * ccommon - a cache common library.
 * Copyright (C) 2013 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cc_stream.h>

#include <cc_debug.h>
#include <cc_log.h>
#include <cc_mbuf.h>
#include <cc_mm.h>
#include <cc_nio.h>
#include <cc_pool.h>
#include <cc_util.h>

#include <limits.h>
#include <sys/uio.h>

/*
#if (IOV_MAX > 128)
#define CC_IOV_MAX 128
#else
#define CC_IOV_MAX IOV_MAX
#endif
*/

#define STREAM_MODULE_NAME "ccommon::stream"
FREEPOOL(stream_pool, streamq, stream);
struct stream_pool streamp;

/* recv nbyte at most and store it in rbuf associated with the stream.
 * Stream buffer must provide enough capacity, otherwise CC_NOMEM is returned.
 * callback pre_read, if not NULL, is called before receiving data
 * callback post_read, if not NULL, is called after receiving data
 *
 * the role of pre_read is some sort of check- back pressure? buffer readiness?
 * post_read is the callback that is suppose to deal with the received data,
 * and that's why we won't call it if no data is received
 */
rstatus_t
stream_read(struct stream *stream, size_t nbyte)
{
    stream_handler_t *handler;
    struct conn *c = NULL;
    rstatus_t status;
    size_t capacity;
    ssize_t n = 0; /* bytes actually received */

    ASSERT(stream != NULL);
    ASSERT(stream->type != CHANNEL_UNKNOWN);
    ASSERT(stream->rbuf != NULL);
    ASSERT(stream->handler != NULL);
    ASSERT(nbyte <= SSIZE_MAX);

    handler = stream->handler;

    /* rbuf shouldn't be deallocated after this, do we need an assert below? */
    if (handler->pre_read != NULL) {
        handler->pre_read(stream, nbyte);
    }

    capacity = mbuf_wsize(stream->rbuf);
    if (capacity < nbyte) {
        log_debug(LOG_VERB, "not enough capacity in rbuf at %p of stream at %p:"
                "nbyte %zu, write capacity %zu", stream->rbuf, stream, nbyte,
                capacity);

        return CC_ENOMEM;
    }

    /* receive based on channel type, and schedule retries if necessary */
    switch (stream->type) {
    case CHANNEL_TCP:   /* TCP socket */
        c = stream->channel;
        n = conn_recv(c, stream->rbuf->wpos, nbyte);
        if (n < 0) {
            if (n == CC_EAGAIN) {
                log_debug(LOG_VERB, "recv on stream %p of type %d returns "
                        "rescuable error: EAGAIN", stream, stream->type);

                status = CC_OK;
            } else {
                log_debug(LOG_VERB, "recv on stream %p of type %d returns "
                        "other error: %d", stream, stream->type, n);
                log_debug(LOG_INFO, "channel %p of stream %p of type %d closed", c,
                    stream, stream->type);

                status = CC_ERROR;
            }
        } else if (n == 0) {
            log_debug(LOG_INFO, "channel %p of stream %p of type %d closed", c,
                    stream, stream->type);

            status = CC_ERDHUP;
        } else if (n == nbyte) {
            status = CC_ERETRY;
        } else {
            status = CC_OK;
        }

        break;

    default:
        NOT_REACHED();
        status = CC_ERROR;
    }

    log_debug(LOG_VERB, "recv %zd bytes on stream %p of type %d", n, stream,
            stream->type);

    stream->rbuf->wpos += (n > 0) ? n : 0;
    if (n > 0 && handler->post_read != NULL) {
        handler->post_read(stream, (size_t)n);
    }

    return status;
}


/* send nbyte at most from data stored in wbuf associated with the stream.
 * callback pre_write, if not NULL, is called before sending data
 * callback post_write, if not NULL, is called after sending data
 *
 * the role of pre_write is some sort of check- buffer readiness? bookkeeping?
 * post_write is the callback that is suppose to clean up the buffer after data
 * is sent, and that's why we won't call it if no data is actually sent
*/
rstatus_t
stream_write(struct stream *stream, size_t nbyte)
{
    stream_handler_t *handler;
    struct conn *c = NULL;
    rstatus_t status;
    size_t content;
    ssize_t n = 0; /* bytes actually sent */

    ASSERT(stream != NULL);
    ASSERT(stream->type != CHANNEL_UNKNOWN);
    ASSERT(stream->wbuf != NULL);
    ASSERT(stream->handler != NULL);
    ASSERT(nbyte <= SSIZE_MAX);

    handler = stream->handler;

    if (handler->pre_write != NULL) {
        handler->pre_write(stream, nbyte);
    }

    content = mbuf_rsize(stream->wbuf);

    if (content == 0) {
        log_debug(LOG_VERB, "no data to send in wbuf at %p of stream %p",
                stream->wbuf, stream);

        return CC_EEMPTY;
    }

    /* send based on channel type, and schedule retries if necessary */
    switch (stream->type) {
    case CHANNEL_TCP:   /* TCP socket */
        c = stream->channel;
        n = conn_send(stream->channel, stream->wbuf->rpos, content);

        if (n < 0) {
            if (n == CC_EAGAIN) {
                log_debug(LOG_VERB, "send on stream %p of type %d returns "
                        "rescuable error: EAGAIN", stream, stream->type);
                return CC_EAGAIN;
            } else {
                log_debug(LOG_VERB, "send on stream %p of type %d returns "
                        "other error: %d", stream, stream->type, n);
                log_debug(LOG_INFO, "channel %p of stream %p of type %d closed", c,
                    stream, stream->type);

                return CC_ERROR;
            }
        }

        if (n < content) {
            status = CC_ERETRY;
        } else {
            status = CC_OK;
        }

        break;

    default:
        NOT_REACHED();
        status = CC_ERROR;
    }

    log_debug(LOG_VERB, "send %zd bytes on stream %p of type %d", n, stream,
            stream->type);

    stream->wbuf->rpos += (n > 0) ? n : 0;
    if (n > 0 && handler->post_write != NULL) {
        handler->post_write(stream, (size_t)n);
    }

    return status;
}

struct stream *
stream_create(void)
{
    struct stream *stream;

    stream = cc_alloc(sizeof(struct stream));
    if (stream == NULL) {
        return NULL;
    }

    stream->rbuf = mbuf_borrow();
    if (stream->rbuf == NULL) {
        cc_free(stream);

        return NULL;
    }

    stream->wbuf = mbuf_borrow();
    if (stream->wbuf == NULL) {
        mbuf_return(stream->rbuf);
        cc_free(stream);

        return NULL;
    }

    return stream;
}

void
stream_destroy(struct stream *stream)
{
    ASSERT (stream != NULL);
    ASSERT(stream->handler != NULL);
    ASSERT(stream->data == NULL);

    stream->handler->close(stream->channel);

    mbuf_return(stream->rbuf);
    mbuf_return(stream->wbuf);

    cc_free(stream);
}

void
stream_pool_create(uint32_t max)
{
    log_debug(LOG_INFO, "creating stream pool: max %"PRIu32, max);

    FREEPOOL_CREATE(&streamp, max);
}

void
stream_pool_destroy(void)
{
    struct stream *stream, *tstream;

    log_debug(LOG_INFO, "destroying stream pool: free %"PRIu32, streamp.nfree);

    FREEPOOL_DESTROY(stream, tstream, &streamp, next, stream_destroy);
}

struct stream *
stream_borrow(void)
{
    struct stream *stream;

    FREEPOOL_BORROW(stream, &streamp, next, stream_create);

    if (stream == NULL) {
        log_debug(LOG_DEBUG, "borrow stream failed: OOM");
        return NULL;
    }

    log_debug(LOG_VVERB, "borrow stream %p", stream);

    return stream;
}

void
stream_return(struct stream *stream)
{
    /* NOTE: mbufs are not returned, still affiliated with stream */
    log_debug(LOG_VVERB, "return stream *p", stream);

    FREEPOOL_RETURN(&streamp, stream, next);
}
