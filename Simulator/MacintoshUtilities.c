// ===========================================================================
//	MacintoshUtilities.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MACINTOSH_H__
#include "Macintosh.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif
#ifndef __INPUT_H__
#include "Input.h"
#endif

#include <stdio.h>

// ===========================================================================

/*
Display an alert to inform the user of an error.  MessageID acts as an
index into a STR# resource of error messages.  

BUG NOTE: GetIndString will return a bogus string if the index is not
positive.
*/

static void
MyNumToString(short number, Str255 str)
	{
	char	*pch = (char*) &str[1];
	
	str[0] = 0;
	if (number < 0)
	{
		str[0]++; *pch++ = '-';
		number = -number;
	}
	
	if (number > 99999)
		{ BlockMove("99999", pch, 5); str[0] += 5; return; }
	
	if (number > 9999)
		{ *pch++ = number / 10000; str[0]++; number /= 10; }
	if (number > 999)
		{ *pch++ = number / 1000; str[0]++; number /= 10; }
	if (number > 99)
		{ *pch++ = number / 100; str[0]++; number /= 10; }
	if (number > 9)
		{ *pch++ = number / 10; str[0]++; number /= 10; }
	*pch++ = number; str[0]++;
	}

extern short gAppResRef;	// defined in Macintosh.c

/*
#pragma segment Main
void AlertUser(short error, short messageID)
{
	Str255	msg1;
	Str255	msg2;
	short	theItem;

	short prevResFile = CurResFile();	// save current resource file
	UseResFile(gAppResRef);				// use global resource file

	if (messageID > 0)
		GetIndString(msg1, sErrStrings, messageID);
	else
		msg1[0] = 0;							// case there’s no message
	
	if ((error <= noHardware) && (error >= badFormat))
		GetIndString(msg2, sSMErrStrings, ABS(error) + noHardware + 1);
	else
		MyNumToString(error, msg2);
	
	ParamText(msg1, msg2, "\p", "\p");
	theItem = Alert(rUserAlert, nil);
	
	UseResFile(prevResFile);			// restore current resource file
}
 */
 
void MacAlert(char *msg)
{
	paramtext(msg, "", "", "");
	(void)Alert(rLargeUserAlert, nil);
}

/*
Display an alert that tells the user an error occurred, then exit the
program.  This routine is used as an ultimate bail-out for serious errors
that prohibit the continuation of the application.  Errors that do not
require the termination of the application are handled with AlertUser.
Error checking and reporting has a place even in the simplest application.

BUG NOTE: GetIndString will return a bogus string if the index is not
positive.
*/

#pragma segment Main
void EmergencyExit(short message)
{
	Str255	msg;
	short	theItem;

	SetCursor(&qd.arrow);
	if (message > 0)
		GetIndString(msg, sErrStrings, message);
	else
		msg[0] = 0;								// case there’s no message
	ParamText(msg, "\p", "\p", "\p");
	theItem = Alert(rExitAlert, nil);
	ExitToShell();								//gotta go!
}


//====================================================================================
//	Get memory where ever we can find it, including MFTempMem

const ulong		kTempHdlOffsetToPtr = 16;

Ptr NewSimulatorMemory(ulong amount, Boolean shouldClear)
{
	OSErr	err;
	Ptr		p;
	Handle	hdl;

	if (NGetTrapAddress(0xA88F,OSTrap) != NGetTrapAddress(0xA89F,OSTrap))
	{
		hdl = MFTempNewHandle((Size)(amount+16), &err);
		if (hdl != nil)
		{
			MFTempHLock(hdl, &err);
			
			p = StripAddress(*hdl);
			*(long *)p = 'Joe ';
			*(long *)(p+4) = 'Brit';
			*(long *)(p+8) = (long)h;
			*(long *)(p+12) = 0;
			
			if (shouldClear)
				memset(p + kTempHdlOffsetToPtr, 0, amount);
				
			return p + kTempHdlOffsetToPtr;
		}
	}
	
	return shouldClear ? NewPtrClear(amount) : NewPtr(amount);
}

void DisposeSimulatorMemory(Ptr ptr)
{
	OSErr		err;
	
	if ((Ptr)ApplicationZone() <= ptr && ptr < (Ptr)GetApplLimit())
		DisposePtr(ptr);
	else
		MFTempDisposHandle(RecoverHandle(ptr - kTempHdlOffsetToPtr), &err);
}

//====================================================================================
//	List of unique names...would be better to have some sort of reference counted
//  string store...or at least keep 'em in a binary tree instead of a linked list.

struct NameEntry {
	NameEntry*	next;
	char		name[4];
};
static NameEntry* gUniqueNames = nil;

