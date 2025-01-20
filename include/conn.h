/**
 * @file conn.h
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
/** \defgroup connection Connection
 * 
 * @brief Connection object holds information about TCP and UDP connections.
 *        There is a connection  object for each of the following:
 *        - UDP listener
 *        - TCP listener
 *        - TCP connection.
 * 
 *        TCP (established) connections are tracked in a Least Recently Used
 *        cache. LRU cache is used to check for various timeout conditions.
 *        Each TCP connection has a unique Id (connection ID) associated with it.
 *        The only purpose of this ID is to provide a unique key for LRU cache.
 *        LRU cache implementation uses UT HASH.
 * 
 *        Listener connections are not tracked in LRU cache.
 * 
 *        UDP listener and TCP connection objects have DNS query objects
 *        associated with them.
 *  @{
 */
#ifndef CONN_H
#define CONN_H

#include <stdint.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <time.h>

#include <uthash/uthash.h>

#include "query.h"

/** Marco that returns true if conn (conn_t struct) is a UDP listener */
#define CONN_IS_UDP_LISTENER(conn) \
    conn->lc == 0 && conn->proto == 0

/** Marco that returns true if conn (conn_t struct) is a TCP listener */
#define CONN_IS_TCP_LISTENER(conn) \
    conn->lc == 0 && conn->proto == 1

/** Marco that returns true if conn (conn_t struct) is a TCP connection */
#define CONN_IS_TCP_CONN(conn) \
    conn->lc == 1 && conn->proto == 1

/** Enumerated error returned by listener_start() function. */
typedef enum listener_start_error_e {
    /** Socket create error. */
    LISTENER_ERR_SOCKET                      = -1,

    /** Error setting socket option SO_REUSEADDR. */
    LISTENER_ERR_SOCKET_OPT_REUSEADDR        = -2,
     
    /** Error setting socket option SO_REUSEPORT. */
    LISTENER_ERR_SOCKET_OPT_REUSEPORT        = -3,
     
    /** Error binding socket. */
    LISTENER_ERR_BIND                        = -4,
     
    /** Error calling listen() on socket, applies to TCP only. */
    LISTENER_ERR_LISTEN                      = -5,
     
    /** Error setting socket option IP_PKTINFO. */
    LISTENER_ERR_SOCKET_OPT_IP_PKTINFO       = -6,
     
    /** Error setting socket option IPV6_V6ONLY. */
    LISTENER_ERR_SOCKET_OPT_IPV6_V6ONLY      = -7,
     
    /** Error setting socket option IPV6_RECVPKTINFO. */
    LISTENER_ERR_SOCKET_OPT_IPV6_RECVPKTINFO = -8,

    /** Error setting socket option IPV6_RECVPKTINFO. */
    LISTENER_ERR_SOCKET_OPT_RCVBUF           = -9,

    /** Error setting socket option IPV6_RECVPKTINFO. */
    LISTENER_ERR_SOCKET_OPT_SNDBUF           = -10,
} listener_start_error_t;

/** Enumerated TCP connection states. */
typedef enum conn_tcp_state_e {
    /*8 Error assigning connection ID. */
    TCP_CONN_ST_ASSIGN_CONN_ID_ERR = 0,

    /** Wait for query request (not first one). */
    TCP_CONN_ST_WAIT_FOR_QUERY,

    /* Wait for query data, meaning some data was read in yet not enough to be
     * a complete query request.
     */
    TCP_CONN_ST_WAIT_FOR_QUERY_DATA,

    /** Wait for epoll notification for socket to become "writable". */
    TCP_CONN_ST_WAIT_FOR_WRITE,

    /** TCP connection is closed for read (by far end), also known as half 
     * close.
     */
    TCP_CONN_ST_CLOSED_FOR_READ,

    /** There was an error reading from connection socket.
     * Socket is closed and no further reads nor writes are done.
     */
    TCP_CONN_ST_READ_ERR,

    /** TCP connection is was closed for write.*/
    TCP_CONN_ST_CLOSED_FOR_WRITE,

    /** There was an error writing to connection socket.
     * Socket is closed and no further reads nor writes are done.
     */
    TCP_CONN_ST_WRITE_ERR,

    /** Query size received over TCP connection exceeds RIP_NS_PACKETSZ. */
    TCP_CONN_ST_QUERY_SIZE_TOOLARGE,

} conn_tcp_state_t;

/** Structure holds data specific to TCP listener connection. */
typedef struct conn_tcp_s {   
    /** TCP connection client IP. */
    struct sockaddr_storage client_ip;

    /** TCP connection local IP. */
    struct sockaddr_storage local_ip;

    /** Read buffer. */
    unsigned char *read_buffer;

    /** Read buffer size. */
    size_t read_buffer_size;
 
    /** Length of data in read buffer. */
    size_t read_buffer_len;

    /** Element in query array to start write from. */
    size_t query_write_index;

    /** Index in query element write buffer where to start write from.
     * This is used in case multiple write() calls are needed to write all data
     * in buffer.
     */
    size_t write_index;

    /** Array where queries are parsed into. */
    query_t *queries;

    /** Number of elements in queries array. */
    size_t queries_size;
 
    /** Number of populated elements in queries array. */
    size_t queries_count;

    /** Number of queries received and processed over this TCP connection. */
    size_t queries_total_count;

    /** TCP keepalive as advertised in EDNS tcp-keepalive. */
    size_t tcp_keepalive;

    /** State of this TCP connection. */
    conn_tcp_state_t state;

    /** Time TCP connection was established. */
    struct timespec start_time;

    /** Time when this connection will time out. Which timeout is in effect
     * is governed by connection state.
     */
    struct timespec timeout;

    /** Time TCP connection was established. */
    struct timespec end_time;
} conn_tcp_t;

