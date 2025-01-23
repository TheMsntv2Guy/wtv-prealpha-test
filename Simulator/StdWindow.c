// ===========================================================================
//	StdWindow.c
// 	portions ©1995 by Peter Barrett, All rights reserved.
//  © 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================
//	A standard window has the following layout

/*
		+-------------------------------+
		|			HEADER				|
		+-------------------------------+
		|							  |Æ|
		|							  | |
		|							  | |
		|			BODY			  | |
		|							  | |
		|							  | |
		|							  | |
		|							  |v|
		+-------------------------------+
		| TRAILER  |<|				|>|*|
		+-------------------------------+
		
		It may have a Header for status etc
		It may have a vertical and horizontal scroll bar
		It may have a Trailer for status etc (trailer is same height as scrollbar)

*/
// ===========================================================================


#include "Headers.h"

#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif
#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif
#ifndef __STDWINDOW_PRIVATE_H__
#include "StdWindow.Private.h"
#endif




// ===========================================================================
//	#defines
// ===========================================================================
#define ABS(_x) ((_x) < 0 ? -(_x) : (_x))
#define MIN(_x,_y) (((_x) < (_y)) ? (_x) : (_y))
#define MAX(_x,_y) (((_x) > (_y)) ? (_x) : (_y))


// ===========================================================================
//	local prototypes
// ===========================================================================
static ControlActionUPP gStdWindowScrollAction = 0;

static short IsOnWindow(Rect *r);
static void NewWindowRect(Rect *r);
static void DrawGrowIconOnly(WindowPtr w);
static pascal void StdWindowScrollAction(ControlHandle c, short part);

static short
IsOnWindow(Rect *r)
{
	GrafPtr oldPort;
	WindowPtr	w;
	MacPoint	pt;
	
	GetPort(&oldPort);
	w = FrontWindow();
	while (w) {
		SetPort(w);
		pt.h = pt.v = 0;
		LocalToGlobal(&pt);
		if ((r->top == pt.v) && (r->left == pt.h)) {
			SetPort(oldPort);
			return 1;
		}
		w = (WindowPtr)((WindowPeek)w)->nextWindow;
	}
	SetPort(oldPort);
	return 0;
}

static void
NewWindowRect(Rect *r)
{
	short	h = 0;
	Rect	sr = qd.screenBits.bounds;
	short	maxTop = sr.top + ((sr.bottom - sr.top)*3)/4;
	do {
		SetRect(r,100+h,100,400,300);
		while (IsOnWindow(r)) {
			OffsetRect(r,16,16);
			if (r->top > maxTop)
				break;
		}
		h += 16;
	}
	while (r->top > maxTop);
}

static void
DrawGrowIconOnly(WindowPtr w)
{
	Rect r = w->portRect;
	r.top = r.bottom - 15;
	r.left = r.right - 15;
	RgnHandle saveClipRgn = NewRgn();
	GetClip(saveClipRgn);
	ClipRect(&r);
	DrawGrowIcon(w);
	SetClip(saveClipRgn);
	DisposeRgn(saveClipRgn);
}

static pascal
void StdWindowScrollAction(ControlHandle c, short part)
{
	StdWindow *w = (StdWindow *)GetCRefCon(c);
	w->ScrollAction(c,part);
}

// ===========================================================================
//	Create a new window, don't show it yet

StdWindow::StdWindow()
{
	Rect r;

	NewWindowRect(&r);
	w = NewCWindow(nil, &r, "\pWindow", false, kDefaultStdWindowProcID,
				   (WindowPtr)-1, true, 0);
	SetWRefCon(w,(long)this);

	mHeaderHeight = 0;
	mTrailerWidth = 0;
	
	mWidth = r.right - r.left;
	mHeight = r.bottom - r.top;
	fVScroll = nil;
	fHScroll = nil;
	mLastHScroll = 0;
	mLastVScroll = 0;
	mHLineScroll = 1;
	mVLineScroll = 1;
	mRed = 0xff;
	mGreen = 0xff;
	mBlue = 0xff;
	mActiveScroll = true;
	mWindowKind = 'WIND';
	
	SetIsResizable(true);
	SetMinMaxRect(nil);
	SetDefaultFontID(kStdWindowDefaultFontID);
	SetDefaultFontSize(kStdWindowDefaultFontSize);
	SetDefaultFontStyle(kStdWindowDefaultFontStyle);

	mUsePrefs = true;
	mPrefsName = nil;	// i.e., use window's name as prefs name
}

