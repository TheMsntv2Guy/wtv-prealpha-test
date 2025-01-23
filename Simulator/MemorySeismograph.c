// -----------------------------------------------------------------------------------
//
//	MemorySeismograph.c
//
// -----------------------------------------------------------------------------------

#include "Headers.h"

#ifdef DEBUG_MEMORYSEISMOGRAPH

	#ifndef MEMORY_TRACKING
	#error "DEBUG_MEMORYSEISMOGRAPH requires you to #define MEMORY_TRACKING"
	#endif /* MEMORY_TRACKING */

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYMANAGER_PRIVATE_H__
#include "MemoryManager.Private.h"
#endif
#ifndef __MEMORYSEISMOGRAPH_H__
#include "MemorySeismograph.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif
#ifndef _STRING
#include <string.h>
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif


#define INTERAL_DEBUG_SEISMOGRAPH	// extra safety checks...

// -----------------------------------------------------------------------------------

MemorySeismographWindow* gMemorySeismographWindow = nil;

const long kSafetyDivide = 8; // don't overflow when converting memory addr => screen coords
const kSeismographTimeInterval = 6;	// 1/10 of a second between readings

static RGBColor kSeisBackgroundColor	= {0xcfcf, 0xcfcf, 0xcfcf};
static RGBColor kSeisChangeURLColor		= {0x7f7f, 0x7f7f, 0x7f7f};
static RGBColor kDelSizeColor 			= {0x3f3f, 0x0000, 0x0000};
static RGBColor kCurrSizeColor			= {0x9f9f, 0x3f3f, 0x3f3f};
static RGBColor kCacheUsedColor			= {0x3f3f, 0x7f7f, 0x3f3f};
static RGBColor kCacheFreeColor			= {0x1f1f, 0x3f3f, 0x1f1f};
static RGBColor kScreenBufferColor		= {0x0707, 0x0f0f, 0x0707};

static RGBColor kOldURLColorArray[] =
	{
		{0xffff, 0x0000, 0x0000},
		{0xaaaa, 0x5555, 0x0000},
		{0x5555, 0xaaaa, 0x0000},
		{0x0000, 0xffff, 0x0000},
		{0x0000, 0xaaaa, 0x5555},
		{0x0000, 0x5555, 0xaaaa},
		{0x0000, 0x0000, 0xffff},
		{0x5555, 0x0000, 0xaaaa},
		{0xaaaa, 0x0000, 0x5555},
	};

typedef enum
{
	kString,
	kDecLong,
	kDecLongPercentRAM,
	kHexLongPercentRAM,
	kCacheSummary
	
} SeismographCoerceTo;

struct SeismographHeaderDispItem
{
	char*					title;
	ulong					value;
	SeismographCoerceTo		coerceTo;
	RGBColor*				color;
};

// -----------------------------------------------------------------------------------

SeismographReading::SeismographReading(void)
{
	Init();
}

SeismographReading::~SeismographReading(void)
{
}

void
SeismographReading::Init(void)
{
	startTime = 0;

	currURL = UniqueName(kBootingURLString);
	currSize = 0;
	newSize = 0;
	delSize = 0;

	cacheUsed = 0;
	cacheEntries = 0;

	for(int i=0; i < kNumOldURLs; i++)
	{	oldURL[i] = nil;
		oldURLAlloc[i] = 0;
		oldURLColorIndex[i] = i;
	}
}

void
SeismographReading::Reset()
{
	MemoryTag* tag = &(gTagSlots[0]);
	long i = kTagSlots;

	while (i--)
	{	if (tag->fInUse)
		{	TaggedAlloc(tag);
		}
		tag++;
	}
}

void
SeismographReading::TaggedAlloc(MemoryTag* tag)
{
	currSize += tag->fLength;
	newSize += tag->fLength;
	
	int i;
	// try to pin this alloc on an old URL
	for(i=0; i<kNumOldURLs; i++)
	{
		if (tag->fOwnerURL == oldURL[i])
		{	oldURLAlloc[i] += tag->fLength;
			return;
		}
	}
	
	// look for someone who's out of allocs
	for (i=0; i<kNumOldURLs-1; i++)	// -1 because want to default to bumping last guy, anyway
	{
		if (oldURLAlloc[i] == 0)
			break;
	}
	
	// then slide everyone down, take that URL's color
	int saveURLColorIndex = oldURLColorIndex[i];
	
	while (i-- > 0)
	{	oldURL[i+1] = oldURL[i];
		oldURLAlloc[i+1] = oldURLAlloc[i];
		oldURLColorIndex[i+1] = oldURLColorIndex[i];
	}
	
	oldURL[0] = tag->fOwnerURL;
	oldURLAlloc[0] = tag->fLength;
	oldURLColorIndex[0] = saveURLColorIndex;
}

