/* ===========================================================================
	ppp.h

	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __PPP_H__
#define __PPP_H__

/* Defines for Point-to-Point Protocol (PPP) implementation.      */

/* PPP control characters and params */
#define kFlag			0x7e						/* PPP frame delimiter 		*/
#define kAllCallAddress	0xff						/* all-call frame address	*/
#define kUnnumberedInfo	0x03						/* unnumbered info frames	*/
#define kControlEscape	0x7d						/* character escape 		*/

#define kPPPInitFCS     0xffff  					/* Initial FCS value		*/
#define kPPPGoodFCS     0xf0b8  					/* Good final FCS value		*/

#define kMaximumPDUSize	1250						/* default maximum PDU size	*/
#define kRecvBufferSize (kMaximumPDUSize + 8)		/* receive buffer			*/

/* PPP command bytes */
#define kConfigureRequest	1
#define kConfigureAck		2
#define kConfigureNak		3
#define kConfigureReject	4
#define kTerminateRequest	5
#define kTerminateAck		6
#define kCodeReject			7
#define kProtocolReject		8
#define kEchoRequest		9
#define kEchoReply			10
#define kDiscardRequest		11

/* Link Control Protocol codes */
#define kLCPMaxRecvUnit		1						/* maximum receive unit			*/
#define kLCPCharMap			2						/* async control character map	*/
#define kLCPAuthProt		3						/* authentication protocol		*/
#define kLCPQualityProt		4						/* quality protocol				*/
#define kLCPMagicNumber		5						/* magic number					*/
#define kLCPProtCompress	7						/* protocol field compression	*/
#define kLCPAddrCompress	8						/* address/control field compression  */

/* IP Control Protcol codes */
#define kIPCPAddresses		1						/* obsolete but still in the spec 	  */
#define kIPCPCompress		2						/* compression protocol (Van Jacobson */
#define kIPCPAddress		3						/* IP Address to be assigned		  */


/* private flags */
#define kMaxPacketSizeMask			0x0000FFFF
#define	kAddressFieldCompressMask	0x00010000
#define kProtocolFieldCompressMask	0x00020000

/* pass infomation to PPP LCP handling routines */

typedef struct
	{
	void* next;								/* pointer to next PDUInfo struct in chain */
	uchar* bufPtr;							/* pointer to the data */
	ulong length;							/* data length */
	uchar *hdrPtr;
	ulong  totalLength;
	uchar  id;
	} LCPInfo;


/* State machine for LCP negotiation */
typedef enum
	{  /* note: closed, stopped, closing, stopping never entered */
	initial  = 0,
	starting = 1,
	closed   = 2,
	stopped  = 3,
	closing  = 4,
	stopping = 5,
	req_sent = 6,
	ack_rcvd = 7,
	ack_sent = 8,
	opened   = 9
	} lcpState;

/* format of PDU interface to PPP */
typedef struct
	{
	void* next;								/* pointer to next PDUInfo struct in chain */
	uchar* bufPtr;							/* pointer to the data */
	ulong length;							/* data length */
	unsigned short protocol;						/* PPP protocol value */
	} PPPPDUInfo;

/* Link Control Protocol packet header definition */
typedef struct
	{
	uchar	code;
	uchar	id;
	unsigned short	len;
	uchar	*data;
	} LCPPacket;

/* Combined IP and UDP packet header definition. Waste of bandwidth required to support terminal server */
typedef struct
	{
	/* IP packet header */
	uchar   versLen;		/* version 4 (4 bits) and IP header length in longwords (=5) (4 bits) */
	uchar   serviceType;	/* type of service hint */
	unsigned short  length;			/* total length of IP datagram in bytes, including IP header */
	unsigned short  id;				/* block id, increment each block */
	unsigned short  fragOffset;		/* 4 bits of flags, 12 bits fragment offset. 4000 means do not fragment */
	uchar   timeToLive;		/* time for packet to live in hops (or perhaps seconds) */
	uchar   protocol;		/* protocol identifer, we use IP */
	unsigned short  checksum;		/* IP checksum of header only */
	ulong   sourceAddress;	/* source IP address */
	ulong   destAddress;	/* destination IP address */

	/* UDP packet header */
	unsigned short	udpSourcePort;
	unsigned short  udpDestPort;
	unsigned short	udpLength;		/* total length of UDP datagram in bytes, including UDP header */
	unsigned short	udpChecksum;	/* optional, we leave it zero */
	} IPUDPPacket;

#define kIPPacketSize 20
#define kUDPPacketSize 8

/* PPP Protocol numbers */
#define kLinkControlProtocol	0xc021			/* protocol value for Link Control Protocol */
#define kPasswordAuthProtocol	0xc023			/* EMAC: PAP authentication */
#define kIPCProtocol			0x8021			/* protocol value for Link Control Protocol */
#define kIPProtocol				0x0021			/* straight IP */

/* IP protocol numbers */
#define kUDPProtocol			0x11			/* UDP, used in IP header */
#define kICMPProtocol			0x01			/* ICMP, host sends for error processing */
#define kICTEchoRequest			0x00			/* ICMP ping (not handled) */
#define kICTDestUnreachable		0x03			/* ICMP ic_type for Destination Unreachable */
#define kICTSourceQuench		0x04			/* ICMP ic_type for Source Quench (net congestion) */

extern ulong gOurIPAddr;

long nowstartppp(void);
void WritePDU(ushort protocol, uchar *pkt, ushort length);
long MaxReadPDUSize(void);
long MaxWritePDUSize(void);
void TCPIdle(Boolean multi);
Boolean GetPPPConnected();
void SetPPPConnected(Boolean state);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include ppp.h multiple times"
	#endif
#endif /* __PPP_H__ */