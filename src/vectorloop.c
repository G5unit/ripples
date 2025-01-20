/**
 * @file vectorloop.c
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
#include <sched.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <time.h>

#include "channel.h"
#include "config.h"
#include "conn.h"
#include "log_app.h"
#include "lru_cache.h"
#include "query.h"
#include "utils.h"
#include "vectorloop.h"
#include "vectorloop_epoll.h"

/** Vectorloop function processes messages received on channels.
 * 
 * Channels vectorloop receives messages on are:
 * - resource channel which updates the resource pointer, returns message
 *   indicating operation completed.
 * - query log channel which flips the active query log buffer and returns a
 *   pointer to inactive buffer. Inactive buffer is one written to disk by
 *   query log thread.
 * 
 * @param vl Vectorloop operating on.
 * 
 * @return   Returns number of channel messages processed.
 */
static int
vl_fn_channel_messages(vectorloop_t *vl)
{
    int ret = 0;
    channel_bss_msg_t *ch_msg;

    /* Check for messages on resource channel. */
    if (channel_bss_recv(vl->resource_channel, &ch_msg) != 0) {
        switch (ch_msg->op) {
        case CH_OP_RES_SET_RESOURCE1:
            /* set new pointer for resource 1. */
            debug_printf("vl %d, got channel message for resource 1", vl->id);

            ch_msg->result = 1;
            channel_bssvl_send(vl->resource_channel, ch_msg);
            ret += 1;
            break;

        case CH_OP_RES_SET_RESOURCE2:

            break;

        default:
            /* Code error, unknown resource message op. Log and exit. */
            {
                char *err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
                CHECK_MALLOC(err_str);
                snprintf(err_str, ERR_MSG_LENGTH, "vl_fn_channel_messages() error, "
                        "unknown resource channel message op \"%d\"", ch_msg->op);
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, true);
                channel_log_send(vl->app_log_channel, lmsg);
            }
        }
    }

    /* Check for messages on query log channel. */
    if (channel_bss_recv(vl->query_log_channel, &ch_msg) != 0) {
        switch (ch_msg->op) {
        case CH_OP_QUERY_LOG_FLIP:
            /* Flip log buffer */
            debug_printf("vl %d, got channel message to flip query log buffer", vl->id);

            ch_msg->result = 1;
            ch_msg->p = vl->query_log.buf;
            ch_msg->result = vl->query_log.buf_len;
            query_log_rotate(&vl->query_log);

            channel_bssvl_send(vl->query_log_channel, ch_msg);
            ret += 1;
            break;

        default:
            /* Code error, unknown query log message op. Log and exit. */
            {
                char *err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
                CHECK_MALLOC(err_str);
                snprintf(err_str, ERR_MSG_LENGTH, "vl_fn_channel_messages() error, "
                        "unknown query log channel message op \"%d\"", ch_msg->op);
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, true);
                channel_log_send(vl->app_log_channel, lmsg);
            }
        }
    }

    return ret;
}

/** Vectorloop function polls epoll for events.
 * 
 * Each vectorloop has its own epoll file descriptor with witch all connections
 * are registered for events EPOLLIN (data available to read) and EPOLLOUT
 * (writable). Connections are registerred for Edge Trigger notifications, see
 * Linux manual page "man epoll" for full details on what Edge Triggered means.
 * 
 * @param vl Vectorloop operating on.
 * 
 * @return   Returns number of new epoll events received.
 */
static int
vl_fn_epoll(vectorloop_t *vl)
{
    struct epoll_event *ev;
    int                 event_count = 0;
    int                 count       = 0;
    conn_t             *conn;
    
    /* Check with epoll for new UDP events (non-blocking). */
    count = vl_epoll_wait(vl->ep_fd_udp, vl->ep_events, vl->cfg->epoll_num_events_udp);
    event_count += count;

     /* Handle each event. */
    for (int i = 0; i < count; i++) {
        /* Get event from epoll. */
        ev = &vl->ep_events[i];

        /* Event data is a pointer to connection object it is associated with. */
        conn = (conn_t *)ev->data.u64;

        if (CONN_IS_UDP_LISTENER(conn)) {
            /* Single event could be for either or both EPOLLIN and EPOLLOUT, hence
             * both are checked for.
             */
            if (ev->events & EPOLLIN) {
                if (conn->waiting_for_read) {
                    /* Add conn to read queue. */
                    conn->waiting_for_read = 0;
                    conn_fifo_enqueue_read(&vl->conn_udp_read_queue, conn);
                }
            }
            if (ev->events & EPOLLOUT) {
                if (conn->waiting_for_write) {
                    /* Add conn to write queue. */
                    conn->waiting_for_write = 0;
                    conn_fifo_enqueue_write(&vl->conn_udp_write_queue, conn);
                }
            }
        } else {
            /* Code error, event id not recognized. Log and exit. */
            channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_VL_FN_EPOLL, NULL, true);
            channel_log_send(vl->app_log_channel, lmsg);
            return 0;
        }
    }

    
    /* Check with epoll for new TCP events (non-blocking). */
    count = vl_epoll_wait(vl->ep_fd_tcp, vl->ep_events, vl->cfg->epoll_num_events_tcp);
    event_count += count;

        /* Handle each event. */
        for (int i = 0; i < count; i++) {
            /* Get event from epoll. */
            ev = &vl->ep_events[i];

            /* Event data is a pointer to connection object it is associated with. */
            conn = (conn_t *)ev->data.u64;
            
            if (CONN_IS_TCP_LISTENER(conn)) {
                /* Accept connections from TCP listener. */
                /* Add conn to TCP accept conns queue. */
                conn->waiting_for_read = 0;
                conn_fifo_enqueue_read(&vl->conn_tcp_accept_conns_queue, conn);
                
            } else if (CONN_IS_TCP_CONN(conn)) {
                if (ev->events & EPOLLIN) {
                    if (conn->waiting_for_read) {
                        /* Add conn to read queue. */
                        conn->waiting_for_read = 0;
                        conn_fifo_enqueue_read(&vl->conn_tcp_read_queue, conn);
                    }
                }
                if (ev->events & EPOLLOUT) {
                    if (conn->waiting_for_write) {
                        /* Add conn to write queue. */
                        conn->waiting_for_write = 0;
                        conn_fifo_enqueue_write(&vl->conn_tcp_write_queue, conn);
                    }
                }
            } else {
                /* Code error, event id not recognized. Log and exit. */
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_VL_FN_EPOLL, NULL, true);
                channel_log_send(vl->app_log_channel, lmsg);
                return 0;
            }
        }

    return event_count;
}

