// ===========================================================================
//	CacheWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef DEBUG_CACHEWINDOW

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CACHEWINDOW_H__
#include "CacheWindow.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

// ===========================================================================
//  global/local variables/constants
//

CacheWindow* gCacheWindow = nil;

static const kCacheWindowHeaderLineHeight = 12;
static const kCacheWindowBodyLineHeight = 12;

static const RGBColor kEntryColorComplete			= {0x0000, 0x0000, 0x0000};
static const RGBColor kEntryColorPendingPersistent	= {0x0000, 0xffff, 0x0000};
static const RGBColor kEntryColorPendingImmediate 	= {0x0000, 0xffff, 0x0000};
static const RGBColor kEntryColorPendingSelectable	= {0x0000, 0xffff, 0x0000};
static const RGBColor kEntryColorPendingVisible		= {0x0000, 0x7f7f, 0x0000};
static const RGBColor kEntryColorPendingDefault		= {0x0000, 0x7f7f, 0x7f7f};
static const RGBColor kEntryColorNoError			= {0x0000, 0x0000, 0xffff};
static const RGBColor kEntryColorDefault			= {0xffff, 0x0000, 0x0000};
static const RGBColor kEntryColorRecentlyModified	= {0x7f7f, 0x0000, 0x7f7f};

// ===========================================================================
//  local helper functions
//
/*
void 
CacheWindow::ReportByType(DataType dataType, long& count, long& length)
{
	int index = gRAMCache->GetEntryCount();
	while (index-- > 0) {
		CacheEntry* cacheEntry = gRAMCache->GetCacheEntry(index);
	
		if ((cacheEntry != nil) && (cacheEntry->GetDataType() == dataType)) {
			count++;
			length += cacheEntry->GetLength();
		}
	}
}
*/

// ===========================================================================


CacheWindow::CacheWindow()
{
	if (gCacheWindow != nil)
	{	delete gCacheWindow;
	}

	SetFormat(6*kCacheWindowHeaderLineHeight, 0, true, true); // Header, trailer, vscroll, hscroll
	mVLineScroll = kCacheWindowHeaderLineHeight;
	fLastUpdate = Now();
	ResizeWindow(300, 500);
	SetTitle((const char*)"Cache");

	gCacheWindow = this;
}

CacheWindow::~CacheWindow()
{
	gCacheWindow = nil;
}

void
CacheWindow::Close()
{
	HideWindow();
}

void
CacheWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);
	MenuHandle menu = GetMenu(mCache);
	EnableItem(menu, iCacheWindow);

	SetMenuItemText(menu, iCacheWindow,
					GetVisible() ? "\pHide CacheWindow" : "\pShow CacheWindow");
}

