// ===========================================================================
//	jpDrawing.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __JPINCLUDES_H__
#include "jpIncludes.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __VECTORQUANTIZATION_H__
#include "VectorQuantization.h"
#endif

#ifdef SIMULATOR
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
#endif


const	kDCScale = 3;

#ifdef	SIMULATOR
	#define	SCREENBUFFER_FORMAT	 (gSimulator->GetScreenBitMapFormat())
	#ifdef	DRAW_YUV444
		const		kBufferFormat = yuvFormat;
	#else
		#define		kBufferFormat 	(( SCREENBUFFER_FORMAT == rgb16Format  || SCREENBUFFER_FORMAT == rgb32Format ) ?   rgb32Format : yuv422Format)
	#endif
#else
	const	BitMapFormat kBufferFormat	= yuv422Format;
#endif



// ===========================================================================
//	local helper functions

static short MakeYUVSlice(JPEGDecoder *j, BitMapDevice* w, long width, long height,
						  long leftSkip, long topSkip, long leftOff,long topOff);


//====================================================================
//	Allocate memory to store some or all of each component
//

Error SetupDrawing(JPEGDecoder *j)
{
	short	i,s = 3;
	CompSpec	*cspec;
	
	//Message(("SetupDrawing"));
	
	if (j->thumbnail) 
		s = 0;
	
	for (i = 0; i < j->CompInFrame; i++) {
		cspec = &j->Comp[i];
				
		//Message(("SetupDrawing %dx%d mcu jpeg component %d (%dx%d)",	j->WidthMCU,j->HeightMCU,cspec->Ci,cspec->Hi,cspec->Vi));
		
		if ( j->buffer[i] ) {
			if ( IsError(j->rowBytes[i] != (cspec->Hi<< s) * j->WidthMCU) ) {
				Message(("rowbytes changed %ld %d",j->rowBytes[i],(cspec->Hi<< s) * j->WidthMCU));
				return kGenericError;
			}	
		} else {
			j->rowBytes[i] = (cspec->Hi<< s) * j->WidthMCU;
			if (j->SingleRowProc != nil)
			{	
				j->buffer[i] = (Byte *)AllocateTaggedMemory((cspec->Vi << s) * j->rowBytes[i], "JPEG Buffer");
			}
			else
			{	
				j->buffer[i] = (Byte *)AllocateTaggedMemory((cspec->Vi << s) * j->rowBytes[i] * j->HeightMCU, "JPEG Buffer");
			}
			if (j->buffer[i] == 0) {
				Message(("JPEG: no memory for decode buffer"));
				return kLowMemory;
			}
		}
	}
	return kNoError;
}



void
SetJPEGGamma(JPEGDecoder* j,Gamma gamma,uchar blackLevel,uchar whiteLevel)
{
	j->fGamma = gamma;
	j->fBlackLevel = blackLevel;
	j->fWhiteLevel = whiteLevel;
}


#if GENERATING68K 

#define	NON_RISC

#endif


#ifdef	NON_RISC
	#define	CLIPBYTE(_x)				((_x) < 0 ? 0 : ( (_x) > 255 ? 255 : (_x)) )	
#else
	#define	CLIPBYTE(_x)				(((_x) & (~((_x) >> 9))) | (((ushort)(0xff - (_x))) >> 8))
#endif



#define	ADJUSTLUMINANCE(_y)		((((short)(_y) * (short)kRangeY)>>8) + kBlackY)
#define	ADJUSTCHROMINANCE(_uv)	(_uv)



static inline uchar clipByte(short x)
{
#ifdef	NON_RISC
	if ( x < 0 )
		return 0;
	if ( x > 255 )
		return 255;
	return x;
#else
// assume a few logic instructions are better than compare/branches
// depends on processor/instruction cache/compiler/

	return ( (x & (~(x >> 9))) | (((ushort)(0xff - x)) >> 8));
#endif
}

//	Draw a block into a component
//  only used by DrawMCU in one place so inline is free 

static inline void	
DrawBlock(uchar *dst, long rowBytes, short *block)
{
	register int	x,y;
	register short  a,b,c,d;
	register ulong pix;
	
	rowBytes -= 8;
	for (y = kMCUHeight;y--; ) 
	{
		for (x =kMCUWidth/4; x--; ) 
		{
			a = 128 + *block++;
			b = 128 + *block++;
			c = 128 + *block++;
			d = 128 + *block++;
			pix = CLIPBYTE(a);
			pix <<= 8;
			pix |= CLIPBYTE(b);
			pix <<= 8;
			pix |= CLIPBYTE(c);
			pix <<= 8;
			pix |= CLIPBYTE(d);
			*(long *)dst = pix;
			dst += 4;
		}
		dst += rowBytes;
	}
}

// Draw all components into their buffers

