#include "WTVTypes.h"
#include "ErrorNumbers.h"
#include "Debug.h"
#include "SystemGlobals.h"

#include "HWRegs.h"
#include "Interrupts.h"
#include "BoxUtils.h"

#include "HWModem.h"
#include "boxansi.h"

#include "TellyIO.h"		/* for MonitorCD() */
#include "ppp.h"			/* for MonitorCD() */

uchar	modemRcvData[kModemRcvBufSize];
ulong 	modemRcvHead;
ulong 	modemRcvTail;

uchar	modemXmtData[kModemXmtBufSize];
ulong 	modemXmtHead;
ulong 	modemXmtTail;

ulong 	modemRcvBufDelta;	/* ¥¥¥Êmove to absolute globals */
ulong 	modemXmtBufDelta;	/* ¥¥¥Êmove to absolute globals */
							
uchar	gLSR;				/* updated by int handler on change */
uchar	gMSR;				/* updated by int handler on change */

ulong	gRTSEnableCount = 0; /* when RTS is off, count this down as chars are rcv'ed */
							 /* when it hits 0, set RTS and enable Rx ints */


#ifdef DEBUG
ulong	modemRcvBufOvf;		/* int handler incs if we run out of room in the rcv fifo */
ulong	modemXmtBufOvf;		/* buffer stuffer incs if it outruns int handler */

ulong	modemIntCnt = 0;
ulong	modemStatusIntCnt = 0;
ulong	modemLineStatusIntCnt = 0;
ulong	modemTxBufEmptyCnt = 0;
ulong	modemTxStashCnt = 0;
ulong	modemNoMoreToTxCnt = 0;
ulong 	modemRxBufFullCnt = 0;
ulong	modemRxStashCnt = 0;
ulong	modemNoIntCnt = 0;
ulong	modemCharPutCnt = 0;
ulong	modemCharGotCnt = 0;
#endif

/* as with all driver init routines, only call this with interrupts OFF. */

Error InitModem(void)
{
	Message(("Initializing modem..."));

	modemRcvHead = modemRcvTail = 0;
	modemXmtHead = modemXmtTail = 0;
	
#ifdef DEBUG
	modemRcvBufOvf = 0;
	modemXmtBufOvf = 0;
#endif	
#if 0
	*(volatile ulong *)kModemKBAckCycles = 1;			/* stretch the write pulse */
#endif	
	*(volatile ulong *)kModemLCR = kLCR_DLAB;
	*(volatile ulong *)kModemDivLSB = 0x01;				/* 0x0001 = 115,200 bps */
	*(volatile ulong *)kModemDivMSB = 0x00;

	*(volatile ulong *)kModemLCR = ( kLCR_NoParity + kLCR_8Bits + kLCR_1StopBit);

	*(volatile ulong *)kModemMCR = ( kMCR_RTS + kMCR_DTR );

	*(volatile ulong *)kModemFCR = ( k8ByteFIFO + kTxFIFOReset + kRxFIFOReset + kFIFOEnable );
	
	*(volatile ulong *)kModemIER = ( kIER_RxFullInt + kIER_MSRInt );	/* just do Rx interrupts first */

	*(volatile ulong *)kModemMCR = ( kMCR_IntEnable + kMCR_RTS + kMCR_DTR );

	gMSR = *(volatile ulong *)kModemMSR;
	gLSR = *(volatile ulong *)kModemLSR;
	
	return kNoError;
}

Boolean ModemGetCD()
{
	return gMSR & kMSR_DCD;
}

void MonitorCD()
{
#ifdef HARDWARE
        Boolean        newState;
		static Boolean gCD;

        if ((newState = ModemGetCD()) != gCD)
                {
				if (ModemGetDTR())		/* only do something if we're online */
					{
					if (newState)
							Message(("MonitorCD(): CARRIER DETECT"));
					else
							{
							Message(("MonitorCD(): NO CARRIER"));	
#ifndef GOOBER
							SetScriptResult(kTellyConnecting);
#endif
							}
					}
                gCD = newState;
                }
#endif /* HARDWARE */
}

Boolean gDataTerminalReady;

