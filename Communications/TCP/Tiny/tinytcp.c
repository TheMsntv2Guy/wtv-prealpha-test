/*
 * tinytcp.c - Tiny Implementation of the Transmission Control Protocol
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


/* ===========================================================================
 *	TinyTCP.c
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#include "Headers.h"

#include "MemoryManager.h"

#include "defs.h"
#include "tinyip.h"
#include "tinytcp.h"

#include "Socket.h"

#include "Utilities.h"

#ifdef FOR_MAC
	#ifndef __MACINTOSHUTILITIES_H__
	#include "MacintoshUtilities.h"	/* for pseudo-malloc */
	#endif
#endif

/* ------------------------- state ------------------------- */

/*
 * The following variables encode the shared state of tcp.
 * In a preemptive implementation, access must be synchronized,
 * especially changes to, or traversal of. the all_tcbs list.
 */
 
#ifndef MILLISECONDS
	#define tcp_POLLTIME     15       /* interval at which retransmitter is called */
	#define tcp_CONNTIMEOUT 600       /* timeout for opens */
	#define tcp_TIMEOUT      180       /* initial timeout during a connection */
	#define tcp_MAXTIMEOUT  1800       /* maximum timeout during a connection */
	#define tcp_TIME2MSL     300       /* timeout in TIME WAIT (XXX too small XXX) */
#else
/* Timer definitions (all in milliseconds) */
	#define tcp_POLLTIME     1000       /* interval at which retransmitter is called */
	#define tcp_CONNTIMEOUT 10000       /* timeout for opens */
	#define tcp_TIMEOUT      3000       /* initial timeout during a connection */
	#define tcp_MAXTIMEOUT  30000       /* maximum timeout during a connection */
	#define tcp_TIME2MSL     5000       /* timeout in TIME WAIT (XXX too small XXX) */
#endif

#define tcp_MAXRETRIES      4       /* maximum send attempts */

/* ----------------- local prototypes ----------------- */

static Boolean		tcp_InUse(tcp_State tcp, ushort port, ulong addr, ushort hisport, ulong hisaddr);
static ushort		tcp_NewPort(tcp_State tcp, ulong addr, ushort hisport, ulong hisaddr);
static ushort		tcp_Checksum(iphdr *ip);
static void			tcp_Reset(tcp_State tcp, iphdr *rip, tcphdr *rtp);
static void			tcp_Transmit(tcp_Socket s);
static Boolean		tcp_SaveData(tcp_Socket s, ulong seq, uchar *dp, long len);
static void			tcp_DeliverData(tcp_Socket s);
static void			tcp_ProcessSegment(tcp_Socket s, tcphdr *tp, long len);
static tcp_Socket	tcp_Alloc(tcp_State tcp);
static void			tcp_Add(tcp_State tcp, tcp_Socket s);
static void			tcp_Drop(tcp_State tcp, tcp_Socket ds);
static void			tcp_SignalError(tcp_State tcp, tcp_Socket s, tcp_Error error);
static void			tcp_Timer(tcp_State tcp);

#ifdef UNUSED
static long			tcp_Write(tcp_Socket s, uchar *dp, long len);
#endif



/* ----------------- logging and debugging ----------------- */

#undef TCP_LOGGING

#ifdef TCP_LOGGING

static void			log_msg(tcp_State tcp, int level, const char *msg);
static void			log_packet(tcp_State tcp, iphdr *ip, tcphdr *tp, const char *msg );

/* 
 * Primitive logging facility
 */

static ushort logLevel;

#include <stdio.h>

static void
log_msg(tcp_State tcp, int level, const char *msg) {
	if (tcp->logLevel >= level) {
		printf(msg,'\n');
	}
}

/*
 * Display selected header fields of a packet
 */
static void
log_packet(tcp_State tcp, iphdr *ip, tcphdr *tp, const char *msg ) {
	if (tcp->logLevel != 0) {
		ulong  t = Now() - tcp->timeOrg;
		int len = ip->ip_len - (TCP_HLEN(tp) + IP_HLEN(ip));
		static const char *flags[] = {"FIN", "SYN", "RST", "PUSH", "ACK", "URG"};
		int i;
		ushort f;
	
		printf("*%3lu.%03lu TCP: %s packet:\n", t/1000, t%1000, msg);
		printf("  S: %d; D: %d; ID=%u Seq=%lu Ack=%lu DLen=%d\n",
			tp->th_sport, tp->th_dport, ip->ip_id, tp->th_seq, tp->th_ack, len);
		printf("  W=%d DO=%d, Urg=%d",
			tp->th_win, TCP_HLEN(tp), tp->th_urp);
		/* output flags */
		f = tp->th_off_flags;
		for (i = 0; i < 6; i++)
			if (f & (1 << i)) printf(" %s", flags[i]);
		printf("\n");
	}
}
#else
#define log_msg(tcp,level,msg)
#define log_packet(tcp,ip,tp,msg)
#endif

/* ------------------ buffer management -------------------- */
#define BUFFER_NIL         NIL
#define bufferLock(buf)    (buf)
#define bufferUnlock(buf)          /* generates no code */

/* -------------------- internal procs --------------------- */

/*
 * TCP port number management
 */

