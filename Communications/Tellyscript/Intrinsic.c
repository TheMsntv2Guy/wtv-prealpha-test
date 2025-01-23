/* ===========================================================================
	Intrinsic.c

	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#include "Headers.h"
#include "MemoryManager.h"

#ifndef __TELLYDEFS_H__
#include "TellyDefs.h"
#endif
#ifndef __FORMATSTRING_H__
#include "FormatString.h"
#endif
#ifndef __INTERPRETER_H__
#include "Interpreter.h"
#endif
#ifndef __SERIAL_H__
#include "Serial.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __TELLYIO_H__
#include "TellyIO.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#ifdef HARDWARE
#include "HWRegs.h"
#include "Interrupts.h"
#endif

#include "PageViewer.h"


/* ===========================================================================
	globals/
=========================================================================== */

TellyProgress gScriptProgress;
ConnectionStats	gConnectionStats;
extern volatile ulong gSystemTicks;
long nowstartppp(void);

/* ===========================================================================
	static functions
=========================================================================== */
static long SI_setprogress(long params[]);
static long SI_setconnectionstats(long params[]);
static long SI_beep(long params[]);
static long SI_new(long params[]);
static long SI_free(long params[]);
static long SI_strcat(long params[]);
static long SI_strcmp(long params[]);
static long SI_strcpy(long params[]);
static long SI_atoi(long params[]);	/* Convert string to integer */
static long SI_itoa(long params[]);	/* Convert string to integer */
static long SI_display(long params[]);
static long SI_format(long params[]);
static long SI_ticks(long params[]);
static long SI_time(long params[]);
static long SI_delay(long params[]);
static long SI_idle(long params[]);
static long SI_waitfor(long params[]);
static long SI_getbytes(long params[]);
static long SI_getline(long params[]);
static long SI_sendbyte(long params[]);
static long SI_sendstr(long params[]);
static long SI_flush(long params[]);
static long SI_drain(long params[]);
static long SI_countpending(long params[]);
static long SI_setdtr(long params[]);
static long SI_getdtr(long params[]);
static long SI_oncarrier(long params[]);
static long SI_setbaud(long params[]);
static long SI_getbaud(long params[]);
static long SI_setformat(long params[]);
static long SI_getformat(long params[]);
static long SI_setflowcontrol(long params[]);
static long SI_getflowcontrol(long params[]);
static long SI_vers(long params[]);
static long SI_playmidi(long params[]);
static long SI_startppp(long params[]);
static long SI_enablemodem(long params[]);
static long SI_disablemodem(long params[]);
static long SI_getphonesettings(long params[]);
static long SI_notimplemented(long params[]);

// EMAC: added so this can work with the bfe bootrom
static long SI_noop(long params[]);
static long SI_nonsense(long params[]);

/* ===========================================================================
	implementation
 =========================================================================== */

static long SI_getphonesettings(long params[])
{
#pragma unused (params)
	return (long)GetPhoneSettings();
}

/* ----- SetProgress() -------------------------------------------------------- */

static long SI_setprogress(long params[])
{
	gScriptProgress = (TellyProgress)params[0];
	return 0;
}

/* ----- SetConnectionStats() -------------------------------------------------------- */

static long SI_setconnectionstats(long params[])
{
	gConnectionStats.dterate = params[0];
	gConnectionStats.dcerate = params[1];
	gConnectionStats.protocol = params[2];
	gConnectionStats.compression = params[3];
	return 0;
}

/* ----- beep() -------------------------------------------------------- */

static long SI_beep(long params[])
{
#pragma unused(params)
#ifdef FOR_MAC
	SysBeep(1);
#else
	PostulateFinal(false);		/* TODO: for hardware */
#endif /* FOR_MAC */
	return 0;
}

/* ----- string = new(size) -------------------------------------------- */

static long SI_new(long params[])
{
	return (long)AllocateMemory(params[0]);
}

/* ----- free(string) -------------------------------------------------- */

static long SI_free(long params[])
{
	FreeMemory((char *)params[0]);
	return 0;
}

/* ----- result = strcat(s1, s2) --------------------------------------- */

