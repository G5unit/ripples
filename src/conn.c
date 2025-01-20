/**
 * @file conn.c
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
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "conn.h"
#include "constants.h"
#include "lru_cache.h"
#include "query.h"
#include "utils.h"

/** Function returns string matching a listener error enumerated by
 * @ref listener_start_error_t.
 * 
 * @param err ID to return string message for.
 * 
 * @return    String matching error id. String returned is a constant.
 */
static char *
listener_err_to_str(int err)
{
    switch (err) {
    case -1:
        return "Error creating listener socket";
        break;

    case -2:
        return "Error setting socket option SO_REUSEADDR";
        break;

    case -3:
        return "Error setting socket option SO_REUSEPORT";
        break;
        
    case -4:
        return "Error binding address to socket";
        break;
        
    case -5:
        return "Error starting listener on socket";
        break;

    case -6:
        return "Error setting socket option IP_PKTINFO";
        break;

    case -7:
        return "Error setting socket option IPV6_V6ONLY";
        break;

    case -8:
        return "Error setting socket option IPV6_RECVPKTINFO";
        break;

    case -9:
        return "Error setting socket option SO_RCVBUF";
        break;

    case -10:
        return "Error setting socket option SO_SNDBUF";
        break;  

    default:
        return "Unknown";
    }

    return "Unknown";
}

/** Free memory associated with conn_tcp_t object and free object it self.
 * 
 * @param conn_tcp TCP connection object to release.
 */
void
conn_tcp_release(conn_tcp_t *conn_tcp)
{
    free(conn_tcp->read_buffer);
    for (int i = 0; i < conn_tcp->queries_size; i++) {
        query_clean(&conn_tcp->queries[i]);
    }
    free(conn_tcp->queries);
    free(conn_tcp);
}

/** Free memory associated with conn_udp_t object and free object it self.
 * 
 * @param conn_udp UDP connection object to release.
 */
void
conn_udp_release(conn_udp_t *conn_udp)
{
    for (int i = 0; i < conn_udp->read_vector_count; i++) {
        struct msghdr *mh;

        /* Read vector. */
        mh = &conn_udp->read_vector[i].msg_hdr;
        free(mh->msg_iov);
        free(mh->msg_name);
        free(mh->msg_control);
        /* Query. */
        query_clean(&conn_udp->queries[i]);
        /* Write vector. */
        mh = &conn_udp->write_vector[i].msg_hdr;
        free(mh->msg_iov);
    }
    free(conn_udp->read_vector);
    free(conn_udp->queries);
    free(conn_udp->write_vector);
    free(conn_udp);
}

/** Free memory associated with conn_t object and free object it self.
 * 
 * @param conn Connection object to release.
 */
void
conn_release(conn_t *conn)
{
    if (conn->fd >= 0) {
        close(conn->fd);
        conn->fd = -1;
    }
    
    if (conn->proto == 0) {
        /* UDP */
        conn_udp_release(conn->conn.udp);
    } else {
        /* TCP */
        if (conn->conn.tcp != NULL) {
            /* Not a TCP listener so it will have a conn.tcp section. */
            conn_tcp_release(conn->conn.tcp);
        }
        
    }
    free(conn);
}

/** Create a new TCP connection object for an established TCP connection.
 * 
 * @param fd         Socket for TCP connection.
 * @param cfg        Application configuration to get various settings from.
 * @param ip_version IP version this connection is for, 0=IPv4, 1=IPv6.
 * @param client_ip  Client IP address.
 * @param local_ip   Local IP address.
 * 
 * @return           Returns a newly allocated and initialized TCP connection
 *                   object. 
 */
conn_t *
conn_new_tcp(int fd, config_t *cfg, int ip_version,
             struct sockaddr_storage *client_ip,
             struct sockaddr_storage *local_ip)
{
    conn_t     *conn     = NULL;
    conn_tcp_t *conn_tcp = NULL;

    socklen_t socklen = sizeof(struct sockaddr_in);


    if (ip_version == 1) {
        socklen = sizeof(struct sockaddr_in6);
    }

    conn_tcp = malloc(sizeof(conn_tcp_t));
    CHECK_MALLOC(conn_tcp);

    *conn_tcp = (conn_tcp_t) { };
    memcpy(&conn_tcp->client_ip, client_ip, socklen);
    memcpy(&conn_tcp->local_ip, local_ip, socklen);