static Boolean
tcp_InUse(tcp_State tcp, ushort port, ulong addr, ushort hisport, ulong hisaddr) {
	tcp_Socket  s;

	for (s = tcp->all_tcbs; s; s = s->next)
		if (s->lcl_port == port && s->rem_port == hisport
        &&  s->rem_addr == hisaddr && s->lcl_addr == addr)
			return true;
	return (port == 0);
}

static ushort
tcp_NewPort(tcp_State tcp, ulong addr, ushort hisport, ulong hisaddr) {
	ushort   port, start;

	start = tcp->next_port;
	do {
		port = tcp->next_port;
		tcp->next_port++;
		if (tcp->next_port == 0) tcp->next_port = 1024;
		if (!tcp_InUse(tcp, port, addr, hisport, hisaddr))
			return port;
	} while (port != start);
	return 0;
}

/*
 * Compute the tcp checksum for a packet
 */
static ushort
tcp_Checksum(iphdr *ip) {
	struct {  /* TCP/UDP pseudo header */
		ip_addr     ph_src;
		ip_addr     ph_dst;
		uchar        ph_mbz;
		uchar        ph_p;
		ushort      ph_len;
		ushort      ph_sum;
		ushort      pad;            /* align to 32 bits */
	} ph;
	int len = IP_HLEN(ip);

	ph.ph_src = ip->ip_src;
	ph.ph_dst = ip->ip_dst;
	ph.ph_mbz = 0;
	ph.ph_p   = ip->ip_p;
	ph.ph_len = ip->ip_len - len;
	ph.ph_sum = checksum((ushort *)((uchar *)ip + len), ph.ph_len);
	return ~checksum((ushort *)&ph, 12+2);
}

/*
 * Format and send a reset segment in response to a received packet
 */
static void
tcp_Reset(tcp_State tcp, iphdr *rip, tcphdr *rtp) {
	iphdr  *ip;
	tcphdr *tp;

	ip = (iphdr *)AllocateMemory(sizeof(iphdr) + sizeof(tcphdr));
	if (ip) {

		/* tcp header */
		tp = (tcphdr *)((uchar *)ip + sizeof(iphdr));
		tp->th_sport = rtp->th_dport;
		tp->th_dport = rtp->th_sport;
		tp->th_off_flags = TH_RST | (5<<12);
		if (rtp->th_off_flags & TH_ACK)
			tp->th_seq = rtp->th_ack;
		else {
			tp->th_seq = 0;
			tp->th_off_flags |= TH_ACK;
		}
		tp->th_ack = rtp->th_seq + (rip->ip_len-IP_HLEN(rip)-TCP_HLEN(rtp));
		if (rtp->th_off_flags & TH_SYN) tp->th_ack++;
		if (rtp->th_off_flags & TH_FIN) tp->th_ack++;
		tp->th_win = 0;
		tp->th_sum = 0;
		tp->th_urp = 0;
	
		/* internet header */
		ip->ip_v_hl = IP_V_HLEN;
		ip->ip_tos = rip->ip_tos;
		ip->ip_len = sizeof(iphdr) + sizeof(tcphdr);
		ip->ip_id = tcp->ip_id++;
		ip->ip_off = 0;
		ip->ip_ttl = 64;
		ip->ip_p = IPT_TCP;
		ip->ip_sum = 0;
		ip->ip_src = rip->ip_dst;
		ip->ip_dst = rip->ip_src;
		ip->ip_sum = ~checksum((ushort *)ip, sizeof(iphdr));
	
		tp->th_sum = tcp_Checksum(ip);
	
		log_packet(tcp, ip, tp, "Sending");
		ipSend((uchar *)ip, ip->ip_len);
		FreeMemory((char *)ip);
	}
}

/*
 * Compose and send one or more outgoing packets
 */
