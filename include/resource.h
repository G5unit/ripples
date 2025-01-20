/**
 * @file resource.h
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
/** \defgroup resources Resources
 *
 * @brief Resources are data sets that application uses in its processing logic.
 *        For a DNS authoritative server this could be, amongst others, a GeoIP
 *        location DB, along with DNS records DB (zone data).
 * 
 *        Resources are loaded, and reloaded in a dedicated thread to avoid
 *        blocking and time consuming operations in DNS query processing
 *        threads. Once loaded into memory resources are treated as read only
 *        and shared amongst query processing threads. For data validity purpose
 *        each thread keeps a reference to the shared object. When a resource
 *        changes, processing thread is notified and updates the reference (pointer)
 *        to such a resource. This update is done once all pending queries have
 *        us been processed which ensures data validity (data does not change
 *        under in the middle of resolving a query).
 * 
 *        When all processing threads have updated their resource reference, the
 *        old resource data can safely be disposed of.
 * 
 *  @{
 */
#ifndef RESOURCE_H
#define RESOURCE_H

#include <time.h>

#include "channel.h"
#include "config.h"
#include "metrics.h"

typedef struct resource_s resource_t;

/** Function definition for custom function to be used when checking
 * if resource changed and loading the resource on change.
 * 
 * @param resource Resource object to check and update.
 * @param buf      Where to store pointer to loaded data.
 * @param buf_len  Size of buffer where new data was loaded into.
 * @param err      Where to store error message if error was encountered.
 * @param err_len  Length of err buffer.
 * 
 * @returns On success returns 0, otherwise an error occurred.
 * 
 */
typedef int (*resource_check_load_fn)(resource_t *resource,
                                      void **buf, size_t *buf_len,
                                      char *err, size_t err_len);

/** Function definition for custom function used to release resource data.
 * 
 * @param resource Resource object this resource data is for.
 * @param buf      Data to release.
 */
typedef void (*resource_release_fn)(resource_t *resource, void *buf);

/** Structure describes a resource which is a file on disk.
 * A resource is loaded at application start and used by other parts of the
 * application. Resource could be periodically checked for change and updated
 * resource data loaded.
 */
typedef struct resource_s {
    /** Name of resource. */
    char *name;

    /** Full path on disk for resource. */
    char *filepath;

    /** Frequency at which to check if resource on disk has changed. Unit is
     * seconds. Value of 0 means load resource at startup and then do not check
     * for updates.
     */
    size_t update_frequency;

    /** Channel OP for this resource. */
    channel_bss_ops_t channel_op;

    /** Create time for resource currently loaded into application. This
     * is compared to file on filesystem change time to detect if resource
     * has changed.
     */
    struct timespec create_time;

    /** Time to check if resource on disk has changed. This value is modified
     * after each check.
     */
    struct timespec next_update_time;

    /** Pointer to function that checks for resource change, loads and if
     * necessary transforms data into consumable object.
     */
    resource_check_load_fn check_load_fn;

    /** Pointer to function that releases the resource. */
    resource_release_fn release_fn;

    /** Pointer to current resource object. */
    void *current_resource;

    /** Pointer to incoming (updated) resource. */
    void *incoming_resource;

} resource_t;

/** Structure holds arguments passed to @ref resource_loop function.
 * Since resource_loop function is started by pthread it can only receive one
 * argument hence placing all arguments into one structure.
 */
typedef struct resource_loop_args_s {
    /** Configuration settings to use. */
    config_t *cfg;

    /** Resource channels used to communicate with Vectorloop threads. */
    channel_bss_t *resource_channels;

    /** Application log channel. */
    channel_log_t *app_log_channel;

    /** Pointer to metrics object where to report metrics. */
    metrics_t *metrics;
} resource_loop_args_t;


void * resource_loop(void *args);


void resource_release_raw_file(resource_t *resource, void *buf);
int  resource_check_load_raw_file(resource_t *resource, void **buf, size_t *buf_len,
                                  char *err, size_t err_len);

#endif /* RESOURCE_H */

/** @}*/