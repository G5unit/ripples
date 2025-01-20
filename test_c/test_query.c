/**
 * @file test_query.c
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
/** \ingroup unit_tests 
 * \defgroup query_ut Query
 */

/** \ingroup query_ut
 * \defgroup query_object_ut Query Object
 *
 * @brief Query Object unit tests
 *  @{
 */
#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#include "config.h"
#include "query.h"

/**! @cond */
TestSuite(query);
/**! @endcond */

/** Test query object initialization for UDP protocol. */
Test(query, test_query_init_udp) {
    config_t cfg;
    config_init(&cfg);

    query_t q;

    query_init(&q, &cfg, 0);
    cr_assert(q.protocol == 0);
    cr_assert(q.client_ip != NULL);
    cr_assert(q.local_ip != NULL);
    cr_assert(q.request_buffer != NULL);
    cr_assert(q.request_buffer_size == RIP_NS_PACKETSZ+1);
    cr_assert(q.request_buffer_len == 0);
    cr_assert((void *)q.request_hdr == (void *)q.request_buffer);
    cr_assert(q.query_label != NULL);
    cr_assert(q.query_label_size == RIP_NS_MAXCDNAME + 1);
    cr_assert(q.query_label_len == 0);
    cr_assert(q.query_q_type == rip_ns_t_invalid);
    cr_assert(q.query_q_class == rip_ns_c_invalid);
    cr_assert(q.edns.edns_raw_buf == NULL);
    cr_assert(q.edns.edns_raw_buf_len == 0);
    cr_assert(q.edns.edns_valid == false);
    cr_assert(q.edns.extended_rcode == 0);
    cr_assert(q.edns.version == 0);
    cr_assert(q.edns.udp_resp_len == 0);
    cr_assert(q.edns.dnssec == false);
    cr_assert(q.edns.client_subnet.edns_cs_raw_buf == NULL);
    cr_assert(q.edns.client_subnet.edns_cs_raw_buf_len == 0);
    cr_assert(q.edns.client_subnet.edns_cs_valid == false);
    cr_assert(q.edns.client_subnet.family == 0);
    cr_assert(q.edns.client_subnet.source_mask == 0);
    cr_assert(q.edns.client_subnet.scope_mask == 0);
    cr_assert(q.response_buffer != NULL);
    cr_assert(q.response_buffer_size == RIP_NS_UDP_MAXMSG);
    cr_assert(q.response_buffer_len == 0);
    cr_assert((void *)q.response_hdr == (void *)q.response_buffer);
    cr_assert(q.answer_section_count == 0);
    cr_assert(q.authority_section_count == 0);
    cr_assert(q.additional_section_count == 0);
    cr_assert(q.end_code == -1);
    cr_assert(q.error_message[0] == '\0');
    cr_assert(q.dnptrs[0] == (unsigned char *)q.response_hdr);

    query_clean(&q);
    config_clean(&cfg);
}

