
#include "GraphicsPrivate.h"


#ifdef FOR_MAC
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
#endif





#define FontSize(xfont) ((xfont) & 0xff)
#define FontStyle(xfont) (((xfont)>>8) & 0xff)
#define FontFace(xfont) (((xfont)>>16) & 0xffff)


const kMaxPaintTextPasses = 8;

typedef struct {
	Color	color;
	short	offsetX;
	short	offsetY;
	ulong	transparency;
	Boolean	antiAlias;
} PaintTextPassParams;	
		

Color gPageBackColor = 0;


static ushort gFontExtendWidth = 8;


void
DecompressGlyph(uchar *ibuf,uchar *obuf,short width,short height,short rowBytes);

static long
LookupNextCharacter(Strike *strike,const char *&text,ulong &length,CharacterEncoding encoding);


static int
GetStyledTextParameters(FontStyle style,Color color,ulong transparency,PaintTextPassParams *params,Rectangle &extend);

static CLUT* MakeFontColorTable(short depth, Color foreground, Color background);



static Strike* FontToStrike(XFont font,CharacterEncoding encoding)
{
	static XFont	lastFont = 0;			// one deep cache
	static Strike*	lastStrike = 0;
	
	if (font == lastFont && (encoding == kUnknownEncoding || encoding == kUSASCII ) )
		return lastStrike;
		
	short 	theFont = FontFace(font);
//	short	theStyle = FontStyle(font);		// not used now
	short	theSize = FontSize(font);
	
	
	if ( gSystem->GetUseJapanese() ) {
		if ( encoding != kUnknownEncoding && encoding != kUSASCII ) {
			if ( encoding == kSJIS || encoding == kJIS || encoding == kNEC || encoding ==  kEUC )
				theFont = kOsaka;
		}
	}
	
	theSize &= ~1;				// only have even sizes
	if (theSize > 40)
		theSize = 40;
	else if (theSize < 12)
		theSize = 12;
	
	FSNode*	theStrikeNode;
	
	char fontName[32];
	Boolean triedBigger = false;


	do {
	switch(theFont)
		{
		case kOsaka:
			if ( theSize < 14 )
				theSize = 12;
			else if ( theSize <= 18 )
				theSize = 16;
			else
				theSize = 28;
			snprintf(fontName, sizeof(fontName), "%s_%d", "/ROM/Fonts/Osaka", theSize);
			break;
		case kPalatino:
			snprintf(fontName, sizeof(fontName), "%s_%d", "/ROM/Fonts/Palatino", theSize);
			break;
		case kMonaco:
			snprintf(fontName, sizeof(fontName), "%s_%d", "/ROM/Fonts/Monaco", theSize);
			break;
		default:
			if ( theFont != kHelvetica ) {
				Message(("Unsupported font %X - using Helvetica instead",theFont));
			}
			snprintf(fontName, sizeof(fontName), "%s_%d", "/ROM/Fonts/Helvetica", theSize);
			break;
		}

		theStrikeNode = Resolve(fontName, false);
		if ( theStrikeNode == nil ) {
			if ( theSize == 12 && triedBigger ) {
				if ( theFont == kHelvetica ){
					Complain(("Font '%s' not found",fontName));
					lastStrike = nil;
					lastFont = 0;
					return nil;
				}
				Message(("Warning, font '%s' not found switching to kHelvetica",fontName));
				theFont = kHelvetica;
				theSize = FontSize(font);
			} else { 
				if ( theSize == 12 ) {
					triedBigger = true;
					theSize = 42;
				} else {
					theSize--;
				}
				Message(("Warning font '%s' not found, trying size %d",fontName,theSize));
			}
		}
	} while ( theStrikeNode == nil );
	
	lastFont = font;
	lastStrike = (Strike*)theStrikeNode->data;
	return lastStrike;
}


void TextBounds(BitMapDevice& device, XFont font,CharacterEncoding encoding, const char* text, ulong length, Ordinate x, Ordinate y, Rectangle* bounds)
{
#ifdef CAN_USE_QUICKDRAW	
	if (gMacSimulator->GetUseQuickdraw()) {
		FastTextBounds(device, font, encoding,text, length, x, y, bounds);
		return;
	}
#endif
	long maxAscent  = -100;
	long maxDescent = -100;
	long leftEdge   = x+100;
	long rightEdge  = -100;
	long c;
	Rectangle	extend;
	long	extendWidth = gFontExtendWidth;
	
	if ( device.format )
		;
	
	Strike *strike = FontToStrike(font,encoding);
	if (strike == nil) {
		Complain(("nil strike"));
		return;
	}
	if ( GetStyledTextParameters(FontStyle(font),0,0,nil,extend) > 1 ) {
		extendWidth += (extend.right + extend.left);
	}
	while ( length )	{
		c = LookupNextCharacter(strike,text,length,encoding);
		if ( c == -1 )
			continue;
		Glyph* thisGlyph = &strike->glyphs[c];
		if (thisGlyph->lines) {
			maxAscent  = MAX(thisGlyph->ascent, maxAscent);
			maxDescent = MAX(thisGlyph->lines - thisGlyph->ascent, maxDescent);
			leftEdge   = MIN(x - thisGlyph->kern, leftEdge);
			rightEdge  = x - thisGlyph->kern + thisGlyph->rowBytes * 8 / strike->depth;
			}
		if (strike->version & kFractionalWidthVersion)
			x += (thisGlyph->width + extendWidth) >> kWidthFractionBits;
		else
			x += thisGlyph->width + (extendWidth >> kWidthFractionBits);
		}
	bounds->top    = (y - maxAscent) - (extend.top>>kWidthFractionBits);
	bounds->bottom = (y + maxDescent) + (extend.bottom>>kWidthFractionBits);
	bounds->left   = leftEdge;
	bounds->right  = rightEdge;
}


