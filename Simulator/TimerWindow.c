// ===========================================================================
//	TimerWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#error еее Obsolete

#if 0

#include "Headers.h"

#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __TIMERWINDOW_H__
#include "TimerWindow.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif




// ===========================================================================
//	globals
// ===========================================================================

static const Str255 kStartString = "\pStart";
static const Str255 kStopString = "\pStop";
static const Str255 kPauseString = "\pPause";
static const Str255 kResumeString = "\pResume";
static const Str255 kResetString = "\pReset";




// ===========================================================================
//	implementation
// ===========================================================================

TimerWindow::TimerWindow()
{
	SetFormat(32, 0, true, true);	// header space, no trailer, no scroll bars	

	SetDefaultFontID(monaco);
	SetDefaultFontSize(9);
	SetDefaultFontStyle(normal);

	Rect r;
	GetHeaderRect(&r);
	r.top += 4;
	r.bottom = r.top + 20;
	r.left += 4;
	r.right = r.left + 80;
	fStartStopButton = NewControl(w,&r,kStartString,true,0,0,1,pushButProc,0);
	
	r.left += 88;
	r.right += 88;
	fPauseResumeButton = NewControl(w,&r,kPauseString,true,0,0,1,pushButProc,0);

	r.left += 88;
	r.right += 88;
	fResetButton = NewControl(w,&r,kResetString,true,0,0,1,pushButProc,0);
	
	InvalRect(&r);

	fLapTimer = nil;
}

TimerWindow::~TimerWindow()
{
	if (fStartStopButton != nil)
		DisposeControl(fStartStopButton);
	if (fPauseResumeButton != nil)
		DisposeControl(fPauseResumeButton);
	if (fResetButton != nil)
		DisposeControl(fResetButton);
}

void
TimerWindow::SetTimer(LapTimer* timer)
{
	fLapTimer = timer;
}

Boolean
TimerWindow::SavePrefs(StdWindowPrefs* prefPtr)
{
	TimerWindowPrefs defaultPrefs;
	
	if (prefPtr == nil)
	{	prefPtr = &defaultPrefs;
	}
	
	((TimerWindowPrefs*)prefPtr)->hScroll = mLastHScroll;
	((TimerWindowPrefs*)prefPtr)->vScroll = mLastVScroll;
	return StdWindow::SavePrefs(prefPtr);
}

Boolean
TimerWindow::RestorePrefs(StdWindowPrefs* prefPtr)
{

	TimerWindowPrefs defaultPrefs;
	size_t prefSize = GetPrefsSize();

	if (prefPtr == nil)
	{
		prefPtr = &defaultPrefs;
		prefSize = TimerWindow::GetPrefsSize();
	}

	if (!StdWindow::RestorePrefs(prefPtr))
		return false;

	mLastHScroll = ((TimerWindowPrefs*)prefPtr)->hScroll;
	mLastVScroll = ((TimerWindowPrefs*)prefPtr)->vScroll;
	return true;
}

long
TimerWindow::GetPrefsSize(void)
{
	return sizeof(TimerWindowPrefs);
}

static Boolean MaybeSetControlTitle(ControlHandle c, const Str255 title)
{
	Str255 buffer;
	GetControlTitle(c, buffer);
	
	for (int i=0; i<=buffer[0]; i++)
	{
		if (buffer[0] != title[0])
		{
			SetControlTitle(c, title);
			return true;
		}
	}
	return false;
}

void TimerWindow::UpdateButtons(void)
{
	int startStopHilite = 0;
	int pauseResumeHilite = 0;
	
	if (fLapTimer != nil)
	{
		if (fLapTimer->IsActive())
		{
			MaybeSetControlTitle(fStartStopButton, kStopString);
			if (fLapTimer->IsPaused())
			{	
				MaybeSetControlTitle(fPauseResumeButton, kResumeString);
				startStopHilite = 255;	// dim start/stop while paused
			}
			else
			{
				MaybeSetControlTitle(fPauseResumeButton, kPauseString);
			}
		}
		else
		{
			MaybeSetControlTitle(fStartStopButton, kStartString);
			pauseResumeHilite = 255;	// dim pause/resume while inactive
		}
	}
	if ((**fStartStopButton).contrlHilite != startStopHilite)
	{
		HiliteControl(fStartStopButton, startStopHilite);
	}
	if ((**fPauseResumeButton).contrlHilite != pauseResumeHilite)
	{
		HiliteControl(fPauseResumeButton, pauseResumeHilite);
	}
}

