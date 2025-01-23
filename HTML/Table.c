// ===========================================================================
//	Table.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __DISPLAYABLE_H__
#include "Displayable.h"	/* see if DEBUG_DRAWBOUNDS is set */
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __TABLE_H__
#include "Table.h"
#endif




// ===========================================================================
//	local constants/structs
// ===========================================================================
const long kMinColumnWidth = 1;
const long kFloatingHSpace = 3;

typedef struct PageWidth PageWidth;

struct PageWidth {
	short	min;
	short	max;
};




// ===========================================================================
//	implementation
// ===========================================================================
Table::Table()
{
	fBackgroundColor = (Color)-1;
	fRowBackgroundColor = (Color)-1;
	fCellPadding = 1;	// Netscapeª default
	fCellSpacing = 2;	// Netscapeª default
	fVAlign = AV_MIDDLE;
	fRowAlign = AV_LEFT;

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberTable;
#endif /* DEBUG_CLASSNUMBER */
}

Table::~Table()
{
}

void Table::AddChild(Displayable* child)
{
	// The only children allowed in tables are cells.
	if (IsError(!child->IsCell())) {
		delete(child);
		return;
	}
		
	CompositeDisplayable::AddChild(child);
}

long Table::AlignScrollPoint(long newScroll, long oldScroll) const
{
	Cell*	cell;
	
	// Handle down
	if (newScroll > oldScroll)
	{
		for (cell = (Cell*)GetFirstChild(); cell != nil && cell->GetTop() <= newScroll; cell = (Cell*)cell->Next())
		;
		
		if (cell != nil && cell->Previous() != nil && ((Cell*)cell->Previous())->GetTop() > oldScroll)
			return ((Cell*)cell->Previous())->GetTop();
	}
	
	// Handle up
	else
	{
		for (cell = (Cell*)GetFirstChild(); cell != nil && cell->GetTop() < newScroll; cell = (Cell*)cell->Next())
		;
		
		if (cell != nil && cell->GetTop() < oldScroll)
			return cell->GetTop();
	}
		
	return CompositeDisplayable::AlignScrollPoint(newScroll, oldScroll);
}

void Table::Close()
{
	CloseRow();
	fOpen = false;
}

void Table::CloseCaption()
{
}

void Table::CloseCell()
{
	fInCell = 0;
}

void Table::CloseRow()
{
	CloseCell();
	if (fInRow)
	{
		fInRow = 0;
		fRows++;
	}
}

void Table::Draw(const Document* document, const Rectangle* invalid)
{
	// Draw the border of the table
	// Draw all the children, draw a border around them if required
	
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);

	if (fAlign == AV_LEFT)
		r.right -= kFloatingHSpace;
	else if (fAlign == AV_RIGHT)
		r.left += kFloatingHSpace;
		
	Color oldBackColor = gPageBackColor;

	if (fBackgroundColor != (ulong)-1) {
		gPageBackColor = fBackgroundColor;
		::PaintRectangle(gScreenDevice, r, fBackgroundColor, 0, invalid);
	}
		
	if (fBorder) {
		if (fCaption) {
			if (fCaptionAlign == AV_BOTTOM)
				r.bottom -= fCaption->GetHeight();
			else
				r.top += fCaption->GetHeight();
		}
		r.bottom--;	// Confine bevel inside bounds
		r.right--;
		for (long i = fBorder; i--;) {
			if (fInvertBorder)
				::PaintAntiBevel(gScreenDevice, r, invalid);
			else
				::PaintBevel(gScreenDevice, r, invalid);
			::InsetRectangle(r, 1, 1);
		}			
	}

	Cell* c;
	for (c = (Cell*)GetFirstChild(); c != nil; c = (Cell*)c->Next()) {
		Color oldCellBackColor = gPageBackColor;

		c->GetBounds(&r);
		document->GetView()->ContentToScreen(&r);		

		// Draw background
		::InsetRectangle(r, -fCellPadding, -fCellPadding);
		if (c->GetBackgroundColor() != (ulong)-1 && c->GetFirstChild() && c != fCaption &&
		    RectanglesIntersect(&r, invalid)) {
			gPageBackColor = c->GetBackgroundColor();
			::PaintRectangle(gScreenDevice, r,  c->GetBackgroundColor(), 0, invalid);
		}
				
		::InsetRectangle(r, -fCellBorder, -fCellBorder);
		
		if (RectanglesIntersect(&r, invalid)) {
			// Draw border. Don't border empty cells or captions
			if (fCellBorder && c->GetFirstChild() && c != fCaption) {
				r.bottom--;	// Confine bevel inside bounds
				r.right--;
				
				for (long i = fCellBorder; i--;) {
					if (fInvertBorder)
						::PaintBevel(gScreenDevice, r, invalid);
					else
						::PaintAntiBevel(gScreenDevice, r, invalid);
					::InsetRectangle(r, 1, 1);
				}
			}
			
			c->Draw(document, invalid);
		}
#ifdef DEBUG_DRAWBOUNDS
		c->DebugDrawBounds(document, 0xff00ff);
#endif /* DEBUG_DRAWBOUNDS */
		gPageBackColor = oldCellBackColor;

		// Early exit for optimization.
		if (r.top >= invalid->bottom)
			break;
	}
	
	gPageBackColor = oldBackColor;
	
