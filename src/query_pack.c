/**
 * @file query_pack.c
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
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

#include <stdio.h>

#include "query.h"
#include "rip_ns_utils.h"
#include "rr_record.h"

/** Pack EDNS and it extensions into buffer.
 * 
 * @param buf     Buffer to pack EDNS and extensions into.
 * @param buf_len Length in bytes of available space in buffer.
 * @param edns    EDNS object to pack.
 *
 * @return        On success returns number of bytes packed into buffer.
 *                0 is returned if edns object does not contain valid data.
 *                -1 is returned if there is not enough room in buffer to pack
 *                EDNS and all its extensions in which case buf is left
 *                untouched.
 */
int
query_pack_edns(uint8_t *buf, uint16_t buf_len, edns_t *edns) {
    uint16_t opts_len = 0;

    uint8_t cs_opt_len = 0;
    uint8_t cs_ip_len  = 0;
    
    if (!edns->edns_valid) {
        return 0;
    }

    /* Is there enough room to pack EDNS and its extensions. */
    opts_len += 1 + RIP_NS_RRFIXEDSZ; /* EDNS header */

    if (edns->client_subnet.edns_cs_valid) {
        /* Length needed for client subnet extension. */
        cs_ip_len = edns->client_subnet.source_mask / 8;
        uint8_t r = edns->client_subnet.source_mask % 8;
        if (r > 0) {
            cs_ip_len += 1;
        }
        cs_opt_len = 4 + cs_ip_len;
        opts_len += 4 + cs_opt_len;
    }

    if (buf_len < opts_len) {
        /* Not enough room to pack edns. */
        return -1;
    }

    /* Pack EDNS (name, option code, udp resp. len, extended rcode,
     *            version, DO bit, rdata len).
     */
    *buf = 0;
    buf += 1;
    RIP_NS_PUT16(rip_ns_t_opt, buf);
    RIP_NS_PUT16(edns->udp_resp_len, buf);
    *buf = edns->extended_rcode;
    buf += 1;
    *buf = 0;
    buf += 1;
    *buf  = edns->dnssec << 7;
    buf += 1;
    *buf = 0;
    buf += 1;
    RIP_NS_PUT16((opts_len - 1 - RIP_NS_RRFIXEDSZ) , buf);

    /* Pack client subnet. */
    if (edns->client_subnet.edns_cs_valid) {
        uint8_t *cs_ip = NULL;

        if (edns->client_subnet.family == 1) {
            /* IPv4 */
            struct sockaddr_in *sin = (struct sockaddr_in *)&edns->client_subnet.ip;
            cs_ip = (uint8_t *)&sin->sin_addr.s_addr;
        } else {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&edns->client_subnet.ip;
            cs_ip = (uint8_t *)&sin6->sin6_addr;
        }
        /* Pack EDNS opt header (opt code and opt length) */
        RIP_NS_PUT16(rip_ns_ext_opt_c_cs, buf);
        RIP_NS_PUT16(cs_opt_len, buf);

        /* Pack client subnet (family, source mask, scope mask, ip)*/
        RIP_NS_PUT16(edns->client_subnet.family, buf);
        *buf = edns->client_subnet.source_mask;
        buf += 1;
        *buf = edns->client_subnet.scope_mask;
        buf += 1;
        memcpy(buf, cs_ip, cs_ip_len);
    }

    return opts_len;
}

/** Pack resource record into buffer, non-EDNS. TO pack EDNS use @ref
 * query_pack_edns.
 * 
 * @param name      Name to use to when packing RR. If NULL name in rr is used.
 * @param rr        Resource record to pack.
 * @param buf       Buffer into which to pack the record.
 * @param buf_len   Length (in bytes) of available space in buffer.
 * @param dnptrs    Pointer to first entry in domain name array used to track
 *                  names already packed.
 * @param lastdnptr Last entry in domain name array.
 *
 * @return          On success returns number of bytes packed into buffer,
 *                  Otherwise error occurred. Currently error can occur if
 *                  there if not enough room in buffer to pack the record.
 */
