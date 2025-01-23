// ===========================================================================
//	RemoteWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __REMOTEWINDOW_H__
#include "RemoteWindow.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif



// ===========================================================================

static PicHandle		gRemotePictHdl = nil;			
static Boolean			gRemoteCreated = false;

typedef struct
{
	Boolean		isVisible;
	MacPoint	location;
}
RemoteWindowPrefs;

#define kDefaultRemoteWindowV	122
#define kDefaultRemoteWindowH	688

static RemoteWindowPrefs gRemoteWindowPrefs =
		{
			true,
			{kDefaultRemoteWindowV, kDefaultRemoteWindowH}
		};

static const char kPrefsRemoteWindow[] = "Remote Window Visible/Position";

//
// Button positions
//
// Note: these were in the resource file, but it seems easier
// to edit them in decimal here.  --Chris
	
static Rect gRemoteRects[] = 
{
	{28, 21, 51, 53},		// tv/video
	{27, 57, 51, 81},		// web power
	{27, 91, 51, 114},		// tv power
	{74, 27, 106, 64},		// home
	{74, 74, 106, 114}, 	// options
	{126, 29, 153, 65}, 	// recent
	{162, 30, 189, 63}, 	// back
	{126, 77, 153, 108},	// scroll up
	{161, 77, 188, 105},	// scroll down
	{208, 43, 224, 94},		// up
	{218, 21, 265, 43},		// left
	{231, 46, 254, 91},		// execute
	{219, 92, 265, 114},	// right
	{259, 44, 276, 94}	 	// down
};

const ulong kTVVideoButton = 0;
const ulong kWebPowerButton = 1;
const ulong kTVPowerButton = 2;
const ulong kHomeButton = 3;
const ulong kOptionsButton = 4;
const ulong kRecentButton = 5;
const ulong kBackButton = 6;
const ulong kScrollUpButton = 7;
const ulong kScrollDownButton = 8;
const ulong kUpButton = 9;
const ulong kLeftButton = 10;
const ulong kExecuteButton = 11;
const ulong kRightButton = 12;
const ulong kDownButton = 13;

const ulong kNumRects = 14;
	
static long PtInRemoteRects(MacPoint pt)
{
	Rect* rect = gRemoteRects;
	ulong		i, numRects = kNumRects;
	
	for (i = 0; i < numRects; i++)
		if (PtInRect(pt, rect++))
			return i;
			
	return -1;
}

static void DrawRemoteRects()
{
	Rect* rect = gRemoteRects;
	ulong		i, numRects = kNumRects;
	
	for (i = 0; i < numRects; i++)
	{
		//if (i == 1)	/* power */
			FrameRect(rect);
		rect++;
	}
}

void MouseDownInRemoteWindow(WindowPtr window, MacPoint pt, ulong when)
{
	MacPoint		originalPt = pt;
	SetPort(window);
	GlobalToLocal(&pt);
	ulong partNumber = PtInRemoteRects(pt);
	ulong nextKeyTime = Now() + 10;		// key repeat delay
	Input input;
	
	input.rawData = 0;					// could make this look like the raw IR data...
	input.device = kWebTVIRRemote;
	input.modifiers = 0;
	input.time = when;

	// "power on" is the only command that works when
	// the power is off...
	
	if (!gMacSimulator->GetIsInitialized())
	{
		if (partNumber == 1 || partNumber == 2)
			gMacSimulator->PowerOn();
		if (partNumber != (ulong)-1)
			return;
	}
		
	for(;;)
	{
		switch (partNumber)
		{
			case kTVVideoButton:
			case (ulong)-1:
				LocalToGlobal(&pt);
				DragWindow(window, originalPt, &qd.screenBits.bounds);
				gRemoteWindowPrefs.location.h =  window->portRect.left;
				gRemoteWindowPrefs.location.v = window->portRect.top;
				LocalToGlobal(&(gRemoteWindowPrefs.location));
				gRemoteWindowPrefs.isVisible = true;
				gMacSimulator->SetPreference(kPrefsRemoteWindow, &gRemoteWindowPrefs, sizeof(gRemoteWindowPrefs));
				goto Done;
				
			case kOptionsButton:
				input.data = kOptionsKey;
				break;
			case kTVPowerButton:
			case kWebPowerButton:
				input.data = kPowerKey;
				break;
			case kScrollUpButton: 
				input.data = kScrollUpKey;
				break;
			case kScrollDownButton: 
				input.data = kScrollDownKey;
				break;
			case kBackButton: 
				input.data = kBackKey;
				break;			
			case kUpButton:
				input.data = kUpKey;
				break;
			case kLeftButton:
				input.data = kLeftKey;
				break;
			case kRightButton:
				input.data = kRightKey;
				break;
			case kDownButton:
				input.data = kDownKey;
				break;
			case kExecuteButton:
				input.data = kExecuteKey;
				break;
			case kRecentButton:
				input.data = kRecentKey;
				break;
			case kHomeButton:
				input.data = kHomeKey;
				break;
	#ifdef DEBUG
			default: 
				Trespass();
				goto Done;
	#endif
		}
		
		PostInput(&input);

		
		// catch the loop and Mac update on the back side,
		// so we can simulate the auto-repeat
	
		do 
		{
			gMacSimulator->UserLoopIteration();
			//DoUpdate(gMacSimulator->GetWindow());
			if (!StillDown())
				goto Done;
		} while (Now() < nextKeyTime);
		nextKeyTime = Now() + 6;	// key repeat rate
		
	}
	
Done:
	;
}