#ifdef DEBUG_DRAWBOUNDS
	DebugDrawBounds(document, 0xff0000);
#endif /* DEBUG_DRAWBOUNDS */
}

short Table::GetMinUsedWidth(const Document*) const
{
	return fMinimumUsedWidth;
}

Displayable* Table::GetParent() const
{
	return fParent;
}

Table* Table::GetParentTable() const
{
	return fParentTable;
}

long Table::GetWidth() const
{
	if (fWidth) return fWidth;
	return ((Page*)GetParent())->GetDefaultMarginWidth();
}

Boolean Table::IsFloating() const
{
	return (fAlign == AV_LEFT || fAlign == AV_RIGHT);
}

Boolean Table::IsLayoutComplete() const
{
	return fLayoutComplete;
}

Boolean Table::IsTable() const
{
	return true;
}

Boolean Table::ReadyForLayout(const Document* document)
{
	// Layout only if table is complete and all images are ready.
	return !fOpen && CompositeDisplayable::ReadyForLayout(document);
}

void Table::ResetLayout(Document*)
{
	// Layout only if table is complete and all images are ready.
	fLayoutComplete = false;
}

void Table::Layout(Document* document, Displayable* parent)
{
	// Layout is where the tricky stuff happens
	// Figure the width of the columns, position the cells in X
	// Figure the height of the rows, position the cells in Y
	// Position the caption, if any

	Page*	parentPage = (Page*)parent;
	
	if (GetFirstChild() == nil)
		return;			// Nothing in table
		
	long parentMarginWidth = parentPage->GetDefaultMarginWidth();
	
	if (fKnownWidth != 0)
	{					// Determine width of table
		long width = fIsPercentage ? (fKnownWidth * parentMarginWidth)/100 : fKnownWidth;
		if (width > parentMarginWidth)
		{
			Message(("Shrinking table width from %d to %d to fit parent", width, parentMarginWidth));
			width = parentMarginWidth;
		}
		SetWidth(width);
	} else
		SetWidth(0);					// Indicate that we don't know how big the table should be

	LayoutColumns(document, parentPage);
	LayoutRows(document, parentPage);
	if (fCaption)
		LayoutCaption(document, parentPage);
}

void Table::LayoutCaption(Document* document, Page* parent)
{
	// Can't layout caption it until the end because width and heght are uncertain
	
	fCaption->SetWidth(GetWidth() - (fCellPadding << 1));		// Know how wide table is
	fCaption->Layout(document, parent);							// Determine height
	fCaption->VAlignCell(fCaption->GetHeight() + (fCellPadding << 1));
	
	if (fCaptionAlign == AV_BOTTOM)
		fCaption->SetTop(GetTop() + GetHeight());
	else 
	{		
		long top = GetTop();
		SetTop(top + fCaption->GetHeight());
		fTop = top;
		fCaption->SetTop(top);
	}
	SetHeight(GetHeight() + fCaption->GetHeight());
}