void	DrawMCU(JPEGDecoder *j, short	*blocks,long h,long v)
{
	long		x,y,i;
	uchar	*dst,*mcu;
	long	rowBytes;			
	long  	mcuRowBytes;
	long 	xs;
	CompSpec *cspec;

		
	if ( j->SingleRowProc != nil) 
		v = 0;				// set v to zero to draw into single line buffer
	if ( j->thumbnail ) 
	{
		
		for (i = 0; i < j->CompInFrame; i++) 
		{
			cspec = &j->Comp[i];
			int dcScale = kDCScale;
			
			dcScale -= (cspec->dcBitPos<<kDCScale);
			if ( dcScale < 0 )
				dcScale = 0;
#ifdef	kScaledDCT
			dcScale <<= 1;
#endif
			rowBytes = j->rowBytes[i];
			xs = cspec->Hi;
			mcu = j->buffer[i] + h * xs;
			if ( v ) 
				mcu += rowBytes * v * cspec->Vi;
			for ( y = cspec->Vi; y--; mcu += rowBytes ) 
			{
				dst = mcu;
				for (x = xs; x--; blocks += kMCUSize ) 
				{
					short q = 128 + (*blocks >> dcScale);
					*dst++ = CLIPBYTE(q);
				}
			}
		}
	} 
	else 
	{
		h <<= 3;
		v <<= 3;
		
		for (i = 0; i < j->CompInFrame; i++) 
		{
			cspec = &j->Comp[i];
			rowBytes = j->rowBytes[i];
			mcuRowBytes = rowBytes<<3;
			xs = cspec->Hi;
			mcu = j->buffer[i] + h * xs;
			if ( v )
				rowBytes *= ( v * cspec->Vi);
			for (y = cspec->Vi; y--; mcu += mcuRowBytes ) 
			{
				dst = mcu;
				for (x = xs; x--; dst += kMCUWidth, blocks += kMCUSize) 
					DrawBlock(dst,rowBytes,blocks);
			}
		}
	}
}

/*

	Draw a single component
	
*/

void
DrawMCUPiece(JPEGDecoder *j, short	*blocks,long h,long v)
{

	long		x,i;
	uchar	*dst,*mcu;
	long	rowBytes;			
	CompSpec *cSpec;

	
	
	h <<= 3;
		
	if ( !j->didClearDrawBuffer ) 
	{
		for (i = 0; i < j->CompInFrame; i++) 
		{
			rowBytes = j->rowBytes[i];
			mcu = j->buffer[i];
			for ( x=0; x < rowBytes*8; x++ )
				mcu[x] = i == 0 ? 0 : 128;
		}
		j->didClearDrawBuffer = true;
	}
	
	for (i = 0; i < j->CompInFrame; i++) 
	{
		cSpec = &j->Comp[i];
		rowBytes = j->rowBytes[i];
		mcu = j->buffer[i];
		if ( cSpec->inScan  ) 
		{
			if ( IsError(h < 0) )
				return;
			if ( IsError(h >= rowBytes) )
				return;
			mcu  += h;
			if ( v % cSpec->Vi )
				dst = mcu + rowBytes * 8;
			else
				dst = mcu;
			DrawBlock(dst,rowBytes,blocks);
		}
	}
}




#ifndef	DRAW_YUV444		// default is 422

// depends on the frame buffer hardware

// assumes frame buffer is 2:11 8-bits/component Y Cr Y Cb

