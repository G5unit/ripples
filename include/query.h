/**
 * @file query.h
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
/** \defgroup query DNS Query
 *
 * @brief DNS query is composed of a DNS request (question) and response (answer).
 *        This application only handles DNS query type "question", and provides
 *        a simple response. 
 *        Implementation of DNS records database (zones) is not done as that is
 *        beyond the scope of purpose for this project.
 * 
 *        Parsing EDNS opt RR is supported along with EDNS client subnet
 *        extension.
 * 
 *        A single structure is used to hold query request and query response.
 *        Depending if query arrived over UDP or TCP, appropriate buffers are
 *        made available providing access to raw query data (as received or to
 *        be sent over the network).
 *  @{
 */
#ifndef QUERY_H
#define QUERY_H

#include <stdbool.h>
#include <sys/socket.h>
#include <time.h>

#include "channel.h"
#include "config.h"
#include "constants.h"
#include "metrics.h"
#include "rip_ns_utils.h"
#include "rr_record.h"


/** Structure for EDNS client subnet option. */
typedef struct edns_client_subnet_s {
    /** Pointer in request_buffer where EDNS client subnet opt begins.*/
    unsigned char *edns_cs_raw_buf;

    /** Length of data in raw buffer for EDNS client subnet opt. 
     * A length of 0 means that EDNS client subnet is not present in query request.
     */
    uint16_t edns_cs_raw_buf_len;

    /** Flag to indicate if EDNS client subnet is present & valid
     * (should be used when forming a response).
     */
    bool edns_cs_valid;

    /** Client subnet IP family, 1 = IPv4, 2 = IPv6. */
    uint16_t family;

    /** Client subnet IP. */
    struct sockaddr_storage ip;

    /** Client subnet source mask. This is the mask requested. 
     * Set by query, response mirrors what was in query. (RFC 7871)
     */
    uint8_t source_mask;
    
    /** Client subnet scope mask. This is the mask that we are responding
     * for. 
     * Set by response, in query it MUST be 0 (RFC 7871).
     */
    uint8_t scope_mask;
} edns_client_subnet_t;

/** Structure for EDNS optional resource record. */
typedef struct edns_s {
    /**  Pointer in request_buffere where EDNS rr begins.*/
    unsigned char *edns_raw_buf;

    /** Length of data in raw buffer for EDNS rr. 
     * A length of 0 means that EDNS rr is not present in query request.
     */
    uint16_t edns_raw_buf_len;

    /** Flag to indicate if EDNS is present & valid (should be used when
     * forming a response).
     */
    bool edns_valid;

    /** Extended response code, upper 8 bits. */
    uint8_t extended_rcode;

    /** EDNS version. The only supported versin is 0. */
    uint8_t version;

    /** EDNS advertized client supported UDP response length */
    uint16_t udp_resp_len;

    /** DNSSEC supported (DO bit set). */
    bool dnssec;

    /** EDNS client subnet */
    edns_client_subnet_t client_subnet; 
} edns_t;