void Table::LayoutColumns(Document* document, Page* parent)
// returns whether each of the fields fit in the space
{
	long	i, j, cols;
	long	extra;
	Cell*	c;
	PageWidth	usedWidth, widest, h;
	PageWidth	colWidth[200];

	if (fColumns == 0)
		return;
		
	if (IsWarning(fColumns > 200))
		fColumns = 200;
	
	long		tableWidth = GetWidth();
	long		parentWidth = parent->GetDefaultMarginWidth();
	long		parentMarginWidth = parent->GetMarginWidth();

	// Leave HSpace if table is floating.	
	if (IsFloating())
		tableWidth -= kFloatingHSpace;
	
	// Measure all cells (not the caption), measure all the cols
	// Don't use cells with columnSpan != 1 to measure widest
	for (i = 0; i < fColumns; i++) 
	{		
		widest.min = widest.max = -1;
		for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
		{			
			if (c->GetColumn() == i) 
			{				
				if ((i + c->GetColumnSpan()) > fColumns)			// Limit columns span to the table width.
					c->SetColumnSpan(fColumns - i);
						
				c->SetWidth(parentWidth);							// Initial layout with full width available in parent.
				c->Layout(document, parent);

				// Calculate the minimum size for the cell.				
				usedWidth.min = c->GetMinUsedWidth(document);

				// If the cell specified a width, use it for the maximum. If the minimum is smaller,
				// use it for the minimum also.
				if ((usedWidth.max = c->GetKnownWidth()) != 0)
				{
					if (c->IsKnownWidthPercentage())
						usedWidth.max = usedWidth.max * (tableWidth - fBorder * 2 - fCellSpacing)/ 100;
					else if (usedWidth.max > usedWidth.min || c->IsKnownWidthAbsolute())
						usedWidth.min = usedWidth.max;
				}
				else
					usedWidth.max = c->GetMaxUsedWidth(document);	// See how wide the cell wants to be
								
	
				if (c->GetColumnSpan() == 1)
				{
					widest.min = MAX(widest.min, usedWidth.min);
					widest.max = MAX(MAX(widest.max, usedWidth.max), widest.min);
				}
			}
		}
		colWidth[i] = widest;
		colWidth[i].min = MAX(colWidth[i].min, kMinColumnWidth);	// Minimun cell width.
		colWidth[i].max = MAX(colWidth[i].max, kMinColumnWidth);			
	}

	// Check to see that cells with columnSpan != 1 fit, pad cols if required.
	for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
	{		
		if (c->GetColumnSpan() > 1) 
		{			
			long minUsedWidth = 0;
			long minSpanWidth;
			i = c->GetColumn();
			cols = c->GetColumnSpan();
			for (j = i; j < i + cols; j++)
				if (colWidth[j].min > 0)
					minUsedWidth += colWidth[j].min + (fCellPadding << 1) + fCellSpacing + (fCellBorder << 1);
			minSpanWidth = c->GetMinUsedWidth(document) + (fCellPadding << 1) + fCellSpacing + (fCellBorder << 1);
			
			if (minUsedWidth < minSpanWidth) 
			{				
				extra = ((minSpanWidth - minUsedWidth) + cols/2)/cols;
				for (j = i; j < i + c->GetColumnSpan(); j++)
				{
					colWidth[j].min += extra;
					colWidth[j].max = MAX(colWidth[j].max, colWidth[j].min); // Max must be at least min.
				}
			}
		}
	}
	
	// See if we need to widen or shrink the table to meet a width requirement
	h.min = h.max = fBorder + fCellBorder + fCellSpacing + fCellPadding;	// Get table width in h so far
	for (j = 0; j < fColumns; j++) {
		if (colWidth[j].min > 0) {
			 h.min += colWidth[j].min + (fCellPadding << 1) + fCellSpacing + (fCellBorder << 1);
			 h.max += colWidth[j].max + (fCellPadding << 1) + fCellSpacing + (fCellBorder << 1);
		}
	}
	h.min += fBorder - fCellPadding;
	h.max += fBorder - fCellPadding;
	
	fMinimumUsedWidth = h.min;

	// If the cells will not fit horizontally, we shrink them using the following alogrithm, taken from
	// HTML 3.0, March 28, 1995. Note that the final calculated values are placed in PageWidth::min.
/*	
   The minimum and maximum cell widths are then used to determine the
   corresponding minimum and maximum widths for the columns. These in
   turn, are used to find the minimum and maximum width for the table.
   Note that cells can contain nested tables, but this doesn't
   complicate the code significantly. The next step is to assign column
   widths according to the current window size (more accurately - the
   width between the left and right margins). 

   The table borders and intercell margins need to be included in the
   assignment step. There are three cases: 

   1.  The minimum table width is equal to or wider than the available
       space. In this case, assign the minimum widths and allow the
       user to scroll horizontally. For conversion to braille, it will
       be necessary to replace the cells by references to notes
       containing their full content. By convention these appear before
       the table. 

   2.  The maximum table width fits within the available space. In this
       case, set the columns to their maximum widths. 

   3.  The maximum width of the table is greater than the available
       space, but the minimum table width is smaller. In this case,
       find the difference between the available space and the minimum
       table width, lets call it --W--. Lets also call --D-- the
       difference between maximum and minimum width of the table. 

       For each column, let --d-- be the the difference between maximum
       and minimum width of that column. Now set the column's width to
       the minimum width plus --d-- times --W-- over --D--. This makes
       columns with lots of text wider than columns with smaller
       amounts. 
*/

	// If the table min is less than the parent margin width use the margin width as the available width
	if (h.min <= parentMarginWidth)
		tableWidth = parentWidth = parentMarginWidth;
		
	// Before applying the algorithm, see if an explicit table width is smaller than our parent, and forcing a table
	// size less than max.
	Boolean ignoreKnownWidth = false;
	if (h.max > tableWidth && tableWidth < parentWidth)
	{
		tableWidth = MIN(h.max, parentWidth);
		if (IsFloating())
			tableWidth -= kFloatingHSpace; 
		ignoreKnownWidth = true;	// We're ignoring the known width to expand the table to use available width.
	}
	
	if (h.max <= tableWidth)
	{
		if (fKnownWidth == 0 || ignoreKnownWidth)
		{
			// No width specified and max fits, so just use it.
			for (j = 0; j < fColumns; j++)
				colWidth[j].min = colWidth[j].max;
		}
		else
		{
			// A width was specified, widen the cells equally to fit it.
			extra = (tableWidth - h.max) / fColumns;

			for (j = 0; j < fColumns; j++)
				colWidth[j].min = colWidth[j].max + extra;
		}
	}	
	else if (h.min < tableWidth && h.min < h.max)
	{
		// Min fits but max doesn't. Expand min size to fill width using above algorithm.
		long W = tableWidth - h.min;
		long D = h.max - h.min;
		
		for (j = 0; j < fColumns; j++)
		{
			colWidth[j].min = colWidth[j].min + (((colWidth[j].max - colWidth[j].min) * W) / D);
		}
	}
	else if (h.min > tableWidth)
	{
		// Min doesn't fit. We can do several things.
		
		// 1. Shrink borders
		while (h.min > tableWidth && (fBorder > 1 || fCellBorder > 1))
		{
			if (fBorder > 1)
			{
				fBorder--;
				h.min -= 2;
			}
			if (fCellBorder > 1)
			{
				fCellBorder--;
				h.min -= fColumns * 2;
			}
		}
		
		// 2. Shrink CellPadding and CellSpacing
		while (h.min > tableWidth && (fCellPadding > 1 || fCellSpacing > 1))
		{
			if (fCellPadding > 1)
			{
				fCellPadding--;
				h.min -= fColumns * 2;
			}
			if (fCellSpacing > 1)
			{
				fCellSpacing--;
				h.min -= fColumns + 1;
			}
		}

		// 3. Finally, force cell sizes smaller, The amount removed from each column
		// is proportional to how much oversize it is as follows:
		//
		// W = mininum table width.
		// D = difference between mininum table width and available width.
		// c = the width of an individual cell.
		// For each cell:
		// 		c = c - c * D / W
		
		if (h.min > tableWidth)
		{
			for (j = 0; j < fColumns; j++)
				colWidth[j].min -= (colWidth[j].min * (h.min - tableWidth) + h.min / 2) / h.min;
		}
	}	

	// Position cells horizontally
	long hPos = GetLeft() + fBorder + fCellBorder + fCellSpacing + fCellPadding;
	for (i = 0; i < fColumns; i++) 
	{		
		for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
		{			
			if (c->GetColumn() == i)
				c->SetLeft(hPos);						// Set left edge
		}
		hPos += colWidth[i].min;
		for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
		{			
			if (i == (c->GetColumn() + c->GetColumnSpan() - 1)) 
			{				
				c->SetWidth(hPos - c->GetLeft());		// Set right edge
				c->Layout(document, parent);			// Layout again, centering in cell, etc
			}
		}
		if (colWidth[i].min > 0)
			hPos += (fCellPadding << 1) + fCellSpacing + (fCellBorder << 1); 
	}
	hPos += fBorder - fCellBorder - fCellPadding;
	
	SetWidth(hPos - GetLeft() + (IsFloating() ? kFloatingHSpace : 0));
}

