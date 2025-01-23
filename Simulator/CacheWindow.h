// ===========================================================================
//	CacheWindow.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CACHEWINDOW_H__
#define __CACHEWINDOW_H__

#ifdef DEBUG_CACHEWINDOW

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif

class CacheWindow : public StdWindow
{
public:
							CacheWindow();
	virtual					~CacheWindow();

	virtual void			DoAdjustMenus(ushort modifiers);
	virtual Boolean			DoMenuChoice(long menuChoice, ushort modifiers);
	virtual void			Close();
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			DrawHeader(Rect *r);
	virtual	void			Idle();
	
protected:
			ulong			fLastUpdate;
};

extern CacheWindow* gCacheWindow;

#endif /* #ifdef DEBUG_CACHEWINDOW */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include CacheWindow.h multiple times"
	#endif
#endif /* __CACHEWINDOW_H__ */
