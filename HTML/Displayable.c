// ===========================================================================
//	Displayable.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __DISPLAYABLE_H__
#include "Displayable.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif
#ifndef __REGION_H__
#include "Region.h"
#endif

#ifdef DEBUG_DISPLAYABLEWINDOW
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#endif

// =============================================================================
// HasHTMLAttributes base class

HasHTMLAttributes::HasHTMLAttributes()
{
}

HasHTMLAttributes::~HasHTMLAttributes()
{
}

void HasHTMLAttributes::SetAttribute(Attribute, long, Boolean)
{
}

void HasHTMLAttributes::SetAttributeStr(Attribute, const char*)
{
}

// =============================================================================
// Displayable base class

Displayable::Displayable()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberDisplayable;
#endif /* DEBUG_CLASSNUMBER */
}

Displayable::~Displayable()
{
}

void Displayable::AddChild(Displayable* child)
{
	if (!IsError(child == nil))
		child->SetParent(this);
}

void Displayable::AddChildAfter(Displayable* child, Displayable*)
{
	if (!IsError(child == nil))
		child->SetParent(this);
}

long Displayable::AlignScrollPoint(long newScroll, long) const
{
	return newScroll;
}

Boolean Displayable::BumpSelectionPosition(Coordinate*, const Coordinate*,
										   ContentView*)
{
	return false;
}

void Displayable::Commit(long UNUSED(value))
{
	// Once a selection has been made, commit to that value
}

#ifdef DEBUG_DRAWBOUNDS
void Displayable::DebugDrawBounds(const Document* document, Color color) const
{
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
	::FrameRectangle(gScreenDevice, r, 1, color, 0x80);
}
#endif /* DEBUG_DRAWBOUNDS */

#ifdef DEBUG_DISPLAYABLEWINDOW
void Displayable::DebugPrintInfo(const Document* UNUSED(document), Rectangle* screenR, Rect* clipRect, Boolean UNUSED(showDetails)) const
{
	MacPoint pt;
	GetPen(&pt);
	
	if ((pt.v + 12 > clipRect->top) && (pt.v - 12 < clipRect->bottom))
	{
#ifdef DEBUG_CLASSNUMBER
		const char* className = DebugClassNumberToName(fClassNumber);
		TextFace(bold);
		DrawText(className, 0, strlen(className));
		TextFace(0);
#endif		
	
		long height	= GetHeight();
		long width	= GetWidth();

		if (height || width)
		{
			long top	= GetTop();
			long left	= GetLeft();
			Rectangle bounds;
			
			GetBounds(&bounds);
			MoveTo(130, pt.v);
			if (RectanglesIntersect(&bounds, screenR))
				if (top < screenR->top || (top + height) > screenR->bottom)
					SetColor(0, 255, 0);
				else
					SetColor(0, 0, 255);
			else
				SetColor(0, 0, 0);
			
			char buf[(5*1) + (11*4) + 1]; /* 5 for "{,,,}", 11 per %3ld, 1 for NULL */
			DrawText(buf, 0, snprintf(buf, sizeof(buf), "{%3ld,%3ld,%3ld,%3ld}",
														top, left, height, width));
			SetColor(0, 0, 0);
		}
	}
	MoveTo(pt.h, pt.v + 12);
}
#endif /* DEBUG_DISPLAYABLEWINDOW */
Boolean Displayable::DownInput()
{
	return false;
}

void Displayable::Deselect()
{
}

#ifdef FIDO_INTERCEPT
void Displayable::Draw(const Document* , FidoCompatibilityState& ) const
{
}
#endif

#ifdef DEBUG_DRAWBOUNDS
void Displayable::Draw(const Document* document, const Rectangle* invalid)
{
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
	::PaintBevel(gScreenDevice, r, invalid);
}
#else /* of #if DEBUG_DRAWBOUNDS */
void Displayable::Draw(const Document*, const Rectangle*)
{
}
#endif /* of #else of #if DEBUG_DRAWBOUNDS */

Boolean Displayable::ExecuteInput()
{
	return false;
}

AttributeValue Displayable::GetAlign() const
{
	return AV_BASELINE;
}

void Displayable::GetBounds(Rectangle* r) const
{
	// Bounds is a read only concept, undefined until layout
	
	long	top = GetTop();
	long	left = GetLeft();
	SetRectangle(*r, left, top, left + GetWidth(), top + GetHeight());
}

void Displayable::GetBoundsTight(Rectangle* r) const
{
	GetBounds(r);
}

short Displayable::GetBreakType() const
{
	return 0;
}

Displayable* Displayable::GetFirstChild() const
{
	return nil;
}

void Displayable::GetFirstRegionBounds(Rectangle* bounds) const
{
	GetBounds(bounds);
}

long Displayable::GetHeight() const
{
	return 0;
}

const char* Displayable::GetID() const
{
	return nil;
}

