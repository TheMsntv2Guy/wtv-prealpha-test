

#ifndef	__XGRAPHICS__
#define	__XGRAPHICS__


#ifdef FOR_MAC
#define CAN_USE_QUICKDRAW 1
#endif


#include "Headers.h"

#ifdef SIMULATOR
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
#endif

#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif
#ifndef __HW_DISPLAY__
#include "HWDisplay.h"
#endif
#ifndef __OPTIONSPANEL_H__
#include "OptionsPanel.h"
#endif
#ifndef __BOXABSOLUTEGLOBALS_H__
#include "BoxAbsoluteGlobals.h"
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif
#ifndef __HW_DISPLAY__
#include "HWDisplay.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __OBJECTSTORE_H__
#include "ObjectStore.h"
#endif
#ifndef __OPTIONSPANEL_H__
#include "OptionsPanel.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __SCREEN_H__
#include "Screen.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __PPP_H__
#include "ppp.h"
#endif
#ifdef HARDWARE
#include "BoxUtils.h"
#endif

#ifdef FIDO_INTERCEPT
	#include "fidoBitmap.h"
	#include "fidoCompatibility.h"
#endif

#include	"VectorQuantization.h"

//#define	TRANS_UV_BLEND		// blend u and v of yuv422 50% for transparent pixels
							// that fall on a half YUYV boundary

#define 	kLowBitsMask8	((1<<24)|(1<<16)|(1<<8)|(1<<0))
#define 	kLowBitsMask5	((1<<26)|(1<<21)|(1<<16)|(1<<10)|(1<<5)|(1<<0))



const long			kBitMapMemoryPostflightSize = 32*1024L;
const long			kMinimumRegionBorderOffset = 5;

extern Color		gScreenBorderColor;





// ===========================================================================

struct BlitParams
	{
	Color			color;
	Rectangle			r;
	BitMapDevice*	device;
	ulong*			base;			// long aligned
	long			bump;
	long			longCount;
	long			leftBits;
	long			rightBits;
	long			leftShift;
	long			rightShift;
	ulong			leftMask;
	ulong			rightMask;
	long			xInc;	
	};

struct BlitParams2
	{
	BlitParams		src;
	BlitParams		dst;
	long			srcWidth;
	long			srcHeight;
	long			dstWidth;
	long			dstHeight;
	long			topSkipSrc;
	long			topSkipDst;
	long			leftSkipSrc;
	long			leftSkipDst;
	Boolean			cheapFast;
	};

typedef void (*BlitProcPtr)(BlitParams2& p, ulong transparency);

// ===========================================================================

Boolean BoundsSetup(BlitParams& p);
void BlitSetup(BlitParams& p);
void PaintBlit(BlitParams& p);
void PaintBlitBlendYUV(BlitParams& p, ulong transparency);
void PaintBlitBlend32(BlitParams& p, ulong transparency);
void PaintBlitBlend16(BlitParams& p, ulong transparency, Color color);
void PaintBlitBlendYUYV(BlitParams& p, ulong transparency);
void PaintBlitBlendGray8(BlitParams& p, ulong transparency, Color color);
void CopyBlitFast(BlitParams2& p, ulong transparency);
void ScaleBlitYUVtoYUYV(BlitParams2& p, ulong transparency);
void ScaleBlitYUYVtoYUYV(BlitParams2& p, ulong transparency);
void ScaleBlit4toYUYV(BlitParams2& p, ulong transparency);
void ScaleBlitVQtoYUYV(BlitParams2& p, ulong transparency);
void ScaleBlit8toYUYV(BlitParams2& p, ulong transparency);
void ScaleBlitAlpha8toYUYV(BlitParams2& p, ulong transparency);
void ScaleBlitAntiAlias8toYUYV(BlitParams2& p, ulong transparency);
void ScaleBlitAntiAlias4toYUYV(BlitParams2& p, ulong transparency);
void ScaleBlit16toYUYV(BlitParams2& p, ulong transparency);
void ScaleBlit32toYUYV(BlitParams2& p, ulong transparency);
void ScaleBlit4to16(BlitParams2& p, ulong transparency);
void ScaleBlit8to16(BlitParams2& p, ulong transparency);
void ScaleBlit16to16(BlitParams2& p, ulong transparency);
void ResizeBlit(BlitParams2& p,ulong transparency);
void InterlaceFilterYUYV(BlitParams& p,FilterType filterType);
void	NewInterlaceFilterYUYV(BlitParams& p, FilterType filterType);


