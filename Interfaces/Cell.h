// ===========================================================================
//	Cell.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CELL_H__
#define __CELL_H__

#ifndef __GRAPHICS_H__
#include "Graphics.h"		/* for Color, Rectangle */
#endif
#ifndef __PARSER_H__
#include "Parser.h"			/* for Attribute */
#endif
#ifndef __PAGE_H__
#include "Page.h"			/* for Page */
#endif




class Document;

// ===========================================================================

class Cell : public Page 
{
public:
							Cell();
	virtual					~Cell();
	
	Boolean					IsCell() const;	
	Color					GetBackgroundColor() const;
	short					GetColumn() const;
	short					GetColumnSpan() const;
	long					GetKnownHeight() const;
	long					GetKnownWidth() const;
	Boolean					IsKnownWidthAbsolute() const;
	Boolean					IsKnownWidthPercentage() const;
	short					GetRow() const;
	short					GetRowSpan() const;

	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	void					SetColumnSpan(short span);
	void					SetRowSpan(short span);
	void					SetRowAndCol(short row,short col);

	virtual void			Draw(const Document* document, const Rectangle* invalid);
	void					VAlignCell(long height);
		
protected:
	Color					fBackgroundColor;
	
	short					fColumn;
	short					fColumnSpan;
	short					fKnownHeight;
	short					fKnownWidth;
	short					fRow;
	short					fRowSpan;
	
	unsigned				fIsAbsolute : 1;
	unsigned				fIsPercentage  : 1;
};

// ===========================================================================

inline Color Cell::GetBackgroundColor() const
{
	return fBackgroundColor;
}

// ===========================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Cell.h multiple times"
	#endif
#endif /*__CELL_H__ */