// ---------------------------------------------------------------------------
//	DateTime.h
//
// Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ---------------------------------------------------------------------------

#ifndef __DATETIME_H__
#define __DATETIME_H__

class DateTimeParser {
public:
							DateTimeParser(void);
	virtual					~DateTimeParser(void);
	
	ulong					GetDateTime(void) const { return fDateTime; };
	long					GetDay(void) const { return fDay; };
#ifdef TEST_SIMULATORONLY
	long					GetDayOfWeek(void) const { return fDayOfWeek; };
#endif
	const char*				GetDayOfWeekAbbreviatedString(void) const;
	const char*				GetDayOfWeekString(void) const;
#ifdef TEST_SIMULATORONLY
	long					GetMonth(void) const { return fMonth; };
#endif
	const char*				GetMonthAbbreviatedString(void) const;
	const char*				GetMonthString(void) const;
	long					GetHours(void) const { return fHours; };
	long					GetMinutes(void) const { return fMinutes; };
	long					GetSeconds(void) const { return fSeconds; };
	long					GetYear(void) const { return fYear; };
#ifdef TEST_SIMULATORONLY
	long					GetZoneOffset(void) const { return fZoneOffset; };	// in minutes from GMT
#endif
	
	void					SetDateTime(ulong);
	void					SetDateTimeText(const char*);

	const char*				GetDateString(void) const { return fDateString; };
	const char*				GetTimeString(void) const { return fTimeString; };
	
	void					Reset(void);

#ifdef TEST_SIMULATORONLY
	static Boolean			Test(void);
#endif /* TEST_SIMULATORONLY */

	static void				Initialize(void);
#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize(void);
#endif /* INCLUDE_FINALIZE_CODE */
	static ulong			Parse(const char*);

protected:
	ulong					fDateTime;
	char					fDateString[48];
	char					fTimeString[48];

	long					fZoneOffset;
	long					fYear;
	long					fMonth;
	long					fDay;
	long					fDayOfWeek;
	long					fHours;
	long					fMinutes;
	long					fSeconds;
};

// ---------------------------------------------------------------------------

#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include DateTime.h multiple times"
#endif
#endif /* __DATETIME_H__ */

