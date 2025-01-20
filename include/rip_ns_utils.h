/**
 * @file rip_ns_utils.h
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
 *
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/** \defgroup nsutils NS Utilities
 * 
 * @brief Collection of utilities for working with DNS queries. These utilities 
 *        were copied from glibc then modified.
 *  @{
 */
#ifndef RIP_NS_UTILS_H
#define RIP_NS_UTILS_H

#include <endian.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Define constants based on RFC 883, RFC 1034, RFC 1035 */
#define RIP_NS_PACKETSZ     512     /**< default UDP packet size */
#define RIP_NS_UDP_MAXMSG   4096    /**< maximum UDP response message size */
#define RIP_NS_MAXMSG       65535   /**< maximum message size */
#define RIP_NS_MAXCDNAME    255     /**< maximum compressed domain name */
#define RIP_NS_MAXLABEL     63      /**< maximum length of domain label */
#define RIP_NS_QFIXEDSZ     4       /**< #/bytes of fixed data in query */
#define RIP_NS_RRFIXEDSZ    10      /**< #/bytes of fixed data in r record */
#define RIP_NS_INT32SZ      4       /**< #/bytes of data in a uint32_t */
#define RIP_NS_INT16SZ      2       /**< #/bytes of data in a uint16_t */
#define RIP_NS_INT8SZ       1       /**< #/bytes of data in a uint8_t */
#define RIP_NS_INADDRSZ     4       /**< IPv4 T_A */
#define RIP_NS_IN6ADDRSZ    16      /**< IPv6 T_AAAA */
#define RIP_NS_CMPRSFLGS    0xc0    /**< Flag bits indicating name compression. */

/** Maximum number of answer record query response could have. */
#define RIP_NS_RESP_MAX_ANSW 128    

/** Maximum number of authority record query response could have. */
#define RIP_NS_RESP_MAX_NS 16

/** Maximum number of additional record query response could have, excluding
 * EDNS.
 */
#define RIP_NS_RESP_MAX_ADDL 128

#define RIP_NS_CDNAME_COMP_BUF_LEN 256 /**< size of buffer used to compress CDNAME. */


/** DNS message header flags. */
typedef enum rip_ns_flag_e {
    rip_ns_f_qr,        /**< Question/Response. */
    rip_ns_f_opcode,    /**< Operation code. */
    rip_ns_f_aa,        /**< Authoritative Answer. */
    rip_ns_f_tc,        /**< Truncation occurred. */
    rip_ns_f_rd,        /**< Recursion Desired. */
    rip_ns_f_ra,        /**< Recursion Available. */
    rip_ns_f_z,         /**< MBZ. */
    rip_ns_f_ad,        /**< Authentic Data (DNSSEC). */
    rip_ns_f_cd,        /**< Checking Disabled (DNSSEC). */
    rip_ns_f_rcode,     /**< Response code. */
    rip_ns_f_max
} rip_ns_flag_t;

/** DNS opcodes. */
typedef enum rip_ns_opcode_e {
    rip_ns_o_query  = 0,    /**< Standard query. */
    rip_ns_o_iquery = 1,    /**< Inverse query (deprecated/unsupported). */
    rip_ns_o_status = 2,    /**< Name server status query (unsupported). */
                            /* Opcode 3 is undefined/reserved. */
    rip_ns_o_notify = 4,    /**< Zone change notification. */
    rip_ns_o_update = 5,    /**< Zone update message. */
    rip_ns_o_max    = 6
} rip_ns_opcode_t;

