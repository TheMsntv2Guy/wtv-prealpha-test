// ===========================================================================
//	MemoryWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#if defined(DEBUG_MEMORYWINDOW)

#ifndef MEMORY_TRACKING
#error "Can't turn DEBUG_MEMORYWINDOW on if you don't have MEMORY_TRACKING on, also"
#endif

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYMANAGER_PRIVATE_H__
#include "MemoryManager.Private.h"
#endif
#ifndef __MEMORYWINDOW_H__
#include "MemoryWindow.h"
#endif
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif




// ===========================================================================
//	consts, #defines, structs
// ===========================================================================

static const char kPrefsMemoryWindowCount[] = "MemoryWindow Count";

static const kSummaryHeight = 46;

// ---------------------------------------------------------------------------

typedef struct
{
	CWindowRecord	window;
	ulong			windowNumber;
	ulong			baseOffset;
	ulong			size;
	ulong			lastMemoryTime;
} MemoryWindow;

typedef struct
{
	short 			top;
	short			left;
	ulong			baseOffset;
	ulong			length;
}
MemoryWindowPrefsRecord;

static long gNextWindowNumber = 0;

static ulong ThisBlock(ulong address);
static ulong NextBlock(ulong address);
static ulong PreviousBlock(ulong address);




// ===========================================================================
//	implementations
// ===========================================================================

static void
SetMemoryWindowTitle(MemoryWindow *window)
{
	char title[256];

	snprintf(title, sizeof(title), "0x%X - 0x%X",
		window->baseOffset,
		window->baseOffset + window->size);
	
	(void)c2pstr(title);
	SetWTitle((WindowPtr)&window->window, (StringPtr)title);
}

void
MemoryWindowChanged(WindowPtr window)
{
	MacPoint	pt;
	ulong		windowNumber = ((MemoryWindow *)window)->windowNumber;

	GrafPtr savePort;
	GetPort(&savePort);

	SetPort(window);
	SetMemoryWindowTitle((MemoryWindow *)window);	
	InvalRect(&((GrafPtr)window)->portRect);

	MemoryWindowPrefsRecord windowPrefs;
	pt.v = window->portRect.top;
	pt.h = window->portRect.left;
	LocalToGlobal(&pt);
	windowPrefs.top = pt.v;
	windowPrefs.left = pt.h;
	windowPrefs.baseOffset = ((MemoryWindow *)window)->baseOffset;
	windowPrefs.length = ((MemoryWindow *)window)->size;
	
	char windowName[256];
	snprintf(windowName, sizeof(windowName), "MemoryWindow%d", windowNumber);
	gSimulator->SetPreference(windowName, &windowPrefs, sizeof(windowPrefs));
		
	SetPort(savePort);
}

void CloseMemoryWindow(WindowPtr window)
{
	CloseWindow(window);
	DisposePtr((Ptr)window);
	
	char windowName[256];
	snprintf(windowName, sizeof(windowName), "MemoryWindow%d",
									((MemoryWindow *)window)->windowNumber);
	gSimulator->RemovePreference(windowName);
}

static ulong RoundDownTo2(ulong n)
{
	ulong			mask = 0x80000000;
	
	while ((n & mask) == 0)
		mask >>= 1;
		
	return mask;
}

