// ===========================================================================
//	TouristWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef DEBUG_TOURISTWINDOW

#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __SOUND_H__
#include "Sound.h"
#endif
#ifndef __TOURIST_H__
#include "Tourist.h"
#endif
#ifndef __TOURISTWINDOW_H__
#include "TouristWindow.h"
#endif


// ===========================================================================
//	globals
// ===========================================================================

static const Str255 kStartString	= "\pStart";
static const Str255 kStopString		= "\pStop";
static const Str255 kPauseString	= "\pPause";
static const Str255 kResumeString	= "\pResume";
static const Str255 kShowString		= "\pShow";
static const Str255 kSkipString 	= "\pSkip";

static const kStartButtonWidth = 65;
static const kStartButtonHeight = 20;
static const kStopButtonWidth = 65;
static const kStopButtonHeight = 20;
static const kPauseButtonWidth = 65;
static const kPauseButtonHeight = 20;
static const kResumeButtonWidth = 65;
static const kResumeButtonHeight = 20;
static const kShowButtonWidth = 65;
static const kShowButtonHeight = 20;
static const kSkipButtonWidth = 65;
static const kSkipButtonHeight = 20;

static const kTouristWindowHeader = 64;

static const kControlActive = 0;
static const kControlInactive = 255;

static RGBColor kColumnTitleColor			= {0x0000, 0x0000, 0x0000};
static RGBColor kItinerarySiteColor			= {0x0000, 0x0000, 0x3f3f};
static RGBColor kItineraryRetrySiteColor	= {0x7f7f, 0x0000, 0x3f3f};
static RGBColor kJournalSiteColor			= {0x0000, 0x3f3f, 0x0000};
static RGBColor kJournalRetriedSiteColor	= {0x3f3f, 0x3f3f, 0x0000};
static RGBColor kJournalFailedSiteColor		= {0x7f7f, 0x3f3f, 0x0000};
static RGBColor kCurrentSiteColor 			= {0xffff, 0x0000, 0x0000};

static const kDepthColumnOffset = 10;
static const kRetriesColumnOffset = 65;
static const kCommentColumnOffset = 125;
static const kURLColumnOffset = 250;

static const kDefaultTouristFontID = geneva;
static const kDefaultTouristFontSize = 9;
static const kDefaultTouristLineHeight = 12;	// a guess based on Font ID/Size




static void MyHiliteControl(ControlRecord** control, Boolean active);
static void DrawTouristColumnTitles(MacPoint* where);
static void DrawTouristItinerary(MacPoint* where, TouristItinerary* itinerary);
static void DrawTouristJournal(MacPoint* where, TouristJournal* journal);
static void DrawTouristSite(MacPoint* where, TouristSite* site, RGBColor* color);




// ===========================================================================
//	helper functions
// ===========================================================================

static void
MyHiliteControl(ControlRecord** control, Boolean active)
{
	if (control != nil) {
		int hilite = active ? kControlActive : kControlInactive;
		
		if ((Byte)hilite != (**control).contrlHilite) {
			HiliteControl(control, hilite);
			Draw1Control(control);
		}
	}
}

static void
DrawTouristColumnTitles(MacPoint* where)
{
	RGBForeColor(&kColumnTitleColor);

	const char depthString[] = "Depth";
	const char retriesString[] = "NumRetries";
	const char commentString[] = "Comment";
	const char URLString[] = "URL";
	
	MoveTo(where->h + kDepthColumnOffset, where->v);
	DrawText(depthString, 0, sizeof(depthString)-1);
	
	MoveTo(where->h + kRetriesColumnOffset, where->v);
	DrawText(retriesString, 0, sizeof(retriesString)-1);
	
	MoveTo(where->h + kCommentColumnOffset, where->v);
	DrawText(commentString, 0, sizeof(commentString)-1);
	
	MoveTo(where->h + kURLColumnOffset, where->v);
	DrawText(URLString, 0, sizeof(URLString)-1);

	where->v += kDefaultTouristLineHeight;
}

static void
DrawTouristItinerary(MacPoint* where, TouristItinerary* itinerary)
{
	if (itinerary == nil)
		return;

	TouristItineraryIterator* iterator =
		(TouristItineraryIterator*)itinerary->NewIterator();

	TouristSite* site = (TouristSite*)(iterator->GetFirst());
	while (site != nil) {
		RGBColor *color = &kItinerarySiteColor;
		if (site->GetRetries() > 0) {
			color = &kItineraryRetrySiteColor;
		}
		DrawTouristSite(where, site, color);
		site = (TouristSite*)iterator->GetNext();
	}
	
	delete (iterator);
}

static void
DrawTouristJournal(MacPoint* where, TouristJournal* journal)
{
	if (journal == nil)
		return;
	
	TouristJournalIterator* iterator =
		(TouristJournalIterator*)journal->NewIterator();
	
	TouristSite* site = (TouristSite*)iterator->GetFirst();
	while (site != nil) {
		RGBColor *color = &kJournalSiteColor;
		if (!site->GetVisited()) {
			color = &kJournalFailedSiteColor;
		} else if (site->GetRetries() > 0) {
			color = &kJournalRetriedSiteColor;
		}
		DrawTouristSite(where, site, color);
		site = (TouristSite*)iterator->GetNext();
	}
	delete (iterator);
}

