// ===========================================================================
//	Graphics.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

// graphics data types

typedef long			Ordinate;
typedef ulong			Color;

typedef ulong			XFont;
typedef	ulong			FontID;
typedef	ulong			FontSize;


typedef long	Gamma;			// fixed point > 0 < 3 * kFractOne 

class 	Region;

enum
{
	kWhiteColor			= 0x00ffffff,
	kBlackColor			= 0x00000000,
	kGrayColor			= 0x007f7f7f,
	kLightGrayColor		= 0x00cfcfcf,
	kRedColor			= 0x00ff0000,
	kBlueColor			= 0x000000ff,
	kPurpleColor		= 0x00aa00ff,
	kGreenColor 		= 0x0000aa00,
	kAmberColor 		= 0x00ffdd33
};

typedef enum // FontFace
{
	kPalatino	= 16,
	kHelvetica 	= 21,
	kMonaco		= 4,
	kCourier	= 22,
	kOsaka		= 54
} FontFace;

enum // FontStyle
{
	kNormalStyle		= 0,
	kBoldStyle 			= 1 << 0,
	kItalicStyle		= 1 << 1,
	kUnderlineStyle		= 1 << 2,
	kOutlineStyle		= 1 << 3,
	kShadowStyle		= 1 << 4,
	kReliefStyle 		= 1 << 5,	
	kEmbossStyle 		= 1 << 6
};

typedef ulong FontStyle;

typedef enum // BitMapFormat
{
	yuvFormat	 		= 0x8020,
	yuv422Format 		= 0x8010,
	yuv422BucherFormat 	= 0xc010,			// obsolete
	gray8Format  		= 0x0808,
	antialias8Format 	= 0x0c08,			// grayscale data, but used for solid alpha blits
	antialias4Format 	= 0x0c04,
	rle4Format	 		= 0x0204,			// FIDO run length format
	vqFormat			= 0x0104,			// 8-bit index, yuv422 2x2codebook 			
	index1Format 		= 0x0001,
	index2Format 		= 0x0002,
	index4Format 		= 0x0004,
	index8Format 		= 0x0008,
	alpha8Format 		= 0x0408,
	rgb16Format	 		= 0x0010,
	rgb32Format	 		= 0x0020
} BitMapFormat;




typedef struct // Coordinate
{
	Ordinate	x;
	Ordinate	y;
} Coordinate;

typedef struct // Rectangle
{
	Ordinate		top;
	Ordinate		left;
	Ordinate		bottom;
	Ordinate		right;
} Rectangle;

typedef enum // CLUTType
{
	kRGB24	= 0,
	kRGB15	= 1,
	kYUV24	= 2,
	kYUV32	= 3,				// [Y U A V] format
	kYUV64	= 4				// [Y U Y V] [Y U Y V] VQ format
} CLUTType;

const kGammaCorrectedCLUT = (1<<8);


typedef enum //	FilterType		// for interlace flicker elimination
{
	kNoFilter  = 0,				// no filtering
	kFullFilter  = 0x11,			// filter entire image to background using 1:2:1 kernel, including first and last lines
	kSliceFilter  = 0x12,			// filter image after draw using 1:2:1 kernel, but dont use background lines off top and bottom
	kTopFilter  = 0x14,			// filter image as in kSliceFilter, but only use background for top line
	kBottomFilter  = 0x18,			// filter image as in kSliceFilter, but only use background for bottom line
	kFullFilterSlight  = 0x21,		// same as kFullFilter but only using 1:6:1 kernel
	kSliceFilterSlight  = 0x22,	// same as kSliceFilterHalf but only using 1:6:1 kernel
	kTopFilterSlight  = 0x24,		// filter image as in kSliceFilterSlight, but only use background for top line
	kBottomFilterSlight  = 0x28,	// filter image as in kSliceFilterSlight, but only use background for bottom line
	
	kCopyFilter = 0x80,				// on DESTINATION device, use filtered copy to do blit
	kCopyFilterOffset = 0x84		// same as kCopyFilter, but since src and dst rects are different dont filter top and bottom
} FilterType;

