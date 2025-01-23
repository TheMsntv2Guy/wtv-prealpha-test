// ===========================================================================
//	Page.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGE_H__
#include "Page.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __STATUS_H__
#include "Status.h"
#endif
#ifndef __TABLE_H__
#include "Table.h"
#endif
#ifndef __TEXT_H__
#include "Text.h"
#endif




// ===========================================================================
//	local constants
// ===========================================================================

static const long kBlankLineHeight = 14;
static const long kInterLineHeight = 2;
static const long kMinimumVisibleLineSize = 5;
static const long kMinimumMarginWidth = 20;




// ===========================================================================
// As a line is being layed out vertically, baseline is at zero.
// Top is set to mark ascent, top + height defines descent
// lastDisplayable may be a line break or zero
// Text has already been measured, top is set to - ascent
// All displayables have been positioned horizontally


Page::Page()
{
	fPageAlign = AV_LEFT;
	fAlign = AV_LEFT;
	fVAlign = AV_TOP;

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberPage;
#endif /* DEBUG_CLASSNUMBER */
}

Page::~Page()
{
}

void Page::AddFloatingChild(Displayable* child)
{
	// Remember floating images for later
	
	fFloatingList.Add(child);
	AddChild(child);
}

long Page::BreakLine(long vPos, short breakType)
{
	// Handle a line break .. may be a clear break

	// Only floating images can alter left or right margins.
	long count = fFloatingList.GetCount();
	if (count == 0)
		return 0;
	
	long leftBottom = -1;
	long rightBottom = -1;

	for (long i = 0; i < count; i++) 
	{		
		Rectangle r;
		Displayable* floater = (Displayable *)fFloatingList.At(i);
		if (floater->IsLayoutComplete()) 
		{			
			floater->GetBounds(&r);
			if (vPos >= r.top && vPos < r.bottom) 
			{				
				if (floater->GetAlign() == AV_LEFT || 
					floater->GetAlign() == AV_BLEEDLEFT)
					leftBottom = MAX(leftBottom, r.bottom);
				else
					rightBottom = MAX(rightBottom, r.bottom);
			}
		}
	}
	
	if (leftBottom == -1 && rightBottom == -1)
		return 0;

	// Break left, right or both

	switch (breakType) 
	{		
		case LineBreak::kHardLeft:
			if (leftBottom != -1)
				return leftBottom - vPos;
			break;
		case LineBreak::kHardRight:
			if (rightBottom != -1)
				return rightBottom - vPos;
			break;
		case LineBreak::kHardAll:
			return MAX(leftBottom, rightBottom) - vPos;
			break;
	}
	
	return 0;
}

void Page::Close()
{
	fOpen = false;
}

void Page::GetMargins(long vPos, short* left, short* right) const
{
	// Get left and right margins at current VPos
	
	*left = fLeft + fLeftMarginDefault;
	*right = fLeft + GetWidth();
	
	// Only floating images can alter left or right margins.
	long count = fFloatingList.GetCount();
	
	for (long i = 0; i < count; i++) 
	{		
		Rectangle r;
		Displayable* floater = (Displayable *)fFloatingList.At(i);

		if (floater->IsLayoutComplete()) 
		{			
			floater->GetBounds(&r);
			if (vPos >= r.top && vPos < r.bottom) 
			{				
				if (floater->GetAlign() == AV_LEFT ||
					floater->GetAlign() == AV_BLEEDLEFT)
					*left = MAX(*left, r.right);
				else
					*right = MIN(*right, r.left);
			}
		}
	}
	
	// Constrain margins to reasonable size.
	if ((fLeft + fWidth - *left) < kMinimumMarginWidth)
		*left = MAX(fLeft, fLeft + fWidth - kMinimumMarginWidth);
	if ((*right - *left) < kMinimumMarginWidth)
		*right = MIN(*left + kMinimumMarginWidth, fLeft + fWidth);
}