StdWindow::~StdWindow()
{
	DisposeWindow(w);
}

void
StdWindow::ChangeWindowProcID(short newProcID)
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);

	Rect r;
	r = w->portRect;
	struct MacPoint upperLeft;
	upperLeft.h = r.left;
	upperLeft.v = r.top;
	LocalToGlobal(&upperLeft);
	Str255 oldWindowTitle;
	GetWTitle(w, oldWindowTitle);
	Boolean visible = ((WindowPeek)w)->visible;
	
	DisposeWindow(w);
	fVScroll = nil;
	fHScroll = nil;
	w = NewCWindow(nil, &r, oldWindowTitle, false, newProcID,
				   (WindowPtr)-1, true, 0);
	MoveWindow(w, upperLeft.h, upperLeft.v, false);
	SetWRefCon(w,(long)this);
	if (visible)
		::ShowWindow(w);

	SetPort(savePort);
}

// ----------------------------------------------------------
//	Mick's wacky event handling stuff
// ----------------------------------------------------------
Boolean
StdWindow::HandleEvent(EventRecord* event)
{
	Boolean eventHandled = false;
	
	switch (event->what)
	{
		case nullEvent:
			eventHandled = DoNullEvent(event);
			break;
		case mouseDown:
			eventHandled = DoMouseDownEvent(event);
			break;
		case mouseUp:
			eventHandled = DoMouseUpEvent(event);
			break;
		case keyDown:
			eventHandled = DoKeyDownEvent(event);
			break;
		case keyUp:
			eventHandled = DoKeyUpEvent(event);
			break;
		case autoKey:
			eventHandled = DoAutoKeyEvent(event);
			break;
		case updateEvt:
			if ((WindowPtr)w == (WindowPtr)(event->message))
			{
				eventHandled = DoUpdateEvent(event);
			}
			break;
		case diskEvt:
			eventHandled = DoDiskEvent(event);
			break;
		case activateEvt:
			if ((WindowPtr)w == (WindowPtr)(event->message))
			{
				eventHandled = DoActivateEvent(event);
			}
			break;
		case osEvt:
			eventHandled = DoOSEvent(event);
			break;
		case kHighLevelEvent:
			eventHandled = DoHighLevelEvent(event);
			break;
		default:
			Complain(("StdWindow::HandleEvent didn't understand event->what = %d",
					 event->what));
			break;
	}
	return eventHandled;
}

void StdWindow::DoAdjustMenus(ushort UNUSED(modifiers))
{
}

Boolean StdWindow::DoMenuChoice(long UNUSED(menuChoice), ushort UNUSED(modifiers))
{
	return false;
}

Boolean StdWindow::DoNullEvent(EventRecord* UNUSED(event))
{
	Idle();
	return false;	// let others idle, too
}

Boolean StdWindow::DoMouseDownEvent(EventRecord* event)
{
	Boolean eventHandled = false;
	WindowPtr window;
	short part = FindWindow(event->where, &window);
	
	switch (part)
	{
		case inDesk:
			break;
		case inMenuBar:
			AdjustMenus(event->modifiers);
			WindowPtr aWindow = LMGetWindowList();
			while (aWindow != nil)
			{
				StdWindow* stdWindowPtr = GetStdWindow(aWindow);
				if (stdWindowPtr != nil)
				{	stdWindowPtr->DoAdjustMenus(event->modifiers);
				}
				aWindow = (WindowPtr)(((WindowPeek)aWindow)->nextWindow);
			}

			long menuChoice = MenuSelect(event->where);
			aWindow = LMGetWindowList();
			while ((aWindow != nil) && !eventHandled)
			{
				StdWindow* stdWindowPtr = GetStdWindow(aWindow);
				if (stdWindowPtr != nil)
				{	eventHandled = stdWindowPtr->DoMenuChoice(menuChoice, event->modifiers);
				}
				aWindow = (WindowPtr)(((WindowPeek)aWindow)->nextWindow);
			}
			
			if (!eventHandled)
			{	DoMenuCommand(menuChoice, event->modifiers);
				eventHandled = true;
			}
			
			HiliteMenu(0);
			break;
		case inSysWindow:
			break;
		case inContent:
			if (window == w)	// if it was this window
			{
				WindowPtr frontWindow = FrontWindow();
				if (frontWindow != w)	// if this window isn't in front
				{	SelectWindow();
					eventHandled = true;
				}
				else
				{
					SetPort(window);
					GlobalToLocal(&(event->where));
					Click(&(event->where), event->modifiers);
					eventHandled = true;
				}
			}
			break;
		case inDrag:
			if (window == w)
			{	DragWindow(event->where);
				eventHandled = true;
			}
			break;
		case inGrow:
			if (window == w)
			{	GrowWindow(event->where);
				eventHandled = true;
			}
			break;
		case inGoAway:
			if (window == w)
			{	if (TrackGoAway(window, event->where))
				{	Close();
				}
				eventHandled = true;
			}
			break;
		case inZoomIn:
		case inZoomOut:
			if (window == w)
			{	if (TrackBox(w, event->where, part))
				{	ZoomWindow(part);
				}
				eventHandled = true;
			}
			break;
		default:
			Complain(("StdWindow::DoMouseDownEvent() didn't understand part %d returned by FindWindow", part));
			break;
	}
	return eventHandled;
}