    conn_tcp->read_buffer = malloc(sizeof(unsigned char) * cfg->tcp_readbuff_size);
    CHECK_MALLOC(conn_tcp->read_buffer);
    conn_tcp->read_buffer_size = cfg->tcp_readbuff_size;

    conn_tcp->queries = malloc(sizeof(query_t) * cfg->tcp_conn_simultaneous_queries_count);
    CHECK_MALLOC(conn_tcp->queries);
    conn_tcp->queries_size = cfg->tcp_conn_simultaneous_queries_count;
    for (int i = 0; i < conn_tcp->queries_size; i++) {
        query_init(&conn_tcp->queries[i], cfg, 1);
    }

    conn = malloc(sizeof(conn_t));
    CHECK_MALLOC(conn);
    *conn = (conn_t) {
        .lc         = 1,
        .proto      = 1,
        .ip_version = ip_version,
        .conn.tcp   = conn_tcp,
        .fd         = fd,
    };

    return conn;
}

/** Create a new UDP connection object
 *
 * @param cfg    Application configuration to get various settings from.
 * @param family IP family: AF_INET or AF_INET6.
 * 
 * @return       Returns a newly allocated and initialized UDP connection
 *               object.
 */
conn_udp_t *
conn_udp_new(config_t *cfg, int family)
{
    conn_udp_t *conn_udp = NULL;

    /* Allocate connection UDP object. */
    conn_udp = malloc(sizeof(conn_udp_t));
    CHECK_MALLOC(conn_udp);

    /* Initialize connection UDP object. */
    *conn_udp = (conn_udp_t) {
        .vector_len = cfg->udp_conn_vector_len,
    };

    /* Allocate read & write vectors. */
    conn_udp->read_vector = malloc(sizeof(struct mmsghdr) * cfg->udp_conn_vector_len);
    CHECK_MALLOC(conn_udp->read_vector);
    conn_udp->queries = malloc(sizeof(query_t) * cfg->udp_conn_vector_len);
    CHECK_MALLOC(conn_udp->queries);
    conn_udp->write_vector = malloc(sizeof(struct mmsghdr) * cfg->udp_conn_vector_len);
    CHECK_MALLOC(conn_udp->write_vector);

    for (int i = 0; i < cfg->udp_conn_vector_len; i++) {
        /* Initialize query structures. */
        query_init(&conn_udp->queries[i], cfg, 0);

        /* Allocate read vector structures. */
        struct msghdr *mh = &conn_udp->read_vector[i].msg_hdr;

        struct iovec *iov = malloc(sizeof(struct iovec));
        CHECK_MALLOC(iov);
        iov->iov_base = conn_udp->queries[i].request_buffer;
        iov->iov_len = conn_udp->queries[i].request_buffer_size;;
        mh->msg_iov = iov;
        mh->msg_iovlen = 1;

        mh->msg_name = malloc(sizeof(struct sockaddr_storage));
        CHECK_MALLOC(mh->msg_name);
        mh->msg_namelen = sizeof(struct sockaddr_storage);

        if (family == AF_INET) {
            mh->msg_control = malloc(sizeof(unsigned char) * UDP_MSG_CONTROL_LEN);
            CHECK_MALLOC(mh->msg_control);
            mh->msg_controllen = UDP_MSG_CONTROL_LEN;
        } else {
            mh->msg_control = malloc(sizeof(unsigned char) * UDP_MSG_CONTROL_LEN);
            CHECK_MALLOC(mh->msg_control);
            mh->msg_controllen = UDP_MSG_CONTROL_LEN;
        }


        /* Allocate write vector structures. */
        mh = &conn_udp->write_vector[i].msg_hdr;

        iov = malloc(sizeof(struct iovec));
        CHECK_MALLOC(iov);
        mh->msg_iov = iov;
        mh->msg_iovlen = 1;
    }

    return conn_udp;
}

/** Reset UDP connection vectors so the can be reused for next iteration of
 * read/resolve/send of DNS queries.
 * 
 * @param conn_udp UDP connection to reset vectors for.
 */