const char* UniqueName(const char* newName)
{
	NameEntry* nameEntry;
	
	for (nameEntry = gUniqueNames; nameEntry != nil; nameEntry = nameEntry->next) {
		if (strcmp(newName, nameEntry->name) == 0)
			return nameEntry->name;
	}
	
	nameEntry = (NameEntry*)NewSimulatorMemory(sizeof(*nameEntry) -  sizeof(nameEntry->name) + strlen(newName) + 1, false);
	Assert(nameEntry != nil);
	
	// fill in the data and add him to head of list
	strcpy(nameEntry->name, newName);
	nameEntry->next = gUniqueNames;
	gUniqueNames = nameEntry;
	
	return nameEntry->name;
}

//====================================================================================
//	Generate a unique filename using this filenamePrefix (similar to LISP's gensym)

static char gDumpFilename[33];

const char* GenerateDumpFilename(const char* filenamePrefix)
{
	if (filenamePrefix == nil) {
		filenamePrefix = "WebTV dumpfile";
	}
	
	size_t filenamePrefixLength =
				snprintf(gDumpFilename, sizeof(gDumpFilename), "%.*s",
			 			 sizeof(gDumpFilename) - 4 - 1, // four digits plus NULL character
						 filenamePrefix);
	
	// come up with the next filename for a stat dump
	int filenum = 0;
	char* thousandsDigit = &(gDumpFilename[filenamePrefixLength]);
	char* hundredsDigit  = &(gDumpFilename[filenamePrefixLength + 1]);
	char* tensDigit      = &(gDumpFilename[filenamePrefixLength + 2]);
	char* onesDigit      = &(gDumpFilename[filenamePrefixLength + 3]);
	gDumpFilename[filenamePrefixLength + 4] = 0; // make sure it's null-terminated
	
	*thousandsDigit = '0';	// thousands digit = zero
	*hundredsDigit = '0';	// hundreds digit = zero
	*tensDigit = '0';		// tens digit = zero
	*onesDigit = '0';		// ones digit = zero
	
	FILE* fp = nil;
	
	while ((fp = fopen(gDumpFilename, "r")) != nil)
	{
		fclose(fp);
		filenum++;
		if ((filenum % 1000) == 0)
		{
			(*thousandsDigit)++;
			*hundredsDigit = '0';
			*tensDigit = '0';
			*onesDigit = '0';
		}
		else if ((filenum % 100) == 0)
		{
			(*hundredsDigit)++;
			*tensDigit = '0';
			*onesDigit = '0';
		}
		else if ((filenum % 10) == 0)
		{
			(*tensDigit)++;
			*onesDigit = '0';
		}
		else
		{
			(*onesDigit)++;
		}
	}
	return gDumpFilename;
}

//====================================================================================
//	Helper routines for graphic stuff.

void SetColor(short r, short g, short b)
{
	RGBColor	rgb;
	rgb.red = r << 8;
	rgb.green = g << 8;
	rgb.blue = b << 8;
	RGBForeColor(&rgb);
}

void SetBackColor(short r, short g, short b)
{
	RGBColor	rgb;
	rgb.red = r << 8;
	rgb.green = g << 8;
	rgb.blue = b << 8;
	RGBBackColor(&rgb);
}

void SetGray(short gray)
{
	RGBColor	rgb;
	rgb.red = rgb.green = rgb.blue = gray << 8;
	RGBForeColor(&rgb);
}

void SetBackGray(short gray)
{
	RGBColor	rgb;
	rgb.red = rgb.green = rgb.blue = gray << 8;
	RGBBackColor(&rgb);
}

static short GetWTitleHeight(short variant)
{
	FontInfo info;
	GrafPtr curGraf;
	GrafPtr wMgrPort;
	short wTitleHeight;
	short result;

	if (	(variant == documentProc)
		 || (variant == noGrowDocProc)
		 || (variant == zoomDocProc)
		 || (variant == rDocProc) )
	{
		GetPort(&curGraf);
		GetWMgrPort(&wMgrPort);						//I need to know the font...
		SetPort(wMgrPort);							//info in the System’s port
		GetFontInfo(&info);
		SetPort(curGraf);							//restore current port
		wTitleHeight = info.ascent + info.descent + info.leading + 2;
		if (wTitleHeight < kMinWindowTitleHeight)
			wTitleHeight = kMinWindowTitleHeight;
		result = wTitleHeight;
	} else
		result = 0;							//other window types have no title
	return result;
}

//====================================================================================
//	Helper routines for Phil Goldman-esque interfaces.


#define kOptionCode			60
#define kCapsLockCode 		57

static Boolean KeyIsDown(short keyCode)
{
	KeyMap keys;

	GetKeys(keys);
	return ((((char *)keys)[keyCode >> 3] & (char)(1 << (keyCode & 7))) != (char)0);
}

Boolean OptionKeyDown(void)
{	
	return KeyIsDown(kOptionCode);
}

Boolean CapsLockKeyDown(void)
{	
	return KeyIsDown(kCapsLockCode);
}

//====================================================================================

void DelayTicks(long forTicks)
{
	long	unused;
	
	Delay(forTicks, &unused);
}