/** Test query object initialization for TCP protocol. */
Test(query, test_query_init_tcp) {
    config_t cfg;
    config_init(&cfg);

    query_t q;

    query_init(&q, &cfg, 1);
    cr_assert(q.protocol == 1);
    cr_assert(q.client_ip == NULL);
    cr_assert(q.local_ip == NULL);
    cr_assert(q.request_buffer == NULL);
    cr_assert(q.request_buffer_size == 0);
    cr_assert(q.request_buffer_len == 0);
    cr_assert(q.request_hdr == NULL);
    cr_assert(q.query_label != NULL);
    cr_assert(q.query_label_size == RIP_NS_MAXCDNAME + 1);
    cr_assert(q.query_label_len == 0);
    cr_assert(q.query_q_type == rip_ns_t_invalid);
    cr_assert(q.query_q_class == rip_ns_c_invalid);
    cr_assert(q.edns.edns_raw_buf == NULL);
    cr_assert(q.edns.edns_raw_buf_len == 0);
    cr_assert(q.edns.edns_valid == false);
    cr_assert(q.edns.extended_rcode == 0);
    cr_assert(q.edns.version == 0);
    cr_assert(q.edns.udp_resp_len == 0);
    cr_assert(q.edns.dnssec == false);
    cr_assert(q.edns.client_subnet.edns_cs_raw_buf == NULL);
    cr_assert(q.edns.client_subnet.edns_cs_raw_buf_len == 0);
    cr_assert(q.edns.client_subnet.edns_cs_valid == false);
    cr_assert(q.edns.client_subnet.family == 0);
    cr_assert(q.edns.client_subnet.source_mask == 0);
    cr_assert(q.edns.client_subnet.scope_mask == 0);
    cr_assert(q.response_buffer != NULL);
    cr_assert(q.response_buffer_size == cfg.tcp_writebuff_size);
    cr_assert(q.response_buffer_len == 0);
    cr_assert((void *)q.response_hdr == (void *)(q.response_buffer + 2));
    cr_assert(q.answer_section_count == 0);
    cr_assert(q.authority_section_count == 0);
    cr_assert(q.additional_section_count == 0);
    cr_assert(q.end_code == -1);
    cr_assert(q.error_message[0] == '\0');
    cr_assert(q.dnptrs[0] == (unsigned char *)q.response_hdr);

    query_clean(&q);
    config_clean(&cfg);
}


/** Unit test parameterized inputs for @ref query_reset . */
ParameterizedTestParameters(query, test_query_reset)
{
    static query_t params[] = {
        
        {
            .request_buffer_len = 1,
            .query_label_len    = 1,

            .query_q_type  = rip_ns_t_a,
            .query_q_class = rip_ns_c_in,

            .edns.edns_raw_buf_len = 1,
            .edns.edns_valid       = 1,

            .edns.client_subnet.edns_cs_raw_buf_len = 1,
            .edns.client_subnet.edns_cs_valid      = 1,

            .response_buffer_len = 1,

            .error_message[0] = 'a',

            .dnptrs[0] = NULL,

            .answer_section_count     = 1,
            .authority_section_count  = 1,
            .additional_section_count = 1,

            .end_code = rip_ns_r_nxdomain,
        },
        
    };

    size_t nb_params = sizeof(params) / sizeof(query_t);
    return cr_make_param_array(query_t, params, nb_params);
}