Boolean StdWindow::DoMouseUpEvent(EventRecord* UNUSED(event))
{
	return false;
}

Boolean StdWindow::DoKeyDownEvent(EventRecord* event)
{
	Boolean eventHandled = false;
	
	if ((event->modifiers & cmdKey) != 0)
	{
		WindowPtr aWindow;
		long menuChoice;
		char key = event->message & charCodeMask;
		
		if (IsArrowKey(key))
			return false;
		
		// everyone gets a chance to adjust the menus
		AdjustMenus(event->modifiers);
		aWindow = LMGetWindowList();
		while (aWindow != nil)
		{
			StdWindow* stdWindowPtr = GetStdWindow(aWindow);
			if (stdWindowPtr != nil)
			{	stdWindowPtr->DoAdjustMenus(event->modifiers);
			}
			aWindow = (WindowPtr)(((WindowPeek)aWindow)->nextWindow);
		}

		// did we pick one?
		menuChoice = MenuKey(key);
		if (HiWord(menuChoice) == 0)
			return false;
			
		// everyone gets a chance to handle the menu item
		aWindow = LMGetWindowList();
		while ((aWindow != nil) && !eventHandled)
		{
			StdWindow* stdWindowPtr = GetStdWindow(aWindow);
			if (stdWindowPtr != nil)
			{	eventHandled = stdWindowPtr->DoMenuChoice(menuChoice, event->modifiers);
			}
			aWindow = (WindowPtr)(((WindowPeek)aWindow)->nextWindow);
		}
		
		if (!eventHandled)
		{	DoMenuCommand(menuChoice, event->modifiers);
			eventHandled = true;
		}
		
		HiliteMenu(0);
	}
	
	return eventHandled;
}

Boolean StdWindow::DoKeyUpEvent(EventRecord* UNUSED(event))
{
	return false;
}

Boolean StdWindow::DoAutoKeyEvent(EventRecord* event)
{
	return DoKeyDownEvent(event);
}

Boolean StdWindow::DoUpdateEvent(EventRecord* UNUSED(event))
{
	BeginUpdate(w);
	if (!EmptyRgn(w->visRgn))
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		Draw();
		SetPort(savePort);
	}
	EndUpdate(w);
	return true;
}

Boolean StdWindow::DoDiskEvent(EventRecord* UNUSED(event))
{
	return false;
}

Boolean StdWindow::DoActivateEvent(EventRecord* event)
{
	if (event->modifiers & activeFlag)
	{
		if (fVScroll != nil)
			ShowControl(fVScroll);
		if (fHScroll != nil)
			ShowControl(fHScroll);
	}
	else
	{
		if (fVScroll != nil)
			HideControl(fVScroll);
		if (fHScroll != nil)
			HideControl(fHScroll);
	}
	//SetPort(w);
	//EraseRect(&w->portRect);
	//InvalRect(&w->portRect);
	DrawControls(w);
	return true;
}

Boolean StdWindow::DoOSEvent(EventRecord* UNUSED(event))
{
	return false;
}

Boolean StdWindow::DoHighLevelEvent(EventRecord* UNUSED(event))
{
	return false;
}

void StdWindow::Close(void)
{
	if (mUsePrefs)
		SavePrefs();
	delete(this);
}

// ----------------------------------------------------------
//	Mick's wacky preference-saving stuff
// ----------------------------------------------------------
Boolean
StdWindow::GetUsePrefs(void)
{
	return mUsePrefs;
}