long Page::GetDefaultMarginWidth() const
{
	// Width between left and right default margins
	return fWidth - fLeftMarginDefault;
}

long Page::GetMarginWidth() const
{
	// Width between left and right margins
	return fRightMargin - fLeftMargin;
}


long Page::GetMaxUsedWidth(const Document*) const
{
	// Maximum width used in the last layout
	return fMaxHPos - fLeft;
}

short Page::GetMinUsedWidth(const Document* document) const
{
	short minWidth = 0;
	
	// Compute the minimum width by finding the longest string of non-separable displayables.
	Displayable* d = GetFirstChild();
	while (d != nil)
	{
		short runWidth = d->GetMinUsedWidth(document);
		while (d->Next() != nil && !((Displayable*)d->Next())->IsSeparable(document))
		{
			d = (Displayable*)d->Next();
			runWidth += d->GetMinUsedWidth(document); // If next is text, it may return a min value for a larger word
													  // than the one that is adjacent, and cannot be separated.
		}
		
		minWidth = MAX(runWidth, minWidth);
		
		d = (Displayable*)d->Next();
	}
		
	return minWidth;	
}

void Page::Layout(Document* document, Displayable*)
{
	// Layout all children that are ready. Remember where we left off.
	
	if (fNextToPut == nil)
	{
		if (fLastPut == nil)
		{
			ResetLayout(document);
			fNextToPut = GetFirstChild();
		}
		else
			fNextToPut = (Displayable*)fLastPut->Next();
	}
	
	while (fNextToPut && fNextToPut->ReadyForLayout(document))
	{
		fLastPut = fNextToPut;
		fNextToPut = PutDisplayable(document, fNextToPut);	// Sets fMaxHPos ...
	}

	if (fNextToPut == nil && !fOpen)	// Done, finish line in progress.
	{
		PutLine(document, 0);
		LayoutDeferedFloating(document);		
		fVPos += BreakLine(fVPos, LineBreak::kHardAll);	// Clear floaters

		fLastPut = nil;
	}
		
	fHeight = fVPos - fTop;
}

void Page::LayoutCompleteFor(Displayable*, Document*)
{
	// Only root page marks LayoutComplete as progress is made.
}

void
Page::ResetLayout(Document* document)
{
	CompositeDisplayable::ResetLayout(document);

	RemoveSoftBreaks();
	fAlign = fPageAlign;
	fLeftMarginDefault = 0;
	fLeftMargin = fLeft;
	fRightMargin = fLeft + GetWidth();
	fLineBegin = nil;
	fMaxHPos = 0;
	fVPos = fTop;
	fLastPut = nil;
	fNextToPut = nil;
	PrepareFloating(document);
}

void Page::LayoutFloatingDisplayable(Document* document, Displayable* item)
{
	// Floating images are layed out relative to margins
	
	switch (item->GetAlign()) 
	{		
		case AV_LEFT:
			item->SetTop(fVPos);
			item->SetLeft(fLeftMargin);
			break;
		case AV_BLEEDLEFT:
			item->SetTop(fVPos);
			item->SetLeft(fLeft);
			break;
		case AV_RIGHT:
			item->SetTop(fVPos);
			item->SetLeft(fRightMargin - item->GetWidth());
			break;
		case AV_BLEEDRIGHT:
			item->SetTop(fVPos);
			item->SetLeft(fLeft + fWidth - item->GetWidth());
			break;
		default:
			break;
	}
	item->LayoutComplete(document, this);
	if (item != fDeferredFloating)
		fLastLayedOut = item;
}

void Page::DeferFloatingLayout(Displayable *item)		// Defer layout until newline
{
	fDeferredFloating = item;
}