long PaintText(BitMapDevice& device, XFont font,CharacterEncoding encoding,const char* text, ulong length, Color color, Ordinate x, Ordinate y, ulong transparency, Boolean antiAlias, const Rectangle* clip)
{
	Rectangle	tBounds;
	Strike 		*strike;


#ifdef CAN_USE_QUICKDRAW	
	if (!(device.format & 0x8700) && gMacSimulator->GetUseQuickdraw())
		return PaintTextQD(device, font, encoding,text,  length,  color,  x,  y,  transparency,  antiAlias, clip);
#endif


#if defined(FOR_MAC)
	if ( gMacSimulator->GetForceAntialiasText() )
		antiAlias = true;
#else
	antiAlias = true;
#endif

	strike = FontToStrike(font,encoding);
	if (strike == nil) {
		Complain(("nil strike"));
		return 0;
	}

	

	long textWidth = 0;
	Rectangle charRect, dstRect;
	long depth = strike->depth;
	long c;
	BitMapFormat format;
	BitMapDevice fontDevice;
	CLUT *clut = nil;
	Rectangle	clipRect;
	long dShift = 0;
	ulong transparentColor;
	Rectangle extend;
	long last = strike->last;
	const kDecodeBufferSize = 50*40/2;
	static uchar sGlyphDecodeBuffer[kDecodeBufferSize];
	Byte* bitsBase = (Byte*)&strike->glyphs[last - strike->first + 1];
	long	extendWidth = gFontExtendWidth;
	
	PaintTextPassParams	params[kMaxPaintTextPasses],*p;
	
	if ( strike->version & kTwoByteVersion ) {
		last <<= 8;
		last |= strike->lastLow;
	}
	

	TextBounds(device, font,encoding,text, length, x, y, &tBounds);
	int numPasses = GetStyledTextParameters(FontStyle(font),color,transparency,params,extend);
	
	if ( numPasses > 1 ) {
		extendWidth += extend.right + extend.left;
		antiAlias = true;
	}	
	params[0].antiAlias = antiAlias;

	if ( clip )
		clipRect = *clip;
	else
		clipRect = device.bounds;
	
	if ( antiAlias )
	{
		format = depth == 8 ? antialias8Format : antialias4Format;
		transparentColor = 0;
		clut = nil;
	}
	else
	{
		format = depth == 8  ? index8Format : index4Format;
		clut = MakeFontColorTable(depth, color, gPageBackColor);
		transparentColor = kTransparent;
	}

	if ( depth == 4 )
		dShift = 1;
	
	while (length )	// char is used as an index -- must be unsigned
	{
		c = LookupNextCharacter(strike,text,length,encoding);
		if ( c == -1 )
			continue;
		// generate a GrafDevice corresponding to this character's bits
		Glyph* thisGlyph = &strike->glyphs[c];
		long offset = thisGlyph->offset;
		Boolean isCompressed = (strike->version & kCompressedVersion) && (offset & 0x80000000);
		long width = thisGlyph->width;
		if (strike->version & kFractionalWidthVersion)
			width = (width + extendWidth) >> kWidthFractionBits;
		else
			width += extendWidth>>kWidthFractionBits;

		textWidth += width;
		
		long rowBytes = thisGlyph->rowBytes;
		long compWidth = rowBytes;

		if ( isCompressed ) 
		{
			compWidth = rowBytes;
			rowBytes = (rowBytes+1)>>1;
		}
		dstRect.top = y - thisGlyph->ascent;
		dstRect.bottom = dstRect.top + thisGlyph->lines;
		dstRect.left = x - thisGlyph->kern;
		dstRect.right = dstRect.left + (rowBytes << dShift);

		Rectangle dcr = dstRect;
		IntersectRectangle(&dcr,&clipRect);
		
		if ( !EmptyRectangle(&dcr) )
		{
			charRect = dstRect;
			OffsetRectangle(charRect, -dstRect.left, -dstRect.top);
			
			if ( isCompressed ) {
				offset &= 0x0fffffff;
				
				long decodeSize = compWidth * thisGlyph->lines;
				if ( IsError(decodeSize > kDecodeBufferSize) ) {
					Complain(("not enough room to decompress glyph, make the sGlyphDecodeBuffer bigger"));
					return 0;
				} else {
					DecompressGlyph(bitsBase + offset,sGlyphDecodeBuffer,compWidth,thisGlyph->lines,rowBytes);
					MakeBitMapDevice(fontDevice, charRect, sGlyphDecodeBuffer, rowBytes, format, clut, transparentColor);
				}
				if ( !gFilterOnScreenUpdate ) {
					if (  gSystem->GetUseFlickerFilter() )
					{
#ifdef	FILTER_GLYPHS
						FilterGlyph(&fontDevice);
#else
						fontDevice.filter = kFullFilterSlight;
#endif
					}
				}
			}
			else {
				MakeBitMapDevice(fontDevice, charRect, bitsBase + offset, rowBytes, format, clut, transparentColor);
				fontDevice.filter = kFullFilterSlight;
			}
			for ( int i=numPasses-1; i >= 0; i-- ) {
				p = params + i;
				Rectangle dr = dstRect;
				OffsetRectangle(dr,p->offsetX + (extend.left >>kWidthFractionBits) ,p->offsetY + (extend.top >>kWidthFractionBits));
				fontDevice.foregroundColor = p->color;
				CopyImage(fontDevice, device, charRect, dr, p->transparency, clip);
			}
		}
		x += width;
	}
	DrawingComplete(tBounds, clip);
	return textWidth;
}


