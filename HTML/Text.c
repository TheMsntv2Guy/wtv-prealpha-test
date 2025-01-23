// ===========================================================================
//	Text.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __TEXT_H__
#include "Text.h"
#endif
#ifdef FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif


// =============================================================================
// Bullet .. puts the dot left of the list items
// kind contains 4 bits of kind
// 12 bits of ordinal value

static void MakeAlphaStr(char* label, long count)
{
	if (count < 0) count = -count;
	label[0] = 'A' + ((count - 1) % 26);	// Could do aa, aaa etc.
	label[1] = 0;
}

static Boolean IsPunctuationBreak(uchar c)
{
	switch (c)
	{
		case '@':
		case '!':
		case ')':
		case '/':
		case '-':
		case '+':
		case ':':
		case ';':
		case '<':
		case '=':
		case '>':
		case '?':
		case ']':
		case '}':
			return true;
	}
	
	return false;
}

static char gRC[] = { 'I', 'V', 'X', 'L', 'C', 'D', 'M' };
static long gRN[] = {	1,  5, 10, 50, 100, 500, 1000 };

static char GetRChar(long* count, Boolean* neg)
{
	*neg = false;
	if (*count >= 1000)
		*count = 1000;
	for (short i = 0; i < 7; i++) 
	{		
		if (*count <= gRN[i+1]) 
		{			
			if ((*count - gRN[i]) <= (gRN[i+1] -* count)) 
			{				
				*count -= gRN[i];
				return gRC[i];
			}
			*count = gRN[i+1] -* count;
			*neg = true;
			return gRC[i+1];
		}
	}
	*count = 0;
	return '?';
}

static void MakeRomanStr(char* label, long count)
{
	// Try and make roman numerals
	
	short s = 0, i = 0;
	char stack[64];
	Boolean	neg;
	
	if (count < 0)
		count = 0;
	label[0] = 0;
	while (count) 
	{		
		short c = GetRChar(&count, &neg);
		if (neg)
			stack[s++] = c;
		else
			label[i++] = c;
	}
	while (s)
		label[i++] = stack[--s];
	label[i++] = 0;
}

Bullet::Bullet()
{
	fStyle = gDefaultStyle;
	
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberBullet;
#endif /* DEBUG_CLASSNUMBER */
}

Bullet::~Bullet()
{
}

void Bullet::SetKind(short kind, short count)
{
	fKind = kind;
	fCount = count;
}

short Bullet::GetBreakType() const
{
	Displayable* g;
	if ((g = (Displayable *)Previous()) != nil && g->IsLineBreak())	// Don't break twice
		return 0;
	return LineBreak::kHard;	// This thing is a line break
}

AttributeValue Bullet::GetAlign() const
{
	switch(fKind)
	{
		case '1':
		case 'A':
		case 'a':
		case 'I':
		case 'i':
			return (AttributeValue)0;
	}
	return SpatialDisplayable::GetAlign();
}

void Bullet::Draw(const Document* document, const Rectangle* invalid)
{
	// Decide what kind of bullet to draw, either text or graphic
	
	char label[32];
	label[0] = 0;
			
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);

	switch (fKind) 
	{		
		case 0:
			DrawGraphicBullet(r.left + 14, r.top, invalid);
			break;

		case 'd':
		case 'D':		
		case 'c':
		case 'C':
		case 's':
		case 'S':
			DrawGraphicBullet(r.left - 10, r.top, invalid);
			break;

		case '1':
			snprintf(label, sizeof(label), "%ld", (long)fCount);
			DrawTextBullet(document, label, false, invalid);
			break;

		case 'A':
		case 'a':
			MakeAlphaStr(label, fCount);
			DrawTextBullet(document, label, fKind == 'a', invalid);
			break;

		case 'I':
		case 'i':
			MakeRomanStr(label, fCount);
			DrawTextBullet(document, label, fKind == 'i', invalid);
			break;
	}
}