static short
MakeYUVSlice(JPEGDecoder *j, BitMapDevice* w, long width, long height, 
			 long leftSkip, long topSkip, long leftOff,long topOff)
{
	long		x,yy;
	uchar		u,v;
	uchar		y1,y2,t;
	ulong		pix;
	uchar	*ySrc,*ySrc2,*uSrc,*vSrc,*dst,*dst2;
	long	rowBump,yRowBump,uRowBump,vRowBump;
	Boolean	swapUV = false;
	uchar 	*gammaTable = nil;

	if ( j->fGamma )
		gammaTable = BuildGammaTable(j->fGamma,j->fBlackLevel,j->fWhiteLevel, kBlackY,kWhiteY);
	
//	Draw the image into gWorld

	dst = w->baseAddress;
	dst += topOff * w->rowBytes;
	dst += leftOff << 1;
	if ( leftOff & 2 )
		swapUV = true;
	
	ySrc = j->buffer[0];
	uSrc = j->buffer[1];
	vSrc = j->buffer[2];
	
	// еее other sampling factors may be needed ( or handle it on server with transcoding )
	
	switch (j->sampling) 
	{
		case	0x0000:								// Grayscale
			rowBump = w->rowBytes - (width<<1);
			yRowBump = j->rowBytes[0] - width;
			ySrc += leftSkip;
			if ( topSkip )
				 ySrc += topSkip * j->rowBytes[0];
			for (yy =  height; yy--; ) 
			{
				for (x = width; x--; ) 
				{
					y1 = *ySrc++;
					if ( gammaTable )
						y1 = gammaTable[y1];
					else
						y1 = ADJUSTLUMINANCE(y1);
					*(ushort *)dst = ( y1 << 8) + kUVOffset;
					dst  += 2;
				}
				dst += rowBump;
				ySrc += yRowBump;
			}
			break;
		case	0x0011:	
			yRowBump = j->rowBytes[0] - width;
			uRowBump = j->rowBytes[1] - width;
			vRowBump = j->rowBytes[2] - width;
			rowBump = w->rowBytes - (width<<1);
			ySrc += leftSkip;
			uSrc += leftSkip;
			vSrc += leftSkip;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip * j->rowBytes[1];
				vSrc += topSkip * j->rowBytes[2];
			}
			for (yy = height; yy--; )   {
				for (x = width>>1; x --; ) {
					y1 = *ySrc++;
					y2 = *ySrc++;
					u = *uSrc++;
					u = ( u + *uSrc++) >> 1;
					v = *vSrc++;
					v = ( v + *vSrc++) >> 1;
					if ( swapUV ) 
					{
						t = v;
						v = u;
						u = t;
					}
					if ( gammaTable )
						pix = gammaTable[y1];
					else
					pix = ADJUSTLUMINANCE(y1);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(u);
					pix <<= 8;
					if ( gammaTable )
						pix |= gammaTable[y2];
					else
					pix |=  ADJUSTLUMINANCE(y2);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(v);
					*(ulong *)dst = pix;
					dst += 4;
				}
				ySrc += yRowBump;
				uSrc += uRowBump;
				vSrc += vRowBump;
				dst += rowBump;
			}
			break;
		case	0x0021:								// 2:1 RGB
		
		
			yRowBump = j->rowBytes[0] - width;
			uRowBump = j->rowBytes[1] - width/2;
			vRowBump = j->rowBytes[2] - width/2;;
			rowBump = w->rowBytes - (width<<1);
			ySrc += leftSkip;
			uSrc += leftSkip/2;
			vSrc += leftSkip/2;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip * j->rowBytes[1];
				vSrc += topSkip * j->rowBytes[2];
			}
			
			for (yy = height; yy--; ) 
			{
				for (x = (width >> 1); x--; ) 
				{
					y1 = *ySrc++;
					y2 = *ySrc++;
					u = *uSrc++;
					v = *vSrc++;
					if ( swapUV ) 
					{
						t = v;
						v = u;
						u = t;
					}
					if ( gammaTable )
						pix = gammaTable[y1];
					else
					pix = ADJUSTLUMINANCE(y1);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(u);
					pix <<= 8;
					if ( gammaTable )
						pix |= gammaTable[y2];
					else
					pix |=  ADJUSTLUMINANCE(y2);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(v);
					*(ulong *)dst = pix;
					dst += 4;
				}
				ySrc += yRowBump;
				uSrc += uRowBump;
				vSrc += vRowBump;
				dst += rowBump;
			}
			break;
		case	0x0022:								// 2:2 RGB

			rowBump = 2 * w->rowBytes - (width<<1);
			yRowBump = 2 * j->rowBytes[0] - width;		// 2 rows at a time
			uRowBump = j->rowBytes[1] - width/2;
			vRowBump = j->rowBytes[2] - width/2;;
			ySrc += leftSkip;
			uSrc += leftSkip/2;
			vSrc += leftSkip/2;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip/2 * j->rowBytes[1];
				vSrc += topSkip/2 * j->rowBytes[2];
			}
			dst2 = dst + w->rowBytes;
			ySrc2 = ySrc + j->rowBytes[0];
			
			for (yy = (height >> 1) ; yy-- ; ) 
			{
				for (x = (width >> 1); x-- ; ) 
				{
					y1 = *ySrc++;
					y2 = *ySrc++;
					uchar y3 = *ySrc2++;
					uchar y4 = *ySrc2++;
					if ( swapUV ) 
					{
						v = *uSrc++;
						u = *vSrc++;
					} 
					else 
					{
						u = *uSrc++;
						v = *vSrc++;
					}
					if ( gammaTable )
						pix = gammaTable[y1];
					else
					pix = ADJUSTLUMINANCE(y1);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(u);
					pix <<= 8;
					if ( gammaTable )
						pix |= gammaTable[y2];
					else
					pix |=  ADJUSTLUMINANCE(y2);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(v);
					*(ulong *)dst = pix;
					dst += 4;
					if ( gammaTable )
						pix = gammaTable[y3];
					else
					pix = ADJUSTLUMINANCE(y3);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(u);
					pix <<= 8;
					if ( gammaTable )
						pix |= gammaTable[y4];
					else
					pix |=  ADJUSTLUMINANCE(y4);
					pix <<= 8;
					pix |= ADJUSTCHROMINANCE(v);
					*(ulong *)dst2 = pix;
					dst2 += 4;
				}
				uSrc += uRowBump;
				vSrc += vRowBump;
				ySrc += yRowBump;
				ySrc2 += yRowBump;
				dst += rowBump;
				dst2 += rowBump;
			}
			break;
	}
	return 0;
}

#else

