/**
 * @file resource_loop.c
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
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "resource.h"
#include "channel.h"
#include "config.h"
#include "constants.h"
#include "utils.h"


/** Resource loop function states. */
enum resource_loop_states {
    /* Resource Loop State: Check if resource has changed.*/
    CHECK_RESOURCE = 0,

    /* Resource Loop State: Get next resource to check. */
    GET_NEXT_RESOURCE,

    /* Resource Loop State: Wait for all Vectorloop threads to signal they have
     * processed the resource change.
     */
    WAIT_FOR_RESOURCE_UPDATE,
};

/** Function checks if all vectorloop threads have signaled the resource thread
 * that they processed the latest resource change they were notified of.
 * 
 * @note This is a helper function for @ref resource_loop().
 * 
 * @param resource_channels   Array of resource channels.
 * @param vl_notifications Array of vector loop notifications.
 * @param count            Size of arrays. They MUST both match in size.
 * 
 * @return                 Returns true if all vectorloop threads have processed
 *                         latest resource update, otherwise returns false.
 */
static bool
resource_vls_notified(channel_bss_t *resource_channels, bool *vl_notifications, size_t count)
{
    channel_bss_msg_t *ch_msg = NULL;

    for (int i = 0; i < count; i++) {
        if (vl_notifications[i] == false) {
            if (channel_bssvl_recv(&resource_channels[i], &ch_msg) == 1) {
                vl_notifications[i] = true;
                ch_msg->p = NULL;
                channel_bss_msg_release(ch_msg);
            } else {
                /* Notification from vectorloop not received yet. */
                return false;
            }
        }
    }
    return true;
}


/** Resource loop function. It periodically checks resources for change. On
 * resource change it loads the new resource into memory and notifies each
 * vectorloop thread that resource changed.
 * 
 * @note This loop runs indefinitely and is meant to be run from the main()
 * function, not on vectorloop thread, or any other tread.
 * 
 * @param args Object with arguments passed to resource loop.
 */
