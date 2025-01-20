/**
 * @file test_query_parse_edns_ext.c
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
 * \defgroup query_parse_edns_ext_ut Parse EDNS Option
 *
 * @brief Query parse EDNS option unit tests
 *  @{
 */
#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#include "query.h"

/** Unit test input structure for @ref query_parse_edns_ext. */
struct test_params_query_parse_edns_ext {
    /** Parameter index, useful when troubleshooting unit tests, i.e. adding a
     * message to be reported by cr_assert() on failure such as
     * cr_assert(ret == param->ret,"failed test parameter index is %d", param->ut_param_no) .
     */
    int ut_param_no;
    /** query_parse_edns_ext() argument 2:<br>
     * Bytes in network order to parse EDNS extension client subnet from.
     * 
     * Format is: <br>
     * 2 bytest        = family <br>
     * 1 byte          = source netmask <br>
     * 1 byte          = scope netmask <br>
     * remaining bytes = client subnet <br>
     */
    uint8_t buf[512];

    /** query_parse_edns_ext() argument 3:<br>
     * Pointer to last entry in buffer.
     */
    uint8_t  *eom;

    /** Parsed option code to verify */
    uint16_t opt_code;

    /** Parsed option length to verify */
    uint16_t opt_len;

    /** Function query_parse_edns_ext() return value to verify */
    int ret;
};

/** Unit test parameterized inputs for @ref query_parse_edns_ext. */
ParameterizedTestParameters(query, test_query_parse_edns_ext)
{
    static struct test_params_query_parse_edns_ext params[] = {
        /** Positive, IPv4 10.0.0.0/24. */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 24, 0, 0x0a, 0x00, 0x00},
            .eom = &params[0].buf[6],
            .opt_code = 1,
            .opt_len = 4,
            .ret = 0,
        },
        
    };

    size_t nb_params = sizeof(params) / sizeof(struct test_params_query_parse_edns_ext);
    return cr_make_param_array(struct test_params_query_parse_edns_ext, params, nb_params);
}

/** Unit test for @ref query_parse_edns_ext. */
ParameterizedTest(struct test_params_query_parse_edns_ext *param, query,
                  test_query_parse_edns_ext)
{

    query_t q = {};
    int ret = query_parse_edns_ext(&q, param->buf, param->eom);

    cr_assert(ret == param->ret);

}

/** @}*/