void
SeismographReading::TaggedFree(MemoryTag* tag)
{
	delSize += tag->fLength;
	
	int i;
	
	for(i=0; i<kNumOldURLs; i++)
	{	
		if (tag->fOwnerURL == oldURL[i])
		{
			oldURLAlloc[i] -= tag->fLength;
			if (!(oldURLAlloc[i] >= 0))
			{
				Reset();
			}
			break;
		}
	}
}

void
SeismographReading::TaggedNew(MemoryTag* UNUSED(tag))
{
}

void
SeismographReading::TaggedDelete(MemoryTag* UNUSED(tag))
{
}

void
SeismographReading::AssumeActiveFrom(SeismographReading* oldReading)
{
	// finish up the old
	oldReading->cacheUsed		= (gRAMCache == nil) ? 0 : gRAMCache->GetUsedCount();
	oldReading->cacheEntries	= (gRAMCache == nil) ? 0 : gRAMCache->GetEntryCount();
	
	// copy to me
	if (oldReading != nil)
	{	*this = *oldReading;
	}
	
	// update me
	startTime		= Now();
	currSize		-= delSize;
	newSize			= 0;
	delSize			= 0;
}

void
SeismographReading::DrawReading(short hCoord, short topVCoord, short bottomVCoord, long loValue, long hiValue)
{
	long sizeAccum = 0;

	// -----------
	
	int i=kNumOldURLs;
	
	MoveTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
	while (i--)
	{
		if (oldURLAlloc[i] > 0)
		{
			RGBForeColor(&(kOldURLColorArray[oldURLColorIndex[i]]));
				//MoveTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
			sizeAccum += oldURLAlloc[i];
				LineTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
		}
	}
	
	RGBForeColor(&kCurrSizeColor);
		//MoveTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
	sizeAccum = currSize - delSize;
		LineTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
	
	RGBForeColor(&kDelSizeColor);
		//MoveTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
	sizeAccum = currSize;
		LineTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));

	// -----------
	
	sizeAccum = 0;
	
	RGBForeColor(&kCacheUsedColor);
		MoveTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
	sizeAccum += cacheUsed;
		LineTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
	
	RGBForeColor(&kCacheFreeColor);
		//MoveTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));
	sizeAccum += gRAMCache->GetLength() - cacheUsed;
		LineTo(hCoord, ValueToCoord(sizeAccum, topVCoord, bottomVCoord, loValue, hiValue));

	// -----------
}

