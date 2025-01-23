/* -*-mode:C; tab-width:4-*- */
/*
 * tinyudp.c - Tiny Implementation of the User Datagram Protocol
 *
 * Copyright (C) 1986, IMAGEN Corporation
 *  "This code may be duplicated in whole or in part provided that [1] there
 *   is no commercial gain involved in the duplication, and [2] that this
 *   copyright notice is preserved on all copies.  Any other duplication
 *   requires written notice of the author."
 */


/* ===========================================================================
 *	TinyUDP.c
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#include "Headers.h"

#include "MemoryManager.h"
#include "Network.h"
#include "Socket.h"
#include "tinyip.h"
#include "tinyudp.h"
#include "Utilities.h"

#ifdef FOR_MAC
	#ifndef __MACINTOSHUTILITIES_H__
	#include "MacintoshUtilities.h"	/* for pseudo-malloc */
	#endif
#endif

/* ----------------------- debugging ----------------------- */

#ifdef DEBUG
/* 
 * Primitive logging facility
 */

#define log_msg(udp,level,msg) if (udp->loglevel >= level) Log(("%s", msg))

/*
 * Dump the UDP protocol header of a packet
 */
static void
udp_DumpHeader(udp_State udp, iphdr *ip, udphdr *up, const char *msg) {
	ulong  t = Now() - udp->timeorg;
	long len = ip->ip_len - (UDP_SIZE + IP_HLEN(ip));

	Log(("*%3lu.%03lu UDP: %s packet:", t/1000, t%1000, msg));
	Log(("  S: %d; D: %d; ID=%u C=%x Len=%d DLen=%ld",
			up->uh_sport, up->uh_dport, ip->ip_id, up->uh_sum, up->uh_ulen, len));
}
#else
#define log_msg(udp,level,msg)
#define udp_DumpHeader(udp,ip,up,msg)
#endif

/* -------------------- internal procs --------------------- */

/*
 * UDP port number management
 */

static Boolean
udp_InUse(udp_State udp, ushort port) {
	udp_Socket s;

	for (s = udp->udp_allsocs; s; s = s->next)
		if (s->myport == port)
			return true;
	return (port == 0);
}

static ushort
udp_NewPort(udp_State udp) {
	ushort   port, start;

	start = udp->next_port;
	do {
		port = udp->next_port;
		udp->next_port++;
		if (udp->next_port == 0) udp->next_port = 1024;
		if (!udp_InUse(udp, port))
			return port;
	} while (port != start);
	return 0;
}

#if 0
static ushort
udp_DataSum() {
	UDPPDUInfo *p;
	ulong       offset, sum;

	offset = sum = 0;
	for (p = params; p; p = p->next) {
	  	uchar  *ptr;
		ulong   len;
		ushort  cksum;
		
		ptr = p->bufPtr;  len = p->length;
		if (len != 0) {
			if ((ulong)ptr & 0x1) {
				ushort w = *ptr++;
				if ((offset & 0x1) == 0) w <<= 8;
				sum += w;
				len--;
				offset++;
			}
			cksum = checksum((ushort *)ptr, len);
			if (offset & 0x1) cksum = ((cksum & 0xff) << 8) | (cksum >> 8);
			sum += cksum;
			offset += len;
		}
	}
	sum = (sum & 0xffff) + (sum >> 16);
	sum += (sum >> 16);
	return sum;
}
#endif

static ushort
udp_Checksum(iphdr *ip, long len, ushort extra) {
	struct {  /* TCP/UDP pseudo header */
		ip_addr     src;
		ip_addr     dst;
		uchar        mbz;
		uchar        protocol;
		ushort      length;
		ushort      checksum;
		ushort      pad;            /* align to 32 bits */
	} ph;
	long hlen = IP_HLEN(ip);

	ph.src = ip->ip_src;
	ph.dst = ip->ip_dst;
	ph.mbz = 0;
	ph.protocol = ip->ip_p;
	ph.length = ip->ip_len - hlen;
	ph.checksum = checksum((ushort *)((uchar *)ip + hlen), len - hlen);
	if (extra != 0) {
		ulong sum = (ulong)ph.checksum + (ulong)extra;
		sum = (sum >> 16) + (sum & 0xffff);  /* no carry out */
		ph.checksum = sum;
	}
	return ~checksum((ushort *)&ph, 12+2);
}

/*
 * Unthread a socket from the socket list, if it's there 
 */
static void
udp_Unthread(udp_State udp, udp_Socket ds) {
	udp_Socket s, *sp;

	sp = &udp->udp_allsocs;
	for (;;) {
		s = *sp;
		if (s == ds) {
			*sp = s->next;
			break;
		}
		if (s == nil) break;
		sp = &s->next;
	}
}

/* ------------------- public interface -------------------- */

/*
 * Initialize the UDP implementation
 */
udp_State
udpInit(ip_State *ip, int log_opt) {
	udp_State udp = (udp_State)AllocateMemory(sizeof(struct _udp_state));

	if (udp) {
		udp->udp_allsocs = nil;
		udp->udp_id = 0;
		udp->next_port = 1024;
#ifdef DEBUG
		udp->loglevel = log_opt;
		udp->timeorg = Now();
#else
#pragma unused(log_opt)
#endif /* DEBUG */
		udp->ip = ip;
	}
	return udp;
}

