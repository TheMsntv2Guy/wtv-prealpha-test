// ===========================================================================
//	TouristWindow.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __TOURISTWINDOW_H__
#define __TOURISTWINDOW_H__

#ifdef DEBUG_TOURISTWINDOW

#ifndef DEBUG_TOURIST
#error "Can't have DEBUG_TOURISTWINDOW unless you #define DEBUG_TOURIST, first!"
#endif

#include "Headers.h"

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif
#ifndef __TOURIST_H__
#include "Tourist.h"
#endif

class TouristWindow : public StdWindow
{
public:
							TouristWindow(void);
	virtual					~TouristWindow(void);

	static void				Initialize(void);
	static void				Finalize(void);
		
	virtual void			Click(MacPoint* where, ushort modifiers);
	virtual void			DrawHeader(Rect* r);
	virtual void			DrawBody(Rect* r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			Idle(void);

protected:
	struct ControlRecord**	fStartButton;
	struct ControlRecord**	fStopButton;
	struct ControlRecord**	fPauseButton;
	struct ControlRecord**	fResumeButton;
	struct ControlRecord**	fShowButton;
	struct ControlRecord**	fSkipButton;

	Tourist*				fTourist;
	ulong					fLastDrawTime;
};

#endif /* DEBUG_TOURISTWINDOW */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include TouristWindow.h multiple times"
	#endif
#endif /* __TOURISTWINDOW_H__ */
