// ===========================================================================
//	MiscStatWindow.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef DEBUG_MISCSTATWINDOW

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MISCSTATWINDOW_H__
#include "MiscStatWindow.h"
#endif




MiscStatWindow* gMiscStatWindow = nil;

// ===========================================================================

static const kFirstColumn = 4;
static const kSecondColumn = 80;
static const kLineHeight = 12;
static const kLineSpacing = 15;

static const RGBColor kForeColor	= {0x5f5f, 0x1f1f, 0x1f1f};
static const RGBColor kForeColor2	= {0x1f1f, 0x5f5f, 0x1f1f};
static const RGBColor kBackColor	= {0xbfbf, 0xbfbf, 0xbfbf};
static const RGBColor kTextColor	= {0x0000, 0x0000, 0x0000};

//

static const kMemoryTitleHOffset = kFirstColumn;
static const kMemoryTitleVOffset = 4 + kLineHeight;

static const kMemoryBarHOffset = kSecondColumn;
static const kMemoryBarVOffset = 4 + (kLineHeight/4);

static const kMemoryBarHeight = kLineHeight/2;
static const kMemoryBarWidth = 200;

static const RGBColor& kRAMUsedColor = kForeColor;
static const RGBColor& kRAMFreeColor = kBackColor;
static const RGBColor& kRAMOverheadColor = kForeColor2;

//

static const kCacheTitleHOffset = kFirstColumn;
static const kCacheTitleVOffset = kMemoryTitleVOffset + kLineSpacing;

static const kCacheBarHOffset = kSecondColumn;
static const kCacheBarVOffset = kMemoryBarVOffset + kLineSpacing;

static const kCacheBarHeight = kLineHeight/2;
static const kCacheBarWidth = 200;

static const RGBColor& kCacheUsedColor = kForeColor;
static const RGBColor& kCacheUnusedColor = kBackColor;

//

static const kCurrentURLTitleHOffset = kFirstColumn;
static const kCurrentURLTitleVOffset = kCacheTitleVOffset + kLineSpacing;

static const kCurrentURLHOffset = kSecondColumn;
static const kCurrentURLVOffset = kCurrentURLTitleVOffset;

//

static const kPreviousURLTitleHOffset = kFirstColumn;
static const kPreviousURLTitleVOffset = kCurrentURLTitleVOffset + kLineSpacing;

static const kPreviousURLHOffset = kSecondColumn;
static const kPreviousURLVOffset = kPreviousURLTitleVOffset;

//

static const kRoundtripTitleHOffset = kFirstColumn;
static const kRoundtripTitleVOffset = kPreviousURLTitleVOffset + kLineSpacing;

static const kRoundtripAvgHOffset = kRoundtripTitleHOffset;
static const kRoundtripAvgVOffset = kRoundtripTitleVOffset + kLineSpacing;

static const kRoundtripBarHOffset = kSecondColumn;
static const kRoundtripBarVOffset = kRoundtripTitleVOffset - (3*kLineHeight/4);

static const kRoundtripBarHeight = 60;
static const kRoundtripBarWidth = MiscStatWindow::kNumRoundtrips;

static const RGBColor& kRoundtripForeColor = kForeColor;
static const RGBColor& kRoundtripBackColor = kBackColor;

//

static const kMaxRoundtripTitleHOffset = kFirstColumn;
static const kMaxRoundtripTitleVOffset = kRoundtripBarVOffset + kRoundtripBarHeight + kLineSpacing;

static const kMaxRoundtripHOffset = kSecondColumn;
static const kMaxRoundtripVOffset = kMaxRoundtripTitleVOffset;


// ===========================================================================

MiscStatWindow::MiscStatWindow()
{
	if (gMiscStatWindow != nil)
		delete gMiscStatWindow;

	SetFormat(0, 0, false, false);	// Header, trailer, vscroll, hscroll
	SetDefaultFontID(geneva);
	SetDefaultFontSize(9);
	SetDefaultFontStyle(normal);
	ResizeWindow(400, kMaxRoundtripVOffset + kLineSpacing);
	SetTitle("MiscStatWindow");

	gMiscStatWindow = this;
}

MiscStatWindow::~MiscStatWindow()
{
	gMiscStatWindow = nil;
}

