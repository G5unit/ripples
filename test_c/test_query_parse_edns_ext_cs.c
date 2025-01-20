/**
 * @file test_query_parse_edns_ext_cs.c
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
 * \defgroup query_parse_edns_ext_cs_ut Parse EDNS client subnet extension
 *
 *
 * @brief Query parse EDNS client subnet unit tests
 *  @{
 */
#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#include "query.h"

/** Unit test input structure for @ref query_parse_edns_ext_cs. */
struct test_params_query_parse_edns_ext_cs {
    /** Parameter index, useful when troubleshooting unit tests, i.e. adding a
     * message to be reported by cr_assert() on failure.
     */
    int ut_param_no;

    /** Bytes in network order to parse EDNS extension client subnet from <br>
     * 
     * Format is:  <br>
     * 2 bytest        = family  <br>
     * 1 byte          = source netmask  <br>
     * 1 byte          = scope netmask  <br>
     * remaining bytes = client subnet  <br>
     */
    uint8_t buf[512];

    /** Length of data in buffer. */
    size_t buf_len;

    /** Client subnet data to validate parse outcome. */
    edns_client_subnet_t cs;

    /** IP address as string to validate parse outcome. */
    char ip[INET6_ADDRSTRLEN];

    /** Function query_parse_edns_ext_cs() return value to validate parse
     * outcome.
     */
    int ret;
};

