/* ===========================================================================
	ppp.c
	
	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */
#include "Headers.h"

#include "BoxUtils.h"
#include "defs.h"
#include "OptionsPanel.h"
#include "ppp.h"
#include "Serial.h"
#include "tinyip.h"
#ifndef __TELLYIO_H__
#include "TellyIO.h"
#endif
#ifdef HARDWARE
#include "HWModem.h"
#endif

#include "Utilities.h"

/* ===========================================================================
	consts, globals, #defines
=========================================================================== */

#define	GetLongFromStream(data, ptr)	do { data =	(*((uchar*)ptr + 0) << 24) | (*((uchar*)ptr+1) << 16) | \
													(*((uchar*)ptr + 2) <<  8) | (*((uchar*)ptr+3) <<  0); } while(0)
#define	GetShortFromStream(data, ptr)	do { data = (*((uchar*)ptr + 0) <<  8) | (*((uchar*)ptr+1) << 0); } while(0)
#define	PutLongToStream(data, ptr)		do {	*((uchar*)ptr + 0) = data >> 24; *((uchar*)ptr+1) = data >> 16; \
												*((uchar*)ptr + 2) = data >>  8; *((uchar*)ptr+3) = data >>  0; } while(0)
#define kOneSecond 60

static long gLCPNegotiationLayer;
static long gShouldRetryLCP;
static long gState;
static ulong gLocalOptions;
static ulong gRemoteOptions;
static ulong gRemoteCharMap;
static ulong gTxCharMap;
static ulong gRxCharMap;
static long gMRU;
static uchar gPPPConnected;
static Boolean gConfigAcknowledged;
ip_State *gIPState;
ulong gOurIPAddr;

/* vars used in readFrame.  global so they keep state between calls */
/* NB: globals and not static so we can reset them */

static uchar	gPPPincomingBuffer[kRecvBufferSize];
static uchar 	gPPPoutgoingBuffer[kRecvBufferSize];
static uchar	*gPPPincomingBufferPtr;
static ushort	gPPPlength;
static Boolean	gPPPendOfFrame;
static Boolean	gPPPstartOfFrame;
static Boolean	gPPPlastCharWasEscape;

/*
* The HDLC polynomial: x**0 + x**5 + x**12 + x**16 (0x8408).
*/
#define FACTORS   0x8408

const ushort	CRC16Table[256] =
	{
	0x0000,0x1189,0x2312,0x329B,0x4624,0x57AD,0x6536,0x74BF,
	0x8C48,0x9DC1,0xAF5A,0xBED3,0xCA6C,0xDBE5,0xE97E,0xF8F7, 
	0x1081,0x0108,0x3393,0x221A,0x56A5,0x472C,0x75B7,0x643E,
	0x9CC9,0x8D40,0xBFDB,0xAE52,0xDAED,0xCB64,0xF9FF,0xE876, 
	0x2102,0x308B,0x0210,0x1399,0x6726,0x76AF,0x4434,0x55BD,
	0xAD4A,0xBCC3,0x8E58,0x9FD1,0xEB6E,0xFAE7,0xC87C,0xD9F5, 
	0x3183,0x200A,0x1291,0x0318,0x77A7,0x662E,0x54B5,0x453C,
	0xBDCB,0xAC42,0x9ED9,0x8F50,0xFBEF,0xEA66,0xD8FD,0xC974, 
	0x4204,0x538D,0x6116,0x709F,0x0420,0x15A9,0x2732,0x36BB,
	0xCE4C,0xDFC5,0xED5E,0xFCD7,0x8868,0x99E1,0xAB7A,0xBAF3, 
	0x5285,0x430C,0x7197,0x601E,0x14A1,0x0528,0x37B3,0x263A,
	0xDECD,0xCF44,0xFDDF,0xEC56,0x98E9,0x8960,0xBBFB,0xAA72, 
	0x6306,0x728F,0x4014,0x519D,0x2522,0x34AB,0x0630,0x17B9,
	0xEF4E,0xFEC7,0xCC5C,0xDDD5,0xA96A,0xB8E3,0x8A78,0x9BF1, 
	0x7387,0x620E,0x5095,0x411C,0x35A3,0x242A,0x16B1,0x0738,
	0xFFCF,0xEE46,0xDCDD,0xCD54,0xB9EB,0xA862,0x9AF9,0x8B70, 
	0x8408,0x9581,0xA71A,0xB693,0xC22C,0xD3A5,0xE13E,0xF0B7,
	0x0840,0x19C9,0x2B52,0x3ADB,0x4E64,0x5FED,0x6D76,0x7CFF, 
	0x9489,0x8500,0xB79B,0xA612,0xD2AD,0xC324,0xF1BF,0xE036,
	0x18C1,0x0948,0x3BD3,0x2A5A,0x5EE5,0x4F6C,0x7DF7,0x6C7E, 
	0xA50A,0xB483,0x8618,0x9791,0xE32E,0xF2A7,0xC03C,0xD1B5,
	0x2942,0x38CB,0x0A50,0x1BD9,0x6F66,0x7EEF,0x4C74,0x5DFD, 
	0xB58B,0xA402,0x9699,0x8710,0xF3AF,0xE226,0xD0BD,0xC134,
	0x39C3,0x284A,0x1AD1,0x0B58,0x7FE7,0x6E6E,0x5CF5,0x4D7C, 
	0xC60C,0xD785,0xE51E,0xF497,0x8028,0x91A1,0xA33A,0xB2B3,
	0x4A44,0x5BCD,0x6956,0x78DF,0x0C60,0x1DE9,0x2F72,0x3EFB, 
	0xD68D,0xC704,0xF59F,0xE416,0x90A9,0x8120,0xB3BB,0xA232,
	0x5AC5,0x4B4C,0x79D7,0x685E,0x1CE1,0x0D68,0x3FF3,0x2E7A, 
	0xE70E,0xF687,0xC41C,0xD595,0xA12A,0xB0A3,0x8238,0x93B1,
	0x6B46,0x7ACF,0x4854,0x59DD,0x2D62,0x3CEB,0x0E70,0x1FF9, 
	0xF78F,0xE606,0xD49D,0xC514,0xB1AB,0xA022,0x92B9,0x8330,
	0x7BC7,0x6A4E,0x58D5,0x495C,0x3DE3,0x2C6A,0x1EF1,0x0F78	
	};