/** Vectorloop function accepts new TCP connections.
 * 
 * Number of active TCP connections is limited to
 * configuration setting "tcp_conns_per_vl_max".
 * 
 * @param vl Vectorloop operating on.
 * 
 * @return   Returns number of new TCP connection accepted.
 */
static int
vl_fn_tcp_accept_conns(vectorloop_t *vl)
{
    conn_t                 *conn;
    conn_t                 *tcp_conn;
    struct sockaddr_storage client_ip;
    struct sockaddr_storage local_ip;
    socklen_t               ip_len       = sizeof(struct sockaddr_storage);   
    int                     fd           = -1;
    int                     ip_version   = 0;
    int                     accept_count = 0;
    int                     accept_max   = 0;
    conn_fifo_queue_t       new_queue    = {};

    
    while ((conn = conn_fifo_dequeue_read(&vl->conn_tcp_accept_conns_queue)) != NULL) {
        /* Ensure that newly accepted connection count does not exceed maximum
         * number of active TCP connections allowed.
         */
        accept_max = vl->cfg->tcp_conns_per_vl_max - vl->conns_tcp_active;
        if (accept_max > vl->cfg->tcp_listener_max_accept_new_conn) {
            accept_max = vl->cfg->tcp_listener_max_accept_new_conn;
        }

        while ((fd = accept4(conn->fd, (struct sockaddr *)&client_ip, &ip_len,
                             SOCK_NONBLOCK)) > -1 &&
                accept_count < vl->cfg->tcp_listener_max_accept_new_conn) {
            /* New TCP connection accepted. */
            INCREMENT(accept_count);

            /* Check client IP. */
            if (client_ip.ss_family == AF_INET) {
                ip_version = 0;
                atomic_fetch_add(&vl->metrics->tcp.connections, 1);
            } else if (client_ip.ss_family == AF_INET6) {
                ip_version = 1;
                atomic_fetch_add(&vl->metrics->tcp.connections, 1);
            } else {
                /* Unsupported client_ip socket family */
                close(fd);
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_VL_FN_TCP_CONN_CLIENT_IP_FAM, NULL, false);
                channel_log_send(vl->app_log_channel, lmsg);
                atomic_fetch_add(&vl->metrics->tcp.unknown_client_ip_soc_family, 1);
                continue;
            }

            /* Get local IP. */
            ip_len = sizeof(struct sockaddr_storage);
            if (getsockname(fd, (struct sockaddr *)&local_ip, &ip_len) != 0) {
                /* System error or system out of resources. Log and exit. */
                close(fd);
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_VL_FN_TCP_CONN_GETSOCKNAME, NULL, false);
                channel_log_send(vl->app_log_channel, lmsg);
                atomic_fetch_add(&vl->metrics->tcp.getsockname_err, 1);
                continue;
            }

            /* Check local IP. */
            if (local_ip.ss_family != AF_INET && local_ip.ss_family != AF_INET6) {
                /* Unsupported local_ip socket family. */
                close(fd);
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_VL_FN_TCP_CONN_LOCAL_IP_FAM, NULL, false);
                channel_log_send(vl->app_log_channel, lmsg);
                atomic_fetch_add(&vl->metrics->tcp.unknown_local_ip_soc_family, 1);
                continue;
            }
            
            /* Create TCP conn object. */
            tcp_conn = conn_new_tcp(fd, vl->cfg, ip_version, &client_ip, &local_ip);

            /* Set time, timeout and state. */
            tcp_conn->conn.tcp->start_time = vl->loop_timestamp;
            tcp_conn->conn.tcp->timeout    = vl->loop_timestamp;
            tcp_conn->conn.tcp->timeout.tv_sec += vl->cfg->tcp_query_recv_timeout;
            tcp_conn->conn.tcp->state = TCP_CONN_ST_WAIT_FOR_QUERY_DATA;

            /* Assign conn TCP ID. */
            if (conn_tcp_id_assign(&tcp_conn->cid, &vl->conn_tcp_lru_cache,
                                   &vl->conn_tcp_id_base) == false) {
                /* Could not find available ID to assign, reject connection.
                 * This should never be the case as we could only run out of
                 * IDs to assign if we have uint64_t max active connections.
                 * In addition, we ensure that there are only max of
                 * CONN_TCP_ACTIVE_CONNS_MAX active TCP connections. 
                 */
                close(fd);
                tcp_conn->fd = -1;
                tcp_conn->conn.tcp->state = TCP_CONN_ST_ASSIGN_CONN_ID_ERR;
                conn_fifo_enqueue_release(&vl->conn_tcp_release_queue, tcp_conn);
                continue;
            }
                        
            /* Add new conn to TCP_CONN_TRACK LRU HASH. */
            LRU_CACHE_ADD(vl->conn_tcp_lru_cache, tcp_conn);

            /* Register new conn with epoll. */
            tcp_conn->waiting_for_read = 1;
            vl_epoll_ctl_reg_for_read_et(vl->ep_fd_tcp, fd, (uint64_t)tcp_conn);

            /* Increment active TCP connections count. */
            INCREMENT(vl->conns_tcp_active);
        }

        if (fd < 0 || accept_max <= 0) {
            /* accept4() returned an error. */
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                /* No new connections to accept. Listener conn will be added to
                 * TCP accept queue once epoll event "read is available" arrives.
                 */
                conn->waiting_for_read = 1;
            } else {
                /* Error occurred on TCP listener socket, this is not recoverable. Log and exit. */
                char *err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
                CHECK_MALLOC(err_str);
                snprintf(err_str, ERR_MSG_LENGTH, "listener error, %s", strerror(errno));
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, true);
                channel_log_send(vl->app_log_channel, lmsg);
                return 0 ;
            }
        } else {
            /* There are more connections to accept, or number of connections is
             * maxed out per vl->cfg->tcp_conns_per_vl_max. Try again next loop
             * iteration.
             */
            conn_fifo_enqueue_read(&new_queue, conn);
        }
    }

    /* Repopulate TCP accept queue. */
    if (new_queue.head != NULL) {
        vl->conn_tcp_accept_conns_queue.head = new_queue.head;
        vl->conn_tcp_accept_conns_queue.tail = new_queue.tail;
    }

    return accept_count;
}