void
conn_udp_vectors_reset(conn_udp_t *conn_udp)
{
    for (int i = 0; i < conn_udp->vector_len; i++) {
        conn_udp->read_vector[i].msg_hdr.msg_controllen = UDP_MSG_CONTROL_LEN;
        conn_udp->read_vector[i].msg_hdr.msg_namelen    = sizeof(struct sockaddr_storage);
        conn_udp->read_vector_count                     = 0;
        conn_udp->write_vector_count                    = 0;
        conn_udp->write_vector_write_index              = 0;
        query_reset(&conn_udp->queries[i]);
    }
}

/** Start a TCP or UDP DNS listener.
 * 
 * Starts a listener for given IP family and protocol. Socket is created and
 * socket options set. If protocol is TCP listen() is called to start listening
 * for new TCP connections on socket.
 * 
 * @param cfg      Configuration object that has settings:
 *                 - tcp_listener_port,
 *                 - tcp_writebuff_size,
 *                 - tcp_listener_port,
 *                 -tcp_listener_pending_conns_max,
 *                 - udp_socket_recvbuff_size,
 *                 - udp_socket_sendbuff_size,
 *                 - udp_listener_port.
 * @param family   IP family to start a listener for, valid options are:
 *                 - AF_INET,
 *                 - AF_INET6.
 * @param protocol Protocol to start listener for, valid options are:
 *                 - IPPROTO_TCP,
 *                 - IPPROTO_UDP.
 * @param err_no   Where to store value of errno if error was encountered.
 * 
 * @return         On success returns a socket descriptor for started listener.
 *                 On error returns negative number representing error. Errors
 *                 are of enum type listener_start_error_t:
 *                 typedef enum listener_start_error_e {
 *                     LISTENER_ERR_SOCKET                      = -1,
 *                     LISTENER_ERR_SOCKET_OPT_REUSEADDR        = -2,
 *                     LISTENER_ERR_SOCKET_OPT_REUSEPORT        = -3,
 *                     LISTENER_ERR_BIND                        = -4,
 *                     LISTENER_ERR_LISTEN                      = -5,
 *                     LISTENER_ERR_MALLOC                      = -6,
 *                     LISTENER_ERR_SOCKET_OPT_IP_PKTINFO       = -7,
 *                     LISTENER_ERR_SOCKET_OPT_IPV6_V6ONLY      = -8,
 *                     LISTENER_ERR_SOCKET_OPT_IPV6_RECVPKTINFO = -9,
 *                 } listener_start_error_t;
 */
