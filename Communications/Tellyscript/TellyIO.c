/* ===========================================================================
 *	TellyIO.c
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#include "Headers.h"

#include "MemoryManager.h"
#include "TellyDefs.h"
#include "TellyIO.h"
#include "Interpreter.h"
#include "Scanner.h"
#include "ObjectStore.h"
#include "Serial.h"
#include "System.h"

#ifdef APPROM

#ifndef __CLOCK_H__
#include "Clock.h"
#endif

#endif /* APPROM */

#ifdef BOOTROM
#include "FlashDownload.h"
#endif

#ifdef FOR_MAC
	#ifndef __MACINTOSH_H__
	#include "Macintosh.h"
	#endif
	#ifndef __MACINTOSHUTILITIES_H__
	#include "MacintoshUtilities.h"	/* for pseudo-malloc */
	#endif
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
	#ifndef __MEMORYWINDOW_H__
	#include "MemoryWindow.h"		/* for IdleMemoryWindow() proto */
	#endif
	#ifndef __MESSAGEWINDOW_H__
	#include "MessageWindow.h"		/* for StdWindow:: */
	#endif

static jmp_buf env;
#endif	/* FOR_MAC */

int						ScriptRef;		/* Script file reference */
static short			gScriptUnget;	/* Script file unget buffer */
static Byte*			gScriptBuffer;	/* Script file buffer */
static long				gScriptEob;		/* Size of script file buffer */
static long				gScriptMark;	/* Next position in script file buffer */
static TellyResult		gScriptResult;
static PHONE_SETTINGS	gPhoneSettings;
static ConnectionLog	gConnectionLog;

extern INTRINSIC		Intrinsics[];	/* Script intrinsic functions */
extern TellyProgress 	gScriptProgress;
extern ConnectionStats	gConnectionStats;

void
TellyIdle(void)
{
#ifdef FOR_MAC
	EventRecord		event;

	if (WaitNextEvent(everyEvent, &event, 0L, nil))
	{
		DoEvent(&event);
	}
	else
	{
		MoviesTask(nil, 0);
#ifdef DEBUG_MEMORYWINDOW
		IdleMemoryWindows();			// give them time to update
#endif
		StdWindow::IdleAll();
	}

	if (gMacSimulator->GetPendingPowerOn())
	{
		gSystem->PowerOn();
		gMacSimulator->SetPendingPowerOn(false);
		return;
	}
#endif /* FOR_MAC */

#ifdef APPROM
	gSystem->UserLoopIteration();
#endif

#ifdef BOOTROM
	FlashDownloadIdle();
#endif /* BOOTROM */
}

void SetScriptAborted()
{
	SI_setabort(0);
}

TellyProgress GetScriptProgress()
{
	return gScriptProgress;
}

long GetScriptProgressPercentage(long callCount)
{

/* callCount:

	2  == 800 number call
	1  == first call after 800 number
 <= 0  == first 415 call

*/
	switch (callCount)
		{
		case 2:								/* progress for 800 number call */
			switch (gScriptProgress)
				{
				case kTellyIdle:							return 0;
				case kTellyInitializingModem:				return 7;
				case kTellyDialing:							return 14;
				case kTellyHandshake:						return 21;
				case kTellyCarrier:							return 28;
				case kTellyLogin:							return 35;
				case kTellyNegotiatingPPP:					return 42;
				case kTellyConnected:						return 49;
				}
		case 1:								/* local POP right after 800 call */
			switch (gScriptProgress)
				{
				case kTellyIdle:							return 56;
				case kTellyInitializingModem:				return 63;
				case kTellyDialing:							return 70;
				case kTellyHandshake:						return 77;
				case kTellyCarrier:							return 84;
				case kTellyLogin:							return 91;
				case kTellyNegotiatingPPP:					return 100;
				case kTellyConnected:						return 100;
				}
		default:							/* regular call */
			switch (gScriptProgress)
				{
				case kTellyIdle:							return 0;
				case kTellyInitializingModem:				return 13;
				case kTellyDialing:							return 26;
				case kTellyHandshake:						return 39;
				case kTellyCarrier:							return 52;
				case kTellyLogin:							return 75;
				case kTellyNegotiatingPPP:					return 88;
				case kTellyConnected:						return 100;
				}
		}
	
	return -1;
}

