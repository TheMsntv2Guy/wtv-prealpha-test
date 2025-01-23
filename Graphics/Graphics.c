// ===========================================================================
//	Graphics.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include	"GraphicsPrivate.h"

#ifdef FOR_MAC
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
#endif

#include	 "Animation.h"

// ===========================================================================
//	globals
// ===========================================================================

const Color kOptionsPanelColor = 0x378080;
const Color kBlackBorderColor = 0x108080;

BitMapDevice gScreenDevice;				// offscreen composition buffer

BitMapDevice gOnScreenDevice;			// actual displayed frame buffer


Region	*gScreenDirtyRegion = nil;

const Color kOptionsGrayBlend = 0x00808080;

Boolean gFilterOnScreenUpdate = true;

Color gScreenBorderColor = (kBlackY<<16) + (kUVOffset<<8) + kUVOffset;

static void	DrawScreenBorder();

static void	FilterOptionsPanel();

static void	DoCopyImage(const BitMapDevice& src, BitMapDevice& dst, const Rectangle& srcBounds, 
	const Rectangle& dstBounds, ulong transparency = kNotTransparent, const Rectangle* clip = nil,
	Boolean flipHorizontal = false,Boolean flipVertical = false,Boolean cheapFast=false);

static void
SetScreenBorderColor(Color color);


static Color
BlendColor(Color e,Color f,uchar weight = 128);

// ===========================================================================
//	Initialization
// ===========================================================================


void InitializeGraphics(void)
{
	Rectangle		screenBounds;
	gScreenDirtyRegion = new(Region);

	SetRectangle(screenBounds,0,0,GetDisplayWidth(),GetDisplayHeight());
#ifdef HARDWARE

	MakeBitMapDevice(gScreenDevice, screenBounds,(uchar*)GetDisplayPageBase(1),GetDisplayRowBytes(), yuv422Format, nil, kNoTransparentColor);

#ifdef HW_INTERLACING
	if(SPOTVersion() == kSPOT3)
#else
	if(0)
#endif
	{	// elegant clean SPOT3 normal frame buffer

		MakeBitMapDevice(gOnScreenDevice, screenBounds, 
				(uchar*)GetDisplayPageBase(0),
				GetDisplayRowBytes(), 
				yuv422Format, nil, kNoTransparentColor);

	} 
#ifdef SPOT1
	else {
		// fucked up nasty perverted SPOT1 odd and even segregated frame buffer
	
		MakeBitMapDevice(gOnScreenDevice, screenBounds, 
				(uchar*)GetDisplayPageBase(0),
				GetDisplayRowBytes(), 
				yuv422BucherFormat, nil, kNoTransparentColor);
	}
#endif

	
	SetScreenBorderColor(kBlackBorderColor);

#if	0
	/* hack test */
	
	{
		uchar r,g,b;
		short y,u,v;
		
		
		r = 255;
		g = 255;
		b = 0;
		RGBTOYUV(r,g,b,y,u,v);
		
		SetDisplayOverscanColor((CLIPLUMINANCE(y)<<16) + (CLIPCHROMA(u)<<8) + CLIPCHROMA(v),0);
		
		if ( 0 ) {
			r = 0;
			g = 0;
			b = 255;
			RGBTOYUV(r,g,b,y,u,v);
			
			SetDisplayOverscanColor((CLIPLUMINANCE(y)<<16) + (CLIPCHROMA(u)<<8) + CLIPCHROMA(v),250);
		}
	}
	/* end hack */

#endif


#else


#ifdef FOR_MAC
	gMacSimulator->InitializeScreenRAM();
#endif
	InitializeSimulatorGraphics();

#endif
	ClearBitMapDevice(&gScreenDevice);
}

#ifdef INCLUDE_FINALIZE_CODE
void FinalizeGraphics(void)
{
#ifdef FOR_MAC
//	UnMakeMacDevice(gOnScreenDevice);
//	UnMakeMacDevice(gScreenDevice);
	gMacSimulator->FinalizeScreenRAM();
#endif
}
#endif /* INCLUDE_FINALIZE_CODE */





const signed char kDepthToShiftTable[33] = {-1, 0, 1,-1, 2,-1,-1,-1, 3,-1,-1,-1,-1,-1,-1,-1,4,
										-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5};

void MakeBitMapDevice(BitMapDevice& device, const Rectangle& r, Byte* base,  ulong rowBytes, BitMapFormat format, CLUT* colorTable, Color transparent)
{
	device.bounds = r;
	device.baseAddress = base;
	device.rowBytes = (short)rowBytes;
	device.format = format;
	device.depth = format & 0xff;
	device.depthShift = kDepthToShiftTable[device.depth];
	device.transparentColor = transparent;		// high bit set for transparent colors !!
	device.colorTable = colorTable;
	device.filter = kNoFilter;
	if ( IsError(device.depthShift == -1 ) ) {
		Message(("bad bitmap depth"));
	}
#ifdef FOR_MAC
	device.deviceBounds = r;
#endif
#if defined FIDO_INTERCEPT
	device.clutType = (CLUTType) -1;
	device.clutBase = 0;
	device.textureType = (BitMapFormat) -1;
	device.textureBase = 0;
#endif
}

void
DeleteColorTable(CLUT *clut)
{
	if ( clut )
		FreeTaggedMemory(clut,"Color Table");
}

CLUT *
NewColorTable(CLUTType type,long numColors)
{
	long nc;
	CLUT *colorTable = nil;
	
	switch ( type ) {
	case kRGB24:
		nc = 3;
		break;
	case kRGB15:
		nc = 2;
		break;
	case kYUV24:
		nc = 3;
		break;
	case kYUV32:
		nc = 4;
		break;
	case kYUV64:
		nc = 8;
		break;
	default:
		Complain(("invalid color table format"));
		return nil;
	}
	colorTable = (CLUT*)AllocateTaggedZero(numColors * nc + 4 /*offsetof(CLUT,data)*/, "Color Table");
	if ( colorTable ) {
		colorTable->version = type;
		colorTable->size = nc * numColors;
	}
	return colorTable;
}


ulong
BitMapPixelBufferSize(const BitMapDevice* device)
{
	long height = device->bounds.bottom - device->bounds.top;
	
	if ( device->format == vqFormat )		// pixels are 2x2
		height = (height+1)>>1;	
	return device->rowBytes * height;

}

BitMapDevice* NewBitMapDevice(const Rectangle& r, BitMapFormat format,CLUT* colorTable, Color transparent)
{
	BitMapDevice* device;

	if ( IsError(EmptyRectangle(&r)) ) 
		return nil;
	
	device = (BitMapDevice*)AllocateTaggedMemory(sizeof(BitMapDevice), "BitMapDevice");
	if ( device == nil )
		return nil;
	device->bounds = r;
#ifdef FOR_MAC
	device->deviceBounds = r;
#endif
	device->format = format;
	device->depth = ((short)format) & 0xff;
	device->depthShift = kDepthToShiftTable[device->depth];
	device->transparentColor = transparent;		// high bit set for transparent colors !!
	device->filter = kNoFilter;
	if ( IsError(device->depthShift == -1 ) ) {
		FreeTaggedMemory(device, "BitMapDevice");
		Message(("bad bitmap depth"));
		return nil;
	}
	device->rowBytes = (short)(((r.right - r.left) * device->depth + 31) >> 5) << 2; // Round up to long boundary
	device->baseAddress = (Byte*)AllocateTaggedMemoryNilAllowed(BitMapPixelBufferSize(device), "Pixels");
	
	// Postflight available memory to make sure there is enough to continue.
	if (device->baseAddress != nil) {
		void*	postFlight = AllocateTaggedMemoryNilAllowed(kBitMapMemoryPostflightSize, "NewBitMapDevice");
		if (postFlight == nil) {
			FreeTaggedMemory(device->baseAddress, "Pixels");
			device->baseAddress = nil;
		}
		else
			FreeTaggedMemory(postFlight, "NewBitMapDevice");
	}
	
	device->colorTable = colorTable;
	if (device->baseAddress == nil) {
		FreeTaggedMemory(device, 0);
		device = nil;
	}
#if defined FIDO_INTERCEPT
	device->clutType = (CLUTType) -1;
	device->clutBase = 0;
	device->textureType = (BitMapFormat) -1;
	device->textureBase = 0;
#endif
	return device;
}

void DeleteBitMapDevice(BitMapDevice* device)
{
	if(device) {
		FreeTaggedMemory(device->baseAddress, "Pixels");
		if (device->colorTable)
			DeleteColorTable(device->colorTable);	// we didn't allocate, but we're tossing it anyway.
		FreeTaggedMemory(device, "BitMapDevice");
	#if defined FIDO_INTERCEPT
		if (device->clutType != (CLUTType) -1 && device->clutBase && device->clutBase != (struct codebook*) device->colorTable) {
			Assert(device->clutType >= kYUV32 && device->clutType <= kYUV64);
			FreeArray(device->clutBase, codebook);
			device->clutBase = 0;
		}
		if (device->textureType != (BitMapFormat) -1 && device->textureBase && device->textureBase != (class FidoTexture*) device->baseAddress) {
	//		Assert(device->textureType >= kYAUV_4 && device->textureType <= kYUAV_32);
			FreeArray(device->textureBase, texture);
			device->textureBase = 0;
		}
	#endif
	} else {
		Message(("DeleteBitMapDevice: someone attempting to delete nil BitMapDevice"));
	}
}

void
ClearBitMapDevice(BitMapDevice* device)
{	
	Color black = kBlackColor;
	if ( device ) {
		if ( device->format != vqFormat )
			MapColor(device->format,black);
		if ( black == 0  )
			ZeroMemory(device->baseAddress,BitMapPixelBufferSize(device));
		else {
			int x,y;
			ulong *bp = (ulong *)device->baseAddress;
			
			for ( y=0;y < device->bounds.bottom-device->bounds.top; y++ ) {
				for ( x=0; x < device->rowBytes>>2; x++ ) {
					*bp++ = black;
				}
			}
		}
	}
}


// ===========================================================================
//	Rectangle calculations
// ===========================================================================

void SetRectangle(Rectangle& r, const Ordinate x1, const Ordinate y1, const Ordinate x2, const Ordinate y2)
{
	r.left = x1;
	r.top = y1;
	r.right = x2;
	r.bottom = y2;
}

void OffsetRectangle(Rectangle& r, const Ordinate x, const Ordinate y)
{
	r.left += x;
	r.right += x;
	r.top += y;
	r.bottom += y;
}