void Table::LayoutComplete(Document* document, Displayable* parent)
{
	fLayoutComplete = true;
	CompositeDisplayable::LayoutComplete(document, parent);
}

void Table::LayoutRows(Document*, Page*)
{
	long v, tallest, i, j, rows;
	long usedHeight, spanHeight;
	Cell* c;
	long	rowHeight[250];


	if (IsWarning(fRows > 250))
		fRows = 250;
	
	// Measure the tallest cell in each Column

	for (i = 0; i < fRows; i++) 
	{		
		tallest = -1;
		for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
		{			
			if (c->GetRow() == i) 
			{				
				if ((i + c->GetRowSpan()) > fRows)			// Limit columns span to the table width.
					c->SetRowSpan(fRows - i);

				if (c->GetRowSpan() == 1)
					tallest = MAX(tallest, MAX(c->GetHeight(), c->GetKnownHeight()));
			}
		}
		rowHeight[i] = tallest;
	}

	// Check to see that cells with rowspan != 1 fit, pad rows if required

	for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
	{		
		if (c->GetRowSpan() > 1) 
		{			
			usedHeight = 0;
			i = c->GetRow();
			rows = c->GetRowSpan();
			for (j = i; j < i + rows; j++)
				usedHeight += rowHeight[j] + (fCellPadding << 1);
			spanHeight = c->GetHeight() + (fCellPadding << 1);

			if (usedHeight < spanHeight) 
			{				
				long extra = ((spanHeight - usedHeight) + rows/2)/rows;
				for (j = i; j < i + c->GetRowSpan(); j++)
					rowHeight[j] += extra;
			}
		}
	}

	// Calculate the height of each Column, position cells vertically

	v = GetTop() + fBorder + fCellBorder + fCellSpacing + fCellPadding;
	for (i = 0; i < fRows; i++) 
	{		
		for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
		{			
			if (c->GetRow() == i)
				c->SetTop(v);						// Set top
		}
		v += rowHeight[i];
		for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
		{			
			if (i >= c->GetRow() && i < (c->GetRow() + c->GetRowSpan()))
				c->VAlignCell(v - c->GetTop());		// Set bottom, vertically align cell
		}
		v += (fCellPadding << 1) + fCellSpacing + (fCellBorder << 1); 
	}
	v += fBorder - fCellBorder - fCellPadding;
	fHeight = v - GetTop();
}

