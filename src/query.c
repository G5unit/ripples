/**
 * @file query.c
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
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "query.h"
#include "utils.h"

/** Initialize a query object.
 * 
 * @param q        Query object to initialize.
 * @param cfg      Configuration with settings to use.
 * @param protocol Transport protocol used for query.
 *                 Valid options are 0 = UDP, 1 = TCP.
 */
void query_init(query_t *q, config_t *cfg, uint8_t protocol)
{
    *q = (query_t) {};

    if (protocol == 0) {
        /* UDP. */
        q->client_ip = malloc(sizeof(struct sockaddr_storage));
        CHECK_MALLOC(q->client_ip);
        q->local_ip = malloc(sizeof(struct sockaddr_storage));
        CHECK_MALLOC(q->local_ip);

        /* request buffer size allocated is RIP_NS_PACKETSZ+1. The +1 is to be
         * be able to detect mallformed requests that exceed 512 bytes.
         */
        q->request_buffer = malloc(sizeof(unsigned char) * (RIP_NS_PACKETSZ+1));
        CHECK_MALLOC(q->request_buffer);
        q->request_buffer_size = (RIP_NS_PACKETSZ+1);
        q->request_hdr = (rip_ns_header_t *)q->request_buffer;

        q->response_buffer = malloc(sizeof(unsigned char) * RIP_NS_UDP_MAXMSG);
        CHECK_MALLOC(q->response_buffer);
        q->response_buffer_size = RIP_NS_UDP_MAXMSG;
        q->response_hdr = (rip_ns_header_t *)q->response_buffer;

    } else {
        /* TCP */
        q->protocol = 1;
        q->response_buffer = malloc(sizeof(unsigned char) * cfg->tcp_writebuff_size);
        q->response_buffer_size = cfg->tcp_writebuff_size;
        q->response_hdr = (rip_ns_header_t *)(q->response_buffer + 2);
    }

    q->query_label = malloc(sizeof(unsigned char) * (RIP_NS_MAXCDNAME+1));
    CHECK_MALLOC(q->query_label);
    q->query_label_size = RIP_NS_MAXCDNAME + 1;

    q->error_message[0] = '\0';

    q->dnptrs[0] = (unsigned char *)q->response_hdr;

    q->end_code = -1;
}

/** Reset query object so it is suitable to be reused for new query processing.
 * 
 * @param q Query to reset.
 */
void query_reset(query_t *q)
{
    q->request_buffer_len = 0;
    q->query_label_len    = 0;

    q->query_q_type  = rip_ns_t_invalid;
    q->query_q_class = rip_ns_c_invalid;

    q->edns.edns_raw_buf_len  = 0;
    q->edns.edns_valid        = 0;

    q->edns.client_subnet.edns_cs_raw_buf_len = 0;
    q->edns.client_subnet.edns_cs_valid       = 0;

    q->response_buffer_len = 0;

    q->error_message[0] = '\0';

    for (int i = 0; i < DNS_RESPONSE_COMPRESSED_NAMES_MAX; i++) {
        q->dnptrs[i] = NULL;
    }
    q->dnptrs[0] = (unsigned char *)q->response_hdr;

    q->answer_section_count     = 0;
    q->authority_section_count  = 0;
    q->additional_section_count = 0;

    q->end_code = rip_ns_r_rip_unknown;
}

/** Clean a query object. This releases all memory owned by query object.
 * 
 * @param q Query to clean.
 */
void query_clean(query_t *q)
{
    if (q->protocol == 0) {
        /* UDP specific . */
        free(q->client_ip);
        free(q->local_ip);
        free(q->request_buffer);
    }
    free(q->response_buffer);
    free(q->query_label);
}

/** Increase the query response buffer size. This is only valid if query is
 * used with TCP transport.
 * 
 * Response buffer size is increased via call to realloc() in increments of
 * @ref RIP_NS_UDP_MAXMSG up to max size of @ref RIP_NS_MAXMSG.
 * 
 * @param q Query whose response buffer to increase.
 *
 * @return  Returns 0 on success, otherwise an error occurred and response buffer
 *          was untouched.
 */
int
query_tcp_response_buffer_increase(query_t *q)
{
    size_t buf_size = q->response_buffer_size;
    if (buf_size >= RIP_NS_MAXMSG) {
        return -1;
    }

    buf_size += RIP_NS_UDP_MAXMSG;
    if (buf_size > RIP_NS_MAXMSG) {
        buf_size = RIP_NS_MAXMSG;
    }
    unsigned char *buf = realloc(q->response_buffer, buf_size);
    if (buf == NULL) {
        return -1;
    }
    q->response_buffer = buf;
    q->response_buffer_size = buf_size;
    q->response_hdr = (rip_ns_header_t *)(buf + 2);
    q->dnptrs[0] = (unsigned char *)q->response_hdr;
    return 0;
}