static int
listener_start(config_t *cfg, int family, int protocol, int *err_no)
{
    int ret           = 0;
    int fd            = -1;
    int opt           = 1;
    int socket_type   = 0;
    int socket_rcvbuf = 0;
    int socket_sndbuf = 0;
    uint16_t port     = 0;

    struct sockaddr_storage  ss = {};
    struct sockaddr_in      *sin  = (struct sockaddr_in *)&ss;
    struct sockaddr_in6     *sin6 = (struct sockaddr_in6 *)&ss;

    if (protocol == IPPROTO_TCP) {
        socket_type = SOCK_STREAM | SOCK_NONBLOCK;
        socket_rcvbuf = cfg->tcp_readbuff_size;
        socket_sndbuf = cfg->tcp_writebuff_size;
        port          = cfg->tcp_listener_port;
    } else if (protocol == IPPROTO_UDP) {
        socket_type = SOCK_DGRAM | SOCK_NONBLOCK;
        socket_rcvbuf = cfg->udp_socket_recvbuff_size;
        socket_sndbuf = cfg->udp_socket_sendbuff_size;
        port          = cfg->udp_listener_port;
    }

    /* Create socket. */
    fd = socket(family, socket_type, protocol);
    if (fd < 0) {
        *err_no = errno;
        return LISTENER_ERR_SOCKET;
    }

    /* Set socket options receive buffer and send buffer size. */
    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &socket_rcvbuf,
                     sizeof(socket_rcvbuf));
    if (ret != 0) {
        *err_no = errno;
        close(fd);
        return LISTENER_ERR_SOCKET_OPT_RCVBUF;
    }
    ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &socket_sndbuf,
                     sizeof(socket_sndbuf));
    if (ret != 0) {
        *err_no = errno;
        close(fd);
        return LISTENER_ERR_SOCKET_OPT_SNDBUF;
    }

    /* Set socket option SO_REUSEADDR. */
    opt = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret != 0) {
        *err_no = errno;
        close(fd);
        return LISTENER_ERR_SOCKET_OPT_REUSEADDR;
    }
    /* Set socket option SO_REUSEPORT. */
    opt = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (ret != 0) {
        *err_no = errno;
        close(fd);
        return LISTENER_ERR_SOCKET_OPT_REUSEPORT;
    }
    opt = 1;
    if (family == AF_INET) {
        /* Set socket option IP_PKTINFO to get source IPv4 address on UDP packets. */
        ret = setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt));
        if (ret != 0) {
            *err_no = errno;
            close(fd);
            return LISTENER_ERR_SOCKET_OPT_IP_PKTINFO;
        }
    } else {
        /* Set socket option IPV6_V6ONLY to receive IPv6 UDP packets only,
         * and not get IPv4 mapped into IPv6 packets.
         */
        ret = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
        if (ret != 0) {
            *err_no = errno;
            close(fd);
            return LISTENER_ERR_SOCKET_OPT_IPV6_V6ONLY;
        }
        /* Set socket option IPV6_RECVPKTINFO to get source IPv6 address on UDP
         * packets.
         */
        opt = 1;
        ret = setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &opt, sizeof(opt));
        if (ret != 0) {
            *err_no = errno;
            close(fd);
            return LISTENER_ERR_SOCKET_OPT_IPV6_RECVPKTINFO;
        }
    }

    /* Populate socket address to be any IP address. */
    socklen_t slen = sizeof(struct sockaddr_in6);
    if (family == AF_INET) {
        slen = sizeof(struct sockaddr_in);
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = INADDR_ANY;
        sin->sin_port = htons(port);
    } else {
        slen = sizeof(struct sockaddr_in6);
        sin6->sin6_family = AF_INET6;
        sin6->sin6_addr = in6addr_any;
        sin6->sin6_port = htons(port);
    }

    /* Bind socket. */
    ret = bind(fd, (struct sockaddr *)&ss, slen);
    if (ret != 0) {
        *err_no = errno;
        close(fd);
        return LISTENER_ERR_BIND;
    }

    /* If TCP start listening on socket. */
    if (protocol == IPPROTO_TCP) {
        ret = listen(fd, cfg->tcp_listener_pending_conns_max);
        if (ret != 0) {
            *err_no = errno;
            close(fd);
            return LISTENER_ERR_LISTEN;
        }
    }

    return fd;
}

/** Provision a new TCP or UDP listener and return the corresponding connection
 *  conn_t object.
 * 
 * Listener is started and associated with new connection object. New connection
 * object is initialized meaning it has all necessary buffers allocated.
 * 
 * @param cfg          Configuration object that has settings:
 *                     - udp_conn_vector_len,
 * @param family       IP family to start a listener for, valid options are:
 *                     - AF_INET,
 *                     - AF_INET6.
 * @param protocol     Protocol to start listener for, valid options are:
 *                     - IPPROTO_TCP,
 *                     - IPPROTO_UDP.
 * @param err_buf      Buffer where to store error message if error was encountered.
 *                     If NULL no message is stored.
 * @param err_buf_len  Length of error buffer.
 * 
 * @return             On success returns a newly allocated and active connection
 *                     object.
 *                     On error returns NULL and message is populated into error
 *                     buffer.
 */
conn_t *
conn_listener_provision(config_t *cfg, int family, int protocol,
                        char *err_buf, size_t err_buf_len)
{
    char *protp_str_udp = "UDP";
    char *protp_str_tcp = "TCP";
    char *protp_str     = protp_str_udp;
    char *ip4_str       = "IPv4";
    char *ip6_str       = "IPv6";
    char *ip_str        = ip4_str;

    int err_no = 0;

    /* Verify function arguments. */
    if (family != AF_INET && family != AF_INET6) {
        assert(0);
    }
    if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP) {
        assert(0);
    }

    /* Start listener. */
    int fd = listener_start(cfg, family, protocol, &err_no);

    if (fd < 0) {
        if (protocol == IPPROTO_TCP) {
            protp_str = protp_str_tcp;
        }
        if (family == AF_INET6) {
            ip_str = ip6_str;
        }

        if (err_buf != NULL) {
            snprintf(err_buf, err_buf_len, "Could not start %s %s listener, %s: %s",
                    protp_str, ip_str, listener_err_to_str(fd), strerror(err_no));
        }
        return NULL;
    }

    /* Initialize connection object. */
    conn_t *conn = malloc(sizeof(conn_t));
    CHECK_MALLOC(conn);
    *conn = (conn_t) {
        .fd = fd,
    };

    if (family == AF_INET) {
        conn->ip_version = 0;
    } else {
        conn->ip_version = 1;
    }

    if (protocol == IPPROTO_UDP) {
        conn->proto = 0;
        conn->conn.udp = conn_udp_new(cfg, family);
    } else if (protocol == IPPROTO_TCP) {
        conn->proto = 1;
        /* TCP listener does not have a conn->conn.tcp section. */
    }

    return conn;
}

