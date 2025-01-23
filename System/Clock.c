// ===========================================================================
//	Clock.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef TEST_CLIENTTEST
	#ifndef __CLIENTTEST_H__
	#include "ClientTest.h"
	#endif
#endif
#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __DATETIME_H__
#include "DateTime.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

// ============================================================================
// Class Clock

Clock* gClock;

Clock::~Clock()
{
}

#ifdef INCLUDE_FINALIZE_CODE
void Clock::Finalize()
{
	if (gClock != nil)
	{	delete(gClock);
		gClock = nil;
	}
}
#endif

const char*
Clock::GetDateString() const
{
	return fDateTimeParser.GetDateString();
}

ulong Clock::GetDateTimeGMT() const
{
	if (fDateTimeBase == 0)
		return 0;
	
	return fDateTimeBase + (Now() - fNowBase) / kOneSecond;
}

ulong Clock::GetDateTimeLocal() const
{
	if (fDateTimeBase == 0)
		return 0;
		
	return GetDateTimeGMT() + fTimeZoneOffset;
}

const char*
Clock::GetTimeString() const
{
	return fDateTimeParser.GetTimeString();
}

void Clock::Initialize()
{
	Assert(gClock == nil);
	gClock = new(Clock);
}

Boolean
Clock::SetAttribute(const char* name, char* value)
{
	if (EqualString(name, "wtv-client-date")) {
		SetDateTime(DateTimeParser::Parse(value));
		return true;
	}
	
	if (EqualString(name, "wtv-client-time-zone")) {
		SetTimeZone(value);
		return true;
	}
	
	return false;
}

void Clock::SetDateTime(ulong value)
{
	fDateTimeBase = value;
	fNowBase = Now();
	fDateTimeParser.SetDateTime(GetDateTimeLocal());
	
	Message(("Clock: setting {date,time} to {\"%s\",\"%s\"}",
			fDateTimeParser.GetDateString(), 
			fDateTimeParser.GetTimeString()));
}

void Clock::SetTimeZone(const char* nameAndOffset)
{
	// Format: "PDT -0800"
	
	const char* in = nameAndOffset;
	char name[16];	// length must match sscanf %15s below
	long hours, minutes, seconds;
	Boolean isNegative;
	
	// Parse time zone text [e.g. PDT -0800].
	if (sscanf(in, "%15[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz]", name) != 1)
		goto fail;
	in += strlen(name);
	
	// Parse offset sign [e.g. -0800].
	in = SkipCharacters(in, " ");
	isNegative = (*in == '-');
	in = SkipCharacters(in, "+- ");
	
	// Parse offset.[e.g. 00].
	if (sscanf(in, "%2ld%2ld", &hours, &minutes) != 2)
		goto fail;
		
	// Set name and offset.
	seconds = ((hours * 60) + minutes) * 60;
	if (isNegative)
		seconds = -seconds;
	SetTimeZone(name, seconds);
	return;

fail:
	ImportantMessage(("SetTimeZone: cannot parse %s", nameAndOffset));
}	
	
void Clock::SetTimeZone(const char* name, long offsetInSeconds)
{
	Assert(name && *name);
	TrivialMessage(("SetTimeZone: %s %d minutes", name, offsetInSeconds / 60));
	fTimeZoneOffset = offsetInSeconds;
	CopyStringIntoCharArray(fTimeZoneName, name);
	fDateTimeParser.SetDateTime(GetDateTimeLocal());
}

#ifdef TEST_CLIENTTEST
struct TimeZoneTest {
	const char* nameIn;
	const char* nameOut;
	long		offset;
};

const TimeZoneTest kTimeZoneTest[] = {
	// SetTimeZone(%s)	=>	GetTimeZoneName(%s)	 +	GetTimeZoneOffset(%d)
	{ "CAST +0930", 		"CAST", 				((9*60)+30)*60 },
	{ "PST -0800", 			"PST", 					-8*60*60 }
};

Boolean
ClientTestClock(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;
	Boolean err = false;

	for (int index = 0; index<sizeof(kTimeZoneTest)/sizeof(kTimeZoneTest[0]); index++) {
		gClock->SetTimeZone(kTimeZoneTest[index].nameIn);
		testsAttempted++;
		
		const char* timeZoneName = gClock->GetTimeZoneName();
		long timeZoneOffset = gClock->GetTimeZoneOffset();
		Boolean pass = EqualString(timeZoneName, kTimeZoneTest[index].nameOut)
				  					&& (timeZoneOffset == kTimeZoneTest[index].offset);
		if (pass) {
			testsPassed++;
		}
		
		stream->LevelPrintf(err ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
				"ClientTestClock():  test[%d] SetTimeZone(%s) (%s):\n"
				"\tGetTimeZoneName()=%s (wanted %s)\n"
				"\tGetTimeZoneOffset()=%d (wanted %d)\n",
				testsAttempted,
				kTimeZoneTest[index].nameIn,
				pass ? "passed" : "failed",
				timeZoneName,
				kTimeZoneTest[index].nameOut,
				timeZoneOffset,
				kTimeZoneTest[index].offset);
	}

	Boolean failTest = testsPassed != testsAttempted;
	stream->LevelPrintf(kClientTestPrintLevelSubUnitPassFail,
					"ClientTestClock %s", failTest ? "***** fail *****" : "pass");
	stream->LevelPrintf(kClientTestPrintLevelScore,
					"(passed %d of %d tests)", testsPassed, testsAttempted);
	stream->LevelPrintf(kClientTestPrintLevelSubUnitPassFail, "\n");
	return !failTest;
}
#endif

// ============================================================================