void
MouseDownInMemoryWindow(WindowPtr window, MacPoint pt, Boolean optionKeyDown)
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(window);
	
	GlobalToLocal(&pt);
	if (optionKeyDown)
	{
		((MemoryWindow *)window)->baseOffset = 0;
		((MemoryWindow *)window)->size = gSimulator->GetRAMSize();
	}
	else
	{
		MacPoint dragPt = pt;
		short startPixel, endPixel, winHeight;
	
		while (StillDown())
			GetMouse(&dragPt);
			
		startPixel = (dragPt.v > pt.v) ? pt.v : dragPt.v;
		endPixel   = (dragPt.v < pt.v) ? pt.v : dragPt.v;
		winHeight  = window->portRect.bottom - window->portRect.top - kSummaryHeight;
		if (startPixel < 0) startPixel = 0;
		
		if (endPixel - startPixel > 3)
		{
			// Handle drag case
			((MemoryWindow *)window)->baseOffset += ((MemoryWindow *)window)->size * startPixel / winHeight;
			((MemoryWindow *)window)->size = ((MemoryWindow *)window)->size * (endPixel-startPixel) / winHeight;
		}
		else
		{
			// Handle click case
			if (pt.v >= (window->portRect.bottom + window->portRect.top + 1) / 2)
				((MemoryWindow *)window)->baseOffset =
					(((MemoryWindow *)window)->baseOffset * 2 + ((MemoryWindow *)window)->size) / 2;
			((MemoryWindow *)window)->size /= 2;
		}
	}
	
	
	MemoryWindowChanged(window);
	SetPort(savePort);
}

	
void
NewMemoryWindow(long top, long left, ulong baseOffset, ulong length, ulong windowNumber)
// can pass -1 for windowNumber to get new one
// can pass {0,0} for coords to get auto-assigned
{
	MemoryWindow	*window;
	ulong			maxLength = gSimulator->GetRAMSize();
	static ulong	gOffset = 0;
	long			count = 0;
	
	window = (MemoryWindow *)NewPtrClear(sizeof(MemoryWindow));
	if (window == nil)
		EmergencyExit(sLowMemory);
	
	/* constrain it to existing memory bounds */
	if (baseOffset >= maxLength)
		baseOffset = 0;
	if (baseOffset + length > maxLength)
		length = maxLength - baseOffset;
	
	/* default, for now */
	window->baseOffset = baseOffset;
	window->size = length;
	
	if (GetNewCWindow(rMemoryWindow, (void *)window, (WindowPtr)-1) == nil)
		EmergencyExit(sLowMemory);
	SetWRefCon((WindowPtr)&window->window, rMemoryWindow);
	if (top == 0 && left == 0)
	{	
		top = 40 + gOffset;
		left = 20 + gOffset;
		gOffset += 4;
	}
	MoveWindow((WindowPtr)&window->window, left, top, false);
	
	SetMemoryWindowTitle(window);
	ShowWindow((WindowPtr)&window->window);

	if (windowNumber == (ulong)-1)
		windowNumber = gNextWindowNumber++;
	window->windowNumber = windowNumber;

	gSimulator->GetPreferenceLong(kPrefsMemoryWindowCount, &count);
	if (gNextWindowNumber > count)
		gSimulator->SetPreferenceLong(kPrefsMemoryWindowCount, gNextWindowNumber);

	MemoryWindowChanged((WindowPtr)&window->window);
}
	
static void
InitializeOneMemoryWindow(ulong windowNumber)
{
	char windowName[256];
	snprintf(windowName, sizeof(windowName), "MemoryWindow%d", windowNumber);

	void*  buffer;
	size_t bufferSize;

	if (!gSimulator->GetPreference(windowName, &buffer, &bufferSize))
		return;
	Assert(bufferSize == sizeof(MemoryWindowPrefsRecord));
	MemoryWindowPrefsRecord windowPrefs = *(MemoryWindowPrefsRecord*)buffer;

	if (windowPrefs.length == 0)
		windowPrefs.length = gSimulator->GetRAMSize();
		
	NewMemoryWindow(windowPrefs.top,
					windowPrefs.left,
					windowPrefs.baseOffset,
					windowPrefs.length,
					windowNumber);
}
	
void
InitializeMemoryWindows(void)
{
	long		i, count = 0;
	
	gSimulator->GetPreferenceLong(kPrefsMemoryWindowCount, &count);
	gNextWindowNumber = count;
	
	for (i = 0; i < count; i++)
		InitializeOneMemoryWindow(i);
}

static RGBColor	gScreenBufferColor = {65535,48000,65535};	// lt green
static RGBColor	gVectorsColor = {48000,65535,65535};	// lt pink
static RGBColor	gGlobalsColor = {65535,65535,48000};	// lt blue
static RGBColor	gUsedColor = {32767,32767,32767};	// gray
static RGBColor	gFreeColor = {65535,65535,65535};	// white
static RGBColor	gBlackColor = {0,0,0};				// black
static RGBColor	gDarkGrayColor = {20000,20000,20000};
static RGBColor	gWhiteColor = {65535,65535,65535};

