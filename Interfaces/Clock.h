// ===========================================================================
//	Clock.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CLOCK_H__
#define __CLOCK_H__

#ifndef __DATETIME_H__
#include "DateTime.h"		/* for DateTimeParser */
#endif

#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

class Clock;

#ifdef TEST_CLIENTTEST
class ClientTestStream;
#endif

// =============================================================================

extern Clock* gClock;

class Clock : public HasAttributes {
public:
	virtual					~Clock();
							
	ulong					GetDateTimeGMT() const;
	ulong					GetDateTimeLocal() const;
	const char*				GetTimeZoneName() const;
	long					GetTimeZoneOffset() const;
	const char*				GetDateString() const;
	const char*				GetTimeString() const;
	
	virtual Boolean			SetAttribute(const char* name, char* value);
	void					SetDateTime(ulong dateTime);
	void					SetTimeZone(const char* nameAndOffset);
	void					SetTimeZone(const char* name, long offsetInSeconds);	// GMT + offsetInSeconds = local

#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize();
#endif /* INCLUDE_FINALIZE_CODE */
	static void				Initialize();

#ifdef TEST_CLIENTTEST
	friend Boolean ClientTestClock(ClientTestStream* stream);
#endif
	
protected:
	ulong					GetGMT() const;
	ulong					GetLocal() const;

protected:
	DateTimeParser			fDateTimeParser;

	ulong					fDateTimeBase;
	ulong					fNowBase;
	long					fTimeZoneOffset;
	char					fTimeZoneName[12];
};

#ifdef TEST_CLIENTTEST
inline const char* Clock::GetTimeZoneName() const
{
	return fTimeZoneName;
}
#endif

#ifdef TEST_CLIENTTEST
inline long Clock::GetTimeZoneOffset() const
{
	return fTimeZoneOffset;
}
#endif

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Clock.h multiple times"
	#endif
#endif