int
query_pack_rr(const unsigned char *name, rr_record_t *rr, unsigned char *buf, uint16_t buf_len,
              const unsigned char **dnptrs, const unsigned char **lastdnptr)
{
    int packed_name_len = 0;
    int packed_len      = 0;

    /* pack name */
    if (name != NULL) {
        packed_name_len = rip_ns_name_put(name, buf, buf_len, dnptrs, lastdnptr);
    } else {
        packed_name_len = rip_ns_name_put(rr->name, buf, buf_len, dnptrs, lastdnptr);
    }
    if (packed_name_len < 0) {
        /* Not enough space in buf to pack RR name. */
        return -3;
    }
    buf += packed_name_len;

    packed_len = packed_name_len+ RIP_NS_RRFIXEDSZ + rr->rdata_len;
    if (packed_len > buf_len) {
        /* Not enough space in buf to pack full RR. */
        return -3;
    }

    /* pack type */
    RIP_NS_PUT16(rr->type, buf);

    /* pack class */
    RIP_NS_PUT16(rr->class, buf);

    /* pack ttl */
    RIP_NS_PUT32(rr->ttl, buf);

    /* pack rdata length */
    RIP_NS_PUT16(rr->rdata_len, buf);

    /* pack rdata */
    memcpy(buf, rr->rdata, rr->rdata_len);

    return packed_len;
}


/** Pack query response into query response buffer.
 * 
 * @param q Query to pack response for
 * 
 * @return  Returns 0 on success otherwise error occurred:
 *          -1 indicates that there was not enough room to pack full response
 *             and response is truncated.
 */
int
query_response_pack(query_t *q)
{

    rip_ns_header_t *resp_hdr = q->response_hdr;
    int ret = 0;

    /* Pack header */
    memset(resp_hdr, 0, sizeof(rip_ns_header_t));
    resp_hdr->id      = q->request_hdr->id;
    resp_hdr->rd      = q->request_hdr->rd;
    resp_hdr->tc      = 0;
    resp_hdr->aa      = 1;
    resp_hdr->opcode  = rip_ns_o_query;
    resp_hdr->qr      = 1;
    resp_hdr->rcode   = 0;
    resp_hdr->ancount = 0;
    resp_hdr->nscount = 0;
    resp_hdr->arcount = 0;

    if (q->end_code < 16) {
        resp_hdr->rcode  = q->end_code;
    } else {
        /* it is the extended rcode */
        q->edns.extended_rcode = (q->end_code >> 4);
    }

    unsigned char *buf = (unsigned char *)q->response_hdr + sizeof(rip_ns_header_t);
    int rrs_packed_len = sizeof(rip_ns_header_t);
    int pack_len       = 0;

    /* Pack answer section */
    resp_hdr->ancount = htons(q->answer_section_count);
    for (int i = 0; i < q->answer_section_count; i++) {
        pack_len = query_pack_rr(NULL, q->answer_section[i], buf,
                                 q->response_buffer_size - rrs_packed_len,
                                 &q->dnptrs[0],
                                 &q->dnptrs[DNS_RESPONSE_COMPRESSED_NAMES_MAX-1]);
        if (pack_len < 0) {
            resp_hdr->tc = 1;
            ret = -1;
            goto END;
        }
        rrs_packed_len += pack_len;
        buf += pack_len;
    }

    /* Pack authority section */
    resp_hdr->nscount = htons(q->authority_section_count);
    for (int i = 0; i < q->authority_section_count; i++) { 
        pack_len = query_pack_rr(NULL, q->authority_section[i], buf,
                                 q->response_buffer_size - rrs_packed_len,
                                 &q->dnptrs[0],
                                 &q->dnptrs[DNS_RESPONSE_COMPRESSED_NAMES_MAX-1]);
        if (pack_len < 0) {
            resp_hdr->tc = 1;
            ret = -1;
            goto END;
        }
        rrs_packed_len += pack_len;
        buf += pack_len;
    }

    /* Pack additional section */
    for (int i = 0; i < q->additional_section_count; i++) { 
        pack_len = query_pack_rr(NULL, q->additional_section[i], buf,
                                 q->response_buffer_size - rrs_packed_len,
                                 &q->dnptrs[0],
                                 &q->dnptrs[DNS_RESPONSE_COMPRESSED_NAMES_MAX-1]);
        if (pack_len < 0) {
            resp_hdr->tc = 1;
            ret = -1;
            goto END;
        }
        rrs_packed_len += pack_len;
        buf += pack_len;
    }

    /* Pack edns */
    pack_len = query_pack_edns(buf, q->response_buffer_size - rrs_packed_len, &q->edns);
    if (pack_len < 0) {
        resp_hdr->tc = 1;
        ret = -1;
        goto END;
    } else if (pack_len > 0) {
        q->additional_section_count += 1;
    }
    rrs_packed_len += pack_len;
    buf += pack_len;   

END:
    resp_hdr->arcount = htons(q->additional_section_count);

    q->response_buffer_len = rrs_packed_len;

    /* If TCP add prefix with length of data. */
    if (q->protocol == 1) {
        /* TCP */
        rip_ns_put16(q->response_buffer, (uint16_t )q->response_buffer_len);
        q->response_buffer_len += 2;
    }

    return ret;
}