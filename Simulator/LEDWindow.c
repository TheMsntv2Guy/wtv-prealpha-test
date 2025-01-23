// ---------------------------------------------------------------------------
//	LEDWindow.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ---------------------------------------------------------------------------

#ifdef DEBUG_LEDWINDOW

#include "Headers.h"

#ifndef __BOXUTILS_H__
#include "BoxUtils.h"
#endif
#ifndef __LEDWINDOW_H__
#include "LEDWindow.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif

// ---------------------------------------------------------------------------
//	globals, locals, prototypes
// ---------------------------------------------------------------------------

LEDWindow* gLEDWindow = nil;

// if the pictures are misaligned, you can adjust them here...
// (be sure to uncomment the OffsetRect() calls where they are used, too...
//
//const kLEDPowerHOffset = 0;
//const kLEDPowerVOffset = 0;
//const kLEDConnectHOffset = 0;
//const kLEDConnectVOffset = 0;
//const kLEDMessageHOffset = 0;
//const kLEDMessageVOffset = 0;


static void DrawLEDBackground(void);
static void DrawLEDPower(Boolean powerOn);
static void DrawLEDMessage(Boolean messageOn);
static void DrawLEDConnect(Boolean connectOn);

// ---------------------------------------------------------------------------
//	Drawing helper functions

static void DrawLEDBackground(void)
{
	PicHandle pictureHandle = GetPicture(rLEDBackgroundPicture);
	if (pictureHandle != nil)
	{	Rect pictureRect = (**pictureHandle).picFrame;
		DrawPicture(pictureHandle, &pictureRect);
	}
}

static void DrawLEDConnect(Boolean connectOn)
{
	short pictureIndex = connectOn ? rLEDConnectOnPicture : rLEDConnectOffPicture;

	PicHandle pictureHandle = GetPicture(pictureIndex);
	if (pictureHandle != nil)
	{	Rect pictureRect = (**pictureHandle).picFrame;
		//OffsetRect(&pictureRect, kLEDPowerHOffset, kLEDPowerVOffset);
		DrawPicture(pictureHandle, &pictureRect);
	}
}

static void DrawLEDMessage(Boolean messageOn)
{
	short pictureIndex = messageOn ? rLEDMessageOnPicture : rLEDMessageOffPicture;

	PicHandle pictureHandle = GetPicture(pictureIndex);
	if (pictureHandle != nil)
	{	Rect pictureRect = (**pictureHandle).picFrame;
		//OffsetRect(&pictureRect, kLEDMessageHOffset, kLEDMessageVOffset);
		DrawPicture(pictureHandle, &pictureRect);
	}
}

static void DrawLEDPower(Boolean powerOn)
{
	short pictureIndex = powerOn ? rLEDPowerOnPicture : rLEDPowerOffPicture;

	PicHandle pictureHandle = GetPicture(pictureIndex);
	if (pictureHandle != nil)
	{	Rect pictureRect = (**pictureHandle).picFrame;
		//OffsetRect(&pictureRect, kLEDPowerHOffset, kLEDPowerVOffset);
		DrawPicture(pictureHandle, &pictureRect);
	}
}

// ---------------------------------------------------------------------------

LEDWindow::LEDWindow()
{
	if (gLEDWindow != nil)
	{	delete gLEDWindow;
	}
	
	ChangeWindowProcID(kLEDWindowProcID);
	
	SetFormat(0, 0, false, false);	// Header, trailer, vscroll, hscroll
	PicHandle pictureHandle = GetPicture(rLEDBackgroundPicture);
	SetTitle("LED Window");
	if (pictureHandle != nil)
	{	Rect pictureRect = (**pictureHandle).picFrame;
		ResizeWindow(pictureRect.right - pictureRect.left,
					 pictureRect.bottom - pictureRect.top);
	}

	if (gMacSimulator != nil)
	{	fBoxLEDs = gMacSimulator->GetBoxLEDs();
	}
	else
	{	fBoxLEDs = 0;
	}
	gLEDWindow = this;
}

LEDWindow::~LEDWindow()
{
	gLEDWindow = nil;
}

void
LEDWindow::SetBoxLEDs(ulong bits)
{
	if (bits != fBoxLEDs)
	{	
		fBoxLEDs = bits;

		if (GetVisible())
		{	GrafPtr savePort;
			GetPort(&savePort);
			SetPort(w);
	
			Draw();
	
			SetPort(savePort);
		}
	}
}

// ---------------------------------------------------------------------------

void
LEDWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);
	MenuHandle menu = GetMenu(mHardware);
	EnableItem(menu, iLEDWindow);

	SetMenuItemText(menu, iLEDWindow,
					GetVisible() ? "\pHide LEDWindow" : "\pShow LEDWindow");
}

Boolean
LEDWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);
	Boolean handled = false;

	if ((theMenu == mHardware) && (theItem == iLEDWindow))
	{
		if (GetVisible())
			HideWindow();
		else
			ShowWindow();
		handled = true;
	}
	return handled || StdWindow::DoMenuChoice(menuChoice, modifiers);
}

void
LEDWindow::Close()
{
	HideWindow();
}

void
LEDWindow::DrawBody(Rect* UNUSED(r), short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	DrawLEDBackground();
	DrawLEDConnect(fBoxLEDs & kBoxLEDConnect);
	DrawLEDMessage(fBoxLEDs & kBoxLEDMessage);
	DrawLEDPower  (fBoxLEDs & kBoxLEDPower  );
}

void
LEDWindow::Idle()
{
	if (GetVisible())
	{	SetBoxLEDs(gMacSimulator->GetBoxLEDs());
	}
}

#endif /* DEBUG_LEDWINDOW */