/** Vectorloop function reads data from TCP connections.
 * 
 * @param vl Vectorloop operating on.
 * 
 * @return   Returns number of TCP connections data was read in from.
 */
static int
vl_fn_tcp_read(vectorloop_t *vl)
{
    conn_t            *conn;
    conn_tcp_t        *conn_tcp;
    conn_fifo_queue_t new_queue = {};
    int               read_count = 0;
    ssize_t           ret      = 0;

    while ((conn = conn_fifo_dequeue_read(&vl->conn_tcp_read_queue)) != NULL) {
        INCREMENT(read_count);
        conn_tcp = conn->conn.tcp;

        /* Update conn in LRU cache. */
        if (conn_lru_cache_get(&vl->conn_tcp_lru_cache, conn->cid) == NULL) {
            /* code error. */
            assert(0);
        }

        /* Reset queries array */
        for (int i = 0; i < conn_tcp->queries_count; i++) {
            query_reset(&conn_tcp->queries[i]);
        }
        conn_tcp->queries_count = 0;

        /* Read from socket into read buffer.  */
        ret = read(conn->fd, &conn_tcp->read_buffer[conn_tcp->read_buffer_len],
                   conn_tcp->read_buffer_size - conn_tcp->read_buffer_len);
        conn_tcp->read_buffer_len += ret;

        if (ret == 0) {
            /* Connection was closed for read.
             * Since read is done only if there are no active queries associated
             * with this TCP connection, meaning there are no pending writes,
             * this means it is safe to just close fd and release the connection.
             */
            conn_tcp->state = TCP_CONN_ST_CLOSED_FOR_READ;
            conn_fifo_enqueue_release(&vl->conn_tcp_release_queue, conn);
            continue;
        } else if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Need to wait for data to arrive.
                 * If our read buffer is empty then set the state to
                 * WAIT_FOR_QUERY with appropriate timeout matching the
                 * tcp-keepalive.
                 */
                if (conn_tcp->read_buffer_len == 0) {
                    conn_tcp->state = TCP_CONN_ST_WAIT_FOR_QUERY;
                    conn_tcp->timeout = vl->loop_timestamp;
                    conn_tcp->timeout.tv_sec += conn_tcp->tcp_keepalive;
                }
                conn->waiting_for_read = 1;
                continue;
            } else {
                /* Some other error occurred on socket. End conn. */
                conn_tcp->state = TCP_CONN_ST_READ_ERR;
                conn_fifo_enqueue_release(&vl->conn_tcp_release_queue, conn);
            }
        }

        /* Data was read in, check if full DNS query is in read buffer. */
        int            not_full_query = 0;
        query_t       *queries        = conn_tcp->queries;
        int            i              = 0;
        unsigned char *buf            = conn_tcp->read_buffer;
        size_t         buf_len        = conn_tcp->read_buffer_len;
        uint16_t       q_len          = 0;
        for (i = 0; i < conn_tcp->queries_size; i++) {
            q_len = ntohs(*(uint16_t *)buf);
            if (q_len > RIP_NS_PACKETSZ) {
                /* Bad format, query length exceed RIP_NS_PACKETSZ (512) bytes. */
                conn_tcp->state = TCP_CONN_ST_QUERY_SIZE_TOOLARGE;
                conn_fifo_enqueue_release(&vl->conn_tcp_release_queue, conn);
            }

            if ((q_len + 2) <= buf_len) {
                /* Query present. */
                queries[i].start_time          = vl->loop_timestamp;
                queries[i].protocol            = 1;
                queries[i].client_ip           = &conn_tcp->client_ip;
                queries[i].local_ip            = &conn_tcp->local_ip;
                queries[i].request_buffer      = buf;
                queries[i].request_hdr         = (rip_ns_header_t *)(buf + 2);
                queries[i].request_buffer_size = q_len;
                queries[i].request_buffer_len  = q_len;
                buf += 2 + q_len;
                buf_len -= 2 + q_len;
            } else {
                if (i == 0) {
                    /* Full query not received, need to wait for more data. */
                    conn->waiting_for_read = 1;
                    not_full_query = 1;
                }
                break;
            }
        }
        if (not_full_query) {
            /* Full query not received, need to read in some more. */
            if (conn_tcp->state == TCP_CONN_ST_WAIT_FOR_QUERY) {
                /* When TCP conn is in state WAIT_FOR_QUERY it means that the
                 * previous idle (WAIT_FOR_QUERY) timeout applied. Since we got
                 * data, idle timeout should now be changed to
                 * WAIT_FOR_QUERY_DATA. Timeout for case where conn is in state
                 * WAIT_FOR_FIRST_QUERY and state we are transitioning into are
                 * the same, so we do not need to do anything for that case.
                 */
                conn_tcp->state = TCP_CONN_ST_WAIT_FOR_QUERY_DATA;
                conn_tcp->timeout = vl->loop_timestamp;
                conn_tcp->timeout.tv_sec += vl->cfg->tcp_query_recv_timeout;
            }
            conn_fifo_enqueue_read(&new_queue, conn);
            continue;
        }
        conn_tcp->queries_count        = i;
        conn_tcp->queries_total_count += i;

        /* Query(s) received, move conn to parse query queue. */
        conn_fifo_enqueue_gen(&vl->query_parse_queue, conn);
    }

    /* Repopulate vectorloop tcp read queue. */
    if (new_queue.head != NULL) {
        vl->conn_tcp_read_queue.head = new_queue.head;
        vl->conn_tcp_read_queue.tail = new_queue.tail;
    }

    return read_count;
}

/** Vectorloop function reads data from UDP connections.
 * 
 * @param vl Vectorloop operating on.
 * 
 * @return   Returns number of UDP datagrams read in.
 */