#define	GetCRC16Table()	CRC16Table

/* compute frame check sequence */




/* ===========================================================================
	static function prototypes
=========================================================================== */
static ushort ComputeFCS(ushort fcs, uchar* ptr, ulong len);
static void ConfigureAck(ushort protocol, uchar *pkt, ushort length);
static void ConfigureNak(ushort protocol, uchar *pkt, ushort length);
static long NegotiateLCP(ulong destIP, ushort destPort);
//static void WriteEscapedByte(uchar outChar, ulong map);
static void HandleLCPFrame(ushort protocol, uchar *ptr, ushort length);
static void processFrame(uchar *ptr, ushort length);
static long readFrame(void);
static void ConfigureRequest(ushort protocol, uchar *pkt, ushort length);
static void initPPP(void);

/* ===========================================================================
	implementation
=========================================================================== */

static ushort
ComputeFCS(ushort fcs, uchar* ptr, ulong len)
	{
	const ushort* fcsTable = GetCRC16Table();
	while (len--)
 		fcs = (fcs >> 8) ^ fcsTable[(fcs ^ *ptr++) & 0xff];
	return fcs;
	}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long
MaxReadPDUSize(void)
{
	return gMRU;
}

long
MaxWritePDUSize(void)
{
	return gMRU;
}
#endif

static void
ConfigureAck(ushort protocol, uchar *pkt, ushort length)
	{
	#pragma unused(pkt, length)
	/* advance to new state after config_req event */
	switch (gState) 
		{
		case req_sent:
			gState = ack_rcvd;
			gConfigAcknowledged = true;
			break;
		case ack_rcvd:
			gState = req_sent;
			break;
		case ack_sent:
			/* set these immediately, before an inbound packet might pre-empt */
			if (protocol == kLinkControlProtocol)
				{
				gLCPNegotiationLayer = kIPCProtocol;
				gTxCharMap = gRemoteCharMap;
				gRxCharMap = 0;
				gLocalOptions = gRemoteOptions;
				}
			gState = opened;
			break;
		default:
			Message(("unhandled state %ld in config ack", gState));
		}
	}

static void
ConfigureNak(ushort protocol, uchar *pkt, ushort length)
	{
	LCPPacket 	*packet = (LCPPacket*) pkt;
	uchar 		*optPtr = pkt + 6;	/* skip packet header, type, length */

	/* accept IP address for us to use */
	GetLongFromStream(gOurIPAddr, optPtr);
	optPtr += 4;
	Message(("PPP: our IP addr = 0x%lx", gOurIPAddr));

	/* send configuration request with new options, and advance state */
	packet->code = kConfigureRequest;
	if (gState != ack_sent)
		gState = req_sent;

	WritePDU(protocol, pkt, length);
	}