/** Add (enqueue) a connection object to read fifo queue.
 * 
 * @param queue Read queue to add connection object to.
 * @param conn  Connection object to queue up.
 */
void
conn_fifo_enqueue_read(conn_fifo_queue_t *queue, conn_t *conn)
{
    if (conn->in_read_queue == 0) {
        conn->read_q_handle = NULL;
        if (queue->head == NULL) {
            queue->head = queue->tail = conn;
        } else {
            queue->tail->read_q_handle = conn;
            queue->tail = conn;
        }
        conn->in_read_queue = 1;
    }
}

/** Remove (dequeue) a connection object from read fifo queue.
 * 
 * @param queue Read queue to remove connection object from.
 * 
 * @return      On success returns connection object, otherwise there are
 *              no entries in the queue and returns NULL.
 */
conn_t *
conn_fifo_dequeue_read(conn_fifo_queue_t *queue)
{
    conn_t *conn = NULL;

    conn = queue->head;
    if (queue->head == queue->tail) {
        queue->head = queue->tail = NULL;
    } else {
        queue->head = conn->read_q_handle;
    }
    if (conn != NULL) {
        conn->in_read_queue = 0;
    }
    return conn;
}

/** Add (enqueue) a connection object to generic fifo queue.
 * 
 * @param queue Queue to add object to.
 * @param conn  Object to queue up.
 */
void
conn_fifo_enqueue_gen(conn_fifo_queue_t *queue, conn_t *conn)
{
    conn->gen_q_handle = NULL;
    if (queue->head == NULL) {
        queue->head = queue->tail = conn;
    } else {
        queue->tail->gen_q_handle = conn;
        queue->tail = conn;
    }
}

/** Remove (dequeue) a connection object from generic fifo queue.
 * 
 * @param queue Queue to remove connection object from.
 * 
 * @return      On success returns object, otherwise there are
 *              no entries in the queue and returns NULL.
 */
conn_t *
conn_fifo_dequeue_gen(conn_fifo_queue_t *queue)
{
    conn_t *conn = NULL;

    conn = queue->head;
    if (queue->head == queue->tail) {
        queue->head = queue->tail = NULL;
    } else {
        queue->head = conn->gen_q_handle;
    }
    
    return conn;
}

/** Add (enqueue) a connection object to write fifo queue.
 * 
 * @param queue Write queue to add connection object to.
 * @param conn  Connection object to queue up.
 */
void
conn_fifo_enqueue_write(conn_fifo_queue_t *queue, conn_t *conn)
{
    if (conn->in_write_queue == 0) {
        conn->write_q_handle = NULL;
        if (queue->head == NULL) {
            queue->head = queue->tail = conn;
        } else {
            queue->tail->write_q_handle = conn;
            queue->tail = conn;
        }
        conn->in_write_queue = 1;
    }
}

/** Remove (dequeue) a connection object from write fifo queue.
 * 
 * @param queue Write queue to remove connection object from.
 * 
 * @return      On success returns connection object, otherwise there are
 *              no entries in the queue and returns NULL.
 */
conn_t *
conn_fifo_dequeue_write(conn_fifo_queue_t *queue)
{
    conn_t *conn = NULL;

    conn = queue->head;
    if (queue->head == queue->tail) {
        queue->head = queue->tail = NULL;
    } else {
        queue->head = conn->write_q_handle;
    }
    if (conn != NULL) {
        conn->in_write_queue = 0;
    }
    return conn;
}


/** Add (enqueue) a connection object to release fifo queue.
 * 
 * @param queue Queue to add object to.
 * @param conn  Object to queue up.
 */