Boolean
CacheWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);
	Boolean handled = false;

	if ((theMenu == mCache) && (theItem == iCacheWindow))
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
CacheWindow::DrawBody(Rect *r, short hScroll, short vScroll, Boolean UNUSED(scrolling))
{
	const kWindowLeftMargin = 4;
	const kWindowTopMargin = 11;
	if (gRAMCache == nil)
		return;
		
	EraseRect(r);
	
	struct MacPoint pt;
	pt.h = kWindowLeftMargin + r->left - hScroll;	// scoot it in from margin
	pt.v = kWindowTopMargin + r->top - vScroll;	// scoot it down from top
	TextFont(monaco);
	TextSize(9);
	TextFace(normal);

	RGBColor saveColor;
	GetForeColor(&saveColor);
	
	int index = gRAMCache->GetEntryCount();
	while (index-- > 0) {
		CacheEntry* cacheEntry = gRAMCache->GetCacheEntry(index);
		
		if ((cacheEntry != nil) && !cacheEntry->IsFree()) {
		
			MoveTo(pt.h, pt.v);
			Error currentStatus = cacheEntry->GetStatus();
			const RGBColor* color = &kEntryColorDefault;
			switch (currentStatus) {
				case kComplete:				color = &kEntryColorComplete; break;
				case kPending:		
					switch (cacheEntry->GetPriority()) {
						case kPersistent:	color = &kEntryColorPendingPersistent; break;
						case kImmediate:	color = &kEntryColorPendingImmediate; break;
						case kSelectable:	color = &kEntryColorPendingSelectable; break;
						case kVisible:		color = &kEntryColorPendingVisible; break;
						default:			color = &kEntryColorPendingDefault; break;
					}
					break;
				case kNoError:				color = &kEntryColorNoError; break;
				default:					color = &kEntryColorDefault; break;
			}
			if (cacheEntry->GetDebugModifiedTime() > fLastUpdate) {
				color = &kEntryColorRecentlyModified;
			}
			RGBForeColor(color);
			
			DataType type = cacheEntry->GetDataType();
			char typeChars[4];
			typeChars[0] = type >> 24;
			if (isalnum(typeChars[0])) {
				typeChars[1] = type >> 16;
				typeChars[2] = type >> 8;
				typeChars[3] = type;
			} else {
				typeChars[0] = typeChars[1] = typeChars[2] = typeChars[3] = '?';
			}
	
			char buf[768];
			const char* errorString;
#ifdef DEBUG_NAMES
			errorString = GetErrorString(currentStatus);
#else
			char debugNameBuf[12];
			snprintf(debugNameBuf, sizeof(debugNameBuf), "%ld", (long)currentStatus);
			errorString = &(debugNameBuf[0]);
#endif
			
			long bufLength = snprintf(buf, sizeof(buf),
										"%-12.12s(%2d) %2d(%2d) %6d(%6d)  %c%c%c%c  ",
										errorString,
										cacheEntry->GetPriority(), 
										(short)cacheEntry->GetUserCount(),
										(short)cacheEntry->GetDataUserCount(),
										cacheEntry->GetLength(),
										cacheEntry->GetDataLength(),
										typeChars[0], typeChars[1], typeChars[2], typeChars[3]);
			bufLength += snprintf(&(buf[bufLength]), sizeof(buf) - bufLength,
								  "%.*s", sizeof(buf) - bufLength - 1, cacheEntry->GetName());
	
			DrawText(buf, 0, bufLength);
		
			pt.v += kCacheWindowBodyLineHeight;
		}
	}

	RGBForeColor(&saveColor);

	short bodyHeight = pt.v + vScroll - r->top;
	SetBodySize(700, bodyHeight);

	fLastUpdate = Now();
}

