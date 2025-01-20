/**
 * @file test_utils.c
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
 * \defgroup utils_ut Utilities
 *
 * @brief Utilities unit tests
 *  @{
 */
#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include <time.h>

#include "utils.h"


/**! @cond */
TestSuite(utils);
/**! @endcond */

/** \ingroup utils_ut 
 * \defgroup utl_timespec_to_rfc3339nano_ut utl_timespec_to_rfc3339nano
 *
 * @brief @ref utl_timespec_to_rfc3339nano unit tests
 *  @{
 */
/** Unit test input structure for @ref utl_timespec_to_rfc3339nano . */
struct test_params_utl_timespec_to_rfc3339nano {
    /** Time to test. */
    struct timespec ts;

    /** Validation time as string. */
    char result[TIME_RFC3339_STRLEN];
};

/** Unit test parameterized inputs for @ref utl_timespec_to_rfc3339nano. */
ParameterizedTestParameters(utils, test_utl_timespec_to_rfc3339nano) {
    static struct test_params_utl_timespec_to_rfc3339nano params[] = {
        { {.tv_sec = 123456789, .tv_nsec = 12345}, "1973-11-29T21:33:09.12345Z" }
    };

    size_t nb_params = sizeof(params) / sizeof(struct test_params_utl_timespec_to_rfc3339nano);
    return cr_make_param_array(struct test_params_utl_timespec_to_rfc3339nano, params, nb_params);
}

/** Unit test for @ref utl_timespec_to_rfc3339nano. */
ParameterizedTest(struct test_params_utl_timespec_to_rfc3339nano *param, utils, test_utl_timespec_to_rfc3339nano) {
    char result[TIME_RFC3339_STRLEN];

    utl_timespec_to_rfc3339nano(&param->ts, result);
    cr_assert_str_eq(result, param->result);
}
/** @}*/

/** \ingroup utils_ut 
 * \defgroup str_is_numeric_ut str_is_numeric
 *
 * @brief @ref str_is_numeric unit tests
 *  @{
 */
/** Unit test input structure for @ref str_is_numeric . */
struct test_str_is_numeric {

    /** String to test. */
    char str[24];

    /** Function str_is_numeric() return value to validate against. */
    bool result;
};

/** Unit test parameterized inputs for @ref str_is_numeric. */
ParameterizedTestParameters(utils, test_str_is_numeric) {
    static struct test_str_is_numeric params[] = {
        { "2024-12-24T12:43:9.345", false },
        { "2024", true }
    };

    size_t nb_params = sizeof(params) / sizeof(struct test_str_is_numeric);
    return cr_make_param_array(struct test_str_is_numeric, params, nb_params);
}

/** Unit test for @ref str_is_numeric. */
ParameterizedTest(struct test_str_is_numeric *param, utils, test_str_is_numeric) {
    bool result = str_is_numeric((uint8_t *)param->str, strlen(param->str));
    cr_assert(result == param->result);
}
/** @}*/

/** @}*/