static void
tcp_Transmit(tcp_Socket s) {
	iphdr  *ip;
	tcphdr *tp;
	ushort  mss, dataSize;
	ulong   len;
	ushort  ctl;
	Boolean    more;


	s->del_ack = false;
	
	tcp_State tcp = s->tcp;

	ctl = s->ctl;
	do {
		more = false;

		len = sizeof(iphdr) + sizeof(tcphdr);
		dataSize = 0;  mss = s->snd_mss;

		if (ctl & TH_SYN) {
			len += 4;  mss -= 4;
		}
		if (s->state == tcp_ESTAB || s->state == tcp_CLOSEWT) {
			dataSize = s->dataSize - (s->snd_nxt - s->snd_una);
			if (dataSize != 0) {
				if (dataSize > s->snd_wnd)
					dataSize = (s->snd_wnd !=0) ? s->snd_wnd : 1;
				if (dataSize > mss) dataSize = mss;
				len += dataSize;
			}
		}
		
		ip = (iphdr *)AllocateMemory(len);
		if (ip) {
			uchar  *dp;
			ulong  next;
		
			/* tcp header */
			tp = (tcphdr *)((uchar *)ip + sizeof(iphdr));
			tp->th_sport = s->lcl_port;
			tp->th_dport = s->rem_port;
			tp->th_seq = s->snd_nxt;
			tp->th_ack = s->rcv_nxt;
			tp->th_off_flags = (ctl & ~TH_FIN) | (5<<12);
			tp->th_win = s->rcv_wnd;  /* XXX add hysteresis to avoid sws XXX */
			tp->th_sum = 0;
			tp->th_urp = 0;
			dp = (uchar *)tp + sizeof(tcphdr);  /* word aligned here */
			if (ctl & TH_SYN) {
				tp->th_off_flags += (1<<12);
				*((ulong *)dp) = (TCPOPT_MAXSEG<<24)|(4<<16)|tcp_MSS;
				dp += 4;
				s->snd_nxt++;
				ctl &= ~TH_SYN;
				s->poll = true;
			}
			next = s->snd_nxt;
			if (dataSize != 0) {
				char  *data;
				ulong  offset;

				next += dataSize;
				data = bufferLock(s->data);
				offset = s->dataOrigin + (s->snd_nxt - s->snd_una);
				if (offset >= tcp_MaxData) offset -= tcp_MaxData;
				if (offset + dataSize >= tcp_MaxData) {
					ulong  n = tcp_MaxData - offset;
					memmove(dp, &data[offset], n);
					dp += n;  dataSize -= n;
					offset = 0;
				}
				memmove(dp, &data[offset], dataSize);
				bufferUnlock(s->data);

				if (s->snd_wnd != 0) {
				    s->snd_wnd -= dataSize;
					if (next == s->snd_una + s->dataSize)
						tp->th_off_flags |= TH_PUSH;
					else
						more = true;
				} else {
					if (next == s->snd_una + s->dataSize)
						tp->th_off_flags |= TH_PUSH;
				}
				s->snd_nxt = next;
				s->poll = true;
			}
			if ((ctl & TH_FIN) && next == s->snd_una + s->dataSize) {
				tp->th_off_flags |= TH_FIN;
				s->snd_nxt++;
				s->poll = true;
			}
		
			/* internet header (should move common part to IP) */
			ip->ip_v_hl = IP_V_HLEN;
			ip->ip_tos = 0;
			ip->ip_len = len;
			ip->ip_id = tcp->ip_id++;
			ip->ip_off = 0;
			ip->ip_ttl = 64;
			ip->ip_p = IPT_TCP;
			ip->ip_sum = 0;
			ip->ip_src = s->lcl_addr;
			ip->ip_dst = s->rem_addr;
			ip->ip_sum = ~checksum((ushort *)ip, sizeof(iphdr));
		
			tp->th_sum = tcp_Checksum(ip);
		
			log_packet(tcp, ip, tp, "Sending");
			ipSend((uchar *)ip, ip->ip_len);
			FreeMemory((char *)ip);
		}
	} while (more);
}


typedef struct _seg_buffer {
    char *next;            /* next segment in chain (sorted) */
	ulong         seg_seq;         /* seq number of initial byte */
	ulong         seg_len;         /* length of segment */
	/* buffered segment data follows */
} seg_Buffer;

/*
 * Save any data in an incoming packet that cannot be delivered immediately.
 * Does only simple checks for replicates or overlaps.
 * More complicated cases are resolved by tcp_DeliverData
 */
static Boolean
tcp_SaveData(tcp_Socket s, ulong seq, uchar *dp, long len) {
	Boolean        saved;
	char        *next, *prev, *buf;

	prev = BUFFER_NIL;  next = s->rcv_chain;
	while (next != BUFFER_NIL) {
		seg_Buffer *currSeg = (seg_Buffer *)bufferLock(next);
		ulong       currLen = currSeg->seg_len;
		long        diff = seq - currSeg->seg_seq;
		char        *temp = currSeg->next;

		bufferUnlock(next);
		if (diff == 0 && len <= currLen)
			return false;
		if (diff <= 0) break;
		prev = next;  next = temp;
	}

	saved = false;
	buf = (char *)AllocateMemory(sizeof(seg_Buffer) + len);
	if (buf != BUFFER_NIL) {
		seg_Buffer *seg = (seg_Buffer *)bufferLock(buf);
		seg->seg_seq = seq;
		seg->seg_len = len;
		seg->next = next;
		memmove((uchar *)seg + sizeof(seg_Buffer), dp, len);
		bufferUnlock(buf);
		if (prev == BUFFER_NIL)
			s->rcv_chain = buf;
		else {
			seg_Buffer  *prevSeg = (seg_Buffer *)bufferLock(prev);
			prevSeg->next = buf;
			bufferUnlock(prev);
		}
		saved = true;
	}
	return saved;
}

/*
 * Deliver any saved data that is now in sequence.
 */
static void
tcp_DeliverData(tcp_Socket s) {
	char *current, *next;

	current = s->rcv_chain;
	while (current != BUFFER_NIL) {
		seg_Buffer *currSeg = (seg_Buffer *)bufferLock(current);
		long        len = currSeg->seg_len;
		long        diff = s->rcv_nxt - currSeg->seg_seq;
		uchar       *dp;

		if (diff < 0) {
			bufferUnlock(current);
			break;
		}

		dp = (uchar *)currSeg + sizeof(seg_Buffer) + diff;
		len -= diff;
		if (len > s->rcv_wnd) len = s->rcv_wnd;
		if (len > 0) {
			s->rcv_nxt += len;
			s->rcv_wnd -= len;
			s->rcv_space -= len;
			s->rcv_wnd += s->handler(s, s->link, dp, len);
		}

		next = currSeg->next;
		bufferUnlock(current);
		FreeMemory(current);
		current = next;
	}
	s->rcv_chain = current;
}

/*
 * Process the data segment and FIN bit of an incoming packet,
 *    steps 6 (unimplemented), 7 and 8 of rfc793.
 * Called from all states where incoming data or FIN can be received:
 * synrec, established, fin-wait-1, fin-wait-2, close-wait
 */
