/**
 * @file rip_ns_utils.c
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
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "rip_ns_utils.h"

/** Array mapping Resource Record types to enumerated rip_ns_type_e. 
 * Array is used for fast conversion from enum to string.
 */
static const char *rip_ns_rr_type_array[] = {
    "invalid",
    "A",
    "NS",
    "MD",
    "MF",
    "CNAME",
    "SOA",
    "MB",
    "MG",
    "MR",
    "NULL",
    "WKS",
    "PTR",
    "HINFO",
    "MINFO",
    "MX",
    "TXT",
    "RP",
    "AFSDB",
    "X25",
    "ISDN",
    "RT",
    "NSAP",
    "NSAP_PRT",
    "SIG",
    "KEY",
    "PX",
    "GPOS",
    "AAAA",
    "LOC",
    "NTXT",
    "EID",
    "NIMLOC",
    "SRV",
    "ATMA",
    "NAPTR",
    "KX",
    "CERT",
    "A6",
    "DNAME",
    "SINK",
    "OPT",
};

/** Convert Resource Record enum type to string.
 * 
 * @param type RR type to get string representation for.
 * 
 * @return     Returns pointer to string representation of RR type.
 */
const char *
rip_ns_rr_type_to_str(rip_ns_type_t type)
{
    if (type <= rip_ns_t_opt) {
        return rip_ns_rr_type_array[type];
    }
    return "unknown";
}

/** Convert Resource Record enum class to string.
 * 
 * @param class RR class to get string representation for.
 * 
 * @return      Returns pointer to string representation of RR class.
 */
const char *
rip_ns_class_to_str(rip_ns_class_t class)
{
    switch (class) {
    case rip_ns_c_in:
        return "IN";
    case rip_ns_c_any:
        return "ANY";
    default:
        return "invalid";
    }
    return "invalid";
}


/** Check if DNS resource record type is one of supported types.
 * 
 * @param query_type DNS resource record type to check.
 * @return           Returns true if type is one of supported DNS resource
 *                   record types, otherwise returns false.
 */
bool
rip_ns_rr_type_supported(uint16_t query_type)
{
    uint16_t supported_query_types[] = {
        rip_ns_t_a,
    };
    uint16_t count = sizeof(supported_query_types)/sizeof(supported_query_types[0]);
    for (int i = 0; i < count; i++) {
        if (query_type == supported_query_types[i]) {
            return true;
        }
    }
    return false;
}

/** Check if DNS resource record class is one of supported types.
 * 
 * @note Currently only class of IN (Internet) is supported.
 * 
 * @param query_class DNS resource record type to check.
 *
 * @return           Returns true if type is one of supported DNS resource
 *                   record types, otherwise returns false.
 */
bool
rip_ns_rr_class_supported(uint16_t query_class)
{
    if (query_class == rip_ns_c_in) {
        return true;
    }
    return false;
}


/** Pack an unsigned 16 bit integer "num" in host order into buffer "dst" in
 * network order.
 * 
 * @param dst Destination where to store packed unsigned 16 bit integer.
 * @param num Number to pack.
 */
inline void
rip_ns_put16(unsigned char *dst, uint16_t num)
{
    *dst++ = num >> 8;
    *dst   = num;
};

/** Packs domain name SRC into DST. Domain name MUST be a C string.
 * This function converts the C string into "network" format, then packs it
 * into destination buffer.
 * 
 * DNPTRS is an array of pointers to previous compressed names.
 * DNPTRS[0] is a pointer to the beginning of the message. The array
 * ends with NULL. LASTDNPTR is a pointer to the end of the array
 * pointed to by 'dnptrs'.
 *
 * The list of pointers in DNPTRS is updated for labels inserted into
 * the message as we compress the name. If DNPTRS is NULL, we don't
 * try to compress names. If LASTDNPTR is NULL, we don't update the
 * list.
 * 
 * DNPTRS should be initialized when ready to populate DNS response.
 * It should then be used for all @ref rip_ns_name_put() and @ref
 * rip_ns_name_pack() calls while adding RRs to DNS response.
 *
 * @param src       Domain name in "network" format to pack.
 * @param dst       Destination buffer where to pack the domain name.
 * @param dstsiz    Length of destination buffer.
 * @param dnptrs
 * @param lastdnptr 
 *
 * @return          Returns size of the compressed name, or -1.
 */