#define kUdpSocketTag "UDP Socket"

/*
 * Initialize a UDP socket.
 */
udp_Socket
udpOpen(udp_State udp, ushort lport, udp_upcall dataHandler, Boolean chksum) {
	udp_Socket s;
	ushort  port = (lport != 0) ? lport : udp_NewPort(udp);

	s = nil;
	if (!udp_InUse(udp, port)) {
		s = (udp_Socket)AllocateTaggedMemory(sizeof(struct _udp_socket), kUdpSocketTag);
		if (s) {
			s->myport = port;
			s->dataHandler = dataHandler;
			s->chksum = chksum;
			s->next = udp->udp_allsocs;  udp->udp_allsocs = s;
			s->udp = udp;
		}
	}
	return s;
}

static void
udp_Free(udp_Socket s)
{
	FreeTaggedMemory((char *)s, kUdpSocketTag);
}

/*
 * Release a UDP socket.
 */
void
udpClose(udp_Socket s) {
	udp_Unthread(s->udp, s);
	udp_Free(s);
}

void udpAbort(udp_Socket s) {
	udp_Unthread(s->udp, s);
	udp_Free(s);
}

#if 0
/*
 * Write datagram to a socket.
 * Returns error code (0 for success).
 */
int
udpWrite(udp_Socket *s, ip_addr dest, ushort destPort, uchar *dp, long len) {
	struct _pkt {
		iphdr  in;
		udphdr udp;
	} pkt;
	udp_State udp = s->udp;

	pkt.udp.uh_sport = s->myport;
	pkt.udp.uh_dport = destPort;
	pkt.udp.uh_ulen = UDP_SIZE + len;
	pkt.udp.uh_sum = 0;
	
	/* internet header */
	pkt.in.ip_v_hl = IP_V_HLEN;
	pkt.in.ip_tos = 0;
	pkt.in.ip_len = IP_SIZE + pkt.udp.uh_ulen;
	pkt.in.ip_id = udp->udp_id++;
	pkt.in.ip_off = 0;
	pkt.in.ip_ttl = 64;
	pkt.in.ip_p = IPT_UDP;
	pkt.in.ip_sum = 0;
	pkt.in.ip_src = ipAddress(udp->ip, dest);
	pkt.in.ip_dst = dest;
	pkt.in.ip_sum = ~checksum((ushort *)&pkt.in, sizeof(iphdr));
	
	info.next = params;
	info.bufPtr = (uchar *)&pkt;  info.length = sizeof(struct _pkt);

	if (s->chksum) {
		ushort extra = udp_DataSum(dp);
		pkt.udp.uh_sum = udp_Checksum(&pkt.in, sizeof(struct _pkt), extra);
		if (pkt.udp.uh_sum == 0) pkt.udp.uh_sum = 0xffff;
	}
	
#ifdef DEBUG
	if (udp->loglevel) udp_DumpHeader(udp, &pkt.in, &pkt.udp, "Sending");
#endif
	ipSend(udp->ip, &info);
   	return 0;
}
#endif 

/*
 * Handler for incoming packets.
 */
void
udpHandler(udp_State udp, iphdr *ip) {
	udphdr *up;
	long  len;
	udp_Socket s;
	
	len = IP_HLEN(ip);
	up = (udphdr *)((uchar *)ip + len);
	len = ip->ip_len - len;

	if (len < UDP_SIZE || len < up->uh_ulen) {
		log_msg(udp, 1, "bad udp length, discarding");
		return;
	}
	len = up->uh_ulen - UDP_SIZE;
	if (up->uh_sum != 0 && udp_Checksum(ip, ip->ip_len, 0) != 0) {
		Message(("bad udp checksum, discarding"));
		log_msg(udp, 1, "bad udp checksum, discarding");
		return;
	}
	
	/* demux */
	for (s = udp->udp_allsocs; s; s = s->next)
		if (up->uh_dport == s->myport)
			break;

	if (s) {
#ifdef DEBUG	
		if (udp->loglevel) udp_DumpHeader(udp, ip, up, "Received");
#endif
		(*s->dataHandler)(s, s->link, (uchar *)up+UDP_SIZE, len);
	} else {
#ifdef DEBUG	
		if (udp->loglevel) udp_DumpHeader(udp, ip, up, "Discarding");
#endif
#if 0
		icmp_Err(ICMP_UNREACH, ICMP_UNREACH_PORT, ip, ip->ip_len);
#endif
	}
}

/*
 * Handler for ICMP messages.
 */
void
udpNotify(udp_State udp, iphdr *ip, iphdr *mip) {
	icmp       *imp;
	udphdr     *mup;
	udp_Socket s;

	imp = (icmp *)((uchar *)ip + IP_HLEN(ip));
	mup = (udphdr *)((uchar *)mip + IP_HLEN(mip));

	/* demux */
	for (s = udp->udp_allsocs; s; s = s->next)
		if (mup->uh_dport == s->myport)
			break;
	if (s) {
		switch (imp->icmp_type) {
			case ICMP_SOURCEQUENCH:
			case ICMP_UNREACH:
				/* notify client, passing src ip and port */
				break;
		}
	}
}