void Bullet::DrawGraphicBullet(long left, long top, const Rectangle* invalid)
{
	Rectangle r;
	r.left = left - 2;
	r.top = top - 8;
	r.right = r.left + 4;
	r.bottom = r.top + 4;

	switch (fKind) 
	{		
		case 0:		::PaintBevel(gScreenDevice, r, invalid); 		break;
		case 'D':
// еее Fix when round bevels work!!!!
//		case 'd':	::PaintRoundBevel(gScreenDevice, r, invalid); 	break;	// Disc
		case 'd':	::PaintBevel(gScreenDevice, r, invalid); 	break;	// Disc
		
		case 'C':
// еее Fix when diagonal lines work!!!1
//		case 'c':	::PaintTriangleBevel(gScreenDevice, r, invalid);break;	// Triangle
		case 'c':	::PaintAntiBevel(gScreenDevice, r, invalid); 	break;

		case 'S':
		case 's':	::PaintAntiBevel(gScreenDevice, r, invalid);	break;	// Square
		default:	IsWarning(true);
					::PaintAntiBevel(gScreenDevice, r, invalid);	break;	// Square
	}
}

void Bullet::DrawTextBullet(const Document* document, char* label, Boolean lowerCase, const Rectangle* invalid)
{
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);

	if (lowerCase)
		LowerCase(label);
	
	// Add a period.
	short i = strlen(label);
	label[i] = '.';
	label[i+1] = 0;

	// Paint the text.
	XFont font = document->GetFont(fStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	if ( encoding == kUnknownEncoding )
		encoding = GuessCharacterEncoding(label,strlen(label));
	ulong ascent = ::GetFontAscent(font, encoding);
	ulong textWidth = TextMeasure(gScreenDevice, font, encoding,label, strlen(label)) + 8;
	::PaintText(gScreenDevice, font,encoding, label, i+1,
		GetColor(document), r.left - textWidth, r.top + ascent, GetTransparency(document), false, invalid);
}

Color Bullet::GetColor(const Document* document) const
{
	return document->GetColor(fStyle);
}

uchar Bullet::GetTransparency(const Document*) const
{
	return 0;
}

void Bullet::Layout(Document* document, Displayable*)
{
	// If no containing list exisits, be a dot with width
	SetWidth(fKind ? 0 : (kMarginWidth/2));
	
	switch(fKind)
	{
		case '1':
		case 'A':
		case 'a':
		case 'I':
		case 'i':
			{
				XFont font = document->GetFont(fStyle);
				CharacterEncoding encoding = document->GetCharacterEncoding();
				ulong ascent = ::GetFontAscent(font,encoding);
				ulong descent = ::GetFontDescent(font,encoding);
			
				SetTop(-ascent);
				SetHeight(ascent + descent);
			}
			break;
		
		default:
			SetTop(0);
			break;
	}
}

void Bullet::SetStyle(PackedStyle style)
{
	fStyle = style;
}
	
// =============================================================================

ColorBullet::ColorBullet()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberColorBullet;
#endif /* DEBUG_CLASSNUMBER */
}

ColorBullet::~ColorBullet()
{
}

Color ColorBullet::GetColor(const Document* document) const
{
	return (fStyle.anchor ? document->GetColor(fStyle) : fColor);
}

uchar ColorBullet::GetTransparency(const Document*) const
{
	return fTransparency;
}

// =============================================================================
// Text represents a line of text or a line break

Text::Text()
{
	fStyle = gDefaultStyle;

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberText;
#endif /* DEBUG_CLASSNUMBER */
}

Text::~Text()
{
}