void
StdWindow::SetUsePrefs(Boolean usePrefs)
{
	mUsePrefs = usePrefs;
}

const char*
StdWindow::GetPrefsName(void)
{
	if (mPrefsName == nil)
	{
		static char prefsName[256];
		GetWTitle(w, (StringPtr)prefsName);
		p2cstr((StringPtr)prefsName);
		return &(prefsName[0]);
	}
	return mPrefsName;
}

void
StdWindow::SetPrefsName(const char* name)
{
	mPrefsName = name;
}

Boolean
StdWindow::SavePrefs(StdWindowPrefs* prefPtr)
{
	if (!mUsePrefs)
		return false;

	StdWindowPrefs defaultPrefs;
	size_t prefSize = GetPrefsSize();
	
	if (prefPtr == nil)
	{
		prefPtr = &defaultPrefs;
		prefSize = StdWindow::GetPrefsSize();
	}
	
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	prefPtr->windowLocation.v = w->portRect.top;
	prefPtr->windowLocation.h = w->portRect.left;
	LocalToGlobal(&(prefPtr->windowLocation));
	
	prefPtr->windowSize.v = w->portRect.bottom - w->portRect.top;
	prefPtr->windowSize.h = w->portRect.right - w->portRect.left;
	
	prefPtr->isVisible = GetVisible();
	
	SetPort(savePort);
	
	return gSimulator->SetPreference(GetPrefsName(), prefPtr, prefSize);
}

Boolean
StdWindow::RestorePrefs(StdWindowPrefs* prefPtr)
{
	if ((!mUsePrefs) || (w==nil))
		return false;

	StdWindowPrefs defaultPrefs;
	size_t prefSize = GetPrefsSize();
	
	if (prefPtr == nil)
	{
		prefPtr = &defaultPrefs;
		prefSize = StdWindow::GetPrefsSize();
	}

	void* src;
	size_t sizeFound;
	if (!gSimulator->GetPreference(GetPrefsName(), &src, &sizeFound))
		return false;
		
	if (sizeFound > prefSize)
	{
		Complain(("Preferences record was %d bytes...only have space for %d", sizeFound, prefSize));
	}
	memcpy(prefPtr, src, prefSize);

	MoveWindow(w, prefPtr->windowLocation.h, prefPtr->windowLocation.v, false);
	ResizeWindow(prefPtr->windowSize.h, prefPtr->windowSize.v);
	if (prefPtr->isVisible)
		ShowWindow();
	else
		HideWindow();
	
	return true;
}

long
StdWindow::GetPrefsSize(void)
{
	return sizeof(StdWindowPrefs);
}

// ----------------------------------------------------------

long StdWindow::GetWindowType(void)
{
	return 'WIND';
}

void StdWindow::SetTitle(const char* title)
{
	char name[256];
	snprintf(name, sizeof(name), "%.*s", sizeof(name)-1, title);
	c2pstr(name);
	SetWTitle(w, (StringPtr)name);
	RestorePrefs();
}

void StdWindow::SetActiveScroll(Boolean activeScroll)
{
	mActiveScroll = activeScroll;
}

//	Set the color of the windows contents
//	Keep red, green and blue around for scrollrect

void StdWindow::SetContentColor(short r, short g, short b)
{
	Handle h = GetResource('wctb',1024);
	WCTabHandle c = (WCTabHandle)h;
	(*c)->wCSeed = GetCTSeed();
	(*c)->ctTable[0].rgb.red = (r << 8);
	(*c)->ctTable[0].rgb.green = (g << 8);
	(*c)->ctTable[0].rgb.blue = (b << 8);
		
	mRed = r;
	mGreen = g;
	mBlue = b;
}

// ===========================================================================
//	Establishes the format of the window

void StdWindow::SetFormat(short headerHeight, short trailerWidth, Boolean vScroll, Boolean hScroll)
{
	Rect	r = {0,0,0,0};
	mHeaderHeight = headerHeight;
	mTrailerWidth = trailerWidth;
	
//	Allocate scroll bars

	HidePen();
	if (vScroll)
	{	if (!fVScroll)
		{	fVScroll = NewControl(w,&r,nil,((WindowRecord*)w)->hilited,0,0,0,scrollBarProc,0);
			SetCRefCon(fVScroll,(long)this);
		}
	}
	else
	{
		if (fVScroll)
		{	DisposeControl(fVScroll);
		}
		fVScroll = nil;
	}
	if (hScroll)
	{	if (fHScroll == nil)
		{	fHScroll = NewControl(w,&r,nil,((WindowRecord*)w)->hilited,0,0,0,scrollBarProc,0);
			SetCRefCon(fHScroll,(long)this);
		}
	}
	else
	{
		if (fHScroll != nil)
		{	DisposeControl(fHScroll);
		}
		fHScroll = nil;
	}
	ShowPen();
	mLastHScroll = 0;
	mLastVScroll = 0;
}