static int
vl_fn_udp_read(vectorloop_t *vl)
{

    conn_t            *conn;
    conn_udp_t        *conn_udp;
    conn_fifo_queue_t  new_queue = {};
    int                ret;    
    int                recv_count = 0;

    while ((conn = conn_fifo_dequeue_read(&vl->conn_udp_read_queue)) != NULL) {
        conn_udp = conn->conn.udp;

        /* Reset conn UDP read, query and write vectors. */
        conn_udp_vectors_reset(conn_udp);

        /* Read in packets via recvmmsg() into msg vector */
        ret = recvmmsg(conn->fd, conn_udp->read_vector,
                       conn_udp->vector_len, MSG_DONTWAIT, NULL);
        if (ret > 0) {

            /* UDP packets were received on UDP conn, move conn to parse
             * DNS query queue.
             */
            conn_udp->read_vector_count  = ret;
            conn_fifo_enqueue_gen(&vl->query_parse_queue, conn);
            recv_count += ret;
        } else {
            /* No UDP packets were received on UDP conn. */
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                /* need to wait for read. */
                conn->waiting_for_read = 1;
            } else {
                /* Error occurred on UDP conn. Of potential interest here is EHOSTUNREACH
                 * (see Linux manual "man -s7 ip").
                 * Current action is to log the error and move on.
                 */
                char *err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
                CHECK_MALLOC(err_str);
                snprintf(err_str, ERR_MSG_LENGTH, "vl_fn_udp_read: UDP read error, %s", strerror(errno));
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, false);
                channel_log_send(vl->app_log_channel, lmsg);

                /* Enqueue conn to read again. */
                conn_fifo_enqueue_read(&vl->conn_udp_read_queue, conn);
            }
        }
    }

    /* Repopulate vectorloop udp read queue. */
    if (new_queue.head != NULL) {
        vl->conn_udp_read_queue.head = new_queue.head;
        vl->conn_udp_read_queue.tail = new_queue.tail;
    }

    return recv_count;
}

/** Vectorloop function parses newly received queries.
 * 
 * @param vl Vectorloop operating on.
 */
static void
vl_fn_query_parse(vectorloop_t *vl)
{
    conn_t         *conn;
    struct mmsghdr *read_vector;
    unsigned int    read_vector_count;
    struct mmsghdr *write_vector;
    query_t        *queries;

    while ((conn = conn_fifo_dequeue_gen(&vl->query_parse_queue)) != NULL) {
        if (conn->proto == 0) {
            /* UDP conn. */
            read_vector       = conn->conn.udp->read_vector;
            read_vector_count = conn->conn.udp->read_vector_count;
            queries           = conn->conn.udp->queries;
            write_vector      = conn->conn.udp->write_vector;

            for (int i = 0; i < read_vector_count; i++) {
                /* check request size. */
                if (read_vector[i].msg_len > RIP_NS_PACKETSZ) {
                    /* Request exceeds allowed RIP_NS_PACKETSZ size. */
                    queries[i].end_code = rip_ns_r_rip_toolarge;
                    continue;
                }
                /* Extract destination (local) IP from message control header.
                 * Populate extracted destination IP & port in 
                 * query structure local_ip field.
                 */
                for ( /* iterate through all the control headers */
                    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&read_vector[i].msg_hdr);
                    cmsg != NULL;
                    cmsg = CMSG_NXTHDR(&read_vector[i].msg_hdr, cmsg)) {

                    if (conn->ip_version == 0) {
                        /* IPv4 */
                        /* Ignore the control headers that don't match what we want */
                        if (cmsg->cmsg_level != IPPROTO_IP ||
                            cmsg->cmsg_type != IP_PKTINFO) {
                            continue;
                        }
                        
                        struct in_pktinfo *pi = (struct in_pktinfo *)CMSG_DATA(cmsg);
                        /* At this point:
                         * pi->ipi_spec_dst is the destination in_addr
                         * pi->ipi_addr is the receiving interface in_addr
                         */
                        struct sockaddr_in *sin = (struct sockaddr_in *)queries[i].local_ip;
                        sin->sin_family = AF_INET;
                        sin->sin_addr = pi->ipi_spec_dst;
                        sin->sin_port = htons(vl->cfg->udp_listener_port);
                        
                    } else {
                        /* IPv6 */
                        /* Ignore the control headers that don't match what we want */
                        if (cmsg->cmsg_level != IPPROTO_IPV6 ||
                            cmsg->cmsg_type != IPV6_PKTINFO) {
                            continue;
                        }
                        struct in6_pktinfo *pi6 = (struct in6_pktinfo *)CMSG_DATA(cmsg);
                        /* At this point:
                         * pi->ipi6_addr is the destination in_addr
                         */
                        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)queries[i].local_ip;
                        sin6->sin6_family = AF_INET6;
                        sin6->sin6_addr = pi6->ipi6_addr;
                        sin6->sin6_port = htons(vl->cfg->udp_listener_port);
                    }
                }

                /* Extract source ip from message msg_name field. */
                memcpy(queries[i].client_ip, read_vector[i].msg_hdr.msg_name,
                       read_vector[i].msg_hdr.msg_namelen);

                /* Populate write vector with source & destination IPs. */
                write_vector[i].msg_hdr.msg_control = read_vector[i].msg_hdr.msg_control;
                write_vector[i].msg_hdr.msg_controllen = read_vector[i].msg_hdr.msg_controllen;
                write_vector[i].msg_hdr.msg_name = read_vector[i].msg_hdr.msg_name;
                write_vector[i].msg_hdr.msg_namelen = read_vector[i].msg_hdr.msg_namelen;

                /* Set query start time. */
                queries[i].start_time = vl->loop_timestamp;
        
                /* Parse DNS query from datagram. */
                queries[i].request_buffer_len = read_vector[i].msg_len;
                query_parse(&queries[i]);
            }
        } else {
            /* TCP protocol. */
            for (int i = 0; i < conn->conn.tcp->queries_count; i++) {
                query_parse(&conn->conn.tcp->queries[i]);
            }
        }
        /* All queries for conn parsed, send conn to resolve query queue. */
        conn_fifo_enqueue_gen(&vl->query_resolve_queue, conn);
    }
}

/** Vectorloop function resolves newly parsed queries.
 * 
 * @param vl Vectorloop operating on.
 */