ulong TextMeasure(BitMapDevice& device, XFont font, CharacterEncoding encoding,const char* text, ulong length)
{
	ulong	width = 0;
	Strike* strike = FontToStrike(font,encoding);
	long c;
	Rectangle extend;
	long	extendWidth = gFontExtendWidth;
	
#ifdef FIDO_INTERCEPT
	if (FidoCompatibilityState::FidoIsOff() == false && gFidoCompatibility)
		return gFidoCompatibility->TextMeasure(device, font, text, length);
#endif

#ifdef CAN_USE_QUICKDRAW
	if (gMacSimulator->GetUseQuickdraw())
		return TextMeasureQD( device,  font,  encoding,text,  length);
#endif															
	if (strike == nil) {
		Complain(("nil strike"));
		return 0;
	}
	if ( device.format )
		;
	if ( GetStyledTextParameters(FontStyle(font),0,0,nil,extend) > 1) {
		extendWidth += (extend.right + extend.left);
	}
	
	while ( length ) {
		c = LookupNextCharacter(strike,text,length,encoding);
		if ( c == -1 )
			continue;
		if (strike->version & kFractionalWidthVersion)
			width += (strike->glyphs[c].width + extendWidth) >> kWidthFractionBits;
		else
			width += strike->glyphs[c].width + (extendWidth >> kWidthFractionBits);
	}
	return width;
}

ulong GetFontAscent(const XFont font,CharacterEncoding encoding)

{
	Strike* strike = FontToStrike(font,encoding);
#ifdef CAN_USE_QUICKDRAW
	if (gMacSimulator->GetUseQuickdraw())
		return GetFontAscentQD( font, encoding);
#endif
	return strike->ascent;
}

ulong GetFontDescent(const XFont font,CharacterEncoding encoding)
{
#ifdef CAN_USE_QUICKDRAW
	if (gMacSimulator->GetUseQuickdraw())
		return GetFontDescentQD( font, encoding);
#endif
	Strike* strike = FontToStrike(font,encoding);
	return strike->descent;
}


#ifdef SIMULATOR
ulong GetFontKern(const XFont font,CharacterEncoding encoding)
{
#ifdef CAN_USE_QUICKDRAW
	if (gMacSimulator->GetUseQuickdraw())
		return 2;
#endif
	Strike* strike = FontToStrike(font,encoding);
	return strike->maxKern;
}
#endif

ulong GetFontLeading(const XFont font,CharacterEncoding encoding)
{
#ifdef CAN_USE_QUICKDRAW
	if (gMacSimulator->GetUseQuickdraw())
		return GetFontLeadingQD( font, encoding);
#endif
	Strike* strike = FontToStrike(font,encoding);
	return strike->leading != 0 ? strike->leading : 1;	// pin at 1 or above
}

	
	
static CLUT* MakeFontColorTable(short depth, Color foreground, Color background)
{

	const kMaxFontDepth	= 8;			// could be 4 to save memory
	
	static uchar FontColorTable1[3*(1<<kMaxFontDepth) + 4];	// should be allocated and purgeable
	static uchar FontColorTable2[3*(1<<kMaxFontDepth) + 4 /*offsetof(CLUT, data)*/];
	
	static Color lastFG1 = 0;				// cache the last 2 tables built
	static Color lastFG2 = 0;
	static Color lastBG1 = 0;
	static Color lastBG2 = 0;
	static CLUT* lastTbl1 = (CLUT*)FontColorTable1;
	static CLUT* lastTbl2 = (CLUT*)FontColorTable2;


	ushort i,j,fRed,fGreen,fBlue,bRed,bGreen,bBlue;
	short  clutSize = (1<<depth)*3;
	
	if (lastFG1 == foreground && lastBG1 == background && clutSize == lastTbl1->size )		// first entry hit?
		return lastTbl1;

	// Since the first one missed, swap the 2 cache entries
	
	Color swap;
	swap = lastFG1; lastFG1 = lastFG2; lastFG2 = swap;
	swap = lastBG1; lastBG1 = lastBG2; lastBG2 = swap;
	CLUT* temp;
	temp = lastTbl1; lastTbl1 = lastTbl2; lastTbl2 = temp;
	
	if (lastFG1 == foreground && lastBG1 == background && clutSize == lastTbl1->size)		// other entry hit?
		return lastTbl1;
		
	// Need to build a new color table
	
	uchar* p = (uchar*)&lastTbl1->data;
	
	PIXEL32TORGB(foreground,fRed,fGreen,fBlue);
	PIXEL32TORGB(background,bRed,bGreen,bBlue);
	
	lastTbl1->version = kRGB24;
	lastTbl1->size = clutSize;
	
	*p++ = bRed;	
	*p++ = bGreen;
	*p++ = bBlue;
	for (i=1, j=(1<<depth)-2; i< (1<<depth)-1; i++, j--)
	{
		*p++ = (i * fRed   + j * bRed)   >> depth;	// should technically be divided by 255 NOT 256
		*p++ = (i * fGreen + j * bGreen) >> depth;
		*p++ = (i * fBlue  + j * bBlue)  >> depth;
	}
	*p++ = fRed;	
	*p++ = fGreen;
	*p++ = fBlue;
	
	lastFG1 = foreground;
	lastBG1 = background;
	return lastTbl1;
}

