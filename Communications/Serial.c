// ===========================================================================
//	Serial.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================
#include "Headers.h"

#ifndef __SERIAL_H__
#include "Serial.h"
#endif

#ifdef FOR_MAC
	#ifndef __SERIAL__
	#include <serial.h>
	#endif
#endif

#ifdef HARDWARE
	#include "HWModem.h"
#endif

// ===========================================================================
//	globals/#defines/externs
// ===========================================================================

#define kSerConfig baud19200 + noParity + data8 + stop10

short	gIROutRefNum;
short	gIRInRefNum;
short	gModemOutRefNum;
short	gModemInRefNum;
Boolean gModemInitialized;

// ===========================================================================
//	implementations
// ===========================================================================

#ifdef FOR_MAC
Boolean ModemInitialized()
{
	return gModemInitialized;
}

void GetMacRefNums(PortNums whichport, short *inrefnum, short *outrefnum)
{
	switch (whichport)
	{
		case kIR_PORT:
			*outrefnum = gIROutRefNum;
			*inrefnum = gIRInRefNum;
			break;
		case kMODEM_PORT:
			*outrefnum = gModemOutRefNum;
			*inrefnum = gModemInRefNum;
			break;
		default:
			Message(("GetMacRefNums:  Bogus port = %d", whichport));
	}

}
#endif /* FOR_MAC */

#ifdef SIMULATOR
void SerialInitialize(PortNums whichport)
{
 #ifdef FOR_MAC
    OSErr   openOutErr, openInErr;
    OSErr   setCfgErr, setHskErr;
    SerShk  hskFlags;
    Boolean takeOverPort = true;
    Boolean openAOut, openAIn;
	short	inRefNum, outRefNum;
	unsigned char *inName;
	unsigned char *outName;
	
	Message(("InitializeSerial called."));

	switch (whichport)
	{
		case kIR_PORT:
			inName = kIRDriverInName;
			outName = kIRDriverOutName;
			break;
		case kMODEM_PORT:
			inName = kModemDriverInName;
			outName = kModemDriverOutName;
			break;
		default:
			Message(("ReadSerialAsync:  Bogus port = %d", whichport));
	}

    openAOut = AssertDrvrOpen(outName, &outRefNum) == noErr;
    openAIn = AssertDrvrOpen(inName, &inRefNum) == noErr;
    if (openAOut || openAIn) 
    {
    	Message(("Serial Port in use, resetting..."));
        if (openAIn) 
		{
			KillIO(inRefNum);
			CloseDriver(inRefNum);
		}
		if (openAOut) 
		{
			KillIO(outRefNum);
			CloseDriver(outRefNum);
		}
	}
	
	openOutErr = OpenDriver(outName, &outRefNum);
	openInErr = OpenDriver(inName, &inRefNum);

	if (openOutErr == noErr && openInErr == noErr) 
	{
		char *buf;

		/* Work around a serial driver bug on some macs (7500) under 7.5.2 */
		/* Subsequent reads fail with error -15128 when the internal 64 char buffer fills up */

		buf = (char *)NewPtr(32768);
		
		if ( buf == nil )
			Complain(("Can't get memory for serial port buffer"));
		else
			(void)SerSetBuf(inRefNum, buf, 32768);
	
		hskFlags.fXOn = false;
		hskFlags.fCTS = false;
		hskFlags.xOn = 0x11;
		hskFlags.xOff = 0x13;
		hskFlags.errs = 0;
		hskFlags.evts = 0;              /* assumes we're running >= Sys 7 */
		hskFlags.fInX = false;
		hskFlags.fDTR = false;

		setHskErr = SerHShake(outRefNum, &hskFlags);

		/* Now reset both input and output drivers with the same configuration.  */
		/* Only a single call to the output driver is necessary to do this.      */
		/* Differing concurrent input/output baud rates are not supported.       */

		setCfgErr = SerReset(outRefNum, kSerConfig);
		
		switch (whichport)
		{
			case kIR_PORT:
				gIROutRefNum = outRefNum;
				gIRInRefNum = inRefNum;
				break;
			case kMODEM_PORT:
				gModemOutRefNum = outRefNum;
				gModemInRefNum = inRefNum;
				gModemInitialized = true;
				break;
			default:
				Message(("ReadSerialAsync:  Bogus port = %d", whichport));
		}

		/* disable DTR on both ports.  make them raise it if they need it. */
		SerialSetDtr(whichport, false);
	}
	else
		Message(("Can't open serial port!\n"));
#else
	#pragma unused(whichport)
	PostulateFinal(false);			/* TODO: implement for hardware */
	ModemSetDTR(false);
#endif /* FOR_MAC */
}
#endif