void
StdWindow::SetDefaultFontID(short font)
{	fDefaultFontID = font;
}
void
StdWindow::SetDefaultFontSize(short size)
{	fDefaultFontSize = size;
}
void
StdWindow::SetDefaultFontStyle(short style)
{	fDefaultFontStyle = style;
}
short
StdWindow::GetDefaultFontID(void)
{	return fDefaultFontID;
}
short
StdWindow::GetDefaultFontSize(void)
{	return fDefaultFontSize;
}
short
StdWindow::GetDefaultFontStyle(void)
{	return fDefaultFontStyle;
}

//	All rectangles are based on the current window size

void StdWindow::GetHeaderRect(Rect *r)
{
	*r = w->portRect;
	r->bottom = r->top + mHeaderHeight;
}

void StdWindow::GetBodyRect(Rect *r)
{
	*r = w->portRect;
	r->top += mHeaderHeight;
	r->right -= fVScroll ? 15 : 0;
	if (fHScroll != nil || mTrailerWidth)
		r->bottom -= 15;
}

void StdWindow::GetTrailerRect(Rect *r)
{
	*r = w->portRect;
	r->top = r->bottom - 15;
	r->right = r->left + mTrailerWidth;
}

void StdWindow::GetVScrollRect(Rect *r)
{
	*r = w->portRect;
	r->top += mHeaderHeight - 1;
	r->right += 1;
	r->left = r->right - 16;
	r->bottom -= 14;
}

void StdWindow::GetHScrollRect(Rect *r)
{
	*r = w->portRect;
	r->left += mTrailerWidth;
	r->bottom += 1;
	r->top = r->bottom - 16;
	r->right -= 14;
}

Boolean StdWindow::GetIsResizable(void)
{
	return mResizable;
}

void StdWindow::SetIsResizable(Boolean resizable)
{
	mResizable = resizable;
}

Boolean StdWindow::GetVisible(void)
{
	if (w == nil)
		return false;
	return ((WindowPeek)w)->visible;
}

// ===========================================================================
//	Drawing the window structure

void StdWindow::Idle()
{
}

void StdWindow::Draw()
{
	GrafPtr assertPort;
	GetPort(&assertPort);
	Assert(assertPort == w);	// who set the port to us?

	TextFont(fDefaultFontID);
	TextFace(fDefaultFontStyle);
	TextSize(fDefaultFontSize);

	Rect	r;

	ClipRect(&w->portRect);
	DrawControls(w);
	if (((WindowRecord*)w)->hilited && (fHScroll != nil || fVScroll != nil))
	{
		//Complain(("hlite @ %08x", (int)&((WindowRecord*)w)->hilited));
		DrawGrowIconOnly(w);	// Only draw grow if there are controls
	}

	if (mHeaderHeight != 0)
	{
		GetHeaderRect(&r);			// Draw the header
		ClipRect(&r);
		DrawHeader(&r);
		MoveTo(r.left,r.bottom-3);
		LineTo(r.right,r.bottom-3);
		MoveTo(r.left,r.bottom-1);
		LineTo(r.right,r.bottom-1);
	}

	if (mTrailerWidth != 0)
	{	GetTrailerRect(&r);			// Draw the trailer
		ClipRect(&r);
		DrawTrailer(&r);
	}
	
	GetBodyRect(&r);			// Draw the body
	ClipRect(&r);
	DrawBody(&r,mLastHScroll,mLastVScroll,false);
	
	ClipRect(&w->portRect);
}

// ===========================================================================

void StdWindow::DrawHeader(Rect *r)
{
	EraseRect(r);
	InsetRect(r,1,1);
	FrameRect(r);
	InsetRect(r,2,2);
	FrameRect(r);
}
	
void	StdWindow::DrawTrailer(Rect *r)
{
	EraseRect(r);
	InsetRect(r,1,1);
	FrameRect(r);
	InsetRect(r,2,2);
	FrameRect(r);
}

//	Make body 0,0 coordinates Quickdraws 0,0
//	Preserves clip, sets r to be visible part of body

