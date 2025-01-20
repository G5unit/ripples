/**
 * @file query_parse.c
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
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>

#include "constants.h"
#include "rip_ns_utils.h"
#include "query.h"
#include "utils.h"

/** Parse EDNS extension client subnet.
 * 
 * Format is:<br>
 * 2 bytes         = family<br>
 * 1 byte          = source netmask<br>
 * 1 byte          = scope netmask<br>
 * remaining bytes = client subnet<br>
 * 
 * @param cs      EDNS client subnet object to parse data into.
 *
 * @return        Returns 0 on success, otherwise error occurred i.e.
 *                format error or unsupported address family.
 */
int
query_parse_edns_ext_cs(edns_client_subnet_t *cs)
{

    unsigned char *buf   = cs->edns_cs_raw_buf;
    uint16_t buf_len     = cs->edns_cs_raw_buf_len;
    uint16_t family      = 0;
    uint8_t  source_mask = 0;
    uint8_t  scope_mask  = 0;

    cs->edns_cs_valid = false;

    if (buf_len < 4) {
        /* Invalid format, minimum number of required bytes not present. */
        return -1;
    }

    RIP_NS_GET16(family, buf);
    source_mask = *(uint8_t *)buf;
    buf += 1;
    scope_mask = *(uint8_t *)buf;
    buf += 1;
    uint8_t bytes_to_copy = buf_len - 4;

    if (family == 1) {
        if (source_mask > 32 || scope_mask != 0 || bytes_to_copy > RIP_NS_INADDRSZ) {
            /* Invalid format, MUST be rejected and a FORMERR response */
            return -2;
        }

        struct sockaddr_in *sin = (struct sockaddr_in *)&cs->ip;
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = 0;
        memcpy(&sin->sin_addr.s_addr, buf, bytes_to_copy);
    } else if (family == 2) {
        if (source_mask > 128 || scope_mask != 0 || bytes_to_copy > RIP_NS_IN6ADDRSZ) {
            /* Invalid format, MUST be rejected and a FORMERR response */
            return -3;
        }

        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&cs->ip;
        sin6->sin6_family = AF_INET6;
        memset(&sin6->sin6_addr, 0, sizeof(struct in6_addr));
        memcpy(&sin6->sin6_addr, buf, bytes_to_copy);
    } else {
        /* Unsupported address family. */
        /* Per RFC 7871 section 7.8.1:
         * "
         * A query with a wrongly formatted option (e.g., an unknown FAMILY)
         * MUST be rejected and a FORMERR response MUST be returned to the
         * sender,..
         * "
         */
        return -4;
    }

    uint8_t r = source_mask % 8;
    uint8_t addr_len = source_mask / 8;
    uint8_t comp = 0;
    if (r > 0) {
        addr_len += 1;
        comp = *(buf + addr_len - 1);
    }

    if (addr_len != bytes_to_copy) {
        /* Format error. */
        return -5;
    }

    if (comp > 0) {
        uint8_t shift = 0xff << (8-r);
        if ((shift & comp) != comp) {
            /* Format error. */
            return -6;
        }
    } 

    cs->edns_cs_valid = true;
    cs->family        = family;
    cs->source_mask   = source_mask;
    cs->scope_mask    = scope_mask;

    return 0;
}

/** Parse EDNS extensions.
 * 
 * Format of EDNS option is:
 * 2 bytes              = Option code,
 * 2 bytes              = Option length (length that follows),
 * Option length bytes  = option specific
 * 
 * @param q     Query to parse extensions into.
 * @param buf   Buffer to parse extensions from.
 * @param eobuf End of buffer.
 * 
 * @return      Returns 0 on success, otherwise an error occurred i.e.
 *              format error.
 */
int
query_parse_edns_ext(query_t *q, unsigned char *buf, unsigned char *eobuf)
{
    uint16_t opt_code = 0;
    uint16_t opt_len  = 0;

    while (buf < eobuf) {
        if ((buf + 3) > eobuf) {
            /* Format error, not enough bytes for EDNS extension header. */
            return -1;
        }
        RIP_NS_GET16(opt_code, buf);
        RIP_NS_GET16(opt_len, buf);

        if ((buf + opt_len -1 ) > eobuf) {
            /* Format error, not enough bytes for EDNS extension as indicated by extension length field. */
            return -1;
        }

        switch (opt_code) {
        case rip_ns_ext_opt_c_cs:
            /* Client subnet */
            q->edns.client_subnet.edns_cs_raw_buf = buf;
            q->edns.client_subnet.edns_cs_raw_buf_len = opt_len;
            if (query_parse_edns_ext_cs(&q->edns.client_subnet) != 0) {
                return -1;
            }
            break;

        default:
            /* Extension is not one we support, skip it. */
            break;
        }
        buf += opt_len;
    }
    return 0;
}