short Text::BreakText(const Document* document, short width, Boolean atSpace)
{
	// Break text at a particular width
	
	if (IsError(width <= 0))
		return 0;

	const char* text = document->GetText(fTextOffset);
	XFont font = document->GetFont(fStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	long spaceBreak = -1;
	long punctBreak = -1;
	long n;


	if ( encoding == kUnknownEncoding )
		encoding = GuessCharacterEncoding(text,fTextCount);

	if ( encoding == kUSASCII || encoding == kUnknownEncoding ) {
		// Try and break in a space
	
		n = 0;
		do {
			if (isspace(text[n])) {
				if (TextMeasure(gScreenDevice, font, encoding,text, n) >= width)
					break;
				spaceBreak = n + 1;
			}
		} while (++n < fTextCount);
	
		if (spaceBreak != -1)
			return spaceBreak;
			
		// Try and break in punctuation.
	#if 1
		n = 0;
		do {
			if ( IsThreeByte(text[n],encoding) )
				n += 2;
			else if ( IsTwoByte(text[n],encoding) )
				n += 1;
			else if (IsPunctuationBreak(text[n])) {
				
				if (TextMeasure(gScreenDevice, font, encoding,text, n + 1) >= width)
					break;
				punctBreak = n + 1;
			}
			n++;
		} while (n < fTextCount);
	
		if (punctBreak != -1)
			return punctBreak;
	#endif		
		// Failed to break in space, break anywhere if space break is not forced
	
		n = 0;
			while (!atSpace && TextMeasure(gScreenDevice, font, encoding, text, n) <= width) {
				if ( IsThreeByte(text[n],encoding) )
					n += 3;
				else if ( IsTwoByte(text[n],encoding) )
					n += 2;
				else
					n++;
				if (n >= fTextCount) {
					n = fTextCount;
					break;
				}
			}
		if (n > 0 && text[n - 1] != ' ')
			n--;
	}
	else {
	
		// Japanese does not have spaces, so break anywhere except on prohibited
		// boundaries. if embedded roman punctuation or spaces, use those.
		
		long ln = 0;
		n = 0;
		do {
			if (TextMeasure(gScreenDevice, font, encoding , text, n) >= width ) {
				if ( spaceBreak != -1 || punctBreak != -1 ) {
					if ( spaceBreak > punctBreak )
						return spaceBreak;
					else
						return punctBreak;
				} else {
					return ln;
				}
			}
			ln = n;
			if ( IsThreeByte(text[n],encoding) ) {
				if ( CanBreakText(text+n,fTextCount-n,encoding) )
					punctBreak = n + 3;
				n += 3;
			}
			else if ( IsTwoByte(text[n],encoding) ) {
				if ( CanBreakText(text+n,fTextCount-n,encoding) )
					punctBreak = n + 2;
				n += 2;
			} else {
				if ( isspace(text[n]) ) 
					spaceBreak = n + 1;
				if ( IsPunctuationBreak(text[n]) ) 
					punctBreak = n + 1;
				n++;
			}
		} while ( n < fTextCount);
	}
	
	return n;
}

#ifdef DEBUG_DISPLAYABLEWINDOW
void
Text::DebugPrintInfo(const Document* document, Rectangle* screenR, Rect* clipRect, Boolean showDetails) const
{
	MacPoint pt;
	GetPen(&pt);
	
	if ((pt.v + 12 > clipRect->top) && (pt.v - 12 < clipRect->bottom))
	{
		MoveTo(250, pt.v);
		DrawText(document->GetText(fTextOffset), 0, fTextCount);
		MoveTo(pt.h, pt.v);
	}
	Displayable::DebugPrintInfo(document, screenR, clipRect, showDetails);
}
#endif /* DEBUG_DISPLAYABLEWINDOW */

void Text::Draw(const Document* document, const Rectangle* invalid)
{
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
		
	const char* text = document->GetText(fTextOffset);
	PackedStyle	style = fStyle;
	if (style.subscript || style.superscript)
		style.fontSize = MAX(1, style.fontSize - 2);		
	XFont font = document->GetFont(style);

	CharacterEncoding encoding = document->GetCharacterEncoding();
	if ( encoding == kUnknownEncoding )
		encoding = GuessCharacterEncoding(text,fTextCount);

	long baseline;
	if (style.subscript)
		baseline = r.bottom - ::GetFontDescent(font, encoding);
	else
	 	baseline = r.top + ::GetFontAscent(font, encoding);

	// Check for intersection when the current text highlight range.
	long startHL;
	long endHL;
	
	// Get highlight range and limit to this text's range.
	document->GetTextHighlightRange(&startHL, &endHL);
	startHL = MAX(startHL, fTextOffset);
	endHL = MIN(endHL, fTextOffset + fTextCount);
	
	// Paint text before highlight start.	
	if (fTextOffset < startHL)
		r.left += ::PaintText(gScreenDevice, font, encoding, text, MIN(startHL - fTextOffset, fTextCount),
							  GetColor(document), r.left, baseline, GetTransparency(document), false, invalid);
	
	// Paint text after start & before end (highlighted region).
	if (endHL > fTextOffset && startHL < (fTextOffset + fTextCount)) {
		Rectangle	background  = r;
		background.right = background.left +
						   ::TextMeasure(gScreenDevice, font, encoding, 
									  	 document->GetText(startHL), endHL - startHL);
		::PaintRectangle(gScreenDevice, background, kBlackColor, 0 , invalid);
		r.left += ::PaintText(gScreenDevice, font, encoding, 
							  document->GetText(startHL), endHL - startHL,
							  kAmberColor, r.left, baseline, 0, false, invalid);
	}
	// Paint text after end.
	if (endHL < fTextOffset + fTextCount)
		::PaintText(gScreenDevice, font, encoding, 
					MAX(text, document->GetText(endHL)), MIN(fTextOffset + fTextCount - endHL, fTextCount),
					GetColor(document), r.left, baseline, GetTransparency(document), false, invalid);
}

#ifdef FIDO_INTERCEPT
void Text::Draw(const Document* document, FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);		

	const char* text = document->GetText(fTextOffset);
	PackedStyle	style = fStyle;
	if (style.subscript || style.superscript)
		style.fontSize = MAX(1, style.fontSize - 2);		
	XFont font = document->GetFont(style);

	CharacterEncoding encoding = document->GetCharacterEncoding();
	if ( encoding == kUnknownEncoding )
		encoding = GuessCharacterEncoding(text,fTextCount);

	long baseline;
	if (style.subscript)
		baseline = r.bottom - ::GetFontDescent(font, encoding);
	else
	 	baseline = r.top + ::GetFontAscent(font, encoding);
	
	fidoCompatibility.PaintText(gScreenDevice, font, text, fTextCount, GetColor(document),
		r.left, baseline, GetTransparency(document), false, nil);
}
#endif