void Page::LayoutDeferedFloating(Document* document)
{
	if (fDeferredFloating != nil) {
		// If we can't fit in the current margin, try clearing current floaters.
		if (fDeferredFloating->GetWidth() > GetMarginWidth())
			fVPos += BreakLine(fVPos, LineBreak::kHardAll);
		LayoutFloatingDisplayable(document, fDeferredFloating);
		fHPos = fLeftMargin;
		
		// Look for an anchor end that includes this image and has already been layed out.
		// We need to lay it out again to add the selectable now that the image is layed out.
		Displayable*	item;
		for (item = (Displayable*)fDeferredFloating->Next(); 
			 item != nil && !item->IsAnchor() && !item->IsAnchorEnd();
			 item = (Displayable*)item->Next())
			;
		if (item != nil && item->IsAnchorEnd() && ((AnchorEnd*)item)->IsLayoutComplete())
			item->LayoutComplete(document, this);
				
		fDeferredFloating = nil;
	}
}

long Page::LayoutLineV(Document* document, long vPos, short space, short align, Displayable* firstDisplayable, Displayable* lastDisplayable)
{
	// Calculate vertical positions of displayables that have been arranged horizontally
	// Note: LineBreaks START a line, not finish it
	
	long	maxAscent = 0;
	long	maxDescent = 0;
	long	height, ascent, hOffset;
	Displayable* item;

	if (firstDisplayable == nil)
		return vPos;

	// Figure the tallest piece of text in this line chunk

	for (item = firstDisplayable; item; item = (Displayable*)item->Next())
	{
		if (item->GetAlign() == 0)
		{
			maxAscent = MAX(maxAscent, -item->GetTop());
			maxDescent = MAX(maxDescent,item->GetTop() + item->GetLayoutHeight());
		}

		if (item == lastDisplayable)
			break;
	}
	
	// Position images for AV_ALIGNTEXTTOP, AV_MIDDLE, AV_BOTTOM, AV_BASELINE
	
	for (item = firstDisplayable; item; item = (Displayable*)item->Next())
	{
		height = item->GetLayoutHeight();
		switch(item->GetAlign()) 
		{			
			case AV_TEXTTOP:								// Align to top of text
				item->SetTop(-maxAscent);
				maxDescent = MAX(maxDescent, height - maxAscent);
				break;
			case AV_MIDDLE:									// Center on baseline
				item->SetTop(-height/2);
				maxAscent = MAX(maxAscent, height/2);
				maxDescent = MAX(maxDescent, height - height/2);
				break;
			case AV_CENTER:
			case AV_BOTTOM:									// Sit on baseline
			case AV_BASELINE:
				item->SetTop(-height);
				maxAscent = MAX(maxAscent, height);
				break;
			default:
				break;
		}

		if (item == lastDisplayable)
			break;
	}

	// Position images for AV_TOP, AV_ABSMIDDLE, AV_ABSBOTTOM
	
	for (item = firstDisplayable; item; item = (Displayable*)item->Next())
	{
		height = item->GetLayoutHeight();
		switch(item->GetAlign()) 
		{			
			case AV_TOP:									// Align to top of line (text or images)
				item->SetTop(-maxAscent);
				maxDescent = MAX(maxDescent, height - maxAscent);
				break;
			case AV_ABSMIDDLE:								// Center in line
				ascent = maxAscent - ((maxAscent + maxDescent) - height)/2;
				item->SetTop(-ascent);
				maxAscent = MAX(maxAscent, ascent);
				maxDescent = MAX(maxDescent, height - ascent);
				break;
			case AV_ABSBOTTOM:								// Sit on bottom of line
				item->SetTop(-(height - maxDescent));
				maxAscent = MAX(maxAscent, height - maxDescent);
				break;
			default:
				break;
		}

		if (item == lastDisplayable)
			break;
	}
	
	// Calculate the required horizontal offset

	switch (align) 
	{		
		case AV_CENTER:	hOffset = MAX(0, space/2);	break;
		case AV_RIGHT:	hOffset = MAX(0, space);	break;
		default:		hOffset = 0;
	}

	// Baseline is vpos + maxAscent, move everybody there

	vPos += maxAscent;
	for (item = firstDisplayable; item; item = (Displayable*)item->Next())
	{
		if (!item->IsFloating()) 
		{			
			item->SetTop(item->GetTop() + vPos);
			item->SetLeft(item->GetLeft() + hOffset);
			LayoutCompleteFor(item, document);
			fLastLayedOut = item;
		}
		if (item == lastDisplayable)
			break;
	}
	vPos += maxDescent;
	return vPos;		// Return the new vPos
}