int rip_ns_name_put(const unsigned char *src, unsigned char *dst, int dstsiz,
                    const unsigned char **dnptrs,
                    const unsigned char **lastdnptr)
{
    unsigned char compressed_name[RIP_NS_CDNAME_COMP_BUF_LEN];
    int           ret_p                 = 0;
    int           ret_c                 = 0;

    ret_c = rip_ns_name_pton(src, compressed_name, RIP_NS_CDNAME_COMP_BUF_LEN);

    if (ret_c < 0) {
        return ret_c;
    }

    ret_p = rip_ns_name_pack(compressed_name, dst, dstsiz, dnptrs, lastdnptr);

    return ret_p;
}

/** Get a domain name from a message Resource Record (RR), source may be
 * compressed. Unpacked name will be in stored in "dst" as to regular C string.
 * 
 * @note msg passed in MUST be a continuous buffer.
 *
 * @param msg             Pointer to first byte of DNS message.
 * @param eom             Pointer to last byte DNS in message.
 * @param src             Pointer to first byte of RR name.
 * @param dst             Pointer to buffer where to store the unpacked RR name.
 * @param dstsiz          Size of destination buffer.
 * @param query_label_len Pointer where to store length of label extracted and
 *                        stored in dst.
 * 
 * @return       Returns -1 if it fails, or consumed octets if it succeeds.
 */
int
rip_rr_name_get(const unsigned char *msg, const unsigned char *eom,
                const unsigned char *src, unsigned char *dst, size_t dstsiz,
                uint16_t *query_label_len)
{
    unsigned char compressed_name[256];
    int           ret_u                 = 0;
    int           ret_c                 = 0;

    ret_u = rip_ns_name_unpack(msg, eom, src, compressed_name, 256);

    if (ret_u < 0) {
        return ret_u;
    }

    ret_c = rip_ns_name_ntop(compressed_name, (char *)dst, dstsiz);

    if (ret_c < 0) {
        return ret_c;
    }
    *query_label_len = ret_c;

    return ret_u;
}


/** Unpack a domain name from a message Resource Record (RR), source may be
 * compressed. Unpacked name will be in "network" format. Use @ref 
 * rip_ns_name_ntop() function to convert from "network" to regular C string.
 * 
 * @note msg passed in MUST be a continuous buffer.
 *
 * @param msg    Pointer to first byte of DNS message.
 * @param eom    Pointer to last byte DNS in message.
 * @param src    Pointer to first byte of RR name.
 * @param dst    Pointer to buffer where to store the unpacked RR name.
 * @param dstsiz Size of destination buffer.
 * 
 * @return       Returns -1 if it fails, or consumed octets if it succeeds.
 */
int
rip_ns_name_unpack(const unsigned char *msg, const unsigned char *eom,
                   const unsigned char *src, unsigned char *dst, size_t dstsiz)
{
    const unsigned char *srcp, *dstlim;
    unsigned char       *dstp;
    int                 n, len, checked;

    len = -1;
    checked = 0;
    dstp = dst;
    srcp = src;
    dstlim = dst + dstsiz;
    if (srcp < msg || srcp >= eom) {
        /* EMSGSIZE */
        return -1;
    }
    /* Fetch next label in domain name.  */
    while ((n = *srcp++) != 0) {
        /* Check for indirection.  */
        switch (n & RIP_NS_CMPRSFLGS) {
        case 0:
            /* Limit checks.  */
            if (n >= 64) {
                /* EMSGSIZE */
                return -1;
            }
            /* NB: n + 1 and >= to cover the *dstp = '\0' assignment below.  */
            if (n + 1 >= dstlim - dstp || n >= eom - srcp) {
                /* EMSGSIZE */
                return -1;
            }
            checked += n + 1;
            *dstp++ = n;
            memcpy (dstp, srcp, n);
            dstp += n;
            srcp += n;
            break;

        case RIP_NS_CMPRSFLGS:
            if (srcp >= eom) {
                /* EMSGSIZE */
                return -1;
            }
            if (len < 0) {
                len = srcp - src + 1;
            }
            {
                int target = ((n & 0x3f) << 8) | *srcp;
                if (target >= eom - msg) {
                    /* Out of range. EMSGSIZE */
                    return -1;
                }
                srcp = msg + target;
            }
            checked += 2;
            /* Check for loops in the compressed name; if we've looked at the
             * whole message, there must be a loop.
             */
            if (checked >= eom - msg) {
                /* EMSGSIZE */
                return -1;
            }
            break;

        default:
            /* EMSGSIZE */
            return -1;
        }
    }
    *dstp = '\0';
    if (len < 0) { 
        len = srcp - src;
    }
    return len;
}

