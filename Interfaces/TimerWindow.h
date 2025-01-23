// ===========================================================================
//	TimerWindow.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#error еее Obsolete

#if 0

#ifndef __TIMERWINDOW_H__
#define __TIMERWINDOW_H__

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif

#ifndef __SIMULATORSTATISTICS_H__
#include "SimulatorStatistics.h"
#endif




struct TimerWindowPrefs : public StdWindowPrefs
{
	short			hScroll;
	short			vScroll;
};

class TimerWindow : public StdWindow
{
public:
							TimerWindow(void);
	virtual					~TimerWindow(void);
					
	virtual void			Click(MacPoint *where, ushort modifiers);
	virtual	void			DrawHeader(Rect*);
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			Idle(void);

			void			SetBodyFont(short face, short size, short style);
			void			SetTimer(LapTimer* timer);
	
protected:
	virtual Boolean			SavePrefs(StdWindowPrefs* prefPtr = nil); 
	virtual Boolean			RestorePrefs(StdWindowPrefs* prefPtr = nil); 
	virtual long			GetPrefsSize(void);

			void			UpdateButtons();
			LapTimer*		fLapTimer;
			ulong			fLastUpdate;
			short			fLineHeight;

			struct ControlRecord**	fStartStopButton;
			struct ControlRecord**	fPauseResumeButton;
			struct ControlRecord**	fResetButton;
};

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include TimerWindow.h multiple times"
	#endif
#endif // __TIMERWINDOW_H__

#endif /* #if 0 */