static void
DrawTouristSite(MacPoint* where, TouristSite* site, RGBColor* color)
{
	if (site == nil)
		return;
	
	RGBForeColor(color);

	char numString[20];
	const char* commentString = site->GetComment();
	const char* URLString = site->GetURL();
	
	MoveTo(where->h + kDepthColumnOffset, where->v);
	DrawText(numString, 0,
			 snprintf(numString, sizeof(numString), "%ld", (long)(site->GetDepth())));
	
	MoveTo(where->h + kRetriesColumnOffset, where->v);
	DrawText(numString, 0,
			 snprintf(numString, sizeof(numString), "%ld", (long)(site->GetRetries())));
	
	if (commentString != nil) {
		MoveTo(where->h + kCommentColumnOffset, where->v);
		DrawText(commentString, 0, strlen(commentString));
	}
	
	if (URLString != nil) {
		MoveTo(where->h + kURLColumnOffset, where->v);
		DrawText(URLString, 0, strlen(URLString));
	}
	where->v += kDefaultTouristLineHeight;
}

// ===========================================================================
//	implementation
// ===========================================================================

TouristWindow::TouristWindow()
{
	SetFormat(kTouristWindowHeader, 0, true, true);
		// header space, no trailer, scroll bars	

	SetDefaultFontID(kDefaultTouristFontID);
	SetDefaultFontSize(kDefaultTouristFontSize);

	Rect r;
	GetHeaderRect(&r);
	r.top += 4;
	r.bottom = r.top + kStartButtonHeight;
	r.left += 13;
	r.right = r.left + kStartButtonWidth;
	fStartButton = NewControl(w,&r,kStartString,true,0,0,1,pushButProc,0);
	
	r.bottom = r.top + kStopButtonHeight;
	r.left = r.right + 13;
	r.right = r.left + kStopButtonWidth;
	fStopButton = NewControl(w,&r,kStopString,true,0,0,1,pushButProc,0);
	
	r.bottom = r.top + kPauseButtonHeight;
	r.left = r.right + 13;
	r.right = r.left + kPauseButtonWidth;
	fPauseButton = NewControl(w,&r,kPauseString,true,0,0,1,pushButProc,0);
	
	r.bottom = r.top + kResumeButtonHeight;
	r.left = r.right + 13;
	r.right = r.left + kResumeButtonWidth;
	fResumeButton = NewControl(w,&r,kResumeString,true,0,0,1,pushButProc,0);

	r.bottom = r.top + kShowButtonHeight;
	r.left = r.right + 13;
	r.right = r.left + kShowButtonWidth;
	fShowButton = NewControl(w,&r,kShowString,true,0,0,1,pushButProc,0);

	r.bottom = r.top + kSkipButtonHeight;
	r.left = r.right + 13;
	r.right = r.left + kSkipButtonWidth;
	fSkipButton = NewControl(w,&r,kSkipString,true,0,0,1,pushButProc,0);

	InvalRect(&r);	// force redraw of buttons

	mVLineScroll = kDefaultTouristLineHeight;

	fTourist = (Tourist*)0x00000001;
}

TouristWindow::~TouristWindow()
{
	if (fStartButton != nil)
		DisposeControl(fStartButton);
	if (fStopButton != nil)
		DisposeControl(fStopButton);
	if (fPauseButton != nil)
		DisposeControl(fPauseButton);
	if (fResumeButton != nil)
		DisposeControl(fResumeButton);
	if (fShowButton != nil)
		DisposeControl(fShowButton);
	if (fSkipButton != nil)
		DisposeControl(fSkipButton);
}

void
TouristWindow::Click(MacPoint* where, ushort modifiers)
{

	ControlHandle	c;
	
	if ((FindControl(*where,w,&c))
		&& ((c == fStartButton)
			|| (c == fStopButton)
			|| (c == fPauseButton)
			|| (c == fResumeButton)
			|| (c == fShowButton)
			|| (c == fSkipButton))) {
		
		if (TrackControl(c, *where, nil)) {		
			if (c == fStartButton) {
				gPageViewer->ExecuteURL("client:Tourist");
			} else if (c == fStopButton) {
				gPageViewer->ExecuteURL("client:Tourist?Stop");
			} else if (c == fPauseButton) {
				gPageViewer->ExecuteURL("client:Tourist?Pause");
			} else if (c == fResumeButton) {
				gPageViewer->ExecuteURL("client:Tourist?Resume");
			} else if (c == fShowButton) {
				gPageViewer->ExecuteURL("client:Tourist?Show");
			} else if (c == fSkipButton) {
				gPageViewer->ExecuteURL("client:Tourist?Skip");
			}
		}
	} else {
		StdWindow::Click(where, modifiers);
	}
}