typedef struct // CLUT
{
	ushort		version;
	ushort		size;
	uchar		data[1];		// data follows
} CLUT;

typedef struct // BitMapDevice
{
	Byte*			baseAddress;
	BitMapFormat	format;
	ushort			rowBytes;
	ushort			depth;
	short			depthShift;
	Rectangle		bounds;
	ulong			transparentColor;
	ulong			foregroundColor;
	CLUT*			colorTable;
	FilterType		filter;
#ifdef FOR_MAC
	PixMapHandle	pixmap;
	GDHandle		gdevice;
	CGrafPort		cport;
	GDHandle		saveDevice;
	CGrafPtr		savePort;
	Rectangle		deviceBounds;
#endif
#if defined FIDO_INTERCEPT
	CLUTType		clutType;
	struct codebook* clutBase;
	BitMapFormat	textureType;
	class FidoTexture* textureBase;
#endif
} BitMapDevice;

typedef struct // BitMapArchive
{
	long			version;
	BitMapFormat	format;
	short			rowBytes;
	Rectangle		bounds;
	ulong			transparentColor;
	ushort			clutSize;
} BitMapArchive;

typedef struct {
	ulong	offset;			// offset to font bitmap
	short	width;			// width of glyph
	short	pad;

	// consider moving this info to the bits, so it doesn't take
	// up space for unused characters
	uchar	rowBytes;		// glyph bitmap rowbytes
	char	kern;			// kerning adjust
	char	ascent;			// top of "black" bits
	uchar	lines;			// bottom
} Glyph;



enum 	//CharacterEncoding
{
	kUnknownEncoding = 0,	// default, assume USASCII
	kUSASCII,	
	kUNICODE,	
					// the four most common Japanese encodings
	kJIS,			// JIS Code ISO-2022-JP ( ??? )
	kSJIS,			// SJIS Code ISO-2022-JP ( ??? )
	kEUC,			// extended unix code
	kNEC,			// NEC Japanese code ( not used ??? )

	kKorean,		// KS 5601-1992
	kChinese,		// GB 2312-80 (Chinese, used in PRC ( Hong Kong??) 
	kTaiwan,		// Big Five ( Chinese, used in Taiwan )

	kISO8859_1,		//  Part 1: Latin alphabet No. 1, ISO 8859-1:1987.
	kISO8859_2,		//  Part 2: Latin alphabet No. 2, ISO 8859-2, 1987.
	kISO8859_3,		//  Part 3: Latin alphabet No. 3, ISO 8859-3, 1988.
	kISO8859_4,		//  Part 4: Latin alphabet No. 4, ISO 8859-4, 1988.
	kISO8859_5,		// 	Part 5: Latin/Cyrillic alphabet, ISO 8859-5, 1988.
	kISO8859_6,		// 	Part 6: Latin/Arabic alphabet, ISO 8859-6, 1987.
	kISO8859_7,		// 	Part 7: Latin/Greek alphabet, ISO 8859-7, 1987.
	kISO8859_8,		// 	Part 8: Latin/Hebrew alphabet, ISO 8859-8, 1988.
	kISO8859_9		//	Part 9: Latin alphabet No. 5, ISO 8859-9, 1990.
};

typedef uchar CharacterEncoding;

// An XFont struct can be a standalone object, hense all
// the info fields.  It can also be an embedded part of a
// TypeFace structure.

typedef struct {
	ushort		version;	// font info
	char		name[32];
	ushort		unused1;
	FontSize	size;
	FontStyle	style;
	uchar		depth;
	uchar		first;
	uchar		last;
	uchar		lastLow;
	short		maxWidth;	// font metrics
	short		maxKern;	// font metrics
	short		ascent;
	short		descent;
	short		leading;
	CharacterEncoding		encoding;
	uchar		unused;
	Glyph		glyphs[1];	// array of glyph info
							// font bitmaps follow
} Strike;

