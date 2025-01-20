/**
 * @file query_report_metrics.c
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

#include "query.h"
#include "metrics.h"

/** Report query metrics.
 * 
 * @param q       Query to report metrics for.
 * @param metrics Metrics object where to report metrics.
 */
void
query_report_metrics(query_t *q, metrics_t *metrics)
{

    atomic_ullong *atomic_ul = NULL;

    if (q->protocol == 0) {
        /* UDP */
        atomic_fetch_add(&metrics->udp.queries, 1);
    } else if (q->protocol == 1) {
        /* TCP */
        atomic_fetch_add(&metrics->tcp.queries, 1);
    }

    atomic_ul = NULL;
    switch (q->end_code) {
        case 0:
            atomic_ul = &metrics->dns.queries_rcode_noerror;
            break;
        case 1:
            atomic_ul = &metrics->dns.queries_rcode_formerr;
            break;
        case 2:
            atomic_ul = &metrics->dns.queries_rcode_servfail;
            break;
        case 3:
            atomic_ul = &metrics->dns.queries_rcode_nxdomain;
            break;
        case 4:
            atomic_ul = &metrics->dns.queries_rcode_notimpl;
            break;
        case 5:
            atomic_ul = &metrics->dns.queries_rcode_refused;
            break;
        case 16:
            atomic_ul = &metrics->dns.queries_rcode_badversion;
            break;
        case -2:
            atomic_ul = &metrics->dns.queries_rcode_shortheader;
            break;
        case -3:
            atomic_ul = &metrics->dns.queries_rcode_toolarge;
            break;
        default:
            break;
    }
    if (atomic_ul != NULL) {
        atomic_fetch_add(atomic_ul, 1);
    }

    atomic_ul = NULL;
    switch (q->query_q_type) {
    case 0:
        atomic_ul = &metrics->dns.queries_type_invalid;
        break;
    case 1:
        atomic_ul = &metrics->dns.queries_type_A;
        break;
    case 2:
        atomic_ul = &metrics->dns.queries_type_NS;
        break;
    case 5:
        atomic_ul = &metrics->dns.queries_type_CNAME;
        break;
    case 6:
        atomic_ul = &metrics->dns.queries_type_SOA;
        break;
    case 12:
        atomic_ul = &metrics->dns.queries_type_PTR;
        break;
    case 15:
        atomic_ul = &metrics->dns.queries_type_MX;
        break;
    case 16:
        atomic_ul = &metrics->dns.queries_type_TXT;
        break;
    case 28:
        atomic_ul = &metrics->dns.queries_type_AAAA;
        break;
    case 33:
        atomic_ul = &metrics->dns.queries_type_SRV;
        break;
    default:
        break;
    }
    if (atomic_ul != NULL) {
        atomic_fetch_add(atomic_ul, 1);
    }

    if (q->edns.edns_raw_buf_len > 0) {
        atomic_fetch_add(&metrics->dns.queries_edns_present, 1);
    }
    if (q->edns.edns_valid) {
        atomic_fetch_add(&metrics->dns.queries_edns_valid, 1);
    }
    if (q->edns.dnssec > 0) {
        atomic_fetch_add(&metrics->dns.queries_edns_dobit, 1);
    }
    if (q->edns.client_subnet.edns_cs_valid) {
        atomic_fetch_add(&metrics->dns.queries_clientsubnet, 1);
    }

}