ImageMap* Displayable::GetImageMap() const
{
	Image*	image = GetMappedImage();
	return image == nil ? nil : image->GetImageMap();
}

Displayable* Displayable::GetLastChild() const
{
	return nil;
}

long Displayable::GetLeft() const
{
	return 0;
}

Image* Displayable::GetMappedImage() const
{
	return nil;
}

Displayable* Displayable::GetParent() const
{
	return nil;
}

void Displayable::GetSelectionRegion(Region* region) const
{
	if (!IsError(region == nil))
		region->Reset();
}

long Displayable::GetTop() const
{
	return 0;
}

long Displayable::GetWidth() const
{
	return 0;
}

long Displayable::GetLayoutHeight() const
{
	return GetHeight();
}

long Displayable::GetLayoutWidth() const
{
	return GetWidth();
}

short Displayable::GetMinUsedWidth(const Document*) const
{
	return GetWidth();
}

Boolean Displayable::HasURL() const
{
	return false;
}

Boolean Displayable::IsAnchor() const
{
	return false;
}

Boolean Displayable::IsAnchorEnd() const
{
	return false;
}

Boolean Displayable::IsCell() const
{
	return false;
}

Boolean	Displayable::IsDivider() const
{
	return false;
}

Boolean	Displayable::IsExplicitLineBreak() const
{
	return GetBreakType() != 0 && GetBreakType() != LineBreak::kHardAlign;
}

Boolean	Displayable::IsFloating() const
{
	return false;
}

Boolean	Displayable::IsHighlightable() const
{
	return IsSelectable();
}

Boolean	Displayable::IsImage() const
{
	return false;
}

Boolean	Displayable::IsImageMapSelectable() const
{
	return false;
}

Boolean	Displayable::IsLayoutComplete() const
{
	return false;  // DRA - Currently this is only called for floaters. If we use this 
				   // generally, we need to return a correct status.
}

Boolean	Displayable::IsLineBreak() const
{
	return GetBreakType() != 0;
}

Boolean Displayable::IsSelectable() const
{
	return false;
}

Boolean Displayable::IsInitiallySelected() const
{
	return false;
}

Boolean Displayable::IsSeparable(const Document*) const
{
	// Can this be separated from previous in layout?
	return true;
}

Boolean Displayable::IsSubmit() const
{
	return false;
}

Boolean Displayable::IsTable() const
{
	return false;
}

Boolean Displayable::IsText() const
{
	return false;
}

Boolean Displayable::IsTextField() const
{
	return false;
}

Boolean Displayable::KeyboardInput(Input*)
{
	return false;
}

Boolean Displayable::MoveCursorDown()
{
	return false;
}

Boolean Displayable::MoveCursorLeft()
{
	return false;
}

Boolean Displayable::MoveCursorRight()
{
	return false;
}

Boolean Displayable::MoveCursorUp()
{
	return false;
}

Boolean Displayable::ReadyForLayout(const Document*)
{
	return true;
}

void Displayable::Layout(Document*, Displayable*)
{
	// Determine the dimensions of this and all its childern. Position for drawing
}

void Displayable::LayoutComplete(Document*, Displayable*)
{
	// After we have been positioned	
}

char* Displayable::NewURL(const Coordinate*, const char*) const
{
	return nil;
}

void Displayable::RemoveChild(Displayable* child)
{
	if (!IsError(child == nil))
		child->SetParent(nil);
}

void Displayable::ResetLayout(Document*)
{
	// After we have been positioned	
}

void Displayable::Select()
{
}

void Displayable::SelectFirst()
{
}

void Displayable::SetTop(long UNUSED(top))
{
}

void Displayable::SetLeft(long UNUSED(left))
{
}

void Displayable::SetWidth(long UNUSED(width))
{
}

void Displayable::SetHeight(long UNUSED(height))
{
}

void Displayable::SetAlign(AttributeValue UNUSED(align))
{
}

void Displayable::SetParent(Displayable* UNUSED(parent))
{
}

Boolean Displayable::UpInput()
{
	return false;
}


// =============================================================================
// SpatialDisplayable has instance data for top, left, width, height

SpatialDisplayable::SpatialDisplayable()
{
}

SpatialDisplayable::~SpatialDisplayable()
{
}

long SpatialDisplayable::GetHeight() const
{
	return fHeight;
}

long SpatialDisplayable::GetLeft() const
{
	return fLeft;
}

long SpatialDisplayable::GetTop() const
{
	return fTop;
}

long SpatialDisplayable::GetWidth() const
{
	return fWidth;
}

void SpatialDisplayable::SetHeight(long height)
{
	fHeight = height;
}

void SpatialDisplayable::SetLeft(long left)
{
	fLeft = left;
}

void SpatialDisplayable::SetTop(long top)
{
	fTop = top;
}

void SpatialDisplayable::SetWidth(long width)
{
	fWidth = width;
}

// =============================================================================
// CompositeDisplayable can contain others

