// ---------------------------------------------------------------------------
//	DateTime.c
//
// Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// -----------------------------------------------------------------------------------------

#include "Headers.h"

#ifndef __DATETIME_H__
#include "DateTime.h"
#endif
#ifndef _LIMITS
#include <limits.h>
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

// 5. DATE AND TIME SPECIFICATION (RFC822)
// 
// 5.1. SYNTAX
// 
// date-time   =  [ day "," ] date time     ; dd mm yy
//                                          ;  hh:mm:ss zzz
// 
// day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
//             /  "Fri"  / "Sat" /  "Sun"
// 
// date        =  1*2DIGIT month 2DIGIT     ; day month year
//                                          ;  e.g. 20 Jun 82
// 
// month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
//             /  "May"  /  "Jun" /  "Jul"  /  "Aug"
//             /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"
// 
// time        =  hour zone                 ; ANSI and Military
// 
// hour        =  2DIGIT ":" 2DIGIT [":" 2DIGIT]
//                                          ; 00:00:00 - 23:59:59
// 
// zone        =  "UT"  / "GMT"             ; Universal Time
//                                          ; North American : UT
//             /  "EST" / "EDT"             ;  Eastern:  - 5/ - 4
//             /  "CST" / "CDT"             ;  Central:  - 6/ - 5
//             /  "MST" / "MDT"             ;  Mountain: - 7/ - 6
//             /  "PST" / "PDT"             ;  Pacific:  - 8/ - 7
//             /  1ALPHA                    ; Military: Z = UT;
//                                          ;  A:-1; (J not used)
//                                          ;  M:-12; N:+1; Y:+12
//             / ( ("+" / "-") 4DIGIT )     ; Local differential
//                                          ;  hours+min. (HHMM)
// 
// 5.2. SEMANTICS
// 
// If included, day-of-week must be the day implied by the date
// specification.
// 
// Time zone may be indicated in several ways. "UT" is Univer-
// sal Time (formerly called "Greenwich Mean Time"); "GMT" is per-
// mitted as a reference to Universal Time. The military standard
// uses a single character for each zone. "Z" is Universal Time.
// "A" indicates one hour earlier, and "M" indicates 12 hours ear-
// lier; "N" is one hour later, and "Y" is 12 hours later. The
// letter "J" is not used. The other remaining two forms are taken
// from ANSI standard X3.51-1975. One allows explicit indication of
// the amount of offset from UT; the other uses common 3-character
// strings for indicating time zones in North America.

// =============================================================================

const kSecondsPerMinute				= 60;
const kMinutesPerHour 				= 60;
const kHoursPerDay					= 24;
const kDaysPerWeek					= 7;
const kDaysPerYear					= 365;
const kDaysPerLeapYear				= 366;

const kDayOfWeekOffsetPerYear		= kDaysPerYear      % kDaysPerWeek;
const kDayOfWeekOffsetPerLeapYear	= kDaysPerLeapYear  % kDaysPerWeek;

const kSecondsPerHour				= kSecondsPerMinute * kMinutesPerHour;
const kSecondsPerDay				= kSecondsPerHour   * kHoursPerDay;
const kSecondsPerYear				= kSecondsPerDay    * kDaysPerYear;
const kSecondsPerLeapYear			= kSecondsPerDay    * kDaysPerLeapYear;

