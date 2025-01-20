/**
 * @file conn_tcp_report_metrics.c
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
#include <stdatomic.h>
#include <sys/socket.h>

#include "conn.h"
#include "metrics.h"

/** Report TCP metrics for a query.
 * 
 * @param conn_tcp TCP connection to report metrics for.
 * @param metrics  Metrics object where to report metrics.
 */
void
conn_tcp_report_metrics(conn_tcp_t *conn_tcp, metrics_t *metrics)
{
    switch (conn_tcp->state) {
    case TCP_CONN_ST_ASSIGN_CONN_ID_ERR:
        atomic_fetch_add(&metrics->tcp.conn_id_unavailable, 1);
        break;

    case TCP_CONN_ST_QUERY_SIZE_TOOLARGE:
        atomic_fetch_add(&metrics->tcp.query_len_toolarge, 1);
        break;

    case TCP_CONN_ST_CLOSED_FOR_READ:
        if (conn_tcp->read_buffer_len != 0) {
            atomic_fetch_add(&metrics->tcp.closed_partial_query, 1);
        } else if (conn_tcp->queries_count == 0) {
            atomic_fetch_add(&metrics->tcp.closed_no_query, 1);
        }
        break;

    case TCP_CONN_ST_CLOSED_FOR_WRITE:
        atomic_fetch_add(&metrics->tcp.sock_closed_for_write, 1);
        break;

    case TCP_CONN_ST_READ_ERR:
        atomic_fetch_add(&metrics->tcp.sock_read_err, 1);
        break;

    case TCP_CONN_ST_WAIT_FOR_QUERY:
        atomic_fetch_add(&metrics->tcp.keepalive_timeout, 1);
        break;

    case TCP_CONN_ST_WAIT_FOR_QUERY_DATA:
        atomic_fetch_add(&metrics->tcp.query_recv_timeout, 1);
        break;

    case TCP_CONN_ST_WAIT_FOR_WRITE:
        atomic_fetch_add(&metrics->tcp.sock_write_timeout, 1);
        break;

    case TCP_CONN_ST_WRITE_ERR:
        atomic_fetch_add(&metrics->tcp.sock_write_err, 1);
        break;

    default:
        break;
    }

}