static void
vl_fn_query_resolve(vectorloop_t *vl)
{
    conn_t       *conn;
    unsigned int  read_vector_count;
    query_t      *queries;

    while ((conn = conn_fifo_dequeue_gen(&vl->query_resolve_queue)) != NULL) {
        if (conn->proto == 0) {
            /* UDP conn. */
            conn_udp_t *conn_udp = conn->conn.udp;

            read_vector_count = conn_udp->read_vector_count;
            queries           = conn_udp->queries;
            for (int i = 0; i < read_vector_count; i++) {
                if (queries[i].end_code != -1) {
                    /* End code for query is already set, meaning request
                     * did not pass query_parse() checks.
                     */
                    continue;
                }
                query_resolve(&queries[i]);
            }
            /* All queries for conn resolved, send conn to response pack queue. */
            conn_fifo_enqueue_gen(&vl->query_response_pack_queue, conn);

        } else {
            /* TCP */
            conn_tcp_t *conn_tcp = conn->conn.tcp;

            queries = conn_tcp->queries;
            for (int i = 0; i < conn_tcp->queries_count; i++) {
                if (queries[i].end_code != -1) {
                    /* End code for query is already set, meaning request
                     * did not pass query_parse() checks.
                     */
                    continue;
                }
                query_resolve(&queries[i]);
            }
            /* All queries for conn resolved, send conn to pack query queue. */  
            conn_fifo_enqueue_gen(&vl->query_response_pack_queue, conn);
        }
    }
}

/** Pack query response into buffer suitable to be transmitted over network.
 * 
 * @param vl Vectorloop operating on.
 */
static void
vl_fn_query_response_pack(vectorloop_t *vl)
{
    conn_t       *conn;
    unsigned int  read_vector_count;
    query_t      *queries;

    while ((conn = conn_fifo_dequeue_gen(&vl->query_response_pack_queue)) != NULL) {
        if (conn->proto == 0) {
            /* UDP conn. */
            conn_udp_t *conn_udp = conn->conn.udp;

            read_vector_count = conn_udp->read_vector_count;
            queries           = conn_udp->queries;
            for (int i = 0; i < read_vector_count; i++) {
                if (queries[i].end_code >= 0) {
                    query_response_pack(&queries[i]);
                }
                /* else query has a custom end_code indicating that no
                 * response is to be sent.
                 */
            }
            /* All queries for conn parsed, send conn to UDP write query queue. */
            conn_fifo_enqueue_write(&vl->conn_udp_write_queue, conn);

        } else {
            /* TCP */
            conn_tcp_t *conn_tcp = conn->conn.tcp;

            queries = conn_tcp->queries;
            for (int i = 0; i < conn_tcp->queries_count; i++) {
                if (queries[i].end_code >= 0) {
                    query_response_pack(&queries[i]);
                }
                /* else query has a custom end_code indicating that no
                 * response is to be sent.
                 */
            }
            /* All queries for conn parsed, send conn to TCP write query queue. */
            conn_tcp->state           = TCP_CONN_ST_WAIT_FOR_WRITE;
            conn_tcp->timeout         = vl->loop_timestamp;
            conn_tcp->timeout.tv_sec += vl->cfg->tcp_query_send_timeout;
            conn_fifo_enqueue_write(&vl->conn_tcp_write_queue, conn);
        }
    }
}

/** Vectorloop function sends responses to resolved queries.
 * 
 * @param vl Vectorloop operating on.
 */
static int
vl_fn_tcp_write(vectorloop_t *vl)
{
    conn_t           *conn;
    conn_tcp_t       *conn_tcp  = NULL;
    size_t            write_len = 0;
    ssize_t           ret       = 0;
    bool              done      = false;
    conn_fifo_queue_t new_queue = {};
    int               count     = 0;

    while ((conn = conn_fifo_dequeue_write(&vl->conn_tcp_write_queue)) != NULL) {
        conn_tcp = conn->conn.tcp;
        done     = false;

        for (int i = conn_tcp->query_write_index; i < conn_tcp->queries_count; i++) {
            query_t *query = &conn_tcp->queries[i];
            INCREMENT(count);
            if (query->end_code >= 0) {
                write_len = query->response_buffer_len - conn_tcp->write_index;
                ret =  write(conn->fd,
                             &query->response_buffer[conn_tcp->write_index],
                              write_len);

                if (ret == write_len) {
                    /* Full query written. */
                    utl_clock_gettime_rt_fatal(&query->end_time);
                    conn_tcp->write_index = 0;
                    continue;
                }
                if (ret > 0) {
                    /* Partial write. Enqueue conn to try again next vector loop. */
                    conn_tcp->write_index      += ret;
                    conn_tcp->query_write_index = i;
                    conn_fifo_enqueue_write(&new_queue, conn);
                    done = true; /* Don't enqueue to query log queue */
                } else if (ret < 0) {
                    if (ret == EAGAIN || ret == EWOULDBLOCK) {
                        /* Need to wait for epoll to tell us when we can write again. */
                        done = true; /* Don't enqueue to query log queue */
                    } else {
                        /* Some unrecoverable error occurred. Could be that far 
                         * end closed (reset) connection before we had a chance
                         * to send everything.
                         */
                        query->end_code = rip_ns_r_rip_tcp_write_err;
                        conn_tcp->state = TCP_CONN_ST_WRITE_ERR;
                        utl_clock_gettime_rt_fatal(&conn_tcp->end_time);
                    }
                } else {
                    /* ret == 0, connection was closed before we had a chance
                     * to send back the response.
                     */
                    query->end_code = rip_ns_r_rip_tcp_write_close;
                    conn_tcp->state = TCP_CONN_ST_CLOSED_FOR_WRITE;
                    utl_clock_gettime_rt_fatal(&conn_tcp->end_time);
                }
                break;
            }
        }
        if (done == true) {
            continue;
        }

        /* Move conn to log queue. */
        conn_fifo_enqueue_gen(&vl->query_log_queue, conn);
    }

    /* Requeue any connections back into TCP write queue. */
    if (new_queue.head != NULL) {
        vl->conn_tcp_write_queue.head = new_queue.head;
        vl->conn_tcp_write_queue.tail = new_queue.head;
    }

    return count;
}

/** Vectorloop function that writes DNS query responses on UDP connections.
 * 
 * @param vl Vectorloop operating on.
 * 
 * @return   Returns number of UDP queries responses sent.
 */