#if defined(FOR_MAC) || defined(INCLUDE_FINALIZE_CODE)
void SerialFinalize(PortNums whichport)
{
 #ifdef FOR_MAC
    OSErr   killErr, closeOutErr, closeInErr;
	short	inRefNum, outRefNum;

	Message(("FinalizeSerial called."));

	GetMacRefNums(whichport, &inRefNum, &outRefNum);

    killErr = KillIO(inRefNum);
    closeInErr = CloseDriver(inRefNum);

    killErr = KillIO(outRefNum);
    closeOutErr = CloseDriver(outRefNum);

	switch (whichport)
		{
		case kMODEM_PORT:
			gModemInitialized = false;
		}
#else
	#pragma unused(whichport)
	PostulateFinal(false);		/* implement for embedded sys */
#endif /* FOR_MAC */
}
#endif /* defined(FOR_MAC) || defined(INCLUDE_FINALIZE_CODE) */

#ifdef SIMULATOR
char SerialReadCharSync(PortNums whichport)
{
#ifdef FOR_MAC
	short	outRefNum, inRefNum;
	IOParam pb;
	OSErr err;
	char c;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);

	pb.ioRefNum = inRefNum;
	pb.ioBuffer = (char *)&c;
	pb.ioReqCount = 1;
	
	err = PBReadSync((ParmBlkPtr)&pb);
	return c;	
#else
	#pragma unused(whichport)
	char c;

	while(getbuf_modem(&c, 1) == 0)
		;
	return c;
#endif	/* FOR_MAC */
}
#endif

long SerialReadSync(PortNums whichport, char *buffer, long count)
{
#ifdef FOR_MAC
	short	outRefNum, inRefNum;
	IOParam pb;
	OSErr err;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);

	pb.ioRefNum = inRefNum;
	pb.ioBuffer = (char *)buffer;
	pb.ioReqCount = count;
	
	err = PBReadSync((ParmBlkPtr)&pb);
	return pb.ioActCount;	
#else
	#pragma unused(whichport)
	ulong got = 0;

	while(got < count) 
	{
		got += getbuf_modem(&buffer[got], count - got);
	}

	return got;
#endif	/* FOR_MAC */
}

#ifdef SIMULATOR
void SerialReadAsync(PortNums whichport, IOParam *pb, long count, uchar *buffer, void *completion)
{
#ifdef FOR_MAC
	short inRefNum, outRefNum;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);

	pb->ioCompletion = (IOCompletionUPP)completion;
	pb->ioVRefNum = 0;
	pb->ioRefNum = inRefNum;
	pb->ioVersNum = 0;
	pb->ioPermssn = 0;
	pb->ioMisc = 0;
	pb->ioBuffer = (char *)buffer;
	pb->ioReqCount = count;
	pb->ioActCount = 0;
	pb->ioPosMode = fsFromStart;
	pb->ioPosOffset = 0;
	
	PBReadAsync((ParmBlkPtr)pb);
#else
#pragma unused (whichport, pb, count, buffer, completion)
	PostulateFinal(false);		/* implement for embedded sys */
#endif /* FOR_MAC */
}
#endif

long SerialWriteSync(PortNums whichport, char *buffer, long count)
{
#ifdef FOR_MAC
	short	inRefNum, outRefNum;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);

	(void)FSWrite(outRefNum, &count, buffer);
	return count;	
#else
	#pragma unused(whichport)
	ulong sent = 0;
	while(sent < count)
	{
		if(!putchar_modem(buffer[sent]))
			sent++;
	}

	return sent;
#endif	/* FOR_MAC */
}

#ifdef SIMULATOR
long SerialWriteCharSync(PortNums whichport, char c)
{
#ifdef FOR_MAC
	short	inRefNum, outRefNum;
	long count = 1;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);

	(void)FSWrite(outRefNum, &count, &c);
	return count;	
#else
	#pragma unused(whichport)

	while(putchar_modem(c));
	return 1;
#endif	/* FOR_MAC */
}
#endif

long SerialCountReadPending(PortNums whichport)
{
#ifdef FOR_MAC
	long	count;
	short	inRefNum, outRefNum;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);
	(void)SerGetBuf(inRefNum, &count);
	return count;	
#else
	return ModemFIFOCount();
#endif	/* FOR_MAC */
}