/** Unit test parameterized inputs for @ref query_parse_edns_ext_cs. */
ParameterizedTestParameters(query, test_query_parse_edns_ext_cs)
{
    static struct test_params_query_parse_edns_ext_cs params[] = {
        /** Positive, IPv4 10.0.0.0/24. */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 24, 0, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = true,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 24,
            },
            .ip = "10.0.0.0",
            .ret = 0,
        },
        /** Positive, IPv6 2001:db8:abcd:0012::/64. */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 64, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = true,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 64,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = 0,
        },

        /** Negative, minimum number of required bytes not present, need 4 got 3 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 24},
            .buf_len = 3,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 24,
            },
            .ip = "10.0.0.0",
            .ret = -1,
        },
        /** Negative, minimum number of required bytes not present, need 4 got 2 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01},
            .buf_len = 2,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 24,
            },
            .ip = "10.0.0.0",
            .ret = -1,
        },
        /** Negative, minimum number of required bytes not present, need 4 got 1 */
        {
            .ut_param_no = 1,
            .buf = {0x00},
            .buf_len = 1,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 0,
            },
            .ip = "10.0.0.0",
            .ret = -1,
        },
        /** Negative, minimum number of required bytes not present, need 4 got 0 */
        {
            .ut_param_no = 1,
            .buf = {},
            .buf_len = 0,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 0,
            },
            .ip = "10.0.0.0",
            .ret = -1,
        },


        /** Negative, IPv4 source mask > 32, source mask = 33 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 33, 0, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 33,
            },
            .ip = "10.0.0.0",
            .ret = -2,
        },
        /** Negative, IPv4 source mask > 32, source mask = 64 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 64, 0, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 64,
            },
            .ip = "10.0.0.0",
            .ret = -2,
        },
        /** Negative, IPv4 source mask > 32, source mask = 255 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 255, 0, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 255,
            },
            .ip = "10.0.0.0",
            .ret = -2,
        },

        /** Negative, IPv4 scope mask != 0 , scope mask = 1*/
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 24, 1, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 1,
                .source_mask = 24,
            },
            .ip = "10.0.0.0",
            .ret = -2,
        },
        /** Negative, IPv4 scope mask != 0 , scope mask = 33*/
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 24, 33, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 33,
                .source_mask = 24,
            },
            .ip = "10.0.0.0",
            .ret = -2,
        },
        /** Negative, IPv4 scope mask != 0 , scope mask = 255*/
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 24, 255, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 255,
                .source_mask = 24,
            },
            .ip = "10.0.0.0",
            .ret = -2,
        },
        /** Negative, IPv4 IPv4 source mask > 32 and scope mask != 0,
         * source mask = 43  scope mask = 255.
         */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 43, 255, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 255,
                .source_mask = 43,
            },
            .ip = "10.0.0.0",
            .ret = -2,
        },

        /** Negative, IPv6 source mask > 128, source mask = 129 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 129, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 129,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },
        /** Negative, IPv6 source mask > 128, source mask = 254 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 254, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 254,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },
        /** Negative, IPv6 source mask > 128, source mask = 255 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 255, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 255,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },

        /** Negative, IPv6 scope mask != 0 , scope mask = 1 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 64, 1, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 1,
                .source_mask = 64,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },
        /** Negative, IPv6 scope mask != 0 , scope mask = 2 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 64, 2, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 2,
                .source_mask = 64,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },
        /** Negative, IPv6 scope mask != 0 , scope mask = 64 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 64, 46, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 46,
                .source_mask = 64,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },
        /** Negative, IPv6 scope mask != 0 , scope mask = 128 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 64, 128, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 128,
                .source_mask = 64,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },
        /** Negative, IPv6 scope mask != 0 , scope mask = 255 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 64, 255, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 255,
                .source_mask = 64,
            },
            .ip = "2001:db8:abcd:0012::",
            .ret = -3,
        },

        /** Negative, IPv4 not enough IP bytes as indicated by mask 32, need 4 have 3 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 32, 0, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 32,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 31, need 4 have 2 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 31, 0, 0x0a, 0x00},
            .buf_len = 6,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 31,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 30, need 4 have 1 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 30, 0, 0x0a},
            .buf_len = 5,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 30,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 29, need 4 have 0 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 29, 0},
            .buf_len = 4,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 29,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 25, need 4 have 3 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 25, 0, 0x0a, 0x00, 0x00},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 25,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 24, need 3 have 2 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 24, 0, 0x0a, 0x00},
            .buf_len = 6,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 24,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 23, need 3 have 2 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 23, 0, 0x0a, 0x00},
            .buf_len = 6,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 23,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 17, need 3 have 2 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 17, 0, 0x0a, 0x00},
            .buf_len = 6,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 17,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 16, need 2 have 1 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 16, 0, 0x0a},
            .buf_len = 5,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 16,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 15, need 2 have 1 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 15, 0, 0x0a},
            .buf_len = 5,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 15,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 9, need 2 have 1 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 9, 0, 0x0a},
            .buf_len = 5,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 9,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 8, need 1 have 0 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 8, 0},
            .buf_len = 4,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 8,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 7, need 1 have 0 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 7, 0},
            .buf_len = 4,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 7,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },
        /** Negative, IPv4 not enough IP bytes as indicated by mask 1, need 1 have 0 */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 1, 0},
            .buf_len = 4,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 1,
            },
            .ip = "10.0.0.0",
            .ret = -5,
        },

        /** Negative, IPv6 not enough IP bytes as indicated by mask 128, need 16 have 15 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 128, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00},
            .buf_len = 19,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 128,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 127, need 16 have 15 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 127, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00},
            .buf_len = 19,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 127,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 121, need 16 have 1 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 121, 0, 0x20},
            .buf_len = 5,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 121,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 120, need 15 have 14 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 120, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd},
            .buf_len = 18,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 120,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 119, need 15 have 13 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 119, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12, 0x20, 0x01, 0x0d, 0xb8, 0xab},
            .buf_len = 17,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 119,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 112, need 14 have 13 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 112, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12, 0x20, 0x01, 0x0d, 0xb8, 0xab},
            .buf_len = 17,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 112,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 97, need 13 have 12 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 97, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12, 0x20, 0x01, 0x0d, 0xb8},
            .buf_len = 16,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 97,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 65, need 9 have 8 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 65, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0x12},
            .buf_len = 12,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 65,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 65, need 9 have 4 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 65, 0, 0x20, 0x01, 0x0d, 0xb8},
            .buf_len = 8,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 65,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },
        /** Negative, IPv6 not enough IP bytes as indicated by mask 64, need 8 have 7 */
        {
            .ut_param_no = 2,
            .buf = {0x00, 0x02, 65, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00},
            .buf_len = 11,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 65,
            },
            .ip = "2001:db8:abcd:0012:2001:db8:abcd:0012",
            .ret = -5,
        },

        /** Negative, IPv4, source mask specified "31" does not match IP received "10.0.0.243". */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 31, 0, 0x0a, 0x00, 0x00, 0xf3},
            .buf_len = 8,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 31,
            },
            .ip = "10.0.0.1",
            .ret = -6,
        },
        /** Negative, IPv4, source mask specified "25" does not match IP received "10.0.0.01". */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 25, 0, 0x0a, 0x00, 0x00, 0x01},
            .buf_len = 8,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 25,
            },
            .ip = "10.0.0.1",
            .ret = -6,
        },
        /** Negative, IPv4, source mask specified "23" does not match IP received "10.0.1.0". */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 23, 0, 0x0a, 0x00, 0x01},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 23,
            },
            .ip = "10.0.1.0",
            .ret = -6,
        },
        /** Negative, IPv4, source mask specified "18" does not match IP received "10.0.160.0". */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 18, 0, 0x0a, 0x00, 0xa0},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 18,
            },
            .ip = "10.0.160.0",
            .ret = -6,
        },
        /** Negative, IPv4, source mask specified "18" does not match IP received "10.0.193.0". */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 18, 0, 0x0a, 0x00, 0xc1},
            .buf_len = 7,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 18,
            },
            .ip = "10.0.193.0",
            .ret = -6,
        },
        /** Negative, IPv4, source mask specified "15" does not match IP received "10.0.255.0". */
        {
            .ut_param_no = 1,
            .buf = {0x00, 0x01, 15, 0, 0x0a, 0xff},
            .buf_len = 6,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 15,
            },
            .ip = "10.0.255.0",
            .ret = -6,
        },
        /** Negative, IPv4, source mask specified "1" does not match IP received "10.0.0.0". */
        {
            .ut_param_no = 66,
            .buf = {0x00, 0x01, 1, 0, 0x0a},
            .buf_len = 5,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 1,
            },
            .ip = "10.0.0.0",
            .ret = -6,
        },
        /** Negative, IPv4, source mask specified "1" does not match IP received "129.0.0.0". */
        {
            .ut_param_no = 66,
            .buf = {0x00, 0x01, 1, 0, 0x81},
            .buf_len = 5,
            .cs = {
                .edns_cs_valid = false,
                .family = 1,
                .scope_mask = 0,
                .source_mask = 1,
            },
            .ip = "129.0.0.0",
            .ret = -6,
        },
         
        /** Negative, IPv6, source mask specified "127" does not match IP received
         * "2001:0db8:abcd:12:2001:db8:abcd:00ff".
         */
        {
            .ut_param_no = 77,
            .buf = {0x00, 0x02, 127, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00,
                    0x12, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00, 0xff},
            .buf_len = 20,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 127,
            },
            .ip = "2001:0db8:abcd:12:2001:db8:abcd:00ff",
            .ret = -6,
        },
        /** Negative, IPv6, source mask specified "94" does not match IP received
         * "2001:db8:abcd:12:2001:db8:abcd:ff".
         */
        {
            .ut_param_no = 77,
            .buf = {0x00, 0x02, 94, 0, 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0x00,
                    0x12, 0x20, 0x01, 0x0d, 0xff},
            .buf_len = 16,
            .cs = {
                .edns_cs_valid = false,
                .family = 2,
                .scope_mask = 0,
                .source_mask = 94,
            },
            .ip = "2001:db8:abcd:12:2001:db8:abcd:ff",
            .ret = -6,
        },

    };

    size_t nb_params = sizeof(params) / sizeof(struct test_params_query_parse_edns_ext_cs);
    return cr_make_param_array(struct test_params_query_parse_edns_ext_cs, params, nb_params);
}


