// ---------------------------------------------------------------------------
//	LEDWindow.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ---------------------------------------------------------------------------

#ifndef __LEDWINDOW_H__
#define __LEDWINDOW_H__

// ---------------------------------------------------------------------------

#ifdef DEBUG_LEDWINDOW

#ifndef __STDWINDOW_H__
#include "StdWindow.h"		/* for StdWindow */
#endif

class LEDWindow : public StdWindow
{
	enum { kLEDWindowProcID = documentProc };

public:
					LEDWindow();
	virtual			~LEDWindow();

	ulong			GetBoxLEDs() const { return fBoxLEDs; };
	void			SetBoxLEDs(ulong bits);

	virtual void	DoAdjustMenus(ushort modifiers);
	virtual Boolean	DoMenuChoice(long menuChoice, ushort modifiers);
	virtual void	Close();
	virtual void	DrawBody(Rect* r, short hScroll, short vScroll, Boolean scrolling);
	virtual void	Idle();

protected:
	ulong			fBoxLEDs;
};

extern LEDWindow* gLEDWindow;

#endif /* DEBUG_LEDWINDOW */

// ---------------------------------------------------------------------------

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include ServiceWindow.h multiple times"
	#endif
#endif /* __LEDWINDOW_H__ */