/** Unit test for @ref query_reset. */
ParameterizedTest(query_t *param, query, test_query_reset)
{
    /* Populate parameters that query_Rset() should not change so it
     * could be verified.
     */
    uint8_t                   protocol           = param->protocol            = 1;
    struct sockaddr_storage *client_ip           = param->client_ip           = (struct sockaddr_storage *)0xf0;
    struct sockaddr_storage *local_ip            = param->local_ip            = (struct sockaddr_storage *)0xf1;

    unsigned char           *request_buffer      = param->request_buffer      = (unsigned char *)0xf2;
    size_t                   request_buffer_size = param->request_buffer_size = 13;
    rip_ns_header_t         *request_hdr         = param->request_hdr         = (rip_ns_header_t *)0xf3;

    unsigned char           *query_label         = param->query_label         = (unsigned char *)0xf4;
    uint16_t                 query_label_size    = param->query_label_size    = 14;

    unsigned char           *edns_raw_buf        = param->edns.edns_raw_buf   = (unsigned char *)0xf5;
    uint8_t                  edns_extended_rcode = param->edns.extended_rcode = 2;
    uint8_t                  edns_version        = param->edns.version        = 1;
    uint16_t                 edns_udp_resp_len   = param->edns.udp_resp_len   = 128;
    bool                     edns_dnssec         = param->edns.dnssec         = true;

    unsigned char           *edns_cs_raw_buf     = param->edns.client_subnet.edns_cs_raw_buf = (unsigned char *)0xf6;
    uint16_t                 edns_cs_family      = param->edns.client_subnet.family          = 2;
    uint16_t                 edns_cs_source_mask = param->edns.client_subnet.source_mask     = 21;
    uint16_t                 edns_cs_scope_mask  = param->edns.client_subnet.scope_mask      = 23;

    unsigned char           *response_buffer       = param->response_buffer      = (unsigned char *)0xf7;
    size_t                   response_buffer_size  = param->response_buffer_size = 67;
    rip_ns_header_t         *response_hdr          = param->response_hdr         = (rip_ns_header_t *)0xf8;

    rr_record_t *answer_section[1]     = { (rr_record_t *)0xfa };
    param->answer_section[0]           = (rr_record_t *)0xfa;
    rr_record_t *authority_section[1]  = { (rr_record_t *)0xfb };
    param->authority_section[0]        = (rr_record_t *)0xfb;
    rr_record_t *additional_section[1] = { (rr_record_t *)0xfc };
    param->additional_section[0]       = (rr_record_t *)0xfc;

    struct timespec start_time = {.tv_sec = 1, .tv_nsec = 2};
    param->start_time = (struct timespec) {.tv_sec = 1, .tv_nsec = 2};

    struct timespec end_time ={.tv_sec = 1, .tv_nsec = 2};
    param->end_time   = (struct timespec) {.tv_sec = 1, .tv_nsec = 2};


    query_reset(param);

    /* Assert parameters were reset. */
    cr_assert(param->request_buffer_len == 0);
    cr_assert(param->query_label_len == 0);
    cr_assert(param->query_q_type == rip_ns_t_invalid);
    cr_assert(param->query_q_class == rip_ns_c_invalid);
    cr_assert(param->edns.edns_raw_buf_len == 0);
    cr_assert(param->edns.edns_valid == 0);
    cr_assert(param->edns.client_subnet.edns_cs_raw_buf_len == 0);
    cr_assert(param->edns.client_subnet.edns_cs_valid == 0);
    cr_assert(param->response_buffer_len == 0);
    cr_assert(param->error_message[0] == '\0');
    cr_assert(param->dnptrs[0] == (unsigned char *)param->response_hdr);
    cr_assert(param->answer_section_count == 0);
    cr_assert(param->authority_section_count == 0);
    cr_assert(param->additional_section_count == 0);
    cr_assert(param->end_code == rip_ns_r_rip_unknown);

    /* Assert parameters were left untouched. */
    cr_assert(protocol == param->protocol);
    cr_assert(client_ip == param->client_ip);
    cr_assert(local_ip == param->local_ip);
    cr_assert(request_buffer == param->request_buffer);
    cr_assert(request_buffer_size == param->request_buffer_size);
    cr_assert(request_hdr == param->request_hdr);
    cr_assert(query_label == param->query_label);
    cr_assert(query_label_size == param->query_label_size);
    cr_assert(edns_raw_buf == param->edns.edns_raw_buf);
    cr_assert(edns_extended_rcode == param->edns.extended_rcode);
    cr_assert(edns_version == param->edns.version);
    cr_assert(edns_udp_resp_len == param->edns.udp_resp_len);
    cr_assert(edns_dnssec == param->edns.dnssec);
    cr_assert(edns_cs_raw_buf == param->edns.client_subnet.edns_cs_raw_buf);
    cr_assert(edns_cs_family == param->edns.client_subnet.family);
    cr_assert(edns_cs_source_mask == param->edns.client_subnet.source_mask);
    cr_assert(edns_cs_scope_mask == param->edns.client_subnet.scope_mask);
    cr_assert(response_buffer == param->response_buffer);
    cr_assert(response_buffer_size == param->response_buffer_size);
    cr_assert(response_hdr == param->response_hdr);
    cr_assert(answer_section[0] == param->answer_section[0]);
    cr_assert(authority_section[0] == param->authority_section[0]);
    cr_assert(additional_section[0] == param->additional_section[0]);
    cr_assert(start_time.tv_sec == param->start_time.tv_sec);
    cr_assert(start_time.tv_nsec == param->start_time.tv_nsec);
    cr_assert(end_time.tv_sec == param->end_time.tv_sec);
    cr_assert(end_time.tv_nsec == param->end_time.tv_nsec);
}

