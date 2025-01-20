/**
 * @file metrics.h
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
/** \defgroup metrics Metrics
 * 
 * @brief Metrics is a single structure composed of atomic variables used a
 *        counters. Atomic operations on numbers is quite quick hence no
 *        need to have per thread metrics that would then need to be
 *        colleted and aggregated into one.
 *  @{
 */
#ifndef METRICS_H
#define METRICS_H

#include <stdatomic.h>

/** Structure holds global metrics application collects.
 * Metrics are atomic variables treated as counters.
 */
typedef struct metrics_s {

    /** Structure holds TCP related metrics. */
    struct {
         /** Number of new connections accepted */
        atomic_ullong connections;

        /** Number of queries received */
        atomic_ullong queries;
        
        /** Number of times TCP connection was accepted for unsupported
         * Client IP family. Supported families are IPv4 and IPv6.
         * This condition should never happen as application only binds to
         * IPv4 and IPv6.
         */
        atomic_ullong unknown_client_ip_soc_family;

        /**  Number of times call to getsockname() for TCP connection
         * failed.
         */
        atomic_ullong getsockname_err;

        /** Number of times TCP connection was accepted for unsupported
         * local IP family. Supported families are IPv4 and IPv6.
         * This condition should never happen as application only binds to
         * IPv4 and IPv6.
         */
        atomic_ullong unknown_local_ip_soc_family;
        
        /** Number of times we could not assign a connection ID */
        atomic_ullong conn_id_unavailable;

        /** Number of times we received a query with datagram size larger than 512. */
        atomic_ullong query_len_toolarge;

        /** Number of queries that were partially received and then timed
         * out receiving the full query.
         * Any deviation from steady state could be an indication of DOS.
         */
        atomic_ullong query_recv_timeout;

        /** Number of connections that were closed due to idle (keepalive)
         * timeout.
         */
        atomic_ullong keepalive_timeout;

        /** Number of connections that were closed because not query was
         * ever received on them. I.e. TCP connection was accepted then
         * no data followed, then conenction was closed by far end.
         * Any deviation from steady state could be an indication of DOS.
         */
        atomic_ullong closed_no_query;

        /** Number of connections that were closed because full query was
         * ever received on them. I.e. TCP connection was accepted then
         * partial query data followed, then TCP conenction was closed by far end.
         * Any deviation from steady state could be an indication of DOS.
         */
        atomic_ullong closed_partial_query;

        /** Number of connections that were closed because a socket
         * read error occurred.
         */
        atomic_ullong sock_read_err;

        /** Number of connections that were closed because a socket
         * write error occurred.
         */
        atomic_ullong sock_write_err;

        /** Number of connections that were closed because a socket
         * write timeout occurred.
         */
        atomic_ullong sock_write_timeout;

        /** Number of connections that were closed because a socket
         * was closed for write.
         */
        atomic_ullong sock_closed_for_write;
    } tcp;

    /** Structure holds UDP IPv4 related metrics. */
    struct {
        /** Number of queries received */
        atomic_ullong queries;
    } udp;

    /** Structure holds DNS related metrics. */
    struct {
        /** Number of queries received. */
        atomic_ullong queries;

        /** Number of queries responded to with rcode NOERROR */
        atomic_ullong queries_rcode_noerror;

        /** Number of queries responded to with rcode FORMERR */
        atomic_ullong queries_rcode_formerr;

        /** Number of queries responded to with rcode SERVFAIL */
        atomic_ullong queries_rcode_servfail;

        /** Number of queries responded to with rcode NXDOMAIN */
        atomic_ullong queries_rcode_nxdomain;

        /** Number of queries responded to with rcode NOTIMPL */
        atomic_ullong queries_rcode_notimpl;

        /** Number of queries responded to with rcode REFUSED */
        atomic_ullong queries_rcode_refused;

        /** Number of queries with rcode shortheader */
        atomic_ullong queries_rcode_shortheader;

        /** Number of queries with rcode too large */
        atomic_ullong queries_rcode_toolarge;

        /** Number of queries responded to with rcode bad version.
         * This happens when EDNS version in request is not supported.
         */
        atomic_ullong queries_rcode_badversion;

        /** Number of query request received with question type being invalid. */
        atomic_ullong queries_type_invalid;

        /** Number of query request received with question of type A. */
        atomic_ullong queries_type_A;

        /** Number of query request received with question of type AAAA. */
        atomic_ullong queries_type_AAAA;

        /** Number of query request received with question of type CNAME. */
        atomic_ullong queries_type_CNAME;

        /** Number of query request received with question of type MX. */
        atomic_ullong queries_type_MX;

        /** Number of query request received with question of type NS. */
        atomic_ullong queries_type_NS;

        /** Number of query request received with question of type PTR. */
        atomic_ullong queries_type_PTR;

        /** Number of query request received with question of type SRV. */
        atomic_ullong queries_type_SRV;

        /** Number of query request received with question of type SOA. */
        atomic_ullong queries_type_SOA;

        /** Number of query request received with question of type TXT. */
        atomic_ullong queries_type_TXT;

        /** Number of query request received with question of unsupported type. */
        atomic_ullong queries_type_unsupported;

        atomic_ullong queries_edns_present;
        
        atomic_ullong queries_edns_valid;

        atomic_ullong queries_edns_dobit;

        atomic_ullong queries_clientsubnet;

    } dns;

    /** Structure holds application related metrics.  */
    struct {
        /** Number of times opening application lgo file resulted in error. */
        atomic_ullong app_log_open_error;

        /** Number of times writing to application lgo file resulted in error. */
        atomic_ullong app_log_write_error;

        /** Number of times writing to query log buffer resulted in not having
         * enough space to log the query. It is OK to have this happen under
         * extrem load such as an intense DOS attack. 
         * If this happens under temporary traffic spikes it might be necessary
         * to increase the query log buffer size.
         */
        atomic_ullong query_log_buf_no_space;

        /**  Number of times checking (and if changed reloading) a resource
         * failed. Increase in this counter indicate that something happened to
         * the resource and we might be serving stale responses.
        */
        atomic_ullong resource_reload_error;

        /* Number of times opening query log file failed.*/
        atomic_ullong query_log_open_error;
    } app;

} metrics_t;

#endif /* End of METRICS_H */

/** @}*/