void Page::Open()
{
	fOpen = true;
}

void Page::PrepareFloating(Document* document)
{
	// Ignore Floating displayables until their Layout is Complete
	
	long count = fFloatingList.GetCount();
	
	for (long i = 0; i < count; i++) 
	{		
		Displayable* floater = (Displayable *)fFloatingList.At(i);
		floater->Layout(document, this);	// Starts layout early for floaters
	}
}

Displayable* Page::PutDisplayable(Document* document, Displayable* item)
{
	// Adds a item to the current line
	// If it fits, great
	// If it doesn't fit and there is only one item on the line (wide image), draw it
	// If it doesn't fit draw displayables before it, try again with it

	if (item->IsLineBreak())
		PutLine(document, (Displayable *)item->Previous());	// Layout line, stop before this item
	
	if (!fLineBegin) {				// Start a new line
		if (item->IsLineBreak())
		{
			long	v = BreakLine(fVPos, item->GetBreakType());	// Move vPos if this was a clear break
			if (v == 0 && item->IsExplicitLineBreak() && 
				(item->Previous() == nil || ((Displayable*)item->Previous())->IsExplicitLineBreak()))
				v = kBlankLineHeight;
			fVPos += v;
		}
		fLineBegin = item;
		LayoutDeferedFloating(document);			// Layout any floaters that need attention
		GetMargins(fVPos,&fLeftMargin,&fRightMargin);
		fHPos = fLeftMargin;
	}
	
	item->Layout(document, this);
	short width = item->GetLayoutWidth();

	if (item->IsFloating()) {				// Floating Displayables
		short	align = item->GetAlign();
		
		// If image is left aligned and we are not at the left margin, start new line.
		if ((align == AV_LEFT || align == AV_BLEEDLEFT) && fHPos != fLeftMargin) {
			PutLine(document, (Displayable*)item->Previous());
			return item;
		}
		
		// If the image is right aligned and will not fit on current line, wait until next line break.
		if ((align == AV_RIGHT || align == AV_BLEEDRIGHT) && width > (fRightMargin - fHPos)) {
			DeferFloatingLayout(item);
			return (Displayable*)item->Next();
		}
		
		LayoutFloatingDisplayable(document, item);
		short oldLeftMargin = fLeftMargin;
		GetMargins(fVPos,&fLeftMargin,&fRightMargin);
		fHPos += fLeftMargin - oldLeftMargin;	// Update current position for new margin.
		return (Displayable*)item->Next();
	}

	item->SetLeft(fHPos);
	
	// If the item fits, great

	if (fHPos + width <= fRightMargin) {	// Displayable Fits
		fHPos += width;
		if (fHPos == fRightMargin)
			PutLine(document, item);			// Put line, stop after this item
		return (Displayable *)item->Next();
	}
	
	// Try to break a piece of text to make it fit
	
	if (item->IsText()) {		
		short pos = ((Text *)item)->BreakText(document, fRightMargin - fHPos, fHPos != fLeftMargin);
		
		if (pos > 0 || (item->IsSeparable(document) && fHPos != fLeftMargin))
		{	
			((Text *)item)->InsertSoftBreak(document, this, pos);
			fHPos += item->GetWidth();
			return (Displayable *)item->Next();	// Soft Linebreak will envoke PutLine next time
		}
	}
	
	// Displayable isn't breakable and does not fit. Give up if it is the only one on the line

	if (fHPos == fLeftMargin) {				// Single item does not fit (wide image, for example)
		fVPos += BreakLine(fVPos, LineBreak::kHardAll);	// Clear any floaters.
		GetMargins(fVPos,&fLeftMargin,&fRightMargin);
		fHPos = fLeftMargin;
		item->SetLeft(fHPos);
		fHPos += width;
		PutLine(document, item);			// Put line, stop after this item
		return (Displayable*)item->Next();
	}
	
	// Backup until we can separate item from previous one
	
	while (!item->IsSeparable(document)) 
	{		
		if ((Displayable*)item->Previous() == fLineBegin)
			break;
		item = (Displayable *)item->Previous();
		fHPos -= item->GetWidth();
	}
	
	PutLine(document, (Displayable*)item->Previous());		// Draw line, stop before this item
	return item;							// Try again next line
}