const kFractionalWidthVersion = 1;
const kTwoByteVersion = 2;
const kCompressedVersion = 4;
const kWidthFractionBits = 4;

typedef enum {
	kTransitionNone = 0,
										// blend transitions
	kTransitionBlackFade,
	kTransitionCrossFade,
										// cover transitions ( copy over destination )
	kTransitionWipeLeft,
	kTransitionWipeRight,
	kTransitionWipeUp,
	kTransitionWipeDown,
	kTransitionWipeLeftTop,
	kTransitionWipeRightTop,
	kTransitionWipeLeftBottom,
	kTransitionWipeRightBottom,
	kTransitionSlideLeft,
	kTransitionSlideRight,
	kTransitionSlideUp,
	kTransitionSlideDown,
										// spatial transitions ( destination moves first )
	kTransitionPushLeft,
	kTransitionPushRight,
	kTransitionPushUp,
	kTransitionPushDown,
	
	
	kTransitionZoomInZoomOut,
	kTransitionZoomIn,
	kTransitionZoomOut,
	kTransitionZoomInZoomOutH,
	kTransitionZoomInH,
	kTransitionZoomOutH,
	kTransitionZoomInZoomOutV,
	kTransitionZoomInV,
	kTransitionZoomOutV,
	
	
	kMaxTransitions
} TransitionEffect;




typedef long	Fract;

const	kUVOffset = 128;
const	kTransparentY=0;
const	kBlackY=16;
const	kWhiteY=235;
const	kRangeY = kWhiteY - kBlackY;
const	ulong kTransparentYUYV = (kTransparentY<<24) | (kUVOffset<<16) | (kTransparentY<<8) | kUVOffset;
const	ulong kBlackYUYV = (kBlackY<<24) | (kUVOffset<<16) | (kBlackY<<8) | kUVOffset;

const	kFract		= 16;						// for fixed point math in resizing routines
const	kRound		= (1<<(kFract-1));
const	kFractOne	= (1<<kFract);


#define	FracMul(a,b)	(((a)>>(kFract/2)) * ((b)>>(kFract/2)))
#define	FracDiv(a,b)	(((a)<<(kFract/2)) / ((b)>>(kFract/2)))
#define	FracToInt(a)	((a)>>kFract)
#define	IntToFrac(a)	((a)<<kFract)
#define	FracMan(a)		((a) & ((1<<kFract)-1))		


// transparency parameter for drawing opereations ( affects whole image) ranges from zero ( completely overwrites
// destination to 255 ( almost completely invisible )

const	kFullyTransparent = 255;
const	kHalfTransparent = 128;
const	kNotTransparent = 0;					


// a bitmap can have transparent colors, these are indicated by the transparent color index in 
// indexed bitmap devices, or by Y values of 0 in YUV devices.


#define	kNoTransparentColor 0x00000000			
#define	kTransparent	0x80000000
#define	kTransColorMask	0x00FFFFF


// global variables


extern BitMapDevice gScreenDevice;				// offscreen compositing buffer
extern BitMapDevice gOnScreenDevice;			// actual display buffer
extern Color 		gPageBackColor;				// background color to do cheap antialiasing
extern Boolean		gFilterOnScreenUpdate;		// if set does filter on copy to onscreen bufferinstead of when rendering




/* Rectangle Operations */

void 		SetRectangle(Rectangle&, const Ordinate left, const Ordinate top, const Ordinate right, const Ordinate bottom);
void		OffsetRectangle(Rectangle&, const Ordinate x, const Ordinate y);
void		InsetRectangle(Rectangle&, const Ordinate x, const Ordinate y);
void 		CenterRectangle(Rectangle& r, const Rectangle& inRect);
void 		UnionRectangle(Rectangle* dst, const Rectangle* src);
void 		IntersectRectangle(Rectangle* dst, const Rectangle* src);
Boolean		EqualRectangles(const Rectangle *a,const Rectangle *b);
inline Boolean EmptyRectangle(const Rectangle* r)
	{	return r->left >= r->right || r->top >= r->bottom; }