#define	JISTOSJIS(a,b)	{b += (((a) & 1) ? 31 + ((b) > 95) : 126); a = (((a) + 1) >> 1) + ((a) < 95 ? 112 : 176);}


static ushort
ConvertTwoByteCode(uchar a,uchar b,CharacterEncoding encoding);

static ushort
RemapOneByte(uchar c,CharacterEncoding encoding);

Boolean IsTwoByte(uchar c,CharacterEncoding encoding)
{
	switch ( encoding ) {
	case kSJIS:
	if ( (c >= 0x81 && c < 0xa0) || (c >= 0xe0 && c < 0xf0) ) 
		return true;
	case kEUC:
		if ( (c >= 0xa1 && c <= 0xfe) || c == 0x8e )
			return true;
	}
	return false;
}

Boolean IsThreeByte(uchar c,CharacterEncoding encoding)
{
	switch ( encoding ) {
	case kEUC:
		if ( c == 0x8f )
			return true;
	}
	return false;
}

static ushort
RemapOneByte(uchar c,CharacterEncoding encoding)
{
	switch ( encoding ) {
	case kSJIS:
	if ( c < 0x7f )								// roman ascii range
		return c;
	if ( c >= 0xa1 && c < 0xe0 )				// half width katakana range
		return (c - 0xa1)  + 0x7f;				// in font right after roman ascii
		TrivialMessage(("not a one byte code 0x%02x",c));
		break;
	}
	return c;
}

static ushort
ConvertTwoByteCode(uchar a,uchar b,CharacterEncoding encoding)
{
	switch ( encoding ) {
	case kEUC:
		if ( a == 0x8e )
			return 0x7f + (b-0xa1);
		if ( a == 0x8f ) 			// three byte code
			return 0;
		else {
            a -= 128;
            b -= 128;
            JISTOSJIS(a,b);
        }
	case kSJIS:
		{
			ushort x = (0xe0 - 0xa1)  + 0x7f;		// highest one byte code
			
			if ( a >= 0x81 && a < 0xa0 ) {
				if ( b >= 0x40 && b < 0xfd ) {
					if ( b == 0x7f ) 
						return 0;
					if ( b < 0x7f )
						return x + (b-0x40) + (a-0x81) * ((0xfd-0x40)-1);
					else
						return x + (b-0x40-1) + (a-0x81) * ((0xfd-0x40)-1);
				}
			}
			if ( a >= 0xe0 && a < 0xf0 ) {
				x += ((0xfd-0x40)-1) * (0xa0-0x81);
				if ( b >= 0x40 && b < 0xfd ) {
					if ( b == 0x7f ) 
						return 0;
					if ( b < 0x7f )
						return x + (b-0x40) + (a-0xe0) * ((0xfd-0x40)-1);
					else
						return x + (b-0x40-1) + (a-0xe0) * ((0xfd-0x40)-1);
				}
			}
			TrivialMessage(("not a two byte code 0x%02x",a));
		}
		break;
	}
	return a;
}


const kConversionBufferSize = 512;
static char sJISConversionBuffer[kConversionBufferSize];


typedef enum {
	kNot,
	kMaybe,
	kProbably,
	kDefinitely
} EncodingCheck;

static EncodingCheck
CheckNECEncoding(const char *text,long len);
static EncodingCheck
CheckSJISEncoding(const char *text,long len);

static EncodingCheck
CheckJISEncoding(const char *text,long len);
static EncodingCheck
CheckEUCEncoding(const char *text,long len);



static EncodingCheck
CheckNECEncoding(const char *text,long length)
{
	for (long i=0; i < length; i++ ) {
		if ( *text++ == 0x1b )	{
			if ( ++i < length ) {
				if ( *text++ == 'K' )
					return kDefinitely;
			}
		}
	}
	return kNot;
}



static EncodingCheck
CheckJISEncoding(const char *text,long length)
{
	for (long i=0; i < length; i++ )
	{
		if ( *text++ == 0x1b )	{
			if ( ++i < length ) {
				switch ( *text++ ) 
				{
				case '&':
				case '(':			// )
				case '$':
					return kDefinitely;
				}
			}
		}
	}
	return kNot;
}