static short
MakeYUVSlice(JPEGDecoder *j, BitMapDevice* w, long width, long height, 
			 long leftSkip, long topSkip, long leftOff,long topOff)
{
	long		x,yy;
	short	y,u,v,y1,y2;
	
	uchar	*ySrc,*ySrc2,*uSrc,*vSrc,*dst,*dst2;
	long	rowBump,yRowBump,uRowBump,vRowBump;
	uchar 	*gammaTable = nil;

	if ( j->fGamma )
		gammaTable = BuildGammaTable(j->fGamma,j->fBlackLevel,j->fWhiteLevel);
	
//	Draw the image into gWorld

	dst = w->baseAddress;
	dst += topOff * w->rowBytes;
	dst += leftOff << 2;
	
	ySrc = j->buffer[0];
	uSrc = j->buffer[1];
	vSrc = j->buffer[2];
	
	// еее other sampling factors may be needed ( or handle it on server with transcoding )
	
	switch (j->sampling) 
	{
		case	0x0000:								// Grayscale
			rowBump = w->rowBytes - (width<<2);
			yRowBump = j->rowBytes[0] - width;
			ySrc += leftSkip;
			if ( topSkip )
				 ySrc += topSkip * j->rowBytes[0];
			for (yy =  height; yy--; ) 
			{
				for (x = width; x--; ) 
				{
					y = *ySrc++;
					if ( gammaTable )
						*dst++ = gammaTable[y];
					else
					*dst++ = ADJUSTLUMINANCE(y);
					*dst++ = kUVOffset;
					*dst++ = 0;
					*dst++ = kUVOffset;
				}
				dst += rowBump;
				ySrc += yRowBump;
			}
			break;
		case	0x0011:	
			yRowBump = j->rowBytes[0] - width;
			uRowBump = j->rowBytes[1] - width;
			vRowBump = j->rowBytes[2] - width;
			rowBump = w->rowBytes - (width<<2);
			ySrc += leftSkip;
			uSrc += leftSkip;
			vSrc += leftSkip;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip * j->rowBytes[1];
				vSrc += topSkip * j->rowBytes[2];
			}
			for (yy = height; yy--; )   
			{
				for (x = width; x --; ) 
				{
					y = *ySrc++;
					u = *uSrc++;
					v = *vSrc++;
					if ( gammaTable )
						*dst++ = gammaTable[y];
					else
					*dst++ = ADJUSTLUMINANCE(y);
					*dst++ = ADJUSTCHROMINANCE(u);
					*dst++ = 0;
					*dst++ = ADJUSTCHROMINANCE(v);
				}
				ySrc += yRowBump;
				uSrc += uRowBump;
				vSrc += vRowBump;
				dst += rowBump;
			}
			break;
		case	0x0021:								// 2:1 RGB
		
		
			yRowBump = j->rowBytes[0] - width;
			uRowBump = j->rowBytes[1] - width/2;
			vRowBump = j->rowBytes[2] - width/2;;
			rowBump = w->rowBytes - (width<<2);
			ySrc += leftSkip;
			uSrc += leftSkip/2;
			vSrc += leftSkip/2;
			if ( topSkip ) {
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip * j->rowBytes[1];
				vSrc += topSkip * j->rowBytes[2];
			}
			
			for (yy = height; yy--; ) 
			{
				for (x = (width >> 1); x--; ) 
				{
					y1 = *ySrc++;
					y2 = *ySrc++;
					u = *uSrc++;
					v = *vSrc++;
					if ( gammaTable )
						*dst++ = gammaTable[y1];
					else
					*dst++ = ADJUSTLUMINANCE(y1);
					*dst++ = ADJUSTCHROMINANCE(u);
					*dst++ = 0;
					*dst++ = ADJUSTCHROMINANCE(v);
					if ( gammaTable )
						*dst++ = gammaTable[y2];
					else
					*dst++ = ADJUSTLUMINANCE(y2);
					*dst++ = ADJUSTCHROMINANCE(u);
					*dst++ = 0;
					*dst++ = ADJUSTCHROMINANCE(v);
				}
				ySrc += yRowBump;
				uSrc += uRowBump;
				vSrc += vRowBump;
				dst += rowBump;
			}
			break;
		case	0x0022:								// 2:2 RGB

			rowBump = 2 * w->rowBytes - (width<<2);
			yRowBump = 2 * j->rowBytes[0] - width;		// 2 rows at a time
			uRowBump = j->rowBytes[1] - width/2;
			vRowBump = j->rowBytes[2] - width/2;;
			ySrc += leftSkip;
			uSrc += leftSkip/2;
			vSrc += leftSkip/2;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip/2 * j->rowBytes[1];
				vSrc += topSkip/2 * j->rowBytes[2];
			}
			dst2 = dst + w->rowBytes;
			ySrc2 = ySrc + j->rowBytes[0];
			
			for (yy = (height >> 1) ; yy-- ; ) 
			{
				for (x = (width >> 1); x-- ; ) 
				{
					y1 = *ySrc++;
					y2 = *ySrc++;
					u = *uSrc++;
					v = *vSrc++;
					if ( gammaTable )
						*dst++ = gammaTable[y1];
					else
					*dst++ = ADJUSTLUMINANCE(y1);
					*dst++ = ADJUSTCHROMINANCE(u);
					*dst++ = 0;
					*dst++ = ADJUSTCHROMINANCE(v);
					if ( gammaTable )
						*dst++ = gammaTable[y2];
					else
					*dst++ = ADJUSTLUMINANCE(y2);
					*dst++ = ADJUSTCHROMINANCE(u);
					*dst++ = 0;
					*dst++ = ADJUSTCHROMINANCE(v);
					y1 = *ySrc2++;
					y2 = *ySrc2++;
					if ( gammaTable )
						*dst2++ = gammaTable[y1];
					else
					*dst2++ = ADJUSTLUMINANCE(y1);
					*dst2++ = ADJUSTCHROMINANCE(u);
					*dst2++ = 0;
					*dst2++ = ADJUSTCHROMINANCE(v);
					if ( gammaTable )
						*dst2++ = gammaTable[y2];
					else
					*dst2++ = ADJUSTLUMINANCE(y2);
					*dst2++ = ADJUSTCHROMINANCE(u);
					*dst2++ = 0;
					*dst2++ = ADJUSTCHROMINANCE(v);
				}
				uSrc += uRowBump;
				vSrc += vRowBump;
				ySrc += yRowBump;
				ySrc2 += yRowBump;
				dst += rowBump;
				dst2 += rowBump;
			}
			break;
	}
	return 0;
}

#endif


#ifdef	SIMULATOR

#define RGBOUT(_d,_y,_u,_v,_uvg)								\
				{register ulong pix;register short t;		\
				t = _y + _v;							\
				pix = CLIPBYTE(t); pix<<=16;				\
				t = _y-_uvg;							\
				pix |= ((ulong) CLIPBYTE(t))<<8;			\
				t = _y + _u;								\
				pix |= CLIPBYTE(t);							\
				*(long *)(_d) = pix;						\
				(_d) += 4;									\
				}

#define RGBGAMMAOUT(_d,_y,_u,_v,_uvg,_gt)								\
				{register ulong pix;register short t;		\
				t = _y + _v;							\
				pix = (_gt)[CLIPBYTE(t)]; pix<<=16;				\
				t = _y-_uvg;							\
				pix |= ((ulong) (_gt)[CLIPBYTE(t)])<<8;			\
				t = _y + _u;								\
				pix |= (_gt)[CLIPBYTE(t)];							\
				*(long *)(_d) = pix;						\
				(_d) += 4;									\
				}