/** DNS response codes. */
typedef enum rip_ns_rcode_e {
    rip_ns_r_noerror  = 0,  /*< No error occurred. */
    rip_ns_r_formerr  = 1,  /**< Format error. */
    rip_ns_r_servfail = 2,  /**< Server failure. */
    rip_ns_r_nxdomain = 3,  /**< Name error. */
    rip_ns_r_notimpl  = 4,  /**< Unimplemented. */
    rip_ns_r_refused  = 5,  /**< Operation refused. */
    /* these are for BIND_UPDATE */
    rip_ns_r_yxdomain = 6,  /**< Name exists */
    rip_ns_r_yxrrset  = 7,  /**< RRset exists */
    rip_ns_r_nxrrset  = 8,  /**< RRset does not exist */
    rip_ns_r_notauth  = 9,  /**< Not authoritative for zone */
    rip_ns_r_notzone  = 10, /**< Zone of record different from zone section */
    rip_ns_r_max      = 11,
    /* The following are EDNS extended rcodes */
    rip_ns_r_badvers  = 16,
    /* The following are TSIG errors */
    rip_ns_r_badsig   = 16,
    rip_ns_r_badkey   = 17,
    rip_ns_r_badtime  = 18,

    /* Custom codes used for internal processing. */
    rip_ns_r_rip_unknown     = -1,
    rip_ns_r_rip_shortheader = -2,
    rip_ns_r_rip_toolarge    = -3,
    rip_ns_r_rip_query_tc    = -4,
    rip_ns_r_rip_pack_rr_err = -5,
    rip_ns_r_rip_tcp_write_err = -6,   /**< TCP connection write error, query response not sent */
    rip_ns_r_rip_tcp_write_close = -7, /**< TCP connection closed for write, query response not sent */
} rip_ns_rcode_t;

/** Currently defined type values for DNS resources and queries. */
typedef enum rip_ns_type_e {
    rip_ns_t_invalid = 0,
    rip_ns_t_a = 1,
    rip_ns_t_ns = 2,
    rip_ns_t_md = 3,
    rip_ns_t_mf = 4,
    rip_ns_t_cname = 5,
    rip_ns_t_soa = 6,
    rip_ns_t_mb = 7,
    rip_ns_t_mg = 8,
    rip_ns_t_mr = 9,
    rip_ns_t_null = 10,
    rip_ns_t_wks = 11,
    rip_ns_t_ptr = 12,
    rip_ns_t_hinfo = 13,
    rip_ns_t_minfo = 14,
    rip_ns_t_mx = 15,
    rip_ns_t_txt = 16,
    rip_ns_t_rp = 17,
    rip_ns_t_afsdb = 18,
    rip_ns_t_x25 = 19,
    rip_ns_t_isdn = 20,
    rip_ns_t_rt = 21,
    rip_ns_t_nsap = 22,
    rip_ns_t_nsap_ptr = 23,
    rip_ns_t_sig = 24,
    rip_ns_t_key = 25,
    rip_ns_t_px = 26,
    rip_ns_t_gpos = 27,
    rip_ns_t_aaaa = 28,
    rip_ns_t_loc = 29,
    rip_ns_t_nxt = 30,
    rip_ns_t_eid = 31,
    rip_ns_t_nimloc = 32,
    rip_ns_t_srv = 33,
    rip_ns_t_atma = 34,
    rip_ns_t_naptr = 35,
    rip_ns_t_kx = 36,
    rip_ns_t_cert = 37,
    rip_ns_t_a6 = 38,
    rip_ns_t_dname = 39,
    rip_ns_t_sink = 40,
    rip_ns_t_opt = 41,
    rip_ns_t_apl = 42,
    rip_ns_t_ds = 43,
    rip_ns_t_sshfp = 44,
    rip_ns_t_ipseckey = 45,
    rip_ns_t_rrsig = 46,
    rip_ns_t_nsec = 47,
    rip_ns_t_dnskey = 48,
    rip_ns_t_dhcid = 49,
    rip_ns_t_nsec3 = 50,
    rip_ns_t_nsec3param = 51,
    rip_ns_t_tlsa = 52,
    rip_ns_t_smimea = 53,
    rip_ns_t_hip = 55,
    rip_ns_t_ninfo = 56,
    rip_ns_t_rkey = 57,
    rip_ns_t_talink = 58,
    rip_ns_t_cds = 59,
    rip_ns_t_cdnskey = 60,
    rip_ns_t_openpgpkey = 61,
    rip_ns_t_csync = 62,
    rip_ns_t_spf = 99,
    rip_ns_t_uinfo = 100,
    rip_ns_t_uid = 101,
    rip_ns_t_gid = 102,
    rip_ns_t_unspec = 103,
    rip_ns_t_nid = 104,
    rip_ns_t_l32 = 105,
    rip_ns_t_l64 = 106,
    rip_ns_t_lp = 107,
    rip_ns_t_eui48 = 108,
    rip_ns_t_eui64 = 109,
    rip_ns_t_tkey = 249,
    rip_ns_t_tsig = 250,
    rip_ns_t_ixfr = 251,
    rip_ns_t_axfr = 252,
    rip_ns_t_mailb = 253,
    rip_ns_t_maila = 254,
    rip_ns_t_any = 255,
    rip_ns_t_uri = 256,
    rip_ns_t_caa = 257,
    rip_ns_t_avc = 258,
    rip_ns_t_ta = 32768,
    rip_ns_t_dlv = 32769,

    rip_ns_t_max = 65536
} rip_ns_type_t;

