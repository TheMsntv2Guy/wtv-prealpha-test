/* -*-mode:C; tab-width:4-*- */
#ifndef TINYUDP_H
#define TINYUDP_H
/*
 * tinyudp.h - header file for Tiny Implementation of the User Datagram Protocol
 *
 * Copyright (C) 1986, IMAGEN Corporation
 *  "This code may be duplicated in whole or in part provided that [1] there
 *   is no commercial gain involved in the duplication, and [2] that this
 *   copyright notice is preserved on all copies.  Any other duplication
 *   requires written notice of the author."
 */


#include "ip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _udp_state  *udp_State;

typedef struct _udp_socket *udp_Socket;

typedef void *    uplink;

typedef long (*udp_upcall)(udp_Socket, uplink, uchar *, long);

struct _udp_socket {
	udp_Socket    next;
	ushort         myport;           /* local port (matches any IP address) */
	void          *link;             /* environment for dataHandler */
	udp_upcall    dataHandler;       /* called with incoming datagram */
	Boolean       chksum;        	 /* compute and send udp checksum */
	udp_State     udp;
};

struct _udp_state {
	udp_Socket udp_allsocs;
	ushort      udp_id;        /* IP identification number */
	ushort      next_port;     /* rover for free UDP port numbers */
	ushort      loglevel;
	ulong       timeorg;
	ip_State   *ip;
};


/*
 * UDP Public Interface
 */

/* client calls */
udp_State udpInit(ip_State *ip, int log);

udp_Socket udpOpen(udp_State udp, ushort lport, udp_upcall dataHandler, Boolean chksum);
void udpClose(udp_Socket s);

void udpAbort(udp_Socket s);
int  udpWrite(udp_Socket s, ip_addr dest, uchar *dp, long len);

/* service upcalls */
void udpHandler(udp_State udp, iphdr *ip);
void udpNotify(udp_State udp, iphdr *ip, iphdr *mip);


#ifdef __cplusplus
}
#endif
#endif
