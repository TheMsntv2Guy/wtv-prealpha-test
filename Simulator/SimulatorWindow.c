// ===========================================================================
//	SimulatorWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __HW_DISPLAY__
#include "HWDisplay.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __SCREEN_H__
#include "Screen.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __SIMULATORWINDOW_H__
#include "SimulatorWindow.h"
#endif
#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif




// ===========================================================================
//	globals
// ===========================================================================

SimulatorWindow* gSimulatorWindow = nil;

static const char kSimulatorWindowUseTVWDEF[] = "Simulator Window: UseTVWDEF";
static const char kSimulatorWindowTitle[] = "Simulator Window";

static const Boolean kDefaultUseTVWDEF = true;


// ===========================================================================
//	implementations
// ===========================================================================

static Boolean GetTVGDHandle(GDHandle& tvGDHandle)
{
	Boolean foundPotentialTV = true;

	tvGDHandle = GetDeviceList();
	while (tvGDHandle != nil &&
			  (	TestDeviceAttribute(tvGDHandle, mainScreen) ||		// main screen
				(!TestDeviceAttribute(tvGDHandle, gdDevType)) ||	// B&W device
				(!TestDeviceAttribute(tvGDHandle, screenDevice)) 	// not screen device
		  	  )
		  )
	{	tvGDHandle = GetNextDevice(tvGDHandle);	// that isn't a TV for us
	}
	
	if (tvGDHandle == nil)
	{	tvGDHandle = GetMainDevice();
		foundPotentialTV = false;
	}
	return foundPotentialTV;
}

SimulatorWindow::SimulatorWindow(void)
{
	if (gSimulatorWindow != nil)
	{	delete gSimulatorWindow;
	}
	gSimulatorWindow = this;

	if (gMacSimulator != nil)
	{	gMacSimulator->SetWindow(w);
	}

	SetFormat(0, 0, false, false);	// no header/trailer, no scroll bars
	SetIsResizable(false);
	
	SetDisplayMode(kDisplayModeNTSC);
	fIsCentered = false;
	
	GDHandle tvGDHandle = nil;
	fUseTVWDEF = GetTVGDHandle(tvGDHandle);

	if (gMacSimulator != nil)
	{	SetUseTVWDEF(gMacSimulator->GetUseTVWDEF());
	}
	else
	{	SetUseTVWDEF(fUseTVWDEF);
	}
	
	gSimulatorWindow->SetTitle(kSimulatorWindowTitle);

	if (!gSimulatorWindow->GetVisible())
	{	CenterWindowOnScreen(tvGDHandle);
		gSimulatorWindow->ShowWindow();
	}
}

SimulatorWindow::~SimulatorWindow()
{
	if (gMacSimulator != nil)
	{	gMacSimulator->SetWindow(nil);
	}
	gSimulatorWindow = nil;
}

void
SimulatorWindow::SetUseTVWDEF(Boolean flag)
{
	fUseTVWDEF = flag;
	ChangeWindowProcID(fUseTVWDEF ? kSimulatorWindowProcIDPhilTV
										: kSimulatorWindowProcIDNormal);
	if (w != nil)
	{	if (fUseTVWDEF)
		{	((WindowPeek)w)->dataHandle = (Handle)0x1;	// set frame to black
		}
		else
		{	((WindowPeek)w)->goAwayFlag = false;	// no close box!!!
		}
	
		if (((WindowPeek)w)->visible)
		{	HideWindow();
			ShowWindow();
		}
	}
	if (gMacSimulator != nil)
	{
		gMacSimulator->SetWindow(w);
	}
}

void
SimulatorWindow::SetDisplayMode(DisplayMode displayMode)
{
	short newWidth, newHeight;

	::SetDisplayMode(displayMode);
	newWidth	= GetDisplayWidth();
	newHeight	 = GetDisplayHeight();
	ResizeWindow(newWidth, newHeight);
	if (fIsCentered)
		CenterWindowOnScreen();
}

void
SimulatorWindow::CenterWindowOnScreen(GDHandle gdHandle)
{
	if (gdHandle == nil)
	{	GetTVGDHandle(gdHandle);
	}
	
	if (gdHandle == nil)
		return;

	short windowWidth = w->portRect.right - w->portRect.left;
	short windowHeight = w->portRect.bottom - w->portRect.top;
	short deviceWidth = (**gdHandle).gdRect.right - (**gdHandle).gdRect.left;
	short deviceHeight = (**gdHandle).gdRect.bottom - (**gdHandle).gdRect.top;

	MoveWindow(w, 
			   (**gdHandle).gdRect.left + ((deviceWidth-windowWidth)/2),
			   (**gdHandle).gdRect.top  + ((deviceHeight-windowHeight)/2),
			   false);
	
	fIsCentered = true;
}

