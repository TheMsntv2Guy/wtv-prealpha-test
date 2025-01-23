// ===========================================================================
//	Image.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif

#if defined FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif

// =============================================================================

const short kDefaultImageSize = 40;

// =============================================================================

Image::Image()
{
	fAlign = AV_BASELINE;
	fDrawEmpty = true;
	fHSpace = -1;
	fImageMapSelection = -1;
	fIsVisible = true;
	fPercentageWidth = -1;

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberImage;
#endif /* DEBUG_CLASSNUMBER */
}

Image::~Image()
{
	if (fALT != nil)
		FreeTaggedMemory(fALT, "Image::fALT");
	
	if (fImageData != nil)
		delete(fImageData);
	
	if (fSRC != nil)
		FreeTaggedMemory(fSRC, "Image::fSRC");
		
	if (fUseMap != nil)
		FreeTaggedMemory(fUseMap, "Image::fUseMap");
}

Color Image::AverageColor(ulong backgroundColor)
{
	if (fImageData == nil)
		return kBlackColor;
	
	return fImageData->AverageColor(backgroundColor);
}

void Image::ConstrainWidth(long marginWidth, long maxWidth)
{
	// If the image can fit the margin width without scaling, target the
	// margin width.
	if (marginWidth >= fWidth)
		maxWidth = marginWidth;
		
	if (fIsBackground || maxWidth == 0)
		return;
		
	// First try to fit full width by reducing hspace and borders.
	long width = GetWidth();	
	while (width > maxWidth && fHSpace > 0) {
		fHSpace--;
		width -= 2;
	}
	while(width > maxWidth && fBorder > 0) {
		fBorder--;
		width -= 2;
	}
	
	// If the tight image doesn't fit, scale it.
	if (fWidth > maxWidth) {
		TrivialMessage(("Scaling image from (%d x %ld) to (%d x %ld) to fit screen",
			fWidth, fHeight, maxWidth, (fHeight * maxWidth + fWidth / 2) / fWidth));

		fHeight = (fHeight * maxWidth + fWidth / 2) / fWidth;
		fWidth = maxWidth;
	}
}

void Image::DrawBorder(const Document* document, const Rectangle* invalid)
{
	// We an inset bevel for the border.
	if (fBorder > 0 && document != nil && !fIsBackground) {
		Rectangle r;
		GetBoundsTight(&r);	
		InsetRectangle(r,-fBorder,-fBorder);
		document->GetView()->ContentToScreen(&r);
				
		r.bottom--;	// Confine bevel inside bounds
		r.right--;
		for (long i = fBorder; i--;) {			
			::PaintAntiBevel(gScreenDevice, r, invalid, 160);
			::InsetRectangle(r, 1, 1);
		}
	}
}

#if defined FIDO_INTERCEPT
void Image::DrawBorder(const Document* document, FidoCompatibilityState& fidoCompatibility) const
{
	if (!KnowSize()) return;

	// We an inset bevel for the border.
	if (fBorder > 0 && document != nil && !fIsBackground)
	{
		Rectangle r;
		GetBoundsTight(&r);	
		InsetRectangle(r,-fBorder,-fBorder);
		document->GetView()->ContentToScreen(&r);
				
		r.bottom--;	// Confine bevel inside bounds
		r.right--;
		for (long i = fBorder; i--;) 
		{			
			fidoCompatibility.PaintAntiBevel(gScreenDevice, r, nil);
			::InsetRectangle(r, 1, 1);
		}
	}
}
#endif

void Image::DrawOutline(const Document* document, const Rectangle* invalid)
{
	// We draw a subtle inset bevel if the image has not yet drawn anything
	if (document != nil && !fIsBackground && fBorder <= 0)
	{
		Rectangle r;
		GetBoundsTight(&r);
		
		// Don't bother for really small images
		if (RectangleWidth(r) < 5 || RectangleHeight(r) < 5)
			return;
		
		document->GetView()->ContentToScreen(&r);
				
		r.bottom--;	// Confine bevel inside bounds
		r.right--;
		for (long i = 0; i < 2; i++) {			
			::PaintAntiBevel(gScreenDevice, r, invalid, 210);
			::InsetRectangle(r, 1, 1);
		}
		
		if (GetStatus() < 0)
			::PaintRectangle(gScreenDevice, r, kBlackColor, 236, invalid);
	}
}

