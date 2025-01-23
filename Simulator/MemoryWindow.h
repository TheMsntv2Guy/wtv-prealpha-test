// ===========================================================================
//	MemoryWindow.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MEMORYWINDOW_H__
#define __MEMORYWINDOW_H__

#ifdef DEBUG_MEMORYWINDOW

void InitializeMemoryWindows(void);
void DrawMemoryWindow(struct GrafPort* window);
void MouseDownInMemoryWindow(struct GrafPort* window, MacPoint pt, Boolean optionKeyDown);
void NewMemoryWindow(long top, long left, ulong baseOffset, ulong length, ulong windowNumber);
void MemoryWindowChanged(struct GrafPort* window);
void CloseMemoryWindow(struct GrafPort* window);
void IdleMemoryWindows(void);
void MemoryWindowHandleKey(struct GrafPort* window, char key);

#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MemoryWindow.h multiple times"
	#endif
#endif /* __MEMORYWINDOW_H__ */