void
SimulatorWindow::Close(void)
{
}

void
SimulatorWindow::Draw(void)
{
	if (gScreen != nil)
	{	
		extern Boolean	gInBackground;
		
		if (!gInBackground)
			UpdateMacPort();
	}
	else
	{
		Rect theRect = w->portRect;
		short familyNumber;
		StringPtr displayStr = "\pStandby for WebTV";
	
		if ((gMacSimulator != nil) && gMacSimulator->GetIsOn())
		{
			SetGray(192);
			PaintRect(&theRect);
		}
		else
		{
			SetGray(0);
			PaintRect(&theRect);
			
			PenNormal();
			MoveTo((theRect.left + theRect.right - StringWidth(displayStr) - 120) / 2, (theRect.top + theRect.bottom) / 2);
			GetFNum("\pBrush Script", &familyNumber);
			if (familyNumber != 0) {
				TextMode(srcXor);
				TextFont(familyNumber);
				TextSize(24);
				DrawString(displayStr);
			}
		}
	}
}

void
SimulatorWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);

	MenuHandle menu;
	menu = GetMenu(mDisplay);
	
	DisableItem(menu, iSaveScreenshot);
	
	if (FrontWindow() == w)
	{	menu = GetMenu(mFile);
		DisableItem(menu, iClose);
	}
}

#if 0

static short LookupVRefNum(short fileRefNum, short *vRefNum)
{
	short error;
	FCBPBRec pb;

	pb.ioFCBIndx = 0;
	pb.ioNamePtr = nil;
	pb.ioRefNum = fileRefNum;
	error = PBGetFCBInfoSync(&pb);
	*vRefNum = (error == noErr) ? pb.ioFCBVRefNum : (short)0;
	return error;
}

extern pascal OSErr Create(ConstStr255Param fileName, short vRefNum, OSType creator, OSType fileType);

static void DumpPort(GrafPtr port)
/* write port contents to a 'PICT' document */
{
	FSSpec		tempSpec;
	FSSpec		destSpec;
	
	OSErr		error;
	long		fileNum = 0;
	short		refNum, vRefNum;
	Str255		name;
	GWorldPtr	savePort;
	GDHandle	saveDevice;
	RgnHandle	saveClip;
	QDProcsPtr	saveProcs;
	PicHandle	screenPicture;
	long		nullBits[128];
	long		count;
	Rect		frameRect;
	
	/* find an unused file name */
	do
		{
		snprintf((char*)name, sizeof(name), "WTV Screen %d", ++fileNum);
		c2pstr((char*)name);
		if ((error = FSOpen(name, 0, &refNum)) == noErr)
			FSClose(refNum);
		}
	while (error != fnfErr);
	
	Create(name, 0, 'ttxt', 'PICT');
	FSOpen(name, 0, &refNum);
	
	/* save state */
	GetGWorld(&savePort, &saveDevice);
	SetGWorld((GWorldPtr)port, nil);
	GetClip(saveClip = NewRgn());
	saveProcs = port->grafProcs;
	
	/* record picture */
	port->grafProcs = nil;
	frameRect = port->portRect;
	InsetRect(&frameRect, -1, -1);
	ClipRect(&frameRect);
	PenNormal();
	screenPicture = OpenPicture(&frameRect);
	CopyBits(&port->portBits, &port->portBits, &port->portRect, &port->portRect, srcCopy, nil);
	FrameRect(&frameRect);
	ClosePicture();
	
	/* restore state */
	port->grafProcs = saveProcs;
	SetClip(saveClip);
	DisposeRgn(saveClip);
	SetGWorld(savePort, saveDevice);
	
	/* write header */
	ZeroMemory(nullBits, 128);
	count = 512;
	FSWrite(refNum, &count, nullBits);
	count = GetHandleSize((Handle)screenPicture);
	FSWrite(refNum, &count, *screenPicture);
	LookupVRefNum(refNum, &vRefNum);
	FSClose(refNum);
	FlushVol(nil, vRefNum);
	
	/* clean up */
	KillPicture(screenPicture);
	
	SysBeep(10);
}