/** Values for DNS resource record class field */
typedef enum rip_ns_class_e {
    rip_ns_c_invalid = 0,   /**< Cookie. */
    rip_ns_c_in      = 1,   /**< Internet. */
    rip_ns_c_2       = 2,   /**< unallocated/unsupported. */
    rip_ns_c_chaos   = 3,   /**< MIT Chaos-net. */
    rip_ns_c_hs      = 4,   /**< MIT Hesiod. */
    /* Query class values which do not appear in resource records */
    rip_ns_c_none    = 254, /**< for prereq. sections in update requests */
    rip_ns_c_any     = 255, /**< Wildcard match. */
    rip_ns_c_max     = 65536
} rip_ns_class_t;

/** EDNS extension option codes */
typedef enum rip_ns_ext_opt_code_e {
    rip_ns_ext_opt_c_cs = 8,
} rip_ns_ext_opt_code_t;

/** Structure for DNS query header.  The order of the fields is machine- and
 * compiler-dependent, depending on the byte/bit order and the layout
 * of bit fields. We use bit fields only in int variables, as this
 * is all ANSI requires.
 */
typedef struct rip_ns_header_s {
    unsigned        id :16;         /**< query identification number */
#if __BYTE_ORDER == __BIG_ENDIAN
    /* fields in third byte */
    unsigned        qr: 1;          /**< response flag */
    unsigned        opcode: 4;      /**< purpose of message */
    unsigned        aa: 1;          /**< authoritative answer */
    unsigned        tc: 1;          /**< truncated message */
    unsigned        rd: 1;          /**< recursion desired */
                    /* fields in fourth byte */
    unsigned        ra: 1;          /**< recursion available */
    unsigned        unused :1;      /**< unused bits (MBZ as of 4.9.3a3) */
    unsigned        ad: 1;          /**< authentic data from named */
    unsigned        cd: 1;          /**< checking disabled by resolver */
    unsigned        rcode :4;       /**< response code */
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN || __BYTE_ORDER == __PDP_ENDIAN
    /* fields in third byte */
    unsigned        rd :1;          /**< recursion desired */
    unsigned        tc :1;          /**< truncated message */
    unsigned        aa :1;          /**< authoritative answer */
    unsigned        opcode :4;      /**< purpose of message */
    unsigned        qr :1;          /**< response flag */
    /* fields in fourth byte */
    unsigned        rcode :4;       /**< response code */
    unsigned        cd: 1;          /**< checking disabled by resolver , removed in RFC 1035 */
    unsigned        ad: 1;          /**< authentic data from named , removed in RFC 1035 */
    unsigned        unused :1;      /**< unused bits (MBZ as of 4.9.3a3) */
    unsigned        ra :1;          /**< recursion available */
#endif
    /* remaining bytes */
    unsigned        qdcount :16;    /**< number of question entries */
    unsigned        ancount :16;    /**< number of answer entries */
    unsigned        nscount :16;    /**< number of authority entries */
    unsigned        arcount :16;    /**< number of resource entries */
} rip_ns_header_t;

