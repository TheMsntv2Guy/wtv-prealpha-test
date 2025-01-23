/* ===========================================================================
	Serial.h
	
	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __SERIAL_H__
#define __SERIAL_H__

typedef enum
{
	kNoFlowControl = 0x01,
	kSoftwareFlowControl = 0x02,
	kHardwareFlowControl = 0x03
} FlowTypes;

typedef enum
{
	kIR_PORT = 0x01,
	kMODEM_PORT = 0x02
} PortNums;

void SerialInitialize(PortNums whichport);
#if defined(FOR_MAC) || defined(INCLUDE_FINALIZE_CODE)
void SerialFinalize(PortNums whichport);
#endif /* defined(FOR_MAC) || defined(INCLUDE_FINALIZE_CODE) */
long SerialReadSync(PortNums whichport, char *buffer, long count);
long SerialWriteSync(PortNums whichport, char *buffer, long count);
long SerialCountReadPending(PortNums whichport);
void SerialReadAsync(PortNums whichport, struct IOParam *pb, long count, uchar *buffer, void *completion);
short SerialSetDtr(PortNums whichport, Boolean state);
short SerialSetBaud(PortNums whichport, short rate);
short SerialSetFlowControl(PortNums whichport, FlowTypes type);
void GetMacRefNums(PortNums whichport, short *inrefnum, short *outrefnum);
short AssertDrvrOpen (unsigned char name[256], short *refNum);
long SerialWriteCharSync(PortNums whichport, char c);
char SerialReadCharSync(PortNums whichport);
Boolean SerialGetDtr(PortNums whichport);

#ifdef FOR_MAC
Boolean ModemInitialized(void);
#endif

/* defines */

#define	kModemDriverInName	"\p.AIn"
#define	kModemDriverOutName	"\p.AOut"
#define	kIRDriverInName		"\p.BIn"
#define	kIRDriverOutName	"\p.BOut"

#define	kSerStatus		 8			 /* Serial Driver csCodes */
#define	kSerClrBrk		11           
#define	kSerSetBrk		12
#define	kSerHShakeDTR	14

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Serial.h multiple times"
	#endif
#endif /* __SERIAL_H__ */