void Page::PutLine(Document* document, Displayable* lineEnd)
{
	// Vertically layout the line we have assembled

	if (fLineBegin == nil)
		return;

	fVPos = LayoutLineV(document, fVPos, fRightMargin - fHPos, fAlign, fLineBegin, lineEnd);
	
	// Use min width for dividers. They derive size from page size.
	if (fLineBegin->IsDivider())
		fMaxHPos = MAX(fMaxHPos, fLineBegin->GetMinUsedWidth(document));
	else
		fMaxHPos = MAX(fMaxHPos, fHPos);
		
	fHPos = fLeftMargin;
	fLineBegin = nil;
}

void Page::RemoveSoftBreaks()
{
	// Remove all soft breaks from the page, layout will generate more
	
	Displayable	*item;

	for (item = GetFirstChild(); item; item = (Displayable*)item->Next()) 
		if (item->GetBreakType() == LineBreak::kSoft)
			item = ((LineBreak *)item)->DeleteIfSoftBreak(this); 
}

Rectangle Page::GetLineBounds(Displayable* lineStart) const
{	
	Rectangle lineBounds = {0, 0, 0, 0};
	
	// Skip leading linebreaks;
	Displayable*	item;
	for (item = lineStart; item != nil && item->IsLineBreak(); item = (Displayable*)item->Next())
		;

	for (; item != nil && !item->IsLineBreak(); item = (Displayable*)item->Next())
	{
		Rectangle itemBounds;
		item->GetBounds(&itemBounds);
		
		if (!EmptyRectangle(&itemBounds)) // Skip empty bounds of alignments, anchor end, etc.
		{
			if (EmptyRectangle(&lineBounds))
			{
				lineBounds = itemBounds;
			}
			else
			{
				if (itemBounds.top < lineBounds.top)
					lineBounds.top = itemBounds.top;
				if (itemBounds.bottom > lineBounds.bottom)
					lineBounds.bottom = itemBounds.bottom;
				if (itemBounds.left < lineBounds.left)
					lineBounds.left = itemBounds.left;
				if (itemBounds.right > lineBounds.right)
					lineBounds.right = itemBounds.right;
			}
		}
	}
	
	return lineBounds;
}

void Page::SetLeftMarginDefault(short leftMarginDefault)
{
	// If margin is 0, no indent, just follow floaters.
	if (leftMarginDefault == 0)
		fLeftMarginDefault = 0;
	else
	{
		// First calculate margin due just to floaters.
		fLeftMarginDefault = 0;
		GetMargins(fVPos, &fLeftMargin, &fRightMargin);
		
		// Now set new default margin relative to that.
		fLeftMarginDefault = fLeftMargin - fLeft + leftMarginDefault;
	}
	
	GetMargins(fVPos, &fLeftMargin, &fRightMargin);
	fHPos = fLeftMargin;
}

void Page::SetStyleStackDepth(ushort depth)
{
	fStyleStackDepth = depth;
}
	
ushort Page::GetStyleStackDepth() const
{
	return fStyleStackDepth;
}