void StdWindow::BodyCoordinates(Rect *r)
{
	RgnHandle	rgn;
	GetBodyRect(r);
	rgn = NewRgn();
	GetClip(rgn);
	SetOrigin(-(r->left - mLastHScroll),-(r->top - mLastVScroll));
	OffsetRgn(rgn,mLastHScroll - r->left,mLastVScroll - r->top);
	OffsetRect(r,mLastHScroll - r->left,mLastVScroll - r->top);
	SectRect(&(*rgn)->rgnBBox,r,r);
	SetClip(rgn);
	DisposeRgn(rgn);
}

void StdWindow::DrawBody(Rect *r, short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	short	width = mWidth/8;
	short	height = mHeight/8;
	short	x,y;
	Rect	br;
	char	str[256];

	SetGray(128);
	PaintRect(r);
	SetGray(0);
	
	BodyCoordinates(r);
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			SetRect(&br,0,0,width,height);
			OffsetRect(&br,x*width,y*height);
			InsetRect(&br,1,1);
			FrameRect(&br);
			MoveTo(br.left + 2, br.top + (height - 12)/2);
			DrawText(str,0,snprintf(str,sizeof(str),"Body [%hd,%hd]",x,y));
		}
	}
	SetOrigin(0,0);
}

// ===========================================================================
//	Do something based on the scroll

void StdWindow::ScrollAction(ControlHandle c, short part)
{
	short	line,page,last,now,s;
	Rect	r;
	
	GetBodyRect(&r);
	if (c == fHScroll) {
		line = mHLineScroll;
		page = r.right - r.left;
		last = mLastHScroll;
	} else {
		line = mVLineScroll;
		page = r.bottom - r.top;
		last = mLastVScroll;
	}
	
	switch (part) {
		case inUpButton:	s = -line;					break;
		case inDownButton:	s = line;					break;
		case inPageUp:		s = -page;					break;
		case inPageDown:	s = page;					break;
		case inThumb:		s = GetCtlValue(c) - last;	break;
		default:
			return;
	}
	s = s + last;
	s = (s / line) * line;		// force to a multiple of lines
	SetCtlValue(c,s);
	if (last != (now = GetCtlValue(c)))	{
		if (c == fHScroll) {
			mLastHScroll = now;
			ScrollBody(last - now,0);
		} else {
			mLastVScroll = now;
			ScrollBody(0,last - now);
		}
	}
}

//	Scroll the body around

void StdWindow::ScrollBody(short h, short v)
{
	RgnHandle rgn = NewRgn();
	Rect	r;
	GetBodyRect(&r);
	SetBackColor(mRed,mGreen,mBlue);	// Color of content
	ScrollRect(&r,h,v,rgn);
	SetBackGray(0xFF);
	SetClip(rgn);
	DrawBody(&r,mLastHScroll,mLastVScroll,true);	// Inform Draw we are scrolling
	DisposeRgn(rgn);
	ClipRect(&w->portRect);
}

void StdWindow::ActiveScroll(ControlHandle c, MacPoint *where)
{
	short	now,last;
	MacPoint	m,lastm;
	long	value;
	
	short original = GetCtlValue(c);
	short units = GetCtlMax(c) - GetCtlMin(c);
	short barSize;
	short line = (c == fVScroll) ? mVLineScroll : mHLineScroll;

	Rect r = (*c)->contrlRect;
	if (c == fVScroll) {
		barSize = (r.bottom - r.top) - 2*15 - 18;	// Up and down arrows, thumb
		::InsetRect(&r,-25,-120);					// Tracking slop rect
	} else {
		barSize = (r.right - r.left) - 2*15 - 18;	// Left and right arrows, thumb
		::InsetRect(&r,-120,-25);					// Tracking slop rect
	}
	
//	Do Active scrolling

	while (StillDown()) {
		GetMouse(&m);
		if (::EqualPt(m,lastm))
			continue;
		
		if (::PtInRect(m,&r)) {						// Scrollbar sloprect behavior
			value = (c == fVScroll) ? (m.v - where->v) : (m.h - where->h);
			value = original + value*units/barSize;
		} else
			value = original;
		value = value / line * line;
		SetCtlValue(c,value);
		
		if (c == fVScroll) {
			last = mLastVScroll;
			if (last != (now = GetCtlValue(c)))	{
				mLastVScroll = now;
				PenNormal();
				ScrollBody(0,last - now);
			}
		} else {
			last = mLastHScroll;
			if (last != (now = GetCtlValue(c)))	{
				mLastHScroll = now;
				PenNormal();
				ScrollBody(last - now,0);
			}
		}
		lastm = m;
	}
}