const char* GetScriptProgressText(long callCount)
{
#ifdef FUNNY_HACK
	long rn = Now() ^ Now() >> gScriptProgress & 7;
	static char dest[32];
	long i;

	dest[0] = 0;
	
	for (i=0; i<9; i++)				/* 8 bits plus parity bit */
		if ((rn >> i) & 1)
			strcat(dest, "1");
		else
			strcat(dest, "0");

	return dest;
#endif

	switch (callCount)
		{
		case 2:								/* progress for 800 number call */
			switch (gScriptProgress)
				{
				case kTellyIdle:					return "Not connected";
				case kTellyConnected:				return "Getting local number";
				case kTellyCarrier:					return "Answered";
				case kTellyDialing:					return "Toll-free call";
				case kTellyHandshake:				return "Waiting";
				case kTellyLogin:					return "Connecting";
				case kTellyNegotiatingPPP:			return "Connecting";
				case kTellyInitializingModem:		return "Getting ready";
				}
		case 1:								/* call immediately after 800 */
			switch (gScriptProgress)
				{
				case kTellyIdle:					return "Local call";
				case kTellyConnected:				return "Connected";
				case kTellyCarrier:					return "Answered";
				case kTellyDialing:					return "Calling locally";
				case kTellyHandshake:				return "Waiting";
				case kTellyLogin:					return "Connecting";
				case kTellyNegotiatingPPP:			return "Connecting";
				case kTellyInitializingModem:		return "Getting ready";
				}
		default:							/* regular call */
			switch (gScriptProgress)
				{
				case kTellyIdle:					return "Not connected";
				case kTellyConnected:				return "Connected";
				case kTellyCarrier:					return "Answered";
				case kTellyDialing:					return "Calling";
				case kTellyHandshake:				return "Waiting";
				case kTellyLogin:					return "Connecting";
				case kTellyNegotiatingPPP:			return "Connecting";
				case kTellyInitializingModem:		return "Getting ready";
				}
		}
	
	return "";
}

void SetScriptProgress(TellyProgress status)
{
	gScriptProgress = status;
}

void SetScriptResult(TellyResult status)
{
	gScriptResult = status;
}

void SetPhoneSettings(PHONE_SETTINGS *phone)
{
	gPhoneSettings.usePulseDialing = phone->usePulseDialing;
	gPhoneSettings.audibleDialing = phone->audibleDialing;
	gPhoneSettings.disableCallWaiting = phone->disableCallWaiting;
	gPhoneSettings.dialOutsideLine = phone->dialOutsideLine;
	gPhoneSettings.changedCity = phone->changedCity;
	gPhoneSettings.waitForTone = phone->waitForTone;

	memcpy(gPhoneSettings.callWaitingPrefix, &phone->callWaitingPrefix, kMAX_DIGITS);
	memcpy(gPhoneSettings.dialOutsidePrefix, &phone->dialOutsidePrefix, kMAX_DIGITS);
	memcpy(gPhoneSettings.accessNumber, &phone->accessNumber, kMAX_DIGITS);
}

PHONE_SETTINGS *GetPhoneSettings()
{
	return &gPhoneSettings;
}

ConnectionLog *GetConnectionLog()
{
	return &gConnectionLog;
}

void AddConnectionLog(long type)
{
	long	entry;
	
	if (++gConnectionLog.entries > kMaxLogEntries)
		return;

	entry = gConnectionLog.entries - 1;
	gConnectionLog.items[entry].type = type;
#ifdef APPROM
	gConnectionLog.items[entry].GMT = gClock->GetDateTimeGMT();
#endif
}

void SendConnectionLog()
{
	if (gConnectionLog.entries > 0)
		{
		Message(("Sending connection logs to service"));
#ifdef APPROM
		SendLog("ConnectEvent", (const char *)&gConnectionLog, sizeof(gConnectionLog), nil, 0);
#endif
		gConnectionLog.entries = 0;
		}
}

TellyResult GetScriptResult(void)
{
	return gScriptResult;
}

ConnectionStats* GetConnectionStats()
{
	return &gConnectionStats;
}

#ifdef SIMULATOR
void NewScript(uchar *data, long length)
{
	gScriptUnget = -1;			/* No unget character yet */
	gScriptMark = 0;			/* start at beginning */
	gScriptBuffer = data;
	gScriptEob = length;		/* when to stop reading */
}
#endif

TellyResult RunScript(const char* data, long length)
{
	TellyResult	err;
	Byte*		heap;
	
	SetScriptResult(kTellyConnecting);
	SetScriptProgress(kTellyIdle);

	heap = (Byte*)AllocateMemory(SCRIPTMAX);
	memcpy(heap, data, length);
	SetupPointers(heap, SCRIPTMAX, heap + length);
	AddIntrinsics(Intrinsics);
	LinkFunctions();

#ifdef FOR_MAC
	if (setjmp(env) == 0)
#else
	if (1) 
#endif
	{
		err = (TellyResult)SI_Interpret();	/* Call main() in script */
	} else {					/* Come here after error */
		err = kTellyParseError;
	}

	SetScriptProgress(kTellyIdle);
	SetScriptResult(err);

	if (heap != nil)
		FreeMemory((char *)heap);

	return err;
}

/* ----- Functions called by interpreter ------------------------------- */

void SI_Error(register short erno, register Byte *s, register short line)
{
	if (*s) {
		printf("ERROR #%d: Line %d: %s\n", erno, line, s);
	} else
		printf("ERROR #%d: Line %d\n", erno, line);

#ifdef FOR_MAC
	longjmp(env, 0);	/* Return to Script(), ignore link chain */
#endif
}

#ifdef SIMULATOR
Byte SI_GetSource(void)
{
	register Byte c;
	if (gScriptUnget >= 0) {			/* The was a previous unget */
		c = gScriptUnget;			/* So use the unget buffer */
		gScriptUnget = -1;			/* Unget buffer is empty now */
		return c;
	}
	if (gScriptMark < gScriptEob) {		/* Can still use buffer */
		c = gScriptBuffer[gScriptMark++];
		return c;
	}
	else
		return -1;					/* EOF */
}

void SI_UngetSource(register Byte c)
{
	gScriptUnget = c;				/* That's easy */
}
#endif