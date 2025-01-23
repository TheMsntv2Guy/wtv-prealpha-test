// Copyright(c) 1995 Artemis Research, Inc. All rights reserved.

#ifndef __TEXT_H__
#define __TEXT_H__

#ifndef __DOCUMENT_H__
#include "Document.h"
#endif

// ===========================================================================
// Text represents a line of text or a line break

class Text : public SpatialDisplayable 
{
public:
							Text();
	virtual 				~Text();

	Boolean					ContainsOffset(long offset) const;
		
	virtual AttributeValue	GetAlign() const;
	virtual void			GetBoundsTight(Rectangle*) const;
	virtual short			GetMinUsedWidth(const Document* document) const;
	PackedStyle				GetStyle() const;
	virtual Boolean			IsSeparable(const Document*) const;		// True if it ends with a space
	virtual Boolean			IsText() const;
	
	void					SetText(long textOffset, short textCount, PackedStyle style);

	short					BreakText(const Document*, short width, Boolean atSpace);
	virtual void			Draw(const Document*, const Rectangle* invalid);
	void					InsertSoftBreak(Document*, Displayable* parent, short offset);
	virtual Boolean			IsLayoutComplete() const;
	virtual void			Layout(Document*, Displayable* parent);
	virtual void			LayoutComplete(Document*, Displayable* parent);
	void					RemoveSoftBreak(Text* secondHalf);
	virtual	void			ResetLayout(Document*);
	void					SetStyle(PackedStyle);

#ifdef DEBUG_DISPLAYABLEWINDOW
	virtual void			DebugPrintInfo(const Document* document, Rectangle* screenR, Rect* clipRect, Boolean showDetails) const;
#endif /* DEBUG_DISPLAYABLEWINDOW */
#ifdef FIDO_INTERCEPT
	virtual void			Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
#endif /* FIDO_INTERCEPT */
	
protected:
	virtual Color			GetColor(const Document*) const;
	virtual uchar			GetTransparency(const Document*) const;
	virtual Text*			NewText() const;

	long					fTextOffset;
	short					fTextCount;
	PackedStyle				fStyle;
	uchar					fDescent;
	Boolean					fLayoutComplete;
};

class ColorText : public Text
{
public:
							ColorText();
	virtual					~ColorText();

	void					SetColor(Color);
	void					SetTransparency(uchar);
		
protected:
	virtual Color			GetColor(const Document*) const;
	virtual uchar			GetTransparency(const Document*) const;
	virtual Text*			NewText() const;
	
	unsigned				fColor : 24;
	unsigned				fTransparency : 8;
};

// ===========================================================================
// Bullet -- puts the dot left of the list items

class Bullet : public SpatialDisplayable 
{
public:
							Bullet();
	virtual					~Bullet();
							
	virtual AttributeValue	GetAlign() const;
	virtual short			GetBreakType() const;

	void					Draw(const Document* document, const Rectangle* invalid);
	void					Layout(Document* document, Displayable* parent);

	void					SetKind(short kind, short count);
	void					SetStyle(PackedStyle style);

protected:
	void					DrawGraphicBullet(long left, long top, const Rectangle* invalid);
	void					DrawTextBullet(const Document* document, char* label, Boolean lowerCase, const Rectangle* invalid);
	virtual Color			GetColor(const Document*) const;
	virtual uchar			GetTransparency(const Document*) const;
	
protected:
	PackedStyle				fStyle;
	ushort					fCount;
	Byte					fKind;
};

class ColorBullet : public Bullet 
{
public:
							ColorBullet();
	virtual 				~ColorBullet();

	void					SetColor(Color);
	void					SetTransparency(uchar);
		
protected:
	virtual Color			GetColor(const Document*) const;
	virtual uchar			GetTransparency(const Document*) const;
	
	unsigned				fColor : 24;
	unsigned				fTransparency : 8;
};
							
// ===========================================================================

inline PackedStyle Text::GetStyle() const
{
	return fStyle;
}

inline Boolean Text::ContainsOffset(long offset) const
{
	return (offset >= fTextOffset && offset < (fTextOffset + fTextCount));
}

inline void Text::SetStyle(PackedStyle style)
{
	fStyle = style;
}

inline void ColorText::SetColor(Color color)
{
	fColor = color;
}

inline void ColorText::SetTransparency(uchar transparency)
{
	fTransparency = transparency;
}

inline void ColorBullet::SetColor(Color color)
{
	fColor = color;
}

inline void ColorBullet::SetTransparency(uchar transparency)
{
	fTransparency = transparency;
}

// ===========================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Text.h multiple times"
	#endif
#endif /*__TEXT_H__ */