#if defined __MWERKS__ && defined DEBUG && defined FIDO_INTERCEPT
#pragma global_optimizer on
#endif
void CacheWindow::DrawHeader(Rect *r)
{
	if (gRAMCache == nil)
		return;

#ifdef DEBUG_CACHE_VALIDATE
	if (!gRAMCache->IsValid())
		return;
#endif /* DEBUG_CACHE_VALIDATE */
		
	RGBColor saveColor;
	GetForeColor(&saveColor);
	RGBColor black = {0, 0, 0};
	RGBForeColor(&black);

	EraseRect(r);
	
	TextFont(geneva);
	TextSize(9);
	TextFace(bold);

	MacPoint pt = {r->left+16, r->top+8};

	char buf[128];
	
	const kColumnOffset = 110;
	
	int index = gRAMCache->GetEntryCount();
	
	long numAnimation = 0;
	long numBitmap = 0;
	long numBorder = 0;
	long numGIF = 0;
	long numHTML = 0;
	long numJPEG = 0;
	long numXBitMap = 0;
	long numOther = 0;
	
	long lenAnimation = 0;
	long lenBitmap = 0;
	long lenBorder = 0;
	long lenGIF = 0;
	long lenHTML = 0;
	long lenJPEG = 0;
	long lenXBitMap = 0;
	long lenOther = 0;
	
	while (index-- > 0) {
		CacheEntry* cacheEntry = gRAMCache->GetCacheEntry(index);
	
		if (cacheEntry == nil)
			continue;
		
		DataType dateType = cacheEntry->GetDataType();
		long dataLength = cacheEntry->GetLength();
				
		switch (cacheEntry->GetDataType()) {
			case kDataTypeAnimation:	numAnimation++;	lenAnimation += dataLength;	break;
			case kDataTypeBitmap:		numBitmap++;	lenBitmap += dataLength;	break;
			case kDataTypeBorder:		numBorder++;	lenBorder += dataLength;	break;
			case kDataTypeGIF:			numGIF++;		lenGIF += dataLength;		break;
			case kDataTypeHTML:			numHTML++;		lenHTML += dataLength;		break;
			case kDataTypeJPEG:			numJPEG++;		lenJPEG += dataLength;		break;
			case kDataTypeXBitMap:		numXBitMap++;	lenXBitMap += dataLength;	break;
			default:					numOther++;		lenOther += dataLength;		break;
		}
	}

	MoveTo(pt.h, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "ANIM = %ld/%ld", numAnimation, lenAnimation));
	
	MoveTo(pt.h + kColumnOffset, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "BMIR = %ld/%ld", numBitmap, lenBitmap));

	MoveTo(pt.h + 2*kColumnOffset, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "BORD = %ld/%ld", numBorder, lenBorder));

	MoveTo(pt.h + 3*kColumnOffset, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "GIF = %ld/%ld", numGIF, lenGIF));
	pt.v += kCacheWindowHeaderLineHeight;
	
	MoveTo(pt.h, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "HTML = %ld/%ld", numHTML, lenHTML));
	
	MoveTo(pt.h + kColumnOffset, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "JPEG = %ld/%ld", numJPEG, lenJPEG));

	MoveTo(pt.h + 2*kColumnOffset, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "XBMP = %ld/%ld", numXBitMap, lenXBitMap));

	MoveTo(pt.h + 3*kColumnOffset, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "Other = %ld/%ld", numOther, lenOther));

	pt.v += (3*kCacheWindowHeaderLineHeight)/2;
	
	MoveTo(pt.h, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf),
					"Cache: %2d%% Full    Free = %d    Used = %d   Resets = %d",
					(int) (100 * gRAMCache->GetUsedCount() / gRAMCache->GetLength()),
					(int)gRAMCache->GetFreeCount(),
					(int)gRAMCache->GetUsedCount(),
					(int)gRAMCache->GetResetCount()));
	pt.v += (3*kCacheWindowHeaderLineHeight)/2;
	
	MoveTo(pt.h, pt.v);
	DrawText(buf, 0, snprintf(buf, sizeof(buf),
					"Status (Priority)    Users        Length        Type   Name"));
	pt.v += kCacheWindowHeaderLineHeight;

	RGBForeColor(&saveColor);

	fLastUpdate = Now();
}
#if defined __MWERKS__ && defined DEBUG && defined FIDO_INTERCEPT
#pragma global_optimizer off
#endif

void CacheWindow::Idle()
{
	static Cache* lastRAMCache = (Cache*)0;

	if (!GetVisible())
		return;
	
	Boolean needsUpdate = false;

	if (gRAMCache != lastRAMCache) {
		needsUpdate = true;
		lastRAMCache = gRAMCache;
	}
	
	if (gRAMCache != nil) {
	
		if (!needsUpdate) {
			needsUpdate = fLastUpdate < gRAMCache->GetDebugModifiedTime();
		}
		
		if (!needsUpdate) {
			long index = gRAMCache->GetEntryCount();
			while (index-- > 0) {
				CacheEntry* cacheEntry = gRAMCache->GetCacheEntry(index);
				if (cacheEntry != nil) {
					needsUpdate = fLastUpdate < cacheEntry->GetDebugModifiedTime();
					if (needsUpdate) {
						break;
					}
				}
			}
		}
	}
	
	if (needsUpdate) {
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		InvalRect(&(w->portRect));
		SetPort(savePort);
	}
}

#endif /* #ifdef DEBUG_CACHEWINDOW */