/**
 * @file utils.c
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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

/** From sockaddr_storage extract IP and port into strings.
 * 
 * @param ip       Buffer where to store IP address.
 * @param ip_len   Length of space available in ip buffer.
 * @param port     Buffer where to store TCP/UDP port.
 * @param ss       sockaddr_storage to extract IP and port from.
 * 
 * @return         0 on success, otherwise error occurred.
 */
int
utl_ip_port_from_ss(char *ip, size_t ip_len, uint16_t *port,
                    struct sockaddr_storage *ss) {
    struct sockaddr_in  *sin  = (struct sockaddr_in *)ss;
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ss;

    if (((ss->ss_family != AF_INET && ss->ss_family != AF_INET6)) ||
        (ss->ss_family == AF_INET6 && ip_len < INET6_ADDRSTRLEN)) {
        return -1;
    }
    if (ss->ss_family == AF_INET) {
        if (ip_len < INET_ADDRSTRLEN) {
            return 0;
        }
        if (inet_ntop(sin->sin_family, &sin->sin_addr, ip, ip_len) == NULL) {
            return errno;
        }
        *port = ntohs(sin->sin_port);
    } else if (ss->ss_family == AF_INET6)  {
        if (ip_len < INET6_ADDRSTRLEN) {
            return 0;
        }
        if (inet_ntop(sin6->sin6_family, &sin6->sin6_addr, ip, ip_len) == NULL) {
            return errno;
        }
        *port = ntohs(sin6->sin6_port);
    } else {
        return -1;
    }
    return 0;
}

/** Get a difference between two timespec stuctures t1 - t2. 
 * @note t1 MUST be larger than t2.
 * 
 * @param dst Where to store result.
 * @param t1  Time to subtract from.
 * @param t2  Time to subtract.
 */
void utl_diff_timespec(struct timespec *dst, const struct timespec *t1,
                       const struct timespec *t2)
{
    *dst = (struct timespec) {
        .tv_sec = t1->tv_sec - t2->tv_sec,
        .tv_nsec = t1->tv_nsec - t2->tv_nsec
    };
    if (dst->tv_nsec < 0) {
        dst->tv_nsec += 1000000000;
        dst->tv_sec--;
    }
}

/** Get a difference between two timespec stuctures t1 - t2 and return the
 * result as type double. 
 * 
 * @param t1  Time to subtract from.
 * @param t2  Time to subtract.
 * 
 * @return    Time difference as double.
 */
double
utl_diff_timespec_as_double(const struct timespec *t1, const struct timespec *t2)
{
    return (t1->tv_sec - t2->tv_sec) +
           (t1->tv_nsec - t2->tv_nsec) / 1000000000.0;
}

/** Convert ascii character to lower case. If character is not alphabet [A-Z] or
 * is already in lower case, this is a no-op.
 * 
 * @param c Character to convert to lower case.
 */
inline void
char_to_lc(char *c)
{
    if (*c >= 65 && *c <= 90) {
        /* Character is in upper case, convert it to lower. */
        *c  = *c + 32;
    }
}

/** Convert string ascii characters to lower case.
 *
 * @param str     String to convert to all lower case ASCII.
 * @param str_len Length of string.
 */
void
str_to_lc(uint8_t *str, size_t str_len)
{
    for (int i = 0; i < str_len; i++) {
        char_to_lc((char *)str);
    }
}

/** Check if ascii string is solely composed of numeric [0-9] characters.
 * 
 * @param str     String to check.
 * @param str_len Length of string.
 * 
 * @return  Returns tue if ascii string is numeric, otherwise returns false.
 */
bool str_is_numeric(uint8_t *str, size_t str_len)
{
    for (int i = 0; i < str_len; i++) {
        if (!isdigit(str[i])) {
            /* Character is not a digit */
            return false;
        }
    }
    return true;
}

/** Convert string to unsigned long and store result in dst.
 * This function only handles base 10 numbers.
 * 
 * @param dst Destination to store value to
 * @param str String to convert to unsigned long.
 * 
 * @return    0 on success, otherwise an error occurred.
 */