AttributeValue
Text::GetAlign() const
{
	return (AttributeValue)0;
}

void Text::GetBoundsTight(Rectangle* r) const
{
	GetBounds(r);
	
	r->bottom -= fDescent/2;
}

Color Text::GetColor(const Document* document) const
{
	return document->GetColor(fStyle);
}

uchar Text::GetTransparency(const Document*) const
{
	return 0;
}

short Text::GetMinUsedWidth(const Document* document) const
{
	// Measure and return the widest non-breakable chunk of text.
	
	ulong minWidth = 0;
	long i = 0;
	
	XFont font = document->GetFont(fStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	const char* text = document->GetText(fTextOffset);
	if ( encoding == kUnknownEncoding )
		encoding = GuessCharacterEncoding(text,fTextCount);
	
	while (i < fTextCount)
	{				
		// Find space, punct, or end of text.
		long n;
		for (n = i; !isspace(text[n]) && !IsPunctuationBreak(text[n]) && n < fTextCount; n++) {
			if ( IsThreeByte((uchar)&text[n],encoding) )	
				n += 2;
			else if ( IsTwoByte((uchar)&text[n],encoding) )
				n++;
		}
		
		// Include width of punctuation character.
		if (n < fTextCount && IsPunctuationBreak(text[n]))
			minWidth = MAX(minWidth, TextMeasure(gScreenDevice, font, encoding, text + i, n - i + 1));
			
		// Don't include width of spaces.
		else if (n > i)
			minWidth = MAX(minWidth, TextMeasure(gScreenDevice, font, encoding, text + i, n - i));
		
		if ( IsThreeByte((uchar)&text[n],encoding) )	
			i = n + 3;
		else if ( IsTwoByte((uchar)&text[n],encoding) )	
			i = n + 2;
		else
			i = n + 1; // Skip space/punct
	}
	
	return minWidth;
}

void Text::InsertSoftBreak(Document* document, Displayable* parent, short offset)
{
	// Split a text displayable into two pieces by inserting a soft line break
	// A newline displayable and a new text displayable will be created and inserted into the list after this
	// Width of pieces is not correct until next layout
	
	LineBreak* lineBreak = new(LineBreak);
	Text* secondHalf = NewText();
	
	lineBreak->SetBreakType(LineBreak::kSoft);
	parent->AddChildAfter(lineBreak, this);
	secondHalf->SetText(fTextOffset + offset, fTextCount - offset, fStyle);
	SetText(fTextOffset, offset, fStyle);
	parent->AddChildAfter(secondHalf, lineBreak);	// Second half of text
	
	secondHalf->Layout(document, parent);			// Measure pieces
	Layout(document, parent);
}

Boolean Text::IsLayoutComplete() const
{
	return fLayoutComplete;
}

Boolean Text::IsSeparable(const Document* document) const
{
	// Can this text block be separated from previous
	
	const char* text = document->GetText(fTextOffset);
	
	CharacterEncoding encoding = document->GetCharacterEncoding();
	if ( encoding == kUnknownEncoding )
		encoding = GuessCharacterEncoding(text,fTextCount);
	if ( IsTwoByte((uchar)&text[-1],encoding) )
		return false;
	if ( IsThreeByte((uchar)&text[-1],encoding) || IsThreeByte((uchar)&text[-2],encoding) )
		return false;
	if ( text[0] == 0x1b )
		return false;
	return (isspace(text[0]) || IsPunctuationBreak(text[0]) || isspace(text[-1]) || ispunct(text[-1]));
}

Boolean Text::IsText() const
{
	return true;
}

void Text::Layout(Document* document, Displayable* parent)
{
	if (IsError(document == nil || parent == nil))
		return;

	Page*	parentPage = (Page*)parent;		
	const char* text = document->GetText(fTextOffset);
	XFont font = document->GetFont(fStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	if ( encoding == kUnknownEncoding )
		encoding = GuessCharacterEncoding(text,fTextCount);
	ulong ascent = ::GetFontAscent(font,encoding);
	ulong descent = ::GetFontDescent(font,encoding);
	
	// Don't measure the final space if next displayable is linebreak;
	short textCount = fTextCount;
	Displayable* next = (Displayable*)Next();
	if (textCount > 0 && text[textCount-1] == ' ' && 
	    ((next == nil && !parentPage->IsOpen()) || (next != nil && next->IsLineBreak())))
		textCount--;
	
	// Don't measure leading space if previous is floating.
	if (textCount > 0 && text[0] == ' ' && parentPage->GetHPos() == parentPage->GetLeftMargin()) {
		text++;
		textCount--;
	}

	if (textCount > 0) {	
		SetWidth(TextMeasure(gScreenDevice, font,encoding, text, textCount));

		// Handle subscript and superscript
		if (fStyle.subscript || fStyle.superscript) {
			PackedStyle	sStyle = fStyle;
			sStyle.fontSize = MIN(1, sStyle.fontSize - 1);
			
			XFont		sFont = document->GetFont(sStyle);
			ulong		sAscent = ::GetFontAscent(sFont, encoding);
						
			if (fStyle.subscript)
				descent += sAscent/2;
			else
				ascent += sAscent/2;
		}
		
		SetTop(-ascent);
		SetHeight(ascent + descent);
		fDescent = descent;
	}
	else {
		SetWidth(0);
		SetTop(0);
		SetHeight(0);
	}
}

void Text::LayoutComplete(Document*, Displayable*)
{
	fLayoutComplete = true;
}

Text* Text::NewText() const
{
	return new(Text);
}

void Text::RemoveSoftBreak(Text* secondHalf)
{
	// Merge this text displayable with previous one

	if (IsError(secondHalf == nil))
		return;
		
	fTextCount += secondHalf->fTextCount;
}

void Text::ResetLayout(Document*)
{
	fLayoutComplete = false;
}

void Text::SetText(long textOffset, short textCount, PackedStyle style)
{
	// Text displayables point to text in a separate pool
	
	fTextOffset = textOffset;
	fTextCount = textCount;
	fStyle = style;
}

// =============================================================================

ColorText::ColorText()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberColorText;
#endif /* DEBUG_CLASSNUMBER */
}

ColorText::~ColorText()
{
}

Color ColorText::GetColor(const Document* document) const
{
	return (fStyle.anchor ? document->GetColor(fStyle) : fColor);
}

uchar ColorText::GetTransparency(const Document*) const
{
	return fTransparency;
}

Text* ColorText::NewText() const
{
	ColorText* text = new(ColorText);
	text->SetColor(fColor);
	text->SetTransparency(fTransparency);
	return text;
}

// =============================================================================