static int
vl_fn_udp_write(vectorloop_t *vl)
{
    conn_t           *conn;
    conn_udp_t       *conn_udp            = NULL;
    int               ret                 = 0;
    struct mmsghdr   *read_vector         = NULL;
    struct mmsghdr   *write_vector        = NULL;
    unsigned int      write_vector_count  = 0;
    query_t          *queries             = NULL;
    conn_fifo_queue_t new_queue           = {};

    while ((conn = conn_fifo_dequeue_write(&vl->conn_udp_write_queue)) != NULL) {
        conn_udp     = conn->conn.udp;
        read_vector  = conn_udp->read_vector;
        write_vector = conn_udp->write_vector;
        queries      = conn_udp->queries;

        /* Populate write vector from queries. */
        for (int i = 0; i < conn_udp->read_vector_count; i++) {
            if (queries[i].end_code > -1) {
                struct msghdr *msg_hdr = &write_vector[write_vector_count].msg_hdr;

                msg_hdr->msg_control       = read_vector[i].msg_hdr.msg_control;
                msg_hdr->msg_controllen    = read_vector[i].msg_hdr.msg_controllen;
                msg_hdr->msg_flags         = 0;
                msg_hdr->msg_iov->iov_base = queries[i].response_buffer;
                msg_hdr->msg_iov->iov_len  = queries[i].response_buffer_len;
                msg_hdr->msg_name          = read_vector[i].msg_hdr.msg_name;
                msg_hdr->msg_namelen       = read_vector[i].msg_hdr.msg_namelen;
                write_vector_count += 1;
            }
        }
        conn_udp->write_vector_count = write_vector_count;

        ret = sendmmsg(conn->fd,
                       &write_vector[conn_udp->write_vector_write_index],
                       write_vector_count,
                       0);

        /* Set query end time */
        if (ret > 0) {
            struct timespec ts;
            utl_clock_gettime_rt_fatal(&ts);
            unsigned int index = conn_udp->write_vector_write_index;
            for (int i = 0; i < ret; i++) {
                queries[index + i].end_time = ts;
            }
        }

        if (ret == write_vector_count) {
            /* All messages sent out, move conn to log queue. */
            conn_fifo_enqueue_gen(&vl->query_log_queue, conn);
        } else if (ret < 0) {
            /* Error sending.*/
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                /* Need to wait for writable. When epoll event that this conn
                 * is writable arrives, this conn will be put back into
                 * conn_udp_write_queue so queries could be sent.
                 */
                conn->waiting_for_write = 1;
            } else {
                /* UDP write error occurred. */
                char *err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
                CHECK_MALLOC(err_str);
                snprintf(err_str, ERR_MSG_LENGTH, "vl_fn_udp_read: UDP write error, %s", strerror(errno));
                channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, false);
                channel_log_send(vl->app_log_channel, lmsg);

                /* Put conn back into write queue. */
                conn_fifo_enqueue_write(&new_queue, conn);
            }
        } else {
            /* (ret != write_vector_count)
             * Not all messages were sent, need to try again. Update marker
             * where to start sending messages from and update vector count.
             */
            conn_udp->write_vector_write_index += ret;
            conn_udp->write_vector_count       -= ret;

            /* Put conn back into write queue! */
            conn_fifo_enqueue_write(&new_queue, conn);
        }
    }

    /* Requeue any connections back into UDP write queue. */
    if (new_queue.head != NULL) {
        vl->conn_udp_write_queue.head = new_queue.head;
        vl->conn_udp_write_queue.tail = new_queue.tail;
    }

    return ret;
}

/** Vectorloop function to log processed DNS queries. 
 * 
 * @param vl Vectorloop operating on.
 */
static void
vl_fn_query_log(vectorloop_t *vl)
{
    conn_t     *conn;
    conn_udp_t *conn_udp;
    conn_tcp_t *conn_tcp;
    int         len       = 0;

    while ((conn = conn_fifo_dequeue_gen(&vl->query_log_queue)) != NULL) {
        if (CONN_IS_UDP_LISTENER(conn)) {
            /* UDP */
            conn_udp = conn->conn.udp;

            for (int i =0; i < conn_udp->read_vector_count; i++) {
                len = query_log(vl->query_log.buf + vl->query_log.buf_len,
                                vl->query_log.buf_size - vl->query_log.buf_len,
                                &conn_udp->queries[i]);
                if (len > 0) {
                    vl->query_log.buf_len += len;
                } else {
                    /* error logging query, not enough room in buf. */
                    atomic_fetch_add(&vl->metrics->app.query_log_buf_no_space, 1);
                }

                query_report_metrics(&conn_udp->queries[i], vl->metrics);
            }

            /* Move UDP conn to read queue. */
            conn_fifo_enqueue_read(&vl->conn_udp_read_queue, conn);

        } else if (CONN_IS_TCP_CONN(conn)) {
            /* TCP */
            conn_tcp = conn->conn.tcp;

            for (int i = 0; i < conn_tcp->queries_count; i++) {
                len = query_log(vl->query_log.buf + vl->query_log.buf_len,
                                vl->query_log.buf_size - vl->query_log.buf_len,
                                &conn_tcp->queries[i]);
                if (len > 0) {
                    vl->query_log.buf_len += len;
                } else {
                    /* error logging query, not enough room in buf. */
                    atomic_fetch_add(&vl->metrics->app.query_log_buf_no_space, 1);
                }

                query_report_metrics(&conn_tcp->queries[i], vl->metrics);
            }

            /* If TCP connection is closed for write release conn oject. */
            if (conn_tcp->state == TCP_CONN_ST_CLOSED_FOR_WRITE ||
                conn_tcp->state == TCP_CONN_ST_WRITE_ERR) {
                conn_fifo_enqueue_release(&vl->conn_tcp_release_queue, conn);
                continue;
            }

            /* All messages sent. If there is any extra data (not associated with
            * query) in read_buffer it should be moved to the begining of buffer.
            * Set the timeout appropriately.
            */
            conn_tcp->timeout = vl->loop_timestamp;
            size_t data_len   = 0;
            for (int i = 0; i < conn_tcp->queries_count; i++) {
                data_len += conn_tcp->queries[i].request_buffer_len + 2;
            }
            size_t data_extra = conn_tcp->read_buffer_len - data_len;
            if (data_extra > 0) {
                memmove(conn_tcp->read_buffer, &conn_tcp->read_buffer[data_len], data_extra);
                conn_tcp->state           = TCP_CONN_ST_WAIT_FOR_QUERY_DATA;
                conn_tcp->timeout.tv_sec += vl->cfg->tcp_query_recv_timeout;
            } else {
                conn_tcp->state           = TCP_CONN_ST_WAIT_FOR_QUERY;
                conn_tcp->timeout.tv_sec += vl->cfg->tcp_keepalive;
            }
            conn_tcp->read_buffer_len = data_extra;

            /* Move conn to read queue. */
            conn_fifo_enqueue_read(&vl->conn_tcp_read_queue, conn);
        }
    }
}