static long
NegotiateLCP(ulong destIP, ushort destPort)
	{
	#pragma unused(destIP, destPort)
	ushort	 	retries = 0;
	ulong	 	base;
	uchar 		lpcConfig[14], ipcpConfig[10];
	ushort		protocol;

	lpcConfig[1] = 30;
	ipcpConfig[1] = 40;	

lpc:
	/* CnfReq  ID     len     type2: char map = 00000000.      7: ProtCompr 8: AddrCompr */
	lpcConfig[0] = 0x01; lpcConfig[1]++; lpcConfig[2] = 0x00; lpcConfig[3] = 0x0E; 
	lpcConfig[4] = 0x02; lpcConfig[5] = 0x06; lpcConfig[6] = 0x00; lpcConfig[7] = 0x00; 
	lpcConfig[8] = 0x00; lpcConfig[9] = 0x00; lpcConfig[10] = 0x07; lpcConfig[11] = 0x02; 
	lpcConfig[12] = 0x08; lpcConfig[13] = 0x02;
	
	/* request 0.0.0.0 IP address */
	ipcpConfig[0] = 0x01; ipcpConfig[1]++; ipcpConfig[2] = 0x00; ipcpConfig[3] = 0x0a; ipcpConfig[4] = 0x03; 
	ipcpConfig[5] = 0x06; ipcpConfig[6] = 0x00; ipcpConfig[7] = 0x00; ipcpConfig[8] = 0x00; ipcpConfig[9] = 0x00; 

	gShouldRetryLCP = false;
	gLCPNegotiationLayer = kLinkControlProtocol;
	protocol = kLinkControlProtocol;

	/* Send out our initial Configure Request. If no ACK received in 3 sec, retry up to 3 times */
	gState = req_sent;
	gRxCharMap = 0xffffffff;
	gTxCharMap = 0xffffffff;
	gRemoteCharMap = 0xffffffff;
	gLocalOptions = 0x00;
	gConfigAcknowledged = false;

	WritePDU(protocol, lpcConfig, sizeof(lpcConfig));		/* send LCP config request */

	base = Now();
	while (gState != opened)
		{
		TellyIdle();
		readFrame();
		if (((Now() - base) > (3 * kOneSecond)) && !gConfigAcknowledged)
			{
			if (++retries >= 4)
				{
				Message(("LCP negotiation failed."));
				return 0;
				}
				
			++lpcConfig[1];	/* bump the ID */
			Message(("PPP: Retry (%d) LCP config request", retries));
			WritePDU(protocol, lpcConfig, sizeof(lpcConfig));
			base = Now();
			}
		}

	// EMAC: added for LC2
	Message(("EMAC: Sending hacky PAP login message username=test, password=test"));
	uchar hackyPAPlogin[] = "\x01\x00\x00\x0E\x04test\x04test";
	protocol = kPasswordAuthProtocol;
	gLCPNegotiationLayer = kPasswordAuthProtocol;
	gState = req_sent;
	retries = 0;
	WritePDU(protocol, hackyPAPlogin, 14);
	base = Now();
	while (gState != opened)
		{
		TellyIdle();
		readFrame();
		if (((Now() - base) > (3 * kOneSecond)) && !gConfigAcknowledged)
			{
			if (++retries >= 4)
				{
				Message(("EMAC: PAP login failed."));
				return 0;
				}
			++lpcConfig[1];	/* bump the ID */
			Message(("EMAC: PPP: Retry (%d) PAP login request", retries));
			WritePDU(protocol, hackyPAPlogin, 14);
			base = Now();
			}
		}

	gTxCharMap = gRemoteCharMap;
	gRxCharMap = 0;
	gLocalOptions = gRemoteOptions;
	gConfigAcknowledged = false;

	/* Send out our initial IPCP. If no ACK received in 3 sec, retry up to 3 times */
	protocol = kIPCProtocol;
	gLCPNegotiationLayer = kIPCProtocol;

	gState = req_sent;

	retries = 0;
	WritePDU(protocol, ipcpConfig, sizeof(ipcpConfig));

	base = Now();
	while (gState != opened)
		{
		TellyIdle();
		readFrame();
		if (((Now() - base) >(5 * kOneSecond)) && !gConfigAcknowledged)
			{
			if (gShouldRetryLCP) goto lpc;
			if (++retries >= 4)
				{
				Message(("IPCP negotiation failed."));
				return 0;
				}

			++ipcpConfig[1];	/* bump the ID */
			PutLongToStream(gOurIPAddr, &ipcpConfig[6]);
			WritePDU(protocol, ipcpConfig, sizeof(ipcpConfig));
			base = Now();
			}
		else 
			{
			if (gState == closed)
				goto  lpc;
			}
		}

	gLCPNegotiationLayer = kIPProtocol;
	return 1;
	}