static long SI_strcat(long params[])
{
	register Byte *s1 = (Byte *)params[0];
	register Byte *s2 = (Byte *)params[1];
	
	strcat((char *) s1, (char *) s2);
	return (long) s1;
}

/* ----- result = strcmp(s1, s2) --------------------------------------- */

static long SI_strcmp(long params[])
{
	register Byte *s1 = (Byte *)params[0];
	register Byte *s2 = (Byte *)params[1];

	while (tolower(*s1) == tolower(*s2)) {
		if (!*s1)
			break;
		s1++;
		s2++;
	}
	return (long)(tolower(*s1) - tolower(*s2));
}

/* ----- result = strcpy(s1, s2) --------------------------------------- */

static long SI_strcpy(long params[])
{
	register Byte *s1 = (Byte *)params[0];
	register Byte *s2 = (Byte *)params[1];

	strcpy((char *) s1, (char *) s2);
	return (long) s1;
}

/* ----- i = atoi(str) -------------------------------------------------- */

static long SI_atoi(long params[])	/* Convert string to integer */
{
	return atoi((char *) params[0]);
}

/* ----- str = itoa(i) -------------------------------------------------- */

static long SI_itoa(long params[])	/* Convert string to integer */
{
static char s[256];
	sprintf(s, "%ld", params[0]);
	return (long) s;
}

/* ----- result = display(template, ...) ------------------------------- */

static long SI_display(long params[])
{
	Byte s[256];

	if (params[0]) {
		SFormatString(s, (Byte *)params[0], &params[1]);
		Message(((char *)s));
	} else {			/* No formatting */
		Message(((char *)params[1]));
	}
	return 0;
}

/* ----- format(string, template, ...) --------------------------------- */

static long SI_format(long params[])
{
	SFormatString((Byte *)params[0], (Byte *)params[1], &params[2]);
	return 0;
}

/* ----- ticks() ------------------------------------------------------- */

static long SI_ticks(long params[])
{
#pragma unused(params)
	return Now();
}

/* ----- time() -------------------------------------------------------- */

static long SI_time(long params[])
{
#pragma unused(params)
	long seconds = 0;

#ifdef FOR_MAC
	GetDateTime((unsigned long *)&seconds);
#else
	PostulateFinal(false);	/* need equiv for the embedded system */
#endif

	return seconds;
}

/* ----- delay(int) ---------------------------------------------------- */

static long SI_delay(long params[])
{
	ulong timeout = params[0];
	ulong base = Now();

	while (Now() - base < timeout)
		TellyIdle();

	return 0;
}

/* ----- idle() -------------------------------------------------------- */

static long SI_idle(long params[])
{
#pragma unused(params)

	TellyIdle();
	return 0;
}

/* ----- waitfor(string, timeout) -------------------------------------- */

static long SI_waitfor(long params[])
{
	char *buffer = (char *)params[0];
	long count = params[1];
	ulong timeout = params[2];
	ulong base = Now();
	char ch;
	short i = 0;

	while (Now() - base < timeout)
	{
		(void)SI_idle(0);
		if (SerialCountReadPending(kMODEM_PORT) > 0)
			{
			SerialReadSync(kMODEM_PORT, &ch, 1);
			if (ch == buffer[i++])
				{
				if (i >= count)
					return true;	/* done! */
				}
			else
				i = 0;				/* restart from beginning */
			}
	}	

	return false;					/* false == we didn't find substring */
}

/* ----- getbytes(buffer, count) --------------------------------------- */

static long SI_getbytes(long params[])
{
	long count = params[1];
	return SerialReadSync(kMODEM_PORT, (char *)params[0], count);
}

/* ----- int getline(buffer, buffer_size, timeout) --------------------------------------- */

static long SI_getline(long params[])
{
	char *buffer = (char *)params[0];
	long buffer_size = params[1];
	ulong timeout = params[2];
	ulong base = Now();
	char ch;
	short count = 0;

	while (Now() - base < timeout)
	{
		(void)SI_idle(0);
		if (SerialCountReadPending(kMODEM_PORT) > 0)
			{
			SerialReadSync(kMODEM_PORT, &ch, 1);
			if (ch == 0x0d)
				return count;
			buffer[count++] = ch;
			if (count == buffer_size)
				return 0;
			}
	}	

	return 0;
}