/** Vectorloop function to check for timed out entries in TCP conn LRU cache.
 * 
 * @param vl Vectorloop operating on.
 */
static void
vl_fn_tcp_conn_timeouts(vectorloop_t *vl)
{
    conn_t     *conn, *tmp_conn;
    conn_tcp_t *conn_tcp;

    HASH_ITER(hh, vl->conn_tcp_lru_cache, conn, tmp_conn) {
        /* Check if connection timeout occurred? */
        conn_tcp = conn->conn.tcp;
        if (utl_diff_timespec_as_double(&conn_tcp->timeout, &vl->loop_timestamp) < 0) {
            /* Timeout occurred. */
            conn_fifo_enqueue_release(&vl->conn_tcp_release_queue, conn);
        } else {
            break;
        }
    }
}

/** Vectorloop function to release TCP connection objects. These are TCP
 * connections that have ended.
 * 
 * @param vl Vectorloop operating on.
 */
static void
vl_fn_tcp_conn_release(vectorloop_t *vl)
{
    conn_t *conn;

    while ((conn = conn_fifo_dequeue_release(&vl->conn_tcp_release_queue)) != NULL) {
        /* Remove TCP conn from TCP CONN TRACK LRU HASH. */
        LRU_CACHE_DEL(vl->conn_tcp_lru_cache, conn);

        /* Deregister fd from epoll. */
        if (conn->fd >= 0) {
            vl_epoll_ctl_del(vl->ep_fd_tcp, conn->fd);
            close(conn->fd);
            conn->fd = -1;
        }

        /* Remove conn from read or write queue. Conn could be in either one of
         * these queues if a timeout was detected.
         */
        conn_fifo_remove_from_read_queue(&vl->conn_tcp_read_queue, conn);
        conn_fifo_remove_from_write_queue(&vl->conn_tcp_write_queue, conn);

        /* Report TCP metrics. */
        conn_tcp_report_metrics(conn->conn.tcp, vl->metrics);

        conn_release(conn);
        DECREMENT(vl->conns_tcp_active);
    }
}

/** Register (start) UDP and TCP listeners for this vectorloop, both IPv4 and IPv6.
 * 
 * @param vl Vectorloop operating on.
 */
static void
vl_register_listeners(vectorloop_t *vl)
{
    char   *err_str;
    conn_t *conn = NULL;

    err_str = malloc(sizeof(char) * ERR_MSG_LENGTH);
    CHECK_MALLOC(err_str);

    /* Start UDP listening sockets */
    if (vl->cfg->udp_enable) {
        /* Start UDP IPv4 listener. */
        conn = conn_listener_provision(vl->cfg, AF_INET, IPPROTO_UDP, err_str,
                                       ERR_MSG_LENGTH);
        if (conn == NULL) {
            channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, true);
            channel_log_send(vl->app_log_channel, lmsg);
            return;
        }

        /* Register IPv4 listener fd to epoll for read & write edge triggered. */
        vl_epoll_ctl_reg_for_readwrite_et(vl->ep_fd_udp, conn->fd, (uint64_t)conn);

        /* Add IPv4 listener to Vectorloop UDP read queue. */
        conn_fifo_enqueue_read(&vl->conn_udp_read_queue, conn);

        vl->listener_udp_ipv4 = conn;
        debug_printf("VL ID %d IPv4 UDP listener started", vl->id);

        /* Start UDP IPv6 listener. */
        conn = conn_listener_provision(vl->cfg, AF_INET6, IPPROTO_UDP, err_str,
                                       ERR_MSG_LENGTH);
        if (conn == NULL) {
            channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, true);
            channel_log_send(vl->app_log_channel, lmsg);
            return;
        }

        /* Register IPv6 listener fd to epoll for read & write edge triggered. */
        vl_epoll_ctl_reg_for_readwrite_et(vl->ep_fd_udp, conn->fd, (uint64_t)conn);

        /* Add IPv6 listener to Vectorloop UDP read queue. */
        conn_fifo_enqueue_read(&vl->conn_udp_read_queue, conn);

        vl->listener_udp_ipv6 = conn;
        debug_printf("VL ID %d IPv6 UDP listener started", vl->id);
    }

    /* Start TCP listening sockets */
    if (vl->cfg->tcp_enable) {
        /* Start TCP IPv4 listener. */
        conn = conn_listener_provision(vl->cfg, AF_INET, IPPROTO_TCP, err_str,
                                       ERR_MSG_LENGTH);
        if (conn == NULL) {
            channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, true);
            channel_log_send(vl->app_log_channel, lmsg);
            return;
        }

        /* Register IPv4 listener fd to epoll for read & write edge triggered. */
        vl_epoll_ctl_reg_for_readwrite_et(vl->ep_fd_tcp, conn->fd, (uint64_t)conn);

        /* Add IPv4 listener to Vectorloop TCP read queue. */
        conn_fifo_enqueue_read(&vl->conn_tcp_accept_conns_queue, conn);

        vl->listener_tcp_ipv4 = conn;
        debug_printf("VL ID %d IPv4 TCP listener started", vl->id);


        /* Start TCP IPv6 listener. */
        conn = conn_listener_provision(vl->cfg, AF_INET6, IPPROTO_TCP, err_str,
                                       ERR_MSG_LENGTH);
        if (conn == NULL) {
            channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_CUSTOM, err_str, true);
            channel_log_send(vl->app_log_channel, lmsg);
            return;
        }

        /* Register IPv6 listener fd to epoll for read & write edge triggered. */
        vl_epoll_ctl_reg_for_readwrite_et(vl->ep_fd_tcp, conn->fd, (uint64_t)conn);

        /* Add IPv6 listener to Vectorloop TCP read queue. */
        conn_fifo_enqueue_read(&vl->conn_tcp_accept_conns_queue, conn);

        vl->listener_tcp_ipv6 = conn;
        debug_printf("VL ID %d IPv6 TCP listener started", vl->id);
    }

    free(err_str);

}