void
TouristWindow::DrawHeader(Rect* r)
{
	Rect eraseRect = *r;
	eraseRect.top += 26; // move beyond those buttons

	EraseRect(r);
	DrawControls(w);

	const kVertOffset = 42;
	const kHorzOffset = 4;
	
	long loadTime;
	long showTime;
	long maxRetries;
	Boolean scrollThrough;
	long depth = Tourist::GetDefaultDepth();

	if (gTourist == nil) {
		loadTime = Tourist::GetDefaultLoadTime();
		showTime = Tourist::GetDefaultShowTime();
		maxRetries = Tourist::GetDefaultMaxRetries();
		scrollThrough = Tourist::GetDefaultScrollThrough();
	} else {
		loadTime = gTourist->GetLoadTime();
		showTime = gTourist->GetShowTime();
		maxRetries = gTourist->GetMaxRetries();
		scrollThrough = gTourist->GetScrollThrough();
	}	
	
	char buffer[256];
	char littleBuffer[40] = "No active tour";
	
	if (gTourist != nil) {
		TouristItinerary* itinerary = gTourist->GetTouristItinerary();
		TouristJournal*	journal = gTourist->GetTouristJournal();
		
		if ((itinerary != nil) && (journal != nil)) {
			int completed = journal->GetCount() + 1;
			int total = completed + itinerary->GetCount();
			snprintf(littleBuffer, sizeof(littleBuffer), "%d of %d", completed, total);
		}
	}

	snprintf(buffer, sizeof(buffer),
			 "%s: (%s)"
			 "  LoadTime = %ld ticks"
			 "  ShowTime = %ld ticks"
			 "  MaxRetries = %ld"
			 "  ScrollThrough = %s",
			(gTourist == nil) ? "Default" : "Current",
			littleBuffer,
			loadTime,
			showTime,
			maxRetries,
			scrollThrough ? "yes" : "no");
	
	struct MacPoint topLeft;
	topLeft.v = r->top + kVertOffset;
	topLeft.h = r->left;

	MoveTo(topLeft.h + kHorzOffset, topLeft.v);
	DrawText(buffer, 0, strlen(buffer));
	
	topLeft.v += (5*kDefaultTouristLineHeight)/4;
	DrawTouristColumnTitles(&topLeft);
}


void
TouristWindow::DrawBody(Rect* r, short hScroll, short vScroll, Boolean UNUSED(scrolling))
{
	EraseRect(r);

	if (gTourist == nil) {
		fLastDrawTime = Now();
		return;
	}
	
	TouristItinerary* itinerary = gTourist->GetTouristItinerary();
	TouristSite* site = gTourist->GetTouristSite();
	TouristJournal*	journal = gTourist->GetTouristJournal();

	MacPoint topLeft;
	topLeft.h = r->left - hScroll;	// scoot it in from margin
	topLeft.v = r->top - vScroll;	// scoot it down from top

	RGBColor saveColor;
	GetForeColor(&saveColor);

		topLeft.v += (3*kDefaultTouristLineHeight)/2;
	DrawTouristJournal(&topLeft, journal);
		topLeft.v += kDefaultTouristLineHeight/2;
	DrawTouristSite(&topLeft, site, &kCurrentSiteColor);
		topLeft.v += kDefaultTouristLineHeight/2;
	DrawTouristItinerary(&topLeft, itinerary);
	
	short bodyHeight = topLeft.v + vScroll - r->top;
	SetBodySize(768, bodyHeight);
	
	RGBForeColor(&saveColor);
}

void
TouristWindow::Idle(void)
{
	if (!GetVisible())
		return;

	if ((gTourist != fTourist) ||
		(gTourist != nil) && (gTourist->GetDebugModifiedTime() > fLastDrawTime)) {

		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
	
		InvalRect(&(w->portRect));
		
		SetPort(savePort);
		
		fTourist = gTourist;
		fLastDrawTime = Now();

		// adjust buttons
		Boolean isOn = gMacSimulator->GetIsOn();
		MyHiliteControl(fStartButton, isOn && (gTourist == nil));
		MyHiliteControl(fStopButton, isOn && (gTourist != nil));
		MyHiliteControl(fPauseButton, isOn && (gTourist != nil) && !gTourist->GetPaused());
		MyHiliteControl(fResumeButton, isOn && (gTourist != nil) && gTourist->GetPaused());
		MyHiliteControl(fShowButton, isOn && (gTourist != nil));
		TouristState touristState = (gTourist == nil) ? kTouristStateNone :
										gTourist->GetTouristState();
		MyHiliteControl(fSkipButton,
							isOn && ((touristState == kTouristStateWaitCompleted)
									|| (touristState == kTouristStateWaitShow)
									|| (touristState == kTouristStateScrollDown)
									|| (touristState == kTouristStateScrollUp)));
	}
}

#endif /* DEBUG_TOURISTWINDOW */