void ModemSetDTR(Boolean state)
{
	if (state)
//		*(volatile ulong *)kModemMCR = ( kMCR_IntEnable + kMCR_RTS + kMCR_DTR );
		*(volatile ulong *)kModemMCR |= kMCR_DTR;
	else
//		*(volatile ulong *)kModemMCR = ( kMCR_IntEnable + kMCR_RTS );
		*(volatile ulong *)kModemMCR &= ~kMCR_DTR;

	gDataTerminalReady = state;
	Message(("ModemSetDTR: DTR is now %s", state ? "up" : "down"));
}

Boolean ModemGetDTR(void)
{
	return gDataTerminalReady;

#ifdef BROKEN
	Boolean dtr;
	ulong dtrl;
	
	dtrl = *(volatile ulong *)kModemMCR;
	dtr = (*(volatile ulong *)kModemMCR & kMCR_DTR) != 0;

	Message(("DTR is 0x%lx (%s)", dtrl, dtr ? "up" : "down"));
	
	return dtr;
#endif
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void ModemFlush(void)
{
	ulong ints = DisableInts( kModemIntMask );
	modemRcvHead = modemRcvTail = 0;
	EnableInts( ints );
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void ModemDrain(void)
{
	ulong ints = DisableInts( kModemIntMask );
	modemXmtHead = modemXmtTail = 0;
	EnableInts( ints );
}
#endif

long ModemFIFOCount(void)
{
	long retval;
	ulong ints = DisableInts( kModemIntMask );

	retval =  (long)((modemRcvHead - modemRcvTail) & kModemRcvBufMask);
	EnableInts( ints );

	return retval;
}

/* put a char in the xmt buffer, turn on xmt interrupts */

ulong putchar_modem(int c)
{
	ulong ints;
	Boolean aboutToWrap;

#ifdef DEBUG
	modemCharPutCnt++;
#endif
	
	ints = DisableInts( kModemIntMask );

	aboutToWrap = ((modemXmtHead + 1) & kModemXmtBufMask) == modemXmtTail;
	if(aboutToWrap)
		{
#ifdef DEBUG
		modemXmtBufOvf++;
#endif

		*(volatile ulong *)kModemIER |= kIER_TxEmptyInt;	
		EnableInts( ints );
		return 1;
		}

	modemXmtHead++;
	modemXmtHead &= kModemXmtBufMask;
	modemXmtData[modemXmtHead] = c;			/* write to buf & ensure xmt ints are on */
	
	*(volatile ulong *)kModemIER |= kIER_TxEmptyInt;	
	EnableInts( ints );
	
	return 0;
}

/*  Takes a ptr to a buf and try to fill it with a desired # bytes (cnt).
	Returns # bytes actually read.
 */
 
ulong getbuf_modem(char *buf, ulong cnt)
{
	ulong mycnt, ints;
	static ulong oldCount;

	mycnt = 0;
			
	while ( (modemRcvTail != modemRcvHead) && cnt )
	{
#ifdef DEBUG
		if (modemRcvBufOvf != oldCount)
			{
			Message(("getbuf_modem: MODEM RX_BUF OVERFLOW!  Not calling TcpIdle() often/fast enough..."));
			oldCount = modemRcvBufOvf;
			}
#endif
		if (!(*(volatile ulong *)kModemMCR & kMCR_RTS))	/* has Rx int routine paused the connection? */
		{
			gRTSEnableCount--;				/* only turn Rx ints & RTS back on when we've */
											/* drained this much */
			if (!gRTSEnableCount)
			{
				*(volatile ulong *)kModemIER |= kIER_RxFullInt;			
				*(volatile ulong *)kModemMCR |= kMCR_RTS;
			}
		}

		ints = DisableInts( kModemIntMask );
	
		modemRcvTail++;
		modemRcvTail &= kModemRcvBufMask;
		buf[mycnt] = modemRcvData[modemRcvTail];
	
		EnableInts( ints );
		
#ifdef DEBUG		
		modemCharGotCnt++;
#endif
		mycnt++;		/* bump count of chars stored in buf */
		cnt--;			/* dec count of max chars wanted */
	}

	return mycnt;
}