extern WindowPtr NewPictureWindow(short pictResID);

void
NewRemoteWindow(Boolean fromInit)
{
	WindowPtr	window;
	
	if (gRemotePictHdl == nil)
		gRemotePictHdl = (PicHandle)Get1Resource('PICT', rRemotePicture);

	if (fromInit)
	{
	
		RemoteWindowPrefs *prefPtr;
		size_t size;
		if (gMacSimulator->GetPreference(kPrefsRemoteWindow, &prefPtr, &size))
		{
			if (sizeof(RemoteWindowPrefs) == size)
				gRemoteWindowPrefs = *prefPtr;
		}
		if (!gRemoteWindowPrefs.isVisible)
			return;
	}
			
	window = NewPictureWindow(rRemotePicture);
	if (window == nil)
		EmergencyExit(sInitStatusErr);
	
	MoveWindow(window, gRemoteWindowPrefs.location.h, gRemoteWindowPrefs.location.v, false);
	ShowWindow(window);
	SetPort(window);
	
	Rect	bounds = {0, 0, window->portRect.bottom - window->portRect.top, window->portRect.right - window->portRect.left };
	DrawPicture(GetWindowPic(window), &bounds);
	
	gRemoteWindowPrefs.isVisible = true;
	gMacSimulator->SetPreference(kPrefsRemoteWindow, &gRemoteWindowPrefs, sizeof(RemoteWindowPrefs));
}
	
void
InitRemoteWindows(void)
	{
	NewRemoteWindow(true);
	}

#if 0	
void
DrawRemoteWindow(WindowPtr window,ulong UNUSED(input))
	{
	PicHandle ph;
	
	SetPort(window);
	ph = gRemotePictHdl;

	switch(input)
	{
		case 0:
			ph = gRemotePictHdl;
			break;
		
		case kOptionsKey:
		case kRecentKey:
		case kFavoriteKey:
			ph = gRemotePictHdl;
			break;
			
		case kUpKey:
			ph = gRemotePictHdlUp;
			break;
			
		case kLeftKey:
			ph = gRemotePictHdlLeft;
			break;
			
		case kRightKey:
			ph = gRemotePictHdlRight;
			break;
			
		case kDownKey:
			ph = gRemotePictHdlDown;
			break;
			
		case kExecuteKey:
			ph = gRemotePictHdlExecute;
			break;

		case kBackKey:
			ph = gRemotePictHdlBack;
			break;

		case kPowerKey:
			ph = gRemotePictHdlPower;
			break;

		case kScrollUpKey:
			ph = gRemotePictHdlPageUp;
			break;
			
		case kScrollDownKey:
			ph = gRemotePictHdlPageDown;
			break;
			
		default:
			// don't update for keyboard input
			return;
	}

	DrawPicture(ph, &window->portRect);

	if (!((WindowPeek)window)->hilited)
	{
		struct PenState		penState, oldState;
		RGBColor			grayColor = { 0x37FFF, 0x7FFF, 0x7FFF };
		
		GetPenState(&penState);
		oldState = penState;
		
		RGBForeColor(&grayColor);
		penState.pnMode = subPin;
		SetPenState(&penState);
		
		PaintRect(&window->portRect);
		
		SetPenState(&oldState);
	}
}
#endif

void CloseRemoteWindow(WindowPtr window)
{
	WindowPeek			win;
	
	DisposeWindow(window);
	gRemoteWindowPrefs.isVisible = false;
	
	for (win = (WindowPeek)FrontWindow(); win != nil; win = win->nextWindow)
		if (win->refCon == rRemoteWindow)
			break;

	// see if we can find another remote window still open
	if (win != nil)
	{
		gRemoteWindowPrefs.location.h = win->port.portRect.left; 
		gRemoteWindowPrefs.location.v = win->port.portRect.top;
		gRemoteWindowPrefs.isVisible = true;
	}
	
	gMacSimulator->SetPreference(kPrefsRemoteWindow, &gRemoteWindowPrefs, sizeof(RemoteWindowPrefs));
}
	