/** Unit test for @ref query_parse_edns_ext_cs . */
ParameterizedTest(struct test_params_query_parse_edns_ext_cs *param, query,
                  test_query_parse_edns_ext_cs)
{
    edns_client_subnet_t edns_cs = {};
    int                  ret     = 0;
    struct in6_addr      addr    = {};

    edns_cs.edns_cs_raw_buf     = param->buf;
    edns_cs.edns_cs_raw_buf_len = param->buf_len;

    ret = query_parse_edns_ext_cs(&edns_cs);
    cr_assert(ret == param->ret, "Failed test index: %d, %d != %d",
                                 param->ut_param_no, ret, param->ret);
    if (param->ret < 0) {
        cr_assert(edns_cs.edns_cs_valid == false);
        /* Parsing failed so nothing else to check.*/
        return;
    }
    cr_assert(edns_cs.edns_cs_valid == param->cs.edns_cs_valid);
    cr_assert(edns_cs.family == param->cs.family);
    if (param->cs.family == 1) {
        /* IPv4 */
        cr_assert(inet_pton(AF_INET, param->ip, &addr) == 1);

        struct sockaddr_in *sin = (struct sockaddr_in *)&edns_cs.ip;
        struct in_addr     *in  = (struct in_addr *)&addr;
        cr_assert(sin->sin_addr.s_addr == in->s_addr);
    } else {
        /* IPv6 */
        cr_assert(inet_pton(AF_INET6, param->ip, &addr) == 1);

        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&edns_cs.ip;
        cr_assert(memcmp(&sin6->sin6_addr, &addr, sizeof(struct in6_addr)) == 0);
    }

    cr_assert(edns_cs.source_mask == param->cs.source_mask);
    cr_assert(edns_cs.scope_mask == param->cs.scope_mask);
}

/** @}*/