/* ----- sendbyte(byte) ------------------------------------------------ */

static long SI_sendbyte(long params[])
{
	return SerialWriteSync(kMODEM_PORT, (char *)params[0], 1);
}

/* ----- sendstr(string) ----------------------------------------------- */

static long SI_sendstr(long params[])
{
	long	len = strlen((char *)params[0]);
	return SerialWriteSync(kMODEM_PORT, (char *)params[0], len);
}

/* ----- flush() ------------------------------------------------------- */

static long SI_flush(long params[])
{
#pragma unused(params)
	char buffer[256];
	long waiting, count;

	waiting = SerialCountReadPending(kMODEM_PORT);
	while (waiting > 0)
	{
		count = MIN(waiting, 256);	/* don't overflow buffer */
		(void)SerialReadSync(kMODEM_PORT, buffer, count);
		waiting = SerialCountReadPending(kMODEM_PORT);
	}

	return 0;
}

/* ----- drain() ------------------------------------------------------- */

static long SI_drain(long params[])
{
#pragma unused(params)
	return 0;
}

/* ----- countpending() ------------------------------------------------ */

static long SI_countpending(long params[])
{
#pragma unused(params)
	return SerialCountReadPending(kMODEM_PORT);
}

/* ----- setdtr(state) ------------------------------------------------- */

static long SI_setdtr(long params[])
{
	return SerialSetDtr(kMODEM_PORT, params[0] != 0);
}

/* ----- getdtr() ------------------------------------------------------ */

static long SI_getdtr(long params[])
{
#pragma unused(params)
	return 0;
}

/* ----- oncarrier(???) ------------------------------------------------ */

static long SI_oncarrier(long params[])
{
#pragma unused(params)
	return 0;
}

/* ----- setbaud(rate) ------------------------------------------------- */

static long SI_setbaud(long params[])
{
	return SerialSetBaud(kMODEM_PORT, (short)params[0]);
}

/* ----- getbaud() ----------------------------------------------------- */

static long SI_getbaud(long params[])
{
#pragma unused(params)
	return 0;
}

/* ----- setformat(bitmap) --------------------------------------------- */

static long SI_setformat(long params[])
{
#pragma unused(params)
	return 0;
}

/* ----- getformat() --------------------------------------------------- */

static long SI_getformat(long params[])
{
#pragma unused(params)
	return 0;
}

/* ----- setflowcontrol(bitmap) --------------------------------------------- */

static long SI_setflowcontrol(long params[])
{
	FlowTypes whichtype = (FlowTypes)params[0];

	return SerialSetFlowControl(kMODEM_PORT, whichtype);
}

/* ----- getflowcontrol() --------------------------------------------------- */

static long SI_getflowcontrol(long params[])
{
#pragma unused(params)
	return 0;
}

/* ----- version() ----------------------------------------------------- */

static long SI_vers(long params[])
{
#pragma unused(params)
	return (long)kSCRIPT_VERSION;
}

/* ----- playmidi() ---------------------------------------------------- */

static long SI_playmidi(long params[])
{
#pragma unused(params)	
#ifdef BUGGY
	Song	*song = new(Song);
	song->SetAttributeStr(A_SRC, "/ROM/");
	song->SetLoopCount(loopCount);
	song->Load(&fResource);
	fSongList.Add(song);
#endif
	return 0;
}


/* ----- startppp() ---------------------------------------------------- */

static long SI_startppp(long params[])
{
#pragma unused(params)	
	return nowstartppp();
}


// EMAC: added so this can work with the bfe bootrom
static long SI_noop(long params[])
{
#pragma unused(params)	
	return 0;
}
static long SI_nonsense(long params[])
{
#pragma unused(params)	
	
	static char s[256];
	sprintf(s, "8101020304050607", params[0]);
	return (long) s;
}



static long SI_enablemodem(long params[])
{
#pragma unused(params)	
#ifdef HARDWARE
	EnableInts( kModemIntMask );
#endif
	return 0;
}