static void
HandleLCPFrame(ushort protocol, uchar *ptr, ushort length)
	{
	uchar code;
	LCPPacket *packet = (LCPPacket*)ptr;

	code = packet->code;
	if (protocol != gLCPNegotiationLayer)
		{
		if (gLCPNegotiationLayer == kIPCProtocol)
			{
			gShouldRetryLCP = true;
			return;
			}
		}
	/* process link control frame */
	switch (code)
		{
		case kConfigureRequest:
			ConfigureRequest(protocol, ptr, length);
			break;

		case kConfigureAck:
			ConfigureAck(protocol, ptr, length);
			break;

		case kConfigureNak:
			ConfigureNak(protocol, ptr, length);
			break;

		case kTerminateRequest:
/*				TerminateRequest(&hdrInfo); */
			break;

		case kConfigureReject:
		case kTerminateAck:
		case kCodeReject:
		case kProtocolReject:
		case kEchoRequest:
		case kEchoReply:
		case kDiscardRequest:
		default:
			Message(("can't handle LCP code: %d", code));
			break;
		}
	}

static void
processFrame(uchar *ptr, ushort length)
	{
	ushort		protocol;

	/* process frame */
	if (ComputeFCS(kPPPInitFCS, ptr, length) != kPPPGoodFCS)
		{

		/* data error, abort processing */
		Message(("processFrame: PPP FCS failed (checksum error)"));
		return;
		}

	/* strip off fcs */
	length -= 2;

	if  ((ptr[0] == kAllCallAddress) && (ptr[1] == kUnnumberedInfo))
		{
		/* uncompressed address/type fields, skip over */
		ptr += 2;
		length -= 2;
		}

	/* get first protocol byte */
	protocol = *ptr++;
	length--;
	if ((protocol & 0x01) == 0)
		{
		/* if the low-order bit is 0, 2-byte field */
		protocol = (protocol << 8) + *ptr++;
		length--;
		}

	/* Hack for odd-aligned packets- move everything up one byte. */
	/* The only way the packet can be odd-aligned is via protocol compression, so we have at least 1 byte pre-pad */
	if ((ulong)ptr & 1)
		{
		memmove(ptr - 1, ptr, length);
		--ptr;
		}

	if ((protocol == kIPProtocol) || (protocol == kICMPProtocol))
		ipIn(gIPState, ptr, length);
	else if ((protocol == kLinkControlProtocol)
		|| (protocol == kIPCProtocol))
		HandleLCPFrame(protocol, ptr, length);
	else if ((protocol == kPasswordAuthProtocol))
	{
		Message(("EMAC: ignoring PAP response from server. Assuming login ACK!"));
		gState = opened;
	}
	else
		Message(("processFrame: unknown protocol in protocol field: %d", protocol));
	}

void initPPP()
{
	gPPPincomingBufferPtr = gPPPincomingBuffer;
	gPPPlength = 0;
	gPPPendOfFrame = false;
	gPPPstartOfFrame = false;
	gPPPlastCharWasEscape = false;
}

