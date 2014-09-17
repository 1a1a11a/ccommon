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

#ifndef __CC_NIO_H_
#define __CC_NIO_H_

#include <cc_array.h>
#include <cc_define.h>
#include <cc_stream.h>

#include <inttypes.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>

/* Note(yao): the abstraction of the io module is subject to re-design in the
 * near future.
 *
 * So far, all existing cache code bases implicitly assume using some framed
 * protocol, sockets for I/O, and handle signaling using an asynchronous event
 * loop over a list/pool of connections. While this will continue to be true in
 * many cases, as a library we should maximize flexibility by having connection,
 * I/O, connection pool, connection monitoring independent of each other, and
 * assemble them under a higher level struct. This will also allow us to reuse
 * as much code/abstraction as possible when we introduce I/O over other media,
 * such as shared memory, message queue or files.
 *
 * I'm not exactly sure what will be the right abstraction for misc I/O, having
 * dealt with only network and events. I will revisit this module after gaining
 * more concrete experiences with the generic cases.
 */


#define CONN_RAW        0
#define CONN_CLIENT     1
#define CONN_SERVER     2
#define CONN_PROXY      3

#define CONN_CONNECT    0
#define CONN_CONNECTED  1
#define CONN_EOF        2
#define CONN_CLOSE      3

struct conn {
    STAILQ_ENTRY(conn)  next;           /* for conn pool */

    int                 sd;             /* socket descriptor */
    int                 family;         /* socket address family */
    socklen_t           addrlen;        /* socket length */
    struct sockaddr     *addr;          /* socket address */

    size_t              recv_nbyte;     /* received (read) bytes */
    size_t              send_nbyte;     /* sent (written) bytes */

    bool                recv_active:1;  /* recv active? */
    bool                send_active:1;  /* send active? */
    bool                recv_ready:1;   /* recv ready? */
    bool                send_ready:1;   /* send ready? */

    unsigned            mode:2;         /* client|server|proxy */
    unsigned            state:2;        /* connect|connected|eof|close */
    unsigned            flags:12;       /* annotation fields */

    err_t               err;            /* errno */
};

void conn_setup(void);
void conn_teardown(void);

void conn_reset(struct conn *conn);
struct conn *conn_create(void);
void conn_destroy(struct conn *conn);

ssize_t conn_recv(struct conn *conn, void *buf, size_t nbyte);
ssize_t conn_send(struct conn *conn, void *buf, size_t nbyte);
ssize_t conn_recvv(struct conn *conn, struct array *bufv, size_t nbyte);
ssize_t conn_sendv(struct conn *conn, struct array *bufv, size_t nbyte);

void conn_pool_create(uint32_t max);
void conn_pool_destroy(void);
struct conn *conn_borrow(void);
void conn_return(struct conn *conn);

#endif
