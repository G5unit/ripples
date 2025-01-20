/**
 * @file query_log.c
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
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "query.h"
#include "rip_ns_utils.h"
#include "utils.h"

/** Rotate query log.
 * 
 * Active log to write queries is switched to the other side.
 * 
 * @param query_log Query log to rotate.
 */
void
query_log_rotate(query_log_t *query_log)
{
    if (query_log->buf == query_log->a_buf) {
        /* Switch from A to B. */
        query_log->a_buf_len = query_log->buf_len;

        query_log->buf     = query_log->b_buf;
        query_log->buf_len = query_log->b_buf_len = 0;
    } else {
        /* Switch from B to A. */
        query_log->b_buf_len = query_log->buf_len;

        query_log->buf     = query_log->a_buf;
        query_log->buf_len = query_log->a_buf_len = 0;
    } 
}

/** Log query to buffer.
 *
 * Query log entry ends with new line "\n" (one query per line).
 * 
 * @param buf     Buffer to log query to.
 * @param buf_len Length of buffer (available space).
 * @param q       Query to log.
 *
 * @return        Returns number of bytes added to buf on success,
 *                otherwise returns 0 and error occurred (ie not enough
 *                room in buffer, or not a logable query end code) and nothing
 *                was added to buf.
 *                
 */