void
conn_fifo_enqueue_release(conn_fifo_queue_t *queue, conn_t *conn)
{
    if (conn->in_release_queue == 0) {
        conn->gen_q_handle = NULL;
        if (queue->head == NULL) {
            queue->head = queue->tail = conn;
        } else {
            queue->tail->gen_q_handle = conn;
            queue->tail = conn;
        }
        conn->in_release_queue = 1;
    }
}

/** Remove (dequeue) a generic object from fifo queue.
 * 
 * @param queue Queue to remove connection object from.
 * 
 * @return      On success returns object, otherwise there are
 *              no entries in the queue and returns NULL.
 */
conn_t *
conn_fifo_dequeue_release(conn_fifo_queue_t *queue)
{
    conn_t *conn = NULL;

    conn = queue->head;
    if (queue->head == queue->tail) {
        queue->head = queue->tail = NULL;
    } else {
        queue->head = conn->gen_q_handle;
    }
    if (conn != NULL) {
        conn->in_release_queue = 0;
    }
    
    return conn;
}


/** Remove conn from read queue.
 * 
 * @param queue   Read queue to remove conn object from.
 * @param conn_rm Conn object to remove from queue.
 */
void
conn_fifo_remove_from_read_queue(conn_fifo_queue_t *queue, conn_t *conn_rm)
{
    conn_t            *conn;
    conn_fifo_queue_t new_queue = {};

    if (conn_rm->in_read_queue) {
        while ((conn = conn_fifo_dequeue_read(queue)) != NULL) {
            if (conn != conn_rm) {
                conn_fifo_enqueue_read(queue, conn);
            }
        }
    }

    /* Repopulate queue. */
    if (new_queue.head != NULL) {
        queue->head = new_queue.head;
        queue->tail = new_queue.tail;
    }
}

/** Remove conn from write queue.
 * 
 * @param queue   Write queue to remove conn object from.
 * @param conn_rm Conn object to remove from queue.
 */
void
conn_fifo_remove_from_write_queue(conn_fifo_queue_t *queue, conn_t *conn_rm)
{
    conn_t            *conn;
    conn_fifo_queue_t new_queue = {};

    if (conn_rm->in_read_queue) {
        while ((conn = conn_fifo_dequeue_write(queue)) != NULL) {
            if (conn != conn_rm) {
                conn_fifo_enqueue_write(queue, conn);
            }
        }
    }

    /* Repopulate queue. */
    if (new_queue.head != NULL) {
        queue->head = new_queue.head;
        queue->tail = new_queue.tail;
    }
}

/** Get entry in TCP connection LRU cache and also update LRU state (entry is set as MRU).
 * 
 * @param lru  Pointer to LRU cache.
 * @param id   Entry key to look for.
 * 
 * @return     On success returns a pointer to connection in LRU cache. Entry
 *             place in LRU is updated.
 *             Otherwise returns NULL indicating that entry was not found in
 *             LRU cache.
 */
conn_t *
conn_lru_cache_get(conn_t **lru, uint64_t id)
{
    conn_t *entry = NULL;

    HASH_FIND(hh, *lru, &id, sizeof(uint64_t), entry);

    if (entry != NULL) {
        /* Remove then add entry so it is on the front of the list. */
        LRU_CACHE_DEL(*lru, entry);
        LRU_CACHE_ADD(*lru, entry);
        return entry;
    }
    return NULL;
}

/** Assign a new TCP connection ID.
 * 
 * @param id   Where to store newly assigned TCP connection ID.
 * @param lru  TCP connection LRU cache, used to verify uniqueness of connection
 *             ID being assigned.
 * @param base Base for connection ID. (Base is unique per Vectorloop object.
 * 
 * @return     Returns true if connection ID was assigned, otherwise returns
 *             false.
 */
bool
conn_tcp_id_assign(uint64_t *id, conn_t **lru, uint64_t *base)
{
    conn_t *conn = NULL;

    for (uint64_t i = *base + 1; i < UINTMAX_MAX; i++){
        conn = NULL;
        HASH_FIND(hh, *lru, &i, sizeof(uint64_t), conn);
        if (conn == NULL) {
            *id = i;
            *base = i;
            return true;
        }
    }
    for (uint64_t i = 0; i < *base; i++){
        conn = NULL;
        HASH_FIND(hh, *lru, &i, sizeof(uint64_t), conn);
        if (conn == NULL) {
            *id = i;
            *base = i;
            return true;
        }
    }  

    return false;
}