// ===========================================================================
//	Track the scroll bar clicks
//	StdScrollAction just calls the ScrollAction method

void StdWindow::Click(MacPoint *where, ushort /* modifiers */)
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	ControlHandle	c;
	short	part;
	
	if ((part = FindControl(*where,w,&c)) != 0)
	{
		if (c == fHScroll || c == fVScroll)
		{
			if (!gStdWindowScrollAction)
				gStdWindowScrollAction = NewControlActionProc(&StdWindowScrollAction);	// Allocate scroll hook
			if (part == inThumb)
			{

				if (mActiveScroll)
					ActiveScroll(c,where);								// Scroll Active
				else
					if ((part = TrackControl(c,*where,nil)) != 0)		// Scroll Passive
						ScrollAction(c,part);
			}
			else
				TrackControl(c,*where,gStdWindowScrollAction);
		}
	}
	
	SetPort(savePort);
}

void StdWindow::DragWindow(struct MacPoint where)
{
	::DragWindow(w, where, &qd.screenBits.bounds);
}

// ===========================================================================
//	Handle a grow
	
//	Set the maximum and minimum grow size for the window

void StdWindow::GetMinMaxRect(Rect* r)
{
	*r = mMinMaxRect;
}


void StdWindow::SetMinMaxRect(Rect *r)
{
	const kMaxStdWindowWidth = 4000;
	const kMaxStdWindowHeight = 4000;
	if (r == nil)
	{
		short minWidth = fHScroll ? MAX(kMinWindowWidth,mTrailerWidth + 80) : MAX(kMinWindowWidth,mTrailerWidth);
		short minHeight = fVScroll ? MAX(kMinWindowHeight,mHeaderHeight + 80) : MAX(kMinWindowHeight,mHeaderHeight);
		SetRect(&mMinMaxRect, minWidth, minHeight, kMaxStdWindowWidth, kMaxStdWindowHeight);
	}
	else
	{	mMinMaxRect = *r;
	}
}

void StdWindow::GrowWindow(struct MacPoint where)
{
	if (GetIsResizable())
	{
		Rect	r;
		
		//if (!(fHScroll || fVScroll)) return;
		GetMinMaxRect(&r);
		r.bottom += 1;
		r.right += 1;
		long result = ::GrowWindow(w, where, &r);
		ResizeWindow(result & 0x0FFFF,result >> 16);
		InvalRect(&w->portRect);
	}
}

void StdWindow::HideWindow()
{
	::HideWindow(w);
}

void StdWindow::SelectWindow()
{
	::SelectWindow(w);
}

void StdWindow::ShowWindow()
{
	if (!GetVisible())
	{	::ShowWindow(w);
		SelectWindow();
	}
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	InvalRect(&(w->portRect));
	
	SetPort(savePort);
}

void StdWindow::ZoomWindow(short part)
{
	if (GetIsResizable())
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		EraseRect(&w->portRect);
		::ZoomWindow(w, part, false);	// false => don't bring zoomed window to front
		RecalcScrollBars();	// recalc scroll bars, etc.
		InvalRect(&w->portRect);
		SetPort(savePort);
	}
}

//	Paint the color of the windows content

void StdWindow::PaintContent(Rect *r)
{
	SetColor(mRed,mGreen,mBlue);	// Color of content
	PaintRect(r);
	SetGray(0x00);
}

// ===========================================================================
//	Resize window and controls

void StdWindow::RecalcScrollBars(void)
{
	Rect r;

	//HidePen();
	if (fHScroll != nil) {
		GetHScrollRect(&r);
		MoveControl(fHScroll,r.left,r.top);
		SizeControl(fHScroll,r.right - r.left, r.bottom - r.top);
		r.right += 15;
		InvalRect(&r);
	}
	if (fVScroll) {
		GetVScrollRect(&r);
		MoveControl(fVScroll,r.left,r.top);
		SizeControl(fVScroll,r.right - r.left, r.bottom - r.top);
		r.bottom += 15;
		InvalRect(&r);
	}
	SetBodySize(mWidth,mHeight);
	//ShowPen();
}

