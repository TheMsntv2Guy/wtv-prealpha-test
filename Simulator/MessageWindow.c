// ===========================================================================
//	MessageWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifdef DEBUG

#include "Headers.h"

#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MESSAGEWINDOW_H__
#include "MessageWindow.h"
#endif
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif




// ===========================================================================
//	local variables & prototypes
// ===========================================================================

DebugMessageWindow*		gDebugMessageWindow = nil;
static char				gCurrentMessage[2048];

static void VPrintMessageCommon(DebugMessageImportance importance,
								const char *format, va_list list);





// ===========================================================================
//	implementations
// ===========================================================================

DebugMessageWindow::DebugMessageWindow(void)
{
	if (gDebugMessageWindow != nil)
		delete gDebugMessageWindow;
	gDebugMessageWindow = this; 

	SetBodyFont(monaco, 9, normal);
	SetBodyTEWriteProtect(true);
	SetItemMark(GetMenu(mDebug), iMessageWindow, ' ');
	fNumQueuedMessages = 0;
	ResizeWindow(500, 110);
	SetTitle("Debug Message Window");
	//SetFormat(100, 0, true, true);	// Header, trailer, vscroll, hscroll
}

DebugMessageWindow::~DebugMessageWindow(void)
{
	gDebugMessageWindow = nil;
}

void
DebugMessageWindow::DoAdjustMenus(ushort modifiers)
{
	TEWindow::DoAdjustMenus(modifiers);
	
	MenuHandle menu = GetMenu(mDebug);
	EnableItem(menu, iMessageWindow);
	SetItemMark(menu, iMessageWindow, GetVisible() ? checkMark : ' ');
}

Boolean
DebugMessageWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);
	Boolean handled = false;

	if ((theMenu == mDebug) && (theItem == iMessageWindow))
	{
		if (GetVisible())
			HideWindow();
		else
			ShowWindow();
		
		handled = true;
	}
	return handled || TEWindow::DoMenuChoice(menuChoice, modifiers);
}

void
DebugMessageWindow::Close(void)
{	
	HideWindow();
}

void
DebugMessageWindow::ShowWindow(void)
{
	TEWindow::ShowWindow();
	MenuHandle menuHdl = GetMenu(mDebug);
	if (menuHdl != nil)
		SetItemMark(menuHdl, iMessageWindow, checkMark);
}

void
DebugMessageWindow::HideWindow(void)
{
	TEWindow::HideWindow();
	MenuHandle menuHdl = GetMenu(mDebug);
	if (menuHdl != nil)
		SetItemMark(menuHdl, iMessageWindow, ' ');
}

void
DebugMessageWindow::AddMessage( DebugMessageSender UNUSED(sender),
								DebugMessageImportance UNUSED(priority),
							    char* message )
{
	// if curr size plus message length is larger than fMaxLength, remove some from beginning
	size_t messageLength = strlen(message);

	message[messageLength] = '\r';	// еее really cheesy way to do carriage returns
	AppendText(message, messageLength + 1);
	message[messageLength] = 0;

	fNumQueuedMessages++;
	
	if (fNumQueuedMessages > kNumQueuedMessagesThreshhold)
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		
		Draw();
		
		SetPort(savePort);
		fNumQueuedMessages = 0;
	}
}

// ---------------------------------------------------------------------------

void
PrintMessage(const char *format, ...)
{
    va_list     	list;

    va_start(list, format);
	VPrintMessageCommon(kNormalImportance, format, list);
	va_end(list);
}
	
void
PrintImportantMessage(const char *format, ...)
{
    va_list     	list;

    va_start(list, format);
	VPrintMessageCommon(kHighImportance, format, list);
	va_end(list);
}
	
void
PrintTrivialMessage(const char *format, ...)
{
    va_list     	list;

	if (!gSimulator->GetTrivialMessagesEnabled())
		return;
		
    va_start(list, format);
	VPrintMessageCommon(kTrivialImportance, format, list);
	va_end(list);
}

// ---------------------------------------------------------------------------

static void
VPrintMessageCommon(DebugMessageImportance importance, const char *format, va_list list)
{
	static DebugMessageImportance previousImportance = kNormalImportance;

	if (importance == kPreviousImportance)
	{
		importance = previousImportance;
	}
	else
	{
		previousImportance = importance;
	}
	
	if ((gDebugMessageWindow != nil) && (gDebugMessageWindow->w)
		&& (((WindowPeek)(gDebugMessageWindow->w))->visible))
	{
		static ulong		gMessageNumber = 0;
	
		VLogMessage(format, list);
		
		gMessageNumber++;
		switch(importance)
		{
			case kTrivialImportance:
				snprintf(gCurrentMessage, sizeof(gCurrentMessage), "\t\t%3d. ", gMessageNumber);
				break;
			case kHighImportance:
				snprintf(gCurrentMessage, sizeof(gCurrentMessage), "е%3d. ", gMessageNumber);
				break;
			case kNormalImportance:
			default:
				snprintf(gCurrentMessage, sizeof(gCurrentMessage), "\t%3d. ", gMessageNumber);
				break;
		}
		ulong	messagePrefixSize = strlen(gCurrentMessage);
		
		vsnprintf(gCurrentMessage + messagePrefixSize, sizeof(gCurrentMessage) - messagePrefixSize, format, list);

		for (int i = 0; i < sizeof(gCurrentMessage); i++)
		{
			if (gCurrentMessage[i] == '\n')
				gCurrentMessage[i] = '\r';
			else if (gCurrentMessage[i] == 0)
				break;
		}

		gDebugMessageWindow->AddMessage(nil, importance, gCurrentMessage);
	}
}

#endif /* DEBUG */