CompositeDisplayable::CompositeDisplayable()
{
	fAlign = AV_BASELINE;
}

CompositeDisplayable::~CompositeDisplayable()
{
	fChildren.DeleteAll();
}

void CompositeDisplayable::AddChild(Displayable* child)
{
	// Add a child to the end of the list
	
	fChildren.Add(child);
	Displayable::AddChild(child);
}

void CompositeDisplayable::AddChildAfter(Displayable* child, Displayable* after)
{
	// Add a child to the end of the list
	
	fChildren.AddAfter(child, after);
	Displayable::AddChildAfter(child, after);
}

#ifdef DEBUG_DISPLAYABLEWINDOW
void CompositeDisplayable::DebugPrintInfo(const Document* document, Rectangle* screenR, Rect* clipRect, Boolean showDetails) const
{
	Displayable* child;
	MacPoint pt;
	
	Displayable::DebugPrintInfo(document, screenR, clipRect, showDetails);
	GetPen(&pt);
	
	Move(20, 0);
	for (child = GetFirstChild(); child /*&& (pt.v < clipRect->bottom)*/; child = (Displayable*)child->Next())
	{
		child->DebugPrintInfo(document, screenR, clipRect, showDetails);
		GetPen(&pt);
	}
	Move(-20, 0);
}
#endif /* DEBUG_DISPLAYABLEWINDOW */

#ifdef FIDO_INTERCEPT
void CompositeDisplayable::Draw(const Document* document, FidoCompatibilityState& fidoCompatibility) const
{
	Displayable::Draw(document, fidoCompatibility);
	Displayable* 	child;
	for (child = GetFirstChild(); child != nil; child = (Displayable*)child->Next())
		child->Draw(document, fidoCompatibility);
}
#endif

void CompositeDisplayable::Draw(const Document* document, const Rectangle* invalid)
{
	Displayable* 	child;

	for (child = GetFirstChild(); child != nil; child = (Displayable*)child->Next())
	{
		Rectangle 	bounds;
		child->GetBounds(&bounds);
		document->GetView()->ContentToScreen(&bounds);
		
		if (invalid == nil || RectanglesIntersect(&bounds, invalid))
			child->Draw(document, invalid);

#ifdef DEBUG_DRAWBOUNDS
		child->DebugDrawBounds(document, 0x0000ff);
#endif /* DEBUG_DRAWBOUNDS */
	}
#ifdef DEBUG_DRAWBOUNDS
	DebugDrawBounds(document, 0x00ff00);
#endif /* DEBUG_DRAWBOUNDS */
}

AttributeValue
CompositeDisplayable::GetAlign() const
{
	return fAlign;
}

Displayable* CompositeDisplayable::GetFirstChild() const
{
	return (Displayable*)fChildren.First();
}

Displayable* CompositeDisplayable::GetLastChild() const
{
	return (Displayable*)fChildren.Last();
}

Boolean CompositeDisplayable::ReadyForLayout(const Document* document)
{
	Boolean	ready = true;
	
	for (Displayable* child = GetFirstChild(); child && ready; child = (Displayable*)child->Next())
		ready = child->ReadyForLayout(document);
	
	return ready;
}

void CompositeDisplayable::Layout(Document* document, Displayable* parent)
{
	Displayable* child;

	for (child = GetFirstChild(); child != nil; child = (Displayable*)child->Next())
		child->Layout(document, parent);
}

void CompositeDisplayable::LayoutComplete(Document* document, Displayable* parent)
{
	Displayable* child;

	for (child = GetFirstChild(); child != nil; child = (Displayable*)child->Next())
		if (!child->IsLayoutComplete())
			child->LayoutComplete(document, parent);
}

void CompositeDisplayable::RemoveChild(Displayable* child)
{
	// Add a child to the end of the list
	
	fChildren.Remove(child);
	Displayable::RemoveChild(child);
}

void CompositeDisplayable::ResetLayout(Document* document)
{
	Displayable* child;

	for (child = GetFirstChild(); child != nil; child = (Displayable*)child->Next())
		child->ResetLayout(document);
}

void CompositeDisplayable::SetAlign(AttributeValue align)
{
	// Don't pass align to its offspring, use it for layout
	
	fAlign = align;
}

void CompositeDisplayable::SetLeft(long left)
{
	// Setting top or left adjusts the position of children
	
	long	offset = left - fLeft;	// Offset all children this much
	Displayable*	child;
	
	for (child = GetFirstChild(); child != nil; child = (Displayable *)child->Next())
		child->SetLeft(child->GetLeft() + offset);
	fLeft += offset;
}

void CompositeDisplayable::SetTop(long top)
{
	// Setting top or left adjusts the position of children
	
	long	offset = top - fTop;	// Offset all children this much
	Displayable*	child;
	
	for (child = GetFirstChild(); child != nil; child = (Displayable *)child->Next())
		child->SetTop(child->GetTop() + offset);
	fTop += offset;
}

// =============================================================================