/** Parse query request additional section looking for EDNS RR and if found
 * parse EDNS RR.
 * 
 * @param q    Query to parse request additional section EDNS opt RR.
 * @param ptr  Pointer in request buffer where additional section starts.
 * 
 * @return     On success returns number of bytes consumed.
 *             On error returns -1 in which case query parameter end_code is set.
 */
int
query_parse_request_rr_additional_edns(query_t *q, unsigned char *ptr)
{
    unsigned char *start    = ptr;
    unsigned char *eom      = (unsigned char *)(q->request_hdr + q->request_buffer_len - 1);
    int            rr_count = 0;

    if ((ptr + RIP_NS_RRFIXEDSZ) > eom) {
        /* Header indicated additional records present yet size is less than
         * what we expect if additional records were present.
         */
        q->end_code = rip_ns_r_formerr;
        return -1;
    }

    /* Find first occurring OPT RR EDNS, skip all others.
     *
     * The fixed part of an OPT RR (EDNS0) is structured as follows:
     * +------------+--------------+------------------------------+
     *  | Field Name | Field Type   | Description                  |
     *  +------------+--------------+------------------------------+
     *  | NAME       | domain name  | MUST be 0 (root domain)      |
     *  | TYPE       | u_int16_t    | OPT (41)                     |
     *  | CLASS      | u_int16_t    | requestor's UDP payload size |
     *  | TTL        | u_int32_t    | For EDNS these 4 bytes are:
     *
     *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     *  0: |  EXTENDED-RCODE        |               VERSION         |
     *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     *  2: | DO|                       Z                            |
     *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     *
     *  | RDLEN      | u_int16_t    | length of all RDATA          |
     *  | RDATA      | octet stream | {attribute,value} pairs      |
     *  +------------+--------------+------------------------------+
     */
    while (ptr < eom) {
        /* Parse RR name */
        unsigned char adr_name[RIP_NS_MAXCDNAME] = { '\0' };
        int           unpack        = 0;
        uint16_t      rdata_len     = 0;

        unpack = rip_ns_name_unpack((unsigned char   *)q->request_hdr, eom,
                                    ptr, adr_name, 256);
        if (unpack < 1) {
            /* Invalid format, was not able to upack RR name. */
            q->end_code = rip_ns_r_formerr;
            return -1;
        }
        ptr += unpack;
        if (ptr >= eom || (ptr + RIP_NS_RRFIXEDSZ - 1) > eom) {
            /* Invalid format, not enough data for OPT RR */
            q->end_code = rip_ns_r_formerr;
            return -1;
        }
        if (unpack == 1 && strlen((char *)adr_name) == 0 &&
            ntohs(*(uint16_t *)ptr) == rip_ns_t_opt) {
            /* Found OPT record EDNS. */
            q->edns.edns_raw_buf = ptr - unpack;
            ptr += 2;

            /* Note advertized UDP response length. */
            RIP_NS_GET16(q->edns.udp_resp_len, ptr);
            if (q->edns.udp_resp_len < RIP_NS_PACKETSZ) {
                q->edns.udp_resp_len = RIP_NS_PACKETSZ;
            } else if (q->edns.udp_resp_len > RIP_NS_UDP_MAXMSG) {
                q->edns.udp_resp_len = RIP_NS_UDP_MAXMSG;
            }

            /* Check EDNS version, only version '0' is supported. */
            q->edns.version = *ptr;
            if (q->edns.version != 0) {
                /* EDNS version not supported. Per RFC 6891
                 * https://datatracker.ietf.org/doc/html/rfc6891
                 * we MUST respond with RCODE BADVERS.
                 */
                q->edns.udp_resp_len = 512;
                q->end_code = rip_ns_r_badvers;
                return -1;
            }
            ptr += 2;

            /* Check the DO bit for DNSSEC support. */
            uint8_t *u8 = (uint8_t *)ptr;
            if ((*u8 & 0x80) == 0x80) {
                q->edns.dnssec = true;
            }
            ptr += 2;

            /* Get RDATA length. */
            RIP_NS_GET16(rdata_len, ptr);
            if (rdata_len > 0) {
                /* Have EDNS options to parse. */
                if (ptr >= eom || (ptr + rdata_len - 1) > eom) {
                    /* Invalid format, not enough data for EDNS options. */
                    q->end_code = rip_ns_r_formerr;
                    return -1;
                }
                if (query_parse_edns_ext(q, ptr, ptr + rdata_len - 1) != 0) {
                    /* Error parsing EDNS extensions. */
                    q->end_code = rip_ns_r_formerr;
                    return -1;
                }
            }
            q->edns.edns_raw_buf_len = unpack + RIP_NS_RRFIXEDSZ + rdata_len;
            q->edns.edns_valid = true;
            break;
        } else {
            /* Not EDNS, skip RR. */
            ptr += unpack + 8;
            if (ptr + 1 > eom) {
                q->end_code = rip_ns_r_formerr;
                return -1;
            }
            RIP_NS_GET16(rdata_len, ptr);
            ptr += rdata_len;
        }
        INCREMENT(rr_count);
    }
    if (rr_count != ntohs(q->request_hdr->ancount)) {
        q->end_code = rip_ns_r_formerr;
        return -1;
    }
    return ptr - start;
}