static void
tcp_ProcessSegment(tcp_Socket s, tcphdr *tp, long len) {
	long overlap;
	long hlen;
	ushort flags;
	uchar *dp;
	long  dataSize;

	flags = tp->th_off_flags;
	overlap = s->rcv_nxt - tp->th_seq;
	if (flags & TH_SYN) overlap--;
	hlen = TCP_HLEN(tp);
	dp = (uchar *)tp + hlen;  len -= hlen;
//	if (len > 0 || (flags & (TH_SYN|TH_FIN))) s->pending = true;
	if (s->del_ack || (flags & (TH_SYN|TH_FIN)) || (len > 0 && overlap < 0)) {
		s->del_ack = false;
		s->pending = true;
	}
	else
		if (len > 0) s->del_ack = true;
			
	if (overlap >= 0) {
		dp += overlap;  len -= overlap;
		if (len > s->rcv_wnd) len = s->rcv_wnd;
		if (len > 0) {
			s->rcv_nxt += len;
			s->rcv_wnd -= len;
			s->rcv_space -= len;
			s->rcv_wnd += s->handler(s, s->link, dp, len);
			if (s->rcv_chain != BUFFER_NIL) tcp_DeliverData(s);
		}
		if ((flags & TH_FIN) && s->state != tcp_CLOSEWT) {
			s->rcv_nxt++;
			switch (s->state) {
			  case tcp_ESTAB:
				s->state = tcp_CLOSEWT;
				s->rcv_wnd += s->handler(s, s->link, NIL, 0);
				break;
			  case tcp_FINWT1:
				s->state = (s->snd_una==s->snd_nxt) ? tcp_TIMEWT : tcp_CLOSING;
				break;
			  case tcp_FINWT2:
				s->state = tcp_TIMEWT;
				break;
			}
			if (s->state == tcp_TIMEWT) {
				s->timeout = tcp_TIME2MSL;
				s->poll = true;
				(void)s->handler(s, s->link, NIL, 0);  /* release client */
			}
		}
	} else if (len != 0) {
		log_msg(s->tcp, 1, "Out of order segment");
		if ((flags & (TH_SYN | TH_FIN)) == 0)
			tcp_SaveData(s, tp->th_seq, dp, len);
	}
	if (s->close && s->dataSize == 0) {
		s->ctl = TH_ACK | TH_FIN;
		s->close = false;
		s->state = (s->state == tcp_CLOSEWT) ? tcp_LASTACK : tcp_FINWT1;
		s->pending = true;
	}
	dataSize = s->dataSize - (s->snd_nxt - s->snd_una);
	if (s->pending || (dataSize > 0 && s->snd_wnd != 0)) { /* XXX sws XXX */
		tcp_Transmit(s);
		s->pending = false;
	}
}

/*
 * Acquire the storage associated with socket and initialize common fields
 */
static tcp_Socket
tcp_Alloc(tcp_State tcp) {
	tcp_Socket  s;
	char        *buf;

	s = NIL;
	buf = (char *)AllocateMemory(tcp_MaxData);
	if (buf != BUFFER_NIL) {
		s = (tcp_Socket)AllocateMemory(sizeof(struct tcp_TCB));
		if (s) {
			s->tcp = tcp;
			s->state = tcp_CLOSED;
			s->error = tcp_ok;
			s->pending = s->close = false;
			s->poll = false;
			s->rcv_chain = BUFFER_NIL;
			s->data = buf;
			s->dataOrigin = 0;
			s->dataSize = 0;
		} else
			FreeMemory(buf);
	}
	return s;
}

/*
 * Add a TCB to the shared state (would be a critical section) 
 */
static void
tcp_Add(tcp_State tcp, tcp_Socket s) {
	s->next = tcp->all_tcbs;
	tcp->all_tcbs = s;
}

/*
 * Drop a TCB from the shared state (ditto)
 */
static void
tcp_Drop(tcp_State tcp, tcp_Socket ds) {
	tcp_Socket s, prev;

	prev = NIL;  s = tcp->all_tcbs;
	while (s) {
		if (s == ds) {
			if (prev == NIL)
				tcp->all_tcbs = s->next;
		  	else
				prev->next = s->next;
			break;
		}
		prev = s;  s = s->next;
	}
}

/*
 * Release the storage associated with socket
 */
void
tcp_Free(tcp_Socket s) {
	char *buf, *next;

	for (buf = s->rcv_chain; buf != BUFFER_NIL; buf = next) {
		seg_Buffer *seg = (seg_Buffer *)bufferLock(buf);
		next = seg->next;
		bufferUnlock(buf);
		FreeMemory(buf);
	}
	if (s->data != BUFFER_NIL) FreeMemory(s->data);
	if (s->link != nil) {
		((TinySocket*)s->link)->fSocket = nil;
		((TinySocket*)s->link)->fPhase = kDead;
	}
	FreeMemory((char *)s);
}

/*
 * Drop a connection and signal client after an error
 */
static void
tcp_SignalError(tcp_State tcp, tcp_Socket s, tcp_Error error) {
	s->poll = false;
	s->error = error;
	s->state = tcp_CLOSED;
	tcp_Drop(tcp, s);
	(void)s->handler(s, s->link, NIL, -1);
}

/* ------------------- TCP timer events -------------------- */

/*
 * Timer - called periodically to detect and process timer events
 */
