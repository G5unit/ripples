/**
 * @file utils.h
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
/** \defgroup utils Utilities
 * 
 * @brief This is a small collection of general purpose utilities.
 *
 *  @{
 */
#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#ifdef DEBUG
/** Constant enables printing of debug messages to stderr. Otherwise if not set
 * the compiler optimizes out (removes) all of the debug message code.
 */
#define DEBUG_TEST 1
#else
/** Constant enables printing of debug messages to stderr. Otherwise if not set
 * the compiler optimizes out (removes) all of the debug message code.
 */
#define DEBUG_TEST 0
#endif

/** Length of buffer needed to store time as rfc399nano formatted string */
#define TIME_RFC3339_STRLEN 31

/** Macro to print out debug message if DEBUG is set at compile time.
 * This prints out filename, line in file, and function it was called in. 
 */
#define debug() \
        do { if (DEBUG_TEST) fprintf(stderr, "%s:%d:%s(): \n", __FILE__, \
                                __LINE__, __func__); } while (0)
                                
/** Macro to print out debug message if DEBUG is set at compile time.
 * This prints out everything macro @ref debug does plus a string passed in as
 * argument.
 * 
 * @param str String to print out.
 */
#define debug_print(str) \
        do { if (DEBUG_TEST) fprintf(stderr, "%s:%d:%s(): " str "\n", __FILE__, \
                                __LINE__, __func__); } while (0)

/** Macro to print out debug message if DEBUG is set at compile time.
 * This prints out everything macro @ref debug does plus a string passed in as
 * argument. String can be formatted.
 * 
 * @note Only use this function to output a formatted string as "fmt" + variable
 *       list is REQUIRED.
 *       To output string with no format use @ref debug_print macro.
 * 
 * @param fmt String format followed by variable list. 
 */
#define debug_printf(fmt, ...) \
        do { if (DEBUG_TEST) fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

/** Macro to calculate array size. */
#define ARRAY_COUNT(a) sizeof(a) / sizeof(a[0])

/** Macro to check result of memory allocation, asserts on memory allocation
 * failure.
 */
#define CHECK_MALLOC(a) do { \
    if((a) == NULL) { \
        assert(0); \
    } } while (0)

/** Macro to increment a variable by one. */
#define INCREMENT(a) a += 1

/** Macro to decrement a variable by one. */
#define DECREMENT(a) a -= 1


int  utl_ip_port_from_ss(char *ip, size_t ip_len, uint16_t *port, struct sockaddr_storage *ss);

double utl_diff_timespec_as_double(const struct timespec *t1, const struct timespec *t2);
void   utl_diff_timespec(struct timespec *dst, const struct timespec *t1, const struct timespec *t2);

void char_to_lc(char *c);
void str_to_lc(uint8_t *str, size_t str_len);

bool str_is_numeric(uint8_t *str, size_t str_len);

int str_to_unsigned_long(unsigned long *dst, char *str);

int str_to_bool(bool *dst, char *str);

int parse_csv_to_ul_array(size_t *ul_array, size_t ul_array_len, char *str);

int sockaddr_storage_to_string(char *buf, size_t buf_len, struct sockaddr_storage *ss);

int utl_readall(int fd, size_t size, void **buf, char *err, size_t err_len);
int utl_writeall(int fd, void *buf, size_t buf_len, char *err, size_t err_len);

int utl_timespec_to_rfc3339nano(struct timespec *ts, char *buf);

void utl_clock_gettime_rt_fatal(struct timespec *tp);

#endif /* UTILS_H */

/** @}*/