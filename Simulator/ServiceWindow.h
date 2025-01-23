// ---------------------------------------------------------------------------
//	ServiceWindow.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ---------------------------------------------------------------------------

#ifndef __SERVICEWINDOW_H__
#define __SERVICEWINDOW_H__

#ifdef DEBUG_SERVICEWINDOW

#ifndef __NETWORK_H__
#include "Network.h"		/* for NetState */
#endif
#ifndef __STDWINDOW_H__
#include "StdWindow.h"		/* for StdWindow */
#endif

class ServiceWindow : public StdWindow
{
public:
						ServiceWindow(void);
	virtual				~ServiceWindow(void);

	virtual void		DoAdjustMenus(ushort modifiers);
	virtual Boolean		DoMenuChoice(long menuChoice, ushort modifiers);
	virtual void		Close();

	virtual void		DrawBody(Rect* r, short hScroll, short vScroll, Boolean scrolling);
	virtual void		DrawHeader(Rect* r);
	virtual void		Idle();

protected:
	NetState			fLastNetState;
	ulong				fLastUpdate;
};

extern ServiceWindow* gServiceWindow;

#endif /* DEBUG_SERVICEWINDOW */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include ServiceWindow.h multiple times"
	#endif
#endif /* __SERVICEWINDOW_H__ */