static void
tcp_Timer(tcp_State tcp) {
	tcp_Socket s, next;

	for (s = tcp->all_tcbs; s; s = next) {
		next = s->next;
		if (s->poll) {
			s->timeout -= tcp_POLLTIME;
			if (s->timeout <= 0) {
				s->poll = false;
				if (s->state == tcp_TIMEWT) {
					log_msg(tcp, 1, "Closed");
					s->state = tcp_CLOSED;
					tcp_Drop(tcp, s);
					(void)s->handler(s, s->link, NIL, 0);  /* release client */
					tcp_Free(s);
				} else if (s->retries < tcp_MAXRETRIES) {
					Message(("TCP: Reseding packet, try #%d of %d", s->retries, tcp_MAXRETRIES));
					log_msg(tcp, 1, "Resend");
					s->retries++;
					s->snd_wnd += s->snd_nxt - s->snd_una;
					s->snd_nxt = s->snd_una;
					switch (s->state) {
					  case tcp_FINWT1:
					  case tcp_CLOSING:
					  case tcp_LASTACK:
						s->ctl |= TH_FIN;
						break;
					}
					tcp_Transmit(s);
					s->timeout = (tcp_TIMEOUT << s->retries);
					if (s->timeout > tcp_MAXTIMEOUT)
						s->timeout = tcp_MAXTIMEOUT;
					s->poll = true;
				} else {
					Message(("TCP: Timeout due to too many retransmits (%d)", tcp_MAXRETRIES));
					tcp_Error error;
					log_msg(tcp, 1, "Timeout");
					error = (s->error == tcp_ok) ? tcp_timedout : s->error;
					tcp_SignalError(tcp, s, error);
					tcp_Free(s);
				}
			} /* else consider sending waiting data or acks */
		}
	}
}

#ifdef UNUSED
/*
 * Write data to a connection.
 * Returns number of bytes written, == 0 when connection is not in
 * opening or established state.
 */
static long
tcp_Write(tcp_Socket s, uchar *dp, long len) {
	long x = tcp_MaxData - s->dataSize;
	Boolean queue;

	queue = true;
	switch (s->state) {
	  case tcp_ESTAB:
	  case tcp_CLOSEWT:
		queue = s->pending;
		/* fall through */
	  case tcp_SYNSENT:
	  case tcp_SYNREC:
		if (s->close) len = 0;
		if (len > x)  len = x;
		if (len > 0) {
			uchar  *data;
			ulong  offset, count;

			data = bufferLock((uchar *)s->data);
			count = len;
			offset = s->dataOrigin + s->dataSize;
			if (offset >= tcp_MaxData) offset -= tcp_MaxData;
			if (offset + count >= tcp_MaxData) {
				ulong  n = tcp_MaxData - offset;
				memmove(&data[offset], dp, n);
				s->dataSize += n;
				dp += n;  count -= n;
				offset = 0;
			}
			memmove(&data[offset], dp, count);
			s->dataSize += count;
			bufferUnlock(s->data);

			if (!queue && s->dataSize != 0 && s->snd_una == s->snd_nxt)
				tcp_Transmit(s);
		}
		break;
	  default:
		len = 0;
	}
	return len;
}
#endif

/* ------------------- public interface -------------------- */

/*
 * Initialize the tcp implementation and shared state
 */
tcp_State
tcpInit(void *ip, int log_opt) {
	tcp_State tcp = (tcp_State)AllocateMemory(sizeof(struct tcp_PCB));
	
	if (tcp) {
		tcp->all_tcbs = NIL;
		tcp->logLevel = log_opt;
		tcp->ip_id = 0;
		tcp->conn_id = 0;
		tcp->next_port = 1024 + (Now() % 8192);  /* randomize */
		tcp->timeOrg = Now();
		tcp->timeNext = tcp->timeOrg + tcp_POLLTIME;
		tcp->ip = (ip_State *)ip;
	}
	return tcp;
}

/*
 * Actively open a TCP connection to a particular destination.
 */
tcp_Socket tcpOpen(tcp_State tcp, ushort lport, ip_addr ina, ushort port, upcall handler, uplink self, int credits)
{
	tcp_Socket s;
	ip_addr myaddr = ipAddress(tcp->ip, ina);   /* IP selects the interface */
	ushort  myport = (lport != 0) ? lport : tcp_NewPort(tcp, myaddr, port, ina);

	s = NIL;
	if (!tcp_InUse(tcp, myport, myaddr, port, ina)) {
		s = tcp_Alloc(tcp);
		if (s) {
			s->lcl_addr = myaddr;
			s->lcl_port = myport;
			s->rem_addr = ina;
			s->rem_port = port;
			s->handler = handler;
			s->link = self;
			s->rcv_wnd = (ushort)credits;
			s->rcv_buf = (ushort)credits;
			s->rcv_space = (ushort)credits;
			s->rcv_nxt = 0xCAFEBABE;
			s->wnd_update = false;

#ifdef DEBUG
			s->snd_nxt = s->snd_una = 0;
#else
			s->snd_nxt = s->snd_una = Now()*256 + tcp->conn_id++;
#endif
			s->ctl = TH_SYN;
			s->snd_wnd = 0;
			s->snd_mss = tcp_MSS;
			tcp_Add(tcp, s);
			s->timeout = tcp_CONNTIMEOUT;
			s->retries = 0;
			s->state = tcp_SYNSENT;
			tcp_Transmit(s);
		}
	}
	return s;
}

