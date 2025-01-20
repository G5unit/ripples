/**
 * @file rr_record.h
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
/** \addtogroup query
 *
 *  @{
 */
#ifndef RR_RECORD_H
#define RR_RECORD_H

#include <stdint.h>

/** Structure describes a DNS Resource Record. */
typedef struct rr_record_s {
    /** Resource record domain name as string, uncompressed. MUST have a string
     * terminating '\0' character at the end.
     */
    unsigned char *name;

    /** Length (in bytes) of resource record name, excluding the string '\0'
     * terminating character. Representation is in host order.
     */
    uint16_t name_len;  /* In host order (uncompressed). */

    /** Resource record type. Valid types are defined as @ref rip_ns_type_t. */
    uint16_t type;

    /** Resource record class. Valid classes are defined as @ref rip_ns_class_t. */
    uint16_t class;

    /** Record TTL. Representation is in host order. */
    uint32_t ttl;

    /** Resource record data length. Representation is in host order. */
    uint16_t rdata_len;

    /** Resource record data. Representation is in network order. */
    uint8_t *rdata;
} rr_record_t;

#endif /* End of RR_RECORD_H */

/** @}*/
