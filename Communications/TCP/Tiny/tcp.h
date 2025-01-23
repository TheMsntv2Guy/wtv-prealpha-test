#ifndef TCP_H
#define TCP_H
/* ===========================================================================
 *	tcp.h
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */
 
#include "ip.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------  TCP defns --------------------- */

typedef struct tcphdr {
    ushort          th_sport;
    ushort          th_dport;
    ulong           th_seq;
    ulong           th_ack;
    ushort          th_off_flags; /* length (4 bits), reserved (6 bits), flags (6 bits) */
    ushort          th_win;
    ushort          th_sum;
    ushort          th_urp;
} tcphdr;

#define TCP_SIZE  sizeof(tcphdr)
#define TCP_OFF   0xf000
#define TCP_HLEN(tp)  (((tp)->th_off_flags & 0xf000)>>10)

#define TH_FIN     0x01
#define TH_SYN     0x02
#define TH_RST     0x04
#define TH_PUSH    0x08
#define TH_ACK     0x10
#define TH_URG     0x20

#define TCPOPT_EOL      0
#define TCPOPT_NOOP     1
#define TCPOPT_MAXSEG   2

#ifdef __cplusplus
}
#endif
#endif
