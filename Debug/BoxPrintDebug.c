// ===========================================================================
//	BoxPrintDebug.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __BOXPRINTDEBUG_H__
#include "BoxPrintDebug.h"
#endif

#ifdef DEBUG_BOXPRINT

static int gPrintIndent = 0;

static DebugCookie gDebugCookie = kDebugCookieNone;

// ===========================================================================
//	These are Mac-only for showing box printfs
// ===========================================================================

#ifdef FOR_MAC
	#ifndef __MEMORYMANAGER_H__
	#include "MemoryManager.h"
	#endif
	#ifndef __TEWINDOW_H__
	#include "TEWindow.h"
	#endif

	class BoxPrintWindow : public TEWindow {
		public:
						BoxPrintWindow();
		virtual			~BoxPrintWindow();
	};
	
	static BoxPrintWindow* gBoxPrintWindow = nil;

	BoxPrintWindow::BoxPrintWindow()
	{
		SetTitle("Box Print Window");
		SetBodyFont(monaco, 9, normal);
		ShowWindow();
		gBoxPrintWindow = this;
	}
	
	BoxPrintWindow::~BoxPrintWindow()
	{
		gBoxPrintWindow = nil;
	}

	static Boolean VerifyBoxOutputWindow();
	static Boolean
	VerifyBoxOutputWindow()
	{
		if (gBoxPrintWindow == nil) {
			gBoxPrintWindow = newMac(BoxPrintWindow);
		}
		return gBoxPrintWindow != nil;
	}
#endif
// ===========================================================================


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
BoxPrintCookieStart(const DebugCookie cookie)
{
	IsWarning(gDebugCookie != kDebugCookieNone);
	gDebugCookie = cookie;
	switch (cookie)
	{
		case kDebugCookieNetwork:		BoxPrint("<Begin Network>"); break;
		case kDebugCookieCache:			BoxPrint("<Begin Cache>"); break;
		case kDebugCookieServiceList:	BoxPrint("<Begin ServiceList>"); break;
	}
}
#endif
 
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
BoxPrintCookieEnd(const DebugCookie cookie)
{
	IsWarning(gDebugCookie != cookie);
	gDebugCookie = kDebugCookieNone;
	switch (cookie)
	{
		case kDebugCookieNetwork:		BoxPrint("<End Network>"); break;
		case kDebugCookieCache:			BoxPrint("<End Cache>"); break;
		case kDebugCookieServiceList:	BoxPrint("<End ServiceList>"); break;
	}
}
#endif
 
void
BoxPrint(const char* format, ...)
{
#ifdef FOR_MAC
	if (!VerifyBoxOutputWindow())
		return;
	va_list list;
    va_start(list, format);
	if (gPrintIndent > 0) {
		gBoxPrintWindow->printf("%*s", 4*gPrintIndent, "");
	}
	gBoxPrintWindow->vprintf(format, list);
	gBoxPrintWindow->printf("\r");
	va_end(list);
#else
	va_list list;
    va_start(list, format);
	if (gPrintIndent > 0) {
		for (int i=0; i<gPrintIndent; i++) {
			printf("\t");
		}
	}
	vprintf(format, list);
	printf("\n");
	va_end(list);
#endif
}