Cell* Table::NewCell(Boolean isHeading)
{
	// Let the table instantiate the cell
	
	Cell* cell = new(Cell);
	cell->SetAttribute(A_VALIGN, fVAlign, 0);		// Inherit alignment from table
	if (isHeading)
		cell->SetAttribute(A_ALIGN, AV_CENTER, 0);	// And are centered by default
	else
		cell->SetAttribute(A_ALIGN, fRowAlign, 0);
	cell->SetAttribute(A_BGCOLOR, fRowBackgroundColor, 0);
	return cell;
}

void Table::Open()
{
	fColumns = fRows = 0;
	fCurrentCol = 0;
	fOpen = true;
}

void Table::OpenCaption(Cell* caption)
{
	// Captions are cell displayables too, they have a fRow and fCol of -1
	
	fCaption = caption;
	fCaptionAlign = caption->GetAlign();	// AV_TOP, AV_BOTTOM are alignments relative to table
	caption->SetAlign(AV_CENTER);			// But contents is always centered
	caption->SetRowAndCol(-1, -1);			// Won't get tangled in the rest of the layout
	AddChild(caption);
}

void Table::OpenCell(Cell* cell, Boolean UNUSED(isHeading))
{
	// Open a new cell, position it to avoid rowspans from above
	// Return the new cell so builder can make it a target
	// Does TD and TH tags
	
	if (fInCell)
		CloseCell();
	fInCell = true;
	if (!fInRow)
		OpenRow();
	
	// Find out which col to put cell into

	Cell* c;
	for (c = (Cell *)GetFirstChild(); c; c = (Cell *)c->Next()) 
		if (c->GetRow() + c->GetRowSpan() > fRows)
		{	// This cell intersects with this row
			if (fCurrentCol >= c->GetColumn() && fCurrentCol < (c->GetColumn() + c->GetColumnSpan()))
				fCurrentCol = c->GetColumn() + c->GetColumnSpan();
		}

	cell->SetRowAndCol(fRows, fCurrentCol);	   // Cell is located at row/col
	fColumns = MAX(fColumns, fCurrentCol + 1); // Ignore the column span when computing total columns. If there are that
											   // many columns they will be counted when seen.
	fCurrentCol += cell->GetColumnSpan();	   // Advance horiontally to next col
	AddChild(cell);
}

