/**
 * @file config.h
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
/** \defgroup config Configuration
 * 
 * @brief Configuration is stored in a single object which is then shared
 *        with threads. At startup, application initializes configuration object
 *        with default values, followed by parsing CLI options into
 *        configuration settings. CLI options override default settings.
 * 
 *        Default settings are stored as constants in @ref constants.h file.
 *        Not all settings have CLI option equivalent.
 *  @{
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "constants.h"

/** Structure describes application configuration object. */
typedef struct config_s {
    /** Flag to indicate if receiving DNS queries over UDP should be enabled. */
    bool udp_enable;

    /** UDP listener port. */
    uint16_t udp_listener_port;

    /** UDP socket receive buffer size. */
    size_t udp_socket_recvbuff_size;

    /** UDP socket send buffer size. */
    size_t udp_socket_sendbuff_size;

    /** Number of entries in UDP receive vector. This setting
     * is also used to size UDP write vector and UDP queries vector.
     */
    size_t udp_conn_vector_len;

    /** Flag to indicate if receiving DNS queries over TCP should be enabled. */
    bool tcp_enable;
 
    /** Maximum number of pending connections that TCP listener will allow to
     * be queued. When this max is reached new connection attempts are rejected.
     */
    int tcp_listener_pending_conns_max;

    /** TCP listener port. */
    uint16_t tcp_listener_port;

    /** Maximum number of active TCP connections per vectorloop.
     */
    size_t tcp_conns_per_vl_max;

    /** Maximum number of TCP connections that will be accepted per
     * vectorloop interation.
     */
    size_t tcp_listener_max_accept_new_conn;

    /** TCP socket receive buffer size. */
    size_t tcp_conn_socket_recvbuff_size;
    
    /** TCP socket send buffer size. */
    size_t tcp_conn_socket_sendbuff_size;

    /** Number of queries (in TCP conn read buffer) to parse and resolve
     * simultaneously.
     *
     * This also sets the tcp read buffer size as
     * tcp_conn_simultaneous_queries_count * (2 + dns_query_request_max_len).
     * 
     * This also sets the tcp write buffer size as
     * tcp_conn_simultaneous_queries_count * (2 + dns_query_response_max_len).
     */
    size_t tcp_conn_simultaneous_queries_count;

    /** Size of read buffer in connection object. This setting does not have
     * equivalent CLI option. Instead it is calculated based on  setting
     * "tcp_conn_simultaneous_queries_count" * (RIP_NS_PACKETSZ + 2). 
     */
    size_t tcp_readbuff_size;

    /** Size of write buffer in connection object. This setting does not have
     * equivalent CLI option. Instead it starts at
     * "tcp_conn_simultaneous_queries_count" * (RIP_NS_PACKETSZ + 2)
     * and increased in increments of RIP_NS_UDP_MAXMSG until it reaches max
     * size of RIP_NS_MAXMSG (64k).
     */
    size_t tcp_writebuff_size;

    /** Number of miliseconds TCP connection will stay open and IDLE after
     * receiving a query.
     */
    size_t tcp_keepalive;

    /**  Number of miliseconds TCP connection will wait to receive a full query.
     * This timeout applies when connection is first created, and also when
     * a subsequent partial query is received. 
    */
    size_t tcp_query_recv_timeout;

    /**  Number of miliseconds TCP connection will wait for socket to become
     * unblocked for write. This is basically total time we are willing to wait
     * for query response to be sent over TCP.
     */
    size_t tcp_query_send_timeout;

    /** Maximum number of TCP events epoll will return in a vectorloop iteration.*/
    int epoll_num_events_tcp;

    /** Maximum number of UDP events epoll will return in a vectorloop iteration.*/
    int epoll_num_events_udp;

    /** Number of vectorloop (DNS query processing) threads to start. */
    size_t process_thread_count;

    /** Array of vectorloop thread masks, one for each thread, and each thread
     * can be assigned to only one CPU.
    */
    size_t *process_thread_masks;

    /** Sleep time in microseconds for loop slowdown stage one. */
    size_t loop_slowdown_one;

    /** Sleep time in microseconds for loop slowdown stage two. */
    size_t loop_slowdown_two;

    /** Sleep time in microseconds for loop slowdown stage three. */
    size_t loop_slowdown_three;

    /** Name of resource 1. */
    char  *resource_1_name;

    /** Full file path for resource 1. */
    char  *resource_1_filepath;

    /** Frequency at which to check for updated resource 1. */
    size_t resource_1_update_freq;

    /** Name to use for application log.  */
    char *application_log_name;

    /** Path where to store application log. */
    char *application_log_path;

    /** Holds the realpath for application log. */
    char application_log_realpath[FILE_REALPATH_MAX];

    /** Query log buffer size. */
    size_t query_log_buffer_size;

    /** Query log base name. */
    char *query_log_base_name;

    /** Path on disk where to store query logs. */
    char *query_log_path;

    /** Query log directory realpath */
    char *query_log_realpath;

    /** Size at which to rotate query log. */
    size_t query_log_rotate_size;

} config_t;

void config_init(config_t *cfg);
int  config_parse_opts(config_t *cfg, int argc, char *argv[]);
void config_clean(config_t *cfg);

#endif /* End of CONFIG_H */

/** @}*/