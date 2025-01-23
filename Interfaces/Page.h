// Copyright(c) 1995 Artemis Research, Inc. All rights reserved.

#ifndef __PAGE_H__
#define __PAGE_H__

#ifndef __ANCHOR_H__
#include "Anchor.h"
#endif

#ifndef __FORM_H__
#include "Form.h"
#endif

// =============================================================================
// Page has HTML style line layout

class Page : public CompositeDisplayable 
{
public:
							Page();
	virtual					~Page();

	long					GetDefaultMarginWidth() const;
	long					GetHPos() const;
	long					GetLeftMargin() const;
	long					GetMarginWidth() const;
	long					GetMaxUsedWidth(const Document*) const;
	short					GetMinUsedWidth(const Document*) const;
	AttributeValue			GetPageAlign() const;
	AttributeValue			GetVAlign() const;
	
	Boolean					IsOpen() const;

	void					SetLeftMarginDefault(short leftMarginDefault);
	void					SetVAlign(AttributeValue);

	void					AddFloatingChild(Displayable* child);
	virtual void			Layout(Document*, Displayable* parent);
	virtual void			ResetLayout(Document*);
	
	Rectangle				GetLineBounds(Displayable* lineStart) const;
	
	void					Open();
	void					Close();
	
	// DocumentBuilder only.
public:
	void					SetAlignStackDepth(ushort);
	void					SetStyleStackDepth(ushort);
	ushort					GetAlignStackDepth() const;
	ushort					GetStyleStackDepth() const;

protected:
	long					BreakLine(long vPos, short breakType);
	void					DeferFloatingLayout(Displayable*);		// Defer layout until newline
	void					GetMargins(long vPos, short* left, short* right) const;
	virtual void			LayoutCompleteFor(Displayable*, Document*);
	void					LayoutDeferedFloating(Document*);
	void					LayoutFloatingDisplayable(Document*, Displayable*);
	void					RemoveSoftBreaks();

	virtual long			LayoutLineV(Document*, long vPos, short space, short align, Displayable* firstDisplayable, Displayable* lastDisplayable);
	void					PrepareFloating(Document*);
	Displayable*			PutDisplayable(Document*, Displayable*);
	void					PutLine(Document*, Displayable* lineEnd);

protected:
	ObjectList				fFloatingList;
	Displayable*			fLineBegin;
	Displayable*			fDeferredFloating;
	Displayable*			fNextToPut;
	Displayable*			fLastPut;
	Displayable*			fLastLayedOut;
	long					fVPos;
	short					fHPos;
	short					fLeftMargin;
	short					fLeftMarginDefault;
	short					fMaxHPos;
	short					fRightMargin;
	
	ushort					fStyleStackDepth;
	ushort					fAlignStackDepth;
	
	AttributeValue			fPageAlign;
	AttributeValue			fVAlign;
	
	unsigned				fNoWrap : 1;
	unsigned				fOpen : 1;
};

// =============================================================================

class RootPage : public Page {
public:
							RootPage();
	virtual					~RootPage();

	void					SetTopDisplayable(Displayable*);
	Displayable*			GetTopDisplayable() const;
	
	Boolean					IsLayoutCurrent(const Document*) const;
	
	virtual void			Draw(const Document* document, const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
#endif
	
protected:	
	virtual void			LayoutCompleteFor(Displayable*, Document*);
	
	Displayable*			fTopDisplayable;
};			

// =============================================================================

class SideBar : public Page {
public:
							SideBar();
	virtual					~SideBar();
	
	virtual Boolean			IsLayoutComplete() const;
	virtual void			LayoutComplete(Document*, Displayable* parent);
	virtual void			ResetLayout(Document*);

	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);

protected:
	Boolean					fLayoutComplete;
};			

inline AttributeValue
Page::GetPageAlign() const
{
	return fPageAlign;
}

inline AttributeValue
Page::GetVAlign() const
{
	return fVAlign;
}

inline void
Page::SetVAlign(AttributeValue align)
{
	fVAlign = align;
}

// ===========================================================================
// Margin moves around margins on a newline
// Does not have to descend from SpatialDisplayable, just doing it for debugging

#define kMarginWidth 35

class Margin : public Displayable 
{
public:
							Margin();
	
	virtual short			GetBreakType() const;

	void					SetMargin(short margin);

	void					Draw(const Document* document, const Rectangle* invalid);
	void					Layout(Document*, Displayable* parent);

protected:
	short					fMargin;
};

// DefinitionMargin -- doesn't break if it is not very wide

class DefinitionMargin : public Margin 
{
public:
	virtual short			GetBreakType() const;
};

// ===========================================================================
// Divider Draws horizontal rules

class Divider : public SpatialDisplayable
{
public:
	enum DividerType 
	{							
		kDefault = 0,
		kNoShade
	};
							Divider();

	virtual AttributeValue	GetAlign() const;
	virtual short			GetBreakType() const;
	AttributeValue			GetHAlign() const;							
	virtual	short			GetMinUsedWidth(const class Document*) const;
	virtual Boolean			IsDivider() const;

	void					Draw(const Document* document, const Rectangle* invalid);
	void					Layout(Document*, Displayable* parent);

	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);

protected:
	short					fKnownWidth;

	uchar					fPercentageWidth;	
	DividerType				fDividerType;
	AttributeValue			fHAlign;
	
	unsigned				fInvert : 1;
};

// ===========================================================================
// Line breaks are fairly simple

class LineBreak : public Displayable 
{
public:
	enum BreakType 
	{							
		kNone = 0,
		kSoft,
		kHard,
		kHardLeft,
		kHardRight,
		kHardAll,
		kHardAlign
	};

public:
							LineBreak();

	virtual short			GetBreakType() const;

	void					SetBreakType(BreakType breakType);	// Should only exist inside text

	Displayable*			DeleteIfSoftBreak(Displayable* parent);

protected:
	BreakType				fBreakType;
};

// ===========================================================================
// Alignment defines alignment of subsequent objects

class Alignment : public Displayable 
{
public:
							Alignment();

	virtual short			GetBreakType() const;	

	virtual void			SetAlign(AttributeValue align);
			
	virtual void			Layout(Document*, Displayable* parent);

protected:
	AttributeValue			fAlign;
};

// ===========================================================================


inline long
Page::GetHPos() const
{
	return fHPos;
}

inline long
Page::GetLeftMargin() const
{
	return fLeftMargin;
}

inline Boolean
Page::IsOpen() const
{
	return fOpen;
}

// ===========================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Page.h multiple times"
	#endif
#endif /*__PAGE_H__ */