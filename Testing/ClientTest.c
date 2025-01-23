// ===========================================================================
//	ClientTest.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifdef TEST_CLIENTTEST
#include "Headers.h"

#ifndef __BOXANSI_H__
#include "boxansi.h"
#endif
#ifndef __CACHESTREAM_H__
#include "CacheStream.h"
#endif
#ifndef __CLIENTTEST_H__
#include "ClientTest.h"
#endif
#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif




// ---------------------------------------------------------------------------
//	typedefs
//
typedef Boolean (ClientTestFunction)(ClientTestStream* stream);

struct ClientTestList {
	const char*			name;
	const char*			description;
	ClientTestFunction*	function;
};




// ---------------------------------------------------------------------------
//	static functions
//
Boolean ClientTestHelp(ClientTestStream* stream);




// ---------------------------------------------------------------------------
//	constants
const ClientTestList gClientTestList[] = {
	{"Help",  "Show this screen", ClientTestHelp},	// <-- must come first!!!
	{"ANSI",  "Test the box ANSI library", ClientTestANSI},
	{"Cache", "Test rudimentary Cache functions", ClientTestCache},
	{"Clock", "Test clock timezones, etc.", ClientTestClock}
};

const char kWriteToStreamString[] = "Stream";
const char kWriteToPrintfString[] = "Printf";
#ifdef INCLUDE_FPRINTF
const char kWriteToFILEString[] = "FILE";
#endif





// ---------------------------------------------------------------------------
//	begin ClientTestStream
//

int
ClientTestStream::LevelPrintf(ClientTestPrintLevel printLevel, const char* format, ...)
{
	int result;
	va_list params;
	
	if (printLevel > fPrintLevel)
		return 0;
	
	va_start(params, format);
	result = VPrintf(format, params);
	va_end(params);
	
	return result;
}

int
ClientTestStream::LevelVPrintf(ClientTestPrintLevel printLevel, const char* format, va_list list)
{
	if (printLevel > fPrintLevel)
		return 0;
	
	return VPrintf(format, list);
}

int
ClientTestStream::Printf(const char* format, ...)
{
	int result;
	va_list params;
	
	va_start(params, format);
	result = VPrintf(format, params);
	va_end(params);
	
	return result;
}

int
ClientTestStream::VPrintf(const char* format, va_list list)
{
	int result = 0;
	
	switch (fDestType) {
		case kClientTestDestTypeStream:
			result = ((Stream*)fDest)->VPrintf(format, list);
			break;
		case kClientTestDestTypePrintf:
			result = vprintf(format, list);
			break;
#ifdef INCLUDE_FPRINTF
		case kClientTestDestTypeFILE:
			result = ::vfprintf((FILE*)fDest, format, list);
			break;
#endif
	}
	return result;
}

//
//	end ClientTestStream
// ---------------------------------------------------------------------------

Boolean
ClientTestHelp(ClientTestStream* stream)
{
	stream->Printf("ClientTestHelp()\n");
	return true;
}

void
DoClientTest(const StringDictionary* args)
{
	ClientTestStream clientTestStream;
	
	clientTestStream.fDestType = kClientTestDestNone;
	
	// ----------- where to?
	
	const char* writeToPrintfString = args->GetValue(kWriteToPrintfString);
	if (writeToPrintfString != nil) {
		clientTestStream.fDestType = kClientTestDestTypePrintf;
	}
	
#ifdef INCLUDE_FPRINTF
	const char* writeToFILEString = args->GetValue(kWriteToFILEString);
	if (writeToFILEString != nil) {
		FILE* fp = fopen((strlen(writeToFILEString) > 0) ? writeToFILEString
														: "ClientTest.output", "w");
		if (fp != nil) {
			clientTestStream.fDestType = kClientTestDestTypeFILE;
			clientTestStream.fDest = (void*)fp;
		}
	}
#endif

	const char* writeToStreamString = args->GetValue(kWriteToStreamString);
	if ((writeToStreamString != nil) || (clientTestStream.fDestType == kClientTestDestNone)) {
		Resource* resource = new(Resource);
		if (resource != nil) {
			resource->SetURL(((writeToStreamString != nil) && (*writeToStreamString != '\0'))
								? writeToStreamString : "cache:/ClientTest");
			CacheStream* cacheStream = resource->NewStreamForWriting();
			if (cacheStream != nil) {
				cacheStream->SetDataType(kDataTypeTEXT);
				cacheStream->Printf("ClientTest\n");
				clientTestStream.fDestType = kClientTestDestTypeStream;
				clientTestStream.fDest = (void*)cacheStream;
			}
			delete (resource);
		}	
	}

	const char kClientTestAll[] = "All";
	
	const char* testAllString = args->GetValue(kClientTestAll);
	int testAllPrintLevel;
	
	if (testAllString != nil) {
		if (sscanf(testAllString, "%d", &testAllPrintLevel) != 1) {
			testAllPrintLevel = (int)kClientTestPrintLevelPassFail;
		}
 	}

	for (int i=0; i<sizeof(gClientTestList) / sizeof(gClientTestList[0]); i++) {
		const char* printLevelString = args->GetValue(gClientTestList[i].name);
		if ((printLevelString == nil) && ((testAllString == nil) || (i==0))) // (i==0) --> "Help" and
			continue;											 // test "All" doesn't include "Help"
		
		if (printLevelString != nil) {
			int intPrintLevel;
			if (sscanf(printLevelString, "%d", &intPrintLevel) == 1) {
				clientTestStream.fPrintLevel = (ClientTestPrintLevel)intPrintLevel;
			} else {
				clientTestStream.fPrintLevel = kClientTestPrintLevelSubUnitPassFail;
			}
		} else {
			clientTestStream.fPrintLevel = (ClientTestPrintLevel)testAllPrintLevel;
		}
		
		Boolean result = (*(gClientTestList[i].function))(&clientTestStream);
		
		clientTestStream.LevelPrintf(kClientTestPrintLevelPassFail,
									 "Testing \"%s\":  %s\n",
									 gClientTestList[i].name,
									 result ? "pass" : "***** fail *****");
	}


	// ----------- clean up destination
	
	switch (clientTestStream.fDestType) {
		case kClientTestDestNone:
			break;
		case kClientTestDestTypeStream:
		{
			CacheStream* cacheStream = (CacheStream*)(clientTestStream.fDest);
			cacheStream->SetStatus(kComplete);
			gPageViewer->ExecuteURL(strlen(writeToStreamString) > 0 ? writeToStreamString
																	: "cache:/ClientTest");
			delete (cacheStream);
		}
			break;
		case kClientTestDestTypePrintf:
			break;
#ifdef INCLUDE_FPRINTF
		case kClientTestDestTypeFILE:
			fclose((FILE*)(clientTestStream.fDest));
			break;
#endif
	}
}

#endif /* TEST_CLIENTTEST */