/*
query log format:

client ip, client port, local ip, local port, protocol(TCP|UDP)

time received, time response sent

request:
     (per HEADER): rd, tc, opcode
     edns opts: edns version, DO bit (dnssec), udp response len
              : client subnet: ip, source, scope
     question name, type, class

response:
     (per HEADER): aa, rcode, ra
     edns opts: edns version, DO bit (dnssec), udp response len
              : client subnet: ip, source, scope

     answer section: name, type, class, ttl, rdata (how to reuse name, type, class?)

     authority section: name, type, class, ttl, rdata

     additional section: name, type, class, ttl, rdata

*/
int
query_log(char *buf, size_t buf_len, query_t *q)
{
    char     *buf_start = buf;
    char      client_ip[INET6_ADDRSTRLEN];
    uint16_t  client_port;
    int       client_ip_len;
    char      local_ip[INET6_ADDRSTRLEN];
    uint16_t  local_port;
    int       local_ip_len;

    struct sockaddr_in  *sin;
    struct sockaddr_in6 *sin6;

    char one  = '1';
    char zero = '0';

    const char *cstr;
    size_t      cstr_len;

    /* Space in the buffer MUST be at least 64K or assume there is not enough
     * room to write full query log. We purposefully enforce this
     * requirement to speed up logging by not having to check if there is 
     * room before each memcpy() of sprintf() call.
     */
    if (buf_len < QUERY_LOG_BUF_MIN_SPACE) {
        return 0;
    }

    /* client and local IP & port. */
    if (q->client_ip->ss_family == AF_INET) {
        sin = (struct sockaddr_in *)q->client_ip;
        inet_ntop(AF_INET, &sin->sin_addr, client_ip, INET6_ADDRSTRLEN);
        client_port = ntohs(sin->sin_port);
        sin = (struct sockaddr_in *)q->local_ip;
        inet_ntop(AF_INET, &sin->sin_addr, local_ip, INET6_ADDRSTRLEN);
        local_port = ntohs(sin->sin_port);
    } else {
        sin6 = (struct sockaddr_in6 *)q->client_ip;
        inet_ntop(AF_INET6, &sin6->sin6_addr, client_ip, INET6_ADDRSTRLEN);
        client_port = ntohs(sin6->sin6_port);
        sin6 = (struct sockaddr_in6 *)q->local_ip;
        inet_ntop(AF_INET6, &sin6->sin6_addr, local_ip, INET6_ADDRSTRLEN);
        local_port = ntohs(sin6->sin6_port);
    }
    client_ip_len = strlen(client_ip);
    local_ip_len  = strlen(local_ip);

    memcpy(buf, "{\"c_ip\":\"", 9);
    buf += 9;
    memcpy(buf, client_ip, client_ip_len);
    buf += client_ip_len;
    memcpy(buf,"\",\"c_port\":\"", 12);
    buf += 12;
    buf += sprintf(buf, "%u", client_port);
    memcpy(buf, "\",\"l_ip\":\"", 10);
    buf += 10;
    memcpy(buf, local_ip, local_ip_len);
    buf += local_ip_len;
    memcpy(buf,"\",\"l_port\":\"", 12);
    buf += 12;
    buf += sprintf(buf, "%u", local_port);

    /* receive and send timestamps. */
    memcpy(buf, "\",\"recv_time\":\"", 15);
    buf += 15;
    buf += utl_timespec_to_rfc3339nano(&q->start_time, buf);

    if (q->end_code >= 0) {
        /* Negative end code indicates that response was not sent hence no
         * send time.
         */
        memcpy(buf, "\",\"send_time\":\"", 15);
        buf += 15;
        buf += utl_timespec_to_rfc3339nano(&q->end_time, buf);
        memcpy(buf, "\"", 1);
        buf += 1;
    }

    if (q->end_code != rip_ns_r_noerror &&  q->end_code <= rip_ns_r_formerr ) {
        /* Error code so do not log anything else. */
        memcpy(buf, "}\n", 2);
        buf += 2;
        return buf - buf_start;
    }

    /* Request HEADER: rd, tc, opcode
     *  edns opts: edns version, DO bit (dnssec), udp response len
     *          : client subnet: ip, source, scope
     *  question name, type, class
     *
     */
    memcpy(buf, ",\"request\":{\"rd\":\"", 18);
    buf += 18;
    if (q->request_hdr->rd == 0) {
        *buf = zero;
    } else {
        *buf = one;
    }
    buf += 1;
    memcpy(buf, "\",\"tc\":\"", 8);
    buf += 8;
    if (q->request_hdr->tc == 0) {
        *buf = zero;
    } else {
        *buf = one;
    }
    buf += 1;
    memcpy(buf,"\",\"opcode\":\"query\"", 18);
    buf += 18;
    
    /* edns */
    if (q->edns.edns_valid || q->end_code == rip_ns_r_badvers) {
        memcpy(buf,",\"edns\":{\"resp_size\":\"", 22);
        buf += 22;
        buf += sprintf(buf, "%u", q->edns.udp_resp_len);
        memcpy(buf, "\",\"ver\":\"", 9);
        buf += 9;
        buf += sprintf(buf, "%d", q->edns.version);

        if (q->edns.edns_valid) {
            memcpy(buf, "\",\"do\":\"", 8);
            buf += 8;
            if (q->edns.dnssec) {
                *buf = one;
            } else {
                *buf = zero;
            }
            buf += 1;
            *buf = '\"';
            buf += 1;
            /* edns client subnet */
            if (q->edns.client_subnet.edns_cs_valid) {
                memcpy(buf, ",\"cs\":{\"ip\":\"", 13);
                buf += 13;
                if (q->edns.client_subnet.family == 1) {
                    sin = (struct sockaddr_in *)&q->edns.client_subnet.ip;
                    inet_ntop(AF_INET, &sin->sin_addr, client_ip, INET6_ADDRSTRLEN);
                } else {
                    sin6 = (struct sockaddr_in6 *)&q->edns.client_subnet.ip;
                    inet_ntop(AF_INET6, &sin6->sin6_addr, client_ip, INET6_ADDRSTRLEN);
                }
                client_ip_len = strlen(client_ip);
                memcpy(buf, client_ip, client_ip_len);
                buf += client_ip_len;
                memcpy(buf, "\",\"source\":\"", 12);
                buf += 12;
                buf += sprintf(buf, "%u", q->edns.client_subnet.source_mask);
                memcpy(buf, "\",\"scope\":\"", 11);
                buf += 11;
                buf += sprintf(buf, "%u", q->edns.client_subnet.scope_mask);
                memcpy(buf, "\"}", 2);
                buf += 2;
            }
        }
        memcpy(buf, "}", 1);
        buf += 1;
    }

    /* request question: q_name, q_type, q_class */
    memcpy(buf, ",\"q_name\":\"", 12);
    buf += 12;
    memcpy(buf, q->query_label, q->query_label_len);
    buf += q->query_label_len;
    memcpy(buf, "\",\"q_class\":\"", 13);
    buf += 13;
    cstr = rip_ns_class_to_str(q->query_q_class);
    cstr_len = strlen(cstr);
    memcpy(buf, cstr, cstr_len);
    buf += cstr_len;
    memcpy(buf, "\",\"q_type\":\"", 12);
    buf += 12;
    cstr = rip_ns_rr_type_to_str(q->query_q_type);
    cstr_len = strlen(cstr);
    memcpy(buf, cstr, cstr_len);
    buf += cstr_len;
    memcpy(buf, "\"}", 2);
    buf += 2;

    if (q->end_code == rip_ns_r_servfail ) {
        /* Error code so do not log response. */
        memcpy(buf, "}\n", 2);
        buf += 2;
        return buf - buf_start;
    }

    /* response. */
    if (q->answer_section_count > 0 || q->authority_section_count > 0 ||
        q->additional_section_count > 0) {
        memcpy(buf, ",\"response\":{", 13);
        buf += 13;
        if (q->answer_section_count > 0) {
            memcpy(buf, "\"answer\":[", 10);
            buf += 10;
            /* logs up to 10 entries from answer section */
            for (int i = 0; i < q->answer_section_count && i < 10; i++) {
                memcpy(buf, "{\"name\":\"", 9);
                buf += 9;
                memcpy(buf, q->answer_section[i]->name, q->answer_section[i]->name_len);
                buf += q->answer_section[i]->name_len;
                memcpy(buf, "\",\"class\":\"", 11);
                buf += 11;
                cstr = rip_ns_class_to_str(q->answer_section[i]->class);
                cstr_len = strlen(cstr);
                memcpy(buf, cstr, cstr_len);
                buf += cstr_len;
                memcpy(buf, "\",\"type\":\"", 10);
                buf += 10;
                cstr = rip_ns_rr_type_to_str(q->answer_section[i]->type);
                cstr_len = strlen(cstr);
                memcpy(buf, cstr, cstr_len);
                buf += cstr_len;
                /* Only A type questions are supported, hence only A type
                 * answers.
                 */
                memcpy(buf, "\",\"rdata\":\"", 11);
                buf += 11;
                inet_ntop(AF_INET, q->answer_section[i]->rdata, client_ip, INET6_ADDRSTRLEN);
                client_ip_len = strlen(client_ip);
                memcpy(buf, client_ip, client_ip_len);
                buf += client_ip_len;
                memcpy(buf, "\"},", 3);
                buf += 3;
            }
            buf -= 1; /* rewind buf to not include last ',' */
            *buf = ']';
            buf += 1;
        }

        /* end of response section */
        *buf = '}';
        buf += 1;
    }
    

    /* Add new line at end of each query log entry. */
    memcpy(buf, "}\n", 2);
    buf += 2;
    return buf - buf_start;
}
