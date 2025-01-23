// ===========================================================================
//	Anchor.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __ANCHOR_H__
#include "Anchor.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __REGION_H__
#include "Region.h"
#endif
#ifndef __TEXT_H__
#include "Text.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif



// ===========================================================================
//	implementation
// ===========================================================================

AnchorEnd::~AnchorEnd()
{
}

AnchorEnd::AnchorEnd()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberAnchorEnd;
#endif /* DEBUG_CLASSNUMBER */
}

Anchor* AnchorEnd::FindAnchorStart() const
{
	Displayable* item;
	
	// Find the anchor.
	for (item = (Displayable*)Previous(); item != nil && !item->IsAnchor(); 
		 item = (Displayable*)item->Previous()) {
		// If anchor includes a floating image that has not layed out, don't add
		// the selectable now. We'll add it when image is layed out.
		if (item->IsImage()) {
			Image*	image = (Image*)item;
			if (!image->IsLayoutComplete()) {
				return nil;
			}
		}
	}

	return item != nil ? (Anchor*)item : nil;
}

Boolean AnchorEnd::IsAnchorEnd() const
{
	return true;
}

Boolean AnchorEnd::IsLayoutComplete() const
{
	return fLayoutComplete;
}

void AnchorEnd::ResetLayout(Document*)
{
	fLayoutComplete = false;
}

void AnchorEnd::LayoutComplete(Document* document, Displayable*)
{
	fLayoutComplete = true;

	Anchor*	anchor = FindAnchorStart();
	if (anchor == nil)
		return;

	// If the anchor has an server image map, add the map selectables, otherwise just
	// add the anchor.	
	ImageMap*	map = anchor->GetImageMap();
	if (map != nil && map->GetType() == kServerMap)
	{
		// Add areas for the image
		long	areaCount = map->GetAreaCount();	
		if (areaCount > 0)
		{
			for (long i = 0; i < areaCount; i++)
			{
				ImageMapSelectable*	selectable = new(ImageMapSelectable);
				selectable->SetSelectable(anchor);
				selectable->SetArea(map->AreaAt(i));
				document->AddSelectable(selectable);
			}
		}
		else
		{
			ImageMapSelectable*	selectable = new(ImageMapSelectable);
			selectable->SetSelectable(anchor);
			document->AddSelectable(selectable);
		}
	}
	else
		document->AddSelectable(anchor);
}

// =============================================================================

Anchor::Anchor()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberAnchor;
#endif /* DEBUG_CLASSNUMBER */
}

Anchor::~Anchor()
{
	if (fHREF != nil)
		FreeTaggedMemory(fHREF, "Anchor::fHREF");
	
	if (fName != nil)
		FreeTaggedMemory(fName, "Anchor::fName");
}

AnchorEnd* Anchor::FindAnchorEnd() const
{
	Displayable* item;
	
	// Find the anchor.
	for (item = (Displayable*)Next(); item != nil && !item->IsAnchorEnd(); 
		 item = (Displayable*)item->Next())
		;

	return item != nil ? (AnchorEnd*)item : nil;
}

AttributeValue Anchor::GetAlign() const
{
	return AV_TOP;
}

void Anchor::GetBounds(Rectangle* r) const
{
	const Displayable*	item;
	Rectangle 			accumulator = {0,0,0,0};

	*r = accumulator;	// Start out empty.
	
	if (!IsLayoutComplete())
		return;
	
	// Iterating here is more efficient than creating the region.
	for (item = (Displayable*)Next(); item != nil && !item->IsAnchorEnd(); 
		 item = (Displayable*)item->Next())
	{
		Rectangle	itemBounds;
		item->GetBounds(&itemBounds);
		if (!item->IsDivider() && !EmptyRectangle(&itemBounds))
			UnionRectangle(&accumulator, &itemBounds);
	}
	
	if (item && ((AnchorEnd*)item)->IsLayoutComplete())
		*r = accumulator;
}

void Anchor::GetFirstRegionBounds(Rectangle* r) const
{
	const Displayable*	item;
	r->top = r->bottom = r->left = r->right = 0;	// Start out empty.
	
	if (!IsLayoutComplete())
		return;
	
	// Iterating here is more efficient than creating the region.
	for (item = (Displayable*)Next(); item != nil && !item->IsAnchorEnd();
		 item = (Displayable*)item->Next())
	{
		Rectangle	itemBounds;
		item->GetBounds(&itemBounds);
		if (!item->IsDivider() && !EmptyRectangle(&itemBounds)) {
			*r = itemBounds;
			break;
		}
	}
}