static long SI_disablemodem(long params[])
{
#pragma unused(params)	
#ifdef HARDWARE
	DisableInts( kModemIntMask );
#endif
	return 0;
}

static long SI_notimplemented(long params[])
{
#pragma unused(params)	
	return 0;
}

/* ----- Script intrinsic function table ------------------------------- */

INTRINSIC Intrinsics[] = {
	(Byte *)"beep",				(IFUNC)SI_beep,
	(Byte *)"printf",			(IFUNC)SI_display,
	(Byte *)"sprintf",			(IFUNC)SI_format,
	(Byte *)"free",				(IFUNC)SI_free,
	(Byte *)"new",				(IFUNC)SI_new,
	(Byte *)"stack",			(IFUNC)SI_stack,
	(Byte *)"setabort",			(IFUNC)SI_setabort,
	(Byte *)"strcat",			(IFUNC)SI_strcat,
	(Byte *)"strcmp",			(IFUNC)SI_strcmp,
	(Byte *)"strcpy",			(IFUNC)SI_strcpy,
	(Byte *)"atoi",				(IFUNC)SI_atoi,
	(Byte *)"itoa",				(IFUNC)SI_itoa,

	(Byte *)"version",			(IFUNC)SI_vers,
	(Byte *)"ticks",			(IFUNC)SI_ticks,
	(Byte *)"time",				(IFUNC)SI_time,
	(Byte *)"delay",			(IFUNC)SI_delay,
	(Byte *)"idle",				(IFUNC)SI_idle,
	(Byte *)"waitfor",			(IFUNC)SI_waitfor,
	(Byte *)"sendbyte",			(IFUNC)SI_sendbyte,
	(Byte *)"sendstr",			(IFUNC)SI_sendstr,
	(Byte *)"getbytes",			(IFUNC)SI_getbytes,
	(Byte *)"getline",			(IFUNC)SI_getline,
	(Byte *)"flush",			(IFUNC)SI_flush,
	(Byte *)"drain",			(IFUNC)SI_drain,
	(Byte *)"countpending",		(IFUNC)SI_countpending,
	(Byte *)"setdtr",			(IFUNC)SI_setdtr,
	(Byte *)"getdtr",			(IFUNC)SI_getdtr,
	(Byte *)"oncarrier",		(IFUNC)SI_oncarrier,
	(Byte *)"setbaud",			(IFUNC)SI_setbaud,
	(Byte *)"getbaud",			(IFUNC)SI_getbaud,
	(Byte *)"setformat",		(IFUNC)SI_setformat,
	(Byte *)"getformat",		(IFUNC)SI_getformat,
	(Byte *)"setflowcontrol",	(IFUNC)SI_setflowcontrol,
	(Byte *)"getflowcontrol",	(IFUNC)SI_getflowcontrol,
	(Byte *)"setstatus",		(IFUNC)SI_setprogress,
	(Byte *)"setconnectionstats",	(IFUNC)SI_setconnectionstats,
	(Byte *)"startppp",			(IFUNC)SI_startppp,

	(Byte *)"playmidi",			(IFUNC)SI_playmidi,

	(Byte *)"enablemodem",		(IFUNC)SI_enablemodem,
	(Byte *)"disablemodem",		(IFUNC)SI_disablemodem,

	(Byte *)"getphonesettings", (IFUNC)SI_getphonesettings,

	(Byte *)"dialdifferentnumber",	(IFUNC)SI_notimplemented,

	// EMAC: added so this can work with the bfe bootrom
	(Byte *)"getpreregnumber",			(IFUNC)SI_noop,
	(Byte *)"getsecret",			(IFUNC)SI_noop,
	(Byte *)"setwindowsize",			(IFUNC)SI_noop,
	(Byte *)"getserialnumber",			(IFUNC)SI_nonsense,
	(Byte *)"setforcehook",			(IFUNC)SI_noop,
	(Byte *)"computefcs",			(IFUNC)SI_noop,
	(Byte *)"setusername",			(IFUNC)SI_noop,
	(Byte *)"setpassword",			(IFUNC)SI_noop,
	(Byte *)"setpapmode",			(IFUNC)SI_noop,
	
	
	
	0,			0
};