void
MiscStatWindow::DrawMemoryTitle()
{
	const char title[] = "Memory:";
	RGBForeColor(&kTextColor);
	MoveTo(kMemoryTitleHOffset, kMemoryTitleVOffset);
	DrawText(title, 0, sizeof(title)-1);
}

void
MiscStatWindow::DrawCacheTitle()
{
	const char title[] = "Cache:"; 
	RGBForeColor(&kTextColor);
	MoveTo(kCacheTitleHOffset, kCacheTitleVOffset);
	DrawText(title, 0, sizeof(title)-1);
}

void
MiscStatWindow::DrawCurrentURLTitle()
{
	const char title[] = "CurrURL:"; 
	RGBForeColor(&kTextColor);
	MoveTo(kCurrentURLTitleHOffset, kCurrentURLTitleVOffset);
	DrawText(title, 0, sizeof(title)-1);
}

void
MiscStatWindow::DrawPreviousURLTitle()
{
	const char title[] = "PrevURL:"; 
	RGBForeColor(&kTextColor);
	MoveTo(kPreviousURLTitleHOffset, kPreviousURLTitleVOffset);
	DrawText(title, 0, sizeof(title)-1);
}

void
MiscStatWindow::DrawRoundtripTitle()
{
	const char title[] = "Roundtrips:"; 
	RGBForeColor(&kTextColor);
	MoveTo(kRoundtripTitleHOffset, kRoundtripTitleVOffset);
	DrawText(title, 0, sizeof(title)-1);
}

void
MiscStatWindow::DrawMaxRoundtripTitle()
{
	const char title[] = "Max roundtrips:"; 
	RGBForeColor(&kTextColor);
	MoveTo(kMaxRoundtripTitleHOffset, kMaxRoundtripTitleVOffset);
	DrawText(title, 0, sizeof(title)-1);
}

void
MiscStatWindow::DrawMemoryBar()
{
	Rect RAMUsedRect;
	Rect RAMFreeRect;
	Rect RAMOverheadRect;

	RAMUsedRect.top = kMemoryBarVOffset;
	RAMUsedRect.left = kMemoryBarHOffset;
	RAMUsedRect.bottom = kMemoryBarVOffset + kMemoryBarHeight;
	RAMUsedRect.right = kMemoryBarHOffset +
						((fRAMUsed * kMemoryBarWidth) / gMacSimulator->GetRAMSize());

	RAMFreeRect.top = RAMUsedRect.top;
	RAMFreeRect.left = RAMUsedRect.right;
	RAMFreeRect.bottom = RAMUsedRect.bottom;
	RAMFreeRect.right = kMemoryBarHOffset +
						(((fRAMUsed+fRAMFree) * kMemoryBarWidth) / gMacSimulator->GetRAMSize());

	RAMOverheadRect.top = RAMFreeRect.top;
	RAMOverheadRect.left = RAMFreeRect.right;
	RAMOverheadRect.bottom = RAMFreeRect.bottom;
	RAMOverheadRect.right = kMemoryBarHOffset + kMemoryBarWidth;

	RGBForeColor(&kRAMUsedColor);
	PaintRect(&RAMUsedRect);

	RGBForeColor(&kRAMFreeColor);
	PaintRect(&RAMFreeRect);

	RGBForeColor(&kRAMOverheadColor);
	PaintRect(&RAMOverheadRect);
}

void
MiscStatWindow::DrawCacheBar()
{
	Rect usedRect;
	Rect unusedRect;

	usedRect.top = kCacheBarVOffset;
	usedRect.left = kCacheBarHOffset;
	usedRect.bottom = kCacheBarVOffset + kCacheBarHeight;
	usedRect.right = kCacheBarHOffset +
						((fCacheUsage * kCacheBarWidth) / gRAMCache->GetLength());

	unusedRect.top = usedRect.top;
	unusedRect.left = usedRect.right;
	unusedRect.bottom = usedRect.bottom;
	unusedRect.right = kCacheBarHOffset + kCacheBarWidth;

	RGBForeColor(&kCacheUsedColor);
	PaintRect(&usedRect);

	RGBForeColor(&kCacheUnusedColor);
	PaintRect(&unusedRect);
}