/** Structure describes a DNS query.  */
typedef struct query_s {
    /** Transport protocol this query uses: 0 - UDP, 1 - TCP. */
    uint8_t protocol;

    /** Request client IP */
    struct sockaddr_storage *client_ip;

    /** Request local IP */
    struct sockaddr_storage *local_ip;

    /*** REQUEST ***/
    /** Request buffer with raw DNS request. */
    unsigned char *request_buffer;

    /** Request buffer size. */
    size_t request_buffer_size;

    /** Length of request data in buffer.
     * 
     * @note If protocol is TCP then this does NOT include the 2 bytes prefix.
     */
    size_t request_buffer_len;

    /** Request DNS message HEADER. Points to the place in request buffer
     * where DNS message starts.
     */
    rip_ns_header_t *request_hdr;

    /** Buffer to extract query question label, unpacked & uncompressed and has
     * "." at the end.
     * Label is checked that it conforms to RFC limits, max length 255 for
     * full domain name, and 63 for individual labels in domain name.
     *
     */
    unsigned char *query_label;

    /** Size of query_label buffer */
    uint16_t query_label_size;

    /** Length of query_label string not including string '\0' terminator. */
    uint16_t query_label_len;

    /** Query question type, is one of values in enum @ref rip_ns_type_t.
     * When parsing this from a query it MUST be one of the supported values as
     * identified by function @ref rip_ns_rr_type_supported.
     */
    uint16_t query_q_type;

    /** Query question class, is one of values in enum @ref rip_ns_class_t.
     * When parsing this from a query it MUST be one of the supported values as
     * identified by function @ref rip_ns_rr_class_supported.
     */
    uint16_t query_q_class;

    /** Parsed EDNS(0) if present and valid. */
    edns_t edns;

    /*** RESPONSE ***/
    /** Response buffer to pack response into.
     *
     * @note If protocol is TCP then packing the response needs to account for
     * this and populate the 2 byte prefix and include it in response length.
     * Hence, this points to place where the 2 byte prefix begins.
     */
    unsigned char *response_buffer;

    /** Response buffer size. */
    size_t response_buffer_size;

    /** Length of data in response buffer.
     *
     * @note If protocol is TCP then packing the response needs to account for
     * this and populate the 2 byte prefix and include it in response length.
     */
    size_t response_buffer_len;
    
    /** Response DNS message HEADER. This points to place in response buffer
     * where DNS message begins.
     * 
     * @note If TCP this does not point to the place where 2 byte prefix is, it
     * points to where DNS response header begins.
     */
    rip_ns_header_t *response_hdr;

    /** Array to put response answer section resource records. */
    rr_record_t *answer_section[RIP_NS_RESP_MAX_ANSW];

    /** Number of entries in answer_section array. */
    uint8_t answer_section_count;

    /** Array to put response authority section resource records. These are then
     * packed into response buffer.
     */
    rr_record_t *authority_section[RIP_NS_RESP_MAX_NS];

    /** Number of entries in authority_section array. */
    uint8_t authority_section_count;

    /** Array to put response additional section resource records. This excludes
     * EDNS. These are then packed into response buffer.
     */
    rr_record_t *additional_section[RIP_NS_RESP_MAX_ADDL];
 
    /** Number of entries in additional_section array. */
    uint8_t additional_section_count;

    /** Timestamp when query request was read in from socket. */
    struct timespec start_time;

    /** Timestamp when query response was written to socket. */
    struct timespec end_time;

    /** Query End code.
     * Positive codes >=0 correspond to RCODE and it also means that response
     * should be sent. RCODEs are enumerated in type @ref rip_ns_rcode_t. 
     * 
     * Code -1 means application is still processing the request. This is used
     * by intermediary vectorloop functions as request is being processed to
     * indicate if request processing is complete or not. I.e. if parsing
     * the request indicated an error such that the end_code was set and answer
     * packed, it indicates to next vectorloop step, which is resolve_query,
     * that it should not handle this query in vector.
     * 
     * Negative codes <-1 correspond to other error where response is not sent.
     * These are rare as most errors have a corresponding DNS message response
     * code such as "BAD FORMAT" or "NOT IMPLEMENTED", "REFUSED", or "SERVFAIL".
     * 
     * ns_r_badvers
     * ns_r_notzone
     * ns_r_notauth
     * ns_r_nxrrset
     * ns_r_yxrrset
     * ns_r_yxdomain
     * ns_r_refused
     * ns_r_notimpl
     * ns_r_nxdomain
     * ns_r_servfail
     *  1 - ns_r_formerr
     *  0 - ns_r_noerror
     * -1 - decision not made, keep processing request as it traverses vectorloop functions.
     * -2 - invalid format (incomplete header)
     * -3 - invalid format datagram > RIP_NS_PACKETSZ
    */
    int end_code;

    /** If applicable holds an error message useful for logging purpose. Any
     * string populated MUST end with '\0' terminator.
     */
    char error_message[ERR_MSG_LENGTH];

    /** Array of pointers used when packing RR records into response. */
    const unsigned char *dnptrs[DNS_RESPONSE_COMPRESSED_NAMES_MAX];
} query_t;


void query_init(query_t *q, config_t *cfg, uint8_t protocol);
void query_reset(query_t *q);
void query_clean(query_t *q);
int  query_tcp_response_buffer_increase(query_t *q);

int  query_parse_edns_ext_cs(edns_client_subnet_t *cs);
int  query_parse_edns_ext(query_t *q, unsigned char *buf, unsigned char *eobuf);
int  query_parse_request_rr_additional_edns(query_t *q, unsigned char *ptr);
int  query_parse_request_rr_question(query_t *q);
void query_parse(query_t *q);

void query_resolve(query_t *q);

int  query_pack_edns(uint8_t *buf, uint16_t buf_len, edns_t *edns);
int  query_pack_rr(const unsigned char *name, rr_record_t *rr, unsigned char *buf, uint16_t buf_len,
                   const unsigned char **dnptrs, const unsigned char **lastdnptr);
int  query_response_pack(query_t *q);

/** Stucture describes a query log object.
 *
 * Queries are logged into a buffer. There are two buffers, one is active,
 * while the other one (inactive buffer) is having its data written to disk.
 * This enables us to have no blocking operations (such as writing to disk)
 * in the vectorloop.
 * 
 * Query logging thread uses channels to notify vectorloop thread when it should
 * swap log buffers. Once VL thread swaps buffers, it sends a message back to
 * query logging thread indicating so, which in turn signals the query logging
 * thread that it should log data from VL inactive buffer to disk. Once data
 * is written to disk the process repeats.
 * 
 * Each vectorloop thread has its own set of query log buffers.
 * 
 * Size of each buffer is set by configuration setting
 * query_log_buffer_size.
 */
typedef struct query_log_s {
    /** Size (capacity) of buffers. */
    size_t buf_size;

    /** Buffer A */
    char *a_buf;

    /** Length of data in buffer A. */
    size_t a_buf_len;

    /** Buffer B */
    char *b_buf;
    /** Length of data in buffer B. */
    size_t b_buf_len;

    /** Pointer to active buffer. */
    char *buf;

    /** Length of data in active buffer. */
    size_t buf_len;
} query_log_t;

/** Structure holds arguments passed to @ref query_log_loop function. */
typedef struct query_log_loop_args_s {
    /** Configuration where settings are taken from. */
    config_t *cfg;

    /** Array of query log channels used to exchange messages with vectorloop. */
    channel_bss_t *query_log_channels;

    /** Number of entries in query_log_channels array. */
    size_t query_log_channel_count;

    /** Channel used to send application log messages. */
    channel_log_t *app_log_channel;

    /** Structure to report metrics. */
    metrics_t *metrics;

} query_log_loop_args_t;

int  query_log(char *buf, size_t buf_len, query_t *q);
void query_log_rotate(query_log_t *query_log);

void * query_log_loop(void *args);


void query_report_metrics(query_t *, metrics_t *metrics);

#endif /* End of QUERY_H */

/** @}*/