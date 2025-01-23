#ifndef IP_H
#define IP_H
/* ===========================================================================
 *	ip.h
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------- IP defns  --------------------- */

typedef unsigned long  ip_addr;
 
typedef struct iphdr {
	uchar    ip_v_hl;         /* version (4 bits) and length (4 bits) */
	uchar    ip_tos;
	ushort   ip_len;
	ushort   ip_id;
	ushort   ip_off;          /* flags (3 bits) and fragment offset (13 bits) */
	uchar    ip_ttl;
	uchar    ip_p;
	ushort   ip_sum;
	ip_addr  ip_src;
	ip_addr  ip_dst;
} iphdr_t;
#define IP_SIZE  sizeof(iphdr)
#define IP_HLEN(ip)  (((ip)->ip_v_hl & 0x0f)<<2)

#define IP_VERSION  4
#define IP_MINHLEN  5         /* minimum header length 5*4 = 20 bytes */
#define IP_V_HLEN   0x45      /* v.4, 5*4 = 20 bytes */

#define IP_DF       0x4000    /* don't fragment */
#define IP_MF       0x2000    /* more fragments */
#define IP_OFFMASK  0x1fff    /* mask for fragment offset */

/* assigned numbers for protocols */
#define IPT_ICMP   1   /* ICMP protocol type */
#define IPT_TCP    6   /* TCP protocol type */
#define IPT_UDP   17   /* UDP protocol type */

/* ------------------------ ICMP defns  -------------------- */

/* subset only */

typedef struct icmp {
	uchar    icmp_type;
	uchar    icmp_code;
	ushort   icmp_cksum;
	union {
		struct {ushort icd_id, icd_seq;}  ih_idseq;
		ip_addr                            ih_gwaddr;
		struct {uchar  ic_ptr, ic_pad[3];} ih_pptr;
		ulong                              ih_void;
	}        icmp_uh;
} icmp;
#define ICMP_SIZE sizeof(icmp)
#define icmp_gwaddr icmp_uh.ih_gwaddr

#define ICMP_ECHOREPLY      0
#define ICMP_UNREACH        3
#define      ICMP_UNREACH_NET        0
#define      ICMP_UNREACH_HOST       1
#define      ICMP_UNREACH_PROTOCOL   2
#define      ICMP_UNREACH_PORT       3
#define      ICMP_UNREACH_NEEDFRAG   4
#define      ICMP_UNREACH_SRCFAIL    5
#define ICMP_SOURCEQUENCH   4
#define ICMP_REDIRECT       5
#define ICMP_ECHO           8
#define ICMP_TIMXCEED      11
#define ICMP_PARAMPROB     12

/* ------------------------- UDP defns --------------------- */

typedef struct udphdr {
	ushort   uh_sport;
	ushort   uh_dport;
	ushort   uh_ulen;
	ushort   uh_sum;
} udphdr;
#define UDP_SIZE sizeof(udphdr)

/* ----------------------- byte ordering ------------------- */

/* big-endian assumed for now */
#ifdef ANDY_LITTLE_ENDIAN
#define htons(x) (x)
#define htonl(x) (x)
#define ntohs(x) (x)
#define ntohl(x) (x)
#endif

#ifdef __cplusplus
}
#endif
#endif