/** Search for the counted-label name in an array of compressed names.
 *
 * DNPTRS is the pointer to the first name on the list, not the
 * pointer to the start of the message.
 *
 * @note This is a helper function for @ref rip_ns_name_pack().
 * 
 * @param domain    Name to search for.
 * @param msg       DNS message to search name in.
 * @param dnptrs    See @ref rip_ns_name_put().
 * @param lastdnptr See @ref rip_ns_name_put().
 * 
 * @return          Returns the offset from MSG if found, or -1.
 */
static int
rip_dn_find(const unsigned char *domain, const unsigned char *msg,
            const unsigned char **dnptrs, const unsigned char **lastdnptr)
{
    const unsigned char *dn, *cp, *sp;
    const unsigned char **cpp;
    unsigned int        n;

    for (cpp = dnptrs; cpp < lastdnptr; cpp++) {
        sp = *cpp;
        /* Terminate search on: root label, compression pointer, unusable
         * offset.
         */
        while (*sp != 0 && (*sp & RIP_NS_CMPRSFLGS) == 0 && (sp - msg) < 0x4000) {
            dn = domain;
            cp = sp;
            while ((n = *cp++) != 0) {
                /* Check for indirection.  */
                switch (n & RIP_NS_CMPRSFLGS) {
                case 0: /* Normal case, n == len.  */
                    if (n != *dn++) {
                        goto next;
                    }
                    for (; n > 0; n--) { 
                        if (*dn++ != *cp++) { 
                            goto next;
                        }
                    }
                    /* Is next root for both?  */
                    if (*dn == '\0' && *cp == '\0') { 
                        return sp - msg;
                    }
                    if (*dn) { 
                        continue;
                    }
                    goto next;

                case RIP_NS_CMPRSFLGS:
                    /* Indirection.  */
                    cp = msg + (((n & 0x3f) << 8) | *cp);
                    break;

                default:
                    /* Illegal type. EMSGSIZE */
                    return -1;
                }
            }
            next: ;
            sp += *sp + 1;
        }
    }
    /* EMSGSIZE */
    return -1;
}

/** Converts an ASCII string into an encoded domain name as per
 * RFC1035. Enforces label and domain length limits.
 *
 * @param src    String to encode.
 * @param dst    Destination buffer to store encoded string.
 * @param dstsiz Size of destination buffer.
 *
 * @return       Returns -1 if it fails, 1 if string was fully qualified,
 *               0 is string was not fully qualified.
 */