#endif

Boolean
SimulatorWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	Boolean result = false;
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);

	if ((theMenu == mDisplay) && (theItem == iSaveScreenshot))
	{
		//DumpPort();
		//CenterWindowOnScreen();
		result = true;
	}
	return (result || StdWindow::DoMenuChoice(menuChoice, modifiers));
}

void
SimulatorWindow::DragWindow(struct MacPoint where)
{
	fIsCentered = false;
	StdWindow::DragWindow(where);
}

void
SimulatorWindow::ShowWindow(void)
{
	SInt16 paintWhite = LMGetPaintWhite();
	LMSetPaintWhite(0);
	StdWindow::ShowWindow();
	SetPort(w);
	PaintRect(&w->portRect);
	LMSetPaintWhite(paintWhite);
}

Boolean
SimulatorWindow::SavePrefs(StdWindowPrefs* prefPtr)
{
	SimulatorWindowPrefs defaultPrefs;
	
	if (prefPtr == nil)
	{	prefPtr = &defaultPrefs;
	}
	
	((SimulatorWindowPrefs*)prefPtr)->displayMode = GetDisplayMode();
	((SimulatorWindowPrefs*)prefPtr)->isCentered = fIsCentered;
	return StdWindow::SavePrefs(prefPtr);
}

Boolean
SimulatorWindow::RestorePrefs(StdWindowPrefs* prefPtr)
{

	SimulatorWindowPrefs defaultPrefs;

	if (prefPtr == nil)
	{	prefPtr = &defaultPrefs;
	}

	if (!StdWindow::RestorePrefs(prefPtr))
		return false;

	SetDisplayMode(((SimulatorWindowPrefs*)prefPtr)->displayMode);
	if (((SimulatorWindowPrefs*)prefPtr)->isCentered)
	{	CenterWindowOnScreen();
		fIsCentered = true;
	}
	else
	{	fIsCentered = false;
	}
	return true;
}

long
SimulatorWindow::GetPrefsSize(void)
{
	return sizeof(SimulatorWindowPrefs);
}

// ===========================================================================

/*
new simulator window
{
	WindowPeek		simulatorWindow;
	WindowPtr		window;
	struct Rect		boundsRect;
	struct MacPoint	prefTopLeft;
	Boolean			boundsAreGood = false;

	simulatorWindow = (WindowPeek)NewPtrClear(sizeof(WindowRecord));
	if (simulatorWindow == nil)
		EmergencyExit(sInitStatusErr);
	
	if (gMacSimulator->GetPreferencePoint(kPrefsSimulatorWindowPosition, &prefTopLeft))
	{
		boundsRect.top = prefTopLeft.v;
		boundsRect.left = prefTopLeft.h;
		boundsRect.bottom = boundsRect.top + kNTSCHeight;
		boundsRect.right = boundsRect.left + kNTSCWidth;
		boundsAreGood = (GetMaxDevice(&boundsRect) != nil);
	}
	
	if (!boundsAreGood)
	{
		boundsRect.top = 0;
		boundsRect.left = 0;
		boundsRect.bottom = boundsRect.top + kNTSCHeight;
		boundsRect.right = boundsRect.left + kNTSCWidth;
		CenterWindowRect(2048, &boundsRect);
		prefTopLeft.v = boundsRect.top;
		prefTopLeft.h = boundsRect.left;
		gMacSimulator->SetPreferencePoint(kPrefsSimulatorWindowPosition, &prefTopLeft);
	}
	
	window = NewCWindow((Ptr)simulatorWindow,
		&boundsRect, "\pDisplay Monitor", false, 2048, (WindowPtr)-1, false, 0);

	if (window == nil)
		EmergencyExit(sInitStatusErr);
	SetWRefCon(window, rSimulatorWindow);
	
	SInt16 paintWhite = LMGetPaintWhite();

	if (gMacSimulator->GetUseTVMonitor())
	{
		simulatorWindow->dataHandle = (Handle)0x1;	// set frame to black
		LMSetPaintWhite(0);
	}
	ShowWindow(window);
	if (gMacSimulator->GetUseTVMonitor())
	{
		SetPort(window);
		PaintRect(&window->portRect);
	}
	
	LMSetPaintWhite(paintWhite);
	
	gMacSimulator->SetWindow((WindowPtr)simulatorWindow);
}
*/