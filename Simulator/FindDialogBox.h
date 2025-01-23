// ===========================================================================
//	FindDialogBox.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __FINDDIALOGBOX_H__
#define __FINDDIALOGBOX_H__

MacBoolean DoFindDialog(void);

MacBoolean GetFindDialogCaseSensitive(void);
MacBoolean GetFindDialogEntireWord(void);
MacBoolean GetFindDialogWrapSearch(void);
const char* GetFindDialogSearchString(void);

void SetFindDialogCaseSensitive(MacBoolean caseSensitive);
void SetFindDialogEntireWord(MacBoolean entireWord);
void SetFindDialogWrapSearch(MacBoolean wrapSearch);
void SetFindDialogSearchString(const char* src);
void SetFindDialogSearchString(const char* src, ulong length);

#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include FindDialogBox.h multiple times"
#endif
#endif /* __FINDDIALOGBOX_H__ */