int
rip_ns_name_pton(const unsigned char *src, unsigned char *dst, size_t dstsiz)
{
    unsigned char *label, *bp, *eom;
    int           c, n, escaped;

    escaped = 0;
    bp      = dst;
    eom     = dst + dstsiz;
    label   = bp++;

    while ((c = *src++) != 0) {
        if (escaped) {
            if ('0' <= c && c <= '9') {
                n = (c - '0') * 100;
                if ((c = *src++) == 0 || c < '0' || c > '9') {
                    /* EMSGSIZE */
                    return -1;
                }
                n += (c - '0') * 10;
                if ((c = *src++) == 0 || c < '0' || c > '9') {
                    /* EMSGSIZE */
                    return -1;
                }
                n += c - '0';
                if (n > 255) {
                    /* EMSGSIZE */
                    return -1;
                }
                c = n;
            }
            escaped = 0;
        } else if (c == '\\') {
            escaped = 1;
            continue;
        } else if (c == '.') {
            c = (bp - label - 1);
            if ((c & RIP_NS_CMPRSFLGS) != 0) {
                /* Label too big. EMSGSIZE */
                return -1;
            }
            if (label >= eom) {
                /* EMSGSIZE */
                return -1;
            }
            *label = c;
            /* Fully qualified ? */
            if (*src == '\0') {
                if (c != 0) {
                    if (bp >= eom) {
                        /* EMSGSIZE */
                        return -1;
                    }
                    *bp++ = '\0';
                }
                if ((bp - dst) > RIP_NS_MAXCDNAME)  {
                        /* EMSGSIZE */
                    return -1;
                }
                return 1;
            }
            if (c == 0 || *src == '.') {
                /* EMSGSIZE */
                return -1;
            }
            label = bp++;
            continue;
        }
        if (bp >= eom) {
            /* EMSGSIZE */
            return -1;
        }
        *bp++ = (unsigned char) c;
    }
    if (escaped) {
        /* Trailing backslash. EMSGSIZE */
        return -1;
    }
    c = (bp - label - 1);
    if ((c & RIP_NS_CMPRSFLGS) != 0) {
        /* Label too big. EMSGSIZE */
        return -1;
    }
    if (label >= eom) {
        /* EMSGSIZE */
        return -1;
    }
    *label = c;
    if (c != 0) {
        if (bp >= eom) {
            /* EMSGSIZE */
            return -1;
        }
        *bp++ = 0;
    }
    if ((bp - dst) > RIP_NS_MAXCDNAME) {
        /* src too big. EMSGSIZE */
        return -1;
    }
    return 0;
}

/** Packs domain name SRC into DST. Domain name MUST be in "network" format.
 * To convert C string to "network" format use @ref rip_ns_name_pton().
 *
 * DNPTRS is an array of pointers to previous compressed names.
 * DNPTRS[0] is a pointer to the beginning of the message. The array
 * ends with NULL. LASTDNPTR is a pointer to the end of the array
 * pointed to by 'dnptrs'.
 *
 * The list of pointers in DNPTRS is updated for labels inserted into
 * the message as we compress the name. If DNPTRS is NULL, we don't
 * try to compress names. If LASTDNPTR is NULL, we don't update the
 * list.
 *
 * @param src       Domain name in "network" format to pack.
 * @param dst       Destination buffer where to pack the domain name.
 * @param dstsiz    Length of destination buffer.
 * @param dnptrs
 * @param lastdnptr 
 *
 * @return          Returns size of the compressed name, or -1.
 */
int
rip_ns_name_pack(const unsigned char *src, unsigned char *dst, int dstsiz,
                 const unsigned char **dnptrs, const unsigned char **lastdnptr)
{
    unsigned char       *dstp;
    const unsigned char **cpp, **lpp, *eob, *msg;
    const unsigned char *srcp;
    int                 n, l, first = 1;

    srcp = src;
    dstp = dst;
    eob  = dstp + dstsiz;
    lpp  = cpp = NULL;
    if (dnptrs != NULL) {
        if ((msg = *dnptrs++) != NULL) {
            for (cpp = dnptrs; *cpp != NULL; cpp++)
            ;
            lpp = cpp; /* End of list to search. */
        }
    } else {
        msg = NULL;
    }

    /* Make sure the domain we are about to add is legal. */
    l = 0;
    do {
        n = *srcp;
        if (n >= 64) {
        /* EMSGSIZE */
        return -1;
        }
        l += n + 1;
        if (l > RIP_NS_MAXCDNAME) {
            /* EMSGSIZE */
            return -1;
        }
        srcp += n + 1;
    }
    while (n != 0);

    /* from here on we need to reset compression pointer array on error */
    srcp = src;
    do {
        /* Look to see if we can use pointers. */
        n = *srcp;
        if (n != 0 && msg != NULL) {
            l = rip_dn_find (srcp, msg, dnptrs, lpp);
            if (l >= 0) {
                if (eob - dstp <= 1) {
                    goto cleanup;
                }
                *dstp++ = (l >> 8) | RIP_NS_CMPRSFLGS;
                *dstp++ = l % 256;
                return dstp - dst;
            }
            /* Not found, save it.  */
            if (lastdnptr != NULL && cpp < lastdnptr - 1
                && (dstp - msg) < 0x4000 && first) {
                *cpp++ = dstp;
                *cpp = NULL;
                first = 0;
            }
        }
        /* Copy label to buffer.  */
        if (n >= 64) {
            /* Should not happen.  */
            goto cleanup;
        }
        if (n + 1 > eob - dstp) {
            goto cleanup;
        }
        memcpy (dstp, srcp, n + 1);
        srcp += n + 1;
        dstp += n + 1;
    }
    while (n != 0);

    if (dstp > eob) {
    cleanup:
        if (msg != NULL) {
            *lpp = NULL;
        }
        /* EMSGSIZE */
        return -1;
    }
    return dstp - dst;
}