short
SeismographReading::DrawReadingSummary(Rect *r)
{
	SeismographHeaderDispItem headerItem[] = 
		{
			{"Current URL",    (ulong)(currURL),            kString,              &kSeisChangeURLColor},
			{"Max Size",       (ulong)(currSize),           kDecLongPercentRAM,   &kCurrSizeColor},
			{"Allocated",      (ulong)(newSize),            kDecLong,             &kDelSizeColor},
			{"Released",       (ulong)(delSize),            kDecLong,             &kDelSizeColor},
			{"End Size",       (ulong)(currSize - delSize), kDecLongPercentRAM,   &kCurrSizeColor},
			{"Cache Used",     (ulong)(cacheUsed),          kCacheSummary,		 &kCacheUsedColor}
		};
	
	const colOffset = 75;
	const rowOffset = 12;
	const leftMargin = 4;
	const topMargin = 11;
	const bottomMargin = 3;

	struct MacPoint textPt;
	textPt.h = r->left + leftMargin;
	textPt.v = r->top + topMargin;
	short totalHeaderHeightLeft;
	short totalHeaderHeightRight;

	Rect eraseRect = *r;
	eraseRect.left += colOffset;		// don't erase static labels!
	eraseRect.bottom -= bottomMargin;	// don't erase divider between header & body
	EraseRect(&eraseRect);

	RGBColor saveColor;
	GetForeColor(&saveColor);
	
		TextFont(geneva);
		TextSize(9);
		
		char buffer[256];
		int i;
		
		for (i=0; i<(sizeof(headerItem) / sizeof(headerItem[0])); i++)
		{
			ulong numerator;
			ulong denomenator;
			ulong intPart;
			ulong remPart;
			RGBForeColor(headerItem[i].color);
			MoveTo(textPt.h, textPt.v);
				DrawText(headerItem[i].title, 0, strlen(headerItem[i].title));
			MoveTo(textPt.h + colOffset, textPt.v);
			
			switch (headerItem[i].coerceTo)
			{
				case kString:
					DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
												"%.*s", sizeof(buffer)-1, (char*)(headerItem[i].value)));
					break;
				case kDecLong:
					DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
												"%d", (long)(headerItem[i].value)));
					break;
				case kDecLongPercentRAM:
					numerator = headerItem[i].value;
					denomenator = gSimulator->GetRAMSize();
					intPart = (100*numerator) / denomenator;
					remPart = (100*((100*numerator) - (intPart*denomenator))) / denomenator;
					DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
												"%d (%2u.%.2u%% of RAM)",
												(ulong)(headerItem[i].value), intPart, remPart));
					break;
				case kHexLongPercentRAM:
					numerator = headerItem[i].value;
					denomenator = gSimulator->GetRAMSize();
					intPart = (100*numerator) / denomenator;
					remPart = (100*((100*numerator) - (intPart*denomenator))) / denomenator;
					DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
												"0x%.6x (%2d.%.2d%% of RAM)",
												(ulong)(headerItem[i].value), intPart, remPart));
					break;
				case kCacheSummary:
					numerator = headerItem[i].value;
					denomenator = gRAMCache->GetLength();
					intPart = (100*numerator) / denomenator;
					remPart = (100*((100*numerator) - (intPart*denomenator))) / denomenator;
					DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
												"%d bytes (%2d.%.2d%% of Cache)",
												(ulong)(headerItem[i].value), intPart, remPart));
					break;
				default:
					Complain(("SeismographReading::DrawReadingSummary() Didn't understand coerceTo field of %d.", (long)(headerItem[i].coerceTo)));
					break;
			}
			textPt.v += rowOffset;
		}
		totalHeaderHeightLeft = textPt.v - r->top;

		const URLOffset = 240;	

		textPt.h = r->left + leftMargin + URLOffset;
		textPt.v = r->top + topMargin + rowOffset;	// first row is blank for URL spill from 1st column

		long oldURLTotal = 0;
		
		for (i=0; i<kNumOldURLs; i++)
		{
			if ((oldURL[i] != nil) && (oldURLAlloc[i] > 0))
			{
				RGBForeColor(&(kOldURLColorArray[oldURLColorIndex[i]]));
				MoveTo(textPt.h, textPt.v);
				
				DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
											"(%d bytes) %.*s",
											oldURLAlloc[i],
											sizeof(buffer) - 21, /* 21 = "(%d bytes) " + NULL */
											oldURL[i]));
				textPt.v += rowOffset;
				oldURLTotal += oldURLAlloc[i];
			}
		}
		RGBForeColor(&saveColor);
		MoveTo(textPt.h, textPt.v);
				
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
									"Total = %d bytes", oldURLTotal));
		totalHeaderHeightRight = textPt.v - r->top;

	RGBForeColor(&saveColor);
	return (MAX(totalHeaderHeightLeft, totalHeaderHeightRight) + (2*bottomMargin));
}