static EncodingCheck
CheckSJISEncoding(const char *text,long length)
{
	Boolean maybeSJIS = false;
	Boolean sawFirst = false;
	int		sjisCount = 0;
	
	uchar c;
	for (long i=0; i < length; i++ )
	{
		
		c = *text++;
			
		if ( !sawFirst ) {
			if (  (c >= 0x81 && c < 0xa0) || (c >= 0xe0 && c < 0xf0)  )  {
			sawFirst = true;
			
			// dont count spaces, because japanese authored english docs
			// often have them by mistake since they are invisible 
			
			if ( !(c == 0x81 && *text == 0x40) ) {
				sjisCount++;
				maybeSJIS = true;
			}
			continue;
			} else if ( c >= 0xf0 )	
				return kNot;		// could be a user defined character, but we assume those wont be seen - more likely EUC
		} else {
			if (maybeSJIS ) 
		{
			// check for invalid second SJIS byte
				if ( ( c < 0x40 || c > 0xfc || c == 0x7f ) ) 
					return kNot;
		}
		}
		sawFirst = false;
	}
	if ( maybeSJIS ) {
		if ( sjisCount >= (length>>2) )
			return kProbably;	
	}
	return kMaybe;
}



static EncodingCheck
CheckEUCEncoding(const char *text,long length)
{
	Boolean sawFirst = false;
	Boolean maybeEUC = false;
	int eucCount = 0;
	uchar firstOfSeq = 0;
	
	uchar c;
	for (long i=0; i < length; i++ ) 	{
		c = *text++;
		
		if ( sawFirst == false ) {
			if ( (c >= 0xa1 && c <= 0xfe ) || c == 0x8e || c == 0x8f ) {
				maybeEUC = true;
				sawFirst = true;
				firstOfSeq = c;
				eucCount++;
				continue;
			} else {
				if ( c > 0x7e )
					return kNot;
			}
		}
		else {
			if ( maybeEUC  )  {
				// check for invalid second SJIS byte
				
				switch ( firstOfSeq ) { 
				case 0x8e:
					if ( c < 0xa1 || c > 0xdf )
						return kNot;
					break;
				case 0x8f:
					if ( c < 0xa1 || c > 0xfe )
						return kNot;
					continue;
						break;
				default:
					if ( c < 0xa1 || c > 0xfe )
						return kNot;
					break;
				}
			}
			else {
				if ( c >= 0x7F )
					return kNot;
			}
		}
		sawFirst = false;
	}
	if ( maybeEUC ) {
		if ( eucCount >= length/20 )
			return kProbably;	
	}
	return kMaybe;
}

	
CharacterEncoding 
GuessCharacterEncoding(const char *text,long length)
{
	EncodingCheck	isEUC,isSJIS;
	
		
	if ( CheckNECEncoding(text,length) == kDefinitely )
			return kNEC;
		
	if ( CheckJISEncoding(text,length) == kDefinitely  )
			return kJIS;
		
	isSJIS = CheckSJISEncoding(text,length);
	if ( isSJIS == kDefinitely )
		return kSJIS;
		
	isEUC = CheckEUCEncoding(text,length);
	if ( isEUC == kDefinitely )
		return kEUC;
		
	if ( isEUC == kNot && isSJIS == kNot )
		return kUnknownEncoding;
	
	if ( isSJIS == kProbably && (isEUC == kNot)  )
		return kSJIS;
	if ( isEUC == kProbably && (isSJIS == kNot || isSJIS == kMaybe) )
		return kEUC;
	return kUnknownEncoding;
	
}



Boolean
CanBreakText(const char *s,long length,CharacterEncoding encoding)
{
	switch ( encoding ) {
	case kSJIS:
	case kEUC:
	if ( length ) 
	{
		uchar a = *s++;
		uchar b = *s++;
		if ( length < 2 )
			b = 0;
			if ( IsTwoByte(a,encoding) ) 
		{
			// two-byte characters prohibited from terminating lines

			if ( a == 0x81 ) 
			{
				if ( (b >= 0x65) && (b <= 0x79) && ((b&1) == 1) ) 
					return false; 
				if ( b >= 0x8f &&  b <= 0x92 )
					return false;
				switch ( b) 
				{
				case 0x97:
				case 0x98:
				case 0xa7:
				case 0x94:
				case 0xe1:
					return false;
				}
			}
			if ( length > 2 ) 
			{
				a = *s++;
				b = *s++;
				if ( length < 4 )
					b = 0;
	CheckSecondAsTwoByte:
					if ( IsTwoByte(a,encoding) ) 
				{
					// two-byte characters prohibited from starting lines
					
					if ( a == 0x81 ) 
					{
						if ( b >= 0x40 && b <= 0x50 )
							return false;
						if ( b >= 0x60 && b <= 0x64 )
							return false;
						if ( b >= 0x66 && b <= 0x7a && ((b & 1) == 0 ) )
							return false;
						if ( b >= 0x8b && b <= 0x8f )
							return false;
						if ( b == 0xe2 )
							return false;
					}
					else if ( a == 0x82 ) 
					{
						switch ( b ) 
						{
	
						// hiragana 
						case 0x9f:	// a
						case 0xa0:	// a
						case 0xa1:	// i
						case 0xa2:	// i
						case 0xa3:	// u
						case 0xa4:	// u
						case 0xa5:	// e
						case 0xa6:	// e
						case 0xa7:	// o
						case 0xa8:	// o
	
						case 0xc1:	// tsu	
						case 0xc2:	// tsu	
						case 0xc3:	// dzu
						
						case 0xe1:	// ya
						case 0xe2:	// ya
						case 0xe3:	// yu
						case 0xe4:	// yu
						case 0xe5:	// yo
						case 0xe6:	// yo
						case 0xed:	// wa
							return false;
						}
					}
				}
				else 
				{
					b = a;
					goto StartOneByteCheck;
				}
			}
		} 
		else 
		{
			if ( isspace(a) )
				return true;
		
			// one-byte characters prohibited from terminating lines
			
			switch (a ) 
	{
				
			case '`':
			case '(':
			case '{':
			case '[':
			case '$':
			case '@':
			case '#':
				return false;
			}
			if ( length < 1 ) 
				return true;
				if ( IsTwoByte(b,encoding) ) 
		{
				a = b;
				if ( length < 2 ) 
					return true;
				b = *s++;
				goto CheckSecondAsTwoByte;
			}
	StartOneByteCheck:
			// one-byte characters prohibited from starting lines
			switch (b ) 
			{
			case '.':
			case '!':
			case ',':
			case ':':
			case '"':
			case '\'':
			case ')':
			case '}':
			case ']':
			case '%':
			case '-':
				return false;
			}		
		}
	}
	return true;		
	case kUSASCII:
		return isspace(*s);	
	}
	return false;	
}


