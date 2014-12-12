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

#ifndef _CC_EVENT_H_
#define _CC_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cc_define.h>

#include <inttypes.h>

#define EVENT_SIZE  1024

#define EVENT_READ  0x0000ff
#define EVENT_WRITE 0x00ff00
#define EVENT_ERR   0xff0000

typedef void (*event_cb_t)(void *, uint32_t);  /* event callback */

struct event_base;

/* event base */
struct event_base *event_base_create(int size, event_cb_t cb);
void event_base_destroy(struct event_base **evb);

/* event control */
int event_add_read(struct event_base *evb, int fd, void *data);
int event_add_write(struct event_base *evb, int fd, void *data);
int event_register(struct event_base *evb, int fd, void *data);
int event_deregister(struct event_base *evb, int fd);

/* event wait */
int event_wait(struct event_base *evb, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* _CC_EVENT_H */