short
SeismographReading::ValueToCoord(long value, short topCoord, short bottomCoord, long loValue, long hiValue)
{
	const short kMaxBottomCoord = 0x3fff;
	const short kMaxTopCoord  	= -kMaxBottomCoord;

#if defined(INTERAL_DEBUG_SEISMOGRAPH)
	Assert(topCoord < bottomCoord);
	Assert(loValue < hiValue);
#endif // defined(INTERAL_DEBUG_SEISMOGRAPH)

	long temp = (hiValue - value) / kSafetyDivide;
	temp *= ((long)(bottomCoord - topCoord));
	temp /= ((hiValue - loValue) / kSafetyDivide);
	temp += topCoord;

	if (temp < kMaxTopCoord)
	{	temp = kMaxTopCoord;
	}
	else if (temp > kMaxBottomCoord)
	{	temp = kMaxBottomCoord;
	}

	short result = temp;

#if defined(INTERAL_DEBUG_SEISMOGRAPH)
	if (value <= loValue)
	{
		if (result < bottomCoord)
		{
			Complain(("ValueToCoord(%l,%d,%d,%l,%l) wrapped around bottom", value, topCoord, bottomCoord, loValue, hiValue));
			return kMaxBottomCoord;
		}
	}
	else if (value >= hiValue)
	{
		if (result > topCoord)
		{
			Complain(("ValueToCoord(%l,%d,%d,%l,%l) wrapped around top", value, topCoord, bottomCoord, loValue, hiValue));
			return kMaxTopCoord;
		}
	}
	else	// since value was between lo and hi, result should be between top and bottom
	{
		Assert(topCoord <= result <= bottomCoord);
	}
#endif // defined(INTERAL_DEBUG_SEISMOGRAPH)
	return result;
}

long
MemorySeismographWindow::CoordToValue(short coord, short topCoord, short bottomCoord, long loValue, long hiValue)
{
	const long kMaxLoValue = 0;
	const long kMaxHiValue = gSimulator->GetRAMSize();

#if defined(INTERAL_DEBUG_SEISMOGRAPH)
	Assert(topCoord < bottomCoord);
	Assert(loValue < hiValue);
#endif // defined(INTERAL_DEBUG_SEISMOGRAPH)

	long result = bottomCoord - coord;
	result *= ((hiValue - loValue) / kSafetyDivide);
	result /= ((long)(bottomCoord - topCoord));
	result *= kSafetyDivide;
	result += loValue;

	if (result < kMaxLoValue)
	{	result = kMaxLoValue;
	}
	else if (result > kMaxHiValue)
	{	result = kMaxHiValue;
	}

#if defined(INTERAL_DEBUG_SEISMOGRAPH)
	if (coord <= topCoord)
	{
		if (result < hiValue)
		{
			Complain(("CoordToValue(%hd,%hd,%hd,%ld,%ld) overflowed",
						coord, topCoord, bottomCoord, loValue, hiValue));
			return kMaxHiValue;
		}
	}
	else if (coord >= bottomCoord)
	{
		if (result > loValue)
		{
			Complain(("CoordToValue(%hd,%hd,%hd,%ld,%ld) overflowed",
						coord, topCoord, bottomCoord, loValue, hiValue));
			return kMaxLoValue;
		}
	}
	else
	{
		Assert(loValue <= result <= hiValue);
	}
#endif // defined(INTERAL_DEBUG_SEISMOGRAPH)
	return result;
}

// -----------------------------------------------------------------------------------

MemorySeismographWindow::MemorySeismographWindow()
{
	if (gMemorySeismographWindow != nil)
		delete gMemorySeismographWindow;
	
	gMemorySeismographWindow = this;

	int i=kSeismographLogSize;
	fTimeInterval = kSeismographTimeInterval;

	fActiveReading = 0;
	fDisplayReading = 0;
	fUpdateReadings = 0;	// no new drawing to do
	fAdjustHeaderHeight = 30;	// ah, start with SOMETHING...

	fDispMin = 0;
	fDispMax = gSimulator->GetRAMSize();

	SetFormat(fAdjustHeaderHeight, 0, false, false);	// header, trailer, vscroll, hscroll	
	ResizeWindow(kSeismographLogSize, 300);
	SetTitle("Memory Seismograph");
}

MemorySeismographWindow::~MemorySeismographWindow()
{
	gMemorySeismographWindow = nil;
}

void
MemorySeismographWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);

	MenuHandle menu = GetMenu(mMemory);
	EnableItem(menu, iSeismograph);
	SetItemMark(menu, iSeismograph, GetVisible() ? checkMark : ' ');
	SetMenuItemText(menu, iSeismograph, GetVisible() ? "\pHide MemorySeismograph"
													 : "\pShow MemorySeismograph");
}

