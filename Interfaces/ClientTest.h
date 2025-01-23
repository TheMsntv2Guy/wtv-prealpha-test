// ===========================================================================
//	ClientTest.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CLIENTTEST_H__
#define __CLIENTTEST_H__

#ifdef TEST_CLIENTTEST

#ifndef __BOXANSI_H__
#include "boxansi.h"			/* for va_list */
#endif
#ifndef __LIST_H__
#include "List.h"				/* for StringDictionary */
#endif

// how detailed of output to spit out
enum ClientTestPrintLevel {
	kClientTestPrintLevelNothing			= 0,
	kClientTestPrintLevelPassFail			= 1,
	kClientTestPrintLevelSubUnitPassFail	= 2,
	kClientTestPrintLevelScore				= 3,
	kClientTestPrintLevelFailedTests		= 4,
	kClientTestPrintLevelAllTests			= 5
};

// where to send output
enum ClientTestDestType {
	kClientTestDestNone,
	kClientTestDestTypeStream,
	kClientTestDestTypePrintf,
	kClientTestDestTypeFILE
};


// ---------------------------------------------------------------------------

struct ClientTestStream {
	int	LevelPrintf(ClientTestPrintLevel printLevel, const char* format, ...);
	int	LevelVPrintf(ClientTestPrintLevel printLevel, const char* format, va_list list);
	int	Printf(const char* format, ...);
	int	VPrintf(const char* format, va_list list);

	ClientTestPrintLevel	fPrintLevel;
	ClientTestDestType		fDestType;
	void*					fDest;
};

// ---------------------------------------------------------------------------

void DoClientTest(const StringDictionary* args);

Boolean ClientTestANSI(ClientTestStream* stream);	// in ClientTestANSI.c
Boolean ClientTestCache(ClientTestStream* stream);	// friend of Cache in Cache.c
Boolean ClientTestHelp(ClientTestStream* stream);   // in ClientTest.c

// ---------------------------------------------------------------------------

#endif /* TEST_CLIENTTEST */

#endif /* __CLIENTTEST_H__ */