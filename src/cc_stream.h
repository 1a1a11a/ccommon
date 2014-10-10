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

#ifndef __CC_STREAM_H_
#define __CC_STREAM_H_

/* Note(yao): a stream supports serialized read/write of data, potentially over
 * a number of media: network sockets, file, shared memory, etc. While we have
 * not implemented all the underlying I/O mechanisms for each medium, we can
 * build an abstraction to provide a unified interface on top of them.
 *
 * From a service's perspective it needs a few essential parts: First, there
 * has to be channels over which requests/data arrive in order, such channels
 * can be TCP connections, Unix Domain Socket file desriptors, a consecutive
 * area in memory, among others. Second, there needs to be a destination and an
 * accompanying format at which the arrived data can be read into. This could
 * be as simple as a memory area, or something like the msg data structures
 * implemented in the ccommon library. Finally, given the protocols we will
 * need to support, there should be at least two ways to specify how much data
 * should/can be read from the channels: by length or by delimiter. A protocol
 * that relies on fixed-length fields or fields whose length are already known,
 * such as Redis, can use the former; protocols such as Memcached ASCII relies
 * on delimiters (and sometimes, size), so both are required to support them.
 *
 * To make the channel IO work with the rest of the service, we use callbacks.
 * Upon receiving a message or some data, a pre-defined routine is called; same
 * goes for sending messages/data.
 *
 * The idea described here is not dissimilar to the use of channels/streams
 * in Plan 9.
 */

/* TODO: take buffers out of stream */

#include <cc_define.h>
#include <cc_option.h>
#include <cc_queue.h>

#include <inttypes.h>
#include <stdlib.h>

#define STREAM_POOLSIZE 0 /* unlimited */

/*          name                type                default                 description */
#define STREAM_OPTION(ACTION)                                                                   \
    ACTION( stream_poolsize,    OPTION_TYPE_UINT,   str(STREAM_POOLSIZE),   "stream pool size" )

typedef enum channel_type {
    CHANNEL_UNKNOWN,
    CHANNEL_TCP,
    CHANNEL_TCPLISTEN
} channel_type_t;

typedef void * channel_t;

struct stream;

typedef channel_t (*channel_open_t)(void *data);
typedef void (*channel_close_t)(channel_t channel);
typedef int (*channel_fd_t)(channel_t channel);
typedef void (*data_handler_t)(struct stream *stream, size_t nbyte);

typedef struct stream_handler {
    channel_open_t  open;       /* callback to open a channel */
    channel_close_t close;      /* callback to close a channel */
    channel_fd_t    fd;         /* callback to get channel fd*/
    data_handler_t  pre_read;   /* callback before msg received */
    data_handler_t  post_read;  /* callback after msg received */
    data_handler_t  pre_write;  /* callback before msg sent */
    data_handler_t  post_write; /* callback after msg sent */
} stream_handler_t;

/* Note(yao): should we use function pointers for the underlying actions and
 * initialized them with the proper channel-specific version when we create the
 * stream? (FYI: This style is used by nanomsg.)
 *
 * Note(yao): I'm now much more inclined to use function pointers and keep the
 * stream module to a minimum, so stream really reflects what is an interface
 * in Java or Go, in that it defines to operations but has no 'meat'.
 *
 * we can also support using vector read and write, especially write, but that
 * doesn't necessarily require a vector-ed write buffer (only the iov needs to
 * know where each block of memory starts and ends). For write, the assmebly
 * of that array should happen outside of the stream module.
 */
struct stream {
    STAILQ_ENTRY(stream) next;

    void                 *owner;     /* owner of the stream */

    channel_type_t       type;       /* type of the communication channels */
    channel_t            channel;    /* underlying bi-directional channels */

    struct mbuf          *rbuf;      /* read buffer */
    struct mbuf          *wbuf;      /* write buffer */
    stream_handler_t     *handler;   /* stream handlers */

    void                 *data;      /* stream data: e.g. request queue */

    err_t                err;        /* error */
};

/* channel/medium agnostic data IO */
rstatus_t stream_read(struct stream *stream, size_t nbyte);
rstatus_t stream_write(struct stream *stream, size_t nbyte);

struct stream *stream_create(void);
void stream_destroy(struct stream *stream);
void stream_reset(struct stream *stream);

void stream_pool_create(uint32_t max);
void stream_pool_destroy(void);
struct stream *stream_borrow(void);
void stream_return(struct stream *stream);


/* NOTE(yao): a yield mechanism for the caller to postpone I/O to the future,
 * especially recv. This can be used to avoid starvation and improve fairness.
 */

/* NOTE(yao): we choose not to implement receiving and sending from raw bufs
 * for now, so everything has to be copied into stream's message buffer first,
 * the saving from doing zero-copy can be significant for large payloads, so
 * it will need to be supported before we use unified backend for those cases.
 * Another set of useful APIs are those that read past a certain delimiter
 * before returning, but that can wait, too.
 * We may want to add the scatter-gather style write, i.e. sendmsg() alike,
 * very soon*/

#endif