void Page::SetAlignStackDepth(ushort depth)
{
	fAlignStackDepth = depth;
}
	
ushort Page::GetAlignStackDepth() const
{
	return fAlignStackDepth;
}


// ===========================================================================

RootPage::RootPage()
{
}

RootPage::~RootPage()
{
}

void RootPage::SetTopDisplayable(Displayable* top)
{
	fTopDisplayable = top;
}

Displayable* RootPage::GetTopDisplayable() const
{
	return fTopDisplayable;
}

Boolean RootPage::IsLayoutCurrent(const Document* document) const
{	
	Boolean current = (GetFirstChild() == nil || fChildren.Last() == fLastPut || fChildren.Last() == fLastLayedOut);
	
	// Special case tables. If the table is only blocked because its still open
	// and all of its current children are ready, we're still current.
	if (!current && fNextToPut != nil && fNextToPut->IsTable() &&
	    ((Table*)fNextToPut)->CompositeDisplayable::ReadyForLayout(document))
		current = true;
		
	return current;
}

void RootPage::LayoutCompleteFor(Displayable* item, Document* document)
{
	if (!item->IsLayoutComplete())
		item->LayoutComplete(document, this);
}

void RootPage::Draw(const Document* document, const Rectangle* invalid)
{
	// Check for nothing layed out.
	if (fLastLayedOut == nil) return;
	
	// The visible bounds is used to determine if a line fits on the screen.
	Rectangle visibleBounds;
	document->GetView()->VisibleContentBounds(visibleBounds);

	// Invalid content bounds are used to skip drawing of displayables outside of the invalid area.
	Rectangle invalidContentBounds = *invalid;
	document->GetView()->ScreenToContent(&invalidContentBounds);
	
	// Skip invisible children. If we don't know the top displayable, find the first visible displayable.
	Displayable* child = fTopDisplayable;

	if (child == nil)
	{
		for (child = GetFirstChild();
			 child != fLastLayedOut->Next() && (child->GetTop() + child->GetHeight()) <= visibleBounds.top;
			 child = (Displayable*)child->Next())
			;
		fTopDisplayable = child;
	}

	// Draw visible children in the invalid area. We stop drawing at the first displayable
	// which has not yet been layed out. If fNextToLayout is nil, layout is complete and we
	// will draw to the end of the display list.
	while (child != fLastLayedOut->Next())
	{
		Rectangle	lineVBounds = GetLineBounds(child);
		
		if ((visibleBounds.bottom - lineVBounds.top) > kMinimumVisibleLineSize)
		{
			do
			{
				Rectangle	childBounds;
				child->GetBounds(&childBounds);
				if (RectanglesIntersect(&childBounds, &invalidContentBounds))
					child->Draw(document, invalid);

				child = (Displayable*)child->Next();
			} while (child != fLastLayedOut->Next() && !child->IsLineBreak());
		}
		else
			break;
	}
	
	// Draw any floaters that are clipped at the top of screen
	for (long i = 0; i < fFloatingList.GetCount(); i++)
	{
		Displayable* item = (Displayable*)fFloatingList.At(i);
		
		if (item->GetTop() > visibleBounds.top)
			break;
			
		Rectangle	itemBounds;
		item->GetBounds(&itemBounds);
		if (item->IsLayoutComplete() && RectanglesIntersect(&itemBounds, &invalidContentBounds))
			item->Draw(document, invalid);
	}
}