static long
readFrame(void)
	{
	ulong			waiting;
	uchar 			c;

	while (( (waiting = SerialCountReadPending(kMODEM_PORT)) > 0) && !gPPPendOfFrame)
		while (waiting && !gPPPendOfFrame)
		{
		if (SerialReadSync(kMODEM_PORT, (char*)&c, 1) == 0)
			{
			Message(("readFrame: SerialReadSync failed"));
			return 0;
			}
			
		waiting--;
		if (c == kFlag)
			{
			if (gPPPlastCharWasEscape)
				{
				gPPPlastCharWasEscape = false;
				/* frame abort */
				Message(("readFrame: PPP frame abort"));
				gPPPlength = 0;
				gPPPincomingBufferPtr = gPPPincomingBuffer;
				gPPPendOfFrame = false;
				gPPPstartOfFrame = false;
				return 0;
				}

			if(gPPPstartOfFrame)
				gPPPendOfFrame = true;
			else
				gPPPstartOfFrame = true;

			break;
			}

		if (gPPPstartOfFrame)
			{
			if (gPPPlastCharWasEscape)
				{
				gPPPlastCharWasEscape = false;
				c ^= 0x20;
				*gPPPincomingBufferPtr++ = c;
				gPPPlength++;
				}
			else if (c >= 0x20)
				{
				/* always process characters >= 0x20 */
				if (c == kControlEscape)
					gPPPlastCharWasEscape = true;
				else
					{
					/* put character into the buffer */
					*gPPPincomingBufferPtr++ = c;
					gPPPlength++;
					}
				}
			else if (((gRxCharMap >> c) & 0x01) == 0)
				{
				/* only process non-mapped characters */
				*gPPPincomingBufferPtr++ = c;
				gPPPlength++;
				}
			}
			
		if (!gPPPendOfFrame && (gPPPlength == kRecvBufferSize))
			{
				Message(("readFrame: PPP readFrame: about to overflow buffer!"));
				gPPPlength = 0;
				gPPPincomingBufferPtr = gPPPincomingBuffer;
				gPPPendOfFrame = false;
				gPPPstartOfFrame = false;
				return 0;
			}
		}

		if (gPPPendOfFrame)
			{
			// EMAC: readFrame looses track of the start and end of a frame and interpreats the end as a start and a start as an end so we end up with a 0 byte frame.
			// EMAC: this fixes that so it's re-aligned.

			if(gPPPlength > 0) {
				processFrame(gPPPincomingBuffer, gPPPlength);
			} else {
				Message(("EMAC: looks like the PPP frame reader is mis-aligned. Fixing..."));
			}

			gPPPendOfFrame = false;
			gPPPstartOfFrame = false;
			gPPPlength = 0;
			gPPPincomingBufferPtr = gPPPincomingBuffer;

			if(gPPPlength == 0) {
				gPPPstartOfFrame = true;
				return 0;
			} else {
				return 1;
			}
			}
		return 0;
	}

void
WritePDU(ushort protocol, uchar *pkt, ushort length)
	{
	ulong 	 		map;
	ushort 			oldFcs;
	ushort			outByteCount = 0;
	uchar 			outByte;
	unsigned char*	PPPoutgoingBufferp;
	ulong			localOptions = gLocalOptions;
	long			i;
#if	0
	LCPPacket		*packet = (LCPPacket *)pkt;
	static const char		*lcpCodes[] = {0, "CFG_REQ", "CFG_ACK",
				"CFG_NAK", "CFG_REJ", "TERM_RQ", "TERM_AK"};
#endif
	map = gTxCharMap;

#define WriteByte(byte)					\
		*PPPoutgoingBufferp = byte; 	\
		PPPoutgoingBufferp++;			\
		outByteCount++;
		
#define WriteEscapedByte(byte, bytemap) 				\
		if ((byte == kFlag) ||							\
		(byte == kControlEscape) ||						\
		((byte < 0x20) && ((bytemap >> byte) & 0x01)))	\
		{												\
			WriteByte(kControlEscape);					\
			WriteByte(byte ^ 0x20);						\
		}												\
		else {											\
			WriteByte(byte);							\
		}

//	printf("send LCP packet: protocol = %.4x, code = %s, id=%d, len=%d, map=%lx\n", protocol, lcpCodes[packet->code], packet->id, packet->len, map);

	/* send a flag to start new frame */
//	SerialWriteCharSync(kMODEM_PORT, kFlag);

	PPPoutgoingBufferp = gPPPoutgoingBuffer;
	WriteByte(kFlag);
	
	oldFcs = kPPPInitFCS;

	/* Send the address field ($FF) unless address compression is on */
	if ((localOptions & kAddressFieldCompressMask) == 0)
		{
		/* send address field and frame type */
		outByte = kAllCallAddress;
		oldFcs = ComputeFCS(oldFcs, &outByte, 1);
//		SerialWriteCharSync(kMODEM_PORT, outChar);
		WriteByte(outByte);

		outByte = kUnnumberedInfo;
		oldFcs = ComputeFCS(oldFcs, &outByte, 1);
		WriteEscapedByte(outByte, map);
		}

	/* Send the protocol field. If protocol compression is on, can send just the low byte */
	outByte = protocol >> 8;
	if (((localOptions & kProtocolFieldCompressMask) == 0) || (outByte != 0))
		{
		/* must send high-order byte of protocol field */
		oldFcs = ComputeFCS(oldFcs, &outByte, 1);
		WriteEscapedByte(outByte, map);
		}

	outByte = protocol & 0xff;
	oldFcs = ComputeFCS(oldFcs, &outByte, 1);
	WriteEscapedByte(outByte, map);

	/* compute FCS for infomation in frame */
	oldFcs = ComputeFCS(oldFcs, pkt, length);

	/* output the characters in the buffer */
	i = 0;
	while (i<length) {
		WriteEscapedByte(pkt[i], map);
		i++;
	}

	/* terminate the frame */
	oldFcs ^= 0xffff;
	outByte = oldFcs & 0xff;
	WriteEscapedByte(outByte, map);

	outByte = oldFcs >> 8;
	WriteEscapedByte(outByte, map);
//	SerialWriteCharSync(kMODEM_PORT, kFlag);
	WriteByte(kFlag);

	SerialWriteSync(kMODEM_PORT, (char *)gPPPoutgoingBuffer, outByteCount);
	
#undef WriteByte
#undef WriteEscapedByte
	}

