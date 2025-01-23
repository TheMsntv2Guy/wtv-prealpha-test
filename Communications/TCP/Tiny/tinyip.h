#ifndef __TINYIP_H__
#define __TINYIP_H__
/* ===========================================================================
 *	tinyip.h
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

/*
 * network level dispatch
 */

#include "ip.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IFC_ID	int
#define N_IFC   4

/* ----------------------- utilities ----------------------- */

ushort  checksum(ushort *dp, ulong len);

/* ----------------- IP state information ----------------- */

typedef struct _ip_State {
	ushort  ip_id;
	ip_addr addr;
	void   *ip_udp;           /* transport clients */
	void   *ip_tcp;
} ip_State;
/* typedef struct _ip_State ip_State;  opaque */

/* ------------------- the main dispatch ------------------- */

/* execute periodically (~200-500 ms) to poll status */

void     ipMonitor(ip_State *ip);

/* ------------- network interface management ------------- */

int      ipAttach(ip_State *ip, IFC_ID ifc, ip_addr addr);
int      ipDetach(ip_State *ip, IFC_ID ifc);

/* ---------------- transport client state ---------------- */

void    *ipTransport(ip_State *ip, uchar proto);

/* ------------- the client service interface ------------- */

ip_State *ipInit(void);
#ifdef INCLUDE_FINALIZE_CODE
void      ipFinalize(ip_State *ip);
#endif /* INCLUDE_FINALIZE_CODE */
void 	SetIpAddress(ip_State *ip, ip_addr myAddr);
ip_addr  ipAddress(ip_State *ip, ip_addr dest);
long     ipMRU(ip_State *ip, ip_addr dest);
long     ipMTU(ip_State *ip, ip_addr dest);

void    *ipAlloc(ulong);
void     ipFree(void *);

long     ipSend(uchar *pkt, long len);
void     ipIn(ip_State *ip, uchar *pkt, long len);
#ifdef __cplusplus
}
#endif
#endif /* __TINYIP_H__ */
