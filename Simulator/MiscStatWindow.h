// ===========================================================================
//	MiscStatWindow.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MISCSTATWINDOW_H__
#define __MISCSTATWINDOW_H__

#ifdef DEBUG_MISCSTATWINDOW

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif	/* __STDWINDOW_H__ */

class MiscStatWindow : public StdWindow
{
public:
							MiscStatWindow();
	virtual					~MiscStatWindow();
	
	virtual void			DoAdjustMenus(ushort modifiers);
	virtual Boolean			DoMenuChoice(long menuChoice, ushort modifiers);

	virtual void			Click(struct MacPoint *where, ushort modifiers);
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			Idle(void);
	virtual void			Close(void);
	
	void					DoRoundtrip(void);
	static void				Roundtrip(void);

	enum { kNumRoundtrips = 200};
	enum { kNumMaxRoundtrips = 10};
	
protected:
	void					DrawMemoryTitle();
	void					DrawCacheTitle();
	void					DrawCurrentURLTitle();
	void					DrawPreviousURLTitle();
	void					DrawRoundtripTitle();
	void					DrawMaxRoundtripTitle();

	void					DrawMemoryBar();
	void					DrawCacheBar();
	void					DrawCurrentURL();
	void					DrawPreviousURL();
	void					DrawRoundtripAvg();
	void					DrawRoundtrip(int index);
	void					DrawRoundtrips();
	void					DrawMaxRoundtrips();

	ulong					fRAMUsed;
	ulong					fRAMFree;
	ulong					fCacheUsage;
	const char*				fCurrURL;
	const char*				fPrevURL;
	ushort					fCurrRoundtripIndex;
	ulong					fCurrRoundtripStart;
	ushort					fRoundtrips[kNumRoundtrips];
	ulong					fRoundtripAvg;
	ushort					fMaxRoundtrips[kNumMaxRoundtrips];
};

extern MiscStatWindow* gMiscStatWindow;

#endif /* DEBUG_MISCSTATWINDOW */

// -----------------------------------------------------------------------------------

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MiscStatWindow.h multiple times"
	#endif
#endif /* __MISCSTATWINDOW_H__ */