#ifdef FIDO_INTERCEPT
void RootPage::Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const
{
	// Check for nothing layed out.
	if (fLastLayedOut == nil) return;
	
	// The visible bounds is used to determine if a line fits on the screen.
	Rectangle visibleBounds;
	document->GetView()->VisibleContentBounds(visibleBounds);
	
	// Skip invisible children. If we don't know the top displayable, find the first visible displayable.
	Displayable* child = fTopDisplayable;

	if (child == nil)
	{
		for (child = GetFirstChild();
			 child != fLastLayedOut->Next() && (child->GetTop() + child->GetHeight()) <= visibleBounds.top;
			 child = (Displayable*)child->Next())
			;
	}

	// Draw visible children in the invalid area. We stop drawing at the first displayable
	// which has not yet been layed out. If fNextToLayout is nil, layout is complete and we
	// will draw to the end of the display list.
	while (child != fLastLayedOut->Next())
	{
		Rectangle	lineVBounds = GetLineBounds(child);
		
		if ((visibleBounds.bottom - lineVBounds.top) > kMinimumVisibleLineSize)
		{
			do
			{
				child->Draw(document, fidoCompatibility);

				child = (Displayable*)child->Next();
			} while (child != fLastLayedOut->Next() && !child->IsLineBreak());
		}
		else
			break;
	}
	
	// Draw any floaters that are clipped at the top of screen
	for (long i = 0; i < fFloatingList.GetCount(); i++)
	{
		Displayable* item = (Displayable*)fFloatingList.At(i);
		
		if (item->GetTop() > visibleBounds.top)
			break;
			
		Rectangle	itemBounds;
		item->GetBounds(&itemBounds);
		if (item->IsLayoutComplete())
			item->Draw(document, fidoCompatibility);
	}
}
#endif

// ===========================================================================

SideBar::SideBar()
{
}

SideBar::~SideBar()
{
}

Boolean
SideBar::IsLayoutComplete() const
{
	return fLayoutComplete;
}

void
SideBar::LayoutComplete(Document* document, Displayable* parent)
{
	Page::LayoutComplete(document, parent);

	fLayoutComplete = true;
}

void
SideBar::ResetLayout(Document* document)
{
	Page::ResetLayout(document);

	fLayoutComplete = false;
}

void SideBar::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{
		case A_ALIGN:	if (value > 0) fPageAlign = (AttributeValue)value;	break;
		case A_VALIGN:	if (value > 0) fVAlign = (AttributeValue)value;		break;
		case A_WIDTH:
			if (value > 0) {
				long maxWidth = gPageViewer->GetWidth();	// еее DRA Assumes page view!
				if (isPercentage)
					value = maxWidth * value / 100;		
				fWidth = MIN(value, maxWidth);
			}																break;
		default:															break;
	}
}

// ===========================================================================
// Margin defines left margin position
// It is used to implement lists and defs

Margin::Margin()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberMargin;
#endif /* DEBUG_CLASSNUMBER */
}

void Margin::Draw(const Document*, const Rectangle* UNUSED(invalid))
{
}

short Margin::GetBreakType() const
{
	return LineBreak::kHardAlign;
}

void Margin::Layout(Document*, Displayable* parent)	
{
	if (parent)
		((Page *)parent)->SetLeftMarginDefault(fMargin * kMarginWidth);
}

void Margin::SetMargin(short margin)
{
	fMargin = margin;
}

// ===========================================================================
// DefinitionMargin defines left margin position for dictionary definitions

short DefinitionMargin::GetBreakType() const
{
#if 0 
	// еее Dave - This doesn't work when there are multiple DDs for a DT
	short width = 0;
	Displayable* item;
	
	for (item = (Displayable*)Previous(); item; item = (Displayable*)item->Previous()) {
		if (item->IsLineBreak())
			break;
		width += item->GetWidth();
	}
	
	// Don't line break if DT was narrow.
	if (width && width < kMarginWidth)
		return 0;						
#endif
	return LineBreak::kHardAlign;
}

// ===========================================================================
// Divider Draws horizontal rules, gets its horizontal alignment from alignment displayables

static const long kMinDividerWidth = 10;
static const long kDividerVSpace = 3;

Divider::Divider()
{
	fHAlign = AV_CENTER;
	
	fHeight = (kDividerVSpace + gRuleBorder->InnerBounds().top) * 2;
	
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberDivider;
#endif /* DEBUG_CLASSNUMBER */
}

