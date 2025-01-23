// ===========================================================================
//	Table.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __TABLE_H__
#define __TABLE_H__

#ifndef __CELL_H__
#include "Cell.h"
#endif

// =============================================================================

class Table : public CompositeDisplayable 
{
public:
							Table();
	virtual 				~Table();
	
	virtual void			AddChild(Displayable*);
	virtual long			AlignScrollPoint(long newScroll, long oldScroll) const;	
	virtual	Displayable* 	GetParent() const;
	virtual	long 			GetWidth() const;
	Boolean					IsInCell() const;
	virtual Boolean			IsFloating() const;
	virtual Boolean			IsLayoutComplete() const;
	virtual Boolean			IsTable() const;					
	
	Table*					GetParentTable() const;

	virtual void 			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);

	void					SetParentTable(Table* parentTable);
	void					SetLeft(long);

	void					Close();
	void					CloseCaption();
	void					CloseCell();
	void					CloseRow();
	virtual void			Draw(const Document* document, const Rectangle* invalid);
	virtual	short			GetMinUsedWidth(const Document*) const;
	virtual Boolean			ReadyForLayout(const Document*);
	virtual void			Layout(Document*, Displayable* parent);
	virtual void			LayoutComplete(Document*, Displayable* parent);
	Cell*					NewCell(Boolean isHeading);
	void					Open();
	void					OpenCaption(Cell* caption);
	void					OpenCell(Cell* cell, Boolean isHeading);
	void					OpenRow();
	virtual void			ResetLayout(Document*);
	virtual void			SetParent(Displayable* parent);

protected:
	void					LayoutCaption(Document*, Page* parent);
	void					LayoutColumns(Document*, Page* parent);
	void					LayoutRows(Document*, Page* parent);

protected:
	Color					fBackgroundColor;
	Color					fRowBackgroundColor;
	Cell*					fCaption;
	Displayable*			fParent;
	Table*					fParentTable;

	short					fBorder;
	short					fCellBorder;
	short					fCellPadding;
	short					fCellSpacing;
	short					fColumns;
	short					fCurrentCol;
	short					fKnownWidth;
	short					fMinimumUsedWidth;
	short					fRows;

	AttributeValue			fCaptionAlign;
	AttributeValue			fRowAlign;
	AttributeValue			fVAlign;
	
	unsigned				fInCell : 1;
	unsigned				fInRow : 1;
	unsigned				fInvertBorder : 1;
	unsigned				fIsPercentage : 1;
	unsigned				fLayoutComplete : 1;
	unsigned				fOpen : 1;
};

// =============================================================================

inline Boolean Table::IsInCell() const
{
	return fInCell;
}

// =============================================================================
#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Table.h multiple times"
	#endif
#endif /*__TABLE_H__ */