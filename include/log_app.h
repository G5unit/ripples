/**
 * @file log_app.h
 * @author Faruk Grozdanic
 * @copyright Released under MIT license.
 *
 * Copyright (c) 2025 Faruk Grozdanic
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/** \defgroup applog Application Log
 * 
 * @brief Application log logs application status and abnormal conditions.
 *        There is a dedicated thread to log messages to file (disk). This
 *        ensures that blocking IO operations are isolated from other DNS
 *        query processing threads.
 *
 *  @{
 */
#ifndef LOG_APP_H
#define LOG_APP_H

#include "config.h"
#include "channel.h"
#include "metrics.h"

/** Enumerated static predefined application log messages. 
 * Order matters and MUST match one for @ref app_log_msg_id_txt.
 */
typedef enum app_log_msg_id_e {
    APP_LOG_MSG_CUSTOM = 0,
    APP_LOG_MSG_VL_FN_EPOLL,
    APP_LOG_MSG_VL_FN_TCP_CONN_CLIENT_IP_FAM,
    APP_LOG_MSG_VL_FN_TCP_CONN_LOCAL_IP_FAM,
    APP_LOG_MSG_VL_FN_TCP_CONN_GETSOCKNAME,
    APP_LOG_MSG_VL_RUN_CPU_AFFINITY,
} app_log_msg_id_t;

/** Structure holds arguments for @ref log_app_loop() function. As
 * log_app_loop function is started via a pthread it can only handle a single
 * argument hence having a structure to hold all the different arguments
 * log_app_loop needs.
 */
typedef struct app_log_loop_args_s {
    /** Configuration with settings to use. */
    config_t *cfg;

    /** Application log channels log messages are received on. */
    channel_log_t *app_log_channels;

    /** Metrics object to record statistics. */
    metrics_t *metrics;
} app_log_loop_args_t;

void * log_app_loop(void *args);

#endif /* LOG_APP_H */

/** @}*/