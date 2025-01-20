/**
 * @file ripples.c
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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "log_app.h"
#include "channel.h"
#include "config.h"
#include "metrics.h"
#include "resource.h"
#include "utils.h"
#include "vectorloop.h"

/** This is the main process thread function for ripples application.
 * The reason this code is separated from function @ref main() is so unit tests
 * could be written for it.
 * 
 * @param argc Number of CLI arguments
 * @param argv Array of CLI arguments.
 * 
 * @return     Always returns 0.
 */
int
ripples(int argc, char *argv[])
{
    config_t      *cfg                = NULL;
    pthread_t     *pthreads           = NULL;
    channel_bss_t *resource_channels  = NULL;
    channel_log_t *app_log_channels   = NULL;
    channel_bss_t *query_log_channels = NULL;
    size_t         channels_count     = 0;

    metrics_t *metrics = malloc(sizeof(metrics_t));
    CHECK_MALLOC(metrics);
    *metrics = (metrics_t){ };

    /* Initialize configuration defaults. */
    cfg = malloc(sizeof(config_t));
    CHECK_MALLOC(cfg);
    config_init(cfg);

    /* Parse CLI options into configuration. */
    if (config_parse_opts(cfg, argc, argv) != 0) {
        /* Error to stderr was written by config_parse_opts(). */
        debug_print("Error parsing CLI options");
        exit(1);
    }

    /* Initialize channels. */
    channels_count    = cfg->process_thread_count;
    resource_channels = malloc(sizeof(channel_bss_t) * channels_count);
    CHECK_MALLOC(resource_channels);
    app_log_channels = malloc(sizeof(channel_log_t) * (channels_count + 3)); /* +3 for resource, app log & query log threads. */
    CHECK_MALLOC(app_log_channels);
    query_log_channels = malloc(sizeof(channel_bss_t) * channels_count);
    CHECK_MALLOC(query_log_channels);

    /* Resource and query channels. */
    for (int i = 0; i < channels_count; i++) {
        lfds711_queue_bss_init_valid_on_current_logical_core(&resource_channels[i].bss.qbsss,
            resource_channels[i].bss.qbsse, CHANNEL_BSS_QUEUE_LEN, NULL);
        lfds711_queue_bss_init_valid_on_current_logical_core(&resource_channels[i].vl.qbsss,
            resource_channels[i].bss.qbsse, CHANNEL_BSS_QUEUE_LEN, NULL);

        lfds711_queue_bss_init_valid_on_current_logical_core(&query_log_channels[i].bss.qbsss,
            query_log_channels[i].bss.qbsse, CHANNEL_BSS_QUEUE_LEN, NULL);
        lfds711_queue_bss_init_valid_on_current_logical_core(&query_log_channels[i].vl.qbsss,
            query_log_channels[i].bss.qbsse, CHANNEL_BSS_QUEUE_LEN, NULL);
    }

    /* App log channels. */
    for (int i = 0; i < (channels_count + 2); i++) {
        lfds711_queue_bss_init_valid_on_current_logical_core(&app_log_channels[i].qbsss,
            app_log_channels[i].qbsse, CHANNEL_LOG_QUEUE_LEN, NULL);
    }


    /* Initialize threads. */
    size_t pth_count = cfg->process_thread_count + 3;
    pthreads = malloc(sizeof(pthread_t) * pth_count);
    CHECK_MALLOC(pthreads);

    int pth_ret = 0;
    /* Start vectorloop threads. */
    for (int i = 0; i < cfg->process_thread_count; i++) {
        vectorloop_t *vl = vl_new(cfg, i, &resource_channels[i],
                                 &app_log_channels[i],  &query_log_channels[i],
                                 metrics);

        pth_ret = pthread_create(&pthreads[i], NULL, vl_run, vl);
        if (pth_ret != 0) {
            fprintf(stderr,"Could not start vectorloop thread no. %d, "
                    "error no: %d, error message: %s.",
                    pth_ret, i, strerror(pth_ret));
            exit(1);
        }
    }

    /* Start app log thread */
    app_log_loop_args_t app_log_args = {
        .cfg              = cfg,
        .app_log_channels = app_log_channels,
        .metrics          = metrics,
    };
    pth_ret = pthread_create(&pthreads[cfg->process_thread_count], NULL,
                             log_app_loop, &app_log_args);
    if (pth_ret != 0) {
        fprintf(stderr,"Could not start app log thread, error no: %d, "
                "error message: %s.", pth_ret, strerror(pth_ret));
        exit(1);
    }

    /* Start resource thread */
    resource_loop_args_t res_args = {
        .cfg = cfg,
        .resource_channels = resource_channels,
        .app_log_channel   = &app_log_channels[channels_count],
        .metrics           = metrics,
    };
    pth_ret = pthread_create(&pthreads[cfg->process_thread_count+1], NULL,
                             resource_loop, &res_args);
    if (pth_ret != 0) {
        fprintf(stderr,"Could not start resource thread, error no: %d, "
                "error message: %s.", pth_ret, strerror(pth_ret));
        exit(1);
    }

    /* Start query log thread */
    query_log_loop_args_t query_log_args = {
        .cfg = cfg,
        .metrics = metrics,
        .query_log_channels = query_log_channels,
        .query_log_channel_count = channels_count,
        .app_log_channel = &app_log_channels[channels_count+1],
    };
    pth_ret = pthread_create(&pthreads[cfg->process_thread_count+2], NULL,
                             query_log_loop, &query_log_args);
    if (pth_ret != 0) {
        fprintf(stderr,"Could not start query log thread, error no: %d, "
                "error message: %s.", pth_ret, strerror(pth_ret));
        exit(1);
    }

    /* Wait for threads to exit. */
    for (int i = 0; i < pth_count; i++) {
        pthread_join(pthreads[i], NULL);
    }

    return 0;
}