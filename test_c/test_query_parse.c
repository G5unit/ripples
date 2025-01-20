/**
 * @file test_query_parse.c
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
/** \ingroup query_ut 
 * \defgroup query_parse_ut Parse Query
 *
 * @brief Query parse unit tests
 *  @{
 */
#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#include "query.h"

/** Unit test input structure for @ref query_parse and
 * @ref query_parse_request_rr_question .
 */
struct test_params_query_parse {
    /** Parameterized unit test index. */
    int ut_index;

    /** Buffer with bytes to parse as request. */
    uint8_t buf[RIP_NS_PACKETSZ];

    /** Length of data in buf. */
    uint16_t buf_len;

    /** Validation query label */
    char query_label[RIP_NS_MAXCDNAME];

    /** Validation query label length */
    uint16_t query_label_len;

    /** Validation query type */
    uint16_t query_q_type;

    /** Validation query class */
    uint16_t query_q_class;

    /** Query end code value to verify. */
    int end_code;
};

/** Unit test parameterized inputs for @ref query_parse and
 * @ref query_parse_request_rr_question.
 * 
 * A way to generate positive test input "buf" bytes is to use wireshark.
 * Start wireshark, issue a dig command and in wireshark right click on
 * "Domain Name System" field in the packet info frame, then select
 * copy->...as Hex Dump. You will need to at '0x' in front of each byte, and
 * ',' after each byte. Scripting these last two steps should be simple enough
 * via python, sed, etc.
*/
ParameterizedTestParameters(query, test_query_parse)
{
    static struct test_params_query_parse params[] = {
        /** Negative, not enough bytes for query header, min needed is 12, have 3 */
        {
            .ut_index = 0,
            .buf            = { 0x31, 0x10, 0x01 },
            .buf_len         = 3,
            .end_code        = rip_ns_r_rip_shortheader,
        },
        /** Negative, not enough bytes for query header, min needed is 12, have 0 */
        {
            .ut_index = 1,
            .buf            = { 0x31 },
            .buf_len         = 0,
            .end_code        = rip_ns_r_rip_shortheader,
        },
        /** Negative, not enough bytes for query header, min needed is 12, have 11 */
        {
            .ut_index = 2,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
                                0x00, 0x00 },
            .buf_len         = 11,
            .end_code        = rip_ns_r_rip_shortheader,
        },
        /** Negative, truncated query request not supported */
        {
            .ut_index = 3,
            .buf            = { 0x1f, 0xf9, 0x03, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_rip_query_tc,
        },
        /** Negative, query op code unsupported, got "iquery" (supported is only "query") */
        {
            .ut_index = 4,
            .buf            = { 0x1f, 0xf9, 0x09, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_notimpl,
        },
        /** Negative, query op code unsupported, got "14" (supported is only "query") */
        {
            .ut_index = 5,
            .buf            = { 0x1f, 0xf9, 0x71, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_notimpl,
        },
         /** Negative, header response flag should not be set */
        {
            .ut_index = 6,
            .buf            = { 0x1f, 0xf9, 0x81, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_formerr,
        },
        /** Negative, question section count = 0 */
        {
            .ut_index = 7,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_formerr,
        },
        /** Negative, question section count = 2 */
        {
            .ut_index = 8,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_notimpl,
        },
        /** Negative, query header indicates answer section is present */
        {
            .ut_index = 9,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_formerr,
        },
        /** Negative, query header indicates authority section is present */
        {
            .ut_index = 10,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_formerr,
        },

        /** Positive basic query, no EDNS, A IN www.example.com */
        {
            .ut_index = 11,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_rip_unknown,
            .query_label     = "www.example.com",
            .query_label_len = 15,
            .query_q_type    = rip_ns_t_a,
            .query_q_class   = rip_ns_c_in,
        },
        /** Positive basic query, no EDNS, A IN com */
        {
            .ut_index = 12,
            .buf            = { 0x31, 0x10, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,0x01, 0x00,
                                0x01 },
            .buf_len         = 21,
            .end_code        = rip_ns_r_rip_unknown,
            .query_label     = "com",
            .query_label_len = 3,
            .query_q_type    = rip_ns_t_a,
            .query_q_class   = rip_ns_c_in,
        },
        /** Positive basic query, no EDNS, A IN . */
        {
            .ut_index = 13,
            .buf            = { 0x43, 0xcf, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01 },
            .buf_len         = 17,
            .end_code        = rip_ns_r_rip_unknown,
            .query_label     = ".",
            .query_label_len = 1,
            .query_q_type    = rip_ns_t_a,
            .query_q_class   = rip_ns_c_in,
        },

        /** Negative, question section missing though header indicates it is present. */
        {
            .ut_index = 14,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01 },
            .buf_len         = 12,
            .end_code        = rip_ns_r_formerr,
        },

        /** Negative, question section partial, missing bytes. */
        {
            .ut_index = 5,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d },
            .buf_len         = 28,
            .end_code        = rip_ns_r_formerr,
        },
        /** Negative, question short by one byte */
        {
            .ut_index = 16,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00 },
            .buf_len         = 32,
            .end_code        = rip_ns_r_formerr,
        },
        /** Negative, question short by two bytes */
        {
            .ut_index = 17,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01 },
            .buf_len         = 31,
            .end_code        = rip_ns_r_formerr,
        },
        /** Negative, query type not supported, "rip_ns_t_wks"  */
        {
            .ut_index = 18,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x0b, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_notimpl,
        },
        /** Negative, query type not supported, "rip_ns_t_dlv" */
        {
            .ut_index = 19,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x80,
                                0x01, 0x00, 0x01 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_notimpl,
        },
        /** Negative, query class not supported, "rip_ns_c_chaos" */
        {
            .ut_index = 20,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x03 },
            .buf_len         = 33,
            .end_code        = rip_ns_r_notimpl,
        },
        /** Negative, query class not supported, "rip_ns_c_none" */
        {
            .ut_index = 21,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0xfe },
            .buf_len         = 33,
            .end_code        = rip_ns_r_notimpl,
        },
        /** Negative, extra unaccounted for bytes present. Currently code ignores
         * them and processes the query. Basic query, no EDNS, A IN www.example.com 
         */
        {
            .ut_index = 22,
            .buf            = { 0x1f, 0xf9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x07, 0x65, 0x78, 0x61,
                                0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00,
                                0x01, 0x00, 0x01, 0xfe },
            .buf_len         = 34,
            .end_code        = rip_ns_r_rip_unknown,
            .query_label     = "www.example.com",
            .query_label_len = 15,
            .query_q_type    = rip_ns_t_a,
            .query_q_class   = rip_ns_c_in,
        },
    };

    size_t nb_params = sizeof(params) / sizeof(struct test_params_query_parse);
    return cr_make_param_array(struct test_params_query_parse, params, nb_params);
}

/** Unit test for @ref query_parse and @ref query_parse_request_rr_question .
 * Focus of this test is on parsing query header and question section. Parsing
 * EDNS and EDNS extensions have their own unit tests.
*/
ParameterizedTest(struct test_params_query_parse *param, query,
                  test_query_parse)
{
    config_t cfg;
    query_t  q;

    config_init(&cfg);
    query_init(&q, &cfg, 1);

    q.protocol           = 0;
    q.request_buffer     = param->buf;
    q.request_buffer_len = param->buf_len;
    q.request_hdr        = (struct rip_ns_header_s *)q.request_buffer;

    query_parse(&q);

    cr_assert(q.end_code == param->end_code,
              "ut_index: %d, q.end_code: %d != param->end_code %d",
               param->ut_index, q.end_code, param->end_code);
    if (param->end_code != rip_ns_r_rip_unknown) {
        return;
    }
    cr_assert_str_eq((char *)q.query_label, param->query_label);
    cr_assert(q.query_label_len == param->query_label_len);
    cr_assert(q.query_q_type == param->query_q_type);
    cr_assert(q.query_q_class == param->query_q_class);
}

/** @}*/