#define GRAYOUT(_d,_y)											\
				*(long *)(_d) = ((long)_y<<16) + (_y<<8) + (_y);		\
				(_d) += 4;



 // R4640 smul = 3/2 cycles, shift/add = 1 cycles, sdiv = 36 cycles
 // ideal:	v = 1.402v   u = 1.777u,  uvg = (.3437u + .7143v)
 
 // approx:	v = 1.402v	 u = 1.773u,  uvg = (.343u + .714 v)


#define	PREPUV(_u,_v,_uvg)					\
		_u =	(455 * _u) >> 8;			\
		_v = 	(359 * _v) >> 8 ;			\
		_uvg =  (88 * _u + 183 * _v) >> 8;


//	Make a slice of a picture from buffer
static short MakeRGBSlice(JPEGDecoder *j, BitMapDevice* w, long width, long height,
						  long leftSkip, long topSkip, long leftOff,long topOff);


static short
MakeRGBSlice(JPEGDecoder *j, BitMapDevice* w, long width, long height, 
			 long leftSkip, long topSkip, long leftOff,long topOff)
{
	int		x,yy;
	short	y,u,v,uvg;
	short	y2;
	
	uchar	*ySrc,*ySrc2,*uSrc,*vSrc,*dst,*dst2;
	long	rowBump,yRowBump,uRowBump,vRowBump;
	uchar 	*gammaTable = nil;
	
	if ( j->fGamma )
		gammaTable = BuildGammaTable(j->fGamma,j->fBlackLevel,j->fWhiteLevel);
	
		
//	Draw the image into gWorld

	dst = w->baseAddress;
	dst += topOff * w->rowBytes;
	dst += leftOff << 2;
	
	ySrc = j->buffer[0];
	uSrc = j->buffer[1];
	vSrc = j->buffer[2];
	
	
	// еее other sampling factors may be needed ( or handle it on server with transcoding )
	
	switch (j->sampling) {
		case	0x0000:								// Grayscale
			rowBump = w->rowBytes - (width<<2);
			yRowBump = j->rowBytes[0] - width;
			ySrc += leftSkip;
			if ( topSkip )
				 ySrc += topSkip * j->rowBytes[0];
			for (yy =  height; yy--; ) 
			{
				for (x = width; x--; ) 
				{
					y = *ySrc++;
					GRAYOUT(dst,y);
				}
				dst += rowBump;
				ySrc += yRowBump;
			}
			break;
		case	0x0011:								// 1:1 RGB
		
			yRowBump = j->rowBytes[0] - width;
			uRowBump = j->rowBytes[1] - width;
			vRowBump = j->rowBytes[2] - width;
			rowBump = w->rowBytes - (width<<2);
			ySrc += leftSkip;
			uSrc += leftSkip;
			vSrc += leftSkip;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip * j->rowBytes[1];
				vSrc += topSkip * j->rowBytes[2];
			}
			for (yy = height; yy--; )   
			{
				for (x = width; x --; ) 
				{
					y = *ySrc++;
					u = *uSrc++ - 128;
					v = *vSrc++ - 128;
					PREPUV(u,v,uvg);
					if ( gammaTable ) {
						RGBGAMMAOUT(dst,y,u,v,uvg,gammaTable);
					} else {
					RGBOUT(dst,y,u,v,uvg);
				}
				}
				ySrc += yRowBump;
				uSrc += uRowBump;
				vSrc += vRowBump;
				dst += rowBump;
			}
			break;
		case	0x0021:								// 2:1 RGB
		
		
			yRowBump = j->rowBytes[0] - width;
			uRowBump = j->rowBytes[1] - width/2;
			vRowBump = j->rowBytes[2] - width/2;;
			rowBump = w->rowBytes - (width<<2);
			ySrc += leftSkip;
			uSrc += leftSkip/2;
			vSrc += leftSkip/2;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip * j->rowBytes[1];
				vSrc += topSkip * j->rowBytes[2];
			}
			for (yy = height; yy--; ) 
			{
				for (x = (width >> 1); x--; ) 
				{
					y = *ySrc++;
					y2 = *ySrc++;
					u = *uSrc++ - 128;
					v = *vSrc++ - 128;
					PREPUV(u,v,uvg);
					if ( gammaTable ) {
						RGBGAMMAOUT(dst,y,u,v,uvg,gammaTable);
						RGBGAMMAOUT(dst,y2,u,v,uvg,gammaTable);
					} else {
					RGBOUT(dst,y,u,v,uvg);
					RGBOUT(dst,y2,u,v,uvg);
				}
				}
				ySrc += yRowBump;
				uSrc += uRowBump;
				vSrc += vRowBump;
				dst += rowBump;
			}
			break;
		case	0x0022:								// 2:2 RGB

			rowBump = 2 * w->rowBytes - (width<<2);
			yRowBump = 2 * j->rowBytes[0] - width;		// 2 rows at a time
			uRowBump = j->rowBytes[1] - width/2;
			vRowBump = j->rowBytes[2] - width/2;;
			ySrc += leftSkip;
			uSrc += leftSkip/2;
			vSrc += leftSkip/2;
			if ( topSkip ) 
			{
				ySrc += topSkip * j->rowBytes[0];
				uSrc += topSkip/2 * j->rowBytes[1];
				vSrc += topSkip/2 * j->rowBytes[2];
			}
			dst2 = dst + w->rowBytes;
			ySrc2 = ySrc + j->rowBytes[0];
			
			for (yy = (height >> 1) ; yy-- ; )
			{
				for (x = (width >> 1); x-- ; ) 
				{
					u = *uSrc++ - 128;
					v = *vSrc++ - 128;
					PREPUV(u,v,uvg);
					y = *ySrc++;
					y2 = *ySrc++;
					if ( gammaTable ) {
						RGBGAMMAOUT(dst,y,u,v,uvg,gammaTable);
						RGBGAMMAOUT(dst,y2,u,v,uvg,gammaTable);
					} else {
					RGBOUT(dst,y,u,v,uvg);
					RGBOUT(dst,y2,u,v,uvg);
					}
					y = *ySrc2++;
					y2 = *ySrc2++;
					if ( gammaTable ) {
						RGBGAMMAOUT(dst,y,u,v,uvg,gammaTable);
						RGBGAMMAOUT(dst,y2,u,v,uvg,gammaTable);
					} else {
					RGBOUT(dst2,y,u,v,uvg);
					RGBOUT(dst2,y2,u,v,uvg);
				}
				}
				uSrc += uRowBump;
				vSrc += vRowBump;
				ySrc += yRowBump;
				ySrc2 += yRowBump;
				dst += rowBump;
				dst2 += rowBump;
			}
			break;
	}
	return 0;
}

