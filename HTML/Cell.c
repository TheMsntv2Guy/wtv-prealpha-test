// ===========================================================================
//	Cell.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CELL_H__
#include "Cell.h"
#endif

#ifndef __DISPLAYABLE_H__
#include "Displayable.h"	/* see if DEBUG_DRAWBOUNDS is set */
#endif

// =============================================================================
// Cells are just pages with some additional info about their positioning

Cell::Cell()
{
	fBackgroundColor = (ulong)-1;
	fRowSpan = fColumnSpan = 1;
	fVAlign = AV_MIDDLE;

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberCell;
#endif /* DEBUG_CLASSNUMBER */
}

Cell::~Cell()
{
}

Boolean Cell::IsCell() const
{
	return true;
}

void Cell::Draw(const Document* document, const Rectangle* invalid)
{
	Page::Draw(document, invalid); 			// Draw children
#ifdef DEBUG_DRAWBOUNDS
	DebugDrawBounds(document, 0x00ffff);
#endif /* DEBUG_DRAWBOUNDS */
}

short Cell::GetColumn() const
{
	return fColumn;
}

short Cell::GetColumnSpan() const
{
	return fColumnSpan;
}

long Cell::GetKnownHeight() const
{
	return fKnownHeight;
}

long Cell::GetKnownWidth() const
{
	return fKnownWidth;
}

Boolean Cell::IsKnownWidthAbsolute() const
{
	return fIsAbsolute;
}

Boolean Cell::IsKnownWidthPercentage() const
{
	return fIsPercentage;
}

short Cell::GetRow() const
{
	return fRow;
}

short Cell::GetRowSpan() const
{
	return fRowSpan;
}

void Cell::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{
		case A_BGCOLOR: if (value >= 0) fBackgroundColor = value; 			break;		
		case A_ROWSPAN:	if (value > 0) fRowSpan = value;					break;
		case A_COLSPAN:	if (value > 0) fColumnSpan = value;					break;
		case A_ALIGN:	if (value > 0) fPageAlign = (AttributeValue)value;	break;
		case A_VALIGN:	if (value > 0) fVAlign = (AttributeValue)value;		break;
		case A_NOWRAP:	fNoWrap = true;										break;
		
		case A_HEIGHT:		// еее not currently handling height as percentage.
			if (value > 0) fKnownHeight = value;							break;
		case A_ABSWIDTH:
			fIsAbsolute = true;	// fall through
		case A_WIDTH:		// may be a percentage, must know it was req
			if (value > 0) {
				fKnownWidth = value;
				fIsPercentage = isPercentage;
			}																break;
		default:															break;
	}
}

void Cell::SetColumnSpan(short span)
{
	fColumnSpan = span;
}

void Cell::SetRowSpan(short span)
{
	fRowSpan = span;
}

void Cell::SetRowAndCol(short row, short col)
{
	fRow = row;
	fColumn = col;
}

void Cell::VAlignCell(long height)
{
	// A cell vertically repositions its contents according to VAlign
	
	long offset = 0;
	switch (fVAlign) 
	{		
		case AV_TOP:	offset = 0;							break;
		case AV_MIDDLE:	offset = (height - GetHeight())/2;	break;
		case AV_BOTTOM:	offset = height - GetHeight();		break;
		default:											break;
	}
	Displayable* child;
	for (child = GetFirstChild(); child != nil; child = (Displayable *)child->Next())
		child->SetTop(child->GetTop() + offset);
	SetHeight(height);
}

// =============================================================================

