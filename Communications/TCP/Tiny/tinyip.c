/*
 * tinyip.c - Tiny Implementation of the Transmission Control Protocol
 *
 * Written March 28, 1986 by Geoffrey Cooper, IMAGEN Corporation.
 *
 * This code is a small implementation of the TCP and IP protocols, suitable
 * for burning into ROM.  The implementation is bare-bones and represents
 * two days' coding efforts.  A timer and an ethernet board are assumed.  The
 * implementation is based on busy-waiting, but the tcp_handler procedure
 * could easily be integrated into an interrupt driven scheme.
 *
 * IP routing is accomplished on active opens by broadcasting the tcp SYN
 * packet when ARP mapping fails.  If anyone answers, the ethernet address
 * used is saved for future use.  This also allows IP routing on incoming
 * connections.
 *
 * The TCP does not implement urgent pointers (easy to add), and discards
 * segments that are received out of order.  It ignores the received window
 * and always offers a fixed window size on input (i.e., it is not flow
 * controlled).
 *
 * Special care is taken to access the ethernet buffers only in word
 * mode.  This is to support boards that only allow word accesses.
 *
 * Copyright (C) 1986, IMAGEN Corporation
 *  "This code may be duplicated in whole or in part provided that [1] there
 *   is no commercial gain involved in the duplication, and [2] that this
 *   copyright notice is preserved on all copies.  Any other duplication
 *   requires written notice of the author."
 */

/*
 * TCP, UDP and ICMP dispatching are built in.  To generalize,
 * we need a mechanism for registering new clients with virtual operations.
 */

#include "Headers.h"
#include "MemoryManager.h"

#include "defs.h"
#include "ip.h"
#include "tinyip.h"
#include "tinytcp.h"
#include "tinyudp.h"
#include "ppp.h"

#ifdef FOR_MAC
	#ifndef __MACINTOSHUTILITIES_H__
	#include "MacintoshUtilities.h"	/* for pseudo-malloc */
	#endif
#endif


/* -----------------------  debugging ---------------------- */