int
str_to_unsigned_long(unsigned long *dst, char *str)
{
    char         *end     = NULL;
    size_t        str_len = strlen(str);
    unsigned long ret;

    errno = 0;
    ret = strtoul(str, &end, 10);

    if (errno != 0) {
        return -1;
    }
    
    if (end != (str + str_len) || *end != '\0') {
        /* string passed in is not composed solely of digits. */
        return -1;
    }
    
    *dst = ret;

    return 0;
}

/** Convert strings "true" and "false" to boolean and store result in dst.
 * Input strings can be a mix of upper and lower case characters.
 * 
 * @param dst Destination to store value to
 * @param str String to convert to bool.
 * 
 * @return    0 on success, otherwise an error occurred, meaning string does not
 *            represent "true" or "false".
 */
int str_to_bool(bool *dst, char *str)
{
    static char *bool_true  = "true";
    static char *bool_false = "false";

    char tmp_str[6] = {'\0'};

    size_t str_len = strlen(str);

    if (str_len == 0 || str_len > 5) {
        return -1;
    }

    strncpy(tmp_str, str, 5);
    str_to_lc((uint8_t *)tmp_str, str_len);
    
    if (memcmp(tmp_str, bool_true, str_len) == 0) {
        *dst = true;
    } else if (memcmp(tmp_str, bool_false, str_len) == 0) {
        *dst = false;
    } else {
        return -1;
    }

    return 0;
}

/** Parse comma separated values into array of unsigend long integers.
 * 
 * strtok() is not used as it skips empty entries (i.e. ',,').
 * 
 * @param ul_array     Array to parse entries into.
 * @param ul_array_len Number of slots in array.
 * @param str          String to parse CSV from.
 * 
 * @return             0 on success, otherwise an error occurred.
 */
int
parse_csv_to_ul_array(size_t *ul_array, size_t ul_array_len, char *str)
{
    size_t  str_len   = strlen(str);
    size_t  entry     = 0;
    size_t  entry_len = 0;
    char   *p         = str;
    char   *start     = p;

    if (ul_array_len == 0 || str_len == 0) {
        return 0;
    }

    while (entry < ul_array_len && p < (p + strlen(p) + 1)) {
        if (*p == ',' || *p == '\0') {
            entry_len = p - start;
            if (entry_len > 0 && entry_len < 5) {
                /* Parse entry and put result in ul_array[entry]. */
                char tmp_str[5] = {'\0'};
                strncpy(tmp_str, start, entry_len);
                unsigned long num = 0;
                if (str_to_unsigned_long(&num, tmp_str) != 0) {
                    return -1;
                }
                ul_array[entry] = num;
            } else {
                ul_array[entry] = 0;
            }
            entry += 1;
            start = p + 1;
        } else if (!isdigit(*p)) {
            return -1;
        }

        p += 1;
    }
    return 0;
}

/** Convert IPv4 or IPv6 addresses and port from binary to text form as
 * "ip:port".
 * 
 * @param buf     Buffer to store string into.
 * @param buf_len Length of buffer, must be large enough to account for IPv6,
 *                ":", port (5 characters), plus a '\0' string terminator.
 * @param ss      struct_sockaddr to extract IP and port from.
 * 
 * @return         0 on success, otherwise error occurred.
 *                 -1 indicates that buf_len was too small,
 *                 any other value is errno set by inet_ntop().
 */
int
sockaddr_storage_to_string(char *buf, size_t buf_len, struct sockaddr_storage *ss)
{
    char ip[INET6_ADDRSTRLEN];
    uint16_t port = 0;
    size_t str_len;
    size_t min_len;

    if (utl_ip_port_from_ss(ip, INET6_ADDRSTRLEN, &port, ss) != 0) {
        return -1;
    }
    str_len = strlen(ip);
    min_len = str_len + 6;
    if (ss->ss_family == AF_INET6) {
        min_len += 2; /* For [] around IPv6. */
    }
    if (buf_len < min_len) {
        return -2;
    }
    if (ss->ss_family == AF_INET6) {
        snprintf(buf, buf_len, "[%s]:%d", ip, port); 
    } else {
        snprintf(buf, buf_len, "%s:%d", ip, port);
    }
    return 0;
}