/** Thinking in non-internationalized US-ASCII (per the DNS spec), is
 * this character special ("in need of quoting")?
 *
 * @note This is a helper function for @ref rip_ns_name_ntop()
 *
 * @param ch Character to check.
 *
 * @return   Returns true if character needs quoting, otherwise returns false.
 */
static inline bool
special(int ch)
{
    switch (ch)
    {
    case '"':
    case '.':
    case ';':
    case '\\':
    case '(':
    case ')':
        /* Special modifiers in zone files.  */
    case '@':
    case '$':
        return true;
    default:
        return false;
    }
}

/** Thinking in non-internationalized US-ASCII (per the DNS spec), is
 * this character visible and not a space when printed?
 * 
 * @note This is a helper function for @ref rip_ns_name_ntop()
 * 
 * @param ch Character to check.
 *
 * @return   Returns true if character is visible when printed,
 *           otherwise returns false.
 */
static inline bool
printable(int ch)
{
    return ch > 0x20 && ch < 0x7f;
}

/** Converts an uncompressed, encoded domain name to printable ASCII as
 * per RFC1035.  Returns the number of bytes written to buffer, or -1
 * (with errno set). The root is returned as "."  All other domains
 * are returned in non absolute form. 
 * 
 * @param src    Encoded domain name to convert to printable ASCII.
 * @param dst    Destination buffer where to store converted string.
 * @param dstsiz Size of destination buffer.
 *
 * @return       On success returns length of string in destination buffer,
 *               otherwise returns -1 indicating and error occurred.
 */
int
rip_ns_name_ntop (const unsigned char *src, char *dst, size_t dstsiz)
{
    const unsigned char *cp;
    char                *dn, *eom;
    unsigned char        c;
    int                  l;

    cp = src;
    dn = dst;
    eom = dst + dstsiz;

    while ((l = *cp++) != 0) {
        if (l >= 64) {
            /* Some kind of compression pointer. EMSGSIZE */
            return -1;
        }
        if (dn != dst) {
            if (dn >= eom) {
                /* EMSGSIZE */
                return -1;
            }
            *dn++ = '.';
        }
        for (; l > 0; l--) {
            c = *cp++;
            if (special (c)) {
                if (eom - dn < 2)  {
                    /* EMSGSIZE */
                    return -1;
                }
                *dn++ = '\\';
                *dn++ = c;
            } else if (!printable (c)) {
                if (eom - dn < 4) {
                    /* EMSGSIZE */
                    return -1;
                }
                *dn++ = '\\';
                *dn++ = '0' + (c / 100);
                *dn++ = '0' + ((c % 100) / 10);
                *dn++ = '0' + (c % 10);
            } else  {
                if (eom - dn < 2) {
                    /* EMSGSIZE */
                    return -1;
                }
                *dn++ = c;
            }
        }
    }
    if (dn == dst)  {
        if (dn >= eom) {
            /* EMSGSIZE */
            return -1;
        }
        *dn++ = '.';
    }
    if (dn >= eom) {
        /* EMSGSIZE */
        return -1;
    }
    *dn++ = '\0';
    return dn - dst - 1;
}