#ifdef TCP_LOGGING
static void
print_ip_addr(const char *msg, ip_addr addr) {
	LogPrintF("%s%lu.%lu.%lu.%lu",
		msg, (addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff
	);
}
#endif

/* ----------------------  IP utilities -------------------- */

static ulong
lcsum(ushort *buf, ulong nbytes) {
	ulong    sum;
	ulong	nwords;

	nwords = nbytes >> 1;
	sum = 0;
	while (nwords-- > 0)
		sum += *buf++;
	if (nbytes & 1)
		sum += *buf & 0xff00;
	return sum;
}

static ushort
eac(ulong sum) {
	ushort csum;

	while((csum = sum >> 16) !=0 )
		sum = csum + (sum & 0xffff);
	return (ushort)(sum & 0xffff);
}

static ushort
cksum(ushort *buf, ulong nbytes) {
	ushort result = ~eac(lcsum(buf, nbytes));
	return result;
}

ushort
checksum(ushort *buf, ulong len) {
	ushort result = eac(lcsum(buf, len));
	return result;
}

/* the following code assumes IP headers of minimum size */

#ifdef SIMULATOR
static iphdr_t *
fill_ip(ip_State *s, uchar *n_buf, int len, ip_addr dest_addr, uchar proto) {
	iphdr_t  *ip = (iphdr_t *)n_buf;
	
	ip->ip_v_hl  = IP_V_HLEN;
	ip->ip_tos   = 0;
	ip->ip_len   = len + IP_SIZE;
	ip->ip_id    = s->ip_id++;
	ip->ip_off   = IP_DF;
	ip->ip_ttl   = 16;
	ip->ip_p     = proto;
	ip->ip_sum   = 0;
	ip->ip_src   = gOurIPAddr;
	ip->ip_dst   = dest_addr;

	ip->ip_sum   = cksum((ushort *)ip, IP_SIZE);
	return ip;
}
#endif

/* the following code uses computed header sizes */

static iphdr_t *
chk_ip(uchar *n_buf, long len) {
	long    hl, ip_len;
	iphdr_t  *ip = (iphdr_t *)n_buf;

	if (len < IP_SIZE || ip->ip_v_hl>>4 != IP_VERSION)
		return NULL;
	hl = IP_HLEN(ip);
	if (hl < IP_SIZE || hl > len)
		return NULL;
	if (cksum((ushort *)ip, hl) != 0)
		return NULL;
	ip_len = ip->ip_len;
	if (ip_len < hl || ip_len > len)
		return NULL;
	return ip;
}

/* ----------------------  ICMP messages -------------------- */

/* temporary */
#define icmp_id  icmp_uh.ih_idseq.icd_id
#define icmp_seq icmp_uh.ih_idseq.icd_seq

#define ICMP_MASKREQ    17       /* address mask request */
#define ICMP_MASKREPLY  18       /* address mask reply */

#ifdef SIMULATOR
static void
icmp_request(ip_State *ip, uchar type) {
	struct _pkt {
		iphdr_t in;
		icmp  ic;
		ulong mask;
	}   *pkt;
	long len = ICMP_SIZE + sizeof(ulong);

	pkt = (struct _pkt *)AllocateMemory(sizeof(struct _pkt));
	if (pkt) {
		pkt->ic.icmp_type = type;
		pkt->ic.icmp_code = 0;
		pkt->ic.icmp_id = 0;   /* for now */
		pkt->ic.icmp_seq = 0;  /* ditto */
		pkt->mask = 0;

		pkt->ic.icmp_cksum = cksum((ushort *)&pkt->ic, len);

		fill_ip(ip, (uchar *)pkt, len, 0xffffffff, IPT_ICMP);
		ipSend((uchar *)pkt, IP_SIZE+len);
		FreeMemory((char *)pkt);
	}
}
#endif
	
static void
icmp_echo(iphdr_t *ip) {
	int     len = ip->ip_len;
	icmp   *ic = (icmp *)((uchar *)ip + IP_HLEN(ip));

	ic->icmp_type = ICMP_ECHOREPLY;
	ic->icmp_cksum = cksum((ushort *)ic, len - IP_HLEN(ip));
	ip->ip_dst = ip->ip_src;
	ip->ip_src = gOurIPAddr;
	WritePDU(kIPProtocol, (uchar *)ip, len);
}

static void
icmp_in(ip_State *s, iphdr_t *ip, long len) {
	icmp   *ic = (icmp *)((uchar *)ip + IP_HLEN(ip));
	
	if (cksum((ushort *)ic, len-IP_HLEN(ip)) != 0) {
		printf("icmp_in: check sum error, len %d", len);
		return;
	}
	switch (ic->icmp_type) {
		case ICMP_ECHO:
			icmp_echo(ip);
			break;
		case ICMP_UNREACH:
		case ICMP_SOURCEQUENCH:
		case ICMP_TIMXCEED:
		case ICMP_PARAMPROB: {
				iphdr_t  *pep = (iphdr_t *)((uchar *)ic + ICMP_SIZE);

#ifdef TCP_LOGGING
				const char   *msg;
				print_ip_addr("ICMP: ", pep->ip_dst);
				switch (ic->icmp_type) {
					case ICMP_UNREACH:       msg = "unreachable";    break;
					case ICMP_SOURCEQUENCH:  msg = "source quench";  break;
					case ICMP_TIMXCEED:      msg = "time exceeded";  break;
					case ICMP_PARAMPROB:     msg = "param problem";  break;
					default:                 msg = "other";          break;
				};
				LogPrintF(" %s (%d)", msg, ic->icmp_code);
				print_ip_addr(" per ", ip->ip_src);
				LogPrintF("%c", '\n');
#endif
				switch (pep->ip_p) {
					case IPT_TCP:
						tcpNotify((tcp_PCB *)s->ip_tcp, ip, pep);
						break;
					case IPT_UDP:
						udpNotify((udp_State)s->ip_udp, ip, pep);
						break;
				}
			}
			break;
		case ICMP_MASKREPLY:
			/* fall through */
		default:
#ifdef TCP_LOGGING
			LogPrintF(
				"icmp_in: type %d, code %d, len %d",
				ic->icmp_type, ic->icmp_code, len
			);
			if (ic->icmp_type == ICMP_REDIRECT)
				print_ip_addr(" gateway ", ic->icmp_gwaddr);
			print_ip_addr(" per ", ip->ip_src);
			LogPrintF("%c", '\n');
#endif
			break;
	};
}

/* ----------------- link-level inquiries ------------------ */

void SetIpAddress(ip_State *ip, ip_addr myAddr)
{
	ip->addr = myAddr;
}

ip_addr
ipAddress(ip_State *ip, ip_addr dest) { /* no routing capability for now */
	#pragma unused(dest)
	return ip->addr;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long
ipMRU(ip_State *ip, ip_addr dest) { /* no routing capability for now */
	#pragma unused(dest, ip)
	
	return MaxReadPDUSize() - IP_SIZE;
/*	return 576 - IP_SIZE;   /* IP default */
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long
ipMTU(ip_State *ip, ip_addr dest) { /* no routing capability for now */
	#pragma unused(dest, ip)
	
	return MaxWritePDUSize() - IP_SIZE;
/*	return 576 - IP_SIZE;  /* IP default */
}
#endif

/* --------------- network-level input/output --------------- */

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void *
ipTransport(ip_State *ip, uchar proto) {
	switch (proto) {
		case IPT_TCP:
			return ip->ip_tcp;
		case IPT_UDP:
			return ip->ip_udp;
		default:
			return 0;
	}
}
#endif

void
ipIn(ip_State *s, uchar *n_buf, long len) {
	iphdr_t    *ip;

	ip = chk_ip(n_buf, len);
	if (ip) {
		switch (ip->ip_p) {
			case IPT_ICMP:
				icmp_in(s, ip, len);
				break;
			case IPT_TCP:
				tcpHandler((tcp_PCB *)s->ip_tcp, ip);
				break;
			case IPT_UDP:
				udpHandler((udp_State)s->ip_udp, ip);
				break;
			default:
				printf("unknown ip packet type\n");
				break;
		}
	}
}

long
ipSend(uchar *buf, long length) {
	WritePDU(kIPProtocol, buf, length);
	return 0;
}

/* ----------------------- initialization ----------------------- */

void
ipMonitor(ip_State *ip) {
	if (ip->ip_tcp)
		tcpPoll((tcp_PCB *)ip->ip_tcp);
}

ip_State *
ipInit(void) {
	ip_State *ip = (ip_State *)AllocateMemory(sizeof(ip_State));
	
	if (ip) {
//#ifdef DEBUG
#if 1
		ip->ip_id = 1;
#else
		ip->ip_id = (Now()/1000) & 0xffff;
#endif

		ip->ip_udp = udpInit(ip, 1);
		ip->ip_tcp = tcpInit(ip, 99);
	}
	return ip;
}

#ifdef INCLUDE_FINALIZE_CODE
void
ipFinalize(ip_State *ip) {
	FreeMemory((char *)ip);
}
#endif