void Image::DrawTiled(const Document* document,Rectangle tileRect,Coordinate origin,const Rectangle* invalid,Boolean tileVertical,Boolean tileHorizontal, ulong transparency)
{
	if (IsError(!KnowSize()))
		return;
	
	if (!fIsVisible)
		return;

	if (document != nil) {
		Rectangle r;
		SetRectangle(r,0,0,0,0);

		if (fIsBackground)
			document->GetView()->BackgroundToScreen(&r);
		else
			document->GetView()->ContentToScreen(&r);
		OffsetRectangle(tileRect,r.left,r.top);
		origin.x += r.left;
		origin.y += r.top;
	}

	if (GetStatus() < 0)
		::PaintRectangle(gScreenDevice, tileRect, kBlackColor, 224, invalid);
	else
		fImageData->DrawTiled(tileRect,origin,invalid,tileVertical,tileHorizontal,transparency);
	fIdleDrawEnabled = true;
}


void Image::Draw(const Document* document, const Rectangle* invalid)
{
	Rectangle r;

	if (IsError(!KnowSize()))
		return;
	
	if (!fIsVisible)
		return;
			
	fIdleDrawEnabled = true;
	
	DrawBorder(document, invalid);
	
	GetBoundsTight(&r);

	if (document != nil) {
		if (fIsBackground)
			document->GetView()->BackgroundToScreen(&r);
		else
			document->GetView()->ContentToScreen(&r);
	}
					
	if (fImageData != nil) {
		fImageData->Draw(&r, invalid, fTransparency);		
		fDrawEmpty = EmptyRectangle(&r);
	}
	else
		fDrawEmpty = true;
	
	if (fDrawEmpty)
		DrawOutline(document, invalid);
}

#if defined FIDO_INTERCEPT
void Image::Draw(const Document* document, FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle r;

	if (IsError(!KnowSize()))
		return;

	if (!fIsVisible)
		return;

	DrawBorder(document, fidoCompatibility);

	GetBoundsTight(&r);

	if (document != nil) {
		if (fIsBackground)
			document->GetView()->BackgroundToScreen(&r);
		else
			document->GetView()->ContentToScreen(&r);
	}
	
	if (GetStatus() < 0)
		fidoCompatibility.PaintRectangle(gScreenDevice, r, kBlackColor, 224, 0);
	else if (fImageData != nil)
		fImageData->Draw(&r, fTransparency, fidoCompatibility);
}
#endif

AttributeValue
Image::GetAlign() const
{
	return fAlign;
}

void Image::GetBoundsTight(Rectangle* r) const
{
	short top = GetTop() + fVSpace + fBorder;
	short left = GetLeft() + (fHSpace > 0 ? fHSpace : 0) + fBorder;
	::SetRectangle(*r, left, top, left + fWidth, top + fHeight);
}

long Image::GetHeight() const
{
	return fHeight + ((fVSpace + fBorder) << 1);
}

Image* Image::GetMappedImage() const
{
	return (Image*)this;
}

short Image::GetMinUsedWidth(const Document*) const
{
	// If width is a percentage, pick a minimum size.
	if (fPercentageWidth != -1)
		return 20;
		
	return GetWidth();
}

Displayable*
Image::GetParent() const
{
	return fParent;
}

uchar Image::GetPercentComplete(ulong dataExpected) const
{
	return fResource.GetPercentComplete(dataExpected);
}

Error Image::GetStatus() const
{
	if (fState == kImageError && fResource.GetStatus() >= 0)
		return kResourceNotFound;
	return fResource.GetStatus();
}

void Image::GetUnscaledBoundsTight(Rectangle* r) const
{
	GetBoundsTight(r);

	// Get the image's original bounds.
	Rectangle	imageBounds;
	if (fImageData != nil && fImageData->GetBounds(&imageBounds)) {
		r->bottom = r->top + imageBounds.bottom - imageBounds.top;
		r->right =  r->left + imageBounds.right - imageBounds.left;
	}
}

const char* Image::GetUseMap() const
{
	return fUseMap;
}

long Image::GetWidth() const
{
	return fWidth + (((fHSpace > 0 ? fHSpace : 0) + fBorder) << 1);
}

void Image::Hide()
{
	fIsVisible = false;
}