/** Parse query request question RR.
 * 
 * Format on the wire per RFC 1035 is:
 * 
 *   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                                               |
 *  /                     QNAME                     /
 *  /                                               /
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QTYPE                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QCLASS                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * 
 * @param q Query for which to parse request question.
 * 
 * @return  On success returns number of bytes from request buffer that are
 *          the question RR. On error returns -1 in which case query parameter
 *          end_code is set.
 */
int 
query_parse_request_rr_question(query_t *q)
{
    int            unpack = 0;
    unsigned char *ptr    = (unsigned char *)q->request_hdr;
    unsigned char *eom    = ptr + q->request_buffer_len -1;

    /* Extract question label, type, and class */
    unpack = rip_rr_name_get(ptr, eom, ptr + sizeof(rip_ns_header_t),
                             q->query_label, q->query_label_size,
                             &q->query_label_len);                                       
    if (unpack < 1) {
        q->end_code =  rip_ns_r_formerr;
        return -1;
    }
    ptr += sizeof(rip_ns_header_t) + unpack;
    if (ptr + 3 > eom) {
        /* Message is not long enough to satisfy parsing of RR. */
        q->end_code =  rip_ns_r_formerr;
        return -1;
    }
    RIP_NS_GET16(q->query_q_type, ptr);
    if (!rip_ns_rr_type_supported(q->query_q_type)) {
        /* RR type not supported. */
        q->end_code = rip_ns_r_notimpl;
        return -1;
    }
    RIP_NS_GET16(q->query_q_class, ptr);
    if (!rip_ns_rr_class_supported(q->query_q_class)) {
        /* RR class not supported. */
        q->end_code = rip_ns_r_notimpl;
        return -1;
    }
    return unpack + 4;
}

/** Parse query request from request buffer into query_t structure fields.
 * 
 * DNS request & response format is as per RFC 1035
 * https://datatracker.ietf.org/doc/html/rfc1035.
 *
 * @param q Query to parse DNS request for.
 */
void
query_parse(query_t *q)
{
    unsigned char   *ptr    = (unsigned char *)q->request_hdr;
    unsigned char   *eom    = ptr + q->request_buffer_len -1;
    rip_ns_header_t *header = q->request_hdr;
    int              unpack = 0;

    q->end_code = -1;

    /* Data received should at minimum contain the header. */
    if (q->request_buffer_len < sizeof(rip_ns_header_t)) {
        /* Drop the packet. */
        RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, rip_ns_r_rip_shortheader);
    }

    /* A truncated query is not suported. */
    if (header->tc != 0) {

        RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, rip_ns_r_rip_query_tc);
    }

    /* Queries of type Question are only ones supported. */
    if (header->opcode != rip_ns_o_query) {
        RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, rip_ns_r_notimpl);
    }

    /* Response flag should not be set */
    if (header->qr != 0) {
        RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, rip_ns_r_formerr);
    }

    /* Multiple labels in question are not supported. */
    if (ntohs(header->qdcount) != 1) {
        if (ntohs(header->qdcount) == 0) {
            /*  received question with no question RRs*/
            RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, rip_ns_r_formerr);
        } else {
            /* Question received with more than 1 question RRs*/
            RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, rip_ns_r_notimpl);
        }
    }

    /* Answer entries and Authority entries in question are not supported. */
    if (ntohs(header->ancount) != 0 || ntohs(header->nscount) != 0) {
        RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, rip_ns_r_formerr);
    }

    q->dnptrs[0] = (unsigned char *)q->response_hdr;

    /* Extract question label, type, and class */
    unpack = query_parse_request_rr_question(q);
    if (unpack < 0) {
        return;
    }

    ptr += sizeof(rip_ns_header_t) + unpack;

    /* Are additional opt RRs present */
    uint16_t adr_count = ntohs(header->arcount);
    if (adr_count > 0) {
        /* Parse additional opt RRs, and look for EDNS. */
        ptr += query_parse_request_rr_additional_edns(q, ptr);
    }

    /* Are there extra bytes at the end? */
    if (ptr - 1 != eom) {
        /* Extra unaccounted bytes present.
         * For now ignore it.
         */
    }
}