const char* Anchor::GetID() const
{
	return fName;
}

Image* Anchor::GetMappedImage() const
{
	return fImage;
}

Displayable* Anchor::GetParent() const
{
	return fParent;
}

void Anchor::GetSelectionRegion(Region* region) const
{
	const Displayable* item = this;
	Rectangle accumulator = {0,0,0,0};
	Rectangle r;
	Boolean useBounds = false;

	if (IsError(region == nil))
		return;
		
	region->Reset();
		
	if (!IsLayoutComplete())
		return;
	
	// accumulate all rects on a given line into one
	
	while ((item = (Displayable*)item->Next()) != nil && !item->IsAnchorEnd())
	{
		// Use the entire bounds for images, since multi-line selection drawing
		// currently looks bad for thumbnails (e.g. Recent).
		PostulateFinal(false);
		if (item->IsImage())
			useBounds = true;
			
		item->GetBoundsTight(&r);
		
		if (item->IsLineBreak())
		{
			if (accumulator.top < accumulator.bottom && accumulator.left < accumulator.right)
				region->Add(&accumulator);
			accumulator.top = accumulator.bottom = accumulator.left = accumulator.right = 0;
		}
		else if (!item->IsDivider() && !EmptyRectangle(&r))
			UnionRectangle(&accumulator, &r);
	}
	
	// If we never saw an AnchorEnd, document build is still in progress. Return empty region.
	if (item == nil)
		region->Reset();
	
	else if (accumulator.top < accumulator.bottom && accumulator.left < accumulator.right)
		region->Add(&accumulator);
	
	if (useBounds && !region->IsEmpty())
	{
		region->GetBounds(&r);
		region->Reset();
		region->Add(&r);
	}
}

char* Anchor::NewURL(const Coordinate*, const char* USED_FOR_MEMORY_TRACKING(tag)) const
{
	return CopyString(fHREF, tag);
}

Boolean Anchor::HasURL() const
{
	return fHREF != nil;
}

Boolean Anchor::IsHighlightable() const
{
	return (fNoHighlight ? false : IsSelectable());
}

Boolean Anchor::IsAnchor() const
{
	return true;
}

Boolean Anchor::IsLayoutComplete() const
{
	AnchorEnd* end = FindAnchorEnd();
	
	return end != nil && end->IsLayoutComplete();
}

Boolean Anchor::IsSelectable() const
{
	// If we have a URL and no ImageMap, we are selectable
	return (HasURL() && GetImageMap() == nil);	
}

Boolean Anchor::IsInitiallySelected() const
{
	return fIsInitiallySelected;
}

void Anchor::ResetHREF()
{
	if (fHREF != nil) {
		FreeTaggedMemory(fHREF, "Anchor::fHREF");
		fHREF = nil;
	}
}

void Anchor::SetAttribute(Attribute attributeID, long, Boolean)
{
	switch (attributeID) {
		case A_SELECTED:	fIsInitiallySelected = true;	break;
		case A_NOHIGHLIGHT:	fNoHighlight = true;			break;
		default:			break;
	}
}

void Anchor::SetAttributeStr(Attribute attributeID, const char* value)
{
	switch (attributeID) 
	{		
		case A_HREF:	if (value != nil && *value != '\0')
							fHREF = CopyStringTo(fHREF, value, "Anchor::fHREF"); break;
		case A_ID:
		case A_NAME:	fName = CopyStringTo(fName, value, "Anchor::fName"); break;
		default:		break;
	}
}

void Anchor::SetMappedImage(Image* image)
{
	fImage = image;
}

void Anchor::SetParent(Displayable* parent)
{
	fParent = parent;
}

void Anchor::SetVisited()
{
	Displayable* item;
	
	// Set the visited bit on any text items in this anchor
	for (item = (Displayable*)Next(); item != nil && !item->IsAnchorEnd(); 
		 item = (Displayable*)item->Next()) {
		 if (item->IsText()) {
		 	Text*	text = (Text*)item;
		 	PackedStyle style = text->GetStyle();
		 	if (style.anchor) {
		 		style.visited = true;
		 		text->SetStyle(style);
			}
		}
	}
}

// =============================================================================

