/**
 * @file log_app.c
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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "constants.h"
#include "channel.h"
#include "log_app.h"
#include "utils.h"

/** Static predefined application log messages. 
 * Order matters and MUST match one for @ref app_log_loop_args_t.
 */
static const char *app_log_msg_id_txt[] = {
    "Unknown", //0
    "vl_fn_epoll: code error, event id not recognized",
    "vl_fn_tcp_accept_conns: non-supported client IP socket family on TCP connection",
    "vl_fn_tcp_accept_conns: non-supported local IP socket family on TCP connection",
    "vl_fn_tcp_accept_conns: getsockname() coder error or system out of resources",
    "vl_run: could not set CPU affinity for vectorloop thread, performance might be impacted.", //5
};

/** Application log loop function. This function is started by pthread.
 * 
 * @param args Arguments passed to application log loop.
 * 
 * @return     On loop exit returns a NULL pointer only because it needs to
 *             abide by pthread API.
 */
void *
log_app_loop(void *args)
{
    app_log_loop_args_t *app_loop_args    = (app_log_loop_args_t *)args;
    config_t            *cfg              = app_loop_args->cfg;
    channel_log_t       *app_log_channels = app_loop_args->app_log_channels;
    metrics_t           *metrics          = app_loop_args->metrics;
    size_t               channel_count    = cfg->process_thread_count;
    int                  msg_count        = 0;
    channel_log_msg_t  **messages         = NULL;
    struct timespec      sleep_time       = {
                                                .tv_sec = 0,
                                                .tv_nsec = APP_LOG_LOOP_SLEEP_TIME,
                                            };
    struct timespec      current_time;
    struct iovec        *iov;
    size_t               byte_count;
    ssize_t              ret;
    char                 time_buf[TIME_RFC3339_STRLEN+3];
    size_t               time_len;
    bool                 exit_by_msg      = false;

    int                  log_fd = -1;
    struct timespec      log_open_time = { .tv_sec = 0, .tv_nsec = 0};

    /* Initialize channels on this thread(core). */
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
    
    messages = malloc(sizeof(channel_log_msg_t *) * channel_count);
    CHECK_MALLOC(messages);

    /* Initialize IO vector*/
    iov = malloc(sizeof(struct iovec) * channel_count * 3);

    while (1) {
        /* Get current time. */
        utl_clock_gettime_rt_fatal(&current_time);


        /* If file is not opened for append, see if it is time to try to open it. */
        if (log_fd < 0 &&
            utl_diff_timespec_as_double(&log_open_time, &current_time) <= 0) {
            /* Attempt to open log file for append writes. */
            log_fd = open(cfg->application_log_realpath, O_CREAT|O_WRONLY|O_APPEND,
                          00777);
            if (log_fd < 0) {
                /* Error opening file! */
                debug_printf("Error opening application log file %s, %s",
                             cfg->application_log_name,  strerror(errno));
                atomic_fetch_add(&metrics->app.app_log_open_error, 1);
                
                /* Set next try time. */
                log_open_time = current_time;
                log_open_time.tv_sec += APP_LOG_OPEN_WAIT_TIME;
            } else {
                debug_printf("Application log file %s opened for append writes, fd: %d",
                             cfg->application_log_name, log_fd);
            }
        }

        msg_count  = 0;
        byte_count = 0;
        time_len   = 0;
        
        /* Collect messages from channels and add then to io vector.*/
        for (size_t i = 0; i < channel_count; i++) {
            if (channel_log_recv(&app_log_channels[i], &messages[msg_count]) != 0) {
                /* If time was not formatted, format it! */
                if (time_len == 0) {
                    utl_timespec_to_rfc3339nano(&current_time, time_buf);
                    time_len = strlen(time_buf);
                    time_buf[time_len]   = ' ';
                    time_buf[time_len+1] = '-';
                    time_buf[time_len+2] = ' ';
                }

                /* Add message to io vector. */
                if (log_fd > -1) {
                    int iov_i = msg_count * 3;
                    iov[iov_i].iov_base = time_buf;
                    iov[iov_i].iov_len  = time_len + 3;

                    if (messages[msg_count]->log_msg_id != 0) {
                        iov[iov_i+1].iov_base = (void *)app_log_msg_id_txt[messages[msg_count]->log_msg_id];
                        iov[iov_i+1].iov_len  = strlen(app_log_msg_id_txt[messages[msg_count]->log_msg_id]);
                    } else {
                        iov[iov_i+1].iov_base = messages[msg_count]->log_msg;
                        iov[iov_i+1].iov_len  = strlen(messages[msg_count]->log_msg);
                    }
                    iov[iov_i+2].iov_base = "\n";
                    iov[iov_i+2].iov_len  = 1;

                    byte_count += iov[iov_i].iov_len + iov[iov_i+1].iov_len + 1;

                    /* Was exit indicated. */
                    if (messages[msg_count]->exit == true) {
                        fprintf(stderr, "%s", messages[msg_count]->log_msg);
                        exit_by_msg = true;
                    }
                }
                msg_count  += 1;
            }
        }

        /* If there were messages received write them to log file. */
        if (msg_count > 0) {
            /* Write io vector to log. */
            if (log_fd > -1) {
                errno = 0;
                ret = writev(log_fd, iov, msg_count * 3);
                if (ret < byte_count) {
                    /* Error writing to disk. */
                    debug_printf("Error writing to application log file %s, writev() returned: %ld",
                                 cfg->application_log_name, ret);
                    atomic_fetch_add(&metrics->app.app_log_write_error, msg_count);

                    /* Close log file & try to reopen it in next loop iteration. */
                    close(log_fd);
                    log_fd = -1;
                    log_open_time = (struct timespec) { .tv_sec = 0, .tv_nsec = 0};
                }
            } else {
                /* Application log file is not open, increment message drop counter. */
                atomic_fetch_add(&metrics->app.app_log_write_error, msg_count);
            }
            /* Cleanup messages. */
            for (int i = 0; i < msg_count; i++) {
                channel_log_msg_release(messages[msg_count]);
            }
            /* Exit if requested. */
            if (exit_by_msg == true) {
                exit(1);
            }
        } else {
            /* No messages to write to log. */
            clock_nanosleep(CLOCK_REALTIME, 0, &sleep_time, NULL);
        }
    }

    return NULL;
}
