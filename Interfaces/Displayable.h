// ===========================================================================
//	Displayable.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __DISPLAYABLE_H__
#define __DISPLAYABLE_H__

#ifdef DEBUG_CLASSNUMBER
	#ifndef __CLASSES_H__
	#include "Classes.h"				/* for ClassNumber */
	#endif
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"					/* for Rectangle */
#endif
#ifndef __LINKABLE_H__
#include "Linkable.h"					/* for Linkable */
#endif
#ifndef __PARSER_H__
#include "Parser.h"						/* for Attribute */
#endif

//#define DEBUG_DRAWBOUNDS   // for displayable bounds debugging

class ContentView;
class Document;
class Image;
class ImageMap;
class Input;
class Layer;
class Region;

// ===========================================================================
// HasHTMLAttributes base class
class HasHTMLAttributes : public Linkable
{
public:
							HasHTMLAttributes();
	virtual					~HasHTMLAttributes();

	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
};	

// ===========================================================================
// Displayable base class
class Displayable : public HasHTMLAttributes
{
public:
							Displayable();
	virtual 				~Displayable();

	virtual long			AlignScrollPoint(long newScroll, long oldScroll) const;
	
	virtual AttributeValue	GetAlign() const;
	virtual void			GetBounds(Rectangle* r) const;		// empty
	virtual void 			GetBoundsTight(Rectangle* r) const;	// defaults to GetBounds
	virtual short			GetBreakType() const;				// 0
	virtual Displayable*	GetFirstChild() const;
	virtual void			GetFirstRegionBounds(Rectangle* r) const; // defaults to GetBounds
	virtual const char*		GetID() const;
	virtual ImageMap*		GetImageMap() const;
	virtual long			GetHeight() const;
	virtual Displayable*	GetLastChild() const;
	virtual long			GetLeft() const;
	virtual Image*			GetMappedImage() const;
	virtual Displayable*	GetParent() const;
	virtual void			GetSelectionRegion(Region*) const;	
	virtual long			GetTop() const;
	virtual long			GetWidth() const;
	virtual long			GetLayoutWidth() const;				// override to affect layout
	virtual long			GetLayoutHeight() const;			// override to affect layout
	virtual	short			GetMinUsedWidth(const class Document*) const;	// GetWidth();
	virtual Boolean			HasURL() const;						// false
	virtual Boolean			IsAnchor() const;				// false
	virtual Boolean			IsAnchorEnd() const;				// false
	virtual Boolean			IsDivider() const;					// false
	Boolean					IsExplicitLineBreak() const;		// false
	virtual Boolean			IsFloating() const;					// false
	virtual Boolean			IsHighlightable() const;			// IsSelectable()
	virtual Boolean			IsImage() const;					// false
	virtual Boolean			IsImageMapSelectable() const;		// false
	virtual Boolean			IsLayoutComplete() const;			// true
	Boolean					IsLineBreak() const;				// false
	virtual Boolean			IsSelectable() const;				// false
	virtual Boolean			IsInitiallySelected() const;		// false
	virtual Boolean			IsSeparable(const class Document*) const;	// false
	virtual Boolean			IsSubmit() const;
	virtual Boolean			IsText() const;						// false
	virtual Boolean			IsTextField() const;				// false
	virtual Boolean			IsCell() const;						// false
	virtual Boolean			IsTable() const;					// false

	virtual char*			NewURL(const Coordinate* selectionPosition, const char* tag) const; // nil

	virtual void			Select();
	virtual void			SelectFirst();
	virtual void			Deselect();
	
	virtual void			SetAlign(AttributeValue align);
	virtual void			SetHeight(long height);
	virtual void			SetLeft(long left);
	virtual void			SetParent(Displayable* parent);
	virtual void			SetTop(long top);
	virtual void			SetWidth(long width);

	virtual void			AddChild(Displayable* child);
	virtual void			AddChildAfter(Displayable* child, Displayable* after);
	virtual void			RemoveChild(Displayable*);
	
	virtual Boolean			BumpSelectionPosition(Coordinate*, const Coordinate*,
												  ContentView*);
	virtual void			Commit(long value);
	virtual void			Draw(const Document*, const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual Boolean			DownInput();
	virtual Boolean			ExecuteInput();
	virtual Boolean			KeyboardInput(Input*);
	virtual Boolean			UpInput();
	virtual Boolean			MoveCursorDown();
	virtual Boolean			MoveCursorLeft();
	virtual Boolean			MoveCursorRight();
	virtual Boolean			MoveCursorUp();
	virtual Boolean			ReadyForLayout(const Document*);	// true;	
	virtual void			Layout(Document*, Displayable* parent);
	virtual void			LayoutComplete(Document*, Displayable* parent);
	virtual void			ResetLayout(Document*);

public:
#ifdef DEBUG_DRAWBOUNDS
	void					DebugDrawBounds(const Document* document, Color color) const;
#endif /* DEBUG_DRAWBOUNDS */
#ifdef DEBUG_DISPLAYABLEWINDOW
	virtual void			DebugPrintInfo(const Document* document, Rectangle* screenR, Rect* clipRect, Boolean showDetails) const;
#endif /* DEBUG_DISPLAYABLEWINDOW */
#ifdef DEBUG_CLASSNUMBER
	ClassNumber				fClassNumber;
#endif /* DEBUG_CLASSNUMBER */
};

// ===========================================================================
// SpatialDisplayable has instance data for top, left, width, height

class SpatialDisplayable : public Displayable 
{
public:
							SpatialDisplayable();
	virtual					~SpatialDisplayable();
	
	virtual long			GetHeight() const;
	virtual long			GetLeft() const;
	virtual long			GetTop() const;
	virtual long			GetWidth() const;
	
	virtual void			SetHeight(long);
	virtual void			SetLeft(long);
	virtual void			SetTop(long);
	virtual void			SetWidth(long);
	
protected:
	long					fTop;				// Need to be signed for layout
	long					fHeight;					
	short					fLeft;
	short					fWidth;
};

// ===========================================================================
// CompositeDisplayable can contain others

class CompositeDisplayable : public SpatialDisplayable 
{
public:
							CompositeDisplayable();
	virtual 				~CompositeDisplayable();

	virtual AttributeValue	GetAlign() const;
	virtual Displayable*	GetFirstChild() const;
	virtual Displayable*	GetLastChild() const;

	virtual void			SetAlign(AttributeValue align);
	virtual void			SetLeft(long left);
	virtual void			SetTop(long top);

	virtual void			AddChild(Displayable*);
	virtual void			AddChildAfter(Displayable* child, Displayable* after);
	virtual void			RemoveChild(Displayable*);
	virtual void			Draw(const Document*, const Rectangle* invalid);
	
	virtual Boolean			ReadyForLayout(const Document*);
	virtual void			Layout(Document*, Displayable* parent);
	virtual void			LayoutComplete(Document*, Displayable* parent);
	virtual void			ResetLayout(Document*);

#ifdef DEBUG_DISPLAYABLEWINDOW
	virtual void			DebugPrintInfo(const Document* document, Rectangle* screenR, Rect* clipRect, Boolean showDetails) const;
#endif /* DEBUG_DISPLAYABLEWINDOW */
#ifdef FIDO_INTERCEPT
	virtual void			Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
#endif
protected:
	LinkedList				fChildren;
	AttributeValue			fAlign;
};


// ===========================================================================

#endif /*__DISPLAYABLE_H__ */