void Divider::Draw(const Document* document, const Rectangle* invalid)
{
	Rectangle bounds;
	GetBounds(&bounds);

	document->GetView()->ContentToScreen(&bounds);

	if (fDividerType == kNoShade) {
		::InsetRectangle(bounds, 0, kDividerVSpace + gRuleBorder->InnerBounds().top - 1);
		PaintRectangle(gScreenDevice, bounds, kBlackColor, 0, invalid);
	}
	else {
		::InsetRectangle(bounds, 0, kDividerVSpace);
		if (fInvert && gRuleBorder->GetFrameCount() > 1)
			gRuleBorder->SetFrame(1);
		else
			gRuleBorder->SetFrame(0);
		gRuleBorder->Draw(&bounds, invalid);
	}
}

AttributeValue
Divider::GetAlign() const
{
	return AV_MIDDLE;
}

AttributeValue
Divider::GetHAlign() const
{
	return fHAlign;
}

short Divider::GetBreakType() const
{
	return LineBreak::kHardAlign;
}

short Divider::GetMinUsedWidth(const Document*) const
{
	if (fKnownWidth > 0 && !fPercentageWidth)
		return fWidth;
		
	return kMinDividerWidth;
}

Boolean Divider::IsDivider() const
{
	return true;
}

void Divider::Layout(Document*, Displayable* parent)
{
	if (fPercentageWidth)
		fWidth = (fPercentageWidth * ((Page*)parent)->GetMarginWidth())/100;
	else 
	{		
		fWidth = ((Page*)parent)->GetMarginWidth();
		if (fKnownWidth > 0)
			fWidth = MIN(fKnownWidth, fWidth);
	}
}

void Divider::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{				
		case A_SIZE:	if (value >= 0)
							fHeight = MAX(value - 2, 0) + (kDividerVSpace + gRuleBorder->InnerBounds().top) * 2;
						break;
	
		case A_ALIGN:	if (value > 0) fHAlign = (AttributeValue)value; 	break;
		case A_INVERTBORDER:	fInvert = true;								break;
		case A_NOSHADE:	fDividerType = Divider::kNoShade; 					break;
		case A_WIDTH:
			if (value >= 0)
				if (isPercentage)
					fPercentageWidth = MIN(value, 100);
				else
					fKnownWidth = value;
			break;
		default:															break;
	}
}

// ===========================================================================
// Line breaks are fairly simple

LineBreak::LineBreak()
{
	fBreakType = kHard;	// Hard break by default

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberLineBreak;
#endif /* DEBUG_CLASSNUMBER */
}

Displayable* LineBreak::DeleteIfSoftBreak(Displayable* parent)
{
	// Delete a soft break and merge two text objects into one
	
	if (fBreakType != kSoft)
		return this;
		
	Text* firstHalf = (Text*)Previous();
	Text* secondHalf = (Text*)Next();
	firstHalf->RemoveSoftBreak(secondHalf);
	
	parent->RemoveChild(secondHalf);
	parent->RemoveChild(this);
	
	delete(secondHalf);
	delete(this);
	return firstHalf;
}

short LineBreak::GetBreakType() const
{
	return fBreakType;
}

void LineBreak::SetBreakType(BreakType breakType)	// Should only exist inside text
{
	fBreakType = breakType;
}

// ===========================================================================
// Alignment defines alignment of subsequent displayables
// It has a parent so it can set alignment of its parent when it lays out

Alignment::Alignment()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberAlignment;
#endif /* DEBUG_CLASSNUMBER */
}

void Alignment::Layout(Document*, Displayable* parent)
{
	if (parent)
		parent->SetAlign(fAlign);
}

void Alignment::SetAlign(AttributeValue align)
{
	fAlign = align;
}

short Alignment::GetBreakType() const
{
	return LineBreak::kHardAlign;
}

// ===========================================================================