/** Create a new vectorloop object
 * 
 * @param cfg               Configuration with settings.
 * @param id                Vectorloop ID.
 * @param res_ch            Resource channel used for this vectorloop.
 * @param app_log_channel   Application log channel used for this vectorloop.
 * @param query_log_channel Query log channel used for this vectorloop.
 * @param metrics           Metrics object vectorloop to use.
 *
 * @return                  Returns newly created vectorloop object. 
 */
vectorloop_t *
vl_new(config_t *cfg, int id, channel_bss_t *res_ch,
       channel_log_t *app_log_channel, channel_bss_t *query_log_channel,
       metrics_t *metrics) {
    /* Init new vectorloop object. */
    vectorloop_t *vl = malloc(sizeof(vectorloop_t));
    CHECK_MALLOC(vl);
    *vl = (vectorloop_t) {
        .cfg               = cfg,
        .id                = id,
        .resource_channel  = res_ch,
        .app_log_channel   = app_log_channel,
        .query_log_channel = query_log_channel,
        .metrics           = metrics,
        .ep_fd_tcp         = -1,
        .ep_fd_udp         = -1,
    };

    /* Allocate epoll events array. */
    int num_events = cfg->epoll_num_events_udp > cfg->epoll_num_events_tcp ?
                     cfg->epoll_num_events_udp : cfg->epoll_num_events_tcp;
    vl->ep_events = malloc(sizeof(struct epoll_event) * num_events);
    CHECK_MALLOC(vl->ep_events);

    /* Create epoll fd. */
    vl->ep_fd_udp = vl_epoll_create();

    /* Create epoll fd. */
    vl->ep_fd_tcp = vl_epoll_create(); 

    /* Allocate query log buffers. */
    vl->query_log.a_buf    = malloc(vl->cfg->query_log_buffer_size);
    CHECK_MALLOC(vl->query_log.a_buf);
    vl->query_log.b_buf    = malloc(vl->cfg->query_log_buffer_size);
    CHECK_MALLOC(vl->query_log.b_buf);
    vl->query_log.buf      = vl->query_log.a_buf;
    vl->query_log.buf_size = vl->cfg->query_log_buffer_size;
    vl->query_log.buf_len  = 0;

    return vl;
}

/** Main Vectorloop function (loop) that receives DNS queries, processes then,
 * sends and logs responses.
 *
 * This is a continuous loop that slows it self down if there was no data to
 * process. Slowdown is done so we do not run CPU hot needlessly.
 * Policy is to slow down (sleep) after 8 iteration by 10us, then 8 more
 * iterations by 50us, then by 1ms. from there on. If data is received the
 * slowdown loop stages start over.
 * 
 * @param arg Pointer to vectorloop object to run. Argument is of type void* as
 *            this function is invoked by pthread_create().
 * 
 * @return    NULL pointer returned upon termination.
 */
void *
vl_run(void *arg)
{
    vectorloop_t *vl       = (vectorloop_t *)arg;
    int           loop_end = 0;
    cpu_set_t     cpu_set;
    int           affinity;

    /* Set CPU affinity. */
    if (vl->cfg->process_thread_masks[vl->id] > 0) {
        CPU_ZERO(&cpu_set);
        CPU_SET(vl->cfg->process_thread_masks[vl->id]-1, &cpu_set);
        affinity = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
        if (affinity < 0) {
            channel_log_msg_t *lmsg = channel_log_msg_create(APP_LOG_MSG_VL_RUN_CPU_AFFINITY, NULL, false);
            channel_log_send(vl->app_log_channel, lmsg);
        }
    }

    /* Initialize admin channel on this thread(core). */
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

    /* Start listeners. */
    vl_register_listeners(vl);
    
    /* Vector loop. */
    while (!loop_end)
    {
        /* ret is used to track how many queries were processed in a loop
         * with purpose of slowing down the loop frequency so we do not burn up
         * the CPU unnecessarily.
         */
        int ret = 0;

        /* Get timestamp of loop iteration. */
        utl_clock_gettime_rt_fatal(&vl->loop_timestamp); 

        /* Check for messages on channels from other treads */
        ret += vl_fn_channel_messages(vl);

        /* Check epoll for events. */
        ret += vl_fn_epoll(vl);

        /* Read data in from UDP sockets. */
        ret += vl_fn_udp_read(vl);

        /* Accept new TCP connections. */
        ret += vl_fn_tcp_accept_conns(vl);

        /* Read data from TCP connections. */
        ret += vl_fn_tcp_read(vl);

        /* Parse data into queries. */
        vl_fn_query_parse(vl);

        /* Resolve queries into answers. */
        vl_fn_query_resolve(vl);

        vl_fn_query_response_pack(vl);

        /* Send queries answers UDP. */
        ret += vl_fn_udp_write(vl);

        /* Send queries answers TCP. */
        ret += vl_fn_tcp_write(vl);

        /* Log queries. */
        vl_fn_query_log(vl);

        /* Check TCP LRU cache for timeouts. */
        vl_fn_tcp_conn_timeouts(vl);

        /* Release TCP connection objects. */
        vl_fn_tcp_conn_release(vl);

        /* Idle backoff time. */
        if (ret == 0) {
            /* Need to slow down the loop as there was nothing to process. */
            INCREMENT(vl->idle_count);
            if (vl->idle_count < 8) {
               usleep(vl->cfg->loop_slowdown_one);
            } else if (vl->idle_count < 16) {
               usleep(vl->cfg->loop_slowdown_two);
            } else {
                usleep(vl->cfg->loop_slowdown_three);
            }
        } else if (vl->idle_count != 0) {
            vl->idle_count = 0;
        }
    }

    return NULL;
}
