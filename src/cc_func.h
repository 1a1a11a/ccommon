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

#ifndef __CC_FUNC_H__
#define __CC_FUNC_H__

#define CC_OK        0
#define CC_ERROR    -1

#define CC_EAGAIN   -2
#define CC_ENOMEM   -3

typedef int rstatus_t;  /* generic function return value type */
typedef int err_t; /* erroneous values for rstatus_t */

#endif