static void
ConfigureRequest(ushort protocol, uchar *pkt, ushort length)
	{
	Boolean 	reject = false;
	LCPPacket 	*packet = (LCPPacket*)pkt;
	ushort		localLength = length - 4;
	uchar 		*optPtr = pkt + 4;	/* skip cnfReq packet header (FF03 x021) */
#if	0
	static const char *LOptTypes[] = { 0, "max_rcv:", "cc_map:", "auth_prot:", "qual_prot:",
										"magic_num:","RESERVED", "prot_compress", "addr_compress"};
	static const char *NOptTypes[] = { 0, "ip_addrs", "ip_compr:", "ip_addr:"};
#endif
	while (localLength > 0)
		{
		ushort optLength;
		uchar  optType;

		optType = *optPtr++;
		optLength = *optPtr++;
		localLength -= optLength;
		if (optLength > length)
			{
			/* format problem, return error */
			Message(("bad LCP length: %d", optLength));
			break;
			}

		if (protocol == kLinkControlProtocol)
			switch (optType)
				{
				case kLCPMaxRecvUnit:
					GetShortFromStream(gMRU, optPtr);
					break;

				case kLCPCharMap:
					/* accept control character map. When negotiation complete, we set our txCharMap to this */
					GetLongFromStream(gRemoteCharMap, optPtr);
					break;

				case kLCPProtCompress:
					/* accept option and change PPPServer configuration */
					gRemoteOptions |= kProtocolFieldCompressMask;
					break;

				case kLCPAddrCompress:
					/* accept option and change PPPServer configuration */
					gRemoteOptions |= kAddressFieldCompressMask;
					break;

				case kLCPAuthProt:
				case kLCPQualityProt:
				case kLCPMagicNumber:
				default:
					break;
				}
		else
			switch (optType)
				{
				ulong ipn;
				case kIPCPAddress:
					GetLongFromStream(ipn, optPtr);
					Message(("PPP: servers ip addr = 0x%lx", ipn));
					break;
				case kIPCPCompress:
					break;
				case kIPCPAddresses:	/* this is obsolete but we must handle it */
				default:
					break;
				}

			optPtr += optLength - 2;	/* bump to next entry, but already added in type and length bytes */
			}

	if (reject)
		{
		/* some problem, reject configuration request */
		packet->code = kConfigureNak;
		WritePDU(protocol, pkt, length);
		}
	else
		{
		/* advance to new state after config_req event */
		switch (gState) 
			{
			case req_sent:
				gState = ack_sent;
				break;
			case ack_rcvd:
				gState = opened;
				break;
			case ack_sent:
				/* state = ack_sent, stay in same state. */
				break;
			case opened:
				gState = ack_sent; 
				break;
			default:
				Message(("unhandled state %ld in config req", gState));
				break;
			}

		packet->code = kConfigureAck;
		WritePDU(protocol, pkt, length);					/* send ack */
		}
	}

Boolean GetPPPConnected()
{
	return gPPPConnected;
}

void SetPPPConnected(Boolean state)
{
	gPPPConnected = state;
}

void
TCPIdle(Boolean multi)
{
#ifdef HARDWARE
	MonitorCD();
#endif /* HARDWARE */

	if (GetPPPConnected() && (gIPState != 0))
		{
			if (multi) {
				while (readFrame())
					ipMonitor(gIPState);
			}
			else {
				readFrame();
				ipMonitor(gIPState);
			}
				
		}
}

long nowstartppp(void)
{
	Boolean status;

	initPPP();						/* reset ppp state vars */
	status = NegotiateLCP(0, 0);
	SetPPPConnected(status);
	return status;
}
