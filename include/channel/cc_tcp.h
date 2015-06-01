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

#ifndef __CC_TCP_H_
#define __CC_TCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cc_array.h>
#include <cc_channel.h>
#include <cc_define.h>
#include <cc_event.h>
#include <cc_option.h>
#include <cc_queue.h>
#include <cc_util.h>

#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

/**
 * This implements the channel interface for TCP.
 */

#define TCP_BACKLOG  128
#define TCP_POOLSIZE 0 /* unlimited */

/*          name                type            description */
#define TCP_METRIC(ACTION)                                                      \
    ACTION( tcp_conn_created,   METRIC_COUNTER, "# tcp connections created"    )\
    ACTION( tcp_conn_create_ex, METRIC_COUNTER, "# tcp conn create exceptions" )\
    ACTION( tcp_conn_destroyed, METRIC_COUNTER, "# tcp connections destroyed"  )\
    ACTION( tcp_conn_total,     METRIC_GAUGE,   "# tcp conn allocated"         )\
    ACTION( tcp_conn_borrowed,  METRIC_COUNTER, "# tcp connections borrowed"   )\
    ACTION( tcp_conn_borrow_ex, METRIC_COUNTER, "# tcp conn borrow exceptions" )\
    ACTION( tcp_conn_returned,  METRIC_COUNTER, "# tcp connections returned"   )\
    ACTION( tcp_conn_active,    METRIC_GAUGE,   "# tcp conn being borrowed"    )\
    ACTION( tcp_accept,         METRIC_COUNTER, "# tcp connection accepts"     )\
    ACTION( tcp_accept_ex,      METRIC_COUNTER, "# tcp accept exceptions"      )\
    ACTION( tcp_reject,         METRIC_COUNTER, "# tcp connection rejects"     )\
    ACTION( tcp_reject_ex,      METRIC_COUNTER, "# tcp reject exceptions"      )\
    ACTION( tcp_connect,        METRIC_COUNTER, "# tcp connects made"          )\
    ACTION( tcp_connect_ex,     METRIC_COUNTER, "# tcp connect exceptions "    )\
    ACTION( tcp_close,          METRIC_COUNTER, "# tcp connection closed"      )\
    ACTION( tcp_recv,           METRIC_COUNTER, "# recv attempted"             )\
    ACTION( tcp_recv_ex,        METRIC_COUNTER, "# recv exceptions"            )\
    ACTION( tcp_recv_byte,      METRIC_COUNTER, "# bytes received"             )\
    ACTION( tcp_send,           METRIC_COUNTER, "# send attempted"             )\
    ACTION( tcp_send_ex,        METRIC_COUNTER, "# send exceptions"            )\
    ACTION( tcp_send_byte,      METRIC_COUNTER, "# bytes sent"                 )

typedef struct {
    TCP_METRIC(METRIC_DECLARE)
} tcp_metric_st;


/*          name            type                default             description */
#define TCP_OPTION(ACTION)                                                                  \
    ACTION( tcp_backlog,   OPTION_TYPE_UINT,   str(TCP_BACKLOG),  "tcp conn backlog limit" )\
    ACTION( tcp_poolsize,  OPTION_TYPE_UINT,   str(TCP_POOLSIZE), "tcp conn pool size"     )

/* possible states */
#define CONN_UNKNOWN    0
#define CONN_CONNECT    1
#define CONN_CONNECTED  2
#define CONN_EOF        3
#define CONN_CLOSING    4
#define CONN_LISTEN     5

/* TODO(yao): make struct conn tcp specific */
struct conn {
    STAILQ_ENTRY(conn)  next;           /* for conn pool */
    bool                free;           /* in use? */

    ch_type_t           type;           /* channel type */
    ch_level_t          level;          /* meta or base */
    ch_id_t             sd;             /* socket descriptor */

    size_t              recv_nbyte;     /* received (read) bytes */
    size_t              send_nbyte;     /* sent (written) bytes */

    unsigned            state:4;        /* defined as TCP_ above */
    unsigned            flags:12;       /* annotation fields */

    err_t               err;            /* errno */
};

void tcp_setup(int backlog, tcp_metric_st *metrics);
void tcp_teardown(void);

void conn_reset(struct conn *c);

/* resource management */
struct conn *conn_create(void);     /* channel_get_fn, with allocation */
void conn_destroy(struct conn **c); /* channel_put_fn, with deallocation  */

void conn_pool_create(uint32_t max);
void conn_pool_destroy(void);
struct conn *conn_borrow(void);     /* channel_get_fn, with resource pool */
void conn_return(struct conn **c);  /* channel_put_fn, with resource pool */

static inline ch_id_t conn_id(struct conn *c)
{
    return c->sd;
}

/* basic channel maintenance */
bool tcp_connect(struct addrinfo *ai, struct conn *c);  /* channel_open_fn, client */
bool tcp_listen(struct addrinfo *ai, struct conn *c);   /* channel_open_fn, server */
void tcp_close(struct conn *c);                         /* channel_term_fn */
ssize_t tcp_recv(struct conn *c, void *buf, size_t nbyte); /* channel_recv_fn */
ssize_t tcp_send(struct conn *c, void *buf, size_t nbyte); /* channel_send_fn */
ssize_t tcp_recvv(struct conn *c, struct array *bufv, size_t nbyte);
ssize_t tcp_sendv(struct conn *c, struct array *bufv, size_t nbyte);

bool tcp_accept(struct conn *sc, struct conn *c);   /* channel_accept_fn */
void tcp_reject(struct conn *sc);                   /* channel_reject_fn */

/* functions getting/setting connection attribute */
int tcp_set_blocking(int sd);
int tcp_set_nonblocking(int sd);
int tcp_set_reuseaddr(int sd);
int tcp_set_tcpnodelay(int sd);
int tcp_set_keepalive(int sd);
int tcp_set_linger(int sd, int timeout);
int tcp_unset_linger(int sd);
int tcp_set_sndbuf(int sd, int size);
int tcp_set_rcvbuf(int sd, int size);
int tcp_get_sndbuf(int sd);
int tcp_get_rcvbuf(int sd);
int tcp_get_soerror(int sd);
void tcp_maximize_sndbuf(int sd);

#ifdef __cplusplus
}
#endif

#endif