/*
 * Passive open: listen for a connection on a particular port
 */
tcp_Socket
tcpListen(
	tcp_State tcp, ushort port, upcall handler, uplink env, int credits, long timeout
) {
	tcp_Socket s;

	s = tcp_Alloc(tcp);
	if (s) {
		s->lcl_port = port;
		s->rem_port = 0;
		s->handler = handler;
		s->link = env;
		s->ctl = 0;
		s->rcv_wnd = (ushort)credits;
		s->rcv_space = (ushort)credits;
		s->snd_wnd = 0;
		s->snd_mss = tcp_MSS;
		tcp_Add(tcp, s);
		s->timeout = timeout;
		s->poll = (timeout != 0);
		s->state = tcp_LISTEN;
	}
	return s;
}

/*
 * Send data over a connection.  (safe within an upcall)
 * Returns number of bytes written, == 0 when connection is not in
 * opening or established state.
 */
long
tcpSend(tcp_Socket s, uchar *dp, long len) {
	long max = tcp_MaxData - s->dataSize;
	Boolean queue;

	queue = true;
	switch (s->state) {
	  case tcp_ESTAB:
	  case tcp_CLOSEWT:
		queue = s->pending;
		/* fall through */
	  case tcp_SYNSENT:
	  case tcp_SYNREC:
		if (s->close) len = 0;
		if (len > max)  len = max;
		if (len > 0) {
			char  *data;
			ulong  offset, count;

			data = bufferLock(s->data);
			count = len;
			offset = s->dataOrigin + s->dataSize;
			if (offset >= tcp_MaxData) offset -= tcp_MaxData;
			if (offset + count >= tcp_MaxData) {
				ulong  n = tcp_MaxData - offset;
				memmove(&data[offset], dp, n);
				s->dataSize += n;
				dp += n;  count -= n;
				offset = 0;
			}
			memmove(&data[offset], dp, count);
			s->dataSize += count;
			bufferUnlock(s->data);

			/* use original Nagle algorithm (rfc896) */
			if (!queue && s->dataSize != 0 && s->snd_una == s->snd_nxt)
				tcp_Transmit(s);
		}
		break;
	  default:
		len = 0;
	}
	return len;
}

/*
 * Send any pending data.  (safe within an upcall)
 */
#ifdef SIMULATOR
void
tcpFlush(tcp_Socket s) {
	if (s->snd_nxt != s->snd_una + s->dataSize && s->snd_wnd != 0) {
		s->ctl |= TH_PUSH;
		tcp_Transmit(s);
	}
}
#endif

/*
 * Adjust credits from the client
 */
void
tcpCredit(tcp_Socket s, int credits) {
	ushort  wnd = s->rcv_wnd;
	
	s->rcv_space += credits;   /* XXX avoid SWS, conditionally force ack XXX */
	if (s->rcv_space > s->rcv_buf)
		s->rcv_space = s->rcv_buf;
	
	if ((s->rcv_space - wnd) >= s->snd_mss)
		{
			s->wnd_update = true;
			if ((s->rcv_space - wnd) < (s->rcv_buf / 2))
				s->rcv_wnd += s->snd_mss;
			else
				s->rcv_wnd = s->rcv_space - wnd;
		}
	
	if (s->wnd_update)
	{
		s->wnd_update = false;
		tcp_Transmit(s);
	}

}

/*
 * Return the buffer space available in the socket
 */
#ifdef SIMULATOR
long tcpSendSpaceAvail(tcp_Socket s)
{
	return (tcp_MaxData - s->dataSize);
}
#endif

/*
 * Return the error status of a connection.  (safe within an upcall)
 */
#ifdef SIMULATOR
tcp_Error
tcpStatus(tcp_Socket s) {
	return s->error;
}
#endif

/*
 * Close a tcp connection gracefully.  (safe within an upcall)
 */
#ifdef SIMULATOR
void
tcpClose(tcp_Socket s) {
	switch (s->state) {
	  case tcp_ESTAB:
	  case tcp_SYNREC:
	  case tcp_CLOSEWT:
		if (s->dataSize != 0)
			s->close = true;
		else {
			s->ctl = TH_ACK | TH_FIN;
			s->state = (s->state == tcp_CLOSEWT) ? tcp_LASTACK : tcp_FINWT1;
			if (!s->pending) tcp_Transmit(s);
		}
		break;
	  case tcp_LISTEN:
	  case tcp_SYNSENT:
		if (s->dataSize != 0)
			s->close = true;
		else {                 /* cannot be in an upcall here */
			s->poll = false;
			s->state = tcp_CLOSED;
			tcp_Drop(s->tcp, s);
			(void)s->handler(s, s->link, NIL, 0);  /* release client */
			tcp_Free(s);
			log_msg(s->tcp, 1, "Closed");
		}
		break;
	}
}
#endif

/*
 * Abort a tcp connection.  (NOT currently safe within an upcall)
 */
void
tcpAbort(tcp_Socket s) {
	s->dataSize = 0;
	s->snd_una = s->snd_nxt;
	switch (s->state) {
	  case tcp_SYNREC:
	  case tcp_ESTAB:
	  case tcp_FINWT1:
	  case tcp_FINWT2:
	  case tcp_CLOSEWT:
		s->ctl = TH_RST;
		tcp_Transmit(s);
		break;
	  default:
		break;
	}
	tcp_SignalError(s->tcp, s, tcp_aborted);
	tcp_Free(s);
}