inline Boolean RectanglesIntersect(const Rectangle* r, const Rectangle* q)
	{	return r->bottom > q->top && r->top < q->bottom && r->right > q->left && r->left < q->right; }
inline Ordinate RectangleWidth(const Rectangle& bounds)
	{	return bounds.right - bounds.left; }
inline Ordinate RectangleHeight(const Rectangle& bounds)
	{	return bounds.bottom - bounds.top; }
inline Boolean RectangleContainedInRectangle(const Rectangle* r, const Rectangle* q)
{
	return r->top >= q->top && r->bottom <= q->bottom && r->left >= q->left && r->right <= q->right;
}


/* Color table and color conversion operations */

#define 	PIXEL32TORGB(pixel, r, g, b)	{(r=(pixel>>16)&0xff); (g=(pixel>>8)&0xff); (b=pixel&0xff);}
void 		yuvtorgb(short y, short u, short v, uchar* r, uchar* g, uchar* b);
void 		rgbtoyuv(uchar r, uchar g, uchar b, short* y, short* u, short* v);
#define 	MakeColor(r,g,b)	(((r)<<16)+((g)<<8)+(b))
#define 	MakeGrayColor(gray)	(((gray)<<16)+((gray)<<8)+(gray))
void		LookUpRGB(uchar index,const CLUT *clut,uchar &r,uchar &g,uchar &b);
void 		MapColor(BitMapFormat format, Color& color);
const uchar* 		GetCLUTYUV(const BitMapDevice* device);
const uchar* 		GetCLUT24(const BitMapDevice* device);

#ifdef	SIMULATOR
const ushort* 	GetCLUT16(const BitMapDevice* device);
#endif
void		DeleteColorTable(CLUT *clut);
CLUT*		NewColorTable(CLUTType type,long numColors);

/* Gamma correction */

uchar*		BuildGammaTable(Gamma gamma, uchar blackLevel = 0, uchar whiteLevel = 255,uchar deviceBlack = 0,uchar deviceWhite = 255);
void		GammaCorrect(CLUT *colorTable, Gamma gamma, uchar blackLevel, uchar whiteLevel);



/* Device Management */

void			InitializeGraphics(void);
ulong			BitMapPixelBufferSize(const BitMapDevice* device);
void			MakeBitMapDevice(BitMapDevice&, const Rectangle&, Byte*,  ulong rowBytes,  BitMapFormat format, CLUT* colorTable, Color transparent);
BitMapDevice*	NewBitMapDevice(const Rectangle& r, BitMapFormat format, CLUT* colorTable = nil, Color transparent = kNoTransparentColor);
void			DeleteBitMapDevice(BitMapDevice* device);
void			ClearBitMapDevice(BitMapDevice* device);
BitMapDevice* 	NewThumbnail(const BitMapDevice* src, short scale);
Color			AverageImage(const BitMapDevice& device, ulong backgroundColor,const Rectangle *rect=nil,Boolean onlyIfSolid=false,Color tolerance=0x040404);
#ifdef INCLUDE_FINALIZE_CODE
void			FinalizeGraphics(void);
#endif


/*	Screen update */

void 	UpdateScreenBits();

#ifdef	FOR_MAC
void		UpdateMacPort();
#endif

#ifndef __REGION_H__
#include "Region.h"
#endif


/* Image drawing */

void	CopyImage(const BitMapDevice& src, BitMapDevice& dst, const Rectangle& srcBounds, const Rectangle& dstBounds, ulong transparency = kNotTransparent, const Rectangle* clip = nil,Boolean flipHorizontal = false,Boolean flipVertical = false,Boolean cheapFast=false);
void	DrawBorderImage(BitMapDevice& device, const BitMapDevice& src, const Region* region,const Rectangle& innerR,
	 ulong transparency = kNotTransparent, const Rectangle* clip=nil ,Boolean stretchNotTile= false,Boolean drawCenter= false, Boolean hasInnerCorners=false);