Boolean
MemorySeismographWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	Boolean handled = false;
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);

	if ((theMenu == mMemory) && (theItem == iSeismograph))
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
MemorySeismographWindow::TaggedAlloc(MemoryTag* tag)
{
	CheckForAdvance();
	fReading[fActiveReading].TaggedAlloc(tag);
}

void
MemorySeismographWindow::TaggedFree(MemoryTag* tag)
{
	CheckForAdvance();
	fReading[fActiveReading].TaggedFree(tag);
}

void
MemorySeismographWindow::TaggedNew(MemoryTag* tag)
{
	CheckForAdvance();
	fReading[fActiveReading].TaggedNew(tag);
}

void
MemorySeismographWindow::TaggedDelete(MemoryTag* tag)
{
	CheckForAdvance();
	fReading[fActiveReading].TaggedDelete(tag);
}

void
MemorySeismographWindow::Click(struct MacPoint* where, ushort modifiers)
{
	if (PtInRect(*where, &fGraphRect))
	{
		if (modifiers & cmdKey)	// set display to this point
		{
			DrawGraph(&fGraphRect);	// make sure graph is up-to-date
			ChooseDisplayReading(where, modifiers);
		}
		else if (StillDown())	// drag to select
		{
			DrawGraph(&fGraphRect);	// make sure graph is up-to-date
			RescaleGraph(where, modifiers);
		}
	}
}

void
MemorySeismographWindow::GetGraphRect(Rect* graphRect)
{
	*graphRect = fGraphRect;
}


void
MemorySeismographWindow::DrawBody(Rect* UNUSED(r), short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	// draw bounds
		Rect r;
		GetBodyRect(&r);
		fGraphRect = r;
		r.right = r.left + 60;
		fGraphRect.left = r.right;
		EraseRect(&r);
		
		MoveTo(r.right - 1, r.top);
		LineTo(r.right - 1, r.bottom);
		
		TextFont(geneva);
		TextSize(9);

		char buffer[12]; /* 11 for %ld plus 1 for NULL */
		
		MoveTo(r.left + 4, r.top + 14);
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "%8ld", fDispMax));
		
		MoveTo(r.left + 4, ((r.top + r.bottom)/2) - 9);
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "Range:"));

		MoveTo(r.left + 4, ((r.top + r.bottom)/2) + 3);
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "%8ld", fDispMax - fDispMin));

		MoveTo(r.left + 4, r.bottom - 4);
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "%8ld", fDispMin));

	// recalc fGraphRect
	if (fGraphRect.right > fGraphRect.left + kSeismographLogSize)
	{
		fGraphRect.right = fGraphRect.left + kSeismographLogSize;
	}

	// now draw to it
	DrawGraph(&fGraphRect);
}


// assume port is set to this window and rect is in proper coords for this window
void
MemorySeismographWindow::DrawGraph(Rect* graphRect)
{
	RgnHandle saveClipRgn = NewRgn();
	RgnHandle newClipRgn = NewRgn();
	GetClip(saveClipRgn);
	RectRgn(newClipRgn, graphRect);
	SectRgn(saveClipRgn, newClipRgn, newClipRgn);
	SetClip(newClipRgn);
	DisposeRgn(newClipRgn);

	long length = (fDispMax - fDispMin) / kSafetyDivide;
	long rectHeight = graphRect->bottom - graphRect->top;
	
	RGBColor saveColor;
	GetForeColor(&saveColor);
	
	short leftUpdateEdge = (**(w->visRgn)).rgnBBox.left;
	short rightUpdateEdge = (**(w->visRgn)).rgnBBox.right;
	
	// h = horizontal position, i counts from 0 to num log entries - 1
	short hCoord=graphRect->left, hCoordEnd = graphRect->right;
	short i=0;
	
	if (leftUpdateEdge > graphRect->left)
	{	hCoord = leftUpdateEdge;
		i = leftUpdateEdge - graphRect->left;
	}
	if (rightUpdateEdge < graphRect->right)
	{	hCoordEnd = rightUpdateEdge;
	}

	// convert i to meaningful index value
	short readingIndex=(fActiveReading + i) % kSeismographLogSize;
	
	while(hCoord<=hCoordEnd && i<kSeismographLogSize && fReading[readingIndex].startTime!=0)
	{
		SeismographReading* reading = &(fReading[readingIndex]);
		short prevReadingIndex = readingIndex - 1;
		if (prevReadingIndex < 0)
			prevReadingIndex = kSeismographLogSize - 1;
		SeismographReading* prevReading = &(fReading[prevReadingIndex]);
		
		if ((i == 0) || (i == kSeismographLogSize - 1) || (reading->currURL == prevReading->currURL))
			RGBForeColor(&kSeisBackgroundColor);
		else
			RGBForeColor(&kSeisChangeURLColor);

		MoveTo(hCoord, graphRect->top);
		LineTo(hCoord, graphRect->bottom);
	
		reading->DrawReading(hCoord, graphRect->top, graphRect->bottom, fDispMin, fDispMax);

		hCoord++;
		i++;
		readingIndex = prevReadingIndex;
	}
	fUpdateReadings = 0;		// we've updated necessary parts...
	RGBForeColor(&saveColor);
	SetClip(saveClipRgn);
	DisposeRgn(saveClipRgn);
}