BlitProcPtr LookupBlitProc(const BitMapDevice *srcDevice,const BitMapDevice *dstDevice,ulong transparency);

// ===========================================================================





const 	kScaleShift=9;

#define	SCALE_ORD(_x,_n,_d)	((((((_n) * (_x)) << kScaleShift)) / (_d)+ ( 1<<(kScaleShift-1))) >> kScaleShift)


#define PIXEL16TORGB(pixel, r, g, b)	{(r=(pixel>>7)&0xf8); (g=(pixel>>2)&0xf8); (b=(pixel<<3)&0xf8);}
#define RGBTOPIXEL16(r, g, b, pixel)	{(pixel = ((r & 0xf8) << 7) + ((g & 0xf8) << 2) + ((b & 0xf8) >> 3));}
#define	RGBTOPIXEL32(r,g,b,pixel)		{(pixel = ((ulong)r<<16) + ((ulong)g<<8) + (b));}
#define	PIXEL32TOYUV(pixel,y,u,v)		{(y=(pixel>>24)&0xff); (u=(pixel>>16)&0xff); (v=pixel&0xff);}
#define	PIXEL32TOYUVA(pixel,y,u,v,a)		{(y=(pixel>>24)&0xff); (u=(pixel>>16)&0xff); (a=(pixel>>8)&0xff); (v=pixel&0xff);}
#define	YUVTOPIXEL32(y,u,v,pixel)		{(pixel = ((ulong)y<<24) + ((ulong)u<<16) + (v));}


#define	PIXELS16TOYUYV(pixel,y1,u,y2,v)		{y1=(pixel>>24)&0xff; u=(pixel>>16)&0xff; y2=(pixel>>8)&0xff; v=pixel&0xff;}
#define	YUYVTOPIXELS16(y1,u,y2,v,pixel)		{(pixel = ((ulong)y1<<24) + ((ulong)u<<16) + ((ulong)y2<<8) + (v));}

#ifdef	NON_RISC
	#define	CLIPBYTE(_x)				((_x) < 0 ? 0 : ( (_x) > 255 ? 255 : (_x)) )	
#else
	#define	CLIPBYTE(_x)				(((_x) & (~((_x) >> 9))) | (((ushort)(0xff - (_x))) >> 8))
#endif

#define	CLIPLUMINANCE(_x)				((_x) < kBlackY ? kBlackY: ( (_x) > kWhiteY ? kWhiteY : (_x)) )			// should be 16 to 235
#define	CLIPCHROMA(_x)					(_x)

// note Y=255 is transparent case for YUYV so it is reserved


#define	PREPUV(_u,_v,_uvg)													\
		{(_u) -= kUVOffset; (_u) = (((short)455 * (short)(_u))+ (1<<7)) >> 8;			\
		(_v) -= kUVOffset; (_v) = (((short)358 * (short)(_v))+ (1<<7)) >> 8;			\
		(_uvg) = (((short)50 * (short)(_u) + (short)131 * (short)(_v))+ (1<<7)) >> 8;	\
		}

#define	RGBTOYUV(_r,_g,_b,_y,_u,_v)																		\
		{(_y) = ((((short)77*(short)(_r) + (short)150*(short)(_g) + (short)29*(short)(_b) + (1<<7))) >> 8);	\
		(_u) = kUVOffset + ((((short)144 * (short)((_b) - (_y)) )+ (1<<7))>>8);									\
		if ( (_u) < kBlackY ) (_u) = kBlackY; 															\
		if ( (_u) > kWhiteY ) (_u) = kWhiteY;															\
		(_v) = kUVOffset + ((((short)183 * (short)((_r) - (_y)) )+ (1<<7)) >> 8);									\
		if ( (_v) < kBlackY ) (_v) = kBlackY;															\
		if ( (_v) > kWhiteY ) (_v) = kWhiteY;															\
		(_y) = kBlackY + ((((_y) * kRangeY) + (1<<7)) >> 8);														\
		if ( (_y) > kWhiteY ) (_y) = kWhiteY; 															\
		if ( (_y) < kBlackY ) (_y) = kBlackY; 															\
		}


#define YUVPTORGB(_y,_u,_v,_uvg,_r,_g,_b)	\
				{register short t;			\
				(_y) -= kBlackY;			\
				(_y) = ((((_y)<<8)+(1<<7)) / kRangeY );	\
				t = (_y) + (_v);			\
				(_r) = CLIPBYTE(t);			\
				t = (_y) - (_uvg);			\
				(_g) = CLIPBYTE(t);			\
				t = (_y) + (_u);			\
				(_b) = CLIPBYTE(t);			\
				}
