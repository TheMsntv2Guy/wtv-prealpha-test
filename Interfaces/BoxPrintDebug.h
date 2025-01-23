// ===========================================================================
//	BoxPrintDebug.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __BOXPRINTDEBUG_H__
#define __BOXPRINTDEBUG_H__

#ifdef DEBUG_BOXPRINT

typedef enum
{
	kDebugCookieNone,
	kDebugCookieNetwork,
	kDebugCookieCache,
	kDebugCookieServiceList
	
} DebugCookie;

// ===========================================================================

void	BoxPrintCookieStart(const DebugCookie cookie);
void	BoxPrintCookieEnd(const DebugCookie cookie);
void	BoxPrint(const char* format, ...);
void	AdjustBoxIndent(int adjustIndent);
int		GetBoxIndent(void);
void	SetBoxIndent(int setIndent);

void BoxPrintDebugGlobal(const char* type, const char* name);

// ===========================================================================

#endif /* DEBUG_BOXPRINT */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include BoxPrintDebug.h multiple times"
	#endif
#endif

// ===========================================================================