/*
 * Handler for incoming packets from IP.
 */
void
tcpHandler(tcp_State tcp, iphdr *ip) {
	tcphdr *tp;
	long  hlen, len;
	Boolean  acked;
	long  acks;
	tcp_Socket s, ls;
	ushort flags;

	hlen = IP_HLEN(ip);
	tp = (tcphdr *)((uchar *)ip + hlen);
	len = ip->ip_len - hlen;

	if (tcp_Checksum(ip) != 0) {
		printf("Bad tcp checksum, discarding\n");
		return;
	}

	flags = tp->th_off_flags;

	/* find the TCB */
	ls = NIL;
	for (s = tcp->all_tcbs; s; s = s->next)
		if (tp->th_dport == s->lcl_port) {
			if (s->rem_port == 0)
				ls = s;    /* candidate passive listener */
			else if (ip->ip_src == s->rem_addr && tp->th_sport == s->rem_port)
				break;
		}
	if (!s) {
		if (ls && (flags & TH_SYN))
			s = ls;
		else {
			log_packet(tcp, ip, tp, "Discarding");
			if ((flags & TH_RST) == 0) tcp_Reset(tcp, ip, tp);
			return;
		}
	}

	log_packet(tcp, ip, tp, "Received");

	acked = false;  acks = 0;
	if (flags & TH_ACK) {
		long  diff = s->snd_nxt - tp->th_ack;
		acks = tp->th_ack - s->snd_una;
		if (diff == 0) s->poll = false;
#ifdef BUGGY
	        if (diff == 0 && s->dataSize == 0) s->poll = false;
#endif
		if (acks > 0 && diff >= 0) acked = true;
	}

	hlen = TCP_HLEN(tp);
	if (hlen > TCP_SIZE) {
		uchar *op;
		op = (uchar *)tp + TCP_SIZE;
		while (op < (uchar *)tp + TCP_HLEN(tp) && op[0] != TCPOPT_EOL) {
			if (op[0] == TCPOPT_NOOP)
				op++;
			else {
				if (op[0] == TCPOPT_MAXSEG && op[1] == 4 && (flags & TH_SYN)) {
					ushort mss = (op[2]<<8) | op[3];
					if (mss > 0 && mss < s->snd_mss) s->snd_mss = mss;
				}
				op += (op[1] == 0) ? 1 : op[1];   /* 0 == TCPOPT_EOL */
			}
		}
	}

	if (s->state == tcp_LISTEN) {

		if (flags & TH_RST) return;
		if (flags & TH_ACK) {
			tcp_Reset(tcp, ip, tp);
			return;
		}
		if (flags & TH_SYN) {
			s->rem_port = tp->th_sport;
			s->rem_addr = ip->ip_src;
			s->lcl_addr = ip->ip_dst;
			s->rcv_nxt = tp->th_seq + 1;
#ifdef DEBUG
			s->snd_nxt = s->snd_una = 0;
#else
			s->snd_nxt = s->snd_una = Now()*256 + tcp->conn_id++;
#endif
			s->snd_wnd = tp->th_win;
			s->ctl = TH_SYN | TH_ACK;
			s->timeout = tcp_TIMEOUT;
			s->retries = 0;
			tcp_Transmit(s);
			s->state = tcp_SYNREC;
#ifdef DEBUG
			if (tcp->logLevel >= 1)
				printf("Connection from 0x%08lx:%d\n",
					s->rem_addr, s->rem_port);
#endif
			/* accept data or FIN now */
		}

	} else if (s->state == tcp_SYNSENT) {

		if (flags & TH_ACK && (!acked || acks != 1)) {
			if ((flags & TH_RST) == 0) tcp_Reset(tcp, ip, tp);
			return;
		}
		if (flags & TH_RST) {
			if ((flags & TH_ACK) == 0) {
				log_msg(tcp, 1, "Connection refused");
				tcp_SignalError(tcp, s, tcp_refused);
				tcp_Free(s);
			}
			return;
		}
		if (flags & TH_SYN) {
			s->rcv_nxt = tp->th_seq + 1;
			s->snd_wnd = tp->th_win;
			s->ctl = TH_ACK;
			s->timeout = tcp_TIMEOUT;
			s->retries = 0;
			if (acked) {
				log_msg(tcp, 1, "Open");
				s->snd_una++;
				s->state = tcp_ESTAB;
				/* XXX accept data or FIN now XXX */
			} else {
				s->ctl |= TH_SYN;
				s->snd_nxt = s->snd_una;  /* ISS */
				s->state = tcp_SYNREC;
			}
			tcp_Transmit(s);
		}

	} else {

		/* XXX validate sequence number here, don't rely on ProcessSegment */

		if (flags & TH_RST) {
			log_msg(tcp, 1, "Connection reset");
			tcp_SignalError(tcp, s, tcp_reset);
			tcp_Free(s);
			return;
		}
		/* XXX check SYN here XXX */
		if ((flags & TH_ACK) == 0) return;

		switch (s->state) {

		  case tcp_SYNREC:
			if (acked) {
				s->snd_una += acks;
				if (--acks > 0) {
					s->dataOrigin += acks;
					if (s->dataOrigin >= tcp_MaxData)
						s->dataOrigin -= tcp_MaxData;
					s->dataSize -= acks;
				}
				s->ctl = TH_ACK;
				s->timeout = tcp_TIMEOUT;
				s->retries = 0;
				s->state = tcp_ESTAB;
				s->snd_wnd = tp->th_win - (s->snd_nxt - tp->th_ack);
				tcp_ProcessSegment(s, tp, len);
			} else
				tcp_Reset(tcp, ip, tp);
			break;

		  case tcp_ESTAB:
		  case tcp_CLOSEWT:
			s->ctl = TH_ACK;
			if (acked) {
				s->snd_una += acks;
				s->dataOrigin += acks;
				if (s->dataOrigin >= tcp_MaxData)
					s->dataOrigin -= tcp_MaxData;
				s->dataSize -= acks;
				s->retries = 0;
				s->timeout = tcp_TIMEOUT;
			}
			s->snd_wnd = tp->th_win - (s->snd_nxt - tp->th_ack);
			tcp_ProcessSegment(s, tp, len);
			break;

		  case tcp_FINWT1:
			s->ctl = TH_ACK;
			if (acked) {
				s->snd_una += acks;
				if (s->snd_una == s->snd_nxt) {
					acks -= 1;
					s->state = tcp_FINWT2;
				} else
					s->ctl |= TH_FIN;
				if (acks > 0) {
					s->dataOrigin += acks;
					if (s->dataOrigin >= tcp_MaxData)
						s->dataOrigin -= tcp_MaxData;
					s->dataSize -= acks;
				}
			}
			s->snd_wnd = tp->th_win - (s->snd_nxt - tp->th_ack);
			tcp_ProcessSegment(s, tp, len);
			break;

		  case tcp_FINWT2:
			s->ctl = TH_ACK;
			tcp_ProcessSegment(s, tp, len);
			break;

		  case tcp_CLOSING:
			if (acked) {
				s->timeout = tcp_TIME2MSL;
				s->retries = 0;
				s->poll = true;
				s->state = tcp_TIMEWT;
				(void)s->handler(s, s->link, NIL, 0);    /* release client */
			}
			break;

		  case tcp_LASTACK:
			if (acked) {
				s->dataSize = 0;
				s->poll = false;
				s->state = tcp_CLOSED;
				tcp_Drop(tcp, s);
				(void)s->handler(s, s->link, NIL, 0);  /* release client */
				tcp_Free(s);
				log_msg(tcp, 1, "Closed");
			}
			break;

		  case tcp_TIMEWT:
			if (flags & TH_SYN)
				tcp_Reset(tcp, ip, tp);
			else {
				s->ctl = TH_ACK;
				tcp_Transmit(s);
			}
			break;

		  case tcp_CLOSED:
			tcp_Reset(tcp, ip, tp);
			break;

		}
	}
}

