/**
 * @file query_resolve.c
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
#include <stdlib.h>
#include <string.h>

#include <stdio.h>


#include "query.h"



/** Resolve a query and if appropriate pack the answer into response buffer.
 * 
 * @param q Query to resolve.
 */
void
query_resolve(query_t *q)
{

    static rr_record_t example_com_rr_A = {
        .class     = rip_ns_c_in,
        .type      = rip_ns_t_a,
        .ttl       = 60,
        .rdata_len = 4,
    };
    if (example_com_rr_A.rdata == NULL) {
        example_com_rr_A.name = q->query_label;
        example_com_rr_A.name_len = q->query_label_len;
        example_com_rr_A.rdata = malloc(4);
        *(uint32_t *)example_com_rr_A.rdata = htonl(INADDR_LOOPBACK);
    }

    static rr_record_t example_com_rr_NS = {
        .class     = rip_ns_c_in,
        .type      = rip_ns_t_ns,
        .ttl       = 60,
    };
    static char *ns_name = "ns.example.com";
    if (example_com_rr_NS.rdata == NULL) {
        example_com_rr_NS.name = q->query_label;
        example_com_rr_NS.name_len = q->query_label_len;
        example_com_rr_NS.rdata = malloc(16);
        example_com_rr_NS.rdata_len = rip_ns_name_pton((unsigned char *)ns_name, example_com_rr_NS.rdata, 16);
        example_com_rr_NS.rdata_len = 16;
    }

    static rr_record_t example_com_rr_NS_A = {
        .name_len  = 14,
        .class     = rip_ns_c_in,
        .type      = rip_ns_t_a,
        .ttl       = 60,
        .rdata_len = 4,
    };
    if (example_com_rr_NS_A.rdata == NULL) {
        example_com_rr_NS_A.name = (unsigned char *)strdup("ns.example.com");
        example_com_rr_NS_A.rdata = malloc(4);
        *(uint32_t *)example_com_rr_NS_A.rdata = htonl(INADDR_LOOPBACK);
    }

    static rr_record_t example_com_rr_NS_AAAA = {
        .name_len  = 14,
        .class     = rip_ns_c_in,
        .type      = rip_ns_t_aaaa,
        .ttl       = 60,
        .rdata_len = 16,
    };
    if (example_com_rr_NS_AAAA.rdata == NULL) {
        example_com_rr_NS_AAAA.name = (unsigned char *)strdup("ns.example.com");
        example_com_rr_NS_AAAA.rdata = malloc(16);
        struct in6_addr in6 = IN6ADDR_LOOPBACK_INIT;
        memcpy(example_com_rr_NS_AAAA.rdata, &in6 , 16);
    }

    q->end_code = rip_ns_r_noerror;

    /* answer section */
    q->answer_section[q->answer_section_count] = &example_com_rr_A;
    q->answer_section_count += 1;

    /* authority section */
    q->authority_section[q->authority_section_count] = &example_com_rr_NS;
    q->authority_section_count += 1;

    /* additional section */
    q->additional_section[q->additional_section_count] = &example_com_rr_NS_A;
    q->additional_section_count += 1;
    q->additional_section[q->additional_section_count] = &example_com_rr_NS_AAAA;
    q->additional_section_count += 1;

}