void
MiscStatWindow::DrawCurrentURL()
{
	Rect eraseRect;
	
	eraseRect.top = kCurrentURLVOffset - kLineHeight;
	eraseRect.left = kCurrentURLHOffset;
	eraseRect.bottom = eraseRect.top + kLineSpacing;
	eraseRect.right = kCurrentURLHOffset + 1024;
	EraseRect(&eraseRect);

	const char* url = "<nil>";
	if (fCurrURL != nil) {
		url = fCurrURL;
	}	
	RGBForeColor(&kTextColor);
	MoveTo(kCurrentURLHOffset, kCurrentURLVOffset);
	DrawText(url, 0, strlen(url));
}

void
MiscStatWindow::DrawPreviousURL()
{
	Rect eraseRect;
	
	eraseRect.top = kPreviousURLVOffset - kLineHeight;
	eraseRect.left = kPreviousURLHOffset;
	eraseRect.bottom = eraseRect.top + kLineSpacing;
	eraseRect.right = kPreviousURLHOffset + 1024;
	EraseRect(&eraseRect);

	const char* url = "<nil>";
	if (fPrevURL != nil) {
		url = fPrevURL;
	}	
	RGBForeColor(&kTextColor);
	MoveTo(kPreviousURLHOffset, kPreviousURLVOffset);
	DrawText(url, 0, strlen(url));
}

void
MiscStatWindow::DrawRoundtripAvg()
{
	Rect eraseRect;
	
	eraseRect.top = kRoundtripAvgVOffset - kLineHeight;
	eraseRect.left = kRoundtripAvgHOffset;
	eraseRect.bottom = eraseRect.top + kLineSpacing;
	eraseRect.right = kRoundtripBarHOffset;
	EraseRect(&eraseRect);

	char buffer[50];
	long intPart = fRoundtripAvg / kNumRoundtrips;
	long fractPart = ((fRoundtripAvg - (intPart * kNumRoundtrips)) * 100) / kNumRoundtrips;
	
	RGBForeColor(&kTextColor);
	MoveTo(kRoundtripAvgHOffset, kRoundtripAvgVOffset);
	DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "Avg=%d.%02d ticks", intPart, fractPart));
}

void
MiscStatWindow::DrawRoundtrip(int index)
{
	const kMaxRoundtripTime = 120;

	short height = fRoundtrips[index]*kRoundtripBarHeight / kMaxRoundtripTime;
	if (height > kRoundtripBarHeight)
		height = kRoundtripBarHeight;

	RGBForeColor(&kRoundtripBackColor);
	MoveTo(kRoundtripBarHOffset + index, kRoundtripBarVOffset);
	LineTo(kRoundtripBarHOffset + index, kRoundtripBarVOffset + kRoundtripBarHeight - height);
	
	RGBForeColor(&kRoundtripForeColor);
	LineTo(kRoundtripBarHOffset + index, kRoundtripBarVOffset + kRoundtripBarHeight);
}

void
MiscStatWindow::DrawRoundtrips()
{
	for (int i=0; i<kNumRoundtrips; i++) {
		DrawRoundtrip(i);
	}
}

void
MiscStatWindow::DrawMaxRoundtrips()
{
	Rect eraseRect;
	
	eraseRect.top = kMaxRoundtripVOffset - kLineHeight;
	eraseRect.left = kMaxRoundtripHOffset;
	eraseRect.bottom = eraseRect.top + kLineSpacing;
	eraseRect.right = kMaxRoundtripHOffset + 1024;
	EraseRect(&eraseRect);

	char buffer[512];
	char* bufferstart = buffer;
	char* bufferend = bufferstart + sizeof(buffer);
	
	for (int i=0; i<kNumMaxRoundtrips; i++) {
		bufferstart += snprintf(bufferstart, bufferend - bufferstart,
								"%hu ", fMaxRoundtrips[i]);
	}
	RGBForeColor(&kTextColor);
	MoveTo(kMaxRoundtripHOffset, kMaxRoundtripVOffset);
	DrawText(buffer, 0, bufferstart - &(buffer[0]));
}

void
MiscStatWindow::Roundtrip()
{
	if (gMiscStatWindow != nil) {
		gMiscStatWindow->DoRoundtrip();
	}
}