/*
 * Handler for ICMP messages.
 */
void
tcpNotify(tcp_State tcp, iphdr *ip, iphdr *mip) {
	icmp       *imp;
	tcphdr     *mtp;
	tcp_Socket  s;

	imp = (icmp *)((uchar *)ip + IP_HLEN(ip));
	mtp = (tcphdr *)((uchar *)mip + IP_HLEN(mip));

	/* demux */
	for (s = tcp->all_tcbs; s; s = s->next) {
		if (mip->ip_src == s->lcl_addr  && mtp->th_sport == s->lcl_port
		&&  mip->ip_dst == s->rem_addr && mtp->th_dport == s->rem_port)
			break;
	}
	if (s) {
		switch (imp->icmp_type) {   /* see RFC 1122 */
			case ICMP_SOURCEQUENCH:
				/* adjust rate */
				break;
			case ICMP_UNREACH:
				switch (imp->icmp_code) {
					case ICMP_UNREACH_NET:
					case ICMP_UNREACH_HOST:
					case ICMP_UNREACH_SRCFAIL:
						s->error = tcp_unreachable;  /* hint only */
						break;
					case ICMP_UNREACH_PROTOCOL:
					case ICMP_UNREACH_PORT:
					case ICMP_UNREACH_NEEDFRAG:
						if (s->state != tcp_LISTEN) {
							log_msg(tcp, 1, "Destination unreachable");
							tcp_SignalError(tcp, s, tcp_unreachable);
							tcp_Free(s);
						}
						break;
				}
				break;
			case ICMP_TIMXCEED:
			case ICMP_PARAMPROB:
				s->error = tcp_unreachable;  /* hint only */
				break;
		}
	}
}

/*
 * periodic processing for tcp.  Called from main event loop.
 */
void
tcpPoll(tcp_PCB *tcp) {
    long  diff = Now() - tcp->timeNext;

	if (diff > 0) {
		tcp_Timer(tcp);
		tcp->timeNext = Now() + tcp_POLLTIME;
	}
}

/*
ulong MakeIPAddress(char *string)
{
	ulong	piece = 0;
	ulong	piecesSeen = 0;
	ulong	address = 0;
	ulong	count;
	uchar	c;

	count = strlen(string);
	while (count && piecesSeen < 4)
		{
		c = *string++;
		count--;
		if (c >= '0' && c <= '9')
			piece = (piece * 10) + (c - '0');
		if (c == '.' || !count)
			{
			address <<= 8;
			address |= piece;
			piecesSeen++;
			piece = 0;
			}
		}

	if (!count && piecesSeen == 4)
		return address;
	return 0;
	}
*/