void Table::OpenRow()
{
	if (fInRow) CloseRow();
	fInRow = true;
	fCurrentCol = 0;
	fRowBackgroundColor = (Color)-1;
	fRowAlign = AV_LEFT;
	fVAlign = AV_MIDDLE;
}

void Table::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{		
		case 	A_BGCOLOR:
			if (value >= 0) fInRow ?
							fRowBackgroundColor = value :
							fBackgroundColor = value;
			break;		
		case	A_BORDER:
			fBorder = (value == -1) ? 1 : MAX(value, 0);
			fCellBorder = MIN(fBorder, 1);
			break;
		case	A_CELLBORDER:
			fCellBorder = (value == -1) ? 1 : MAX(value, 0);
			break;
		case	A_INVERTBORDER:	fInvertBorder = true;					break;
		case	A_CELLSPACING:	if (value >= 0) fCellSpacing = value;	break;	// Space between cells
		case	A_CELLPADDING:	if (value >= 0) fCellPadding = value;	break;	// Space between cell walls and content
		case	A_ALIGN:		if (value > 0) fInRow ? 
									fRowAlign = (AttributeValue)value : 
									fAlign = (AttributeValue)value; 	break;	// In <TABLE> tag and <ROW> tag...
		case	A_VALIGN:		if (value > 0) fVAlign = (AttributeValue)value;	break;	// In <ROW> tag only
		
		case A_WIDTH:	// may be a percentage, must know it was req
			if (value > 0) fKnownWidth = value;
			fIsPercentage = isPercentage;
			break;
		default:
			break;
	}
}

void Table::SetParent(Displayable* parent)
{
	fParent = parent;
}

void Table::SetParentTable(Table* parentTable)
{
	fParentTable = parentTable;
}

void Table::SetLeft(long left)
{
	long newLeft = left;
	if (fAlign == AV_RIGHT)
		newLeft += kFloatingHSpace;
	
	CompositeDisplayable::SetLeft(newLeft);
	fLeft = left;
}

// =============================================================================