#ifdef	DEBUG

#define DEBUGYUVPTORGB(_y,_u,_v,_uvg,_r,_g,_b)	\
				{register short t;			\
				if ( (_y) < kBlackY ) { (_r) = 0xff; (_g) = 0; (_b) = 0; } \
				else if ( (_y) > kWhiteY ) { (_r) = 0; (_g) = 0xff; (_b) = 0; } \
				else {						\
				(_y) -= kBlackY;			\
				(_y) = ((((_y)<<8)+(1<<7)) / kRangeY );	\
				t = (_y) + (_v);			\
				(_r) = CLIPBYTE(t);			\
				t = (_y) - (_uvg);			\
				(_g) = CLIPBYTE(t);			\
				t = (_y) + (_u);			\
				(_b) = CLIPBYTE(t);			\
				}}
#else

#define DEBUGYUVPTORGB	YUVPTORGB

#endif

#define YUVTORGB(_y,_u,_v,_r,_g,_b)			\
				{register short t,uvg;		\
				(_y) -= kBlackY;			\
				(_y) = ((((_y)<<8)+(1<<7)) / kRangeY );	\
				PREPUV((_u),(_v),uvg);		\
				t = (_y) + (_v);			\
				(_r) = CLIPBYTE(t);			\
				t = (_y) - uvg;				\
				(_g) = CLIPBYTE(t);			\
				t = (_y) + (_u);			\
				(_b) = CLIPBYTE(t);			\
				}



// ===========================================================================


// per image transparency
// zero is fully opaque, 255 is almost fully transparent


inline uchar
TransparencyBlend(uchar a,uchar src,uchar dst)	
{
	if ( a )
		return (((uchar)(256-a) * (uchar)src + (uchar)a*(uchar)dst)>>8);	
	return src;
}

#if	0

inline uchar
TransparencyBlendY(uchar a,uchar src,uchar dst)	
{
	if ( a )
		return (((uchar)(256-a) * (uchar)(src) + (uchar)a*(uchar)(dst))>>8);	
	return src;
}

#else

#define	TransparencyBlendY	TransparencyBlend

#endif


const	kAlphaOpaque = 255;
const	kAlphaTransparent = 0;



// per pixel alpha channel
// zero is completely transparent, 255 is completely opaque
inline uchar
AlphaBlend(uchar a,uchar src,uchar dst)	
{
	if ( a == kAlphaOpaque )
		return src;
	if ( a == kAlphaTransparent )
		return dst;
	dst = ((uchar)a * (short)(src+1) + (uchar)(kAlphaOpaque-a)*(short)(dst+1))>>8;		// should be divide by (kAlphaOpaque-kAlphaTransparent)
	return CLIPBYTE(dst);
}


extern Rectangle gScreenDirtyRectangle;

#define	FilterBytes( a, b, c)			(((a)+(c) +((b)<<1))>>2)
#define	SFilterBytes( a, b, c)			(((a)+(c) +((b)*6))>>3)
//#define	DONT_FILTER_CHROMA

#ifdef	DONT_FILTER_CHROMA
#define	FilterChromaBytes( a, b, c)			(b)
#define	SFilterChromaBytes( a, b, c)		(b)
#define	FilterLongs(a,b,c)		(b & 0x00ff00ff) |					 		\
			(~0x00ff00ff & 				 		\
			((((((a>>24)&0xff) + ((c>>24)&0xff)) + (((b>>24)&0xff)<<1)) << (24-2)) + 				 		\
			((((a&0xff00) + (c&0xff00)) + ((b&0xff00)<<1)) >> 2))				 		\
			)
#define	SFilterLongs(a,b,c)		(b & 0x00ff00ff) |					 		\
			(~0x00ff00ff & 				 		\
			((((((a>>24)&0xff) + ((c>>24)&0xff)) + (((b>>24)&0xff)*6)) << (24-3)) + 				 		\
			((((a&0xff00) + (c&0xff00)) + ((b&0xff00)*6)) >> 3))				 		\
			)
#else

#define	FilterLongs		FFilterLongs
#define	SFilterLongs	FSFilterLongs

#define	FilterChromaBytes		FilterBytes
#define	SFilterChromaBytes		SFilterBytes

#endif