long 
CleanJISText(const char **textP,long length,Boolean knownToBeJIS)
{
	const uchar *text = (const uchar *)*textP;
	const uchar *p = text;
	uchar *tempbuffer = nil;


	// All Japanese text must be converted to SJIS format for font lookup
	// Most common formats are JIS or SJIS, this routine converts JIS to SJIS. 
	// (JIS is normal ascii with escape sequences that switch to 2-byte codes )
	// 
	// Other formats are EUC ( extended unix code ) or NEC which are not handled now.
	//
	
	if ( !knownToBeJIS ) {
		switch ( GuessCharacterEncoding(*textP, length) ) {
		case kJIS:
			break;
		case kNEC:
			Message(("NEC format Japanese encoding"));
		return length;
		default:
		return length;
	}
	}

	uchar *opstart,*opend;
	p = text;
	if ( length > kConversionBufferSize/2 ) {
		tempbuffer = (uchar *)AllocateTaggedMemory(length*2,"JIS BUFFER");
		if ( IsError(tempbuffer == nil) )
			return length;;
		opstart = tempbuffer;
		opend = opstart + length*2;
	}
	else {
		opstart = (uchar *)sJISConversionBuffer;
		opend = opstart + kConversionBufferSize;
	}
	
	uchar c,d;
	Boolean inTwoByte = false;
	long l = length;
	uchar *op = opstart;
	while ( l-- > 0) {
		c = *p++;
		switch ( c) {
		case 0x1b:
			if ( l-- == 0 )
				goto endofinput;
			c = *p++;
			switch ( c ) {
			case '&':
				if ( l-- == 0 )
					goto endofinput;
				p++;
				break;
			case '$':
				inTwoByte = true;
				if ( l-- == 0 )
					goto endofinput;
				p++;
				break;
			case '(':			// )
				inTwoByte = false;
				if ( l-- == 0 )
					goto endofinput;
				p++;
				break;
			case 'K':
				inTwoByte = true;
				break;
			default:
				inTwoByte = false;
				break;
			}
			break;
		case 0xd:
		case 0xa:
			if ( inTwoByte )
				inTwoByte = false;
			*op++ = c;
			break;
		case 0xc:
			break;
		default:
			if ( inTwoByte ) {
				if ( IsError(l == 0 ) ) {
					Message(("Split two-byte code"));
					goto done;
				}
				l--;
				d = *p++;
     			JISTOSJIS(c,d);
				*op++ = c;
				*op++ = d;
			}
			else
				*op++ = c;
			break;
		}
		if ( op > opend-5 ) {
			Complain(("conversion buffer overflow"));
			goto done;
		}
	}
endofinput:
	*op = 0;
	length = op - opstart;
	*textP = (const char *)opstart;
done:
	if ( tempbuffer ) {
		FreeTaggedMemory(tempbuffer,"JIS BUFFER");
	}
	return length;
}


void
FilterGlyph(BitMapDevice *dev);


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
FilterGlyph(BitMapDevice *dev)
{
	uchar *lbp,*nbp,*bp = dev->baseAddress;
	long  x,y,w,h;
	long rb = dev->rowBytes;
	
	w = dev->bounds.right-dev->bounds.left;
	h = dev->bounds.bottom-dev->bounds.top;
	
	if ( (dev->format  & 0xf ) == 8 ) {
		for ( y=0; y < h; y++ ) {
			if ( y == 0 )
				lbp = bp;
			else
				lbp = bp - rb;
			if ( y == h-1 )
				nbp = bp;
			else
				nbp = bp + rb;
			for ( x=0; x < w; x++ ) {
				*bp = (*lbp + *nbp + *bp * 6)>>3;
				bp++;
				lbp++;
				nbp++;
			}
			bp += rb - w;
		}
	} else if ( (dev->format  & 0xf ) == 4 ) {
		for ( y=0; y < h; y++ ) {
			if ( y == 0 )
				lbp = bp;
			else
				lbp = bp - rb;
			if ( y == h-1 )
				nbp = bp;
			else
				nbp = bp + rb;
			for ( x=0; x < (w+1)/2; x++ ) {
				uchar pix = *bp;
				uchar lpix = *lbp++;
				uchar npix = *nbp++;
				pix = (((  (lpix & 0xf0) + (npix & 0xf0) + (pix & 0xf0)*6) >> 3 ) & 0xf0)  +
					  (((  (lpix & 0x0f) + (npix & 0x0f) + (pix & 0x0f)*6) >> 3 ) & 0x0f);
				*bp++ = pix;
			}
			bp += rb - w/2;
		}
	}
}
#endif