#endif

//====================================================================


//#define kScaleShift 	18
//#define	kHalfScaleShift	(kScaleShift/2)

#define kScaleShift 	kFract
#define	kHalfScaleShift	(kFract/2)


Error
DrawSingleJPEGRow(void *i)
{
	JPEGDecoder*	j = (JPEGDecoder*)i;
	long	rowOffset = j->currentSlice * j->MCUVPixels;
	long	rowHeight = j->MCUVPixels;
	long	rowWidth = j->MCUHPixels * j->WidthMCU;
	long	height = j->Height;
	long	width = j->Width;
	Rectangle  srcBounds,dstBounds;
	BitMapDevice	*world = j->DrawRefCon;
	long	srcWidth,srcHeight;
	long	dstWidth,dstHeight,dstLeft,dstTop;
	long	rowSkipLeft = 0;
	long	rowSkipTop = 0;
	long	unusedLeft = 0;
	long	unusedRight;
	long	unusedTop = 0;
	long	unusedBottom = 0;
	Rectangle srcClip;
	long	thisSliceWidth;
	long	thisSliceHeight;
	long	intervalH,intervalV;
	Boolean	isScaled;
	ulong	scaleH = 0,scaleV = 0;
	long 	n;
	
	// for faster background images, we could do the tiling once per decode
	// need to add support at higher level...
	
	long 	numTiles = 1;
	long 	tileLeft = j->fDrawRect.left;
	long 	tileTop = j->fDrawRect.top;
	long 	tileWidth = (j->fDrawRect.right - j->fDrawRect.left);
	long 	tileHeight = (j->fDrawRect.bottom - j->fDrawRect.top);
	
	dstWidth = j->fDrawRect.right - j->fDrawRect.left;
	dstHeight = j->fDrawRect.bottom - j->fDrawRect.top;
	
	isScaled = ( !j->thumbnail && ( width != dstWidth || height !=  dstHeight)  );
	
	if ( isScaled ) 
	{
		scaleH = (dstWidth << kScaleShift) / width;
		scaleV = (dstHeight<< kScaleShift) / height;
	}
	if (j->thumbnail) 
	{
		height = height >> 3;
		width = width >> 3;
	}
	srcClip = j->fClipRectangle;

	// map clipping rectangle to src space

	IntersectRectangle(&srcClip,&j->fDrawRect);

	thisSliceWidth = rowWidth;
	
	thisSliceWidth += (thisSliceWidth&1);
	
	thisSliceHeight = rowHeight;
	unusedRight = rowWidth - width;
	
	
	// allocate a slice buffer the first time through
	
	if ( world == nil ) 
	{
		Rectangle sliceBufferBounds;
		long	maxWidth;
		
		//	See we understand the color subsampling
		
		if (j->CompInFrame == 1) 
		{
			j->sampling = 0x0000;			// Grayscale
		} else 
		{
			CompSpec *y,*u,*v;
			
			y = &j->Comp[0];
			u = &j->Comp[1];
			v = &j->Comp[2];
			
			if ( u->Hi + u->Vi + v->Hi + v->Vi != 4) 
			{
				Message(("Can only draw 1:1 chroma <%d:%d> <%d:%d>",
				u->Hi , u->Vi , v->Hi , v->Vi));
				return kNoError;
			}
			
			j->sampling = ( y->Hi << 4) | y->Vi;
			
			switch (j->sampling) 
			{
				case	0x0011:
				case	0x0021:
				case	0x0022:
					break;
				default:
					Message(("Can only draw 1:1, 2:1 or 2:2 <%d:%d>",y->Hi,y->Vi));
					return kNoError;
			}
		}
		
		
		// allocate worst case slice buffer
		
		maxWidth = thisSliceWidth;
		if ( !isScaled )  
		{	
			long clipWidth = srcClip.right - srcClip.left;
			if ( clipWidth < thisSliceWidth ) 
				maxWidth = clipWidth + (srcClip.left&1) + (srcClip.right&1);
		}
		
		SetRectangle(sliceBufferBounds, 0, 0, maxWidth, rowHeight);
		world = j->DrawRefCon = NewBitMapDevice(sliceBufferBounds, kBufferFormat, 0, kNoTransparentColor);
		if ( world == nil ) 
		{
			Message(("JPEG No memory for offscreen buffer"));
			return kLowMemory;
		}
#ifdef	FILTER_JPEG
		world->filter = kTopFilter;			// do inerlace filter - dont do last line
#endif
	
	
		if ( isScaled  ) {
			Rectangle scanLine = sliceBufferBounds;
			scanLine.top = 0;
			scanLine.bottom = 1;
			
			if ( j->fResizeBuffer )
				DeleteBitMapDevice(j->fResizeBuffer);
			if ( j->fResizeAccumulateBuffer )
				DeleteBitMapDevice(j->fResizeAccumulateBuffer);
			j->fResizeBuffer = NewBitMapDevice(scanLine,yuv422Format,0,kNoTransparentColor);
			j->fResizeAccumulateBuffer = NewBitMapDevice(scanLine,yuv422Format,0,kNoTransparentColor);
			ClearYUYVScanLine((ushort *)j->fResizeAccumulateBuffer->baseAddress,thisSliceWidth,false);
			
			j->fDstYInc = IntToFrac(j->fDrawRect.bottom - j->fDrawRect.top) / j->Height;
			j->fDstYPos = IntToFrac(j->fDrawRect.top + FracToInt(rowOffset * scaleV));
			j->fDstYLast = j->fDstYPos;
		}
	}


	srcWidth = width;							// width of strip
	srcHeight = rowHeight;


	// figure out minimum size of a "whole" pixel - used when clipping
	
	switch (j->sampling) 
	{
	case	0x0011:
	case	0x0000:
		intervalH = 0;
		intervalV = 0;
		break;
	case	0x0021:
		intervalH = 1;
		intervalV = 0;
		break;
	case	0x0022:
		// еее╩if we support other YUV interleaves we need to increase these
		intervalH = 1;
		intervalV = 1;
		break;
	default:
		Message(("invalid sampling %lx",j->sampling));
		return kNoError;
	}
	
	
	// if last slice, do partial height strip at bottom
	
	if ( (rowOffset + rowHeight) > height )
		srcHeight = height - rowOffset;			

	if ( srcHeight == 0 )
		return kNoError;

	
	if ( isScaled ) 
	{
		dstTop = (rowOffset * scaleV);
		dstHeight = ((rowOffset + srcHeight) * scaleV);						
		dstHeight = FracToInt(dstHeight) - FracToInt(dstTop);
		dstTop = FracToInt(dstTop);
		dstWidth = (srcWidth * scaleH + kRound) >> kScaleShift;
		dstLeft = 0;
	} 
	else 
	{
		dstHeight = srcHeight;						
		dstTop = rowOffset;
		dstWidth = srcWidth;
		dstLeft = 0;
	}	
	

	// clipping
		
	if ( !j->thumbnail && numTiles == 1) 
	{ 
		if ( isScaled	)	
		{

			//	for scaled scaled case we could map clipping rectangle to src space
			// 	but since it's not common we will just ignore the clip until draw time
	
		} 
		else 
		{ 
			
			OffsetRectangle(srcClip,-tileLeft,-tileTop);

			// vertical clipping, including trivial reject
			
			if ( srcClip.bottom < rowOffset ) 
				return kNoError;
			if ( srcClip.top >= rowOffset + rowHeight ) 
				return kNoError;
			if ( srcClip.top <= rowOffset )
				srcClip.top = rowOffset;
			else 
			{
				rowSkipTop = srcClip.top - rowOffset;
				dstTop = srcClip.top;
				unusedTop = rowSkipTop & intervalV;
				rowSkipTop -= unusedTop;
			}
			if ( srcClip.bottom >= rowOffset + rowHeight )
				srcClip.bottom = rowOffset + rowHeight;
			srcHeight = srcClip.bottom - srcClip.top;

			if ( srcHeight <= 0 ) 
				return kNoError;
			unusedBottom = (srcClip.bottom & intervalV);
			
			dstHeight = srcHeight;
			thisSliceHeight = srcHeight + unusedTop + unusedBottom;
	
			//	horizontal clipping
			
			srcWidth = srcClip.right - srcClip.left;
			if ( srcWidth <= 0 ) 
				return kNoError;

			unusedLeft =  (srcClip.left & 1);
			unusedRight = (srcClip.right & 1);
			rowSkipLeft = srcClip.left - unusedLeft;
			thisSliceWidth = srcWidth + unusedLeft + unusedRight;
			if ( thisSliceWidth > world->bounds.right-world->bounds.left )
				thisSliceWidth = world->bounds.right-world->bounds.left;
	
			dstLeft = srcClip.left;
			dstWidth = srcWidth;
		}
	}

	// convert visible portion to RGB and write to slice buffer


#ifdef	SIMULATOR
	BitMapFormat screenBufferFormat = SCREENBUFFER_FORMAT;
	if ( screenBufferFormat == rgb16Format || screenBufferFormat == rgb32Format )
		MakeRGBSlice(j,world,thisSliceWidth,thisSliceHeight,rowSkipLeft,rowSkipTop,0,0);
	else
#endif
		MakeYUVSlice(j,world,thisSliceWidth,thisSliceHeight,rowSkipLeft,rowSkipTop,0,0);


//#define	TEST_VECTOR_QUANTIZATION
#ifdef	TEST_VECTOR_QUANTIZATION
	{
		static long gVQSize = 0;
		static BitMapDevice *lw = 0;
		long s;
		
		if ( lw != world ) {
			lw = world;
			gVQSize = 0;
		}
		VectorQuantizer *vq = new(VectorQuantizer);
		if ( vq ) {
			
			s = (world->bounds.bottom - world->bounds.top) * (world->bounds.right - world->bounds.left);
	
			if ( s > 512*32 )
				s = 256;
			else
				s = 128;
				
			vq->Start(world,nil,nil,s);	
			while ( !vq->Continue(false) )
				;
			BitMapDevice *vqworld = vq->GetBitMap(false);	
			if ( vqworld ) 
			{
				DrawImage(*world,*vqworld, world->bounds);
				
				s = sizeof(BitMapDevice);
				gVQSize += s;
				s = vqworld->colorTable->size;
				gVQSize += s;
				s = BitMapPixelBufferSize(vqworld);
				gVQSize += s;
			}
			delete(vq);
		}
	}
#endif	
	
	for ( n=0; n < numTiles; n++ ) 
	{

		SetRectangle(srcBounds,	unusedLeft,unusedTop,unusedLeft + srcWidth,
			unusedTop + srcHeight);
		SetRectangle(dstBounds,tileLeft + dstLeft,tileTop + dstTop,
					tileLeft + dstLeft + dstWidth,tileTop  + dstTop + dstHeight );

		// copy to screen
		
		//Message(("Drawing JPEG w=%d, h=%d to w=%d, h=%d, at %d, %d \n", world->bounds.right - world->bounds.left, world->bounds.bottom - world->bounds.top, destinationBounds.right - destinationBounds.left, destinationBounds.bottom - destinationBounds.top, destinationBounds.top, destinationBounds.left));
		
		
#ifdef	FILTER_JPEG
		if ( j->currentSlice == j->lastSlice )
		{
			if ( world->filter == kTopFilter )
				world->filter = kFullFilter;
			else
				world->filter = kBottomFilter;
		}
#endif
		Rectangle dr = dstBounds;
		IntersectRectangle(&dr,&j->fClipRectangle);
		if (j->fDrawDevice )
			DrawImage(*j->fDrawDevice, *world, dstBounds, j->fTransparency,&j->fClipRectangle,&srcBounds);
		else
		{
			if ( isScaled && j->fResizeBuffer) {
				int srcY;
				Rectangle dstScanLine,srcScanLine;
				Rectangle bufferScanLine;
				srcScanLine = srcBounds;
				dstScanLine = dstBounds;
				bufferScanLine.top = 0;
				bufferScanLine.bottom = 1;
				bufferScanLine.left = 0;
				bufferScanLine.right = srcBounds.right - srcBounds.left;
				
				if ( (scaleV < (1<<kScaleShift) ) ) {
				for ( srcY=srcBounds.top; srcY < srcBounds.bottom; srcY++ ) {
					srcScanLine.top = srcY;
					srcScanLine.bottom = srcScanLine.top + 1;
					
						DrawImage(*j->fResizeBuffer,*world, bufferScanLine,0,nil,&srcScanLine);
					j->fDstYLast = j->fDstYPos;
					j->fDstYPos += j->fDstYInc;
					if ( FracToInt(j->fDstYPos) > FracToInt(j->fDstYLast) ) {
							dstScanLine.top = FracToInt(j->fDstYLast);
							dstScanLine.bottom = dstScanLine.top+1;
						AddYUYVScanLine((ushort *)j->fResizeBuffer->baseAddress,(ushort *)j->fResizeAccumulateBuffer->baseAddress,thisSliceWidth,false,kFractOne - FracMan(j->fDstYLast));
							DrawImage(gScreenDevice,*j->fResizeAccumulateBuffer, dstScanLine,j->fTransparency,&j->fClipRectangle,&bufferScanLine);
						ClearYUYVScanLine((ushort *)j->fResizeAccumulateBuffer->baseAddress,thisSliceWidth,false);
						AddYUYVScanLine((ushort *)j->fResizeBuffer->baseAddress,(ushort *)j->fResizeAccumulateBuffer->baseAddress,thisSliceWidth,false,FracMan(j->fDstYPos));
					} else {
						AddYUYVScanLine((ushort *)j->fResizeBuffer->baseAddress,(ushort *)j->fResizeAccumulateBuffer->baseAddress,thisSliceWidth,false,j->fDstYInc);
					}
				}
			} else {
					long weight;
					for ( srcY=srcBounds.top; srcY < srcBounds.bottom; srcY++ ) {
						srcScanLine.top = srcY;
						srcScanLine.bottom = srcScanLine.top + 1;
						j->fDstYLast = j->fDstYPos;
						j->fDstYPos += j->fDstYInc;
						dstScanLine.top = FracToInt(j->fDstYLast);
						dstScanLine.bottom = dstScanLine.top+1;
						weight = FracMan(j->fDstYLast);
						if ( weight ) {
							weight *= 256;
							weight >>= kFract;
							DrawImage(*j->fResizeAccumulateBuffer,*world, bufferScanLine,0,nil,&srcScanLine);
							BlendYUYVScanLine((ushort *)j->fResizeBuffer->baseAddress,(ushort *)j->fResizeAccumulateBuffer->baseAddress,thisSliceWidth,false,weight);
							DrawImage(gScreenDevice,*j->fResizeAccumulateBuffer,dstScanLine,j->fTransparency,&j->fClipRectangle,&bufferScanLine);
							dstScanLine.top++;
						}
						DrawImage(*j->fResizeBuffer,*world, bufferScanLine,0,nil,&srcScanLine);
						while ( dstScanLine.top < FracToInt(j->fDstYPos) ) {
							dstScanLine.bottom = dstScanLine.top+1;
							DrawImage(gScreenDevice,*world, dstScanLine,j->fTransparency,&j->fClipRectangle,&srcScanLine);
							dstScanLine.top++;
						}
					}
				}
			} else {
		DrawImage(gScreenDevice, *world, dstBounds, j->fTransparency,&j->fClipRectangle,&srcBounds);
			}
		}


		UnionRectangle(&j->fDrewRectangle,&dr);
#ifdef	FILTER_JPEG
		world->filter = kSliceFilter;
#endif		
		//ValidRect(&r);
	
	
		tileLeft += tileWidth;
		if ( tileLeft >= j->fDrawRect.right ) 
		{
			tileLeft = j->fDrawRect.left;
			tileTop += tileHeight;
		}
	}
	return kNoError;							// return 1 to cancel
}