/** Read size specified from file descriptor into memory. 
 * 
 * @note Per Linux manual read() will return at most 0x7ffff000 (2,147,479,552)
 *       bytes, hence a need for this function to keep reading until the
 *       full size specified is read into memory buffer.
 * 
 * @param fd       Open file descriptor to read from, seek position MUST
 *                 be at the begining of the file.
 * @param size     Number of bytes to read in.
 * @param buf      Pointer where to store buffer data is read into.
 * @param err      Buffer where to store error string if error occurred. Error
 *                 string is guaranteed to end with '\0' so i.e. a followup call
 *                 to strlen() is a valid action.
 * @param err_len  Size of err buffer.
 * 
 * @return         On success returns 0, otherwise error occurred in which case
 *                 *buf is set to NULL and error message is populated.
 */
int
utl_readall(int fd, size_t size, void **buf, char *err, size_t err_len)
{
    size_t  read_count = 0;
    ssize_t ret;

    if (size == 0) {
        /* Empty file. */
        return -1;
    }

    *buf = malloc(sizeof(char) * size);
    CHECK_MALLOC(*buf);
    while (read_count < size) {
        ret = read(fd, *buf + read_count, size - read_count);
        if (ret > 0) {
            read_count += ret;
        } else {
            /* Error. */
            free(*buf);
            *buf = NULL;
            char *err_str;
            if (ret == 0) {
                /* Zero bytes read in. */
                err_str = "unkown error, zero bytes read in";
            } else {
                /* ret < 0, Error reading from file. */
                err_str = strerror(errno);
            }
            if (err != NULL && err_len != 0) {
                strncpy(err, err_str, err_len);
                *(err + err_len -1) = '\0'; /* In case exactly err_len bytes were copied. */
            }
            return -1;
        }
    }

    return 0;    
}

/** Write data from buffer to open file descriptor fd until all data was written.
 * 
 * 
 * 
 * @param fd       Open file descriptor to write to.
 * @param buf      Buffer to write data from.
 * @param buf_len  Length of data in buff to write.
 * @param err      Buffer where to store error message if one is encountered.
 * @param err_len  Size of err buffer available to store error message.
 * 
 * @return         On success returns 0. Otherwise error occurred and error
 *                 message was populated. If error occurred it is possible that
 *                 only part of the data was written.
 *                 
 */
int
utl_writeall(int fd, void *buf, size_t buf_len, char *err, size_t err_len)
{
    size_t  write_count = 0;
    ssize_t ret;

    char *src = (char *)buf;

    if (buf_len == 0) {
        return 0;
    }
    while (write_count < buf_len) {
        errno = 0;
        ret = write(fd, &src[write_count], buf_len - write_count);
        if (ret > 0) {
            write_count += ret;
        } else {
            char *err_str;
            if (ret == 0) {
                /* Zero bytes read in. */
                err_str = "unkown error";
            } else {
                /* ret < 0, Error reading from file. */
                err_str = strerror(errno);
            }
            if (err != NULL && err_len != 0) {
                strncpy(err, err_str, err_len);
                *(err + err_len -1) = '\0'; /* In case exactly err_len bytes were copied. */
            }
            return -1;
        }
    }
    return 0;
}

/** Store time in ts as string in GMT rfc3339nano format.
 * 
 * @param ts  Time to store as string.
 * @param buf Buffer to store times string at. This buffer MUST have minimum
 *            length of @ref TIME_RFC3339_STRLEN.
 */
int
utl_timespec_to_rfc3339nano(struct timespec *ts, char *buf)
{
    strftime(buf, 31, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&ts->tv_sec));
    return 19 + sprintf(buf+19, ".%ldZ", ts->tv_nsec);
    
}

/** Get current time from clock CLOCK_REALTIME (see man -3 time) and store it
 * in tp.
 * 
 * @note This function is fatal on error. Error can only occur if there is
 *       something majorly wrong with underlying OS. Regardless, not being
 *       able to get current time impacts critical application function. 
 * 
 * @param tp Where to store current time.
 */
void
utl_clock_gettime_rt_fatal(struct timespec *tp)
{
    if (clock_gettime(CLOCK_REALTIME, tp) != 0) {
        assert(0);
    }  
}

