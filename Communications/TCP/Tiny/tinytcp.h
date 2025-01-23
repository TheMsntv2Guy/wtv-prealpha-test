/* -*-mode:C; tab-width:4-*- */
#ifndef _TINYTCP_
#define _TINYTCP_
/* ===========================================================================
 *	tinytcp.h
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

/*
 * tinytcp.h - header file for tinytcp.c
 *
 * Copyright (C) 1986, IMAGEN Corporation
 *  "This code may be duplicated in whole or in part provided that [1] there
 *   is no commercial gain involved in the duplication, and [2] that this
 *   copyright notice is preserved on all copies.  Any other duplication
 *   requires written notice of the author."
 *
 * Note: the structures herein must guarantee that the
 *       code only performs word fetches, since the
 *       imagenether card doesn't accept byte accesses.
 */


#include "tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NIL         0                /* The distinguished empty pointer */

/*
 * TCP states, from tcp spec (rfc793), codes match tcp mib
 */
#define tcp_CLOSED   1     /* idle */
#define tcp_LISTEN   2     /* listening for connection */
#define tcp_SYNSENT  3     /* SYN sent, active open */
#define tcp_SYNREC   4     /* SYN received, SYN+ACK sent. */
#define tcp_ESTAB    5     /* established */
#define tcp_FINWT1   6     /* FIN sent, active close */
#define tcp_FINWT2   7     /* FIN sent, ACK received, need FIN */
#define tcp_CLOSEWT  8     /* FIN received, waiting for close */
#define tcp_LASTACK  9     /* FIN received, FIN+ACK sent */
#define tcp_CLOSING 10     /* FIN sent, FIN received, need ACK */
#define tcp_TIMEWT  11     /* 2MSL wait, after active close */
#define tcp_DELETE  12     /* SNMP delete */

/*
 * TCP public types
 *  Note that tcp_Socket is not a socket in either the BSD or the rfc793
 *  sense but names a connection endpoint represented by a TCB
 */

typedef struct tcp_TCB *tcp_Socket;  /* pointer to Transport Control Block */

typedef enum {
	tcp_ok,
	tcp_aborted,
	tcp_refused,
	tcp_reset,
	tcp_timedout,
	tcp_unreachable
} tcp_Error;

typedef struct tcp_PCB {
	tcp_Socket  all_tcbs;           /* list of all control blocks (sockets) */
	ushort      ip_id;              /* counter for the ip id field */
	ulong       conn_id;            /* used to generate ISS */
	ushort      next_port;          /* port number rover */
	ulong       timeNext;           /* next poll time */
	ulong       timeOrg;
	ushort      logLevel;
	ip_State   *ip;                 /* server IP instance */
} tcp_PCB;

typedef struct tcp_PCB *tcp_State;   /* pointer to Protocol Control Block */

typedef void *    uplink;

typedef long (*upcall)(tcp_Socket, uplink, uchar *, long);

#define tcp_MSS      536             /* maximum segment size (always default) */
#define tcp_MaxData  (3*tcp_MSS)     /* maximum bytes to buffer on output */
//#define tcp_MaxData  (15*tcp_MSS)     /* maximum bytes to buffer on output */
//#define tcp_MaxData  (20*tcp_MSS)     /* maximum bytes to buffer on output */


/*
 * TCP Transmission Control Block (TCB) definition 
 */

struct tcp_TCB {
	tcp_State     tcp;             /* link back to shared TCP state */
	tcp_Socket    next;

	int           state;           /* connection state */
	ip_addr       lcl_addr;        /* local IP address for this connection */
	ip_addr       rem_addr;        /* foreign IP address */
	ushort        lcl_port;        /* local tcp port for this connection */
	ushort        rem_port;        /* foreign tcp port */
	ulong         snd_nxt;         /* tx sequence num (SND.NXT) */
	ulong         snd_una;         /* tx ack point (SND.UNA) */
	ushort        snd_wnd;         /* tx window, from peer (SND.WND) */
	ushort        snd_mss;         /* mss of peer */
	ulong         rcv_nxt;         /* rx ack point (RCV.NXT) */
	ushort        rcv_wnd;         /* rx window, to peer (RCV.WND) */
	ushort		  rcv_space;	   /* space available in socket buffer */
	ushort		  rcv_buf;	   	   /* size of our socket buffer */
	ushort        ctl;             /* tcp control bits for next packet sent */
	Boolean		  wnd_update;	   /* need window update to peer */
	Boolean       pending;         /* flag, work pending after upcall */
	Boolean       close;           /* flag, close pending */
	Boolean       poll;            /* flag, timer running */
	Boolean		  del_ack;		   /* ack pending */
	tcp_Error     error;           /* error code after failure */
	long          timeout;         /* timeout, in milliseconds */
	int           retries;         /* number of retries with no ack */
	upcall        handler;         /* called for data  or control indication */
	uplink        link;            /* environment for dataHandler */
	ulong         dataSize;        /* number of buffered bytes of data to send */
	ulong         dataOrigin;      /* start of valid data in output buffer */
	char		*data;            /* handle for (circular) output buffer */

	char		*rcv_chain;       /* seg_Buffers for out of order input */
};

#define kMSSSlop 8

/*
 * TCP Public Interface
 */

/* client calls */
tcp_State tcpInit(void *ip, int log);

tcp_Socket tcpOpen(tcp_State tcp, ushort lport, ip_addr ina, ushort port, upcall handler, uplink self, int credits);
tcp_Socket tcpListen(tcp_State tcp, ushort port, upcall handler, uplink env, int credits, long timeout);

long tcpSend(tcp_Socket s, uchar *dp, long len);
void tcpFlush(tcp_Socket s);
void tcpCredit(tcp_Socket s, int credits);
long tcpSendSpaceAvail(tcp_Socket s);
tcp_Error tcpStatus(tcp_Socket s);

void tcpClose(tcp_Socket s);
void tcpAbort(tcp_Socket s);       /* XXX unsafe in upcalls XXX */

/* service upcalls (from IP) */
void tcpHandler(tcp_PCB *tcp, iphdr *ip);
void tcpNotify(tcp_PCB *tcp, iphdr *ip, iphdr *mip);
void tcpPoll(tcp_PCB *tcp);

#ifdef __cplusplus
}
#endif
#endif