void InsetRectangle(Rectangle& r, const Ordinate x, const Ordinate y)
{
	r.left += x;
	r.right -= x;
	r.top += y;
	r.bottom -= y;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void CenterRectangle(Rectangle& r, const Rectangle& inRect)
{
	Ordinate width = r.right-r.left;
	Ordinate height = r.bottom-r.top;
	Ordinate dH = ((inRect.right-inRect.left) - width) / 2;
	Ordinate dV = ((inRect.bottom-inRect.top) - height) / 2;
	r.left = inRect.left + dH; r.right = r.left + width;
	r.top = inRect.top + dV; r.bottom = r.top + height;
}
#endif



Boolean EqualRectangles(const Rectangle *a,const Rectangle *b)
{
	if ( a->left != b->left )
		return false;
	if ( a->top != b->top )
		return false;
	if ( a->right != b->right )
		return false;
	if ( a->bottom != b->bottom )
		return false;
	return true;
}

void UnionRectangle(Rectangle* dst, const Rectangle* src)
{
	if (dst->left == dst->right && dst->top == dst->bottom)
		*dst = *src;
	else {
		if ( !EmptyRectangle(src) ) {
			if (src->left < dst->left)
				dst->left = src->left;
			if (src->right > dst->right)
				dst->right = src->right;
			if (src->top < dst->top)
				dst->top = src->top;
			if (src->bottom > dst->bottom)
				dst->bottom = src->bottom;
		}
	}
}

void IntersectRectangle(Rectangle* dst, const Rectangle* src)
{
	if (src == nil)
		return;		// allow nil for 2nd argument
		
	if (src->right < dst->right)
		dst->right = src->right;
	if (src->left > dst->left)
		dst->left = src->left;
	
	if (src->top > dst->top)
		dst->top = src->top;
	if (src->bottom < dst->bottom)
		dst->bottom = src->bottom;
		
	if (dst->left >= dst->right || dst->top >= dst->bottom)
		dst->left = dst->right = dst->top = dst->bottom = 0;
}



// ===========================================================================
//	Color conversions
// ===========================================================================


void
yuvtorgb(short y,short u,short v,uchar *r,uchar * g,uchar * b)
{
	uchar tempr, tempg, tempb;
 	YUVTORGB(y, u, v, tempr, tempg, tempb);
 	*r = tempr;
 	*g = tempg;
 	*b = tempb;
}

void
rgbtoyuv(uchar r,uchar g,uchar b,short *y,short *u,short *v)
{
	short tempy, tempu, tempv;
 	RGBTOYUV(r, g, b, tempy, tempu, tempv);
 	*y = tempy;
 	*u = tempu;
 	*v = tempv;
}


void
MapColor(BitMapFormat format, Color& color)
{
	uchar r,g,b;
	short y, u, v;
	
	switch (format)
	{
	case rgb32Format:
		color = color & 0xffffff;
		break;
	case rgb16Format:
		color = ((color >> 9) & 0x7c00) + ((color >> 6) & 0x03e0) + ((color >> 3) & 0x001f);
		color |= color << 16;
		break;
	case gray8Format:
		color = (((color >> 16) & 0xff) * 5 + ((color >> 8) & 0xff) * 9 + (color & 0xff) * 2) >> 4;
		color |= color << 8;
		color |= color << 16;
		break;
#ifdef SPOT1
	case yuv422BucherFormat:
#endif
	case yuv422Format:
		r = color >> 16;
		g = color >> 8;
		b = color;
		RGBTOYUV(r,g,b,y,u,v);
		color = ((ulong)y << 24) + (((ulong)u) << 16) + ((y) << 8)  + v ;
		break;
	case yuvFormat:
		r = color >> 16;
		g = color >> 8;
		b = color;
		RGBTOYUV(r,g,b,y,u,v);
		color = ((ulong)y << 24) + (((ulong)u) << 16) + v ;
		break;
	default:
		Complain(("bad bitmap format"));
		break;
	}
}

const Byte*
GetCLUT24(const BitMapDevice* device)
{	
	static uchar gGrayClut8[(1<<8)*3];
	static const uchar gGrayClut4[(1<<4)*3] = {
		0x00,0x00,0x00,
		0x14,0x14,0x14,
		0x24,0x24,0x24,
		0x34,0x34,0x34,
		0x44,0x44,0x44,
		0x54,0x54,0x54,
		0x64,0x64,0x64,
		0x74,0x74,0x74,
		0x88,0x88,0x88,
		0x98,0x98,0x98,
		0xa8,0xa8,0xa8,
		0xb8,0xb8,0xb8,
		0xc8,0xc8,0xc8,
		0xd8,0xd8,0xd8,
		0xe8,0xe8,0xe8,
		0xff,0xff,0xff
	};

	ushort  i;
	if ( device->colorTable == nil )  {
		if ( device->depth == 4 ) {
			return gGrayClut4;
		}		
		else
		{
			for ( i=0; i < (1<<8); i++ ) {
				gGrayClut8[i*3] = i;
				gGrayClut8[i*3+1] = i;
				gGrayClut8[i*3+2] = i;
			}
			return gGrayClut8;
		}
	}
	
	ushort count = device->colorTable->size/3;
	uchar*  pixel24 = (uchar*)&device->colorTable->data;	
	uchar r, g, b;
	
	
	switch ( device->colorTable->version & 0xf )
	{
	case kRGB15:
		{
			ushort* pixel16 = (ushort*)(&device->colorTable->data);
			ushort p16;
			
			pixel16 += count;
			pixel24 += count *3;
			for (i=0; i<count; i++) {
								// build the table backwards on top of itself
				p16 = *--pixel16;
				PIXEL16TORGB(p16, r, g, b);
				*--pixel24 = b;
				*--pixel24 = g;
				*--pixel24 = r;
			}
		}
		break;
	case kYUV24:
		{
			short y,u,v;
			uchar*  pixelYUV = pixel24; {
				y = *pixelYUV++;
				u = *pixelYUV++;
				v = *pixelYUV++;
				YUVTORGB(y,u,v,r,g,b);
				*pixel24++ = r;
				*pixel24++ = g;
				*pixel24++ = b;
			}
		}
		break;
	case kRGB24:
		break;
	default:
		Complain(("Bad CLUT conversion %d -> 24",device->colorTable->version));
		return nil;
	} 
	device->colorTable->version &= ~0xf;
	device->colorTable->version |= kRGB24;
	return (Byte*)&device->colorTable->data;
}


const Byte*
GetCLUTYUV(const BitMapDevice* device)
{
	static uchar gGrayYUVClut8[(1<<8)*3];		// this too should be static const tables so they dont use up RAM
	static const uchar gGrayYUVClut4[(1<<4)*3] = {
		0x00,kUVOffset,kUVOffset,
		0x14,kUVOffset,kUVOffset,
		0x24,kUVOffset,kUVOffset,
		0x34,kUVOffset,kUVOffset,
		0x44,kUVOffset,kUVOffset,
		0x54,kUVOffset,kUVOffset,
		0x64,kUVOffset,kUVOffset,
		0x74,kUVOffset,kUVOffset,
		0x88,kUVOffset,kUVOffset,
		0x98,kUVOffset,kUVOffset,
		0xa8,kUVOffset,kUVOffset,
		0xb8,kUVOffset,kUVOffset,
		0xc8,kUVOffset,kUVOffset,
		0xd8,kUVOffset,kUVOffset,
		0xe8,kUVOffset,kUVOffset,
		0xff,kUVOffset,kUVOffset
	};

	ushort  i;
	uchar r, g, b;
	short y,u,v;
	if ( device->colorTable == nil )  {
		if ( device->depth == 4 ) {
			return gGrayYUVClut4;
		}		
		else {
			for ( i=0; i < (1<<8); i++ ) {
				gGrayYUVClut8[i*3] = i;
				gGrayYUVClut8[i*3+1] = kUVOffset;
				gGrayYUVClut8[i*3+2] = kUVOffset;
			}
			return gGrayYUVClut8;
		}
	}
	uchar*  pixelYUV = (uchar*)&device->colorTable->data;
	ushort  count = device->colorTable->size/3;
	
	
	switch (device->colorTable->version & 0xf )
	{
	case kRGB15:
		{
			ushort*  pixel16 = (ushort*)&device->colorTable->data;
			ushort 	p16;
			
			pixel16 += count;
			pixelYUV += count*3;
			
			for (i=0; i<count; i++) {
										// build the table backwards on top of itself
				p16 = *--pixel16;
				PIXEL16TORGB(p16, r, g, b);
				RGBTOYUV(r,g,b,y,u,v);
				*--pixelYUV = v;
				*--pixelYUV = u;
				*--pixelYUV = y;
			}
		}
		break;
	case kRGB24:
		{
			uchar*  pixel24 = pixelYUV;
			for (i=0; i<count; i++) {
				r = *pixel24++;
				g = *pixel24++;
				b = *pixel24++;
				RGBTOYUV(r,g,b,y,u,v);
				*pixelYUV++ = y;
				*pixelYUV++ = u;
				*pixelYUV++ = v;
			}
		}
		break;
	case kYUV24:
		break;
	default:
		Complain(("Bad CLUT conversion %d -> YUV",device->colorTable->version));
		return nil;
	}
	device->colorTable->version &= ~0xf;
	device->colorTable->version |= kYUV24;
	return (Byte*)&device->colorTable->data;
}




#ifdef SIMULATOR
const ushort* 
GetCLUT16(const BitMapDevice* device)
{
	static ushort gGray15Clut8[(1<<8)];
	static ushort gGray15Clut4[(1<<4)];
	
	ushort  i;
	if ( device->colorTable == nil )  {
		if ( device->depth == 4 ) {
			for ( i=0; i < (1<<4); i++ )  {
				 RGBTOPIXEL16(i<<4,i<<4,i<<4,gGray15Clut4[i]);
			}
			return gGray15Clut4;
		}		
		else {
			for ( i=0; i < (1<<8); i++ ) {
				 RGBTOPIXEL16(i,i,i,gGray15Clut8[i]);
			}
			return gGray15Clut8;
		}
	}
	ushort* pixel16 = (ushort*)&device->colorTable->data;
	uchar r, g, b;
	ushort count = device->colorTable->size/3;
	uchar*  pixel24 = (uchar*)&device->colorTable->data;

	switch (device->colorTable->version & 0xf)
	{	
	case kRGB24:
		for (i=0; i<count; i++) {
			r = *pixel24++;
			g = *pixel24++;
			b = *pixel24++;
			RGBTOPIXEL16(r, g, b, *pixel16++);
		}
		break;
	case kYUV24:
		{
			short y,u,v;
			
			for (i=0; i<count; i++) {
				y = *pixel24++;
				u = *pixel24++;
				v = *pixel24++;
				YUVTORGB(y,u,v,r,g,b);
				RGBTOPIXEL16(r, g, b, *pixel16++);
				
			}
		}
		break;
	case kRGB15:
		break;
	default:
		Complain(("Bad CLUT conversion %d -> 16",device->colorTable->version));
		return nil;	
	}
	device->colorTable->version &= ~0xf;
	device->colorTable->version |= kRGB15;
	return (ushort*)&device->colorTable->data;
}
#endif


void
LookUpRGB(uchar index,const CLUT *clut,uchar &r,uchar &g,uchar &b)
{
	const uchar *p;
	ushort pixel;
	short y,u,v;
	
	
	if ( clut == nil ) {			// return gray if nil
		r = index;
		g = index;
		b = index;
	}
	switch (clut->version & 0xf ) {
	case kRGB24:
		p = &clut->data[(short)index * 3];
		r = *p++;
		g = *p++;
		b = *p++;
		break;
	case kRGB15:
		pixel = *(ushort*)&clut->data[(short)index<<1];
		PIXEL16TORGB(pixel, r, g, b);
		break;
	case kYUV24:
		p = (uchar*)&clut->data[(short)index * 3];
		y = *p++;
		u = *p++;
		v = *p++;
		YUVTORGB(y,u,v,r,g,b);
		break;
	case kYUV32:
		p = (uchar*)&clut->data[(short)index <<2];		// ignores alpha
		y = *p++;
		u = *p++;
		p++;
		v = *p++;
		YUVTORGB(y,u,v,r,g,b);
		break;
	}
}



static Color
BlendColor(Color e,Color f,uchar weight)
{
	ulong r,g,b;
	
	r = (e>>16) & 0xff;
	g = (e>>8) & 0xff;
	b = (e) & 0xff;
	r = TransparencyBlend(weight,r,((f>>16) & 0xff));
	g = TransparencyBlend(weight,g,((f>>8) & 0xff));
	b = TransparencyBlend(weight,b,((f) & 0xff));
	return ((ulong)r<<16) + (g<<8) + b;	
}


// ===========================================================================
//	Gamma calculations
// ===========================================================================

static long
FracIPow(Fract x,long y);

static long
FracIPow(Fract x,long y)
{
	if ( y == kFractOne )
		return kFractOne;
	if ( x == 0 )
		return 0;
		
	if (y < 0) {
		y = -y;
		x = FracDiv(kFractOne,x);
	}
    long r = kFractOne;
    while ( 1 )  {
    	if (y & 1)
			r = FracMul(r,x);
		if ((y >>= 1) == 0)
			break;
		else
			x = FracMul(x,x);
    }
	return r;
}

//#define	USE_FLOATING_POINT

#ifdef	USE_FLOATING_POINT
	#include	<math.h>
#endif

static long
FracPow(Fract x,Fract y);

static long
FracPow(Fract x,Fract y)
{
	if ( IsError(x < 0 || x > kFractOne || y < 0 ) )
		return 0;
	if ( y == 0 )
		return kFractOne;
	if ( y == kFractOne )
		return x;
	if ( x == 0 )
		return 0;

#ifdef	USE_FLOATING_POINT

	double xF = (double)x / (double)kFractOne;
	double yF = (double)y / (double)kFractOne;
	double rF = pow(xF,yF);
	return (int)(rF * (double)kFractOne);
#else
	const	Fract powMin = 0x0100;
	static const ushort powMinPow[256] = {
		0x0000,0xfa83,0xfb31,0xfb97,0xfbdf,0xfc18,0xfc46,0xfc6d,
		0xfc8e,0xfcac,0xfcc7,0xfcdf,0xfcf5,0xfd09,0xfd1c,0xfd2d,
		0xfd3e,0xfd4d,0xfd5b,0xfd69,0xfd76,0xfd82,0xfd8e,0xfd9a,
		0xfda4,0xfdaf,0xfdb9,0xfdc2,0xfdcb,0xfdd4,0xfddd,0xfde5,
		0xfded,0xfdf5,0xfdfd,0xfe04,0xfe0b,0xfe12,0xfe19,0xfe20,
		0xfe26,0xfe2c,0xfe32,0xfe38,0xfe3e,0xfe44,0xfe4a,0xfe4f,
		0xfe54,0xfe5a,0xfe5f,0xfe64,0xfe69,0xfe6e,0xfe72,0xfe77,
		0xfe7c,0xfe80,0xfe85,0xfe89,0xfe8d,0xfe91,0xfe95,0xfe9a,
		0xfe9e,0xfea2,0xfea5,0xfea9,0xfead,0xfeb1,0xfeb4,0xfeb8,
		0xfebc,0xfebf,0xfec3,0xfec6,0xfec9,0xfecd,0xfed0,0xfed3,
		0xfed6,0xfeda,0xfedd,0xfee0,0xfee3,0xfee6,0xfee9,0xfeec,
		0xfeef,0xfef2,0xfef4,0xfef7,0xfefa,0xfefd,0xff00,0xff02,
		0xff05,0xff08,0xff0a,0xff0d,0xff0f,0xff12,0xff14,0xff17,
		0xff19,0xff1c,0xff1e,0xff21,0xff23,0xff25,0xff28,0xff2a,
		0xff2c,0xff2e,0xff31,0xff33,0xff35,0xff37,0xff3a,0xff3c,
		0xff3e,0xff40,0xff42,0xff44,0xff46,0xff48,0xff4a,0xff4c,
		0xff4e,0xff50,0xff52,0xff54,0xff56,0xff58,0xff5a,0xff5c,
		0xff5e,0xff60,0xff62,0xff63,0xff65,0xff67,0xff69,0xff6b,
		0xff6c,0xff6e,0xff70,0xff72,0xff73,0xff75,0xff77,0xff78,
		0xff7a,0xff7c,0xff7e,0xff7f,0xff81,0xff82,0xff84,0xff86,
		0xff87,0xff89,0xff8a,0xff8c,0xff8e,0xff8f,0xff91,0xff92,
		0xff94,0xff95,0xff97,0xff98,0xff9a,0xff9b,0xff9d,0xff9e,
		0xffa0,0xffa1,0xffa3,0xffa4,0xffa5,0xffa7,0xffa8,0xffaa,
		0xffab,0xffac,0xffae,0xffaf,0xffb1,0xffb2,0xffb3,0xffb5,
		0xffb6,0xffb7,0xffb9,0xffba,0xffbb,0xffbc,0xffbe,0xffbf,
		0xffc0,0xffc2,0xffc3,0xffc4,0xffc5,0xffc7,0xffc8,0xffc9,
		0xffca,0xffcc,0xffcd,0xffce,0xffcf,0xffd0,0xffd2,0xffd3,
		0xffd4,0xffd5,0xffd6,0xffd8,0xffd9,0xffda,0xffdb,0xffdc,
		0xffdd,0xffde,0xffe0,0xffe1,0xffe2,0xffe3,0xffe4,0xffe5,
		0xffe6,0xffe7,0xffe9,0xffea,0xffeb,0xffec,0xffed,0xffee,
		0xffef,0xfff0,0xfff1,0xfff2,0xfff3,0xfff4,0xfff5,0xfff6,
		0xfff7,0xfff8,0xfff9,0xfffa,0xfffb,0xfffc,0xfffd,0xfffe,
	};

	ulong	iPow = kFractOne;
	long	intP = 1;
	ulong	pmf;
	x = FracToInt(256 * x + kRound);
	
	if ( IsWarning(x > 255) )
		x = 255;
	pmf = powMinPow[x];
		
	intP = FracToInt(y);
	y = FracMan(y);
	while ( y > powMin ) {
		y -= powMin;
		iPow = pmf * iPow;
		iPow += 0x8000;
		iPow >>= 16;
	}
	if ( intP != 1) 
		iPow = FracMul(iPow,FracIPow(x,intP));
	return iPow;
#endif
}



uchar *
BuildGammaTable(Gamma gamma, uchar blackLevel, uchar whiteLevel, 	
		uchar deviceBlack,uchar deviceWhite)
{
	static uchar sGammaTable[256];
	static Gamma sLastGamma = 0;
	
	if ( gamma == 0 || gamma > kFractOne*3 )
		return nil;
		
	if ( sLastGamma != gamma ) {
		long range = whiteLevel - blackLevel;
		long igamma = FracDiv(kFractOne,gamma);
		long drange = IntToFrac(deviceWhite-deviceBlack);
		long 	x,y;

		for ( x=0; x < 256; x++ ) {
			if ( x <= blackLevel )
				y = deviceBlack;
			else if ( x >= whiteLevel )
				y = deviceWhite;
			else {
				long pv = IntToFrac(x-blackLevel);
				pv /= range;
				pv = FracPow(pv,igamma);
				y = FracToInt(FracMul(drange,pv) + kRound);
				y += deviceBlack;
				if ( y < deviceBlack )
					y = deviceBlack;
				if ( y > deviceWhite )
					y = deviceWhite;
			}
			sGammaTable[x] = y;
		}
	}
	return sGammaTable;
}


void
GammaCorrect(CLUT *colorTable, Gamma gamma, uchar blackLevel, uchar whiteLevel)
{
	long count = colorTable->size;
	uchar black,white;
	uchar *gammaTable;
	uchar *ctb = (uchar *)colorTable->data;
	
	if ( gamma == 0 )
		return;
	if ( colorTable->version & kGammaCorrectedCLUT )		// dont re-correct
		return;	
		
	switch ( (colorTable->version  & 0xf) ) {
	case kRGB24:
		black = 0;		
		white = 255;
		break;
	case kYUV32:
	case kYUV24:
	case kYUV64:
		black = kBlackY;		
		white = kWhiteY;
		break;
	default:
		return;		// we dont do 16-bit cluts
		break;
	}
	gammaTable = BuildGammaTable(gamma,blackLevel,whiteLevel,black,white);
	if ( gammaTable ) {	
		switch ( (colorTable->version  & 0xf) ) {
		case kRGB24:
			while ( count-- ) {
				*ctb++ = gammaTable[*ctb];
			}
			break;
		case kYUV32:
			while ( count ) {
				*ctb = gammaTable[*ctb];
				ctb += 4;
				count -= 4;
			}
			break;
		case kYUV24:
			while ( count ) {
				*ctb = gammaTable[*ctb];
				ctb += 3;
				count -= 3;
			}
			break;
		case kYUV64:
			while ( count ) {
				*ctb = gammaTable[*ctb];
				ctb += 2;
				count -= 2;
			}
			break;
		}
		colorTable->version |= kGammaCorrectedCLUT;
	}
}

void
VDelay(long ticks = 0,Boolean syncToVBlank = true);

void
VDelay(long ticks,Boolean syncToVBlank)
{
	ulong  later;
	if ( ticks < 0 )
		ticks = 0;
	later = Now() + ticks - 1;
	
	
	while ( Now() < later )
		TCPIdle(false);
		
	if ( syncToVBlank ) {
#ifdef HARDWARE
		if (!gSystemTicks & 1)
			while (!gSystemTicks & 1)
				;
		else
			while (gSystemTicks & 1)
				;
#endif
	}

}


// ===========================================================================
//	Screen scrolling / transition operations
// ===========================================================================

static void ScrollDown(BitMapDevice& dstDevice, BitMapDevice& srcDevice, const Rectangle& srcRect, short scrollSize)
{
	Rectangle	oldRect, newRect, copyFromRect, copyToRect;
	long	scrollDelta = kInitialScrollDelta;
	long 	srcHeight = RectangleHeight(srcRect);
	
	newRect = srcRect;
	copyFromRect = copyToRect = srcRect;
	OffsetRectangle(copyToRect, 0, scrollSize);

	// Overlap by 2 pixels to overwrite lines filtered with options panel.
	if (scrollSize < (srcHeight - 2)) {
		copyFromRect.top += srcHeight - scrollSize - 2;
		copyToRect.top += srcHeight - scrollSize - 2;
	}

	// This scrolling algorithm maximizes the amount of the on-screen 
	// buffer that is scrolled, and minimizes the amount of the off-screen
	// buffer that is copied on, to eliminate glitches caused by the top
	// margin of the new page.
		
	while (scrollSize > 0) {
		scrollDelta = MIN(scrollDelta, scrollSize);
			
		oldRect = newRect;
		OffsetRectangle(newRect, 0, -scrollDelta);
		
		OffsetRectangle(copyToRect, 0, -scrollDelta);	

		VDelay();
		DoCopyImage(dstDevice, dstDevice, oldRect, newRect, kNotTransparent, &srcRect);
		dstDevice.filter = kCopyFilterOffset;
		DoCopyImage(srcDevice, dstDevice, copyFromRect, copyToRect, kNotTransparent, &srcRect);
		dstDevice.filter = kNoFilter;

		scrollSize -= scrollDelta;
				
		if (scrollDelta < kMaxScrollDelta)
			scrollDelta += kScrollAcceleration;
	}
	
	// Final complete copy to guarantee top margin is drawn correctly.
	dstDevice.filter = kCopyFilter;
	DoCopyImage(srcDevice, dstDevice, srcRect, srcRect, kNotTransparent, &srcRect);
	dstDevice.filter = kNoFilter;
}

static void ScrollUp(BitMapDevice& dstDevice, BitMapDevice& srcDevice, const Rectangle& srcRect, short scrollSize)
{
	Rectangle	oldRect, newRect, copyFromRect, copyToRect;
	long	scrollDelta = kInitialScrollDelta;
	long 	srcHeight = RectangleHeight(srcRect);

	scrollSize = -scrollSize;

	newRect = srcRect;
	copyFromRect = copyToRect = srcRect;
	OffsetRectangle(copyToRect, 0, -scrollSize);

	if (scrollSize < srcHeight)
		newRect.top += srcHeight - scrollSize;

	// This scrolling algorithm minimizes the amount of the on-screen 
	// buffer that is scrolled, and maximizes the amount of the off-screen
	// buffer that is copied on, to eliminate glitches caused by the top
	// margin of the old page.
		
	while (scrollSize > 0) {
		scrollDelta = MIN(scrollDelta, scrollSize);

		oldRect = newRect;
		OffsetRectangle(newRect, 0, scrollDelta);
		
		OffsetRectangle(copyToRect, 0, scrollDelta);
		
		VDelay();
		DoCopyImage(dstDevice, dstDevice, oldRect, newRect, kNotTransparent, &srcRect);
		dstDevice.filter = kCopyFilterOffset;	// use filter blit
		DoCopyImage(srcDevice, dstDevice, copyFromRect, copyToRect, kNotTransparent, &srcRect);
		dstDevice.filter = kNoFilter;
		
		scrollSize -= scrollDelta;
				
		if (scrollDelta < kMaxScrollDelta)
			scrollDelta += kScrollAcceleration;
	}
	
	// Final complete copy to guarantee top of option panel is filtered correctly.
	dstDevice.filter = kCopyFilter;
	DoCopyImage(srcDevice, dstDevice, srcRect, srcRect, kNotTransparent, &srcRect);
	dstDevice.filter = kNoFilter;
}


const kTransitionEffectSteps = 16;
#ifdef FOR_MAC
const kCrossFadeSteps = 5;
const kBlackFadeSteps = 5;
#else
const kCrossFadeSteps = 8;
const kBlackFadeSteps = 8;
#endif





typedef struct  {
	long xInc;
	long yInc;
	long xInc2;
	long yInc2;
	long xSizeInc;
	long ySizeInc;
	Rectangle rect;
} TransitionParams;

static void	DoScreenTransition(const BitMapDevice& src, BitMapDevice& dst, const Rectangle& srcBounds, 
	const Rectangle& dstBounds, TransitionEffect effectType,ulong delay=0,Color newOverscanColor = 0);



static void DoScreenTransition(const BitMapDevice& src, BitMapDevice& dst, const Rectangle& srcBounds, const Rectangle& dstBounds,
	TransitionEffect effectType,ulong delayTime,Color newOverscanColor)
{
	int step;
	long dstWidth = dstBounds.right - dstBounds.left;
	long dstHeight = dstBounds.bottom - dstBounds.top;
	long srcWidth = srcBounds.right - srcBounds.left;
	long srcHeight = srcBounds.bottom - srcBounds.top;
	
	long	dstXInt = dstWidth/ kTransitionEffectSteps;
	long	dstYInt = dstHeight/ kTransitionEffectSteps;
	long	srcXInt = srcWidth/ kTransitionEffectSteps;
	long	srcYInt = srcHeight/ kTransitionEffectSteps;
	TransitionParams	newSrc;
	TransitionParams	newDst;
	TransitionParams	oldSrc;
	TransitionParams	oldDst;
	TransitionEffect	nextEffect = kTransitionNone;
	Boolean		updateOldSrcRect = false;
	long		acceleration = 0;
	ulong	    transparency = kNotTransparent;
	const Boolean		goFast = true;
	
	long delay = delayTime<<2;	// make fractional
	
	if ( effectType == kTransitionNone )
		return;
	newSrc.xInc = 0;
	newSrc.xInc2 = 0;
	newSrc.xSizeInc = 0;
	newSrc.yInc = 0;
	newSrc.yInc2 = 0;
	newSrc.ySizeInc = 0;
	newSrc.rect = srcBounds;

	newDst.xInc = 0;
	newDst.xInc2 = 0;
	newDst.xSizeInc = 0;
	newDst.yInc = 0;
	newDst.yInc2 = 0;
	newDst.ySizeInc = 0;
	newDst.rect = dstBounds;

	oldSrc.xInc = 0;
	oldSrc.xInc2 = 0;
	oldSrc.xSizeInc = 0;
	oldSrc.yInc = 0;
	oldSrc.yInc2 = 0;
	oldSrc.ySizeInc = 0;
	oldSrc.rect = dstBounds;

	oldDst.xInc = 0;
	oldDst.xInc2 = 0;
	oldDst.xSizeInc = 0;
	oldDst.yInc = 0;
	oldDst.yInc2 = 0;
	oldDst.ySizeInc = 0;
	oldDst.rect = dstBounds;


	switch ( effectType )
	{
	case kTransitionBlackFade:
#if	1
#ifndef	SPOT1	// can't paint rect to bucher format screen
		for ( step = 0; step < kBlackFadeSteps; step++) {
			VDelay(delay>>2);
			PaintRectangle(dst, srcBounds, kBlackColor, kHalfTransparent);	// darken the screen
		}
#endif
		// and then fall thru to kTransitionCrossFade case
#endif		
	case kTransitionCrossFade:					// power of two cross fade

		
		for ( step = 0; step < kCrossFadeSteps; step++) {
			VDelay(delay>>2);
			DoCopyImage(src, dst, srcBounds, dstBounds, kHalfTransparent, nil);
			SetScreenBorderColor(BlendColor(gScreenBorderColor,newOverscanColor));
			if ( gFilterOnScreenUpdate )
				DrawScreenBorder();
		}
		goto done;

	case kTransitionWipeLeft:
		newSrc.xInc = -srcXInt;	
		newSrc.rect.left += srcWidth;
		newSrc.rect.right += srcWidth;
		newDst.xInc = -dstXInt;		
		newDst.rect.left += dstWidth;
		newDst.rect.right += dstWidth;
		goto coverTransition;
	case kTransitionWipeRight:
		newSrc.xInc = srcXInt;	
		newSrc.rect.left -= srcWidth;
		newSrc.rect.right -= srcWidth;
		newDst.xInc = dstXInt;		
		newDst.rect.left -= dstWidth;
		newDst.rect.right -= dstWidth;
		goto coverTransition;
	case kTransitionWipeUp:
wipeUp:
		newSrc.yInc = -srcYInt;	
		newSrc.rect.top += srcHeight;
		newSrc.rect.bottom += srcHeight;
		newDst.yInc = -dstYInt;		
		newDst.rect.top += dstHeight;
		newDst.rect.bottom += dstHeight;
		goto coverTransition;
	case kTransitionWipeDown:
wipeDown:
		newSrc.yInc = srcYInt;	
		newSrc.rect.top -= srcHeight;
		newSrc.rect.bottom -= srcHeight;
		newDst.yInc = dstYInt;		
		newDst.rect.top -= dstHeight;
		newDst.rect.bottom -= dstHeight;
		goto coverTransition;
	case kTransitionWipeLeftTop:
		newSrc.xInc = -srcXInt;	
		newSrc.rect.left += srcWidth;
		newSrc.rect.right += srcWidth;
		newDst.xInc = -dstXInt;		
		newDst.rect.left += dstWidth;
		newDst.rect.right += dstWidth;
		goto wipeDown;
	case kTransitionWipeRightTop:
		newSrc.xInc = srcXInt;	
		newSrc.rect.left -= srcWidth;
		newSrc.rect.right -= srcWidth;
		newDst.xInc = dstXInt;		
		newDst.rect.left -= dstWidth;
		newDst.rect.right -= dstWidth;
		goto wipeDown;
	case kTransitionWipeLeftBottom:
		newSrc.xInc = -srcXInt;	
		newSrc.rect.left += srcWidth;
		newSrc.rect.right += srcWidth;
		newDst.xInc = -dstXInt;		
		newDst.rect.left += dstWidth;
		newDst.rect.right += dstWidth;
		goto wipeUp;
	case kTransitionWipeRightBottom:
		newSrc.xInc = srcXInt;	
		newSrc.rect.left -= srcWidth;
		newSrc.rect.right -= srcWidth;
		newDst.xInc = dstXInt;		
		newDst.rect.left -= dstWidth;
		newDst.rect.right -= dstWidth;
		goto wipeUp;

	case kTransitionSlideLeft:
		newDst.xInc = -dstXInt;		
		newDst.rect.left += dstWidth;
		newDst.rect.right += dstWidth;
		goto coverTransition;
	case kTransitionSlideRight:
		newDst.xInc = dstXInt;		
		newDst.rect.left -= dstWidth;
		newDst.rect.right -= dstWidth;
		goto coverTransition;
	case kTransitionSlideUp:
		newDst.yInc = -dstYInt;		
		newDst.rect.top += dstHeight;
		newDst.rect.bottom += dstHeight;
		goto coverTransition;
	case kTransitionSlideDown:
		newDst.yInc = dstYInt;		
		newDst.rect.top -= dstHeight;
		newDst.rect.bottom -= dstHeight;
		goto coverTransition;



	case kTransitionPushLeft:
		newDst.xInc = -dstXInt;		
		newDst.rect.left += dstWidth;
		newDst.rect.right += dstWidth;
		oldSrc.xInc = -dstXInt;
		oldDst.xInc = -dstXInt;		
		oldDst.rect.left += -dstXInt;
		oldDst.rect.right += -dstXInt;
		goto spatialTransition;
		
	case kTransitionPushRight:
		newDst.xInc = dstXInt;		
		newDst.rect.left -= dstWidth;
		newDst.rect.right -= dstWidth;
		oldSrc.xInc = dstXInt;
		oldDst.xInc = dstXInt;		
		oldDst.rect.left += dstXInt;
		oldDst.rect.right += dstXInt;
		goto spatialTransition;

	case kTransitionPushUp:
		newDst.yInc = -dstYInt;		
		newDst.rect.top += dstHeight;
		newDst.rect.bottom += dstHeight;
		oldSrc.yInc = -dstYInt;
		oldDst.yInc = -dstYInt;		
		oldDst.rect.top += -dstYInt;
		oldDst.rect.bottom += -dstYInt;
		goto spatialTransition;
		
	case kTransitionPushDown:
		newDst.yInc = dstYInt;		
		newDst.rect.top -= dstHeight;
		newDst.rect.bottom -= dstHeight;
		oldSrc.yInc = dstYInt;
		oldDst.yInc = dstYInt;		
		oldDst.rect.top += dstYInt;
		oldDst.rect.bottom += dstYInt;
		goto spatialTransition;
		
		
	case kTransitionZoomOut:
	
		newDst.rect.right = (newDst.rect.left += dstWidth/2);
		newDst.rect.bottom = (newDst.rect.top += dstHeight/2);
		newDst.xInc = -dstXInt/2;		
		newDst.xInc2 = dstXInt/2;		
		newDst.yInc = -dstYInt/2;		
		newDst.yInc2 = dstYInt/2;	
		acceleration = 2;	
		goto coverTransition2;

	case kTransitionZoomOutH:
	
		newDst.rect.right = (newDst.rect.left += dstWidth/2);
		newDst.xInc = -dstXInt/2;		
		newDst.xInc2 = dstXInt/2;		
		acceleration = 1;	
		goto coverTransition2;

	case kTransitionZoomOutV:
	
		newDst.rect.bottom = (newDst.rect.top += dstHeight/2);
		newDst.yInc = -dstYInt/2;		
		newDst.yInc2 = dstYInt/2;		
		acceleration = 1;	
		goto coverTransition2;


	case kTransitionZoomInZoomOut:
	
		oldDst.xInc = dstXInt/2;		
		oldDst.xInc2 = -dstXInt/2;		
		oldDst.yInc = dstYInt/2;		
		oldDst.yInc2 = -dstYInt/2;
		SetRectangle(newDst.rect,0,0,0,0);
		SetRectangle(newSrc.rect,0,0,0,0);
		updateOldSrcRect = true;
		nextEffect = kTransitionZoomOut;
		goto spatialTransition2;

	case kTransitionZoomInZoomOutH:
	
		oldDst.xInc = dstXInt/2;		
		oldDst.xInc2 = -dstXInt/2;		
		SetRectangle(newDst.rect,0,0,0,0);
		SetRectangle(newSrc.rect,0,0,0,0);
		updateOldSrcRect = true;
		nextEffect = kTransitionZoomOutH;
		goto spatialTransition2;

	case kTransitionZoomInZoomOutV:
	
		oldDst.yInc = dstYInt/2;		
		oldDst.yInc2 = -dstYInt/2;
		SetRectangle(newDst.rect,0,0,0,0);
		SetRectangle(newSrc.rect,0,0,0,0);
		updateOldSrcRect = true;
		nextEffect = kTransitionZoomOutV;
		goto spatialTransition2;

	default:
		goto done;
	}
	
	Trespass();

coverTransition:						// spatial only, destination does not move

	newSrc.xInc2 = newSrc.xInc;
	newSrc.yInc2 = newSrc.yInc;
	newDst.xInc2 = newDst.xInc;
	newDst.yInc2 = newDst.yInc;

coverTransition2:

	for ( step = 0;step < kTransitionEffectSteps; step++)
	{
		newSrc.rect.left   += newSrc.xInc;
		newSrc.rect.right  += newSrc.xInc2;
		newSrc.rect.top    += newSrc.yInc;
		newSrc.rect.bottom += newSrc.yInc2;
		
		newDst.rect.left   += newDst.xInc;
		newDst.rect.right  += newDst.xInc2;
		newDst.rect.top    += newDst.yInc;
		newDst.rect.bottom += newDst.yInc2;
		if ( delay < 0 ) {
			delay = 0;
			continue;
		}
		if ( acceleration ) {
			delay -= acceleration;
		}
		VDelay(delay>>2);
		DoCopyImage(src, dst, newSrc.rect, newDst.rect, transparency, &dstBounds,false,false,goFast);
	}
	goto done;
	
spatialTransition:						// spatial only

	newSrc.xInc2 = newSrc.xInc;
	newSrc.yInc2 = newSrc.yInc;
	newDst.xInc2 = newDst.xInc;
	newDst.yInc2 = newDst.yInc;
	oldSrc.xInc2 = oldSrc.xInc;
	oldSrc.yInc2 = oldSrc.yInc;
	oldDst.xInc2 = oldDst.xInc;
	oldDst.yInc2 = oldDst.yInc;

spatialTransition2:

	for ( step = 0;step < kTransitionEffectSteps; step++)
	{
		VDelay(delay>>2);
		DoCopyImage(dst, dst, oldSrc.rect, oldDst.rect, transparency, &dstBounds,false,false,goFast);
		oldSrc.rect.left += oldSrc.xInc;
		oldSrc.rect.right += oldSrc.xInc2;
		oldSrc.rect.top += oldSrc.yInc;
		oldSrc.rect.bottom += oldSrc.yInc2;
		
		if ( updateOldSrcRect )
			oldSrc.rect = oldDst.rect;
		oldDst.rect.left += oldDst.xInc;
		oldDst.rect.right += oldDst.xInc2;
		oldDst.rect.top += oldDst.yInc;
		oldDst.rect.bottom += oldDst.yInc2;

		newSrc.rect.left += newSrc.xInc;
		newSrc.rect.right += newSrc.xInc2;
		newSrc.rect.top += newSrc.yInc;
		newSrc.rect.bottom += newSrc.yInc2;
		
		newDst.rect.left += newDst.xInc;
		newDst.rect.right += newDst.xInc2;
		newDst.rect.top += newDst.yInc;
		newDst.rect.bottom += newDst.yInc2;
		if ( !EmptyRectangle(&newDst.rect) ) {
			VDelay(0);
			DoCopyImage(src, dst, newSrc.rect, newDst.rect, transparency, &dstBounds,false,false,goFast);
		}
	}
done:

	VDelay(delay>>2);

	if ( nextEffect != kTransitionNone )
		DoScreenTransition(src, dst, srcBounds, dstBounds,nextEffect,delay>>2,newOverscanColor);
	else
		DoCopyImage(src, dst, srcBounds, dstBounds, kNotTransparent, nil);
}



static void
SetScreenBorderColor(Color color)
{
	if ( color != gScreenBorderColor ) {
#ifdef	HARDWARE
		Rectangle opanRect;
		SetDisplayOverscanColor(color,0);
		
		if (gOptionsPanel &&  gOptionsPanel->IsVisible() ) {
			gOptionsPanel->GetBounds(&opanRect);
	//		SetDisplayOverscanColor(kOptionsPanelColor,opanRect.top);
		}
#endif
		gScreenBorderColor = color;
	}
}



static void 
UpdateOptionsPanel();

static void
AlignUpdateRect(Rectangle &dirtyRect, Boolean horizontalOnly = false);


static void 
UpdateOptionsPanel()
{
	Rectangle optionsPanelBounds;
	if ( gOptionsPanel && gOptionsPanel->IsVisible() ) {
		gOptionsPanel->GetBounds(&optionsPanelBounds);
		gOnScreenDevice.filter = kCopyFilter;		// use filter blit
		if ( gFilterOnScreenUpdate ) 
			optionsPanelBounds.top -= 1;			// filter into image
		DoCopyImage(gScreenDevice, gOnScreenDevice, optionsPanelBounds, optionsPanelBounds);
		gOnScreenDevice.filter = kNoFilter;
	}
}


static void
AlignUpdateRect(Rectangle &dirtyRect, Boolean horizontalOnly)
{
	// always do long-aligned blits since they are twice as fast
	
	if ( dirtyRect.left & 1 )
		dirtyRect.left--;
		
	// always do last YV of a YUYV pair so we funny colors dont happen

	if ( dirtyRect.right & 1 )
		dirtyRect.right++;

	if (horizontalOnly)
		return;

	if ( gFilterOnScreenUpdate ) {
		if ( dirtyRect.top > gOnScreenDevice.bounds.top )
			dirtyRect.top -= 1;
		if ( dirtyRect.bottom < gOnScreenDevice.bounds.bottom )
			dirtyRect.bottom += 1;
	}
}

void UpdateScreenBits()
{
	if (   !gScreenDirtyRegion->IsEmpty() ) {
		Rectangle dirtyRect;
		Color	edgeColor = kBlackBorderColor;
		Boolean doFilterOptionsPanel = !gFilterOnScreenUpdate && gOptionsPanel && gOptionsPanel->IsVisible();
#ifdef FOR_MAC
		MakeBitMapDevice(gOnScreenDevice, (CGrafPtr)gMacSimulator->GetWindow());
#endif


#ifdef	CHRIS_LOVES_THE_FLOATING_OPTION_PANEL
		{
			uchar r,g,b;
			short y,u,v;
			r = gPageBackColor >> 16;
			g = gPageBackColor >> 8;
			b = gPageBackColor;
			RGBTOYUV(r,g,b,y,u,v);
			edgeColor = ((ulong)y << 16) + (((ulong)u) << 8) + v ;		
		}
#else
		if ( gOptionsPanel && gOptionsPanel->IsVisible() )
			edgeColor = kOptionsPanelColor;
#endif
		// scroll update 
		
		if ( gPageViewerScrollDirection != 0 && gPageViewer != nil ) {
			gPageViewer->GetScrollBounds(&dirtyRect);
			SetScreenBorderColor(edgeColor);
			AlignUpdateRect(dirtyRect, true);
			if (gPageViewerScrollDirection > 0)
				ScrollDown(gOnScreenDevice, gScreenDevice, dirtyRect, gPageViewerScrollDirection);
			else
				ScrollUp(gOnScreenDevice, gScreenDevice, dirtyRect, gPageViewerScrollDirection);
			UpdateOptionsPanel();
			if ( doFilterOptionsPanel) 
				FilterOptionsPanel();
			if ( gFilterOnScreenUpdate )
				DrawScreenBorder();
				
			gPageViewerScrollDirection = 0;
			
		// transition effect update
		
		} else if ( gAnimationTransitionType || gPageViewerTransitionType ) {
			Rectangle optionsPanelBounds;
			TransitionEffect effect = gPageViewerTransitionType;
			ulong period = gPageViewerTransitionDelay;
			gScreenDirtyRegion->GetBounds(&dirtyRect);
			AlignUpdateRect(dirtyRect);
			if ( effect == kTransitionNone ) {
				effect = gAnimationTransitionType;
				period = gPendingAnimationDissolvePeriod;
			}
			if ( gOptionsPanel->IsVisible() && effect != kTransitionCrossFade) {
				gOptionsPanel->GetBounds(&optionsPanelBounds);
				dirtyRect.bottom = optionsPanelBounds.top;
			}
			gOnScreenDevice.filter = kCopyFilter;	// use filter blit
			DoScreenTransition(gScreenDevice, gOnScreenDevice, dirtyRect, dirtyRect,effect,period,edgeColor);
			SetScreenBorderColor(edgeColor);
			UpdateOptionsPanel();
			if ( doFilterOptionsPanel) 
				FilterOptionsPanel();
			if ( gFilterOnScreenUpdate )
				DrawScreenBorder();
			gPendingAnimationDissolvePeriod = 0;
			gPageViewerTransitionType = kTransitionNone;
		
		} else { 

			// partial update

			gOnScreenDevice.filter = kCopyFilter;		// use filter blit
#if	1	// for comparision
			gScreenDirtyRegion->GetBounds(&dirtyRect);
			AlignUpdateRect(dirtyRect);
			DoCopyImage(gScreenDevice, gOnScreenDevice, dirtyRect, dirtyRect);
#else
			int disjointRectCount = gScreenDirtyRegion->GetSubRegionCount();	
			int	i;
			Region *r;
			
			if ( disjointRectCount > 1 ) {
				for ( i=0; i < disjointRectCount; i++ ) {
					r = gScreenDirtyRegion->GetSubRegion(i);
					if ( r ) {
						r->GetBounds(&dirtyRect);
						AlignUpdateRect(dirtyRect);
						DoCopyImage(gScreenDevice, gOnScreenDevice, dirtyRect, dirtyRect);
						delete(r);
					}
				}
			} else {
				gScreenDirtyRegion->GetBounds(&dirtyRect);
				AlignUpdateRect(dirtyRect);
				DoCopyImage(gScreenDevice, gOnScreenDevice, dirtyRect, dirtyRect);
			}
			if ( gFilterOnScreenUpdate )
				DrawScreenBorder();
#endif
		}
		gScreenDirtyRegion->Reset();
		gOnScreenDevice.filter = kNoFilter;
	}
}



void DrawingComplete(const Rectangle& dirtyRect, const Rectangle* clip)
{
	Rectangle trimmed = dirtyRect;
	if (clip != nil)
		IntersectRectangle(&trimmed, clip);		// don't update stuff we didn't draw
#ifdef FOR_MAC
	UpdateMacScreen(trimmed);
#endif
	if ( gScreenDirtyRegion ) {
		Rectangle r;
		if ( gScreenDirtyRegion->GetCount() > 32 ) {
			TrivialMessage(("lots of updates"));
			gScreenDirtyRegion->GetBounds(&r);
			gScreenDirtyRegion->Reset();
			UnionRectangle(&trimmed,&r);
		}
		gScreenDirtyRegion->Add(&trimmed);
	}
}



// ===========================================================================
//	Flicker compensation
// ===========================================================================


static void
DrawScreenBorder()
{
#ifdef	HARDWARE
	ulong screenPixels = GetDisplayWidth()/2;

#if	0
	if ( gOnScreenDevice.format == yuv422Format ) {
		ushort *p,*q;
		short 	y,u,v;
		long	i;

#if	1		
		RGBTOYUV(0xff,0,0xff,y,u,v);
		
		// draw top edge
		p = (ushort *)(gOnScreenDevice.baseAddress-gOnScreenDevice.rowBytes);
		q = (ushort *)((uchar *)p + gOnScreenDevice.rowBytes);
		for ( i=0; i < screenPixels; i++ )  {
			RGBTOYUV(0,0,0xff,y,u,v);
			*p++ = ((y<<8) + u);
			*p++ = ((y<<8) + v);
			RGBTOYUV(0,0xff,0,y,u,v);
			*q++ = ((y<<8) + u);
			*q++ = ((y<<8) + v);
		}
#endif

		// draw bottom edge
		p = (ushort *)(gOnScreenDevice.baseAddress + ((gOnScreenDevice.bounds.bottom-gOnScreenDevice.bounds.top)) * gOnScreenDevice.rowBytes);
		q = (ushort *)((uchar *)p - gOnScreenDevice.rowBytes);
		for ( i=0; i < screenPixels; i++ )  {
			RGBTOYUV(0xff,0,0,y,u,v);
			*p++ = ((y<<8) + u);
			*p++ = ((y<<8) + v);
			RGBTOYUV(0,0xff,0,y,u,v);
			*q++ = ((y<<8) + u);
			*q++ = ((y<<8) + v);
		}
	}

#else

	if ( gOnScreenDevice.format == yuv422Format ) {
		ushort *p,*q;
		ushort backY = gScreenBorderColor>>16;
		ushort backU = (gScreenBorderColor>>8) & 0xff;
		ushort backV = (gScreenBorderColor>>0) & 0xff;
		ushort yu,yv;
		long	i;
		
		// draw top edge
		p = (ushort *)(gOnScreenDevice.baseAddress-gOnScreenDevice.rowBytes);
		q = (ushort *)((uchar *)p + gOnScreenDevice.rowBytes);
		for ( i=0; i < screenPixels; i++ )  {
			yu = *q++;
			yv = *q++;
			*p++ = (((((yu>>8) + backY) >> 1) & 0xff) << 8) + ((((yu & 0xff) + backU) >> 1));
			*p++ = (((((yv>>8) + backY) >> 1) & 0xff) << 8) + ((((yv & 0xff) + backV) >> 1));
		}

		// draw bottom edge
		p = (ushort *)(gOnScreenDevice.baseAddress + ((gOnScreenDevice.bounds.bottom-gOnScreenDevice.bounds.top)) * gOnScreenDevice.rowBytes);
		q = (ushort *)((uchar *)p - gOnScreenDevice.rowBytes);
		for ( i=0; i < screenPixels; i++ )  {
			yu = *q++;
			yv = *q++;
			*p++ = (((((yu>>8) + backY) >> 1) & 0xff) << 8) + ((((yu & 0xff) + backU) >> 1));
			*p++ = (((((yv>>8) + backY) >> 1) & 0xff) << 8) + ((((yv & 0xff) + backV) >> 1));
		}
	}
#endif
#endif
}

static void
FilterOptionsPanel()
{
	Rectangle bounds;
	
	if (!gSystem->GetUseFlickerFilter() || gFilterOnScreenUpdate )
		return;

	if ( gOptionsPanel && gOptionsPanel->IsVisible() &&
	     ( gOptionsPanel->IsShown() || gOptionsPanel->IsOpen() ) ) {
		// blend the options panel into the screen
		gOptionsPanel->GetBounds(&bounds);
		Rectangle topLine = bounds;
		topLine.top = topLine.top-1;
		topLine.bottom = topLine.top + 1;
		Color blend = BlendColor(kOptionsGrayBlend,gPageBackColor);
		XPaintRect(gScreenDevice, topLine, blend, kHalfTransparent, nil);
		topLine.top += 1;
		topLine.bottom += 1;
		XPaintRect(gScreenDevice, topLine, kOptionsGrayBlend, 0, nil);
		bounds.top -= 1;
	}
}

void FlickerFilterBounds(BitMapDevice& device,const Rectangle& r,FilterType filterType,const Rectangle* clip)
{

	if ( !gFilterOnScreenUpdate )
		XFilterBounds(device,r,filterType,clip);
}


// ===========================================================================
//	Border image drawing
// ===========================================================================


typedef struct {
	BitMapDevice *dst;
	BitMapDevice *src;
	Rectangle innerR;
	ulong transparency;
	Rectangle clip;
	Boolean stretchNotTile;
	Boolean drawCenter;
	Boolean	hasInnerCorners;
} BorderParams;
		
typedef enum {
	kTopEdge,
	kBottomEdge,
	kLeftEdge,
	kRightEdge,
	
	// extended or displaced edges for regions
	
	kTopEdgeXL,
	kTopEdgeXR,
	kBottomEdgeXL,
	kBottomEdgeXR,
	kLeftEdgeXB,
	kRightEdgeXB,
			
	kTopLeftCorner,
	kTopRightCorner,
	kBottomLeftCorner,
	kBottomRightCorner,
		
	// inside corners
		
	kTopLeftInsideCorner,
	kBottomLeftInsideCorner,
				
	kTopRightInsideCorner,
	kBottomRightInsideCorner,
				
	// displaced outside corners for regions
	
	kBottomRightCornerXB,
	kBottomLeftCornerXB,
		
	// straight connectors
		
	kRightSideConnector,
	kLeftSideConnector
			
} BorderPiece;
			

#define	IsCorner(x)	(x>=kTopLeftCorner)

static void DrawBorderPiece(BorderPiece piece,Coordinate *start,Coordinate *end,BorderParams *params);

static void DrawBorderPiece(BorderPiece piece,Coordinate *start,Coordinate *end,BorderParams *params)
{
	Rectangle srcRect,dstRect;
	long	w,h,t;
	Ordinate left,right = 0x7fffffff,top,bottom = 0x7fffffff;
	Boolean lengthen = false;
	long	ot,ol,or,ob;
	Boolean	isVertical = false;
	
	if ( IsCorner(piece) ) 
		end = start;

	left = start->x;
	right = end->x;
	if ( right < left ) {
		t = left;
		left = right;
		right = t;
	}
	top = start->y;
	bottom = end->y;
	if ( bottom < top ) {
		t = top;
		top = bottom;
		bottom = t;
	}
	
	ol = params->innerR.left - params->src->bounds.left;
	ot = params->innerR.top - params->src->bounds.top;
	or = params->src->bounds.right - params->innerR.right;
	ob = params->src->bounds.bottom - params->innerR.bottom;
	
	FilterType saveFilterType = params->src->filter;

	if ( params->src->filter )
		params->src->filter = kSliceFilter;
	
	h = w = 0;
	if ( IsCorner(piece)  ) {
	
		if ( !params->hasInnerCorners ) { 
			// these inside corners are not correct if there are variations on the corners 
			switch ( piece ) {
			case kTopRightInsideCorner:
				piece = kTopRightCorner;
				left += or;
				right = left;
				break;
			case kBottomRightInsideCorner:
				piece = kBottomRightCorner;
				top += ob;
				left += ol;
				bottom = top;
				right = left;
				break;
			case kTopLeftInsideCorner:
				piece = kTopLeftCorner;
				left -= ol;
				right = left;
				break;
			case kBottomLeftInsideCorner:
				piece = kBottomLeftCorner;
				top += ob;
				left -= or;
				bottom = top;
				break;
			default:
				break;
			}
		}
		switch ( piece ) {
		
		case kTopLeftInsideCorner:
			w = ol;
			h = ot;
			SetRectangle(srcRect, params->innerR.left,  params->innerR.top, params->innerR.left+w, params->innerR.top+h);
			SetRectangle(dstRect, left-w, top, left, top+h);
			break;
		
		case kTopLeftCorner:
			w = ol;
			h = ot;
			SetRectangle(srcRect, params->src->bounds.left,  params->src->bounds.top, params->innerR.left, params->innerR.top);
			SetRectangle(dstRect, left, top, left+w, top+h);
			break;
			
		case kTopRightInsideCorner:
			w = or;
			h = ot;
			SetRectangle(srcRect, params->innerR.right-or,  params->innerR.top, params->innerR.right, params->innerR.top+ot);
			SetRectangle(dstRect, right, top, right+w, top+h);
			break;
			
		case kTopRightCorner:
			w = or;
			h = ot;
			SetRectangle(srcRect, params->innerR.right,  params->src->bounds.top, params->src->bounds.right, params->innerR.top);
			SetRectangle(dstRect, right-w, top, right, top+h);
			break;
			
		case kBottomLeftInsideCorner:
			w = ol;
			h = ob;
			SetRectangle(srcRect, params->innerR.left,  params->innerR.bottom-h, params->innerR.left+w, params->innerR.bottom);
			SetRectangle(dstRect, left-w, bottom, left, bottom+h);
			break;

		case kBottomLeftCornerXB:
			bottom += ob;
		case kBottomLeftCorner:
			w = ol;
			h = ob;
			SetRectangle(srcRect, params->src->bounds.left,  params->innerR.bottom, params->innerR.left, params->src->bounds.bottom);
			SetRectangle(dstRect, left, bottom-h, left+w, bottom);
			break;
			
		case kBottomRightInsideCorner:
			w = or;
			h = ob;
			SetRectangle(srcRect, params->innerR.right-w,  params->innerR.bottom-h, params->innerR.right, params->innerR.bottom);
			SetRectangle(dstRect, right, bottom, right+w, bottom+h);
			break;

		case kBottomRightCornerXB:
			bottom += ob;
		case kBottomRightCorner:
			w = or;
			h = ob;
			SetRectangle(srcRect, params->innerR.right,  params->innerR.bottom, params->src->bounds.right, params->src->bounds.bottom);
			SetRectangle(dstRect, right-w, bottom-h, right, bottom);
			break;

		case kRightSideConnector:
			w = or;
			h = ot;
			SetRectangle(srcRect, params->innerR.right, params->innerR.top, params->src->bounds.right, params->innerR.top + h);
			SetRectangle(dstRect, right-w, top , right, top+h);
			break;
				
		case kLeftSideConnector:
			w = ol;
			h = ot;
			SetRectangle(srcRect, params->src->bounds.left, params->innerR.top, params->innerR.left, params->innerR.top+h);
			SetRectangle(dstRect, left, top, left+w, top+h);
			break;
		default:
			break;
		}
		CopyImage(*params->src, *params->dst, srcRect, dstRect, params->transparency, &params->clip);

	} else {
		switch ( piece ) {
		case kTopEdgeXL:
			goto texl;
		case kTopEdgeXR:
			left += ol;
			goto texr;
		case kTopEdge:
			left += ol;
texl:
			right -= or;
texr:
			h = ot;
			SetRectangle(srcRect, params->innerR.left, params->src->bounds.top, params->innerR.right, params->innerR.top);
			SetRectangle(dstRect, left, top, right, top + h);
			break;
			
		case kBottomEdgeXL:
			bottom += ob;
			goto bexl;
		case kBottomEdgeXR:
			bottom += ob;
			left += ol;
			goto bexr;
		case kBottomEdge:
			left += ol;
bexl:
			right -= or;
bexr:
			h = ob;
			SetRectangle(srcRect, params->innerR.left, params->innerR.bottom, params->innerR.right, params->src->bounds.bottom);
			SetRectangle(dstRect, left, bottom-h, right, bottom);
			break;

					
		case kLeftEdgeXB:
			lengthen = true;
		case kLeftEdge:
			w = ol;
			top += ot;
			if ( !lengthen )
				bottom -= ob;
			SetRectangle(srcRect, params->src->bounds.left, params->innerR.top, params->innerR.left, params->innerR.bottom);
			SetRectangle(dstRect, left, top, left+w, bottom);
			isVertical = true;
			break;
		
		case kRightEdgeXB:
			lengthen = true;
		case kRightEdge:
			w = or;
			top += ot;
			if ( !lengthen )
				bottom -= ob;
			SetRectangle(srcRect, params->innerR.right, params->innerR.top, params->src->bounds.right, params->innerR.bottom);
			SetRectangle(dstRect, right-w, top , right, bottom);
			isVertical = true;
			break;
		default:
			break;
		}
		if ( params->stretchNotTile ) {
			CopyImage(*params->src, *params->dst, srcRect, dstRect, params->transparency, &params->clip);
		} else {
			if ( isVertical ) {
				long inc = params->innerR.bottom - params->innerR.top;
				t = dstRect.bottom;
				dstRect.bottom = dstRect.top + inc;
				while ( 1 ) {
					if ( dstRect.bottom > t ) {
						dstRect.bottom = t;
						srcRect.bottom = srcRect.top + (dstRect.bottom-dstRect.top);
					}
					CopyImage(*params->src, *params->dst, srcRect, dstRect, params->transparency, &params->clip);
					if ( dstRect.bottom == t )
						break;
					dstRect.top += inc;
					dstRect.bottom += inc;
				}
			} else {
				long inc = params->innerR.right - params->innerR.left;
				t = dstRect.right;
				dstRect.right = dstRect.left + inc;
				while ( 1 ) {
					if ( dstRect.right > t ) {
						dstRect.right = t;
						srcRect.right = srcRect.left + (dstRect.right-dstRect.left);
					}
					CopyImage(*params->src, *params->dst, srcRect, dstRect, params->transparency, &params->clip);
					if ( dstRect.right == t )
						break;
					dstRect.left += inc;
					dstRect.right += inc;
				}
			}
		}
	}
	params->src->filter = saveFilterType;
}
	
void DrawBorderImage(BitMapDevice& device, const BitMapDevice& src, const Region* region, 
	const Rectangle& innerR, ulong transparency, const Rectangle* clip ,Boolean stretchNotTile,Boolean drawCenter,
	Boolean hasInnerCorners)
{
	long	i;
	long	rcount;
	Coordinate start,end;
	Rectangle r,nr;
	BorderParams	params;
	int	direction = 1;
	int expandH = 0,expandV = 0;

	params.dst = &device;
	params.src = (BitMapDevice *)&src;
	params.innerR = innerR;
	params.transparency = transparency;
	if ( clip )
		params.clip = *clip;
	else
		params.clip = device.bounds;
	params.stretchNotTile = stretchNotTile;
	if ( src.colorTable->version == kYUV32 )
		params.stretchNotTile = false;
	params.drawCenter = drawCenter;
	params.hasInnerCorners = hasInnerCorners;
	
	direction = 1;
	
	
	expandH = (src.bounds.right - innerR.right);
	expandV = (src.bounds.bottom - innerR.bottom);
	
	rcount = region->GetCount();
	if ( rcount == 0 )
		return;
	if ( region->IsRectangle() ) {
		region->GetBounds(&r);
		InsetRectangle(r,-expandH,-expandV);
		DrawBorderImage(device,src,r,innerR,transparency,clip,stretchNotTile,drawCenter);
	} else {
		long n = region->GetSubRegionCount();
		if ( n > 1 ) {
			for ( i=0; i < n; i++ ) {
				Region *subRegion = region->GetSubRegion(i);
				DrawBorderImage(device,src,subRegion,innerR,transparency,clip,stretchNotTile,drawCenter);
				delete(subRegion);
			}
			return;
		}
		

		// NOTE: assumes rectangles are pre-sorted top to bottom
			
		r = *region->RectangleAt(0);
		InsetRectangle(r,-expandH,-expandV);
		start.x = r.left;
		start.y = r.top;
		end.x = r.right;
		end.y = r.top;
		DrawBorderPiece(kTopLeftCorner,&start,nil,&params);
		DrawBorderPiece(kTopEdge,&start,&end,&params);
		DrawBorderPiece(kTopRightCorner,&end,nil,&params);
		start = end;
		for (i = 0; i < rcount; i++) {
			if ( i < (rcount-1) ) {
				nr = *region->RectangleAt(i+1);
				InsetRectangle(nr,-expandH,-expandV);
				
				// if the edges almost match, line them up
				if ( ABS(nr.right-r.right) < kMinimumRegionBorderOffset )
					nr.right = r.right;
				
				// for disjoint regions, make big
				
				if ( nr.right <= r.left )
					nr.right = r.right;
				
				if (nr.right < r.right)	
					nr.top += expandV;
				
				end.x = r.right;
				end.y = nr.top;
				DrawBorderPiece(kRightEdgeXB,&start,&end,&params);
				if ( nr.right > r.right ) {
					direction = 1;
					DrawBorderPiece(kBottomLeftInsideCorner,&end,nil,&params);
				} else if ( nr.right < r.right ) {
					direction = -1;
					DrawBorderPiece(kBottomRightCornerXB,&end,nil,&params);
				} else {
					DrawBorderPiece(kRightSideConnector,&end,nil,&params);
					direction = 0;
				}
				start = end;
				r = nr;
			
			} else {
				
				// draw bottom edge
				end.x = r.right;
				end.y = r.bottom;
				DrawBorderPiece(kRightEdge,&start,&end,&params);
				DrawBorderPiece(kBottomRightCorner,&end,nil,&params);
				start.x = r.left;
				start.y = r.bottom;
				end.x = r.right;
				end.y = r.bottom;
				DrawBorderPiece(kBottomEdge,&start,&end,&params);
				break;
			}
			end.x = r.right;
			end.y = r.top;
			if ( direction == 1 ) {
				DrawBorderPiece(kTopEdgeXL,&start,&end,&params);
				DrawBorderPiece(kTopRightCorner,&end,nil,&params);
			} else if ( direction == -1 ) {
				DrawBorderPiece(kBottomEdgeXL,&start,&end,&params);
				DrawBorderPiece(kTopLeftInsideCorner,&end,nil,&params);
			}
			start = end;
		}
		
		r = *region->RectangleAt(0);
		InsetRectangle(r,-expandH,-expandV);
		start.x = r.left;
		start.y = r.top;
		end.x = r.right;
		end.y = r.top;
		for (i = 0; i < rcount; i++) {
			if ( i < (rcount-1) ) {
				nr = *region->RectangleAt(i+1);
				InsetRectangle(nr,-expandH,-expandV);
				
				// if the edges almost match, line them up
				
				if ( ABS(nr.left-r.left) < kMinimumRegionBorderOffset )
					nr.left = r.left;
					
				// for disjoint regions, make big
				
				if ( nr.left >= r.right )
					nr.left = r.left;
				
				if (nr.left > r.left)			
					nr.top += expandV;
					
				end.x = r.left;
				end.y = nr.top;
				DrawBorderPiece(kLeftEdgeXB,&start,&end,&params);
				if ( nr.left > r.left ) {
					direction = 1;
					DrawBorderPiece(kBottomLeftCornerXB,&end,nil,&params);
				} else if ( nr.left < r.left ) {
					direction = -1;
					DrawBorderPiece(kBottomRightInsideCorner,&end,nil,&params);
				} else {
					DrawBorderPiece(kLeftSideConnector,&end,nil,&params);
					direction = 0;
				}
				start = end;
				r = nr;
					
			} else {
				end.x = r.left;
				end.y = r.bottom;
				DrawBorderPiece(kLeftEdge,&start,&end,&params);
				DrawBorderPiece(kBottomLeftCorner,&end,nil,&params);
				break;
			}
			end.x = r.left;
			end.y = r.top;
			if ( direction == 1 ) {
				DrawBorderPiece(kBottomEdgeXR,&start,&end,&params);
				DrawBorderPiece(kTopRightInsideCorner,&end,nil,&params);
			} else if ( direction == -1 ) {
				DrawBorderPiece(kTopEdgeXR,&start,&end,&params);
				DrawBorderPiece(kTopLeftCorner,&end,nil,&params);
			}
			start = end;
		}
	}
}
	
	

void DrawBorderImage(BitMapDevice& device, const BitMapDevice& src, const Rectangle& dstR, 
	const Rectangle& innerR, ulong transparency, const Rectangle* clip ,Boolean stretchNotTile,Boolean drawCenter)
{
	Rectangle destR = dstR;
	BorderParams	params;
	Coordinate start,end;
	BitMapDevice source = src;
	Rectangle	sourceR = source.bounds;

	if ( drawCenter ) {
		if ( src.filter )
			source.filter = (FilterType)((src.filter & ~0xf) | kSliceFilter);

		if ( stretchNotTile )  {
			Rectangle  dst = destR;
			dst.left += innerR.left;
			dst.top += innerR.top;
			dst.right -= sourceR.right-innerR.right;
			dst.bottom -= sourceR.bottom-innerR.bottom;
			CopyImage(source, device, innerR, dst, transparency, clip);
		}
		else {
			long	x,y;
			Rectangle dst;
			Rectangle centerClip;
			Rectangle  center = destR;
			center.left += innerR.left;
			center.top += innerR.top;
			center.right -= sourceR.right-innerR.right;
			center.bottom -= sourceR.bottom-innerR.bottom;
			
			long innerH = innerR.bottom-innerR.top;
			long innerW = innerR.right-innerR.left;
			centerClip = center;
			
			if ( clip )
				IntersectRectangle(&centerClip,clip);
			for ( y=center.top; y < center.bottom ; y += innerH ) {
				for ( x=center.left; x < center.right ; x += innerW ) {
					dst.left = x;
					dst.right = x + innerW;
					dst.top = y;
					dst.bottom = y + innerH;
					CopyImage(source, device, innerR, dst, transparency, &centerClip);
				}
			}
		}
		source.filter = src.filter;
	}
		
	params.dst = &device;
	params.src = (BitMapDevice *)&src;
	params.innerR = innerR;
	params.transparency = transparency;
	if ( clip )
		params.clip = *clip;
	else
		params.clip = device.bounds;

	params.stretchNotTile = stretchNotTile;
	
	if ( source.colorTable->version == kYUV32 )
		params.stretchNotTile = false;
		
	params.drawCenter = drawCenter;
	
	start.x = destR.left;
	end.x = destR.right;
	
	start.y = destR.top;
	end.y = destR.top;
	DrawBorderPiece(kTopLeftCorner,&start,nil,&params);
	DrawBorderPiece(kTopEdge,&start,&end,&params);
	DrawBorderPiece(kTopRightCorner,&end,nil,&params);

	start.y = destR.bottom;
	end.y = destR.bottom;
	DrawBorderPiece(kBottomLeftCorner,&start,nil,&params);
	DrawBorderPiece(kBottomEdge,&start,&end,&params);
	DrawBorderPiece(kBottomRightCorner,&end,nil,&params);

	start.y = destR.top;
	end.y = destR.bottom;
	start.x = destR.left;
	end.x = destR.left;
	DrawBorderPiece(kLeftEdge,&start,&end,&params);
	start.x = destR.right;
	end.x = destR.right;
	DrawBorderPiece(kRightEdge,&start,&end,&params);
}

// ===========================================================================
//	Thumbnail image creation
// ===========================================================================


BitMapDevice*  NewThumbnail(const BitMapDevice* src, short scale)
{	
	Rectangle thumbnail;
	thumbnail.top = 0; thumbnail.left = 0;
	thumbnail.bottom = (src->bounds.bottom - src->bounds.top)/scale;
	thumbnail.right = (src->bounds.right - src->bounds.left)/scale;
	
	BitMapDevice* nailDevice = NewBitMapDevice(thumbnail, src->format, src->colorTable, src->transparentColor);

	if ( nailDevice )  {
		CopyImage(*src, *nailDevice, src->bounds, thumbnail, kNotTransparent, nil);
		nailDevice->filter = kFullFilter;
		
		// compress thumbnail to VQ to save about 5 K
		if ( nailDevice->format == yuv422Format )  {
			VectorQuantizer *vq = new(VectorQuantizer);
			vq->Start(nailDevice,nil,nil,64,4);				// 128 codebooks, 4 iterations - can tune for image quality
			
			while ( !vq->Continue(false) )
//				TCPIdle(false);
				;
			
			BitMapDevice *vqnail = vq->GetBitMap(true);		// we own it now
			if ( vqnail) {
				DeleteBitMapDevice(nailDevice);
				nailDevice = vqnail;
			}
			delete(vq);
		}
	}
	return nailDevice;
}




// ===========================================================================
//	Determine acverage color of image
// ===========================================================================

Color AverageImage(const BitMapDevice& device, Color	backgroundColor,const Rectangle *rect,Boolean onlyIfSolid,Color tolerance)
{

	ulong	height = device.bounds.bottom - device.bounds.top;
	ulong	width  = device.bounds.right  - device.bounds.left;
	ulong	pixelCount;
	ulong	red   = 0;
	ulong	green = 0;
	ulong	blue  = 0;
	const uchar*	src,*bsrc   = device.baseAddress;
	ulong	i;
	short	y = 0,u = 0,v = 0;
	uchar	r,g,b;
	uchar	br,bg,bb;			// background color
	ushort	pixel;
	long	transIndex = -1;
	Boolean	firstOne = true;
	Boolean oddPixel,oddPixelStart = false;
	long	toleranceR = (tolerance>>16) & 0xff;
	long	toleranceG = (tolerance>>8) & 0xff;
	long	toleranceB = (tolerance) & 0xff;
	CLUT	*colorTable = device.colorTable;
	uchar	ind;
	
	PIXEL32TORGB(backgroundColor, br, bg, bb);
	if ( device.transparentColor & kTransparent )
		transIndex = (Byte)device.transparentColor & kTransColorMask;
	if ( rect )  {
		Rectangle newRect = *rect;
		IntersectRectangle(&newRect,&device.bounds);
		height = newRect.bottom - newRect.top;
		bsrc += newRect.top * device.rowBytes;	
		bsrc += ((newRect.left<<device.depthShift)>>3);
		width = newRect.right - newRect.left;
		oddPixelStart = newRect.left & 1;
	}
	pixelCount = height * width;
	
	while (height > 0) {
		src = bsrc;
		oddPixel = oddPixelStart;
		for (i=0; i<width; i++) {
		switch (device.format) {
			case rgb32Format:
				{	
					src++; 
					r = *src++;
					g = *src++;
					b = *src++; 
				}
				break;
			case yuvFormat:
				{	
					y = *src++; u= *src++; src++; v  = *src++; 
					YUVTORGB(y,u,v,r,g,b);
				}
				break;
			case yuv422Format:
				{	
					if ( i == width-1 ) {
						if ( (((ulong)src) & 2 ) == 0 ) {
							y = *src++; u= *src++;
						}
						else {
							y = *src++; v= *src++;
						}
					} else {
						if ( (((ulong)src) & 2 ) == 0 ) {
							y = *src++; u= *src++; y += *src; v = src[1];
						}
						else {
							y = *src++; v= *src++; y += *src; u = src[1];
						}
						y += 1;
						y >>= 1; 
					}
					yuvtorgb(y,u,v,&r,&g,&b);		// can't use macro because it nukes v
				}
				break;
			case rgb16Format:
				{
					pixel = *(short*)src;
					src += 2;
					PIXEL16TORGB(pixel, r, g, b);
				}
				break;
			case gray8Format:
				{
					r = g = b = *src++;
				}
				break;
			case index4Format:
				{
					ind = *src;
					if ( oddPixel ) {
						src++;
						ind &= 0xf;
					}
					else
						ind >>= 4;
					if ( ind == transIndex) {	
						r = br; g = bg; b = bb; 
					}
					else {
						if ( colorTable == nil )
							r = g = b = GetCLUT24(&device) [ind];
						else
							LookUpRGB(ind,colorTable,r,g,b);
					}
				}
				break;
				
			case index8Format:
				
				ind = *src++;
				if (device.transparentColor & kTransparent) {
					{	
						if (ind == transIndex) {	
							r = br; g = bg; b = bb; 
						}
						else {
							if ( colorTable == nil )
								r = g = b = GetCLUT24(&device) [ind];
							else
								LookUpRGB(ind,colorTable,r,g,b);
						}
					}
				}
				else {
					if ( colorTable == nil )
						r = g = b = GetCLUT24(&device) [ind];
					else
						LookUpRGB(ind,colorTable,r,g,b);
				}
				break;
			default:
				return backgroundColor;
				break;
			}
			if ( onlyIfSolid ) {
				if ( firstOne ) {
					firstOne = false;
					red = r; green = g; blue = b;
				}
				else {
					if ( ABS((short)r-(short)red) > toleranceR ||  ABS((short)g-(short)green) > toleranceG  || ABS((short)b-(short)blue) > toleranceB )
						return (Color)-1;
				}
			} 
			else  {
				red += r; green += g; blue += b;
			}
			oddPixel = !oddPixel;
		}
		bsrc += device.rowBytes;
		height--;
	}
	if ( onlyIfSolid )
		return (red<<16) + (green<<8) + (blue);
	return ((red / pixelCount) << 16) + ((green / pixelCount) << 8) + (blue / pixelCount);
}


#if	0
static Color
AverageEdgeColor(BitMapDevice& device,const Rectangle *rect)
{
	long	height = rect->bottom - rect->top;
#ifdef	DO_RIGHT_EDGE	
	long	width  = rect->right  - rect->left;
#endif
	long 	i;
	uchar 	*sp,*src = (uchar *)device.baseAddress;
	ulong   y = 0;
	ulong   u = 0;
	ulong   v = 0;

	if ( device.format != yuv422Format )
		return 0x808080;
	src += rect->top * device.rowBytes;
	src += rect->left * 2;
	
	for (i=0; i < height; i++) {
		sp = src;
		y += sp[0] - kBlackY;
		y += sp[2] - kBlackY;
		u += sp[1] - kUVOffset;
		v += sp[3] - kUVOffset;
#ifdef	DO_RIGHT_EDGE	
		sp += (width-2)<<1;
		y += sp[0];
		y += sp[2];
		if ( (ulong)sp & 2) {
			u += sp[1] - kUVOffset;
			v += sp[3] - kUVOffset;
		} else {
			u += sp[3] - kUVOffset;
			v += sp[1] - kUVOffset;
		}
#endif
		src += device.rowBytes;
	}
	
#ifdef	DO_RIGHT_EDGE	
	height <<= 1;
#endif
	u /= (height);
	v /= (height);
	y /= (height<<1);
	y += kBlackY;
	u += kUVOffset;
	v += kUVOffset;
	return (CLIPLUMINANCE(y<<16) + (u<<8) + (v));
}

#endif

// ===========================================================================
//	Primitive drawing operations
// ===========================================================================



void PaintAntiBevel(BitMapDevice& device, const Rectangle& r, const Rectangle* clip, ulong transparency)
{

	Boolean doSoftEdge = !gFilterOnScreenUpdate && ((&device == &gScreenDevice) || (&device == &gOnScreenDevice));
	if (!gSystem->GetUseFlickerFilter())
		doSoftEdge = false;

	if (r.right <= r.left || r.bottom <= r.top)
		return;
		
	ulong softTransparency = (transparency + 255) / 2;

	if ( doSoftEdge )
		PaintLine(device, r.left+1, r.top-1, r.right, r.top-1, 1, kBlackColor, softTransparency, clip);
	PaintLine(device, r.left+1, r.top, r.right, r.top, 1, kBlackColor, transparency, clip);
	if ( doSoftEdge )
		PaintLine(device, r.left+1, r.top+1, r.right, r.top+1, 1, kBlackColor, softTransparency, clip);

	PaintLine(device, r.left, r.bottom, r.left, r.top, 1, kBlackColor, transparency, clip);
	PaintLine(device, r.right, r.top, r.right, r.bottom, 1, kWhiteColor, transparency, clip);

	if ( doSoftEdge )
		PaintLine(device, r.right+1, r.bottom-1, r.left, r.bottom-1, 1, kWhiteColor, softTransparency, clip);
	PaintLine(device, r.right+1, r.bottom, r.left, r.bottom, 1, kWhiteColor, transparency, clip);
	if ( doSoftEdge )
		PaintLine(device, r.right+1, r.bottom+1, r.left, r.bottom+1, 1, kWhiteColor, softTransparency, clip);
}

void PaintBevel(BitMapDevice& device, const Rectangle& r, const Rectangle* clip, ulong transparency)
{
	Boolean doSoftEdge = !gFilterOnScreenUpdate && ((&device == &gScreenDevice) || (&device == &gOnScreenDevice));
	if (!gSystem->GetUseFlickerFilter())
		doSoftEdge = false;
 
	if (r.right <= r.left || r.bottom <= r.top)
		return;

	ulong softTransparency = (transparency + 255) / 2;

	if ( doSoftEdge )
		PaintLine(device, r.left+1, r.top-1, r.right, r.top-1, 1, kWhiteColor, softTransparency, clip);
	PaintLine(device, r.left+1, r.top, r.right, r.top, 1, kWhiteColor, transparency, clip);
	if ( doSoftEdge )
		PaintLine(device, r.left+1, r.top+1, r.right, r.top+1, 1, kWhiteColor, softTransparency, clip);
	
	PaintLine(device, r.left, r.bottom, r.left, r.top, 1, kWhiteColor, transparency, clip);
	PaintLine(device, r.right, r.top, r.right, r.bottom, 1, kBlackColor, transparency, clip);

	if ( doSoftEdge )
		PaintLine(device, r.left+1, r.bottom-1, r.right, r.bottom-1, 1, kBlackColor, softTransparency, clip);
	PaintLine(device, r.right+1, r.bottom, r.left, r.bottom, 1, kBlackColor, transparency, clip);
	if ( doSoftEdge )
		PaintLine(device, r.right+1, r.bottom+1, r.left, r.bottom+1, 1, kBlackColor, softTransparency, clip);
}



#ifdef SIMULATOR
void PaintRoundBevel(BitMapDevice& device, const Rectangle& rect, const Rectangle* clip)
{
	if (rect.right <= rect.left || rect.bottom <= rect.top)
		return;

	Rectangle r = rect;
	r.right++;
	r.bottom++;
	
	OffsetRectangle(r, -1, -1);
	FrameOvalInRectangle(device, r, 1, kWhiteColor, 160, clip);	

	OffsetRectangle(r, 1, 1);
	FrameOvalInRectangle(device, r, 1, kBlackColor, 160, clip);
}

void PaintTriangleBevel(BitMapDevice& device, const Rectangle& rect, const Rectangle* clip)
{
	if (rect.right <= rect.left || rect.bottom <= rect.top)
		return;

	Rectangle r = rect;
	OffsetRectangle(r, -1, -1);
	PaintLine(device, r.left, r.top, r.right, r.top, 1, MakeGrayColor(240), 0, clip);
	PaintLine(device, r.right, r.top, (r.right + r.left) / 2, r.bottom, 1, MakeGrayColor(240), 0, clip);
	PaintLine(device, (r.right + r.left) / 2, r.bottom, r.left, r.top, 1, MakeGrayColor(240), 0, clip);
	
	OffsetRectangle(r, 1, 1);
	PaintLine(device, r.left, r.top, r.right, r.top, 1, MakeGrayColor(128), 0, clip);
	PaintLine(device, r.right, r.top, (r.right + r.left) / 2, r.bottom, 1, MakeGrayColor(128), 0, clip);
	PaintLine(device, r.right, r.top, r.left, r.top, 1, MakeGrayColor(128), 0, clip);
}
#endif


void PaintRectangle(BitMapDevice& device, const Rectangle& r, Color color, ulong transparency, const Rectangle* clip)
{



#ifdef CAN_USE_QUICKDRAW
	if ( !(device.format & 0x8700) && gMacSimulator->GetUseQuickdraw()) {
		PaintRectangleQD(device, r, color, transparency,clip);
		DrawingComplete(r, clip);
		return;
	}
#endif
	{
		Boolean doSoftEdge = !gFilterOnScreenUpdate && ((&device == &gScreenDevice) || (&device == &gOnScreenDevice));
		if (!gSystem->GetUseFlickerFilter())
			doSoftEdge = false;

		Rectangle trimmed = r;
		Rectangle clipped;
		
		if ( clip == nil )
			clipped = device.bounds;
		else
			clipped = *clip;
		IntersectRectangle(&trimmed,&clipped);
		if ( doSoftEdge && (trimmed.bottom - trimmed.top) > 2 && (transparency < 200 ) ) {
			Rectangle tb = trimmed;
			ulong edge = 256 - (256-transparency)/2;
			if ( trimmed.top != 0 )  {
				trimmed.bottom = trimmed.top + 1;
				XPaintRect(device, trimmed, color, edge, clip);
				DrawingComplete(trimmed, clip);
				tb.top += 1;
			}
			XPaintRect(device, tb, color, transparency, clip);
			DrawingComplete(r, clip);
			if ( tb.bottom < (device.bounds.bottom-1) ) {
				tb.top = tb.bottom-1;
			}
			XPaintRect(device, tb, color, edge, clip);
			DrawingComplete(r, clip);
		}
		else {
			XPaintRect(device, trimmed, color, transparency, clip);
			DrawingComplete(r, clip);
		}
	}
}




void PaintOvalInRectangle(BitMapDevice& device, const Rectangle& r, ulong /*width*/, Color color, ulong transparency, const Rectangle* clip)
{
#ifdef CAN_USE_QUICKDRAW
	if ( !(device.format & 0x8700) && gMacSimulator->GetUseQuickdraw()) {
		PaintRectangleQD(device, r, color, transparency,clip);
		DrawingComplete(r, clip);
		return;
	}
#endif
	XPaintRect(device, r, color, transparency, clip);
	DrawingComplete(r, clip);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void FrameOvalInRectangle(BitMapDevice& device, const Rectangle& r, ulong width, Color color, ulong transparency, const Rectangle* clip)
{
#ifdef CAN_USE_QUICKDRAW
	if (!(device.format & 0x8700) && gMacSimulator->GetUseQuickdraw()){
		FrameRectangleQD(device, r, color, transparency,clip);
		DrawingComplete(r, clip);
		return;
	}
#endif
	FrameRectangle(device, r, width, color, transparency, clip);
	DrawingComplete(r, clip);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void FrameRectangle(BitMapDevice& device, const Rectangle& r, ulong width, Color color, ulong transparency, const Rectangle* clip)
{
	// empty check
	if (r.right <= r.left || r.bottom <= r.top) {
		//Complain(("Called FrameRectangle w/ empty rect"));
		return;
	}
	
	Boolean doSoftEdge = !gFilterOnScreenUpdate && ((&device == &gScreenDevice) || (&device == &gOnScreenDevice));
	if (!gSystem->GetUseFlickerFilter())
		doSoftEdge = false;


	Rectangle line = r;
	line.bottom = r.top + width;
	PaintRectangle(device, line, color, transparency, clip);
	line.top = r.bottom - width; line.bottom = r.bottom;
	PaintRectangle(device, line, color, transparency, clip);
	line = r;
	line.right = r.left + width;
	PaintRectangle(device, line, color, transparency, clip);
	line.left = r.right - width; line.right = r.right;
	PaintRectangle(device, line, color, transparency, clip);
	if ( doSoftEdge && transparency < 200 ) {
		ulong edge;
		
		edge = 256 - (256-transparency)/2;
		PaintLine(device, r.left, r.top-1, r.right, r.top-1, 1, color, edge, clip);
		PaintLine(device, r.left, r.top+1, r.right, r.top+1, 1, color, edge, clip);
		PaintLine(device, r.left, r.bottom-1, r.right, r.bottom-1, 1, color, edge, clip);
		PaintLine(device, r.left, r.bottom+1, r.right, r.bottom+1, 1, color, edge, clip);
	}		
}
#endif

void PaintLine(BitMapDevice& device, Ordinate x1, Ordinate y1, Ordinate x2, Ordinate y2, ulong /*width*/, Color color, ulong transparency, const Rectangle* clip)
{
	Ordinate	temp;
	
	if (y2 == y1)	 {
		if (x2 < x1)
			{ temp = x1; x1 = x2; x2 = temp; }
		Rectangle	line;
		line.top = y1; line.left = x1;
		line.bottom = y1+1; line.right = x2;
		PaintRectangle(device, line, color, transparency, clip);
	}
	else if (x2 == x1) {
		if (y2 < y1)
			{ temp = y1; y1 = y2; y2 = temp; }
		Rectangle	line;
		line.top = y1; line.left = x1;
		line.bottom = y2; line.right = x1+1;
		PaintRectangle(device, line, color, transparency, clip);
	}
	else
		Complain(("Trying to draw diagonal line (%d,%d) to (%d,%d)", (int)x1,(int)y1,(int)x2,(int)y2));
}





// ===========================================================================
//	Image drawing
// ===========================================================================



void DrawImage(BitMapDevice& device, const BitMapDevice& source, const Rectangle& destR, ulong transparency, 
		const Rectangle* clip, const Rectangle* srcR,Boolean flipHorizontal,Boolean flipVertical)
{
	// handle nil case by just using bounds
	if (srcR == nil)
		srcR = &source.bounds;
	
	DoCopyImage(source, device, *srcR, destR, transparency, clip,flipHorizontal,flipVertical);
	DrawingComplete(destR, clip);
}


void CopyImage(const BitMapDevice& srcDevice,BitMapDevice& dstDevice,
				const Rectangle& srcRect, const Rectangle& dstRect, ulong transparency, const Rectangle* clip,
				Boolean flipHorizontal,Boolean flipVertical,Boolean cheapFast)
{
	DoCopyImage(srcDevice,dstDevice, srcRect,dstRect,transparency,clip, flipHorizontal, flipVertical,cheapFast);
	DrawingComplete(dstRect, clip);
}



void DoCopyImage(const BitMapDevice& srcDevice,BitMapDevice& dstDevice,
				const Rectangle& srcRect, const Rectangle& dstRect, ulong transparency, const Rectangle* clip,
				Boolean flipHorizontal,Boolean flipVertical,Boolean cheapFast)
{
	BlitProcPtr	blitProc;
	BlitParams2 p;
	long dstWidth,dstHeight;
	long srcWidth,srcHeight;
	long	ca;
	ulong 	temp;
	
	p.cheapFast = cheapFast;
	
#ifdef CAN_USE_QUICKDRAW

	if ( !(srcDevice.format & 0x8700) && !(dstDevice.format & 0x8700) && transparency == 0 &&
		!flipHorizontal &&  !flipVertical) {
		if (  gMacSimulator->GetUseQuickdraw() ) {
			CopyImageQD(srcDevice,dstDevice,srcRect,dstRect,transparency,clip);
			DrawingComplete(dstRect, clip);
			return;
		}
		// hack for fast scrolling on graphics-accelerated PowerMac 9500s 
		else if ( &dstDevice == &gOnScreenDevice && transparency == 0 ) {
			CopyImageQD(srcDevice,dstDevice,srcRect,dstRect,transparency,clip);
			DrawingComplete(dstRect, clip);
			return;
		}
	}
#endif
	
	p.src.device = (BitMapDevice*)&srcDevice;
	p.dst.device = (BitMapDevice*)&dstDevice;

	// start with src calculations
	Ordinate sx1 = srcRect.left;
	Ordinate sy1 = srcRect.top;
	Ordinate sx2 = srcRect.right;
	Ordinate sy2 = srcRect.bottom;

	// localize source coordinates
	sx1 -= srcDevice.bounds.left;
	sx2 -= srcDevice.bounds.left;
	sy1 -= srcDevice.bounds.top;
	sy2 -= srcDevice.bounds.top;

	// dest calculations
	Ordinate dx1 = dstRect.left;
	Ordinate dy1 = dstRect.top;
	Ordinate dx2 = dstRect.right;
	Ordinate dy2 = dstRect.bottom;

	
	Rectangle clippedDevice = dstDevice.bounds;
	
	// devices implicitly clip to zero (needed only for the Mac)
#ifdef FOR_MAC 
	IntersectRectangle(&clippedDevice, &dstDevice.deviceBounds);
#endif
		
	if (clip != nil)
		IntersectRectangle(&clippedDevice, clip);
	
	if ( dx2 < clippedDevice.left || dx1 > clippedDevice.right ||
		 dy2 < clippedDevice.top || dy1 > clippedDevice.bottom)
		 return;
	
	p.dstWidth = dstWidth = dstRect.right-dstRect.left;
	p.dstHeight = dstHeight = dstRect.bottom-dstRect.top;
	p.srcWidth = srcWidth = srcRect.right-srcRect.left;
	p.srcHeight = srcHeight = srcRect.bottom-srcRect.top;
	if ( (p.srcWidth == 1 && p.srcHeight == 1 ) ||
		 ( p.srcWidth < 32 && p.srcHeight < 32 ) ) {
		 if ( srcDevice.format != alpha8Format && srcDevice.format != antialias8Format && srcDevice.format != antialias4Format 
#ifdef SPOT1
		 	&& dstDevice.format != yuv422BucherFormat 
#endif
		 	 ) {
			Color color = AverageImage(srcDevice,0x123456,&srcRect,true,0);
#if	1
			if ( color != (Color)-1 ) {	
				if  ( srcDevice.transparentColor && color == 0x123456 ) {
					if ( AverageImage(srcDevice,0x89abcd,&srcRect,true,0) == 0x89abcd )
						goto done;
				} else {
					XPaintRect(dstDevice, dstRect, color, transparency, clip);
				}
				goto done;
			}
#endif
		}
	}
	p.topSkipSrc = 0;
	p.topSkipDst = 0;
	p.leftSkipSrc = 0;
	p.leftSkipDst = 0;
	
	// clip dest to device bounds and adjust source if neccessary
	if (dx1 < clippedDevice.left) {
		p.leftSkipSrc = p.leftSkipDst = clippedDevice.left - dx1;
		if ( dstWidth != srcWidth )
			p.leftSkipSrc = SCALE_ORD(p.leftSkipDst,srcWidth,dstWidth);
		sx1 += p.leftSkipSrc;
		dx1 = clippedDevice.left;
	}
	if (dy1 < clippedDevice.top) {
		p.topSkipDst = p.topSkipSrc = clippedDevice.top - dy1;
		if ( dstHeight != srcHeight ) 
			p.topSkipSrc = SCALE_ORD(p.topSkipDst,srcHeight,dstHeight);
		sy1 += p.topSkipSrc;
		dy1 = clippedDevice.top;
	}
	if (dx2 > clippedDevice.right)  {
		ca = clippedDevice.right - dx2;
		if ( dstWidth != srcWidth )
			ca = SCALE_ORD(ca,srcWidth,dstWidth);
		sx2 += ca;
		dx2 = clippedDevice.right;
	}
	if (dy2 > clippedDevice.bottom)  {
		ca = clippedDevice.bottom - dy2;
		if ( dstHeight != srcHeight )
			ca = SCALE_ORD(ca,srcHeight,dstHeight);
		sy2 += ca;
		dy2 = clippedDevice.bottom;
	}

	// empty check
	if (dx2 <= dx1 || dy2 <= dy1)
		return;

	// localize dest coordinates
	dx1 -= dstDevice.bounds.left;
	dx2 -= dstDevice.bounds.left;
	dy1 -= dstDevice.bounds.top;
	dy2 -= dstDevice.bounds.top;

	p.src.xInc = 1;
	if  ( flipHorizontal )  {
		long sdw = srcDevice.bounds.right - srcDevice.bounds.left;
		long t = sx1;
		sx1 = sdw - sx2;
		sx2 = sdw - t;
		p.src.xInc = -1;
	}
	if  ( flipVertical )  {
		long sdh = srcDevice.bounds.bottom - srcDevice.bounds.top;
		long t = sy1;
		sy1 = sdh - sy2;
		sy2 = sdh - t;
	}


	SetRectangle(p.src.r, sx1, sy1, sx2, sy2);
	SetRectangle(p.dst.r, dx1, dy1, dx2, dy2);

	// turn x ords into bit positions
	// --------------------------------
	p.src.leftBits  = sx1 << srcDevice.depthShift;
	p.dst.leftBits  = dx1 << dstDevice.depthShift;
	p.dst.rightBits = dx2 << dstDevice.depthShift;

	
	if (srcDevice.baseAddress == dstDevice.baseAddress && dy1 > sy1) {
			// must blit BOTTOM -> TOP
		p.src.bump = -srcDevice.rowBytes;
		p.src.base = (ulong*)(srcDevice.baseAddress + (srcDevice.rowBytes * (sy2-1)));
		p.dst.bump = -dstDevice.rowBytes;
		p.dst.base = (ulong*)(dstDevice.baseAddress + (dstDevice.rowBytes * (dy2-1)));
	}
	else	// TOP -> BOTTOM
	{
		
		p.src.bump = srcDevice.rowBytes;
		if ( srcDevice.format == vqFormat ) {
			p.src.base = (ulong*)(srcDevice.baseAddress + (srcDevice.rowBytes * ((sy1)/2)));
		}
		else {
			p.src.base = (ulong*)(srcDevice.baseAddress + (srcDevice.rowBytes * sy1));
		}
		p.dst.bump = dstDevice.rowBytes;
		p.dst.base = (ulong*)(dstDevice.baseAddress + (dstDevice.rowBytes * dy1));
	}
	
	if ( flipVertical )
	{
		p.src.bump = -p.src.bump;
		if ( p.src.bump < 0 )
			p.src.base = (ulong*)(srcDevice.baseAddress + (srcDevice.rowBytes * (sy2-1)));
		else
			p.src.base = (ulong*)(srcDevice.baseAddress + (srcDevice.rowBytes * sy1));
	}
	
	// adjust for x-starting positions and compute count of longs
	p.src.base += p.src.leftBits >> 5;
	p.dst.base += p.dst.leftBits >> 5;
	p.dst.longCount = (p.dst.rightBits >> 5) - (p.dst.leftBits >> 5);

	// compute edge masks		
	p.dst.leftMask = 0xffffffff << (p.dst.leftBits & 31);
	p.dst.rightMask = 0xffffffff;

	temp = (32 - (p.dst.rightBits & 31));
	
	if (temp == 32)								// handle flush right edge case
		p.dst.longCount--;
	else
		p.dst.rightMask <<= temp;
		

	// special cases 
	// ----- stretch or shrink
	
	if (  (dstWidth !=  srcWidth) ||  (dstHeight != srcHeight) 
#ifdef	SIMULATOR
		 ||	 (srcDevice.format == index4Format && (dstDevice.format == rgb32Format ||dstDevice.format == yuvFormat)  )
#endif
	) 
		blitProc = ResizeBlit;

	else if ( ( (dstDevice.filter & kCopyFilter) != 0 ) && gFilterOnScreenUpdate )
		blitProc = LookupBlitProc(&srcDevice,&dstDevice,transparency);
	else {
		// ----- YUV422 with mis-aligned u and v
	
		if (srcDevice.format == yuv422Format && dstDevice.format == yuv422Format && ((p.src.leftBits >> 4) & 1) != (p.dst.leftBits >> 4) & 1) 
			blitProc = ScaleBlitYUYVtoYUYV;
	
		// ----- no transparency fast blast
	
		else if (srcDevice.format == dstDevice.format && transparency == kNotTransparent && !flipHorizontal && !flipVertical 
#ifdef SPOT1
			&& srcDevice.format != yuv422BucherFormat 
#endif
			&& dstDevice.filter == kNoFilter)
			blitProc = CopyBlitFast;
		else 
			blitProc = LookupBlitProc(&srcDevice,&dstDevice,transparency);
	}
	
	if ( blitProc == nil )
	{
		// this case is slow so we should not use it
		PostulateFinal(false);		
		if ( IsWarning(blitProc == nil) )
			;
		blitProc = ResizeBlit;
	}
	if ( blitProc == nil )
	{
		Complain(("Format conversion %d to %d not supported",srcDevice.format,dstDevice.format));
		return;
	}
	
	blitProc(p,transparency);

done:
	if ( !gFilterOnScreenUpdate ) {
	
		// flicker elimination filter

		if ( srcDevice.filter != 0 ) {
			if (  gSystem->GetUseFlickerFilter() ) {
				if ( dstDevice.format == yuv422Format ) 
					NewInterlaceFilterYUYV(p.dst,srcDevice.filter);
			}
		}
	}
}
