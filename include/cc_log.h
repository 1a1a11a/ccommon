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

#ifndef _CC_LOG_H_
#define _CC_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cc_define.h>
#include <cc_util.h>

#define LOG_LEVEL 4 /* default log level */

/*          name        type                default           description */
#define LOG_OPTION(ACTION)                                                  \
    ACTION( log_level,  OPTION_TYPE_UINT,   str(LOG_LEVEL),   "log level"  )\
    ACTION( log_name,   OPTION_TYPE_STR,    NULL,             "log name"   )\

#define LOG_MAX_LEN 2560 /* max length of log message */

/*
 * TODO(yao): a reasonable guideline for using these different levels.
 */
#define LOG_ALWAYS  0   /* always log, special value  */
#define LOG_CRIT    1   /* critical: usually warrants exiting */
#define LOG_ERROR   2   /* error: may need action */
#define LOG_WARN    3   /* warning: may need attention */
#define LOG_INFO    4   /* informational: important but normal */
#define LOG_DEBUG   5   /* debug: abnormal behavior that's not an error */
#define LOG_VERB    6   /* verbose: showing normal logic flow */
#define LOG_VVERB   7   /* verbose on crack, for annoying msg e.g. timer */

int log_level;

/* NOTE(yao): it may be useful to have a sampled log func for bursty events */

/*
 * log_stderr   - log to stderr
 *
 * loga         - log always
 * loga_hexdump - log hexdump always
 *
 * log_panic    - log messages followed by a panic, when LOG_CRIT is met
 * log_error    - error log messages
 * log_warn     - warning log messages
 * ...
 *
 * log          - debug log messages based on a log level (subject to config)
 * log_hexdump  - hexadump -C of a log buffer (subject to config)
 */

#define loga(...) _log(__FILE__, __LINE__, LOG_ALWAYS, __VA_ARGS__)

#define loga_hexdump(_data, _datalen, ...) do {                             \
    _log(__FILE__,__LINE__, LOG_ALWAYS, __VA_ARGS__);                       \
    _log_hexdump(-1, (char *)(_data), (int)(_datalen));                     \
} while (0)                                                                 \

#define log_panic(...) do {                                                 \
    _log(__FILE__, __LINE__, LOG_CRIT, __VA_ARGS__);                        \
    abort();                                                                \
} while (0)

#if defined CC_LOGGING && CC_LOGGING == 1

#define log_crit(...) do {                                                  \
    if (log_level >= LOG_CRIT) {                                            \
        _log(__FILE__, __LINE__, LOG_CRIT, __VA_ARGS__);                    \
    }                                                                       \
} while (0)

#define log_error(...) do {                                                 \
    if (log_level >= LOG_ERROR) {                                           \
        _log(__FILE__, __LINE__, LOG_ERROR, __VA_ARGS__);                   \
    }                                                                       \
} while (0)

#define log_warn(...) do {                                                  \
    if (log_level >= LOG_WARN) {                                            \
        _log(__FILE__, __LINE__, LOG_WARN, __VA_ARGS__);                    \
    }                                                                       \
} while (0)

#define log_info(...) do {                                                  \
    if (log_level >= LOG_INFO) {                                            \
        _log(__FILE__, __LINE__, LOG_INFO, __VA_ARGS__);                    \
    }                                                                       \
} while (0)

#define log_debug(...) do {                                                 \
    if (log_level >= LOG_DEBUG) {                                           \
        _log(__FILE__, __LINE__, LOG_DEBUG, __VA_ARGS__);                   \
    }                                                                       \
} while (0)

#define log_verb(...) do {                                                  \
    if (log_level >= LOG_VERB) {                                            \
        _log(__FILE__, __LINE__, LOG_VERB, __VA_ARGS__);                    \
    }                                                                       \
} while (0)

#define log_vverb(...) do {                                                 \
    if (log_level >= LOG_VVERB) {                                           \
        _log(__FILE__, __LINE__, LOG_VVERB, __VA_ARGS__);                   \
    }                                                                       \
} while (0)

#define log(_level, ...) _log(__FILE__, __LINE__, _level, __VA_ARGS__)

#define log_hexdump(_level, _data, _datalen, ...) do {                      \
    _log(__FILE__,__LINE__, _level, __VA_ARGS__);                           \
    _log_hexdump(_level, (char *)(_data), (int)(_datalen));                 \
} while (0)

#else

#define log_crit(...)
#define log_error(...)
#define log_warn(...)
#define log_info(...)
#define log_debug(...)
#define log_verb(...)
#define log_vverb(...)

#define log(_level, ...)
#define log_hexdump(_level, _data, _datalen, ...)

#endif

#define log_stderr(...) _log_fd(STDERR_FILENO, __VA_ARGS__)
#define log_stdout(...) _log_fd(STDOUT_FILENO, __VA_ARGS__)

int log_setup(int level, char *filename);
void log_teardown(void);

void log_level_up(void);
void log_level_down(void);
void log_level_set(int level);

void log_reopen(void);

void _log(const char *file, int line, int level, const char *fmt, ...);
void _log_fd(int fd, const char *fmt, ...);
void _log_hexdump(int level, char *data, int datalen);

#ifdef __cplusplus
}
#endif

#endif /* _CC_LOG_H_ */