void TimerWindow::Click(MacPoint *where, ushort modifiers)
{
	ControlHandle	c;
	
	if ((FindControl(*where,w,&c))
		&& ((c == fStartStopButton)
			|| (c == fPauseResumeButton)
			|| (c == fResetButton)))
	{
		if ((TrackControl(c, *where, nil)) && (fLapTimer != nil))
		{
			if (c == fStartStopButton)
			{
				if (fLapTimer->IsActive())
				{
					fLapTimer->Stop();
				}
				else
				{	fLapTimer->Start();
				}
			}
			else if (c == fPauseResumeButton)
			{
				if (fLapTimer->IsPaused())
				{
					fLapTimer->Resume();
				}
				else
				{	fLapTimer->Pause();
				}
			}
			else if (c == fResetButton)
			{
				fLapTimer->Reset();
				fLapTimer->Start();
			}
		}
	}
	else
	{	StdWindow::Click(where, modifiers);
	}
}

void TimerWindow::DrawHeader(Rect*)
{
}


void TimerWindow::DrawBody(Rect* r, short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	if (fLapTimer != nil)
	{
		GetBodyRect(r);
		EraseRect(r);
		SetGray(0);
		BodyCoordinates(r);

		ulong lines = 2;
		char lineText[256];
	
		sprintf(lineText, " Total Time: %5.2f seconds (%5.2f active, %5.2f paused)",
							(float)fLapTimer->GetTotalTime()/kOneSecond,
							(float)fLapTimer->GetActiveTime()/kOneSecond,
							(float)fLapTimer->GetPauseTime()/kOneSecond);			
				MoveTo(4, lines++ * fLineHeight);
				DrawText(lineText, 0, strlen(lineText));
		
		sprintf(lineText, " Small Laps: (<%d ticks) %5d laps in %5.2f seconds (%5.2f seconds/lap)",
							fLapTimer->GetBigLapCutoff(),
							fLapTimer->GetNumSmallLaps(),
							(float)fLapTimer->GetSmallLapsTime()/kOneSecond,
							((float)fLapTimer->GetSmallLapsTime()/kOneSecond)/fLapTimer->GetNumSmallLaps());			
				MoveTo(4, lines++ * fLineHeight);
				DrawText(lineText, 0, strlen(lineText));
	
		sprintf(lineText, " Big Laps: (>= %d ticks) %5d laps in %5.2f seconds (%5.2f seconds/lap)",
							fLapTimer->GetBigLapCutoff(),
							fLapTimer->GetNumBigLaps(),
							(float)fLapTimer->GetBigLapsTime()/kOneSecond,
							((float)fLapTimer->GetBigLapsTime()/kOneSecond)/fLapTimer->GetNumBigLaps());			
				MoveTo(4, lines++ * fLineHeight);
				DrawText(lineText, 0, strlen(lineText));

		BodyCoordinates(r);
	}
	
	fLastUpdate = Now();
}

void TimerWindow::SetBodyFont(short face, short size, short style)
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	SetDefaultFontID(face);
	SetDefaultFontSize(size);
	SetDefaultFontStyle(style);

	TextFont(GetDefaultFontID());
	TextSize(GetDefaultFontSize());
	TextStyle(GetDefaultFontStyle());
	
	FontInfo	finfo;
	GetFontInfo(&finfo);
	fLineHeight = finfo.ascent + finfo.descent + finfo.leading + 1;
	
	InvalRect(&(w->portRect));
	SetPort(savePort);
}

void TimerWindow::Idle()
{
	Rect r;
	UpdateButtons();
	//if (UpdateButtons())
	//{
	//	r = w->portRect;
	//	InvalRect(&r);
	//}
	//else
	if (Now() > fLastUpdate + (kOneSecond/2))  // wait at least three seconds to update this
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		GetBodyRect(&r);
		InvalRect(&r);
		SetPort(savePort);
	}
}

#endif /* #if 0 */