const long kDaysPerMonth[13]		= {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const long kDaysPerLeapMonth[13]	= {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

enum									{kSunday=0,  kMonday,  kTuesday,  kWednesday,  kThursday,  kFriday,  kSaturday};
const char* kDayOfWeek[7] =				{"Sunday",   "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* kDayOfWeekAbbreviation[7] =	{"Sun",      "Mon",    "Tues",    "Wed",       "Thur",     "Fri",    "Sat"};

enum 									{kNone=0,  kJanuary,  kFebruary,  kMarch,  kApril,  kMay,  kJune,  kJuly,  kAugust,  kSeptember,  kOctober,  kNovember,  kDecember};
const char* kMonth[13] =				{"",       "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char* kMonthAbbreviation[13] =	{"",       "Jan",     "Feb",      "Mar",   "Apr",   "May", "Jun",  "Jul",  "Aug",    "Sep",       "Oct",     "Nov",      "Dec"};

const kEpochYear = 1970;
const kEpochDayOfWeek = kThursday;
const kEndOfEpoch = kEpochYear + (((ulong)ULONG_MAX) / ((ulong)kSecondsPerLeapYear));

// =============================================================================

typedef struct TimeZone {
	const char* name;
	long offset;	// in minutes
} TimeZone;

#define HOUR(x)		(kMinutesPerHour * (x))

static const TimeZone gTimeZone[] = {
    { "gmt", HOUR(0) },			/* Greenwich Mean */
    { "ut", HOUR(0) },			/* Universal */
    { "utc", HOUR(0) },			/* Universal Coordinated */
    { "cut", HOUR(0) },			/* Coordinated Universal */
    { "z", HOUR(0) },			/* Greenwich Mean */
    { "wet", HOUR(0) },			/* Western European */
    { "bst", HOUR(0) },			/* British Summer */
    { "nst", -(HOUR(3)+30) },	/* Newfoundland Standard */
    { "ndt", -(HOUR(3)+30) },	/* Newfoundland Daylight */
    { "ast", -HOUR(4) },		/* Atlantic Standard */
    { "adt", -HOUR(4) },		/* Atlantic Daylight */
    { "est", -HOUR(5) },		/* Eastern Standard */
    { "edt", -HOUR(5) },		/* Eastern Daylight */
    { "cst", -HOUR(6) },		/* Central Standard */
    { "cdt", -HOUR(6) },		/* Central Daylight */
    { "mst", -HOUR(7) },		/* Mountain Standard */
    { "mdt", -HOUR(7) },		/* Mountain Daylight */
    { "pst", -HOUR(8) },		/* Pacific Standard */
    { "pdt", -HOUR(8) },		/* Pacific Daylight */
    { "yst", -HOUR(9) },		/* Yukon Standard */
    { "ydt", -HOUR(9) },		/* Yukon Daylight */
    { "akst", -HOUR(9) },		/* Alaska Standard */
    { "akdt", -HOUR(9) },		/* Alaska Daylight */
    { "hst", -HOUR(10) },		/* Hawaii Standard */
    { "hast", -HOUR(10) },		/* Hawaii-Aleutian Standard */
    { "hadt", -HOUR(10) },		/* Hawaii-Aleutian Daylight */
    { "ces", HOUR(1) },			/* Central European Summer */
    { "cest", HOUR(1) },		/* Central European Summer */
    { "mez", HOUR(1) },			/* Middle European */
    { "mezt", HOUR(1) },		/* Middle European Summer */
    { "cet", HOUR(1) },			/* Central European */
    { "met", HOUR(1) },			/* Middle European */
    { "eet", HOUR(2) },			/* Eastern Europe */
    { "msk", HOUR(3) },			/* Moscow Winter */
    { "msd", HOUR(3) },			/* Moscow Summer */
    { "wast", HOUR(8) },		/* West Australian Standard */
    { "wadt", HOUR(8) },		/* West Australian Daylight */
    { "hkt", HOUR(8) },			/* Hong Kong */
    { "cct", HOUR(8) },			/* China Coast */
    { "jst", HOUR(9) },			/* Japan Standard */
    { "kst", HOUR(9) },			/* Korean Standard */
    { "kdt", HOUR(9) },			/* Korean Daylight */
    { "cast", HOUR(9)+30 },		/* Central Australian Standard */
    { "cadt", HOUR(9)+30 },		/* Central Australian Daylight */
    { "east", HOUR(10) },		/* Eastern Australian Standard */
    { "eadt", HOUR(10) },		/* Eastern Australian Daylight */
    { "nzst", HOUR(12) },		/* New Zealand Standard */
    { "nzdt", HOUR(12) }		/* New Zealand Daylight */
};

// =============================================================================

static long FindZoneOffset(const char* string);
static long FindMonth(const char* string);
static long GetDaysInMonth(long month, long year);
static Boolean IsLeapYear(long year);

// =============================================================================

DateTimeParser::DateTimeParser(void)
{
}

DateTimeParser::~DateTimeParser(void)
{
}

const char*
DateTimeParser::GetMonthString(void) const
{
	return kMonth[fMonth];
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
const char*
DateTimeParser::GetMonthAbbreviatedString(void) const
{
	return kMonthAbbreviation[fMonth];
}
#endif

const char* DateTimeParser::GetDayOfWeekString(void) const
{
	return kDayOfWeek[fDayOfWeek];
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
const char* DateTimeParser::GetDayOfWeekAbbreviatedString(void) const
{
	return kDayOfWeekAbbreviation[fDayOfWeek];
}
#endif

// ---------------------------------------------------------------------------

ulong
DateTimeParser::Parse(const char* parseString)
{
	if (IsWarning(parseString == nil))
		return 0;
	
	DateTimeParser dateTimeParser;
	dateTimeParser.SetDateTimeText(parseString);
	ulong result = dateTimeParser.GetDateTime();
	return result;
}

void
DateTimeParser::Reset(void)
{
	fDateTime = 0;

	fZoneOffset = 0;
	fYear = 0;
	fMonth = 0;
	fDay = 0;
	fHours = 0;
	fMinutes = 0;
	fSeconds = 0;
}

void
DateTimeParser::SetDateTime(ulong secondsInEpoch)
{
	fDateTime = secondsInEpoch;

	// calculate year in epoch

	fYear = kEpochYear;
	fDayOfWeek = kEpochDayOfWeek;
	
	Boolean isLeapYear = IsLeapYear(fYear);
	long secondsPerYear = isLeapYear ? kSecondsPerLeapYear : kSecondsPerYear;
	long dayOfWeekOffset = isLeapYear ? kDayOfWeekOffsetPerLeapYear : kDayOfWeekOffsetPerYear;
	
	while (secondsInEpoch >= secondsPerYear)
	{	secondsInEpoch -= secondsPerYear;
		fDayOfWeek += dayOfWeekOffset;
		fYear++;
		isLeapYear = IsLeapYear(fYear);
		if (isLeapYear)
		{	dayOfWeekOffset = kDayOfWeekOffsetPerLeapYear;
			secondsPerYear = kSecondsPerLeapYear;
		}
		else
		{	dayOfWeekOffset = kDayOfWeekOffsetPerYear;
			secondsPerYear = kSecondsPerYear;
		}
	}

	// calculate day of the week
	fDayOfWeek += secondsInEpoch/kSecondsPerDay;
	fDayOfWeek = fDayOfWeek % kDaysPerWeek;

	// calculate month in the year
	fMonth = kJanuary;
	long secondsPerMonth = kSecondsPerDay * (isLeapYear ? kDaysPerLeapMonth[fMonth] : kDaysPerMonth[fMonth]);
	
	while (secondsInEpoch >= secondsPerMonth) {
		secondsInEpoch -= secondsPerMonth;
		fMonth++;
		secondsPerMonth = kSecondsPerDay * (isLeapYear ? kDaysPerLeapMonth[fMonth] : kDaysPerMonth[fMonth]);
	}
	
	// calculate day of the month
	fDay = secondsInEpoch / kSecondsPerDay;
	secondsInEpoch -= fDay * kSecondsPerDay;
	fDay++;		// day starts at 1st of the month
	IsError(secondsInEpoch >= kSecondsPerDay);
	
	// calculate number of hours
	fHours = secondsInEpoch / kSecondsPerHour;
	secondsInEpoch -= fHours * kSecondsPerHour;
	IsError(secondsInEpoch >= kSecondsPerHour);
	
	// calculate number of minutes
	fMinutes = secondsInEpoch / kSecondsPerMinute;
	secondsInEpoch -= fMinutes * kSecondsPerMinute;
	IsError(secondsInEpoch >= kSecondsPerMinute);

	// calculate number of seconds
	fSeconds = secondsInEpoch;

	// create textual representation of date & time
	snprintf(fDateString, sizeof(fDateString), "%s, %s, %d, %d",
		GetDayOfWeekString(), GetMonthString(), GetDay(), GetYear());
		
	snprintf(fTimeString, sizeof(fTimeString), "%02d:%02d:%02d",
			(int)GetHours(), (int)GetMinutes(), (int)GetSeconds());
}

void
DateTimeParser::SetDateTimeText(const char* input)
{
	char buffer[16];	// length must match sscanf %15s found below
	const char* in = input;
	const char* p;
	long count;
	long hours, minutes;
	Boolean isNegative;
	Boolean isLeapYear;
	long month;
	long year;
	
	// Skip day of week, if present [e.g. Sunday, 07-Jan-96 21:46:45 PST].
	if ((p = FindCharacter(in, ",")) != nil)
		in = p + 1;
	in = SkipCharacters(in, " ");
	
	// Parse day [e.g. 07-Jan-96 21:46:45 PST].
	if (sscanf(in, "%ld%ln", &fDay, &count) != 1)
		goto fail;
	if (fDay < 1 || fDay > 31)
		goto fail;
	in += count;
	in = SkipCharacters(in, " -");
	
	// Parse month [e.g. Jan-96 21:46:45 PST].
	if ((p = FindCharacter(in, " -")) == nil)
		goto fail;
	if (sscanf(in, "%15[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz]", buffer) != 1)
		goto fail;
	fMonth = FindMonth(buffer);
	if (fMonth < kJanuary || fMonth > kDecember)
		goto fail;
	in += strlen(buffer);
	in = SkipCharacters(in, " -");
	
	// Parse year [e.g. 96 21:46:45 PST].
	if (sscanf(in, "%ld%ln", &fYear, &count) != 1)
		goto fail;
	if (fYear < 0)
		goto fail;
	if (fYear < 100)
		fYear += 1900;
	if (fYear < kEpochYear)
		fYear += 100;
	if (fYear < kEpochYear || fYear > kEndOfEpoch)
		goto fail;
	if (fDay > GetDaysInMonth(fMonth,  fYear))
		goto fail;
	in += count;
	in = SkipCharacters(in, " -");
	
	// Parse hours [e.g. 21:46:45 PST].
	if (sscanf(in, "%ld%ln", &fHours, &count) != 1)
		goto fail;
	if (fHours < 0 || fHours > 23)
		goto fail;
	in += count;
	in = SkipCharacters(in, " :");
	
	// Parse minutes [e.g. 46:45 PST].
	if (sscanf(in, "%ld%ln", &fMinutes, &count) != 1)
		goto fail;
	if (fMinutes < 0 || fMinutes > 59)
		goto fail;
	in += count;
	in = SkipCharacters(in, " :");
	
	// Parse seconds [e.g. 45 PST].
	if (sscanf(in, "%ld%ln", &fSeconds, &count) != 1)
		goto fail;
	if (fSeconds < 0 || fSeconds > 59)
		goto fail;
	in += count;
	in = SkipCharacters(in, " ");
	
	// Parse time zone [e.g. PST].
	if (*in == '+' || *in == '-') {
		isNegative = (*in == '-');
		in = SkipCharacters(in, "+- ");
		if (sscanf(in, "%2ld%2ld", &hours, &minutes) != 2)
			goto fail;
		fZoneOffset = hours * 60 + minutes;
		if (isNegative)
			fZoneOffset = -fZoneOffset;
	} else {
		if (sscanf(in, "%15[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz]", buffer) != 1)
			goto fail;
		fZoneOffset = FindZoneOffset(buffer);
	}
	
	// Compute day of the week while converting string to ulong.
	fDateTime = 0;
	year = kEpochYear;
	//fDayOfWeek = kEpochDayOfWeek;
	
	isLeapYear = IsLeapYear(year);
	while (year < fYear)
	{
		if (isLeapYear)
		{	fDateTime += kSecondsPerLeapYear;
			//fDayOfWeek += kDayOfWeekOffsetPerLeapYear;
		}
		else
		{	fDateTime += kSecondsPerYear;
			//fDayOfWeek += kDayOfWeekOffsetPerYear;
		}
		year++;
		isLeapYear = IsLeapYear(year);
	}
	month = kJanuary;
	while (month < fMonth)
	{	
		if (isLeapYear)
		{
			fDateTime += kSecondsPerDay * kDaysPerLeapMonth[month];
			//fDayOfWeek += kDaysPerLeapMonth[month];
		}
		else
		{
			fDateTime += kSecondsPerDay * kDaysPerMonth[month];
			//fDayOfWeek += kDaysPerLeapMonth[month];
		}
		month++;
	}
	//fDayOfWeek = fDayOfWeek % kDaysPerWeek;
	fDateTime += (fDay - 1) * kSecondsPerDay;
    fDateTime += fHours * kSecondsPerHour;
    fDateTime += fMinutes * kSecondsPerMinute;
    fDateTime += fSeconds;
    fDateTime -= fZoneOffset * kSecondsPerMinute;

    SetDateTime(fDateTime);
    return;

fail:
	ImportantMessage(("SetDateTimeText:: cannot parse %s", input));
	Reset();
}

// ---------------------------------------------------------------------------
//	Helper functions
// ---------------------------------------------------------------------------

static long
FindZoneOffset(const char* string)
{
	long i;
	
	for (i = 0; i < sizeof (gTimeZone) / sizeof (TimeZone); i++)
		if (EqualString(gTimeZone[i].name, string))
			return gTimeZone[i].offset;
	
	IsError(true);
	ImportantMessage(("FindZoneOffset: cannot parse %s", string));
	return 0;
}

static long
FindMonth(const char* string)
{
	long i;
	
	if (IsError(string == nil || *string == 0))
		return kJanuary;
	
	for (i = kJanuary; i <= kDecember; i++) {
		if (EqualString(kMonth[i], string))
			return i;
		if (EqualString(kMonthAbbreviation[i], string))
			return i;
	}
	
	IsError(true);
	ImportantMessage(("FindMonth: cannot parse %s", string));
	return kJanuary;
}

static long
GetDaysInMonth(long month, long year)
{
	if (IsError(month < 1))
		month = 1;

	if (IsError(month > 12))
		month = 12;
		
	if (IsLeapYear(year))
		return kDaysPerLeapMonth[month];
	
	return kDaysPerMonth[month];
}

static Boolean
IsLeapYear(long year)
{
	return (year%4 == 0) && ( (year%100 != 0) || (year%400 == 0) );
}


// =============================================================================
//  Testing suite for a few nasty cases
// =============================================================================

#ifdef TEST_SIMULATORONLY

struct DateTimeTestRecord
{
	const char*	text;
	long		zoneOffset;
	long		year;
	long		month;
	long		day;
	long		dayOfWeek;
	long		hours;
	long		minutes;
	long		seconds;

	Boolean		CanParseCorrectly(void);
};

DateTimeTestRecord gDateTimeTestRecord[] =
{
	{"01-Mar-95 00:00:00 GMT",				0,		1995, kMarch,      1,  kWednesday,  0,  0,  0},		
	{"01-Jan-70 00:00:00 GMT", 				0,      1970, kJanuary,    1,  kThursday,   0,  0,  0},
	{"Sunday, 07-Jan-96 13:46:45 PST",		-(8*60),1996, kJanuary,    7,  kSunday,    21, 46, 45},
	{"Sunday, 07-Jan-96 13:46:45 PST (foo)",-(8*60),1996, kJanuary,    7,  kSunday,    21, 46, 45},
	{"28-Feb-71 00:00:00 GMT",				0,		1971, kFebruary,   28, kSunday,     0,  0,  0},		
	{"28-Feb-72 00:00:00 GMT",				0,		1972, kFebruary,   28, kMonday,     0,  0,  0},		
	{"28-Feb-73 00:00:00 GMT",				0,		1973, kFebruary,   28, kWednesday,  0,  0,  0},		
	{"28-Feb-74 00:00:00 GMT",				0,		1974, kFebruary,   28, kThursday,   0,  0,  0},		
	{"28-Feb-75 00:00:00 GMT",				0,		1975, kFebruary,   28, kFriday,     0,  0,  0},		
	{"28-Feb-76 00:00:00 GMT",				0,		1976, kFebruary,   28, kSaturday,   0,  0,  0},		
	{"28-Feb-77 00:00:00 GMT",				0,		1977, kFebruary,   28, kMonday,     0,  0,  0},		
	{"01-Mar-96 00:00:00 GMT",				0,		1996, kMarch,      1,  kFriday,     0,  0,  0},		
	{"01-Mar-97 00:00:00 GMT",				0,		1997, kMarch,      1,  kSaturday,   0,  0,  0},		
	{"01-Mar-98 00:00:00 GMT",				0,		1998, kMarch,      1,  kSunday,     0,  0,  0},		
	{"01-Mar-99 00:00:00 GMT",				0,		1999, kMarch,      1,  kMonday,     0,  0,  0},		
	{"01-Mar-00 00:00:00 GMT",				0,		2000, kMarch,      1,  kWednesday,  0,  0,  0},		
	{"01-Mar-01 00:00:00 GMT",				0,		2001, kMarch,      1,  kThursday,   0,  0,  0},		
	{"01-Mar-02 00:00:00 GMT",				0,		2002, kMarch,      1,  kFriday,     0,  0,  0},		
	{"31-Dec-18 00:00:00 GMT",				0,		2018, kDecember,   31, kMonday,     0,  0,  0},		
	{"31-Dec-19 00:00:00 GMT",				0,		2019, kDecember,   31, kTuesday,    0,  0,  0},		
	{"31-Dec-20 00:00:00 GMT",				0,		2020, kDecember,   31, kThursday,   0,  0,  0},		
	{"01-Feb-03 04:05:06 MDT (mdt=-7 hrs)", -(7*60),2003, kFebruary,   1,  kSaturday,  11,  5,  6},		
};

Boolean
DateTimeTestRecord::CanParseCorrectly(void)
{
	DateTimeParser tempParser;

	tempParser.Reset();
	tempParser.SetDateTimeText(text);
	Boolean passTest = true;
	long result;
	
	result = tempParser.GetZoneOffset();
	passTest = passTest && (result == zoneOffset);
	Assert(passTest);
	
	result = tempParser.GetYear();
	passTest = passTest && (result == year);
	Assert(passTest);

	result = tempParser.GetMonth();
	passTest = passTest && (result == month);
	Assert(passTest);

	result = tempParser.GetDay();
	passTest = passTest && (result == day);
	Assert(passTest);

	result = tempParser.GetDayOfWeek();
	passTest = passTest && (result == dayOfWeek);
	Assert(passTest);

	result = tempParser.GetHours();
	passTest = passTest && (result == hours);
	Assert(passTest);

	result = tempParser.GetMinutes();
	passTest = passTest && (result == minutes);
	Assert(passTest);
	
	result = tempParser.GetSeconds();
	passTest = passTest && (result == seconds);
	Assert(passTest);
	
	ulong setTextResult = tempParser.GetDateTime();
	ulong parseResult = DateTimeParser::Parse(text);
	passTest = passTest && (setTextResult == parseResult);
	Assert(passTest);

	return passTest;
}

Boolean
DateTimeParser::Test(void)
{
	long testsPassed = 0;
	long testsAttempted = 0;
	long before;
	long after;
	
	for (int i=0; i<sizeof(gDateTimeTestRecord)/sizeof(gDateTimeTestRecord[0]); i++) {
		testsAttempted++;
		if (gDateTimeTestRecord[i].CanParseCorrectly())
			testsPassed++;
		else {
			Complain(("DateTimeParser didn't pass test for \"%s\"", gDateTimeTestRecord[i].text));
		}
	}
	
	DateTimeParser beforeParser;
	DateTimeParser afterParser;
	
	beforeParser.SetDateTimeText("31-Dec-95 23:59:59 GMT");
	before = beforeParser.GetDateTime();
	afterParser.SetDateTimeText("01-Jan-96 00:00:00 GMT");
	after = afterParser.GetDateTime();
	testsAttempted++;
	if (after - before == 1)
		testsPassed++;
	else {
		Complain(("DateTimeParser::Test()  Parser didn't span '95-96 new year correctly"));
	}

	beforeParser.SetDateTimeText("31-Dec-96 23:59:59 GMT");
	before = beforeParser.GetDateTime();
	afterParser.SetDateTimeText("01-Jan-97 00:00:00 GMT");
	after = afterParser.GetDateTime();
	testsAttempted++;
	if (after - before == 1)
		testsPassed++;
	else {
		Complain(("DateTimeParser::Test()  Parser didn't span '95-96 new year correctly"));
	}

	beforeParser.SetDateTimeText("28-Feb-95 23:59:59 GMT");
	before = beforeParser.GetDateTime();
	afterParser.SetDateTimeText("01-Mar-95 00:00:00 GMT");
	after = afterParser.GetDateTime();
	testsAttempted++;
	if (after - before == 1)
		testsPassed++;
	else {
		Complain(("DateTimeParser::Test()  Parser didn't span Feb-Mar of '95 correctly"));
	}

	beforeParser.SetDateTimeText("28-Feb-96 23:59:59 GMT");
	before = beforeParser.GetDateTime();
	afterParser.SetDateTimeText("01-Mar-96 00:00:00 GMT");
	after = afterParser.GetDateTime();
	testsAttempted++;
	if (after - before == 1 + kSecondsPerDay)
		testsPassed++;
	else {
		Complain(("DateTimeParser::Test()  Parser didn't span Feb-Mar of '96 correctly"));
	}

	beforeParser.SetDateTimeText("28-Feb-97 23:59:59 GMT");
	before = beforeParser.GetDateTime();
	afterParser.SetDateTimeText("01-Mar-97 00:00:00 GMT");
	after = afterParser.GetDateTime();
	testsAttempted++;
	if (after - before == 1)
		testsPassed++;
	else {
		Complain(("DateTimeParser::Test()  Parser didn't span Feb-Mar of '97 correctly"));
	}

	Message(("DateTimeParser::Test() completed %d of %d tests successfully.", testsPassed, testsAttempted));
	return (testsPassed == testsAttempted);
}
#endif /* TEST_SIMULATORONLY */

// =============================================================================