/** Structure holds data specific to UDP listener connection. */
typedef struct conn_udp_s {
    /** Size (length) of arrays: read_vector, queries, write_vector.
     *
     * @note Max is UIO_MAXIOV (1024), limited by Linux OS.
     */
    unsigned int vector_len;

    /** Vector (array) of mmsg to read data into.  */
    struct mmsghdr *read_vector;

    /** Number of active entries in read vector. */
    unsigned int read_vector_count;

    /** Array where queries are parsed into. */
    query_t *queries;

    /** Vector (array) of mmsg to send data from. This is also vector where
     * query responses are packed into.
     */
    struct mmsghdr *write_vector;

    /** Index where to start writing (sending UDP datagrams) from. At first this
     * value is 0, then if not all data was written via call to sendmmsg(), 
     * it is updated and connection is queued back into udp write queue
     * so another call to sendmmsg() is made this time with write_vector
     * starting at index recorded here. This is repeated until all messages
     * are sent (put on wire).
     */
    unsigned int write_vector_write_index;

    /** Number of active entries in write vector starting with
     * write_vector_write_index (see above comment).
     */
    unsigned int write_vector_count;

} conn_udp_t;

/** Structure holds data common to TCP and UDP connections. */
typedef struct conn_s {
    /** @private LRU cache handle. Only used for TCP connections. */
    UT_hash_handle hh;

    /** Connection ID. While it is set for all connections, it is only used for
     * TCP connections.
    */
    uint64_t cid;

    /** Flag indicating if 0=listener, or 1=connection(only applies to TCP). */
    uint8_t lc: 1;

    /** Flag indicating protocol: 0=UDP, 1=TCP. */
    uint8_t proto: 1;

    /** Flag indicating if this conn is:  0=IPv4, 1=IPv6. */
    uint8_t ip_version: 1;

    /** Flag indicating if connection object is waiting fore socket to become
     * readable.
     */
    uint8_t waiting_for_read: 1;

    /** Flag indicating if connection object is waiting fore socket to become
     * writable.
     */
    uint8_t waiting_for_write: 1;

    /** Flag indicating if connection object is in read queue. */
    uint8_t in_read_queue: 1;

    /** Flag indicating if connection object is in write queue. */
    uint8_t in_write_queue: 1;

    /** Flag indicating if connection object is in release queue. Only applies
     * to TCP connections.
     */
    uint8_t in_release_queue: 1;

    /** @private handle for read queue. */
    struct conn_s *read_q_handle;

    /** @private handle for write queue. */
    struct conn_s *write_q_handle;

    /** @private handle for general queues: parse, resolve, log. */
    struct conn_s *gen_q_handle;

    /** Socket descriptor this connection is associated with. */
    int fd;

    /** Holds either TCP or UDP specific data depending on connection protocol. */
    union {
        conn_udp_t *udp;
        conn_tcp_t *tcp;
    } conn;

} conn_t;

/** Connection object FIFO queue. New objects are enqueued (added) to the tail
 *  of the queue, and objects are dequeued (removed) from head of the queue.
 */
typedef struct conn_fifo_queue_s {
   /** Pointer to queue head. */
    conn_t *head;

    /** Pointer to queue tail. */
    conn_t *tail;

} conn_fifo_queue_t;


void     conn_fifo_enqueue_read(conn_fifo_queue_t *queue, conn_t *entry);
conn_t * conn_fifo_dequeue_read(conn_fifo_queue_t *queue);
void     conn_fifo_enqueue_write(conn_fifo_queue_t *queue, conn_t *entry);
conn_t * conn_fifo_dequeue_write(conn_fifo_queue_t *queue);
void     conn_fifo_enqueue_gen(conn_fifo_queue_t *queue, conn_t *conn);
conn_t * conn_fifo_dequeue_gen(conn_fifo_queue_t *queue);
void     conn_fifo_remove_from_read_queue(conn_fifo_queue_t *queue, conn_t *conn_rm);
void     conn_fifo_remove_from_write_queue(conn_fifo_queue_t *queue, conn_t *conn_rm);
void     conn_fifo_enqueue_release(conn_fifo_queue_t *queue, conn_t *conn);
conn_t * conn_fifo_dequeue_release(conn_fifo_queue_t *queue);

void         conn_tcp_release(conn_tcp_t *conn_tcp);
void         conn_udp_release(conn_udp_t *conn_udp);
void         conn_release(conn_t *conn);
void         conn_udp_vectors_reset(conn_udp_t *conn_udp);
conn_udp_t * conn_udp_new(config_t *cfg, int family);
conn_t     * conn_new_tcp(int fd, config_t *cfg, int ip_version,
                          struct sockaddr_storage *client_ip,
                          struct sockaddr_storage *local_ip);

conn_t * conn_listener_provision(config_t *cfg, int family, int protocol,
                                 char *err_buf, size_t err_buf_len);

conn_t * conn_lru_cache_get(conn_t **lru, uint64_t id);

bool conn_tcp_id_assign(uint64_t *id, conn_t **lru, uint64_t *base);

void conn_tcp_report_metrics(conn_tcp_t *conn_tcp, metrics_t *metrics);

#endif /* End of CONN_H */

/** @}*/
