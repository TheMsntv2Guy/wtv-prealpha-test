// ===========================================================================
//	MacintoshDialogs.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MACINTOSHDIALOGS_H__
#define __MACINTOSHDIALOGS_H__

#ifndef __SYSTEM_H__
#include "System.h"
#endif

Boolean SetFontSizeRecord(FontSizeRecord* result, ulong font, const FontSizeRecord* defaultValues);
Boolean SetServerConfiguration(void);

extern const char kPrefsDevelopmentServerName[];
extern const char kPrefsDevelopmentUserID[];

Boolean AlertUser(short dialogID, Boolean allowCancel = false);
Boolean QueryUser(short dialogID, char *result,Boolean isDefault=false);
Boolean QueryUser2(short dialogID, char *result1, char *result2);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MacintoshDialogs.h multiple times"
	#endif
#endif /* __MACINTOSHDIALOGS_H__ */