#define kTagColors	17
static RGBColor	gUsedColors[kTagColors] =
{
	{0,65535,65535},		// blue-green
	{65535,0,65535},		// purple
	{65535,65535,0},		// brown
	{0,0,32767},			// dark brown
	{0,65535,0},			// purple
	{0,32767,0},
	{0,32767,32767},
	{32767,32767,0},
	{32767,0,0},
	{0,32767,65535},
	{0,32767,16383},
	{65535,32767,0},
	{0,65535,32767},
	{16383,16383,16383},
	{65535,16383,32767},
	{16383,65535,16383},
	{65535,16383,16383}
};


static void DrawOneBlock(WindowPtr window, const char *blockName,
	ulong base, ulong length, Boolean shouldPrintStats, ulong showBase, ulong showSize, const RGBColor *color,
	char* urlName, char* sourceFile, ulong lineNumber)
{
	long			value;
	ulong			windowHeight = window->portRect.bottom - window->portRect.top - kSummaryHeight;
	Rect			bounds = window->portRect;

	/* do bounds check before any drawing */
	if (base + length < showBase || (ulong)base >= showBase + showSize)
		return;
		
	// calculate and pin
	value = ((long)base - (long)showBase);
	if (windowHeight > showSize)
		value *= windowHeight/showSize;
	else
		value /= showSize/windowHeight;
	bounds.top = (short)value;
	if (bounds.top < window->portRect.top)
		bounds.top = window->portRect.top;
	value = base + length - (long)showBase;
	if (windowHeight > showSize)
		value *= windowHeight/showSize;
	else
		value /= showSize/windowHeight;
	bounds.bottom = (short)value;
	if (bounds.bottom > window->portRect.top + windowHeight)
		bounds.bottom = window->portRect.top + windowHeight;
	
	RGBForeColor(color);
	PaintRect(&bounds);
		
	/* draw the division lines */
	RGBForeColor(&gBlackColor);
	MoveTo(bounds.left, bounds.top); Line(20, 0);
	MoveTo(bounds.right, bounds.top); Line(-20, 0);
	MoveTo(bounds.left, bounds.bottom); Line(20, 0);
	MoveTo(bounds.right, bounds.bottom); Line(-20, 0);
	
	/* draw the name, if enough room */
	if (shouldPrintStats && bounds.bottom > bounds.top + 11)
	{
		long		textWidth = TextWidth(blockName, 0, strlen(blockName));
		
		MoveTo(bounds.left + (bounds.right - bounds.left - textWidth)/2, bounds.top + 11);
		DrawText(blockName, 0, strlen(blockName));
		
		if (bounds.bottom > bounds.top + 22)
		{
			char buffer[256];
			int bufLength = snprintf(buffer, sizeof(buffer), "%d bytes @ 0x%X", length, base);

			textWidth = TextWidth(buffer, 0, bufLength);
			MoveTo(bounds.left + (bounds.right - bounds.left - textWidth)/2, bounds.top + 22);
			DrawText(buffer, 0, bufLength);
			
			if (sourceFile != nil && bounds.bottom > bounds.top + 33)
			{
				bufLength = snprintf(buffer, sizeof(buffer), "File %s; line %d", sourceFile, lineNumber);
				textWidth = TextWidth(buffer, 0, bufLength);
				MoveTo(bounds.left + (bounds.right - bounds.left - textWidth)/2, bounds.top + 33);
				DrawText(buffer, 0, bufLength);
				
				if (urlName != nil && bounds.bottom > bounds.top + 44)
				{
					int urlLength = strlen(urlName);
					textWidth = TextWidth(urlName, 0, urlLength);
					MoveTo(bounds.left + (bounds.right - bounds.left - textWidth)/2, bounds.top + 44);
					DrawText(urlName, 0, urlLength);
				}
			}
		}
	}
}

// Support routines for walking through the memory display:
//
// 		Addresses are relative to RAMBaseAddress
// 		ulongs are used to make it easier on the caller

static ulong
ThisBlock(ulong address)
{
	void*			block = nil;
	Boolean			used;
	ulong			length;
	void*			p = (void*)(address + gSimulator->GetRAMBaseAddress());
	
	while ((block = NextMemoryBlock(block, &used, &length, nil, nil, nil, nil, 0)) != nil)
		if (block <= p && ((long)block + length) > (long)p)
			break;
	return (ulong)block - (ulong)gSimulator->GetRAMBaseAddress();
}