short SerialSetFlowControl(PortNums whichport, FlowTypes whichtype)
{
#ifdef FOR_MAC
	SerShk	flags = {0, 0, 0x11, 0x13, 0, 0, 0, 0};
	short	inRefNum, outRefNum;
	CntrlParam		pb;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);

	switch (whichtype) {
		case kNoFlowControl:
			flags.fXOn = 0;
			flags.fCTS = 0;
			flags.fInX = 0;
			break;
		case kSoftwareFlowControl:
			flags.fXOn = true;
			flags.fCTS = 0;
			flags.fInX = 0;
			break;
		case kHardwareFlowControl:
			flags.fXOn = 0;
			flags.fCTS = true;
			flags.fInX = 0;
			break;
		default:
			flags.fXOn = 0;			/* assume no flow control */
			flags.fCTS = 0;
			flags.fInX = 0;
			Complain(("Unknown enum passed to setflowcontrol()"));
	}

	pb.ioCRefNum = outRefNum;
	pb.csCode = 14;
	pb.csParam[0] = flags.fXOn;
	pb.csParam[1] = flags.fCTS;
	pb.csParam[2] = flags.xOn;
	pb.csParam[3] = flags.xOff;
	pb.csParam[4] = flags.errs;
	pb.csParam[5] = flags.evts;
	pb.csParam[6] = flags.fInX;
	pb.csParam[7] = flags.fDTR;

	return (short)PBControl((ParmBlkPtr) &pb, false);
#else
	PostulateFinal(false);			/* TODO: fix for hardware */
	return 0;
#endif 	/* FOR_MAC */
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean SerialGetDtr(PortNums /*whichport*/)
{
#ifdef HARDWARE
	return ModemGetDTR();
#else
	return 0;
#endif	/* HARDWARE */
}
#endif

short SerialSetDtr(PortNums whichport, Boolean state)
{
#ifdef FOR_MAC
	short			inRefNum, outRefNum;
	CntrlParam		pb;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);
	
	pb.ioCRefNum = outRefNum;
	pb.csCode = state ? 17 : 18;
	return (short)PBControl((ParmBlkPtr) &pb, false);
#else
	#pragma unused(whichport)	
	ModemSetDTR(state);
	return 0;
#endif /* FOR_MAC */
}

short SerialSetBaud(PortNums whichport, short rate)
{
#ifdef FOR_MAC
	short			inRefNum, outRefNum;
	CntrlParam		pb;

	GetMacRefNums(whichport, &inRefNum, &outRefNum);
	
	pb.ioCRefNum = outRefNum;
	pb.csCode = serdSetBaud;
	pb.csParam[0] = rate;
	return (short)PBControl((ParmBlkPtr) &pb, false);
#else
	PostulateFinal(false);			/* TODO: fix for hardware */
	return 0;
#endif /* FOR_MAC */
}

/* ----- Serial Utils --------------------------------------------------------- */

#ifdef FOR_MAC

#define UTableBase		0x11C		/*[GLOBAL VAR] Base address of unit table unit I/O table [pointer]*/
#define UnitNtryCnt		0x1D2		/*[GLOBAL VAR]  count of entries in unit table [word]*/
#define	drvrName		0x12		/* offset to driver name in 'DRVR' std. header */

OSErr AssertDrvrOpen (Str255 name, short *refNum)
{
    DCtlHandle  *pUTEntry;
    Ptr         pDrvr;
    OSErr       result = notOpenErr;        /* assume not open */
    short       unitNo;
    char        *aDrvrName;

    /* The point here is to determine whether a driver is open, given its name.  */
    /* This allows one to check a driver to see if it's open without hard coding */
    /* its reference number. (Normally, the way to get the refNum is to open     */
    /* the driver--but that defeats the whole purpose!)                          */
    /* This is an extension of the code discussed in Tech Note #71.              */

    *refNum = 0;
    pUTEntry = *(DCtlHandle **) UTableBase;
    for (unitNo = 0; unitNo < *(short *) UnitNtryCnt; unitNo++, pUTEntry++) 
    {
        if (*pUTEntry != nil && **pUTEntry != nil) 
        {
            if (((***pUTEntry).dCtlFlags & (1 << dRAMBased)) != 0)
                pDrvr = *(Handle) (***pUTEntry).dCtlDriver;
            else
                pDrvr = (***pUTEntry).dCtlDriver;

            if (pDrvr != nil) 
            {
                aDrvrName = pDrvr + drvrName;
                if (memcmp(aDrvrName, name, 1 + name[0]) == 0) 
                {
                    /* We found the one we're after. */
                    *refNum = ~unitNo;
                    if (((***pUTEntry).dCtlFlags & 1 << dOpened) != 0)
                        result = noErr;
                    break;
                }
            }
        }
    }

    return result;
}
#endif /* FOR_MAC */