Boolean Image::Idle(Layer* layer)
{
	PerfDump perfdump("Image::Idle");

	// Idles the resource and returns true if image discovered that it's size changed

	Boolean		changed = false;
	Rectangle	r;
	
	if ( fImageData && fImageData->GetStatus() == kWrongImageType) {
		delete(fImageData);
		fImageData = ImageData::NewImageData(&fResource);
	}
	Error status = (fImageData != nil ? fImageData->GetStatus() : fResource.GetStatus());

	if (fState < kImageError && TrueError(status) && status != kTruncated)
	{
		ImportantMessage(("Image::Idle - Error status from image data: %d", status));
		if (fImageData != nil) {
			if (!fImageData->IsDrawingComplete())	// If a draw was in progress
				InvalidateBounds(layer);
			if (fCreatedImageData) {
				delete(fImageData);
				fImageData = nil;
			}
		}
		fState = kImageError;
		if (fKnownWidth == 0 || fKnownHeight == 0) {
			fKnownWidth = fKnownHeight = kDefaultImageSize;
			return true;
		}
		return false;
	}
	
	// If the stream has been reset, give the image data a chance to clean up.
	else if (fImageData != nil && status == kStreamReset)
		fImageData->PushData(true);
	
	switch (fState)
	{
		case kCreateImageData:
			if ((fImageData = ImageData::NewImageData(&fResource)) == nil) {
				// If the stream is complete, and we still can't create it, we don't
				// know the type.
				if (fResource.GetStatus() == kComplete) {
					ImportantMessage(("Can't create ImageData on complete stream. "
									  "DataType is not known image type."));
					fState = kImageError;
					
					if (fKnownWidth == 0 || fKnownHeight == 0) {
						fKnownWidth = fKnownHeight = kDefaultImageSize;
						return true;
					}
				}
				break;
			}
			fCreatedImageData = true;
			if ( fIsBackground )	
				fImageData->SetIsBackground(fIsBackground);
			
			if (fNoFilter)
				fImageData->SetFilterType(kNoFilter);

			// If we've already been layed out, because our size was hinted, invalidate now
			// to kick off the first draw.
			if (fLayoutComplete && fIdleDrawEnabled)
				InvalidateBounds(layer);
			fState = kDetermineImageSize;
			
		case kDetermineImageSize:	
			if (KnowSize()) {
				fState = kWaitForImageLayout;
				fSizeKnown = true;	// Set this here, in case height or width is scaled to 0.
			}			
			else if (fImageData->GetBounds(&r))
			{
				if (fKnownWidth != 0)
					fKnownHeight = ((r.bottom - r.top) * fKnownWidth + (r.right - r.left) / 2) / (r.right - r.left);
				else if (fKnownHeight != 0)
					fKnownWidth = ((r.right - r.left) * fKnownHeight + (r.bottom - r.top) / 2) / (r.bottom - r.top);
				else {
					fKnownWidth = r.right - r.left;
					fKnownHeight = r.bottom - r.top;
				}
				fState = kWaitForImageLayout;
				fSizeKnown = true;
				changed = true;
			}
			else
				break;
	
		case kWaitForImageLayout:
			if (!fLayoutComplete)
				break;
			fState = kDrawImage;
				
		case kDrawImage:
			// Continue any drawing in progress
			if (fIsVisible && !fImageData->IsDrawingComplete()) {
				GetBoundsTight(&r);
				
				if (layer->IsContentView()) {
					if (fIsBackground)
						((ContentView*)layer)->BackgroundToScreen(&r);
					else
						((ContentView*)layer)->ContentToScreen(&r);
				}
				
				// DrawingIdle returns actual drawn bounds in r.		
				if (fImageData->DrawingIdle(&r)) {
					layer->InvalidateAbove(&r);
					if (layer->IsContentView())
						((ContentView*)layer)->GetSelectionLayer()->UpdateSavedBits(&r);
					// If this is the first draw, invalidate to remove outline.
					if (fDrawEmpty && !EmptyRectangle(&r))
						InvalidateBounds(layer);
				}
			}
			break;
		case kImageError:
			break;
	}

	return changed;
}

void Image::InvalidateBounds(Layer* layer)
{
	Rectangle r;
	
	GetBoundsTight(&r);

	if (layer->IsContentView()) {	
		if (fIsBackground)
			((ContentView*)layer)->BackgroundToScreen(&r);
		else
			((ContentView*)layer)->ContentToScreen(&r);
	}
	InsetRectangle(r,0,-1);		// increase by one line to fix flicker filter effects
	layer->InvalidateBounds(&r);
}