void StdWindow::ResizeWindow(short width,short height)
{
	Rect r;
	
	SetPort(w);
	
//	Erase Scroll bars if the window is growing
	if (fHScroll != nil && (w->portRect.bottom - w->portRect.top) < height) {
		GetHScrollRect(&r);
		r.right += 16;
		PaintContent(&r);
		GetTrailerRect(&r);
		PaintContent(&r);
	}
	if (fVScroll && (w->portRect.right - w->portRect.left) < width) {
		GetVScrollRect(&r);
		r.bottom += 16;
		PaintContent(&r);
	}
	
	SizeWindow(w, width, height, true);
	
	RecalcScrollBars();
}

#if 0
/*
//	Fit a full open window on screen
void StdWindow::FitOnScreen(short wantWidth, short wantHeight)
{
	Rect	sr = qd.screenBits.bounds
	Rect	r;
	MacPoint	pt;

	SetPort(w);
	pt.h = pt.v = 0;
	LocalToGlobal(&pt);
	GetMinMaxRect(&r);
	r.top = pt.v;
	r.left = pt.h;
	r.bottom = wantHeight ? wantHeight : r.bottom;
	r.right = wantWidth ? wantWidth : r.right;
	r.bottom += r.top;
	r.right += r.left;
	InsetRect(&sr,8,8);
	SectRect(&sr,&r,&r);
	ResizeWindow(r.right - r.left,r.bottom - r.top);
}
*/
#endif

// ===========================================================================
//	Set the dimensions of the body, redraw as required

void StdWindow::SetBodySize(long width, long height)
{
	long	scroll;
	Rect	r;
	Rect	scrollR;
	
	GetBodyRect(&r);
	mWidth = width;
	mHeight = height;
	HidePen();
	if (fHScroll != nil) {
		scroll = MAX(0,width - (r.right - r.left));
		if (GetCtlMax(fHScroll) != scroll)
		{
			SetCtlMax(fHScroll,scroll);
			GetHScrollRect(&scrollR);
			InvalRect(&scrollR);
		}
		mLastHScroll = MIN(scroll,mLastHScroll);
	}
	if (fVScroll) {
		scroll = MAX(0,height - (r.bottom - r.top));
		if (GetCtlMax(fVScroll) != scroll)
		{
			SetCtlMax(fVScroll,scroll);
			GetVScrollRect(&scrollR);
			InvalRect(&scrollR);
		}
		mLastVScroll = MIN(scroll,mLastVScroll);
	}
	ShowPen();
}

void
StdWindow::DeleteAll(void)
{
	WindowPtr thisWindow = LMGetWindowList();
	while (thisWindow != nil)
	{
		StdWindow* stdWindowPtr = GetStdWindow(thisWindow);
		if (stdWindowPtr != nil)
		{	
			delete stdWindowPtr;
			thisWindow = LMGetWindowList();	// start again from beginning
											// because maybe nextWindow got
											// deleted by thisWindow 
		}
		else
		{	// grab the next one
			thisWindow = (WindowPtr)(((WindowPeek)thisWindow)->nextWindow);
		}
	}
}

void
StdWindow::IdleAll()
{
	GrafPtr savePort;
	GetPort(&savePort);
	
	WindowPeek thisWindow = (WindowPeek)LMGetWindowList();
	while (thisWindow != nil)
	{
		WindowPeek nextWindow = thisWindow->nextWindow;
		
		StdWindow* stdWindowPtr = GetStdWindow((WindowPtr)thisWindow);
		if (stdWindowPtr != nil)
		{	stdWindowPtr->Idle();
		}
		thisWindow = nextWindow;
	}
	
	SetPort(savePort);
}

void
StdWindow::SavePrefsAll()
{
	WindowPeek thisWindow = (WindowPeek)LMGetWindowList();
	while (thisWindow != nil)
	{
		WindowPeek nextWindow = thisWindow->nextWindow;
		
		StdWindow* stdWindowPtr = GetStdWindow((WindowPtr)thisWindow);
		if (stdWindowPtr != nil)
		{	stdWindowPtr->SavePrefs();
		}
		thisWindow = nextWindow;
	}
}


StdWindow*
StdWindow::GetStdWindow(WindowPtr windowPtr) 
{
	StdWindow*	stdWindowPtr = (StdWindow*)GetWRefCon(windowPtr);
	long windowKind = stdWindowPtr->mWindowKind;
	
	for (int i=0; i<kNumStdWindowTypes; i++)
	{	if (windowKind == kStdWindowTypes[i])
			return stdWindowPtr;
	}
	return nil;
}