void
DecompressGlyph(uchar *ibuf,uchar *obuf,short width,short height,short rowBytes)
{
	uchar *bp = ibuf;
	uchar pix;
	uchar runPix;
	uchar runCount;
	short x,y;
	uchar *obp;
	uchar	opix;
	
	uchar fbp = 0;
	runPix = 0xff;
	runCount = 0;
	for ( y=0; y < height; y++ ) {
		obp = obuf;
		for ( x=0; x < width; x++) {
			if ( runCount ) {
				opix = runPix;
				runCount--;
			} else {
				if ( (fbp++ & 1) == 0 )
					pix = *bp >> 4;
				else
					pix = (*bp++ & 0xf);
				if ( pix & 8 ) {
					runCount = (pix & 3) + 2;
					if ( pix & 4 )
						runPix = 15;
					else
						runPix = 0;
					opix = runPix;
					runCount--;
				}
				else {
					opix = ((pix<<1) | (pix>>2));
				}
			}
			opix <<= 4;
			x++;
			if ( x < width ) {
				if ( runCount ) {
					opix |= runPix;
					runCount--;
				} else {
					if ( (fbp++ & 1) == 0 )
						pix = *bp >> 4;
					else
						pix = (*bp++ & 0xf);
					if ( pix & 8 ) {
						runCount = (pix & 3) + 2;
						if ( pix & 4 )
							runPix = 15;
						else
							runPix = 0;
						opix |= runPix;
						runCount--;
					}
					else {
						opix |= ((pix<<1) | (pix>>2));
					}
				}
			}
			*obp++ = opix;
		}
		obuf += rowBytes;
	}
}

static long
LookupNextCharacter(Strike *strike,const char *&text,ulong &length,CharacterEncoding encoding)
{
	long c;
	long last = strike->last;
	c = (uchar)*text++;
	length--;
	if ( strike->version & kTwoByteVersion ) {
		last <<= 8;
		last |= strike->lastLow;
		if ( strike->encoding == 0 )		// backward compatible with old font format
			strike->encoding = kSJIS;		// backward compatible with old font format
			
		if ( IsThreeByte(c,encoding) )  {
			if ( length < 2 )
				return -1;
			text += 2;
			length -= 2;
			return -1;
		}
		if ( IsTwoByte(c,encoding) )  {
			if ( length == 0 )
				return -1;
			length--;
			c = ConvertTwoByteCode(c,*(uchar*)text++,encoding);
		}
		else	
			c = RemapOneByte(c,encoding);
	}
	if (c < strike->first || c > last)						// skip unpopulated characters
		return -1;
	return c - strike->first;
}