void *
resource_loop(void *args)
{
    resource_loop_args_t   *res_args          = (resource_loop_args_t *)args;
    config_t               *cfg               = res_args->cfg;
    channel_bss_t          *resource_channels = res_args->resource_channels;
    channel_log_t          *app_log_channel   = res_args->app_log_channel;
    metrics_t              *metrics           = res_args->metrics;

    struct timespec         current_time;
    struct timespec         wait_time;
    double                  delta_time;
    double                  top_delta_time    = RESOURCE_LOOP_TOP_DELTA_TIME;
    int                     next_res_index    = 0;
    resource_check_load_fn  check_load;

    /* Array of all resources that are updated by this resource loop. */
    resource_t resources[RESOURCE_COUNT];

    /* Buffer passed to check_load functions for purpose of logging any error(s)
     * encountered. */
    char    err[ERR_MSG_LENGTH];

    int     res_count         = RESOURCE_COUNT;
    void   *new_resource      = NULL;
    size_t  new_resource_len  = 0;
    int     vl_count          = cfg->process_thread_count;

    /* Array that holds VL notifications for resource update acknowledgments. */
    bool               vl_notifications[vl_count];

    channel_bss_msg_t *ch_msg;
    uint64_t           channel_msg_id_base = 0;
    int                ret                 = 0;

    /* used to track how long resource loop waits for all notifications from VL
     * threads to be received. */
    size_t vectorloop_update_wait_time = 0;

    /* Starting state of resource loop. */
    enum resource_loop_states state = CHECK_RESOURCE;

    /* Initialize channels on this thread(core). */
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

    /* Initialize resources. */
    resources[0] = (resource_t) {};
    resources[0].name = cfg->resource_1_name;
    resources[0].filepath = cfg->resource_1_filepath;
    resources[0].channel_op = CH_OP_RES_SET_RESOURCE1;
    resources[0].update_frequency = 5;
    resources[0].next_update_time.tv_sec = 0;
    resources[0].next_update_time.tv_nsec = 0;
    resources[0].check_load_fn = &resource_check_load_raw_file;
    resources[0].release_fn = &resource_release_raw_file;
    resources[0].current_resource = NULL;
    resources[0].incoming_resource = NULL;

    /* Check for updates and load updates when change in resource detected. */
    while (1) {
        switch (state) {
        case CHECK_RESOURCE:
            /* Call check_load. */
            check_load = resources[next_res_index].check_load_fn;
            memset(err, '\0', ERR_MSG_LENGTH);
            ret = check_load(&resources[next_res_index], &new_resource, &new_resource_len,
                             err, ERR_MSG_LENGTH);
            debug_printf("resource index %d, check_load returned %d", next_res_index, ret);

            if (ret == 1) {
                /* Update present so notify vectorloops via channels and
                 * wait for all notifications to return.
                 */
                resources[next_res_index].incoming_resource = new_resource;
                for (int i = 0; i < vl_count; i++) {
                    vl_notifications[i] = false;
                    ch_msg = channel_bss_msg_create(channel_bss_assign_msg_id(&channel_msg_id_base),
                                                    resources[next_res_index].channel_op,
                                                    new_resource);
                    channel_bss_send(&resource_channels[i], ch_msg);
                    debug_print("channel message sent from resource loop to vector loop");
                }
                vectorloop_update_wait_time = 0;
                state = WAIT_FOR_RESOURCE_UPDATE;
            } else if (ret == 0) {
                /* Resource has not changed, move to state GET_NEXT_RESOURCE */
                state = GET_NEXT_RESOURCE;
            } else {
                /* Error reading in file. Log and try again later on. Keep using
                 * the old file.
                 */
                char *err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
                CHECK_MALLOC(err_str);
                snprintf(err_str, ERR_MSG_LENGTH, "Error opening resource file \"%s\", %s",
                         resources[next_res_index].filepath, err);
                channel_log_msg_t *cmsg = channel_log_msg_create(0, err_str, false);
                channel_log_send(app_log_channel, cmsg);

                atomic_fetch_add(&metrics->app.resource_reload_error, 1);

                state = GET_NEXT_RESOURCE;
            }

            /* Update next check time for this resource. */
            utl_clock_gettime_rt_fatal(&resources[next_res_index].next_update_time);
            resources[next_res_index].next_update_time.tv_sec += 
                                resources[next_res_index].update_frequency;             
            break;

        case WAIT_FOR_RESOURCE_UPDATE:
            /* Check if all vl loops returned notifications */
            if (resource_vls_notified(resource_channels, vl_notifications, vl_count) == true) {
                /* All Vectorloops updated. */
                resources[next_res_index].release_fn(&resources[next_res_index],
                                                     resources[next_res_index].current_resource);
                resources[next_res_index].current_resource = resources[0].incoming_resource;
                resources[next_res_index].incoming_resource = NULL;
                state = GET_NEXT_RESOURCE;
            } else {
                /* Not all vectorloops updated, wait 1us and check again.
                 * We wait for 10 seconds (const VL_RESOURCE_NOTIFY_WAIT_TIME_MAX)
                 * and if not all vectorloops have responded message is logged and
                 * application exits!
                 * 
                 * If a vectorloop thread takes more than
                 * 10s to run a single loop it means that something is wrong and it
                 * is blocked. There SHOULD be no blocking operations in vectorloop.
                 */
                if (vectorloop_update_wait_time > VL_RESOURCE_NOTIFY_WAIT_TIME_MAX) {
                    char *err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
                    CHECK_MALLOC(err_str);
                    snprintf(err_str, ERR_MSG_LENGTH,
                             "Vectorloop resource update timed out (10s) for resource \"%s\"",
                             resources[next_res_index].filepath);
                    channel_log_msg_t *cmsg = channel_log_msg_create(0, err_str, true);
                    channel_log_send(app_log_channel, cmsg);
                }
                wait_time.tv_sec = 0;
                wait_time.tv_nsec = 1000;
                clock_nanosleep(CLOCK_REALTIME, 0, &wait_time, NULL);
            }
            break;

        case GET_NEXT_RESOURCE:
            top_delta_time = RESOURCE_LOOP_TOP_DELTA_TIME;
            /* Get current clock time. */
            utl_clock_gettime_rt_fatal(&current_time);

            /* Find index and delta time of next resource to be checked for updates. */
            for (int i = 0; i < res_count; i++) {
                delta_time = utl_diff_timespec_as_double(&current_time,
                                                        &resources[i].next_update_time);
                if (delta_time > top_delta_time) {
                    top_delta_time = delta_time;
                    next_res_index = i;
                }
                if (delta_time < 0) {
                    utl_diff_timespec(&wait_time,
                                      &resources[i].next_update_time, &current_time);
                }
            }
            if (top_delta_time < 0) {
                /* Wait until it is time for resource at next_res_index to be checked. */
                clock_nanosleep(CLOCK_REALTIME, 0, &wait_time, NULL);
            }
            state = CHECK_RESOURCE;
            break;

        default:
            /* Coding error. */
            assert(0);
        }
    }

    return NULL;
}