/** Macro to set a query code and return from function.
 * This is useful when handling a query and we want to: set query end code
 * and stop any further processing.
 */
#define RIP_NS_QUERY_SET_END_CODE_AND_RETURN(q, c) do { \
    (q)->end_code = c; \
    return; \
} while (0)

/** Get a network order unsigned 16 bit integer from buffer "cp" and store it in
 * "s" in host order. "cp" pointer is advanced by 2 bytes.
 */
#define RIP_NS_GET16(s, cp) do { \
    const unsigned char *t_cp = (const unsigned char *)(cp); \
    (s) = ((uint16_t)t_cp[0] << 8) \
        | ((uint16_t)t_cp[1]) \
        ; \
    (cp) += RIP_NS_INT16SZ; \
} while (0)

/** Pack an unsigned 16 bit integer "s" in host order into buffer "cp" in
 * network order. "cp" pointer is advanced by 2 bytes.
 */
#define RIP_NS_PUT16(s, cp) do { \
    uint16_t t_s = (uint16_t)(s); \
    unsigned char *t_cp = (unsigned char *)(cp); \
    *t_cp++ = t_s >> 8; \
    *t_cp   = t_s; \
    (cp) += RIP_NS_INT16SZ; \
} while (0)

/** Get a network order unsigned 32 bit integer from buffer "cp" and store it in
 * "l" in host order. "cp" pointer is advanced by 4 bytes.
 */
#define RIP_NS_GET32(l, cp) do { \
    const unsigned char *t_cp = (const unsigned char *)(cp); \
    (l) = ((uint32_t)t_cp[0] << 24) \
        | ((uint32_t)t_cp[1] << 16) \
        | ((uint32_t)t_cp[2] << 8) \
        | ((uint32_t)t_cp[3]) \
        ; \
    (cp) += RIP_NS_INT32SZ; \
} while (0)

/** Pack an unsigned 32 bit integer "l" in host order into buffer "cp" in
 * network order. "cp" pointer is advanced by 4 bytes.
 */
#define RIP_NS_PUT32(l, cp) do { \
    uint32_t t_l = (uint32_t)(l); \
    unsigned char *t_cp = (unsigned char *)(cp); \
    *t_cp++ = t_l >> 24; \
    *t_cp++ = t_l >> 16; \
    *t_cp++ = t_l >> 8; \
    *t_cp   = t_l; \
    (cp) += RIP_NS_INT32SZ; \
} while (0)

const char * rip_ns_class_to_str(rip_ns_class_t class);
const char * rip_ns_rr_type_to_str(rip_ns_type_t type);

bool rip_ns_rr_type_supported(uint16_t query_type);

bool rip_ns_rr_class_supported(uint16_t query_class);

void rip_ns_put16(unsigned char *dst, uint16_t num);

int rip_ns_name_unpack(const unsigned char *msg, const unsigned char *eom,
                       const unsigned char *src, unsigned char *dst,
                       size_t dstsiz);

int rip_ns_name_pack(const unsigned char *src, unsigned char *dst, int dstsiz,
                     const unsigned char **dnptrs,
                     const unsigned char **lastdnptr);

int rip_ns_name_pton(const unsigned char *src, unsigned char *dst, size_t dstsiz);
int rip_ns_name_ntop(const unsigned char *src, char *dst, size_t dstsiz);

int rip_rr_name_get(const unsigned char *msg, const unsigned char *eom,
                       const unsigned char *src, unsigned char *dst,
                       size_t dstsiz, uint16_t *query_label_len);

int rip_ns_name_put(const unsigned char *src, unsigned char *dst, int dstsiz,
                     const unsigned char **dnptrs,
                     const unsigned char **lastdnptr);

#endif /* End of CONNS_UTILS_HN_H */

/** @}*/