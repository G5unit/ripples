/**
 * @file query_log_loop.c
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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "constants.h"
#include "channel.h"
#include "log_app.h"
#include "query.h"
#include "utils.h"


/** Open query log file for write.
 * 
 * @param cfg         Configuration with settings to use.
 * @param err_msg     Buffer to populate error message if error is one encountered.
 * @param err_msg_len Length (in bytes) of err_msg buffer.
 *
 * @return            On success return sfile descriptor, otherwise error
 *                    occurred and -1 is returned in which case error message
 *                    in populated in err_mgs buffer.
 */
static int
query_log_loop_openfile(config_t *cfg, char *err_msg, size_t err_msg_len)
{
    int             fd = -1;
    struct timespec ts;
    char            current_time_str[TIME_RFC3339_STRLEN];
    char            filename[QUERY_LOG_FILENAME_MAX_LEN];

    utl_clock_gettime_rt_fatal(&ts);
    utl_timespec_to_rfc3339nano(&ts, current_time_str);
    snprintf(filename, QUERY_LOG_FILENAME_MAX_LEN, "%s/%s_%s",
             cfg->query_log_realpath, cfg->query_log_base_name, current_time_str);
    fd = open(filename, O_CREAT|O_WRONLY|O_APPEND, 00777);
    if (fd < 0) {
        /* Error opening file! */
        snprintf(err_msg, err_msg_len, "Error opening query log file %s, %s",
                 filename, strerror(errno));
        debug_printf("%s", err_msg);
    } else {
        debug_printf("Query log file %s opened for append writes, fd: %d",
                     filename, fd);
    }
    return fd;
}

/** Function polls vectorloop threads for query log data and writes it to file.
 * This function is meant to run in its own dedicated thread.
 * 
 * @param args  Structure with settings to use.
 * 
 * @return     Returns nULL just to abide by the pthread API.
 */
void *
query_log_loop(void *args)
{
    query_log_loop_args_t *ql_args = (query_log_loop_args_t *)args;

    config_t          *cfg                     = ql_args->cfg;
    channel_bss_t     *query_log_channels      = ql_args->query_log_channels;
    size_t             query_log_channel_count = ql_args->query_log_channel_count;
    channel_bss_msg_t *ch_msg;
    uint64_t           channel_msg_id_base = 0;

    bool   waiting;
    char  *buf;
    size_t buf_len;
    size_t data_written           = 0;
    size_t current_file_size      = 0;
    int    current_file_fd        = -1;
    char   err_msg[ERR_MSG_LENGTH];

    /* Initialize admin channel on this thread(core). */
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

    while (1)
    {
        /* Is the query log file open for write. */
        if (current_file_fd < 0) {
            current_file_fd = query_log_loop_openfile(cfg, err_msg, ERR_MSG_LENGTH);
            if (current_file_fd < 0) {
                /* Error opening file! */
                char              *log_err = strndup(err_msg, ERR_MSG_LENGTH);
                channel_log_msg_t *log_msg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, log_err, false);

                channel_log_send(ql_args->app_log_channel, log_msg);

                atomic_fetch_add(&ql_args->metrics->app.query_log_open_error, 1);
                usleep(QUERY_LOG_FILE_OPEN_RETRY_TIME);
                continue;
            } else {
                current_file_size = 0;
            }
        }

        /* Send channel message to each vectorloop to flip query log buffer
         * and log the data in previous active buffer.
         */
        data_written = 0;
        for (int i = 0; i < query_log_channel_count; i++) {
            /* Send flip message via channel. */
            ch_msg = channel_bss_msg_create(channel_bss_assign_msg_id(&channel_msg_id_base),
                                            CH_OP_QUERY_LOG_FLIP,
                                            NULL);
            channel_bss_send(&query_log_channels[i], ch_msg);

            /* Wait for response. */
            ch_msg = NULL;
            waiting = true;
            while (waiting) {
                /* check if there is message on channel. */
                if (channel_bssvl_recv(&query_log_channels[i], &ch_msg) == 1) {
                    buf = ch_msg->p;
                    buf_len = ch_msg->result;
                    ch_msg->p = NULL;
                    channel_bss_msg_release(ch_msg);
                    waiting = false;
                } else {
                    /* no message, wait some more. */
                    usleep(QUERY_LOG_LOOP_MSG_WAIT_TIME);
                }
            }

            /* Write data to disk. */
            if (buf_len != 0) {
                err_msg[0] = '\0';
                if (utl_writeall(current_file_fd, buf, buf_len,
                                 err_msg, ERR_MSG_LENGTH) != 0) {
                    /* Could not write data to file. */

                    debug_printf("Could not write data to file, %s", err_msg);
                    /* Close file and */
                    close(current_file_fd);
                    current_file_fd = -1;
                    break;
                }
            }

            /* Check if latest write puts us over max file size limit. */
            current_file_size += buf_len;
            data_written += buf_len;
            if (current_file_size >= ql_args->cfg->query_log_rotate_size) {
                /* Rotate file. */
                current_file_size = 0;
                close(current_file_fd);
                current_file_fd = -1;
                current_file_fd = query_log_loop_openfile(cfg, err_msg, ERR_MSG_LENGTH);
                if (current_file_fd < 0) {
                    /* Error opening file! */
                    char              *log_err = strndup(err_msg, ERR_MSG_LENGTH);
                    channel_log_msg_t *log_msg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, log_err, false);
                    
                    channel_log_send(ql_args->app_log_channel, log_msg);
                    
                    atomic_fetch_add(&ql_args->metrics->app.query_log_open_error, 1);
                    usleep(QUERY_LOG_FILE_OPEN_RETRY_TIME);
                    break;
                }
            }
        }

        /* If amount of data written is 0 slow down the loop a bit. */
        if (data_written == 0) {
            usleep(QUERY_LOG_LOOP_SLOWDOWN);
        }
    }
    
    return NULL;
}