void
MemorySeismographWindow::DrawHeader(Rect *r)
{
	RgnHandle saveClipRgn = NewRgn();
	RgnHandle newClipRgn = NewRgn();
	GetClip(saveClipRgn);
	RectRgn(newClipRgn, r);
	SectRgn(saveClipRgn, newClipRgn, newClipRgn);
	SetClip(newClipRgn);
	DisposeRgn(newClipRgn);

	short newHeaderHeight = fReading[fDisplayReading].DrawReadingSummary(r);
	if (fAdjustHeaderHeight < newHeaderHeight)
	{	fAdjustHeaderHeight = newHeaderHeight;
	}
	
	SetClip(saveClipRgn);
	DisposeRgn(saveClipRgn);
	
};

void
MemorySeismographWindow::Idle(void)
{
	short windowHeight = w->portRect.bottom - w->portRect.top;

	if ((fAdjustHeaderHeight > mHeaderHeight) && (fAdjustHeaderHeight < windowHeight/2))
	{
		if (GetVisible())
		{
			GrafPtr savePort;
			GetPort(&savePort);
			SetPort(w);
			InvalRect(&(w->portRect));
			EraseRect(&(w->portRect));
			SetPort(savePort);
		}
		mHeaderHeight = fAdjustHeaderHeight;
		SetMinMaxRect(nil);
	}
	
	CheckForAdvance();
	
	StdWindow::Idle();
}

void
MemorySeismographWindow::Close(void)
{
	HideWindow();
}

void
MemorySeismographWindow::CheckForAdvance(void)
{
	if ((fReading[fActiveReading].currURL != gLastRequestedURL)
		|| (fReading[fActiveReading].startTime + fTimeInterval <= Now()) && (fReading[fActiveReading].newSize != 0))
	{
		// update internal data
		
		short nextReading = fActiveReading + 1;
		if (nextReading == kSeismographLogSize) 
			nextReading = 0;
		if (fDisplayReading == fActiveReading)
			fDisplayReading = nextReading;		
		
		fReading[nextReading].AssumeActiveFrom(&fReading[fActiveReading]);
		
		fActiveReading = nextReading;
		
		// update view
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		
		Rect r = fGraphRect;
		r.left += fUpdateReadings;
		if (r.left+2 <= r.right)	// at least two pixels wide to scroll...one src pixel, one dest pixel
		{
			RgnHandle rgn = NewRgn();			
			ScrollRect(&r,1,0,rgn);
			InvalRgn(rgn);
			DisposeRgn(rgn);
			fUpdateReadings++;
		}

		GetHeaderRect(&r);
		InvalRect(&r);
		SetPort(savePort);
	}
	fReading[fActiveReading].currURL = gLastRequestedURL;
}