void
MiscStatWindow::DoRoundtrip()
{
	if (fCurrRoundtripStart == 0) {
		fCurrRoundtripStart = Now();
		fRoundtripAvg = 0;
	}

	ulong tripTime = Now() - fCurrRoundtripStart;

	fRoundtripAvg -= fRoundtrips[fCurrRoundtripIndex];
	fRoundtripAvg += tripTime;
	
	fRoundtrips[fCurrRoundtripIndex] = tripTime;

	int i = 0;
	while ((i<kNumMaxRoundtrips) && (tripTime <= fMaxRoundtrips[i])) {
		i++;
	}
	
	Boolean updateMaxRoundtrips = false;
	if (i<kNumMaxRoundtrips) {
		long tempTime;
		while (i<kNumMaxRoundtrips) {
			tempTime = fMaxRoundtrips[i];
			fMaxRoundtrips[i] = tripTime;
			tripTime = tempTime;
			i++;
		}
		updateMaxRoundtrips = true;
	}

	if (GetVisible()) {
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		// draw curr roundtrip
		DrawRoundtrip(fCurrRoundtripIndex);
		DrawRoundtripAvg();
		
		// draw max roundtrip
		DrawMaxRoundtrips();
	
		SetPort(savePort);
	}
	
	fCurrRoundtripIndex = (fCurrRoundtripIndex+1) % kNumRoundtrips;
	fCurrRoundtripStart = Now();
}

void
MiscStatWindow::DrawBody(Rect* r, short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	RGBColor saveColor;
	GetForeColor(&saveColor);

		EraseRect(r);
	
		DrawMemoryTitle();
		DrawCacheTitle();
		DrawCurrentURLTitle();
		DrawPreviousURLTitle();
		DrawRoundtripTitle();
		DrawMaxRoundtripTitle();
		
		DrawMemoryBar();
		DrawCacheBar();
		DrawCurrentURL();
		DrawPreviousURL();
		DrawRoundtrips();
		DrawRoundtripAvg();
		DrawMaxRoundtrips();
	
	RGBForeColor(&saveColor);
}

void
MiscStatWindow::Idle()
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);

	ulong RAMUsed = GetUsedMemorySize();
	ulong RAMFree = GetFreeListMemorySize();
	
	if ((RAMUsed != fRAMUsed) || (RAMFree != fRAMFree)) {
		fRAMUsed = RAMUsed;
		fRAMFree = RAMFree;
		if (GetVisible()) {
			DrawMemoryBar();
		}
	}
	
	ulong cacheUsage = (gRAMCache == nil) ? 0 : gRAMCache->GetUsedCount();
	if (cacheUsage != fCacheUsage) {
		fCacheUsage = cacheUsage;
		if (GetVisible()) {
			DrawCacheBar();
		}
	}

#ifdef DEBUG	
	if (fCurrURL != gDebugParentURL) {
		fPrevURL = fCurrURL;
		fCurrURL = gDebugParentURL;
		if (GetVisible()) {
			DrawCurrentURL();
			DrawPreviousURL();
		}
	}
#endif

	SetPort(savePort);
}

void
MiscStatWindow::Close()
{
	HideWindow();
}

void
MiscStatWindow::Click(struct MacPoint* UNUSED(where), ushort modifiers)
{
	if (modifiers & optionKey) {
		fMaxRoundtrips[0] = 0;
		fRoundtripAvg = 0;
		for (int i=0; i<kNumRoundtrips; i++) {
			fRoundtripAvg += fRoundtrips[i];
		}
	}
}

void
MiscStatWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);
	MenuHandle menu = GetMenu(mDebug);
	EnableItem(menu, iMiscStatWindow);

	SetMenuItemText(menu, iMiscStatWindow,
					GetVisible() ? "\pHide MiscStatWindow" : "\pShow MiscStatWindow");
}

Boolean
MiscStatWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);
	Boolean handled = false;

	if ((theMenu == mDebug) && (theItem == iMiscStatWindow))
	{
		if (GetVisible())
			HideWindow();
		else
			ShowWindow();
		handled = true;
	}
	return handled || StdWindow::DoMenuChoice(menuChoice, modifiers);
}

#endif /* DEBUG_MISCSTATWINDOW */





