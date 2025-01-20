/**
 * @file vectorloop.h
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
/** \defgroup vectorloop Vectorloop
 *
 * @brief Vectorloop is function that processes queries. It runs in a dedicated
 *        thread. Multiple vectorloop threads could be started (configurable).
 * 
 *        Vectorloop is designed to process data as sets (vectors) hence
 *        optimizing CPU cache hits and minimizing context switching.
 *  @{
 */
#ifndef VECTORLOOP_H
#define VECTORLOOP_H

#include <stdint.h>
#include <sys/epoll.h>
#include <time.h>

// Debug
#include <stdio.h>
//EofDebug


#include "channel.h"
#include "constants.h"
#include "conn.h"
#include "metrics.h"
#include "query.h"


/** Structure represents a VectorLoop. */
typedef struct vectorloop_s
{
    /** Configuration */
    config_t *cfg;

    /** Vectorloop ID, should be unique per application thread. */
    uint16_t id;

    /** Resource channel. */
    channel_bss_t *resource_channel;

    /** Application log  channel. */
    channel_log_t *app_log_channel;

    /** Application log  channel. */
    channel_bss_t *query_log_channel;

    /** Metrics object where to report statistics. */
    metrics_t *metrics;

    /** Loop timestamp, taken each iteration and used to check for timeouts
     * in LRU cache.
     */
    struct timespec loop_timestamp;

    /** epoll file descriptor for UDP connections. */
    int ep_fd_udp;

    /** epoll file descriptor for TCP connections. */
    int ep_fd_tcp;

    /** Number of active TCP connections this vectorloop has.
     * This count includes both IPv4 and IPv6.
     */
    uint64_t conns_tcp_active;

    /** Array of epoll events submitted to epoll_wait(). */
    struct epoll_event *ep_events;

    /** Pointer to UDP IPv4 listener connection. */
    conn_t *listener_udp_ipv4;

    /** Pointer to UDP IPv6 listener connection. */
    conn_t *listener_udp_ipv6;

    /** Pointer to TCP IPv4 listener connection. */
    conn_t *listener_tcp_ipv4;

    /** Pointer to TCP IPv6 listener connection. */
    conn_t *listener_tcp_ipv6;

    /** TCP connection LRU cache. */
    conn_t *conn_tcp_lru_cache;

    /** TCP connection base ID. */
    uint64_t conn_tcp_id_base;

    /** Read queue */
    conn_fifo_queue_t conn_udp_read_queue;

    /** Write queue */
    conn_fifo_queue_t conn_udp_write_queue;

    /** TCP accept connections queue */
    conn_fifo_queue_t conn_tcp_accept_conns_queue;

    /** TCP connections read queue */
    conn_fifo_queue_t conn_tcp_read_queue;

    /** TCP connections write queue */
    conn_fifo_queue_t conn_tcp_write_queue;

    /** TCP connections release queue */
    conn_fifo_queue_t conn_tcp_release_queue;

    /** Parse DNS queries queue */
    conn_fifo_queue_t query_parse_queue;

    /** Resolve DNS queries queue */
    conn_fifo_queue_t query_resolve_queue;

     /** Pack DNS queries response queue */
    conn_fifo_queue_t query_response_pack_queue;

    /** Log DNS queries queue */
    conn_fifo_queue_t query_log_queue;

    /** Query log object */
    query_log_t query_log;

    /** Counter used to track when loop was idle (processed nothing) so the
     * loop could slow it self down and not burn CPU cycles needlessly.
     */
    uint32_t idle_count;
} vectorloop_t;

vectorloop_t * vl_new(config_t *cfg, int id, channel_bss_t *res_ch,
                      channel_log_t *app_log_channel,
                      channel_bss_t *query_log_channel,
                      metrics_t *metrics);
void         * vl_run(void *arg);

#endif /* End of VECTORLOOP_H */

/** @}*/