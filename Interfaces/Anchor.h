// ===========================================================================
//	Anchor.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __ANCHOR_H__
#define __ANCHOR_H__

#ifndef __DISPLAYABLE_H__
#include "Displayable.h"			/* for Displayable */
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"				/* for Rectangle */
#endif
#ifndef __PARSER_H__
#include "Parser.h"					/* for Attribute */
#endif



class Anchor;
class Document;
class Image;
class Region;

// ===========================================================================
// AnchorEnd just turns things off at the end of an anchor

class AnchorEnd: public Displayable 
{
public:
							AnchorEnd();
	virtual 				~AnchorEnd();
							
	virtual Boolean			IsAnchorEnd() const;

	Boolean					IsLayoutComplete() const;
	virtual void			LayoutComplete(Document*, Displayable* parent);
	virtual void			ResetLayout(Document*);

protected:
	Anchor*					FindAnchorStart() const;
	
	Boolean					fLayoutComplete;
};

// ===========================================================================

class Anchor : public Displayable 
{
public:
							Anchor();
	virtual 				~Anchor();
	
	virtual AttributeValue	GetAlign() const;
	virtual void			GetBounds(Rectangle*) const;
	virtual void			GetFirstRegionBounds(Rectangle*) const;
	const char*				GetHREF() const;
	virtual const char*		GetID() const;
	virtual Image*			GetMappedImage() const;
	virtual Displayable*	GetParent() const;
	virtual void			GetSelectionRegion(Region* region) const;
	virtual long			GetTop() const;
	virtual Boolean			HasURL() const;
	virtual Boolean			IsAnchor() const;
	virtual Boolean			IsHighlightable() const;
	virtual Boolean			IsInitiallySelected() const;
	virtual Boolean			IsLayoutComplete() const;
	virtual Boolean			IsSelectable() const;
	virtual char*			NewURL(const Coordinate*, const char* tag) const;

	void					ResetHREF();
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
	void					SetMappedImage(Image*);
	virtual void			SetParent(Displayable*);
	virtual void			SetTop(long);
	void					SetVisited();

protected:
	AnchorEnd*				FindAnchorEnd() const;

	char*					fHREF;
	Image*					fImage;
	char*					fName;
	Displayable*			fParent;
	long 					fTop;

	unsigned				fIsInitiallySelected : 1;
	unsigned				fNoHighlight : 1;
};

// ===========================================================================

inline const char* Anchor::GetHREF() const
{
	return fHREF;
}

inline long Anchor::GetTop() const
{
	return fTop;
}

inline void Anchor::SetTop(long top)
{
	fTop = top;
}

// ===========================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Anchor.h multiple times"
	#endif
#endif /*__ANCHOR_H__ */