void
MemorySeismographWindow::ChooseDisplayReading(struct MacPoint* where, ushort UNUSED(modifiers))
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	PenState oldPS, newPS;
	GetPenState(&oldPS);

	PenMode(patXor);
	PenPat(&(qd.gray));
	GetPenState(&newPS);

		Rect headerRect;
		GetHeaderRect(&headerRect);
		
		struct MacPoint readingPoint = *where;
		MoveTo(readingPoint.h, fGraphRect.top);
		LineTo(readingPoint.h, fGraphRect.bottom);
		while (StillDown())
		{
			struct MacPoint newPoint;
			GetMouse(&newPoint);
			if (newPoint.h < fGraphRect.left)
			{	newPoint.h = fGraphRect.left;
			}
			if (newPoint.h >= fGraphRect.right)
			{	newPoint.h = fGraphRect.right - 1;
			}
			
			if (newPoint.h != readingPoint.h)
			{	MoveTo(readingPoint.h, fGraphRect.top);
				LineTo(readingPoint.h, fGraphRect.bottom);
				MoveTo(newPoint.h, fGraphRect.top);
				LineTo(newPoint.h, fGraphRect.bottom);
				readingPoint = newPoint;
				fDisplayReading = (fActiveReading - (newPoint.h - fGraphRect.left));
				if (fDisplayReading < 0)
					fDisplayReading += kSeismographLogSize;
				InvalRect(&headerRect);
				SetPenState(&oldPS);
				DrawHeader(&headerRect);
				SetPenState(&newPS);
			}
			readingPoint = newPoint;
		}
		MoveTo(readingPoint.h, fGraphRect.top);
		LineTo(readingPoint.h, fGraphRect.bottom);
	
	fDisplayReading = fActiveReading;
	SetPenState(&oldPS);
	SetPort(savePort);
}

void
MemorySeismographWindow::RescaleGraph(struct MacPoint* where, ushort modifiers)
{
	const kMinDisplaySize = 16;	// entire display should not represent less than 16 bytes...

	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	PenState ps;
	GetPenState(&ps);

	PenMode(patXor);
	PenPat(&(qd.gray));

		struct MacPoint endPoint = *where;
		while (StillDown())
		{
			struct MacPoint newPoint;
			GetMouse(&newPoint);
			if (newPoint.v < fGraphRect.top)
			{	newPoint.v = fGraphRect.top;
			}
			else if (newPoint.v > fGraphRect.bottom-1)
			{	newPoint.v = fGraphRect.bottom-1;
			}
			if (newPoint.v != endPoint.v)
			{	MoveTo(fGraphRect.left, endPoint.v);
				LineTo(fGraphRect.right, endPoint.v);
				MoveTo(fGraphRect.left, newPoint.v);
				LineTo(fGraphRect.right, newPoint.v);
			}
			endPoint = newPoint;
		}
		MoveTo(fGraphRect.left, where->v);
		LineTo(fGraphRect.right, where->v);
		MoveTo(fGraphRect.left, endPoint.v);
		LineTo(fGraphRect.right, endPoint.v);

		short topCoord = where->v;
		short bottomCoord = endPoint.v;
	
		if (where->v > endPoint.v)
		{	topCoord = endPoint.v;
			bottomCoord = where->v;
		}
	
		if (topCoord + 3 < bottomCoord)		// now recalculate bounds
		{
			long newMinValue, newMaxValue;
			
			if (modifiers & optionKey)	// expand view area
			{
				newMinValue = CoordToValue(fGraphRect.bottom-1, topCoord, bottomCoord, fDispMin, fDispMax);
				newMaxValue = CoordToValue(fGraphRect.top,      topCoord, bottomCoord, fDispMin, fDispMax);
				if (newMaxValue - newMinValue < kMinDisplaySize)	// make sure we've got SOME space
				{
					newMinValue = (newMaxValue+newMinValue)/2;
					newMaxValue = newMinValue;
					newMinValue -= kMinDisplaySize/2;
					newMaxValue += kMinDisplaySize/2;
				}
			}
			else
			{
				newMinValue = CoordToValue(bottomCoord, fGraphRect.top, fGraphRect.bottom-1, fDispMin, fDispMax);
				newMaxValue = CoordToValue(topCoord,    fGraphRect.top, fGraphRect.bottom-1, fDispMin, fDispMax);
			}
			
			fDispMin = newMinValue;
			fDispMax = newMaxValue;
	
			Rect r;
			GetBodyRect(&r);
			InvalRect(&r);
		}
		
	SetPenState(&ps);
	SetPort(savePort);
}

#endif /* DEBUG_MEMORYSEISMOGRAPH */