Boolean Image::IsAnimation() const
{
	return false;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean	Image::IsBackground() const
{
	return fIsBackground;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean Image::IsDrawingComplete() const
{
	if (fImageData)
		return fImageData->IsDrawingComplete();
	else
		return true;
}
#endif

Boolean Image::IsFloating() const
{
	return (fAlign == AV_LEFT || fAlign == AV_RIGHT ||
			fAlign == AV_BLEEDLEFT || fAlign == AV_BLEEDRIGHT);
}

Boolean Image::IsImage() const
{
	return true;
}

Boolean Image::IsLayoutComplete() const
{
	return fLayoutComplete;
}

Boolean	Image::IsMap() const
{
	return fIsMap;
}

void Image::Load(const Resource* parent)
{
	if (fSRC == nil || *fSRC == '\0') {
		fState = kImageError;
		if (fBorder < 1 ) fBorder = 1;	// Force border for missing SRC
		return;
	}
	
	if (parent != nil)
		fResource.SetURL(fSRC, parent);
	else
		fResource.SetURL(fSRC);
		
	FreeTaggedMemory(fSRC, "Image::fSRC");
	fSRC = nil;
}

Boolean Image::KnowSize() const
{
	if (fSizeKnown)
		return true;
	
	if (fKnownWidth != 0 && fKnownHeight != 0)
		return true;
		
	return false;
}

Boolean Image::ReadyForLayout(const Document*)
{
	return KnowSize() && (fImageMap == nil || fImageMap->IsComplete());
}

void Image::ResetLayout(Document*)
{
	fLayoutComplete = false;
}

void Image::PurgeResource()
{
	fResource.Purge();
}

void Image::Layout(Document* document, Displayable* parent)
{
	// Default HSpace for floaters is 3.
	if (IsFloating() && fHSpace < 0)
		fHSpace = 3;
		
	// constrain width to available size of container
	long containerWidth = 0;
	long containerMargin = 0;
	if (fIsBackground)
		containerWidth = containerMargin = document->GetView()->GetWidth();
	else if (parent != nil) {
		containerWidth = ((Page*)parent)->GetDefaultMarginWidth();
		containerMargin = ((Page*)parent)->GetMarginWidth();
	}
	
	fWidth = fKnownWidth;
	fHeight = fKnownHeight;
	
	if (fPercentageWidth > -1) {
		fWidth = (fPercentageWidth * containerWidth + 50) / 100;
		fHeight = (fKnownHeight * fWidth + fKnownWidth / 2) / fKnownWidth;
	}
	
	ConstrainWidth(containerMargin, containerWidth);
}

void Image::LayoutComplete(Document* document, Displayable*)
{
	fLayoutComplete = true;

	// If we have a client image map, add the map selectables.	
	long	areaCount = 0;	
	if (fImageMap != nil && fImageMap->GetType() == kClientMap &&
	    (areaCount = fImageMap->GetAreaCount()) > 0)
	{
		for (long i = 0; i < areaCount; i++)
		{
			ImageMapSelectable*	selectable = new(ImageMapSelectable);
			
			selectable->SetSelectable(this);
			selectable->SetArea(fImageMap->AreaAt(i));
			
			if (i == fImageMapSelection)
				selectable->SetInitiallySelected(true);
				
			document->AddSelectable(selectable);
		}
	}
}

void Image::Show()
{
	fIsVisible = true;
}

void Image::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{		
		case A_ISMAP:			fIsMap = true;								break;
		case A_SELECTED:		fImageMapSelection = MAX(value, 0);			break;
		case A_WIDTH:
				if (isPercentage)
					fPercentageWidth = MIN(MAX(value, 0), 100);
				else
					fKnownWidth = MAX(value, 0);				
				break;
		case A_HEIGHT:			fKnownHeight = MAX(value, 0);				break;
		case A_BORDER:			fBorder = (value == -1 ? 1 : MAX(value,0));	break;
		case A_ALIGN:			if (value > 0) fAlign = (AttributeValue)value;	break;
		case A_HSPACE:			fHSpace = MAX(value, 0);					break;
		case A_VSPACE:			fVSpace = MAX(value, 0);					break;
		case A_NOFILTER:		fNoFilter = true;							break;
		case A_TRANSPARENCY:
				if (value < 0)
				{
					ImportantMessage(("Pinning transparency value of %ld to 0", value));
					value = 0;
				}
				else if (value > 100)
				{
					ImportantMessage(("Pinning transparency value of %ld to 100", value));
					value = 100;
				}
				Message(("Setting transparency to %ld%%", value));
				fTransparency = (ushort)(value*255/100);
				break;					
		default:				Displayable::SetAttribute(attributeID, value, isPercentage);
	}
}

void Image::SetAttributeStr(Attribute attributeID, const char* value)
{	
	switch (attributeID) 
	{		
		case A_SRC:
			if (fSRC != nil)
				break;	// already have one
			TrivialMessage(("Setting %x->SRC = '%s'", (ulong)this, value));
			fSRC = CopyStringTo(fSRC, value, "Image::fSRC");
			break;
		case A_ALT:	
			fALT = CopyStringTo(fALT, value, "Image::fALT");
			break;
		case A_USEMAP:
			fUseMap = CopyStringTo(fUseMap, value, "Image::fUseMap");
			break;
		default:
			Displayable::SetAttributeStr(attributeID, value);
	}
}

void Image::SetIsBackground(Boolean background)
{	
	fIsBackground = background;
	if ( fImageData )	
		fImageData->SetIsBackground(background);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void Image::SetKeepBitMap(Boolean keep)
{
#ifdef FIDO_INTERCEPT
	keep = true;
#endif
	if (fImageData != nil)
		fImageData->SetKeepBitMap(keep ? kNormalBitMap : kNoBitMap);
}
#endif

void Image::SetParent(Displayable* parent)
{
	fParent = parent;
}

// =============================================================================