static int
GetStyledTextParameters(FontStyle style,Color color,ulong transparency,PaintTextPassParams *params,Rectangle &extend)
{
	Boolean	invert = false;
	long	numPasses;
	ulong	effectTransparency = 0;
	ulong  	effectHalfTransparency = 128;
	PaintTextPassParams dummy[kMaxPaintTextPasses];
	if ( params == nil )
		params = dummy;

	SetRectangle(extend,0,0,0,0);
	
	numPasses = 1;
	params[0].color = color;
	params[0].offsetX = 0;
	params[0].offsetY = 0;
	params[0].transparency = transparency;
		
	// since we dont do underline or italic now, substitute outline and convex
	
	if ( style & kUnderlineStyle )
		style |= kOutlineStyle;
	if ( style & kItalicStyle )
		style |= kReliefStyle;

	style &= ~(kUnderlineStyle + kItalicStyle);

	effectTransparency = transparency;
	effectHalfTransparency = transparency + (kFullyTransparent-transparency)/2;

	switch (style) {
	case kBoldStyle:
		params[1].color = color;
		params[1].offsetX = 1;
		params[1].offsetY = 1;
		params[1].transparency = effectTransparency;
		params[1].antiAlias = true;
		numPasses = 2;
		extend.bottom = (1<<kWidthFractionBits)/2;
		extend.right = (1<<kWidthFractionBits)/2;
		break;
	case kShadowStyle:
		params[1].color = kBlackColor;	
		params[1].offsetX = 1;
		params[1].offsetY = 1;
		params[1].transparency = effectTransparency;
		params[1].antiAlias = true;
		params[0].transparency = 0;
		numPasses = 2;
		extend.bottom = (1<<kWidthFractionBits)/4;
		extend.right = (1<<kWidthFractionBits)/4;
		break;
	case kShadowStyle + kBoldStyle:
		params[2].color = kBlackColor;
		params[2].offsetX = 2;
		params[2].offsetY = 2;
		params[2].transparency = effectTransparency;
		params[2].antiAlias = true;
		params[1].color = kBlackColor;
		params[1].offsetX = 1;
		params[1].offsetY = 1;
		params[1].transparency = effectTransparency;
		params[1].antiAlias = true;
		params[0].transparency = 0;
		numPasses = 3;
		extend.bottom = (3<<kWidthFractionBits)/4;
		extend.right = (3<<kWidthFractionBits)/4;
		break;
		
	case kEmbossStyle:
		invert = true;
	case kReliefStyle:
 		params[2].color = invert ? kBlackColor : kWhiteColor;
		params[2].offsetX = -1;
		params[2].offsetY = -1;
		params[2].transparency = effectTransparency;
		params[2].antiAlias = true;
		params[1].color = invert ? kWhiteColor : kBlackColor;
		params[1].offsetX = 1;
		params[1].offsetY = 1;
		params[1].transparency = effectTransparency;
		params[1].antiAlias = true;
		params[0].antiAlias = true;
		params[0].transparency = 0;
		extend.left = (1<<kWidthFractionBits) / 2;
		extend.top = (1<<kWidthFractionBits) / 2;
		extend.right = (1<<kWidthFractionBits) / 2;
		extend.bottom = (1<<kWidthFractionBits) / 2;
		numPasses = 3;
		break;
	case kEmbossStyle + kBoldStyle:
		invert = true;
	case kReliefStyle + kBoldStyle:
		params[4].color = invert ? kBlackColor : kWhiteColor;
		params[4].offsetX = -2;
		params[4].offsetY = -2;
		params[4].transparency = effectHalfTransparency ;
		params[4].antiAlias = true;
		params[3].color = invert ? kBlackColor : kWhiteColor;
		params[3].offsetX = -1;
		params[3].offsetY = -1;
		params[3].transparency = effectTransparency;
		params[3].antiAlias = true;
		params[2].color = invert ? kWhiteColor : kBlackColor;
		params[2].offsetX = 1;
		params[2].offsetY = 1;
		params[2].transparency = effectTransparency;
		params[2].antiAlias = true;
		params[1].color = invert ? kWhiteColor : kBlackColor;
		params[1].offsetX = 2;
		params[1].offsetY = 2;
		params[1].transparency = effectHalfTransparency;
		params[1].antiAlias = true;
		params[0].antiAlias = true;
		params[0].transparency = 0;
		extend.left = (3<<kWidthFractionBits) / 4;
		extend.top = (3<<kWidthFractionBits) / 4;
		extend.right = (3<<kWidthFractionBits) / 4;
		extend.bottom = (3<<kWidthFractionBits) / 4;
		numPasses = 5;
		break;
	case kOutlineStyle:
		params[4].color = color;
		params[4].offsetX = -1;
		params[4].offsetY = -1;
		params[4].transparency = effectTransparency;
		params[4].antiAlias = true;
		
		params[3].color = color;
		params[3].offsetX = 1;
		params[3].offsetY = -1;
		params[3].transparency = effectTransparency;
		params[3].antiAlias = true;
		
		params[2].color = color;
		params[2].offsetX = 1;
		params[2].offsetY = 1;
		params[2].transparency = effectTransparency;
		params[2].antiAlias = true;
		
		params[1].color = color;
		params[1].offsetX = -1;
		params[1].offsetY = 1;
		params[1].transparency = effectTransparency;
		params[1].antiAlias = true;
		
		params[0].transparency = 0;
		params[0].antiAlias = true;
		params[0].color = gPageBackColor;
		numPasses = 5;
		extend.left = (1<<kWidthFractionBits);
		extend.top = (1<<kWidthFractionBits);
		extend.right = (1<<kWidthFractionBits);
		extend.bottom = (1<<kWidthFractionBits);
		break;
	case kOutlineStyle + kBoldStyle:

		params[7].color = color;
		params[7].offsetX = 2;
		params[7].offsetY = 2;
		params[7].transparency = effectTransparency;
		params[7].antiAlias = true;

		params[6].color = color;
		params[6].offsetX = 1;
		params[6].offsetY = 2;
		params[6].transparency = effectTransparency;
		params[6].antiAlias = true;

		params[5].color = color;
		params[5].offsetX = 2;
		params[5].offsetY = 1;
		params[5].transparency = effectTransparency;
		params[5].antiAlias = true;

		params[4].color = color;
		params[4].offsetX = -1;
		params[4].offsetY = -1;
		params[4].transparency = effectTransparency;
		params[4].antiAlias = true;
		
		params[3].color = color;
		params[3].offsetX = 1;
		params[3].offsetY = -1;
		params[3].transparency = effectTransparency;
		params[3].antiAlias = true;
		
		params[2].color = color;
		params[2].offsetX = 1;
		params[2].offsetY = 1;
		params[2].transparency = effectTransparency;
		params[2].antiAlias = true;
		
		params[1].color = color;
		params[1].offsetX = -1;
		params[1].offsetY = 1;
		params[1].transparency = effectTransparency;
		params[1].antiAlias = true;
		
		params[0].transparency = 0;
		params[0].antiAlias = true;
		params[0].color = gPageBackColor;
		numPasses = 8;
		extend.left = (3<<kWidthFractionBits)/2;
		extend.top = (3<<kWidthFractionBits)/2;
		extend.right = (3<<kWidthFractionBits)/2;
		extend.bottom = (3<<kWidthFractionBits)/2;
		break;
	}
	return numPasses;
}