/** Test query_clean() function for UDP transport. This unit test is valuable
 * when unit test binary is run via valgrind to detect memory leaks.
*/
Test(query, test_query_clean_udp) {
    config_t cfg;
    config_init(&cfg);

    query_t q;

    query_init(&q, &cfg, 1);
    q.client_ip = malloc(sizeof(struct sockaddr_storage));
    q.local_ip = malloc(sizeof(struct sockaddr_storage));
    q.request_buffer = malloc(sizeof(unsigned char)* 14);
    q.response_buffer = malloc(sizeof(unsigned char)* 15);
    q.query_label = malloc(sizeof(unsigned char)* 1);

    query_clean(&q);
    config_clean(&cfg);
}

/** Test query_clean() function for TCP transport. This unit test is valuable
 * when unit test binary is run via valgrind to detect memory leaks.
*/
Test(query, test_query_clean_tcp) {
    config_t cfg;
    config_init(&cfg);

    query_t q;

    query_init(&q, &cfg, 1);

    q.response_buffer = malloc(sizeof(unsigned char)* 15);
    q.query_label = malloc(sizeof(unsigned char)* 1);

    query_clean(&q);
    config_clean(&cfg);
}

/** Unit test input structure for @ref query_tcp_response_buffer_increase. */
struct test_params_query_tcp_response_buffer_increase {
    /** Initial size of buffer. */
    size_t size;

    /** Size of buffer expected after increase. */
    size_t new_size;

    /** Function query_tcp_response_buffer_increase() return value to verify */
    int ret;
};

/** Unit test parameterized inputs for @ref query_tcp_response_buffer_increase . */
ParameterizedTestParameters(query, test_query_tcp_response_buffer_increase)
{
    static struct test_params_query_tcp_response_buffer_increase params[] = {
        /* fields represent: initial buffer size, result buffer size, func() return value. */
        {
            .size = 1, .new_size = RIP_NS_UDP_MAXMSG+1, .ret = 0
        },
        {
            .size = 2, .new_size = RIP_NS_UDP_MAXMSG+2, .ret = 0
        },
        {
            .size = 512, .new_size = RIP_NS_UDP_MAXMSG+512, .ret = 0
        },
        {
            .size = RIP_NS_UDP_MAXMSG, .new_size = RIP_NS_UDP_MAXMSG+RIP_NS_UDP_MAXMSG, .ret = 0
        },
        {
            .size = RIP_NS_MAXMSG, .new_size = RIP_NS_MAXMSG, .ret = -1
        },
        {
            .size = RIP_NS_MAXMSG-1, .new_size = RIP_NS_MAXMSG, .ret = 0
        },
    };

    size_t nb_params = sizeof(params) / sizeof(struct test_params_query_tcp_response_buffer_increase);
    return cr_make_param_array(struct test_params_query_tcp_response_buffer_increase, params, nb_params);
}

/** Unit test for @ref query_tcp_response_buffer_increase. */
ParameterizedTest(struct test_params_query_tcp_response_buffer_increase *param,
                  query, test_query_tcp_response_buffer_increase)
{
    config_t cfg;
    config_init(&cfg);

    query_t q;

    query_init(&q, &cfg, 1);
    free(q.response_buffer);

    q.response_buffer = malloc(sizeof(unsigned char *) * param->size);
    q.response_buffer_size = param->size;

    int ret = query_tcp_response_buffer_increase(&q);

    cr_assert(ret == param->ret, "ret: %d, pret: %d", ret, param->ret);
    if (ret == 0) {
        cr_assert(q.response_buffer_size == param->new_size,
                  "q.response_buffer_size: %lu, param->new_size: %lu",
                  q.response_buffer_size, param->new_size);
    }
    
    query_clean(&q);
    config_clean(&cfg);
}

/** @}*/