void	DrawBorderImage(BitMapDevice&, const BitMapDevice& source, const Rectangle& dest, const Rectangle& inner, ulong transparency = kNotTransparent, const Rectangle* clip = nil,Boolean strecthNotTile = false,Boolean drawCenter = false);
void	DrawImage(BitMapDevice&, const BitMapDevice& source, const Rectangle&, ulong transparency = kNotTransparent, const Rectangle* clip = nil, const Rectangle* srcR = nil,Boolean flipHorizonal = false,Boolean flipVertical = false);

/* High Level Drawing Functions	*/

void	FrameRectangle(BitMapDevice& device, const Rectangle& r, ulong width, Color color, ulong transparency = kNotTransparent, const Rectangle* clip = nil);
void 	FrameOvalInRectangle(BitMapDevice& device, const Rectangle& r, ulong width, Color color, ulong transparency = kNotTransparent, const Rectangle* clip = nil);
void 	PaintLine(BitMapDevice&, Ordinate x1, Ordinate y1, Ordinate x2, Ordinate y2, ulong width, Color, ulong transparency = kNotTransparent, const Rectangle* clip = nil);
void 	PaintRectangle(BitMapDevice&, const Rectangle&, Color, ulong transparency = kNotTransparent, const Rectangle* clip = nil);
void 	PaintOvalInRectangle(BitMapDevice& device, const Rectangle& r, ulong width, Color color, ulong transparency, const Rectangle* clip = nil);
void 	FlickerFilterBounds(BitMapDevice& device,const Rectangle& r,FilterType filterType,const Rectangle* clip);

/* low level operations for resize averaging */

void 	ClearYUYVScanLine(ushort *src, long width, Boolean transparent);
void 	AddYUYVScanLine(ushort *src, ushort *dst, long width, Boolean srcHasTransparentPixels,long weight);
void	BlendYUYVScanLine(ushort *src,ushort *dst,long width,Boolean srcHasTransparentPixels,long weight);

/* Silliness */

void 	PaintAntiBevel(BitMapDevice&, const Rectangle& r, const Rectangle* clip, ulong transparency = 160);
void 	PaintBevel(BitMapDevice&, const Rectangle& r, const Rectangle* clip, ulong transparency = 160);
void 	PaintRoundBevel(BitMapDevice&, const Rectangle& r, const Rectangle* clip);
void 	PaintTriangleBevel(BitMapDevice&, const Rectangle& r, const Rectangle* clip);


/* Text rendering */

inline	XFont GetFont(ushort face, uchar size, uchar style)	{ return (((face) << 16) + ((style) << 8) + (size)); }

long 	PaintText(BitMapDevice&, XFont, CharacterEncoding, const char*, ulong length, Color, Ordinate x, Ordinate y, ulong transparency = kNotTransparent, Boolean antiAlias = false, const Rectangle* clip = nil);
ulong 	GetFontAscent(const XFont font,CharacterEncoding encoding);
ulong 	GetFontDescent(const XFont font,CharacterEncoding encoding);
ulong 	GetFontKern(const XFont font,CharacterEncoding encoding);
ulong 	GetFontLeading(const XFont font,CharacterEncoding encoding);
void  	TextBounds(BitMapDevice& device, XFont font, CharacterEncoding,const char* text, ulong length, Ordinate x, Ordinate y, Rectangle* bounds);
ulong 	TextMeasure(BitMapDevice&, XFont,CharacterEncoding, const char*, ulong length);
CharacterEncoding 	GuessCharacterEncoding(const char *text,long length);
Boolean				CanBreakText(const char *s,long length, CharacterEncoding encoding);
Boolean 			IsTwoByte(uchar c,CharacterEncoding encoding);
Boolean 			IsThreeByte(uchar c,CharacterEncoding encoding);
long 				CleanJISText(const char **textP,long length,Boolean knownToBeJIS);



void
GraphicsTest();

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Graphics.h multiple times"
	#endif
#endif /* __GRAPHICS_H__ */