static ulong
NextBlock(ulong address)
{
	void*			block = nil;
	Boolean			used;
	ulong			length;
	void*			p = (void*)(address + gSimulator->GetRAMBaseAddress());
	
	while ((block = NextMemoryBlock(block, &used, &length, nil, nil, nil, nil, 0)) != nil)
		if (block <= p && ((long)block + length) > (long)p)
			break;
	return (ulong)NextMemoryBlock(block, &used, &length, nil, nil, nil, nil, 0) - (ulong)gSimulator->GetRAMBaseAddress();
}

static ulong
PreviousBlock(ulong address)
{
	void*			block;
	void*			prev = nil;
	Boolean			used;
	ulong			length;
	void*			p = (void*)(address + gSimulator->GetRAMBaseAddress());
	
	while ((block = NextMemoryBlock(prev, &used, &length, nil, nil, nil, nil, 0)) != nil)
	{
		if (block <= p && ((long)block + length) > (long)p)
			break;
		prev = block;
	}
	return (ulong)prev - (ulong)gSimulator->GetRAMBaseAddress();
}


static ulong MWNameHash(char* name)
{
	ulong			hashValue = 0;
	
	while (*name != 0)
		hashValue += (uchar)(*name++);
		
	return hashValue;
}

struct DrawOneEntryParameters
{
	WindowPtr		window;
	ulong			showBase;
	ulong			showSize;
};
typedef struct DrawOneEntryParameters DrawOneEntryParameters;

static Boolean DrawOneEntry(const char* name, const char* data, ulong dataLength, DrawOneEntryParameters *parameters)
{
	RGBColor		color = { 0x8000, 0x8000, 0x8000 };
	
	DrawOneBlock(parameters->window, name, (ulong)data, dataLength, true, parameters->showBase, parameters->showSize, &color, nil, nil, 0);	
	return false;
}


void DrawMemoryWindow(WindowPtr window)
/* just randomly drawing, for now */
{
	void*			block = nil;
	Boolean			used;
	ulong			length;
	ulong			height = window->portRect.bottom - window->portRect.top - kSummaryHeight;
	Rect			bounds;
	ulong			showBase = (ulong)gSimulator->GetRAMBaseAddress() + ((MemoryWindow *)window)->baseOffset;
	ulong			showSize = ((MemoryWindow *)window)->size;
	ulong			usedCount = 0, freeCount = 0, cplusCount = 0;
	ulong			usedByteCount = 0, freeByteCount = 0, cplusByteCount = 0;
	char			buffer[256];
	char			sourceFile[256];
	char			urlName[256];
	ulong			lineNumber;
	Boolean			isCPlus;
	DrawOneEntryParameters	parameters;
	GrafPtr			savePort;
	
	GetPort(&savePort);
	parameters.window = window;
	parameters.showBase = showBase;
	parameters.showSize = showSize;
	
	SetPort(window);
	bounds = window->portRect;
	ClipRect(&bounds);
	
	if (!gSimulator->GetIsInitialized())
	{
		RGBForeColor(&gBlackColor);
		PaintRect(&bounds);
		return;
	}
	
	TextFont(geneva);
	TextSize(9);
	ulong showEnd = showBase + showSize;

	// show all memory manager blocks (inclusing the cache)
	while ((block = NextMemoryBlock(block, &used, &length, buffer, urlName, &isCPlus, sourceFile, &lineNumber)) != nil)
	{
		if (used)
		{
			usedCount++; usedByteCount += length;
			if (isCPlus)
			{	
				cplusCount++; 
				cplusByteCount += length;
				if (strcmp(buffer, "Cache") == 0)
				{
					Cache*	cache = (Cache*)((UsedBlock*)block)->data;
					if ((ulong)block + length > showBase && (ulong)block < showEnd) {
#ifdef DEBUG_MEMORYWINDOW
						(void)cache->EachEntry((EachEntryFunction*)DrawOneEntry, &parameters);
#endif
					}
					continue;
				}
			 }
		}
		else
		{	freeCount++; freeByteCount += length; }

		if ((ulong)block + length > showBase && (ulong)block < showEnd)
		{
			ulong		colorNumber = MWNameHash(buffer) % kTagColors;
			RGBColor	*color = used ? &gUsedColors[colorNumber] : &gFreeColor;
			DrawOneBlock(window, buffer, (ulong)block, length,
				used && strcmp(buffer, "Cache::fData") != 0, showBase, showSize, color, urlName, sourceFile, lineNumber);
		}
	}

	RGBForeColor(&gBlackColor);
	
	// draw summary text
	bounds = window->portRect;
	bounds.top = window->portRect.bottom - kSummaryHeight;

	RGBForeColor(&gWhiteColor);
	if (window == FrontWindow())
		RGBBackColor(&gBlackColor);
	else
		RGBBackColor(&gDarkGrayColor);
		
	EraseRect(&bounds);
	
	bounds.top += 4;
	snprintf(buffer, sizeof(buffer),
					"   Used blocks:\t%d (%d bytes)\015"
					"   Free blocks:\t%d (%d bytes)\015"
					"   C++ objects:\t%d (%d bytes)\015",
		usedCount, usedByteCount, freeCount, freeByteCount, cplusCount, cplusByteCount);
	TextBox(buffer, strlen(buffer), &bounds, teJustLeft);

	RGBForeColor(&gBlackColor);
	RGBBackColor(&gWhiteColor);
	SetPort(savePort);
}