void
AdjustBoxIndent(int adjustIndent)
{
	gPrintIndent += adjustIndent;
	if (IsWarning(gPrintIndent < 0)) {
		gPrintIndent = 0;
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
int
GetBoxIndent(void)
{
	return gPrintIndent;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
SetBoxIndent(int setIndent)
{
	gPrintIndent = setIndent;
}
#endif

// ===========================================================================

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __NETWORK_H__
#include "Network.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __SERVICE_H__
#include "Service.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif

// ===========================================================================

typedef void (BoxDebugPrintFunction)(const void* theThis, long theWhatToPrint);
static const void* BoxPrintDebugLookupGlobalAddress(const char* name);
static const char* BoxPrintDebugLookupGlobalType(const char* name);
static BoxDebugPrintFunction* BoxPrintDebugLookupPrintFunction(const char* type);



// global lookup table
struct BoxPrintGlobalEntry
{
	const char*				name;
	const void**			address;
	const char*				type;
};

// type lookup table
struct BoxPrintTypeEntry
{
	const char*				type;
	BoxDebugPrintFunction*	function;
};

const BoxPrintGlobalEntry gBoxPrintGlobalEntry[] =
{
	// var. name		ptr to var.						var. type
	{ "gNetwork", 		(const void**)&gNetwork,		"Network" },
	{ "gRAMCache",		(const void**)&gRAMCache,		"Cache" },
	{ "gServiceList",	(const void**)&gServiceList,	"ServiceList" },
	{ "gSystem",		(const void**)&gSystem,			"System" },
};
const kNumBoxPrintGlobalEntry = sizeof(gBoxPrintGlobalEntry) / sizeof(gBoxPrintGlobalEntry[0]);

const BoxPrintTypeEntry gBoxPrintTypeEntry[] =
{	// var. type		ptr to var's print function
	{ "Cache",			Cache::StaticBoxPrintDebug },
	{ "CacheEntry",		CacheEntry::StaticBoxPrintDebug },
	{ "Network", 		Network::StaticBoxPrintDebug },		
	{ "Resource",		Resource::StaticBoxPrintDebug },
	{ "ServiceList",	ServiceList::StaticBoxPrintDebug },
	{ "System",			System::StaticBoxPrintDebug }
};

const kNumBoxPrintTypeEntry = sizeof(gBoxPrintTypeEntry) / sizeof(BoxPrintTypeEntry);

static const void*
BoxPrintDebugLookupGlobalAddress(const char* name)
{
	const void* address = nil;
	if (name != nil) {
		int index=0;
		while (index<kNumBoxPrintGlobalEntry) {
			if (strcmp(name, gBoxPrintGlobalEntry[index].name) == 0) {
				address = *(gBoxPrintGlobalEntry[index].address);
				break;
			}
			index++;
		}
		if (index == kNumBoxPrintGlobalEntry) {
			long parseAsNumber;
			if (sscanf(name, "%li", &parseAsNumber) == 1) {
				address = (const void*)parseAsNumber;
			}
		}
	}
	return address;
}

static const char*
BoxPrintDebugLookupGlobalType(const char* name)
{
	const char* type = nil;
	if (name != nil) {
		for (int index=0; index<kNumBoxPrintGlobalEntry; index++) {
			if (strcmp(name, gBoxPrintGlobalEntry[index].name) == 0) {
				type = gBoxPrintGlobalEntry[index].type;
				break;
			}
		}
	}
	return type;
}

static BoxDebugPrintFunction* 
BoxPrintDebugLookupPrintFunction(const char* type)
{
	BoxDebugPrintFunction* function = nil;
	if (type != nil) {
		for (int index=0; index<kNumBoxPrintTypeEntry; index++) {
			if (strcmp(type, gBoxPrintTypeEntry[index].type) == 0) {
				function = gBoxPrintTypeEntry[index].function;
				break;
			}
		}
	}
	return function;
}

void
BoxPrintDebugGlobal(const char* type, const char* name)
{
	const void* address = BoxPrintDebugLookupGlobalAddress(name);

	BoxDebugPrintFunction* function = nil;
	if (type != nil) {
		function = BoxPrintDebugLookupPrintFunction(type);
	}
	if (function == nil) {
		type = BoxPrintDebugLookupGlobalType(name);
		if (type != nil) {
			function = BoxPrintDebugLookupPrintFunction(type);
		}
	}
	
	if (function != nil) {
		(*function)(address, 0);
	} else {
		BoxPrint("Can't find print function for type '%s'",
				 (type == nil) ? "nil" : type);
	}
}

#endif /* DEBUG_BOXPRINT */