extern volatile ulong gSystemTicks;

const long		kInitialScrollDelta = 1;

#ifndef FOR_MAC
const long		kScrollAcceleration = 4;
const long		kMaxScrollDelta = 48;
#else
const long		kScrollAcceleration = 1;
const long		kMaxScrollDelta = 32;
#endif


void DrawingComplete(const Rectangle& dirtyRect, const Rectangle* clip);
void XPaintRect(const BitMapDevice& device, const Rectangle& r, Color color, ulong transparency, const Rectangle* clip);
void XFilterBounds(const BitMapDevice& device, const Rectangle& r,FilterType filterType,const Rectangle* clip);


extern const signed char kDepthToShiftTable[33];

// ===========================================================================



#ifdef	SIMULATOR

void ScaleBlitAntiAlias8to32(BlitParams2& p, ulong transparency);
void ScaleBlitAntiAlias8to16(BlitParams2& p, ulong transparency);
void ScaleBlitAntiAlias4to16(BlitParams2& p, ulong transparency);
void InterlaceFilterRGB32(BlitParams& p, FilterType filterType);
void ScaleBlit8to32(BlitParams2& p, ulong transparency);
void ScaleBlit32toGray8(BlitParams2& p, ulong transparency);
void ScaleBlit8toGray8(BlitParams2& p, ulong transparency);
void ScaleBlitAlpha8to16(BlitParams2& p, ulong transparency);
void ScaleBlitGray8toGray8(BlitParams2& p, ulong transparency);
void ScaleBlitYUVtoYUV(BlitParams2& p, ulong transparency);
void ScaleBlitYUVto16(BlitParams2& p, ulong transparency);
void ScaleBlit8toYUV(BlitParams2& p, ulong transparency);
void ScaleBlitAntiAlias8toYUV(BlitParams2& p, ulong transparency);
void ScaleBlitYUVto32(BlitParams2& p, ulong transparency);
void ScaleBlit32to32(BlitParams2& p, ulong transparency);
void ScaleBlit32to16(BlitParams2& p, ulong transparency);
void ScaleBlit16to32(BlitParams2& p, ulong transparency);
void ScaleBlitYUYVto32(BlitParams2& p, ulong transparency);
void ScaleBlitYUYVtoYUV(BlitParams2& p, ulong transparency);
void ScaleBlitYUYVto16(BlitParams2& p, ulong transparency);
void FilterBlitYUYVto32(BlitParams2& p, ulong transparency);
void FilterBlitYUYVto16(BlitParams2& p, ulong transparency);

void
ScaleBlitVQto16(BlitParams2& p, ulong transparency);

void SetRectangle(Rectangle& r, const Rect& mac);
void MacRect(const Rectangle& r, Rect& mac);



void 	CopyImageQD(const BitMapDevice& srcDevice, BitMapDevice& dstDevice,
				const Rectangle& srcRect, const Rectangle& dstRect, ulong transparency = 0, const Rectangle* clip = nil);
void 	FrameRectangleQD(BitMapDevice& device, const Rectangle& r, Color color, ulong transparency = 0, const Rectangle* clip = nil);
void 	PaintRectangleQD(BitMapDevice& device, const Rectangle& r, Color color, ulong transparency = 0, const Rectangle* clip= nil);
void 	FastTextBounds(BitMapDevice& device, XFont font,CharacterEncoding encoding,const char* text, ulong length, Ordinate x, Ordinate y, Rectangle* bounds);
ulong 	TextMeasureQD(BitMapDevice& device, XFont font, CharacterEncoding encoding,const char* text, ulong length);
ulong 	GetFontLeadingQD(const XFont font,CharacterEncoding encoding);
ulong 	GetFontDescentQD(const XFont font,CharacterEncoding encoding);
ulong 	GetFontAscentQD(const XFont font,CharacterEncoding encoding);
long 	PaintTextQD(BitMapDevice& device, XFont font,CharacterEncoding encoding,const char* text, ulong length, Color color, Ordinate x, Ordinate y, ulong transparency, Boolean antiAlias, const Rectangle* clip);
void 	ScrollBoundsToMacPort(GrafPtr port, const Rectangle& srcBounds, short scrollDirection, Boolean scrollOriginal);
void	UpdateMacScreen(Rectangle dirty,const Rectangle *clip = nil);
void	MakeBitMapDevice(BitMapDevice&, CGrafPtr port);


#endif
void
InitializeSimulatorGraphics(void);

#endif