void IdleMemoryWindows()
{
	WindowPeek win = (WindowPeek)FrontWindow();
	GrafPtr savePort;
	GetPort(&savePort);
	
	while (win != nil)
	{
		if ((win->refCon == rMemoryWindow) && ((MemoryWindow*)win)->lastMemoryTime != MemoryTime())
		{
			SetPort((GrafPtr)win);
			((MemoryWindow*)win)->lastMemoryTime = MemoryTime();
			InvalRect(&((GrafPtr)win)->portRect);
		}
		win = win->nextWindow;
	}
	SetPort(savePort);
}


void MemoryWindowHandleKey(WindowPtr window, char key)
{
	ulong	lastBase, currentBase;
	
	switch (key)
	{
		case 0x1F:	/* down arrow on keyboard */
			lastBase = ThisBlock(((MemoryWindow *)window)->baseOffset);
			((MemoryWindow *)window)->baseOffset+=((MemoryWindow *)window)->size/4;
			currentBase = ThisBlock(((MemoryWindow *)window)->baseOffset);
			
			if (lastBase == currentBase)
				((MemoryWindow *)window)->baseOffset = NextBlock(((MemoryWindow *)window)->baseOffset);
				
			if (((MemoryWindow *)window)->baseOffset + ((MemoryWindow *)window)->size > gSimulator->GetRAMSize())
				((MemoryWindow *)window)->baseOffset = gSimulator->GetRAMSize() - ((MemoryWindow *)window)->size;
			break;
			
		case 0x1D:	/* right arrow on keyboard */
			((MemoryWindow *)window)->size*=2;
			((MemoryWindow *)window)->size/=3;
			if (((MemoryWindow *)window)->size < 128)
				((MemoryWindow *)window)->size = 128;

			break;
			
		case 0x1E:	/* up arrow on keyboard */
			lastBase = ThisBlock(((MemoryWindow *)window)->baseOffset);
			((MemoryWindow *)window)->baseOffset-=((MemoryWindow *)window)->size/4;
			currentBase = ThisBlock(((MemoryWindow *)window)->baseOffset);

			if (lastBase == currentBase)
				((MemoryWindow *)window)->baseOffset = PreviousBlock(((MemoryWindow *)window)->baseOffset);
				
			if ((long)((MemoryWindow *)window)->baseOffset < 0)
				((MemoryWindow *)window)->baseOffset = 0;
			break;
			
		case 0x1C:	/* left arrow on keyboard */
			((MemoryWindow *)window)->size*=4;
			((MemoryWindow *)window)->size/=2;
			if (((MemoryWindow *)window)->size > gSimulator->GetRAMSize())
				((MemoryWindow *)window)->size = gSimulator->GetRAMSize();
			if (((MemoryWindow *)window)->baseOffset + ((MemoryWindow *)window)->size > gSimulator->GetRAMSize())
				((MemoryWindow *)window)->baseOffset = gSimulator->GetRAMSize() - ((MemoryWindow *)window)->size;
			break;
			   			
	}
	MemoryWindowChanged(window);
}

#endif /* DEBUG_MEMORYWINDOW */