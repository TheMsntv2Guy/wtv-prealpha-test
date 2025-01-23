// ===========================================================================
//	GraphicsBlit.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================



#include	"GraphicsPrivate.h"



inline ulong	FFilterLongs( ulong a,ulong b,ulong c);	
inline ulong	FSFilterLongs( ulong a,ulong b,ulong c);	




inline ulong
FFilterLongs(register ulong a,register ulong b,register ulong c)	
{
#ifdef	DONT_FILTER_CHROMA
	register ulong chromamask = 0x00ff00ff;
	register ulong wmask = 0xff00;
	register ulong bmask = 0xff;
	return (b & chromamask) | 
			(~chromamask & 
			((((((a>>24)&bmask) + ((c>>24)&bmask)) + (((b>>24)&bmask)<<1)) << (24-2)) + 
			((((a&wmask) + (c&wmask)) + ((b&wmask)<<1)) >> 2))
			);
#else
	register ulong d;
	register uchar ya,yb,yc;
	ya = a>>24;
	yb = b>>24;
	yc = c>>24;
	d = FilterBytes(ya,yb,yc);
	d <<= 24;
	ya = (a>>8);
	yb = (b>>8);
	yc = (c>>8);
	d |= FilterBytes(ya,yb,yc)<<8;
	ya = 0xff & (a>>16);
	yb = 0xff & (b>>16);
	yc = 0xff & (c>>16);
	d |= (FilterChromaBytes(ya,yb,yc) << 16);
	ya = 0xff & (a);
	yb = 0xff & (b);
	yc = 0xff & (c);
	d |= FilterChromaBytes(ya,yb,yc);
	return d;
#endif	
}




inline ulong
FSFilterLongs( ulong a,ulong b,ulong c)	
{
	register ulong d;
	register uchar ya,yb,yc;
	
	ya = a>>24;
	yb = b>>24;
	yc = c>>24;
	d = SFilterBytes(ya,yb,yc);
	d <<= 24;
	ya = 0xff & (a>>8);
	yb = 0xff & (b>>8);
	yc = 0xff & (c>>8);
	d |= (SFilterBytes(ya,yb,yc) << 8);
#ifdef	DONT_FILTER_CHROMA
	d |= (b & 0x00ff00ff);
#else
	ya = 0xff & (a>>16);
	yb = 0xff & (b>>16);
	yc = 0xff & (c>>16);
	d |= (SFilterChromaBytes(ya,yb,yc) << 16);
	ya = 0xff & (a);
	yb = 0xff & (b);
	yc = 0xff & (c);
	d |= SFilterChromaBytes(ya,yb,yc);
#endif	
	return d;
}


Boolean
BoundsSetup(BlitParams& p)
{
	Ordinate minLeft = p.device->bounds.left;
	Ordinate maxRight = p.device->bounds.right;
	Ordinate minTop = p.device->bounds.top;
	Ordinate maxBottom = p.device->bounds.bottom;

	// clip to device bounds
	if (p.r.left < minLeft) 
		p.r.left = minLeft;
	if (p.r.right > maxRight) 
		p.r.right = maxRight;
	if (p.r.top < minTop) 
		p.r.top = minTop;
	if (p.r.bottom > maxBottom) 
		p.r.bottom = maxBottom;

	// empty check
	if (p.r.right <= p.r.left || p.r.bottom <= p.r.top)
		return false;

	// localize coordinates
	p.r.left -= minLeft;
	p.r.right -= minLeft;
	p.r.top -= minTop;
	p.r.bottom -= minTop;

	return true;
}

void
BlitSetup(BlitParams& p)
{
	// 
	MapColor(p.device->format, p.color);

	// turn x ords into bit positions
	long leftBits = p.r.left << p.device->depthShift;
	long rightBits = p.r.right << p.device->depthShift;
	p.leftBits = leftBits;
	p.rightBits = rightBits;

	// compute destination long ptr and count of longs
	long rowBytes = p.device->rowBytes;
	p.bump = rowBytes;
	p.base = (ulong*)(p.device->baseAddress + (rowBytes * p.r.top)) + (leftBits >> 5);
	
	//TrivialMessage(("p.base == %0x%x", p.base));
	p.longCount = (rightBits >> 5) - (leftBits >> 5);
	
	// compute edge masks	
	p.leftMask = 0xffffffff >> (leftBits & 31);
	p.rightMask = 0xffffffff; 
	ulong temp;
	
	temp = (32 - (rightBits & 31));			// This was nasty! 0xffffffff << (32 - (rightBits & 31))
											// is undefined!
	// handle flush right edge case

	if (temp == 32)
		p.longCount -= 1;
	else
		p.rightMask <<= temp;
}



// PaintRect
void
PaintBlit(BlitParams& p)
{
	// important inner loop variables (consider reading sequentially)
	register ulong*		dst			= p.base;
	register long		rowBump		= p.bump;
	register long		longCount	= p.longCount;
	register ulong 		leftMask	= p.leftMask;
	register ulong		rightMask	= p.rightMask;
	register ulong		height		= p.r.bottom - p.r.top;
	register Color		color		= p.color;

	if (longCount == 0) {
		// special fits in one long case (both edges overlap)
		leftMask &= rightMask;		// combine mask for 1 long case
		color &= leftMask;
		leftMask = ~leftMask;

		while (height > 0) {
			*dst = *dst & leftMask | color;
			dst = (ulong*)((char*)dst + rowBump);
			height--;
		}
	}
	else {
		// standard mulitple long case
		rowBump -= longCount * 4;
		
		while (height > 0) {
			*dst++ = (*dst & ~leftMask) | (color & leftMask);		// left edge
			for (long i=1; i < longCount; i++)						// middle longs
				*dst++ = color;
			*dst = (*dst & ~rightMask) | (color & rightMask);		// right edge
			dst = (ulong*)((char*)dst + rowBump);
			height--;
		}
	}
}


void
PaintBlitBlendYUYV(BlitParams& p, ulong transparency)
{
	// important inner loop variables (consider reading sequentially)
	register uchar*	dst			= (uchar*)p.base;
	register long		rowBump		= p.bump;
	register long		pixelCount = (p.rightBits >> 4) - (p.leftBits >> 4);
	register ulong		height		= p.r.bottom - p.r.top;
	Color		color		= p.color;
	short					y,u,v;
	ushort					pix,opix;
	register int			i,longCount = 0 ;
	
	dst += (p.leftBits >> 3) & 3;	// advance to ragged word boundary (dst is a ushort*)

	rowBump -= pixelCount * 2;

	y = (short)((color>>24) & 0xff);
	u = (short)((color>>16) & 0xff);
	v = (short)( color      & 0xff);
	
	if (transparency == kHalfTransparent) {
		ulong	yuyv = 0,sm = 0,d;
		ushort  yum =0,yu = 0,yvm = 0,yv = 0;
		
		if ( ((ulong)dst & 3) != 0  ) {
			yu = (y<<8) + u;
			yv = (y<<8) + v;
			yum = (yu & ~kLowBitsMask8) >> 1;
			yvm = (yv & ~kLowBitsMask8) >> 1;
		}
		else {
			longCount = (pixelCount) >> 1;
			pixelCount -= (longCount<<1);
			yuyv = ((ulong)y<<24) + ((ulong)u<<16) + (y<<8) + v;
			sm = yuyv & ~kLowBitsMask8;
			sm >>= 1;
		}
		
		while (height > 0) {
			for (i=0; i < longCount; i++)  {
				d = *(ulong *)dst;
				*(ulong *)dst = ((d & ~kLowBitsMask8) >> 1) + sm + ((yuyv & d) & kLowBitsMask8);
				dst += 4;
			}
			for (i=0; i < pixelCount; i++)  {
				ushort sd = *(ushort *)dst;
				if ( (ulong)dst & 2 )
					*(ushort *)dst = ((sd & ~kLowBitsMask8) >> 1) + yvm + ((yv & sd) & kLowBitsMask8);
				else
					*(ushort *)dst = ((sd & ~kLowBitsMask8) >> 1) + yum + ((yu & sd) & kLowBitsMask8);
				dst += 2;
			}
			dst += rowBump;
			height--;
		}
	} 
	else 
	{
		y = (short)y * (short)(kFullyTransparent-transparency);
		u = (short)u * (short)(kFullyTransparent-transparency);
		v = (short)v * (short)(kFullyTransparent-transparency);

		register short a = transparency;
		while (height > 0) {
			for (i=0; i < pixelCount; i++) {
				pix = *(ushort *)dst;
				opix = CLIPLUMINANCE(((pix>>8) * a + y)>>8) << 8;
				pix &= 0xff;
				pix *= a;
				pix += ((ulong)dst & 2 ) ? v : u;
				*(ushort *)dst = opix | CLIPCHROMA(pix>>8);
				dst += 2;
			}
			dst += rowBump;
			height--;
		}
	}
}



void 
XPaintRect(const BitMapDevice& device, const Rectangle& r, Color color, ulong transparency, const Rectangle* clip)
{
	BlitParams p;
	p.device = (BitMapDevice*)&device;
	p.r = r;
		
	if (clip != nil)
		IntersectRectangle(&p.r, clip);
	p.color = color;

#ifdef SPOT1
	if ( device.format == yuv422BucherFormat) {
		TrivialMessage(("Need a bucher paitblit loop"));
		return;
	}
#endif

	if (!BoundsSetup(p))
		return;

	BlitSetup(p);

	if (transparency == kNotTransparent)
		PaintBlit(p);
	else {
		switch ( device.format ) {
		case yuv422Format :
			PaintBlitBlendYUYV(p, transparency);
			break;
#ifdef	SIMULATOR
		case yuvFormat:
			PaintBlitBlendYUV(p, transparency);
			break;
		case rgb32Format:
			PaintBlitBlend32(p, transparency);
			break;
		case rgb16Format:
			PaintBlitBlend16(p, transparency,color);
			break;
		case gray8Format:
			PaintBlitBlendGray8(p, transparency,color);
			break;
#endif
		default:
			if ( IsError(1) )
				Message(("Can't paint to that format bitmap"));
			break;
		}
	}
}



void XFilterBounds(const BitMapDevice& device, const Rectangle& r,FilterType filterType,const Rectangle* clip)
{
	
	if (!gSystem->GetUseFlickerFilter())
		return;
		
	if ( filterType != kNoFilter ) {	
		BlitParams p;
		p.device = (BitMapDevice*)&device;
		p.r = r;
		Rectangle clippedDevice = device.bounds;

		if (clip != nil)
			IntersectRectangle(&clippedDevice, clip);

		IntersectRectangle(&p.r, &clippedDevice);
	
		if (!BoundsSetup(p))
			return;
	
			BlitSetup(p);
		if ( device.format == yuv422Format) {
			NewInterlaceFilterYUYV(p,filterType);		// slower but better filter
		}
#ifdef	SIMULATOR
		else if ( device.format == rgb32Format ) 
			InterlaceFilterRGB32(p,filterType);	
#endif

	}
}


void
CopyBlitFast(BlitParams2& p, ulong UNUSED(transparency))
	{
	// important inner loop variables
	register ulong*		src = p.src.base;
	register ulong*		dst = p.dst.base;
	register ulong		leftMask  = p.dst.leftMask;
	register ulong		rightMask = p.dst.rightMask;
	register long		longCount = p.dst.longCount;

	register long		leftShift;
	register long		rightShift;
	register long		srcBump;
	register long		dstBump;

	Ordinate sx1 = p.src.leftBits;
	Ordinate dx1 = p.dst.leftBits;
	long height = p.dst.r.bottom - p.dst.r.top;

	// compute src to dst bit shift
	leftShift = (sx1 & 31) - (dx1 & 31);
	if (leftShift < 0)
		{
		leftShift += 32;
		src--;
		}
	rightShift = 32 - leftShift;

	// blt
	if (longCount == 0)
		{
		// special fits in one long case (both edges overlap)
		srcBump = p.src.bump;
		dstBump = p.dst.bump;
		
		ulong srcMask = leftMask & rightMask;
		ulong dstMask = ~srcMask;

		if (leftShift == 0)
			{
			// one long & no shift
			while (height > 0)
				{
				*dst = ((*dst & dstMask) | (*src & srcMask));
				src = (ulong*)((char*)src + srcBump);
				dst = (ulong*)((char*)dst + dstBump);
				height--;
				}
			}
		else
			{
			// one long & shift case
			while (height > 0)
				{
				register ulong bits = (*src << leftShift) | (*(src+1) >> rightShift);
				*dst = ((*dst & dstMask) | (bits & srcMask));
				src = (ulong*)((char*)src + srcBump);
				dst = (ulong*)((char*)dst + dstBump);
				height--;
				}
			}
		}
	else	// multiple longs case
		{
		if (leftShift == 0)
			{
			// aligned multiple longs
			srcBump = p.src.bump - longCount * 4;
			dstBump = p.dst.bump - longCount * 4;
			while (height > 0)
				{
				*dst++ = (*dst & ~leftMask) | (*src++ & leftMask);
				for (long i=1; i < longCount; i++)
					*dst++ = *src++;
				*dst = (*dst & ~rightMask) | (*src & rightMask);

				dst = (unsigned long*)((char*)dst + dstBump);
				src = (unsigned long*)((char*)src + srcBump);
				height--;
				}
			}
		else
			{
			// unaligned multiple longs
			srcBump = p.src.bump - (longCount+1) * 4;
			dstBump = p.dst.bump - longCount * 4;
			while (height > 0)
				{
				register ulong bits = *src++ << leftShift;
				register ulong prevBits = *src++;
				bits |= prevBits >> rightShift;
				*dst++ = (*dst & ~leftMask) | (bits & leftMask);
				for (long i=1; i < longCount; i++)
					{
					bits = prevBits << leftShift;
					prevBits = *src++;
					*dst++ = bits | (prevBits >> rightShift);
					}
				bits = (prevBits << leftShift) | (*src >> rightShift);
				*dst = (*dst & ~rightMask) | (bits & rightMask);

				src = (unsigned long*)((char*)src + srcBump);
				dst = (unsigned long*)((char*)dst + dstBump);
				height--;
				}
			}
		}
	}

	
	
	



#ifdef	SPOT1	

/*

еее

The two following BLIT routines are temporary hacks for the hosed SPOT1 frame buffer
which has odd and even scanline segrated in two sections of the frame buffer. They 
should go away when the hardware works in a correct manner.

еее

*/

	
void
ScaleBlitBuchertoBucher(BlitParams2& p, ulong transparency);


void
ScaleBlitYUYVtoBucher(BlitParams2& p, ulong transparency);


void
ScaleBlitBuchertoBucher(BlitParams2& p, ulong transparency)
{
	PostulateFinal(false);
	
	register uchar*	src;
	register uchar*	dst;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	long	dstFieldOffset = GetDisplayPageBase(2) - GetDisplayPageBase(0);
	long	srcFieldOffset = dstFieldOffset;
	
	long 	quadLongCount;
	long 	extraPixels;
	long	i;
	
	srcBump = (p.src.device->rowBytes - pixelCount * 2);
	dstBump = (p.dst.device->rowBytes - pixelCount * 2);
	
	if (p.src.r.top >= p.dst.r.top)
	{
		//Message(("BucherToBucher1"));

		src = p.src.device->baseAddress;
		src += (p.src.r.top>>1) * p.src.device->rowBytes;
		src += (p.src.leftBits >> 4) * 2;	// advance to ragged word boundary
	
		dst = p.dst.device->baseAddress;
		dst += (p.dst.r.top>>1) * p.dst.device->rowBytes;
		dst += (p.dst.leftBits >> 4) * 2;	// advance to ragged word boundary
	
		if ( !(p.src.r.top & 1) ) {
			src += srcFieldOffset;
			srcFieldOffset = -srcFieldOffset;
		}

		if ( !(p.dst.r.top & 1) ) {
			dst += dstFieldOffset;
			dstFieldOffset = -dstFieldOffset;
		}

		if (((long)src & 3) == 0 && ((long)dst & 3) == 0)
		{										// src and dst long word aligned
			quadLongCount = pixelCount >> 3;
			extraPixels = pixelCount & 7;
		}
		else
		{										// go slow
			//Message(("slow1"));
			quadLongCount = 0;
			extraPixels = pixelCount;
		}

		while (height > 0)
		{
			for (i=0; i < quadLongCount; i++) 
			{
				register long t1,t2,t3,t4;
				
				t1 = *(long *)src;
				t2 = *(long *)(src+4);
				t3 = *(long *)(src+8);
				t4 = *(long *)(src+12);
				*(long *)dst 	  = t1;
				*(long *)(dst+4)  = t2;
				*(long *)(dst+8)  = t3;
				*(long *)(dst+12) = t4;
				dst += 16;
				src += 16;
			}
			for (i=0; i < extraPixels; i++) 
				{
					*(short *)dst = *(short *)src;
					dst += 2;
					src += 2;
				}
			dst += dstFieldOffset;
			if (dstFieldOffset < 0)
				dst -= pixelCount * 2;
			else
				dst += dstBump;
			dstFieldOffset = -dstFieldOffset;
	
			src += srcFieldOffset;
			if (srcFieldOffset < 0)
				src -= pixelCount * 2;
			else
				src += srcBump;
			srcFieldOffset = -srcFieldOffset;
	
			height--;
		}
	}
	else
	{

		//Message(("BucherToBucher2"));

		src = p.src.device->baseAddress;
		src += ((p.src.r.bottom-1)>>1) * p.src.device->rowBytes;
		src += (p.src.leftBits >> 4) * 2;	// advance to ragged word boundary
	
		dst = p.dst.device->baseAddress;
		dst += ((p.dst.r.bottom-1)>>1) * p.dst.device->rowBytes;
		dst += (p.dst.leftBits >> 4) * 2;	// advance to ragged word boundary

		if ( !((p.src.r.bottom-1) & 1) ) {
			src += srcFieldOffset;
			srcFieldOffset = -srcFieldOffset;
		}
	
		if ( !((p.dst.r.bottom-1) & 1) ) {
			dst += dstFieldOffset;
			dstFieldOffset = -dstFieldOffset;
		}
	
		if (((long)src & 3) == 0 && ((long)dst & 3) == 0)
		{										// src and dst long word aligned
			quadLongCount = pixelCount >> 3;
			extraPixels = pixelCount & 7;
			}
		else
		{										// go slow
			//Message(("slow2"));
			quadLongCount = 0;
			extraPixels = pixelCount;
		}
		
		while (height > 0)
		{
			for (i=0; i < quadLongCount; i++) 
			{
				register long t1,t2,t3,t4;
				
				t1 = *(long *)src;
				t2 = *(long *)(src+4);
				t3 = *(long *)(src+8);
				t4 = *(long *)(src+12);
				*(long *)dst 	  = t1;
				*(long *)(dst+4)  = t2;
				*(long *)(dst+8)  = t3;
				*(long *)(dst+12) = t4;
				dst += 16;
				src += 16;
			}
			for (i=0; i < extraPixels; i++) 
				{
				*(short *)dst = *(short *)src;
				dst += 2;
				src += 2;
			}
			dst += dstFieldOffset;
			dst -= pixelCount * 2;
			if (dstFieldOffset < 0)
				dst -= p.dst.device->rowBytes;
			dstFieldOffset = -dstFieldOffset;
	
			src += srcFieldOffset;
			src -= pixelCount * 2;
			if (srcFieldOffset < 0)
				src -= p.src.device->rowBytes;
			srcFieldOffset = -srcFieldOffset;
	
			height--;
		}
	}
}



void
ScaleBlitYUYVtoBucher(BlitParams2& p, ulong transparency)
{
	PostulateFinal(false);
	
	register uchar*	src = (uchar*)p.src.base;
	register uchar*	dst = (uchar*)p.dst.base;
	
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	long	fieldOffset = GetDisplayPageBase(2) - GetDisplayPageBase(0);

	long 	quadLongCount;
	long 	extraPixels;
	long	i;
	
	src += ((p.src.leftBits >> 4) & 1) * 2;	// advance to ragged word boundary
	
	dst = p.dst.device->baseAddress;
	dst += (p.dst.r.top>>1) * p.dst.device->rowBytes;
	dst += (p.dst.leftBits >> 4) * 2;	// advance to ragged word boundary

	srcBump = (p.src.bump - pixelCount * 2);
	dstBump = (p.dst.bump - pixelCount * 2);

	
	if ( !(p.dst.r.top & 1) ) {
		dst += fieldOffset;
		fieldOffset = -fieldOffset;
	}

	if (transparency == kNotTransparent)
	{
		if (((long)src & 3) == 0 && ((long)dst & 3) == 0)
		{										// src and dst long word aligned
			quadLongCount = pixelCount >> 3;
			extraPixels = pixelCount & 7;
		}
		else
		{										// go slow
			//Message(("slow3"));
			quadLongCount = 0;
			extraPixels = pixelCount;
		}
		
		while (height > 0)
		{
			for (i=0; i < quadLongCount; i++) 
			{
				register long t1,t2,t3,t4;
				
				t1 = *(long *)src;
				t2 = *(long *)(src+4);
				t3 = *(long *)(src+8);
				t4 = *(long *)(src+12);
				*(long *)dst 	  = t1;
				*(long *)(dst+4)  = t2;
				*(long *)(dst+8)  = t3;
				*(long *)(dst+12) = t4;
				dst += 16;
				src += 16;
			}
			for (i=0; i < extraPixels; i++) 
			{
				*(short *)dst = *(short *)src;
				dst += 2;
				src += 2;
			}
			dst += fieldOffset;
			if (fieldOffset < 0)
				dst -= pixelCount * 2;
			else
				dst += dstBump;
			fieldOffset = -fieldOffset;
				src += srcBump;
			height--;
		}
	}
	else if ( transparency == kHalfTransparent ) {
		ulong	d;
		
		if (((long)src & 3) == 0 && ((long)dst & 3) == 0)
		{										// src and dst long word aligned
			quadLongCount = pixelCount >> 3;
			extraPixels = pixelCount & 7;
		}
	else
		{										// go slow
			quadLongCount = 0;
			extraPixels = pixelCount;
		}
		
		while (height > 0)
		{
			for (i=0; i < quadLongCount; i++) 
			{
				register ulong t1,t2,t3,t4;
				
				t1 = *(ulong *)src;
				t2 = *(ulong *)(src+4);
				t3 = *(ulong *)(src+8);
				t4 = *(ulong *)(src+12);
				src += 16;
				
				d = *(ulong *)dst;
				*(ulong *)dst = ((d & ~kLowBitsMask8) >> 1) + ((t1 & ~kLowBitsMask8) >> 1) + ((t1 & d) & kLowBitsMask8);
				dst += 4;
				d = *(ulong *)dst;
				*(ulong *)dst = ((d & ~kLowBitsMask8) >> 1) + ((t2 & ~kLowBitsMask8) >> 1) + ((t2 & d) & kLowBitsMask8);
				dst += 4;
				d = *(ulong *)dst;
				*(ulong *)dst = ((d & ~kLowBitsMask8) >> 1) + ((t3 & ~kLowBitsMask8) >> 1) + ((t3 & d) & kLowBitsMask8);
				dst += 4;
				d = *(ulong *)dst;
				*(ulong *)dst = ((d & ~kLowBitsMask8) >> 1) + ((t4 & ~kLowBitsMask8) >> 1) + ((t4 & d) & kLowBitsMask8);
				dst += 4;
			}
			for (i=0; i < extraPixels; i++)  {
				ushort hd = *(ushort *)dst;
				ushort hs = *(ushort *)src;
				*(ushort *)dst = ((hd & ~kLowBitsMask8) >> 1) + ((hs & ~kLowBitsMask8) >> 1) + ((hs & hd) & kLowBitsMask8);
				dst += 2;
				src += 2;
			}
			dst += fieldOffset;
			if (fieldOffset < 0)
				dst -= pixelCount * 2;
			else
				dst += dstBump;
			fieldOffset = -fieldOffset;
				src += srcBump;
			height--;
		}
	}
	else {
		long longCount;
		
		if (((long)src & 3) == 0 && ((long)dst & 3) == 0)
		{										// src and dst long word aligned
			// do 2 pixels at a time
			longCount = pixelCount >> 1;
			extraPixels = pixelCount & 1;
		}
		else
		{										// go slow
			longCount = 0;
			extraPixels = pixelCount;
		}
		while (height > 0)
		{
			for (i=0; i < longCount; i++)
			{
				uchar t1,t2,t3,t4;
				ulong spix,dpix;
				
				spix = *(ulong*)src;
				dpix = *(ulong*)dst;
				t1 = TransparencyBlendY(transparency,(spix>>24),(dpix>>24));
				t2 = TransparencyBlend(transparency,(spix>>16),(dpix>>16));
				t3 = TransparencyBlendY(transparency,(spix>>8),(dpix>>8));
				t4 = TransparencyBlend(transparency,spix,dpix);
				*(ulong*)dst = (t1<<24) | (t2<<16) | (t3<<8) | t4;
				src+=4;
				dst+=4;
			}
			for (i=0; i < extraPixels; i++)
			{
				ushort s,d;
				s = *(ushort *)src;
				d = *(ushort *)dst;
				*(ushort *)dst = (TransparencyBlendY(transparency,s>>8,d>>8) <<8) + TransparencyBlend(transparency,s,d);
				src += 2;
				dst += 2;
			}
			dst += fieldOffset;
			if (fieldOffset < 0)
				dst -= pixelCount * 2;
			else
				dst += dstBump;
			fieldOffset = -fieldOffset;
			src += srcBump;
			height--;
		}
	}
}



void
FilterBlitYUYVtoBucher(BlitParams2& p, ulong);

void
FilterBlitYUYVtoBucher(BlitParams2& p, ulong)
{
	PostulateFinal(false);
	
	register ushort*	src = (ushort*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;
	register ushort*	lsrc = (ushort*)((uchar*)p.src.base - p.src.device->rowBytes);
	register ushort*	nsrc = (ushort*)((uchar*)p.src.base + p.src.device->rowBytes);
	ushort*				srcEnd = (ushort*)((uchar*)p.src.device->baseAddress + (p.src.device->bounds.bottom-p.src.device->bounds.top) * p.src.device->rowBytes);
	
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	long				fieldOffset = (GetDisplayPageBase(2) - GetDisplayPageBase(0))>>1;
	int					firstLine = lsrc < (ushort *)p.src.device->baseAddress;
	int					nextLine = 0;
	long 				i,quadLongCount,extraPixels;
	ushort				y,uv,pixel,pixelL,pixelN;
	
	src += ((p.src.leftBits >> 4) & 1);	// advance to ragged word boundary
	lsrc += ((p.src.leftBits >> 4) & 1);	// advance to ragged word boundary
	nsrc += ((p.src.leftBits >> 4) & 1);	// advance to ragged word boundary
	
	dst = (ushort*)p.dst.device->baseAddress;
	dst += (p.dst.r.top>>1) * (p.dst.device->rowBytes>>1);
	dst += (p.dst.leftBits >> 4);		// advance to ragged word boundary

	srcBump = (p.src.bump>>1) - pixelCount;
	dstBump = (p.dst.bump>>1) - pixelCount;


	if ( p.dst.device->filter == kCopyFilterOffset ) {			// slice filter
		firstLine = 2;
	}
	if ( !(p.dst.r.top & 1) ) {
		dst += fieldOffset;
		fieldOffset = -fieldOffset;
	}

	if (((long)src & 3) == 0 && ((long)dst & 3) == 0)
	{										// src and dst long word aligned
		quadLongCount = pixelCount >> 3;
		extraPixels = pixelCount & 7;
	}
	else
	{										// go slow
		//Message(("slow3"));
		quadLongCount = 0;
		extraPixels = pixelCount;
	}
	
	while (height > 0)
	{
		if  ( height == 1 &&  ( nsrc >= srcEnd || p.dst.device->filter == kCopyFilterOffset ) ) 
			nextLine = p.dst.device->filter == kCopyFilterOffset ? 2 : 1;
		for (i=0; i < quadLongCount; i++) 
		{
			register long t1,t2,t3,t4;
			register long tl1,tl2,tl3,tl4;
			register long tn1,tn2,tn3,tn4;
			
			t1 = *(long *)src;
			t2 = *(long *)(src+2);
			t3 = *(long *)(src+4);
			t4 = *(long *)(src+6);
			if ( firstLine == 0  ) {
				tl1 = *(long *)lsrc;
				tl2 = *(long *)(lsrc+2);
				tl3 = *(long *)(lsrc+4);
				tl4 = *(long *)(lsrc+6);
			} else if ( firstLine == 1 ) {
				tl1 = (ushort)kBlackYUYV;
				tl2 = (ushort)kBlackYUYV;
				tl3 = (ushort)kBlackYUYV;
				tl4 = (ushort)kBlackYUYV;
			} else {
				tl1 = t1;
				tl2 = t2;
				tl3 = t3;
				tl4 = t4;
			}
			if ( nextLine == 1) {
				tn1 = (ushort)kBlackYUYV;
				tn2 = (ushort)kBlackYUYV;
				tn3 = (ushort)kBlackYUYV;
				tn4 = (ushort)kBlackYUYV;
			} else if ( nextLine == 0 )  {
				tn1 = *(long *)nsrc;
				tn2 = *(long *)(nsrc+2);
				tn3 = *(long *)(nsrc+4);
				tn4 = *(long *)(nsrc+6);
			} else {
				tn1 = t1;
				tn2 = t2;
				tn3 = t3;
				tn4 = t4;
			}
			*(long *)dst 	  = FilterLongs(tl1,t1,tn1);
			*(long *)(dst+2)  = FilterLongs(tl2,t2,tn2);
			*(long *)(dst+4)  = FilterLongs(tl3,t3,tn3);
			*(long *)(dst+6)  = FilterLongs(tl4,t4,tn4);
			dst += 8;
			src += 8;
			lsrc += 8;
			nsrc += 8;
		}
		for (i=0; i < extraPixels; i++) 
		{
			pixel = *src;
			if ( firstLine == 0  ) 
				pixelL = *lsrc;
			else if ( firstLine == 1 )
				pixelL = (ushort)kBlackYUYV;
			else
				pixelL = pixel;
			if ( nextLine == 0 )
				pixelN = *nsrc;
			else if ( nextLine == 1 )
				pixelN = (ushort)kBlackYUYV;
			else
				pixelN = pixel;
			y = FilterBytes((pixelL>>8), (pixel>>8),(pixelN>>8));
			uv = FilterChromaBytes((pixelL&0xff), (pixel&0xff),(pixelN&0xff));
			*dst++ = (y<<8) + uv;
			src++;
			lsrc++;
			nsrc++;
		}
		dst += fieldOffset;
		if (fieldOffset < 0)
			dst -= pixelCount;
		else
			dst += dstBump;
		fieldOffset = -fieldOffset;
		src += srcBump;
		lsrc += srcBump;
		nsrc += srcBump;
		height--;
		firstLine = 0;
	}
}


#endif	/*	SPOT1	*/


void
FilterBlitYUYVtoYUYV(BlitParams2& p, ulong transparency);


void
FilterBlitYUYVtoYUYV(BlitParams2& p, ulong transparency )
{
	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register ushort*	lsrc = (ushort*)((uchar*)p.src.base - p.src.device->rowBytes);
	register ushort*	nsrc = (ushort*)((uchar*)p.src.base + p.src.device->rowBytes);
	ushort*				srcEnd = (ushort*)((uchar*)p.src.device->baseAddress + (p.src.device->bounds.bottom-p.src.device->bounds.top) * p.src.device->rowBytes);
	register ushort*	dst = (ushort*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	long				i;
	int					firstLine = lsrc < (ushort *)p.src.device->baseAddress ? 1 : 0;
	int					nextLine = 0;
	ushort				pixel,pixelN,pixelL;
	long				extraPixels,quadLongCount;
	Boolean				doSlice = p.dst.device->filter == kCopyFilterOffset;
	ulong				borderColor;
	
	
	borderColor = ((gScreenBorderColor>>16)  & 0xff) << 8;		// 0000yy00
	borderColor |= (borderColor<<16);							// yy00yy00
	borderColor |= ((gScreenBorderColor>>8)  & 0xff) << 16;		// yyuuyy00
	borderColor |= gScreenBorderColor & 0xff;					// yyuuyyvv
	
	
	src += ((p.src.leftBits>>4) & 1);	
	lsrc += ((p.src.leftBits>>4) & 1);	
	nsrc += ((p.src.leftBits>>4) & 1);	
	dst += ((p.dst.leftBits>>4) & 1);	
	
	srcBump = (p.src.bump>>1) - pixelCount;
	dstBump = (p.dst.bump>>1) - pixelCount;

	if ( IsError((((ulong)src) &2)  != ((((ulong)dst) & 2)) ) ) {
		Message(("Can't realign u and v in filter loop"));
		return;
	}
		
	if (((long)src & 3) == 0 && ((long)dst & 3) == 0)
	{										// src and dst long word aligned
		quadLongCount = pixelCount >> 3;
		extraPixels = pixelCount & 7;
	}
	else
	{										// go slow
		quadLongCount = 0;
		extraPixels = pixelCount;
	}
	
	if ( doSlice  ) {			// slice filter
		firstLine = 2;
	}
	
	if ( transparency != 0 ) {		// assumes it is kHalfTransparent

		Assert(transparency==kHalfTransparent);
		
		while (height > 0)
		{
			if  ( height == 1 &&  ( nsrc >= srcEnd || doSlice ) ) 
				nextLine = p.dst.r.bottom == p.dst.device->bounds.bottom ? 1 : 2;
			for (i=0; i < quadLongCount; i++) 
			{
				register ulong t1,t2,t3,t4;
				register ulong tl1,tl2,tl3,tl4;
				register ulong tn1,tn2,tn3,tn4;
				
				t1 = *(long *)src;
				t2 = *(long *)(src+2);
				t3 = *(long *)(src+4);
				t4 = *(long *)(src+6);
				
				if ( firstLine == 0 ) {
					tl1 = *(long *)lsrc;
					tl2 = *(long *)(lsrc+2);
					tl3 = *(long *)(lsrc+4);
					tl4 = *(long *)(lsrc+6);
				} else  if ( firstLine == 1 ) {
					tl1 = borderColor;
					tl2 = borderColor;
					tl3 = borderColor;
					tl4 = borderColor;
				} else {
					tl1 = t1;
					tl2 = t2;
					tl3 = t3;
					tl4 = t4;
				}
				if ( nextLine == 0 ) {
					tn1 = *(long *)nsrc;
					tn2 = *(long *)(nsrc+2);
					tn3 = *(long *)(nsrc+4);
					tn4 = *(long *)(nsrc+6);
				} else  if ( nextLine == 1 ) {
					tn1 = borderColor;
					tn2 = borderColor;
					tn3 = borderColor;
					tn4 = borderColor;
				} else {
					tn1 = t1;
					tn2 = t2;
					tn3 = t3;
					tn4 = t4;
				}
				
				t1 = FilterLongs(tl1,t1,tn1);
				t2 = FilterLongs(tl2,t2,tn2);
				t3 = FilterLongs(tl3,t3,tn3);
				t4 = FilterLongs(tl4,t4,tn4);
				tn1 = *(long *)dst;
				tn2 = *(long *)(dst+2);
				tn3 = *(long *)(dst+4);
				tn4 = *(long *)(dst+6);
				
				*(long *)dst =     ((tn1 & ~kLowBitsMask8) >> 1) + ((t1 & ~kLowBitsMask8) >> 1) + ((t1 & tn1) & kLowBitsMask8);
				*(long *)(dst+2) = ((tn2 & ~kLowBitsMask8) >> 1) + ((t2 & ~kLowBitsMask8) >> 1) + ((t2 & tn2) & kLowBitsMask8);
				*(long *)(dst+4) = ((tn3 & ~kLowBitsMask8) >> 1) + ((t3 & ~kLowBitsMask8) >> 1) + ((t3 & tn3) & kLowBitsMask8);
				*(long *)(dst+6) = ((tn4 & ~kLowBitsMask8) >> 1) + ((t4 & ~kLowBitsMask8) >> 1) + ((t4 & tn4) & kLowBitsMask8);
				dst += 8;
				src += 8;
				lsrc += 8;
				nsrc += 8;
			}
			for (i = 0; i < extraPixels; i++) 
			{
				ushort	y,uv;
				ushort	s,d;
				
				pixel = *src;
				
				if ( firstLine == 0 )
					pixelL = *lsrc;
				else if ( firstLine == 1 )
					pixelL = borderColor;
				else
					pixelL = pixel;
					
				if ( nextLine == 0 )
					pixelN = *nsrc;
				else if ( nextLine == 1 )
					pixelN = borderColor;
				else
					pixelN = pixel;
		
				y = FilterBytes((pixelL>>8), (pixel>>8),(pixelN>>8));
				uv = FilterChromaBytes((pixelL&0xff), (pixel&0xff),(pixelN&0xff));
					
				s = (y<<8) + uv;
				d = *dst;
				*dst++ = ((d & ~kLowBitsMask8) >> 1) + ((s & ~kLowBitsMask8) >> 1) + ((s & d) & kLowBitsMask8);
				src++;
				lsrc++;
				nsrc++;
			}
			dst += dstBump;
			src += srcBump;
			lsrc += srcBump;
			nsrc += srcBump;
			height--;
			firstLine = 0;
		}
	}
	else {
		while (height > 0)
		{
			if  ( height == 1 &&  ( nsrc >= srcEnd || doSlice ) ) 
				nextLine = p.dst.r.bottom == p.dst.device->bounds.bottom ? 1 : 2;
			for (i=0; i < quadLongCount; i++) 
			{
				register long t1,t2,t3,t4;
				register long tl1,tl2,tl3,tl4;
				register long tn1,tn2,tn3,tn4;
				
				t1 = *(long *)src;
				t2 = *(long *)(src+2);
				t3 = *(long *)(src+4);
				t4 = *(long *)(src+6);
				
				if ( firstLine == 0 ) {
					tl1 = *(long *)lsrc;
					tl2 = *(long *)(lsrc+2);
					tl3 = *(long *)(lsrc+4);
					tl4 = *(long *)(lsrc+6);
				} else  if ( firstLine == 1 ) {
					tl1 = borderColor;
					tl2 = borderColor;
					tl3 = borderColor;
					tl4 = borderColor;
				} else {
					tl1 = t1;
					tl2 = t2;
					tl3 = t3;
					tl4 = t4;
				}
				if ( nextLine == 0 ) {
					tn1 = *(long *)nsrc;
					tn2 = *(long *)(nsrc+2);
					tn3 = *(long *)(nsrc+4);
					tn4 = *(long *)(nsrc+6);
				} else  if ( nextLine == 1 ) {
					tn1 = borderColor;
					tn2 = borderColor;
					tn3 = borderColor;
					tn4 = borderColor;
				} else {
					tn1 = t1;
					tn2 = t2;
					tn3 = t3;
					tn4 = t4;
				}
				
				*(long *)dst 	  = FilterLongs(tl1,t1,tn1);
				*(long *)(dst+2)  = FilterLongs(tl2,t2,tn2);
				*(long *)(dst+4)  = FilterLongs(tl3,t3,tn3);
				*(long *)(dst+6)  = FilterLongs(tl4,t4,tn4);
				
				dst += 8;
				src += 8;
				lsrc += 8;
				nsrc += 8;
			}
			for (i = 0; i < extraPixels; i++) 
			{
				ushort	y,uv;
				
				pixel = *src;
				
				if ( firstLine == 0 )
					pixelL = *lsrc;
				else if ( firstLine == 1 )
					pixelL = borderColor;
				else
					pixelL = pixel;
					
				if ( nextLine == 0 )
					pixelN = *nsrc;
				else if ( nextLine == 1 )
					pixelN = borderColor;
				else
					pixelN = pixel;
		
				y = FilterBytes((pixelL>>8), (pixel>>8),(pixelN>>8));
				uv = FilterChromaBytes((pixelL&0xff), (pixel&0xff),(pixelN&0xff));
				*dst++ = (y<<8) + uv;
				src++;
				lsrc++;
				nsrc++;
			}
			dst += dstBump;
			src += srcBump;
			lsrc += srcBump;
			nsrc += srcBump;
			height--;
			firstLine = 0;
		}
	}
}

void
ScaleBlitYUYVtoYUYV(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register uchar*		src = (uchar*)p.src.base;
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	ushort				t,y,uv;
	ushort 				sy,suv;
	long				sa,da;
	int					srcInc = p.src.xInc * 2;



	src += ((p.src.leftBits>>4) & 1)<<1;	// advance to ragged word boundary
	dst += ((p.dst.leftBits>>4) & 1)<<1;	// advance to ragged word boundary
	if ( srcInc < 0 ) {
		srcBump = p.src.bump + 2;
		sa = ((ulong)(src + (pixelCount<<1))) & 2;
	} else {
		srcBump = (p.src.bump - (pixelCount<<1) - 2);
		sa = ((ulong)src) & 2;
	}
	da = ((ulong)dst) & 2;
	
	dstBump = (p.dst.bump - (pixelCount<<1) - 2);
		
	Boolean srcHasTransparentColor = (p.src.device->transparentColor & kTransparent) != 0;

	if ( sa == da  ) 
	{
		if (srcHasTransparentColor )
		{
			if (transparency == kNotTransparent)
				while (height > 0)
				{
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					for (long i=0; i <= pixelCount; i++)
					{	
						t = *(ushort*)src;
						if ( (t & 0xff00) != (kTransparentY << 8) )
						{
							*(ushort*)dst = t;
						}
						dst += 2;
						src += srcInc;
					}
//					if ( ((ulong)dst & 2) != 0 && src[-srcInc] != kTransparentY )
//						dst[1] = CLIPCHROMA((dst[1] + src[-srcInc>>1])>>1);
					dst += dstBump;
					src += srcBump;
					height--;
				}
			else	// transparency case
	
				while (height > 0)
				{
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					for (long i=0; i <= pixelCount; i++)
					{	
						t = *(ushort*)src;
						sy = t>>8;
						if (sy != kTransparentY)
						{
							suv = (uchar)t;
							t = *(ushort*)dst;
							y = t>>8;
							uv = (uchar)t;
							y = TransparencyBlendY(transparency,sy,y);
							uv = TransparencyBlend(transparency,suv,uv);
							*(ushort*)dst = (CLIPLUMINANCE(y)<<8) + CLIPCHROMA(uv);
						}
						dst += 2;
						src += srcInc;
					}
					if ( ((ulong)dst & 2) != 0 && src[-srcInc] != kTransparentY )
						dst[1] = CLIPCHROMA(TransparencyBlend(transparency>>1,src[-srcInc>>1],dst[1]));
					dst += dstBump;
					src += srcBump;
					height--;
				}
		}
		else
		{
			if (transparency == kNotTransparent)
				while (height > 0)
				{
					long i=0;
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					else if ( ((ulong)src & 3) == 0 && ((ulong)dst & 3) == 0  )
					{
						long sil = srcInc<<1;
						for (; i < (pixelCount-1); i += 2)  {
							*(ulong *)dst = *(ulong *)src;
							dst += 4;
							src += sil;
						}
					}
					for (; i <= pixelCount-4; i += 4) 
					{
						register ushort a,b,c,d;
						
						a = *(ushort *)src;
						src += srcInc;
						b = *(ushort *)src;
						src += srcInc;
						c = *(ushort *)src;
						src += srcInc;
						d = *(ushort *)src;
						src += srcInc;
						*(ushort *)dst = a;
						dst += 2;
						*(ushort *)dst = b;
						dst += 2;
						*(ushort *)dst = c;
						dst += 2;
						*(ushort *)dst = d;
						dst += 2;
					}
					for (; i <= pixelCount; i++) 
					{
						*(ushort *)dst = *(ushort *)src;
						dst += 2;
						src += srcInc;
					}
//					if ( ((ulong)dst & 2) != 0  )
//						dst[1]= CLIPCHROMA(src[-srcInc>>1] + dst[1])>>1;
					dst += dstBump;
					src += srcBump;
					height--;
				}
			else if (transparency == kHalfTransparent)
				while (height > 0)
				{
					long i=0;
					
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					else if ( ((ulong)src & 3) == 0 && ((ulong)dst & 3) == 0  )
					{
						long sil = srcInc<<1;
						ulong s,d;
						for (; i < (pixelCount-1); i += 2)  {
							s = *(ulong *)src;
							d = *(ulong *)dst;
							*(ulong *)dst = ((d & ~kLowBitsMask8) >> 1) + ((s & ~kLowBitsMask8) >> 1) + ((s & d) & kLowBitsMask8);
							dst += 4;
							src += sil;
						}
					}
					for (; i <= pixelCount-4; i += 4) 
					{
						register ushort a,b,c,d,e;
						a = *(ushort *)src;
						src += srcInc;
						b = *(ushort *)src;
						src += srcInc;
						c = *(ushort *)src;
						src += srcInc;
						d = *(ushort *)src;
						src += srcInc;
						e = *(ushort *)dst;
						*(ushort *)dst = ((e & ~kLowBitsMask8) >> 1) + ((a & ~kLowBitsMask8) >> 1) + ((a & e) & kLowBitsMask8);
						dst += 2;
						e = *(ushort *)dst;
						*(ushort *)dst = ((e & ~kLowBitsMask8) >> 1) + ((b & ~kLowBitsMask8) >> 1) + ((b & e) & kLowBitsMask8);
						dst += 2;
						e = *(ushort *)dst;
						*(ushort *)dst = ((e & ~kLowBitsMask8) >> 1) + ((c & ~kLowBitsMask8) >> 1) + ((c & e) & kLowBitsMask8);
						dst += 2;
						e = *(ushort *)dst;
						*(ushort *)dst = ((e & ~kLowBitsMask8) >> 1) + ((d & ~kLowBitsMask8) >> 1) + ((d & e) & kLowBitsMask8);
						dst += 2;
					}
					for (; i <= pixelCount; i++) 
					{
						ushort s,d;
						d = *(ushort *)dst;
						s = *(ushort *)src;
						*(ushort *)dst = ((d & ~kLowBitsMask8) >> 1) + ((s & ~kLowBitsMask8) >> 1) + ((s & d) & kLowBitsMask8);
						dst += 2;
						src += srcInc;
					}
					if ( ((ulong)dst & 2) == 0 )
						dst[1] = CLIPCHROMA(TransparencyBlend(transparency, src[-srcInc>>1], dst[1]));
					dst += dstBump;
					src += srcBump;
					height--;
				}
			else
			
				while (height > 0)
				{
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					for (long i=0; i <= pixelCount; i++)
					{
						t = *(ushort*)src;
						sy = t>>8;
						suv = (uchar)t;
						t = *(ushort*)dst;
						y = t>>8;
						uv = (uchar)t;
						y = TransparencyBlendY(transparency,sy,y);
						uv = TransparencyBlend(transparency,suv,uv);
						*(ushort*)dst = (CLIPLUMINANCE(y)<<8) + CLIPCHROMA(uv);
						dst += 2;
						src += srcInc;
					}
					if ( ((ulong)dst & 2) == 0 )
						dst[1] = CLIPCHROMA(TransparencyBlend(transparency,src[-srcInc>>1], dst[1]));
					dst += dstBump;
					src += srcBump;
					height--;
				}
		}
	}
	
	// u and v are misaligned in source and destination
	
	else 
	{
		short	puv,td;
		
		if (srcHasTransparentColor)
		{
			if (transparency == kNotTransparent)
				while (height > 0)
				{
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					puv = src[3];
					for (long i=0; i <= pixelCount; i++)
					{	
						t = *(ushort*)src;
						y = 0xff00 & t;
						if (y != (kTransparentY<<8))
						{
							*(ushort *)dst = y | puv;
						}
						puv = (uchar)t;
						src += srcInc;
						dst += 2;
					}
//					if ( ((ulong)dst & 2) != 0 && src[-srcInc] != kTransparentY )
//						dst[1] = CLIPCHROMA(puv + dst[1])>>1;
					dst += dstBump;
					src += srcBump;
					height--;
				}
			else	// transparency case
	
				while (height > 0)
				{
					puv = src[3];
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					for (long i=0; i <= pixelCount; i++)
					{	
						t = *(ushort*)src;
						sy = t>>8;
						if (sy != kTransparentY)
						{
							td = *(ushort*)dst;
							y = td>>8;
							uv = (uchar)td;
							y = TransparencyBlendY(transparency,sy,y);
							uv = TransparencyBlend(transparency,puv,uv);
							*(ushort*)dst = CLIPLUMINANCE(y<<8) + CLIPCHROMA(uv);
						}
						puv = (uchar)t;
						src += srcInc;
						dst += 2;
					}
					if ( ((ulong)dst & 2) != 0 && src[-srcInc] != kTransparentY )
						dst[1] = CLIPCHROMA(TransparencyBlend(transparency>>1,puv,dst[1]));
					dst += dstBump;
					src += srcBump;
					height--;
				}
		}
		else
		{
			if (transparency == kNotTransparent)
			{
				while (height > 0)
				{
					register ushort y1,y2,y3,y4;
					register ushort uv1,uv2,uv3,uv4;
					long i;
					
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					uv1 = src[srcInc+1];
					for (i=0; i <= pixelCount-4; i += 4) 
					{
						y1 = *(ushort*)src;
						src += srcInc;
						uv2 = (uchar)y1;
						y2 = *(ushort*)src;
						src += srcInc;
						uv3 = (uchar)y2;
						y3 = *(ushort*)src;
						src += srcInc;
						uv4 = (uchar)y3;
						y4 = *(ushort*)src;
						src += srcInc;
						*(ushort*)dst = (y1 & 0xff00) + uv1;
						dst += 2;
						*(ushort*)dst = (y2 & 0xff00) + uv2;
						dst += 2;
						*(ushort*)dst = (y3 & 0xff00) + uv3;
						dst += 2;
						*(ushort*)dst = (y4 & 0xff00) + uv4;
						dst += 2;
						uv1 = (uchar)y4;
					}
					for ( ; i <= pixelCount; i++) 
					{
						y1 = *(ushort*)src;
						src += srcInc;
						*(ushort*)dst = (y1 & 0xff00) + uv1;
						uv1 = (uchar)y1;
						dst += 2;
					}
//					if ( ((ulong)dst & 2) != 0 && src[-srcInc] != kTransparentY )
//						dst[1] = (dst[1] + uv1)>>1;
					dst += dstBump;
					src += srcBump;
					height--;
				}
			}
			else
			{
				while (height > 0)
				{
					if  ( srcInc < 0 )
						src += pixelCount<<1;
					puv = CLIPCHROMA(TransparencyBlend(transparency>>1,src[srcInc+1], dst[1]));
					for (long i=0; i <= pixelCount; i++)
					{
						uv = puv;
						t = *(ushort*)src;
						sy = t>>8;
						td = *(ushort*)dst;
						y = td>>8;
						uv = (uchar)td;
						y = TransparencyBlendY(transparency,sy,y);
						uv = TransparencyBlend(transparency,puv,uv);
						*(short*)dst = (CLIPLUMINANCE(y)<<8) + CLIPCHROMA(uv);
						puv = (uchar)t;
						src += srcInc;
						dst += 2;
					}
					if ( ((ulong)dst & 2) != 0 && src[-srcInc] != kTransparentY )
						dst[1] = CLIPCHROMA(TransparencyBlend(transparency>>1,puv,*dst));
					dst += dstBump;
					src += srcBump;
					height--;
				}
			}
		}
	}
}




void
ScaleBlit4toYUYV(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	Byte*				src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register const Byte*		cTable = GetCLUTYUV(p.src.device);
	register Byte*		sp;
	register Byte*		dp;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	uchar				index,index2;
	short				transIndex = -1;
	short				y,u,v,nu,lv,uv;
	int					uvphase;
	int 				uvInitialPhase;
	long				dstInc = p.src.xInc<<1;
	Boolean				oddStart = (p.src.leftBits >> 2) & 1;
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary

	if ( dstInc > 0 ) {
		uvInitialPhase = ( (p.dst.leftBits >> 4) & 1 );
	}
	else {
		uvInitialPhase =  ( ((long)(dst+(pixelCount-1)*2)  & 2) != 0 );
	}
	srcBump = p.src.bump;
	dstBump = p.dst.bump;
	
	
	if (  (p.src.device->transparentColor & kTransparent) != 0 )
		transIndex = (Byte)p.src.device->transparentColor;

	while (height > 0)
	{
		Boolean skipFirst = oddStart;
		sp = src;
		dp = dst;
		if ( dstInc < 0 )
			dp += (pixelCount - 1)<<1;
		uvphase = uvInitialPhase;
		lv = -1;
		index2 = *sp++;
		for (long i=pixelCount; i-- > 0 ;  uvphase++ )
		{
			if ( !skipFirst )
			{
				index = index2 >> 4;
				if (index != transIndex) 
				{
					const Byte* yuv = &cTable[index * 3];
					y = *yuv++;
					u = *yuv++;
					v = *yuv++;
					nu = u;
#ifdef	TRANS_UV_BLEND
					if ( i && (index2 & 0xf) != transIndex ) nu = cTable[(index2 & 0xf) * 3+1];
#endif
					if ( transparency )
					y = TransparencyBlendY(transparency,y,dp[0]);
					if ( (uvphase & 1) )
					{
						if ( lv == -1 )
							lv = v;
						uv = (v+lv)>>1;
					}
					else 
						uv = (u+nu)>>1;
					if ( transparency )
					uv = TransparencyBlend(transparency,uv,dp[1]);
					*(short *)dp = ((short)CLIPLUMINANCE(y) <<8) | CLIPCHROMA(uv);
					lv = v;
				}
				else // transparent pixel
				{
					// if the last v never got written, do a 50% blend so that the color
					// doesn't get completely hosed
					
#ifdef	TRANS_UV_BLEND
					if ( lv != -1 && (uvphase & 1) )
					{
						uchar t = (kFullyTransparent-transparency) >>1;
						uv = (v+lv)>>1;
						uv = TransparencyBlend(t,uv,dp[1]);
						dp[1] = CLIPCHROMA(uv);
					}
#endif
					lv = -1;
				}
				dp += dstInc;
				uvphase++;
				if ( i-- == 0 )
					break;
			}
			skipFirst = false;
			
			// second pixel
			
			index = index2 & 0xf;
			index2 = *sp++;
			if (index != transIndex) 
			{
				const Byte* yuv = &cTable[index * 3];
				y = *yuv++;
				u = *yuv++;
				v = *yuv++;
				nu = u;
#ifdef	TRANS_UV_BLEND
				if ( i && (index2 >> 4) != transIndex ) nu = cTable[(index2 >> 4) * 3+1];
#endif
				if ( transparency )
				y = TransparencyBlendY(transparency,y,dp[0]);
				if ( (uvphase & 1) )
				{
					if ( lv == -1 )
						lv = v;
					uv = (v+lv)>>1;
				}
				else 
					uv = (u+nu)>>1;
				if ( transparency )
				uv = TransparencyBlend(transparency,uv,dp[1]);
				
				*(short *)dp = ((short)CLIPLUMINANCE(y) <<8) | CLIPCHROMA(uv) ;
				lv = v;
			}
			else // transparent pixel
			{
				// if the last v never got written, do a 50% blend so that the color
				// doesn't get completely hosed
				
#ifdef	TRANS_UV_BLEND
				if ( lv != -1 && (uvphase & 1) )
				{
					uchar t = (kFullyTransparent-transparency) >>1;
					uv = (v+lv)>>1;
					uv = TransparencyBlend(t,uv,dp[1]);
					dp[1] = CLIPCHROMA(uv);
				}
#endif
				lv = -1;
			}
			dp += dstInc;
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}

inline void SwapUV(ulong& pix,short &lastV);


inline void
SwapUV(ulong& pix,short &lastV)
{
	ulong uv = pix;

	if ( lastV == -1 )
	{
		pix &= 0xff00ff00;
		uv &= 0x00ff00ff;
		pix |= (uv>>16);
		lastV = uv & 0xff;
	}
	else	
	{
		pix &= 0xff00ff00;
		uv &= 0x00ff00ff;
		pix |= ((ulong)lastV<<16) + (uv>>16);
		lastV = uv & 0xff;
	}
}

void
ScaleBlitVQtoYUYV(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register ulong*		codeBooks = (ulong*)&p.src.device->colorTable->data[0];

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;

	short				index;
	int					vertMisAligned,horzMisAligned;
	Boolean 			uvAligned,longAligned;
	long				srcInc = p.src.xInc;
	register ushort		*dpa,*dpb;
	short				transIndex = -1;
	short				y1,y2,u,v;
	ulong				pix;
	register ulong 		*cbp;
	uchar				*dp;
	uchar				*sp;
	
	src -= (p.src.leftBits >> 5)<<2;
	src += (p.src.leftBits >> 3);
	if ( srcInc < 0 )
		horzMisAligned = (p.src.rightBits >> 2) & 1;
	else
	horzMisAligned = (p.src.leftBits >> 2) & 1;
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary
	longAligned = (srcInc > 0 ) && (((ulong)dst & 2) == 0);

	uvAligned = ( (p.dst.leftBits >> 4) & 1 ) == 0 ;
	vertMisAligned = (p.src.r.top & 1);
	
	if ( (p.src.device->transparentColor & kTransparent) != 0)
		transIndex = (Byte)p.src.device->transparentColor;

	if ( uvAligned ) 
	{
		while (height > 0)
		{
			sp = src;
			if ( srcInc < 0 )
				sp += pixelCount/2;
			if ( vertMisAligned ) 
			{
				if ( vertMisAligned == 1 ) 
				{
					dpa = 0;
					dpb = (ushort *)(dst);
					height += 1;		// really only doing one line this time
					vertMisAligned = 2;
				}
				else 
				{	
					if ( height > 1 )
					{
						dpa = (ushort *)(dst-p.dst.device->rowBytes);
						dpb = (ushort *)dst;
					}
					else
					{
						dpa = (ushort *)(dst-p.dst.device->rowBytes);
						dpb = 0;
					}
				}
			}
			else {
				dpa = (ushort *)dst;
				if ( height > 1 )
					dpb = (ushort *)(dst+p.dst.device->rowBytes);
				else
					dpb = 0;
			} 
			long i = pixelCount;

			if ( horzMisAligned ) {
				index = *sp;
				sp += srcInc;
				cbp = codeBooks + (index<<1);
				if ( dpa ) {
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y2 = y1;
					if ( transparency ) {
						dp = (uchar *)dpa;
						dp++;
						u = TransparencyBlend(transparency,u,*dp++);
						y2 = TransparencyBlendY(transparency,y2,*dp++);
						v = TransparencyBlend(transparency,v,*dp++);
					}
					*dpa++ = (CLIPLUMINANCE(y2)<<8) + v;
				}
				if (dpb ) {
					cbp++;
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y2 = y1;
					if ( transparency ) {
						dp = (uchar *)dpb;
						dp++;
						u = TransparencyBlend(transparency,u,*dp++);
						y2 = TransparencyBlendY(transparency,y2,*dp++);
						v = TransparencyBlend(transparency,v,*dp++);
					}
					*dpb++ = (CLIPLUMINANCE(y2)<<8) + v;
				}
				i--;
			}
			while ( i > 1 ) 
			{
				index = *sp;
				sp += srcInc;
				i -= 2;
				if (index != transIndex ) 
				{
					cbp = codeBooks + (index<<1);
					if ( transparency )
					{
						if ( dpa ) 
						{
							ushort ds = *dpa;
							pix = *cbp;
							PIXELS16TOYUYV(pix,y1,u,y2,v);
							y1 = TransparencyBlendY(transparency,y1,(ds>>8));
							u = TransparencyBlend(transparency,u,(ds&0xff));
							ds = dpa[1];
							y2 = TransparencyBlendY(transparency,y2,(ds>>8));
							v = TransparencyBlend(transparency,v,(ds&0xff));
							if ( srcInc < 0 ) {
								YUYVTOPIXELS16(y2,u,y1,v,pix);
							} else {
								YUYVTOPIXELS16(y1,u,y2,v,pix);
							}
							if ( longAligned ) {
								*(long *)dpa = pix;
								dpa += 2;
							}
							else 
							{
								*dpa++ = pix>>16;
								*dpa++ = pix;
							}
						}
						cbp++;
						if ( dpb ) 
						{
							dp = (uchar *)dpb;
							pix = *cbp++;
							PIXELS16TOYUYV(pix,y1,u,y2,v);
							y1 = TransparencyBlendY(transparency,y1,*dp++);
							u = TransparencyBlend(transparency,u,*dp++);
							y2 = TransparencyBlendY(transparency,y2,*dp++);
							v = TransparencyBlend(transparency,v,*dp++);
							if ( srcInc < 0 ) {
								YUYVTOPIXELS16(y2,u,y1,v,pix);
							} else {
							YUYVTOPIXELS16(y1,u,y2,v,pix);
							}
							if ( longAligned ) {
								*(long *)dpb = pix;
								dpb += 2;
							}
							else 
							{
								*dpb++ = pix>>16;
								*dpb++ = pix;
							}
						}
					} 
					else 
					{
						if ( longAligned ) 
						{
							if ( dpa ) {
								*(long *)dpa = *cbp;
								dpa += 2;
							}
							cbp++;
							if ( dpb ) {
								*(long *)dpb = *cbp;
								dpb += 2;
							}
						}
						else if ( srcInc > 0 )
						{
							if ( dpa )
							{
								pix = *cbp;
								*dpa++ = pix>>16;
								*dpa++ = pix;
							}
							cbp++;
							if ( dpb ) 
							{
								pix = *cbp++;
								*dpb++ = pix>>16;
								*dpb++ = pix;
							}
						}
						else 
						{
							if ( dpa )
							{
								pix = *cbp;
								*dpa++ = (( pix>>16) & 0x00ff) | (pix & 0xff00);
								*dpa++ = (( pix>>16) & 0xff00) | (pix & 0x00ff);
							}
							cbp++;
							if ( dpb ) 
							{
								pix = *cbp++;
								*dpb++ = (( pix>>16) & 0x00ff) | (pix & 0xff00);
								*dpb++ = (( pix>>16) & 0xff00) | (pix & 0x00ff);
							}
						}
					}
				}
			}
			
			if ( i ) {
				index = *sp;
				sp += srcInc;
				cbp = codeBooks + (index<<1);
				if ( dpa ) {
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y1 = y2;
					if ( transparency ) {
						dp = (uchar *)dpa;
						y1 = TransparencyBlendY(transparency,y1,*dp++);
						u = TransparencyBlend(transparency,u,*dp++);
					}
					*dpa++ = (CLIPLUMINANCE(y1)<<8) + u;
				}
				if (dpb ) {
					cbp++;
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y1 = y2;
					if ( transparency ) {
						dp = (uchar *)dpb;
						y1 = TransparencyBlendY(transparency,y1,*dp++);
						u = TransparencyBlend(transparency,u,*dp++);
					}
					*dpb++ = (CLIPLUMINANCE(y1)<<8) + u;
				}
			}
			dst += p.dst.device->rowBytes * 2;
			src += p.src.device->rowBytes;
			height -= 2;
		}
	}
	else
	{
		while (height > 0)
		{
			short lastV1;
			short lastV2;
			
			sp = src;
			if ( srcInc < 0 )
				sp += pixelCount/2;
			dpa = (ushort *)dst;
			if ( vertMisAligned ) 
			{
				if ( vertMisAligned == 1 ) 
				{
					dpa = 0;
					dpb = (ushort *)(dst);
					height += 1;		// really only doing one line this time
					vertMisAligned = 2;
				}
				else 
				{	
					if ( height > 1 )
					{
						dpa = (ushort *)(dst-p.dst.device->rowBytes);
						dpb = (ushort *)dst;
						lastV2 = dst[1];
					}
					else
					{
						dpa = (ushort *)(dst-p.dst.device->rowBytes);
						dpb = 0;
					}
				}
			}
			else {
				dpa = (ushort *)dst;
				if ( height > 1 )
				{
					dpb = (ushort *)(dst+p.dst.device->rowBytes);
					lastV2 = dst[p.dst.device->rowBytes+1];
				}
				else
					dpb = 0;
			} 
			long i=pixelCount;
			lastV1 = dst[1];
			
			if ( horzMisAligned ) {
				index = *sp;
				sp += srcInc;
				cbp = codeBooks + (index<<1);
				if ( dpa ) {
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y2 = y1;
					if ( transparency ) {
						dp = (uchar *)dpa;
						dp++;
						lastV1 = TransparencyBlend(transparency,lastV1,*dp++);
						y2 = TransparencyBlendY(transparency,y2,*dp++);
						u = TransparencyBlend(transparency,u,*dp++);
					}
					*dpa++ = (CLIPLUMINANCE(y2)<<8) + u;
					lastV1 = v;
				}
				if (dpb ) {
					cbp++;
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y2 = y1;
					if ( transparency ) {
						dp = (uchar *)dpb;
						dp++;
						lastV2 = TransparencyBlend(transparency,lastV2,*dp++);
						y2 = TransparencyBlendY(transparency,y2,*dp++);
						u = TransparencyBlend(transparency,u,*dp++);
					}
					*dpb++ = (CLIPLUMINANCE(y2)<<8) + u;
					lastV2 = v;
				}
				i--;
			}
			while ( i > 1 )
			{
				index = *sp;
				sp += srcInc;
				i -= 2;
				
				if (index != transIndex ) 
				{
					cbp = codeBooks + (index<<1);
					if ( transparency )
					{	
						uchar saveV;
						
						if ( dpa ) 
						{
							dp = (uchar *)dpa;
							pix = *cbp;
							PIXELS16TOYUYV(pix,y1,u,y2,v);		
							y1 = TransparencyBlendY(transparency,y1,*dp++);
							lastV1 = TransparencyBlend(transparency,lastV1,*dp++);
							y2 = TransparencyBlendY(transparency,y2,*dp++);
							u = TransparencyBlend(transparency,u,*dp++);
							if ( srcInc < 0 ) {
								YUYVTOPIXELS16(y2,u,y1,v,pix);
							}
							else {
							YUYVTOPIXELS16(y1,lastV1,y2,u,pix);
							}
							if ( longAligned ) {
								*(long *)dpa = pix;
								dpa += 2;
							}
							else 
							{
								*dpa++ = pix>>16;
								*dpa++ = pix;
							}
							lastV1 = v;
						}
						cbp++;
						if ( dpb ) {
							dp = (uchar *)dpb;
							pix = *cbp++;
							PIXELS16TOYUYV(pix,y1,u,y2,v);			
							saveV = v;
							v = lastV2;
							lastV2 = saveV;
							y1 = TransparencyBlendY(transparency,y1,*dp++);
							lastV2 = TransparencyBlend(transparency,lastV2,*dp++);
							y2 = TransparencyBlendY(transparency,y2,*dp++);
							u = TransparencyBlend(transparency,u,*dp++);
							if ( srcInc < 0 ) {
								YUYVTOPIXELS16(y2,u,y1,v,pix);
							}
							else {
							YUYVTOPIXELS16(y1,lastV2,y2,u,pix);
							}
							if ( longAligned ) {
								*(long *)dpb = pix;
								dpb += 2;
							}
							else 
							{
								*dpb++ = pix>>16;
								*dpb++ = pix;
							}
							lastV2 = v;
						}
					}
					else 
					{
						if ( longAligned ) {
							pix = *cbp++;
							if ( dpa ) 
							{
								SwapUV(pix,lastV1);
								*(long *)dpa = pix;
								dpa += 2;
							}
							if ( dpb ) 
							{
								pix = *cbp++;
								SwapUV(pix,lastV2);
								*(long *)dpb = pix;
								dpb += 2;
							}
						}
						else if ( srcInc > 0 )
						{
							pix = *cbp++;
							if ( dpa ) 
							{
								SwapUV(pix,lastV1);
								*dpa++ = pix>>16;
								*dpa++ = pix;
							}
							if ( dpb ) 
							{
								pix = *cbp++;
								SwapUV(pix,lastV2);
								*dpb++ = pix>>16;
								*dpb++ = pix;
							}
						}
						else
						{
							pix = *cbp++;
							if ( dpa ) 
							{
								SwapUV(pix,lastV1);
								*dpa++ = (( pix>>16) & 0x00ff) | (pix & 0xff00);
								*dpa++ = (( pix>>16) & 0xff00) | (pix & 0x00ff);
							}
							if ( dpb ) 
							{
								pix = *cbp++;
								SwapUV(pix,lastV2);
								*dpb++ = (( pix>>16) & 0x00ff) | (pix & 0xff00);
								*dpb++ = (( pix>>16) & 0xff00) | (pix & 0x00ff);
							}
						}
					}
				}
			}
			if ( i ) {
				index = *sp;
				sp += srcInc;
				cbp = codeBooks + (index<<1);
				if ( dpa ) {
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y1 = y2;
					if ( transparency ) {
						dp = (uchar *)dpa;
						y1 = TransparencyBlendY(transparency,y1,*dp++);
						lastV1 = TransparencyBlend(transparency,lastV1,*dp++);
					}
					*dpa++ = (CLIPLUMINANCE(y1)<<8) + lastV1;
				}
				if (dpb ) {
					cbp++;
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					if ( srcInc < 0 )
						y1 = y2;
					if ( transparency ) {
						dp = (uchar *)dpb;
						y1 = TransparencyBlendY(transparency,y1,*dp++);
						lastV2 = TransparencyBlend(transparency,lastV2,*dp++);
					}
					*dpb++ = (CLIPLUMINANCE(y1)<<8) + lastV2;
				}
				i--;
			}
			dst += p.dst.device->rowBytes * 2;
			src += p.src.device->rowBytes;
			height -= 2;
		}
	}
}



void
ScaleBlit8toYUYV(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register const Byte*		cTable = GetCLUTYUV(p.src.device);

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	uchar				index;
	short				transIndex = -1;
	short				y,u,v,nu,lv,uv;
	int					uvphase;
	int 				uvInitialPhase;
	long				srcInc = p.src.xInc;
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary


	uvInitialPhase = ( (p.dst.leftBits >> 4) & 1 );
	
	if ( srcInc < 0 )
		srcBump = p.src.bump + 1;
	else	
		srcBump = p.src.bump - pixelCount - 1;
	dstBump = (p.dst.bump - pixelCount * 2 - 2);
	
	if ( (p.src.device->transparentColor & kTransparent) != 0)
		transIndex = (Byte)p.src.device->transparentColor;

	while (height > 0)
	{
		uvphase = uvInitialPhase;
		lv = -1;
		if ( srcInc < 0 )
			src += pixelCount;
			
		
		if ( transIndex == -1 && transparency == kNotTransparent )
		{
			// fast case
			for (long i=pixelCount; i-- >= 0 ; uvphase++ )
			{
				const Byte* yuv = &cTable[*src * 3];
				src += srcInc;
				nu = cTable[*src * 3 +1];
				y = *yuv++;
				u = *yuv++;
				v = *yuv++;
				if ( (uvphase & 1) )
				{
					if ( lv == -1 )
						lv = v;
					uv = (v+lv)>>1;
				}
				else 
				{
					if ( i < 0 )	
						nu = u;
					uv = (u + nu)/2;
				}
				*(short *)dst = ((short)CLIPLUMINANCE(y) << 8) | CLIPCHROMA(uv);
				dst += 2;
				lv = v;
			}
		}
		else 
		{
			// hard case
			for (long i=pixelCount; i-- >= 0 ; uvphase++ )
			{
				index = *src;
				src += srcInc;
	#ifdef	TRANS_UV_BLEND
				ushort nextIndex = *src;
	#endif
				if (index != transIndex) 
				{
					const Byte* yuv = &cTable[index * 3];
					y = *yuv++;
					u = *yuv++;
					v = *yuv++;
					nu = u;
	#ifdef	TRANS_UV_BLEND
					if ( i >= 0 && nextIndex != transIndex )
						nu = cTable[nextIndex * 3 +1];
	#endif
					if ( transparency )
					y = TransparencyBlendY(transparency,y,dst[0]);
					if ( (uvphase & 1) )
					{
						if ( lv == -1 )
							lv = v;
						uv = (v+lv)>>1;
					}
					else 
						uv = (u+nu)>>1;
					if ( transparency )
					uv = TransparencyBlend(transparency,uv,dst[1]);
					*(short *)dst = ((short)CLIPLUMINANCE(y) << 8) | CLIPCHROMA(uv);
					dst += 2;
					lv = v;
				}
				else // transparent pixel
				{
					dst++;
					
					// if the last v never got written, do a 50% blend so that the color
					// doesn't get completely hosed
					
	#ifdef	TRANS_UV_BLEND
					if ( lv != -1 && (uvphase & 1) )
					{
						uchar t = (kFullyTransparent-transparency)>>1;
						uv = (v+lv)>>1;
						uv = TransparencyBlend(t,uv,*dst);
						*dst = CLIPCHROMA(uv);
					}
	#endif
					dst++;
					lv = -1;
				}
			}
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}


void
ScaleBlitAlpha8toYUYV(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register Byte*		clut =p.src.device->colorTable->data;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	short				transIndex = -1; // wont match
	uchar				index;
	short				y,u,v,lv,uv;
#ifdef	TRANS_UV_BLEND
	short				nu;
#endif

	int					uvphase;
	int 				uvInitialPhase;
	long				srcInc = p.src.xInc;
	uchar				a;
	short				dy;
	uchar				unTransparency = (kFullyTransparent-transparency);
	ushort				dpix;
	ulong				pix;
	
	if ( IsWarning((p.src.device->colorTable->version & 0xf) != kYUV32) )
	{
		Complain(("incompatible color table"));
		ScaleBlit8toYUYV(p,transparency);
		return;
	}
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary

	uvInitialPhase = ( (p.dst.leftBits >> 4) & 1 );
	
	if ( srcInc < 0 )
		srcBump = p.src.bump + 1;
	else	
		srcBump = p.src.bump - pixelCount - 1;
	dstBump = (p.dst.bump - pixelCount * 2 - 2);
	
	
	if ( (p.src.device->transparentColor & kTransparent) != 0)
		transIndex = (Byte)p.src.device->transparentColor;

	
	while (height > 0)
	{
		uvphase = uvInitialPhase;
		lv = -1;
		if ( srcInc < 0 )
			src += pixelCount;
		for (long i=0; i  <= pixelCount; i++,uvphase++ )
		{
			index=*src;
			src += srcInc;
#ifdef	TRANS_UV_BLEND
			ushort nextIndex = *src;
#endif
			if ( index != transIndex ) 
			{	
				pix =  *(((ulong*)clut) + index);

				PIXEL32TOYUVA(pix,y,u,v,a);
				if ( transparency )
					a = (a * unTransparency) >> 8;

				dpix = *(short *)dst;
				dy = dpix>>8;
				dpix &= 0xff;
				y = AlphaBlend(a,y,dy);
#ifdef	TRANS_UV_BLEND
				if ( i != pixelCount-1 && *src != transIndex )
					nu = clut[ (nextIndex<<2) +1];
				else	
					nu = u;
#endif
				if ( (uvphase & 1) )
				{
					if ( lv == -1 )
						lv = v;
					uv = AlphaBlend(a,((v+lv)>>1),dpix);
				}
				else 
				{
#ifdef	TRANS_UV_BLEND
					uv = AlphaBlend(a,((u+nu)>>1),dpix);
#else
					uv = AlphaBlend(a,u,dpix);
#endif
				}
				*(short *)dst = ((short)CLIPLUMINANCE(y) <<8) | CLIPCHROMA(uv);
				dst += 2;
				lv = v;
			}
			else // transparent pixel
			{
				dst++;
				
				// if the last v never got written, do a 50% blend so that the color
				// doesn't get completely hosed
				
#ifdef	TRANS_UV_BLEND
				if ( lv != -1 && (uvphase & 1) )
				{
					uv = AlphaBlend(unTransparency >> 1,((v+lv)>>1),*dst);
					*dst = CLIPCHROMA(uv) ;
				}
#endif
				dst++;
				lv = -1;
			}
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}



void
ScaleBlitAntiAlias8toYUYV(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register short*		d = (short*)p.dst.base;
	register uchar 		*dst;
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	int					uvphase = 0;
	int					uvInitialPhase = 0;
	short				y,u,v;
	uchar				y1,y2 = 0,u1,v1 = kUVOffset;
	short 				alpha = 0;
	short 				nextalpha = 0;
	ulong				fColor = p.src.device->foregroundColor;
	uchar				r,g,b;
	uchar				unTransparency = kFullyTransparent- transparency;
	
	PIXEL32TORGB(fColor,r,g,b);
	RGBTOYUV(r,g,b,y,u,v);


	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	d += ((p.dst.leftBits >> 4) & 1);	// advance to ragged word boundary (dst is a ushort*)
	dst = (uchar*)d;
	
	if ( (p.dst.leftBits >> 4) & 1 )
		uvInitialPhase++;
		
	srcBump = p.src.bump - pixelCount - 1;
	dstBump = (p.dst.bump - pixelCount * 2 - 2);
		
	while (height > 0)
	{
		if ( uvInitialPhase )
		{
			y2 = dst[0];
			v1 = dst[1];
			alpha = src[0];
			nextalpha = src[1];
			alpha = (alpha * unTransparency) >> 8;
			nextalpha = (nextalpha * unTransparency) >> 8;	// apply transparency
			nextalpha = (alpha + nextalpha)>>1;
			dst[-1] = AlphaBlend(nextalpha,u,dst[-1]);
		}
		uvphase = uvInitialPhase;
		for (long i=0; i <= pixelCount; i++, uvphase++)
		{
			alpha = *src++;
			alpha = (alpha * unTransparency) >> 8;	// apply transparency
			if ( !(uvphase&1) )
			{
				ulong dpix = *(long *)dst;
				PIXELS16TOYUYV(dpix,y1,u1,y2,v1);
				*dst++ =AlphaBlend(alpha,y,y1);
				if ( i < pixelCount )
				{
					nextalpha = *src;
					nextalpha = (nextalpha * unTransparency) >> 8;	// apply transparency
					nextalpha = (alpha + nextalpha)>>1;
				}
				else
					nextalpha = alpha/2;
				*dst++ = AlphaBlend(nextalpha,u,u1);
			}
			else 
			{
				*dst++ = AlphaBlend(alpha,y,y2);
				*dst++ = AlphaBlend(nextalpha,v,v1);
			}
		}
		
		// if last pixel in row was the U of a UV pair, then we have to adjust the next V in the dst
		
		if ( (uvphase&1) )
			dst[1] = AlphaBlend(alpha,v,dst[1]);
		dst += dstBump;
		src += srcBump;
		height--;
	}
}

void
ScaleBlitAntiAlias4toYUYV(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register short*		d = (short*)p.dst.base;
	uchar 				*dst;
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;
	register long		srcBump;
	register long		dstBump;
	int					uvphase = 0;
	int					uvInitialPhase = 0;
	short				y,u,v;
	uchar				y1,y2 = 0,u1,v1 = kUVOffset;
	ushort 				alpha;
	ushort 				nextalpha = 0;
	uchar				index;
	ulong				fColor = p.src.device->foregroundColor;
	uchar				r,g,b;
	Boolean				oddStart;
	long				fpixshift,spixshift;
	long				srcInc;
	uchar				unTransparency = kFullyTransparent - transparency;
	
	PIXEL32TORGB(fColor,r,g,b);
	RGBTOYUV(r,g,b,y,u,v);

	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	d += ((p.dst.leftBits >> 4) & 1);	// advance to ragged word boundary (dst is a ushort*)
	dst = (uchar*)d;
	srcBump = p.src.bump;
	oddStart = (p.src.leftBits >> 2) & 1;
	srcInc = p.src.xInc;
	
	if ( (p.dst.leftBits >> 4) & 1 )
		uvInitialPhase++;
		
	dstBump = (p.dst.bump - pixelCount * 2);
	if ( srcInc < 0 )
	{
		fpixshift = 0;
		spixshift = 4;
	}
	else
	{
		fpixshift = 4;
		spixshift = 0;
	}
		
	while (height > 0)
	{
		uchar *sp = src;
		uvphase = uvInitialPhase;
		Boolean skipFirst = oddStart;
		if ( srcInc < 0 )
			sp += (pixelCount+1)/2;

		if ( uvInitialPhase )
		{
			index = *sp;
			y2 = dst[0];
			v1 = dst[1];
			if ( oddStart )
			{
				alpha = (index >> spixshift) & 0xf;
				nextalpha = (sp[srcInc] >> fpixshift) & 0xf;
			} 
			else
			{
				alpha = (index >> fpixshift) & 0xf;
				nextalpha = (index >> spixshift) & 0xf;
			}
			alpha = (alpha<<4) + alpha;
			nextalpha = (nextalpha<<4) + nextalpha;
			if ( transparency )
			{
				nextalpha = (nextalpha * unTransparency) >> 8;
				alpha = (alpha * unTransparency) >> 8;
			}
			nextalpha = (nextalpha + alpha)>>1;
			dst[-1] = CLIPCHROMA(AlphaBlend(nextalpha,u,dst[-1]));
		}
		for (long i=0; i < pixelCount ; i++, uvphase++)
		{
			index = *sp;
			sp += srcInc;
			if ( !skipFirst ) 
			{
				alpha = (index >> fpixshift) & 0xf;
				alpha = (alpha<<4) + alpha;
				if ( transparency )
					alpha = (alpha * unTransparency) >> 8;	// apply transparency
				if ( !(uvphase&1) )
				{
					ulong dpix = *(long *)dst;
					if ( i == pixelCount-1 )				// end of row
						nextalpha = alpha/2;
					else {
						nextalpha = (index >> spixshift) & 0xf;
						nextalpha = (nextalpha<<4) + nextalpha;
						if ( transparency )
							nextalpha = (nextalpha * unTransparency) >> 8;	
						nextalpha = (nextalpha + alpha)>>1;
					}
					PIXELS16TOYUYV(dpix,y1,u1,y2,v1);
					*(short *)dst = (CLIPLUMINANCE(AlphaBlend(alpha,y,y1))<<8) + CLIPCHROMA(AlphaBlend(nextalpha,u,u1));
				}
				else 
					*(short *)dst = (CLIPLUMINANCE(AlphaBlend(alpha,y,y2))<<8) + CLIPCHROMA(AlphaBlend(nextalpha,v,v1));
				dst += 2;
				uvphase++;
				if ( ++i ==  pixelCount )
					break;
			}
			skipFirst = false;
			
			// second pixel
			
			alpha = (index >> spixshift) & 0xf;
			alpha = (alpha<<4) + alpha;
			if ( transparency )
				alpha = (alpha * unTransparency) >> 8;	// apply transparency
			if ( !(uvphase&1) )
			{
				ulong dpix = *(long *)dst;
				if ( i == pixelCount-1 )			// end of row
					nextalpha = alpha/2;
				else {
					nextalpha = (*sp >> fpixshift) & 0xf;
					nextalpha = (nextalpha<<4) + nextalpha;
					if ( transparency )
						nextalpha = (nextalpha * unTransparency) >> 8;	
					nextalpha = (nextalpha + alpha)>>1;
				}
				PIXELS16TOYUYV(dpix,y1,u1,y2,v1);
				*(short *)dst = (CLIPLUMINANCE(AlphaBlend(alpha,y,y1))<<8) + CLIPCHROMA(AlphaBlend(nextalpha,u,u1));
			}
			else 
				*(short *)dst = (CLIPLUMINANCE(AlphaBlend(alpha,y,y2))<<8) + CLIPCHROMA(AlphaBlend(nextalpha,v,v1));
			dst += 2;
		}

		// if last pixel in row was the U of a UV pair, then we have to adjust the next V in the dst
		if ( (uvphase&1) ) {
			dst[1] = CLIPCHROMA(AlphaBlend(nextalpha,v,v1));
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}




const kExtraSrcLeft = 4;	
const kMaxScanLineWidth = 1024;
const kMaxScaleFactor = 32;
const kResizeBufferSize = (kMaxScanLineWidth + kExtraSrcLeft+kMaxScaleFactor) *2;
ushort	gResizeBuffer[kResizeBufferSize];
const BitMapFormat kResizeBufferFormat = yuv422Format;
const kResizeBufferPixelSize = 2;		// 16 bits/pixel
const kResizeBufferDepthShift = 4;		




typedef struct {
	ushort *srcBase;
	ushort *dstBase;
	long	leftSkipDst;
	long	leftSkipSrc;
	long 	srcInc;
	long 	srcIncRecip;
	long 	newNumPixels;
	long 	skipStart;
	Boolean hasTransparent;
	long	srcWidth;
	long	dstWidth;
} ResizeParams;


static void
CopyScanLine(ushort *src,ushort *dst,ushort length);

static void
CopyScanLine(ushort *src,ushort *dst,ushort length)
{
	short 	y,uv;
	
	if ( ((ulong)dst) & 2 ) {		// misab
		uv = ((uchar *)src) [3];
		while ( length-- != 0  ) {
			y = *src++;
			*dst++ = (y & 0xff00) | uv;
			uv = y & 0xff;
		}
	}
	else
		CopyMemory(src,dst,length*kResizeBufferPixelSize);
}



//#define	WeightAccLuminance(w,p)			(((short)((w)>>(kFract/2)) * ((short)(p)-(short)kBlackY)))		// 16x16 multiply
//#define	WeightAccChrominance(w,p)		(((short)((w)>>(kFract/2)) * ((short)(p))))		// 16x16 multiply

#define	WeightAccLuminance(w,p)			((ushort)((w)) * ((short)(p)-(short)kBlackY))		// 16x16 multiply
#define	WeightAccChrominance(w,p)		((ushort)((w)) * ((short)(p)))		// 16x16 multiply


#define	BlendPixelsY(a,b,w)			((((ushort)((a)-kBlackY) * (w)) + ((ushort)((b)-kBlackY) * ((ushort)256-(w)))>>8) + kBlackY)
#define	BlendPixels(a,b,w)			(((ushort)(a) * (w)) + ((ushort)(b) * ((ushort)256-(w)))>>8)

void
ResizeYUYVScanline(ResizeParams *p);



#define	USFracMul(a,b)		( a == kFractOne ? b : (((ushort)a * (ushort)b)>>kFract))



void
ResizeYUYVScanline(ResizeParams *p)
{
	
	long 	y,uv;
	register uchar 	*yp,*uvp;
	register long  	srcPos = 0;
	register long srcInc = p->srcInc;
	register ushort	*dst = p->dstBase;
	register ushort  *src = p->srcBase;
	long 	i,j;
	long 	srcNext;
	long  	srcX;
	long 	uvc;
	
	if ( srcInc == kFractOne ) {
		if ( src != dst ) {
			CopyScanLine(src,dst,p->newNumPixels);
		}
		return;
	}
	if ( srcInc > kFractOne  )			// shrink
	{
		ulong fract;
		ulong weightScale = p->srcIncRecip;
		uchar *luvp = (uchar *)(src - p->skipStart + p->srcWidth) - 3;
		
		if ( p->hasTransparent ) 
		{
			i=0;
			while ( 1 )   
			{	
				long tc = 0;
				srcX = FracToInt(srcPos);
				srcNext = srcPos + srcInc;
				
				if ( srcX >= p->skipStart ) 
				{	
					srcX -= p->skipStart;
					yp = (uchar*)(src + srcX);
					uvp = yp+1;
					if ( ((((ulong)dst) & 2) != 0 ) != (srcX & 1) ) {
						uvp += 2;
					}
					fract = USFracMul((kFractOne - FracMan(srcPos)),weightScale);
					if ( *yp == kTransparentY) {
						y = WeightAccLuminance(fract,kBlackY + kRangeY/2);
						uv = (long)kUVOffset << kFract;
						tc = 1;
						}
					else {
						y = WeightAccLuminance(fract,*yp);
						uv = (long)*uvp << kFract;
					}
					uvc = 1;
					for ( j = FracToInt(srcNext) - FracToInt(srcPos); j > 0;  j-- )  {
						yp += 2;
						if ( (j & 1) == 0 ) {
							if ( uvp < (luvp-4) ) {
							uvp += 4;
								uv += (long)*uvp << kFract;
								uvc++;
							}
						}
						if ( j > 1 ) {
							if ( *yp == kTransparentY ) {
								tc++;
								y +=  WeightAccLuminance(weightScale,y);
							} else{ 
								y += WeightAccLuminance(weightScale,*yp);
							}
						} else {
							fract = USFracMul(FracMan(srcNext),weightScale);
							if ( *yp == kTransparentY ) {
								y += WeightAccLuminance(fract,y);
								tc++;
							} else {
								y += WeightAccLuminance(fract,*yp);
							}
						}
					}
					if ( tc > 0 ) {
						*dst++ = (ushort)kTransparentYUYV;
					} else {
						y >>= kFract;
						switch ( uvc ) {
						case 1:	
							uv >>= kFract;
							break;
						case 2:
							uv >>= (kFract+1);
							break;
						case 4:	
							uv >>= (kFract+2);
							break;
						default:
							uv /= (uvc<<kFract);
							break;
						}
						y += kBlackY;
						*dst++ = (CLIPLUMINANCE(y)<<8) | CLIPCHROMA(uv);
					}
					if ( ++i == p->newNumPixels )
						break;
				}
				srcPos = srcNext;
			} 
		}
		else
		{
			i=0;
			while ( 1 )   
			{	
				srcX = FracToInt(srcPos);
				srcNext = srcPos + srcInc;
				if ( srcX >= p->skipStart ) 
				{	
					srcX -= p->skipStart;
					yp = (uchar*)(src + srcX);
					uvp = yp+1;
					if ( ((((ulong)dst) & 2) != 0 ) != ((srcX & 1) == 1)  ) {
						uvp += 2;
					}
					fract = USFracMul((kFractOne - FracMan(srcPos)),weightScale);
					y = WeightAccLuminance(fract,*yp);
					uv = (long)(*uvp) << kFract;
					uvc = 1;
					for ( j = FracToInt(srcNext) - FracToInt(srcPos); j > 0;  j-- ) 
					{
						yp += 2;
						if ( (j & 1) == 0 ) {
							if ( uvp < (luvp-4) ) {
								uvp += 4;
								uv += (long)(*uvp) << kFract;
								uvc++;
							}
						}
						if ( j > 1 ) {
							y += WeightAccLuminance(weightScale,*yp);
						} else {
							fract = USFracMul(FracMan(srcNext),weightScale);
							y += WeightAccLuminance(fract,*yp);
						}
					}
					y >>= kFract;
					y += kBlackY;
					switch ( uvc ) {
					case 1:	
						uv >>= kFract;
						break;
					case 2:
						uv >>= (kFract+1);
						break;
					case 4:	
						uv >>= (kFract+2);
						break;
					default:
						uv /= (uvc<<kFract);
						break;
					}
					*dst++ = (CLIPLUMINANCE(y)<<8) | CLIPCHROMA(uv);
					if ( ++i == p->newNumPixels )
						break;
				}
				srcPos = srcNext;
			}
		}
	}
	
	else		// stretch
	{
	
		register ushort z,u,v = 0xffff;
		register long  srcI,dstI;
		ushort *lsrc;
		short	ynext;
		long	dstInc = IntToFrac(p->dstWidth) / p->srcWidth;
		long	dstPos = 0;
		ushort	weight = 0;

		z = *src;
		y = 0xff & (z >> 8);
		if ( y == kTransparentY )	
			u = v = kUVOffset;
		else if ( ((((ulong)src) & 2) == 0 ) ) 
		{
			v = z & 0xff;
			u = src[1] & 0xff;
		}
		else 
		{
			u = z & 0xff;
			v = src[1] & 0xff;
		}
		src -= p->leftSkipSrc;
		dst -= p->leftSkipDst;
		srcI = 0;
		dstI = 0;
		dstPos = 0;
		lsrc = src + p->srcWidth;
		
		
		while ( dstI <= p->newNumPixels + p->leftSkipDst ) {
			if ( dstI >= (p->leftSkipDst-2) ) {
				z = *src++;
				y = 0xff & (z >> 8);
				if ( y == kTransparentY )	
				{
					u = v = kUVOffset;
				}
				else if ( ((((ulong)src) & 2) == 0 ) ) 
				{
					v = z & 0xff;
					if ( src < lsrc ) 
						u = *src & 0xff;
				}
				else 
				{
					u = z & 0xff;
					if ( src < lsrc ) 
						v = *src & 0xff;
				}
			}
			else
				src++;
			srcI++;
			dstPos += dstInc;
			long dn = (dstPos>>kFract);
			weight = ((ushort)256 * (ushort)FracMan(dstPos)) >> kFract;
			while ( dstI < dn  ) 
			{
				if ( dstI >= p->leftSkipDst && dst >= p->dstBase /* && dst < p->dstBase + kMaxScanLineWidth+kExtraSrcLeft */ ) {
					if ( y == kTransparentY ) 
						*dst = (ushort)kTransparentYUYV;
					else 
						*dst = (y<<8) | ((((ulong)dst) & 2) ? v : u);
				}
				dst++;
				dstI++;
			}
			
			if ( dstI >= p->leftSkipDst && dst >= p->dstBase ) {
				if ( y == kTransparentY ) {
					*dst = (ushort)kTransparentYUYV;
				} else {
					ynext = *src>>8;
					if ( ynext != kTransparentY )
						y = BlendPixelsY(y,ynext,weight);
					y = CLIPLUMINANCE(y);
					z = (((ulong)dst) & 2) ? v : u;
					z = CLIPCHROMA(z);
					*dst = (y<<8) | z;
				}
			}
			dst++;
			dstI++;
		}
	}
}




void
ResizeYUYVScanlineCheap(ResizeParams *p);


void
ResizeYUYVScanlineCheap(ResizeParams *p)
{
	
	long 	y,uv;
	register uchar 	*yp,*uvp;
	register long  	srcPos = 0;
	register long srcInc = p->srcInc;
	register ushort	*dst = p->dstBase;
	register ushort  *src = p->srcBase;
	long 	i,j;
	long 	srcNext;
	long  	srcX;
	long 	uvc;
	
	if ( srcInc == kFractOne ) {
		if ( src != dst ) {
			CopyScanLine(src,dst,p->newNumPixels);
		}
		return;
	}
	if ( srcInc > kFractOne  )			// shrink
	{
		ulong fract;
		ulong weightScale = p->srcIncRecip;
		uchar *luvp = (uchar *)(src - p->skipStart + p->srcWidth) - 3;
		
		if ( p->hasTransparent ) 
		{
			i=0;
			while ( 1 )   
			{	
				long tc = 0;
				srcX = FracToInt(srcPos);
				srcNext = srcPos + srcInc;
				
				if ( srcX >= p->skipStart ) 
				{	
					srcX -= p->skipStart;
					yp = (uchar*)(src + srcX);
					uvp = yp+1;
					if ( ((((ulong)dst) & 2) != 0 ) != (srcX & 1) ) {
						uvp += 2;
					}
					fract = USFracMul((kFractOne - FracMan(srcPos)),weightScale);
					if ( *yp == kTransparentY) {
						y = WeightAccLuminance(fract,kBlackY + kRangeY/2);
						uv = (long)kUVOffset << kFract;
						tc = 1;
						}
					else {
						y = WeightAccLuminance(fract,*yp);
						uv = (long)*uvp << kFract;
					}
					uvc = 1;
					for ( j = FracToInt(srcNext) - FracToInt(srcPos); j > 0;  j-- )  {
						yp += 2;
						if ( (j & 1) == 0 ) {
							if ( uvp < (luvp-4) ) {
							uvp += 4;
								uv += (long)*uvp << kFract;
								uvc++;
							}
						}
						if ( j > 1 ) {
							if ( *yp == kTransparentY ) {
								tc++;
								y +=  WeightAccLuminance(weightScale,y);
							} else{ 
								y += WeightAccLuminance(weightScale,*yp);
							}
						} else {
							fract = USFracMul(FracMan(srcNext),weightScale);
							if ( *yp == kTransparentY ) {
								y += WeightAccLuminance(fract,y);
								tc++;
							} else {
								y += WeightAccLuminance(fract,*yp);
							}
						}
					}
					if ( tc > 0 ) {
						*dst++ = (ushort)kTransparentYUYV;
					} else {
						y >>= kFract;
						switch ( uvc ) {
						case 1:	
							uv >>= kFract;
							break;
						case 2:
							uv >>= (kFract+1);
							break;
						case 4:	
							uv >>= (kFract+2);
							break;
						default:
							uv /= (uvc<<kFract);
							break;
						}
						y += kBlackY;
						*dst++ = (CLIPLUMINANCE(y)<<8) | CLIPCHROMA(uv);
					}
					if ( ++i == p->newNumPixels )
						break;
				}
				srcPos = srcNext;
			} 
		}
		else {
			i=0;
			while ( 1 )    {
				srcX = FracToInt(srcPos);
				srcNext = srcPos + srcInc;
				if ( srcX >= p->skipStart )  {	
					srcX -= p->skipStart;
					yp = (uchar*)(src + srcX);
					uvp = yp+1;
					if ( ((((ulong)dst) & 2) != 0 ) != ((srcX & 1) == 1)  ) {
						uvp += 2;
					}
					fract = USFracMul((kFractOne - FracMan(srcPos)),weightScale);
					y = *yp;
					uv = *uvp;
					yp += (FracToInt(srcNext) - FracToInt(srcPos))<<1;
					*dst++ = (CLIPLUMINANCE(y)<<8) | CLIPCHROMA(uv);
					if ( ++i == p->newNumPixels )
						break;
				}
				srcPos = srcNext;
			}
		}
	}
	
	else		// stretch
	{
	
		register ushort z,u,v = 0xffff;
		register long  srcI,dstI;
		ushort *lsrc;
		short	ynext;
		long	dstInc = IntToFrac(p->dstWidth) / p->srcWidth;
		long	dstPos = 0;
		ushort	weight = 0;

		z = *src;
		y = 0xff & (z >> 8);
		if ( y == kTransparentY )	
			u = v = kUVOffset;
		else if ( ((((ulong)src) & 2) == 0 ) ) 
		{
			v = z & 0xff;
			u = src[1] & 0xff;
		}
		else 
		{
			u = z & 0xff;
			v = src[1] & 0xff;
		}
		src -= p->leftSkipSrc;
		dst -= p->leftSkipDst;
		srcI = 0;
		dstI = 0;
		dstPos = 0;
		lsrc = src + p->srcWidth;
		
		
		while ( dstI <= p->newNumPixels + p->leftSkipDst ) {
			if ( dstI >= (p->leftSkipDst-2) ) {
				z = *src++;
				y = 0xff & (z >> 8);
				if ( y == kTransparentY )	
				{
					u = v = kUVOffset;
				}
				else if ( ((((ulong)src) & 2) == 0 ) ) 
				{
					v = z & 0xff;
					if ( src < lsrc ) 
						u = *src & 0xff;
				}
				else 
				{
					u = z & 0xff;
					if ( src < lsrc ) 
						v = *src & 0xff;
				}
			}
			else
				src++;
			srcI++;
			dstPos += dstInc;
			long dn = (dstPos>>kFract);
			weight = ((ushort)256 * (ushort)FracMan(dstPos)) >> kFract;
			while ( dstI < dn  ) 
			{
				if ( dstI >= p->leftSkipDst && dst >= p->dstBase /* && dst < p->dstBase + kMaxScanLineWidth+kExtraSrcLeft */ ) {
					if ( y == kTransparentY ) 
						*dst = (ushort)kTransparentYUYV;
					else 
						*dst = (y<<8) | ((((ulong)dst) & 2) ? v : u);
				}
				dst++;
				dstI++;
			}
			
			if ( dstI >= p->leftSkipDst && dst >= p->dstBase ) {
				if ( y == kTransparentY ) {
					*dst = (ushort)kTransparentYUYV;
				} else {
					ynext = *src>>8;
					if ( ynext != kTransparentY )
						y = BlendPixelsY(y,ynext,weight);
					y = CLIPLUMINANCE(y);
					z = (((ulong)dst) & 2) ? v : u;
					z = CLIPCHROMA(z);
					*dst = (y<<8) | z;
				}
			}
			dst++;
			dstI++;
		}
	}
}




void
ClearYUYVScanLine(ushort *src,long width,Boolean transparent)
{
	ulong *sp = (ulong *)src;
	ulong fillLong = transparent ? kTransparentYUYV : kBlackYUYV;
	
	width += 1;
	width >>= 1;
	while ( width > 4 ) {
		*sp++ = fillLong;
		*sp++ = fillLong;
		*sp++ = fillLong;
		*sp++ = fillLong;
		width -= 4;
		}
		while ( width-- )
		*sp++ = fillLong;
}




#define	AddWeightedLuminance(w,p,a)		{a = ((((ushort)(w)) * (ushort)((p)-kBlackY))>>(kFract/2)) + a;}		// 16x16 multiply
#define	AddWeightedChrominance(w,p,a)	{a = ((((ushort)(w)) * (short)((p)-kUVOffset))>>(kFract/2)) + a;}		// 16x16 multiply


void
AddYUYVScanLine(ushort *src,ushort *dst,long width,Boolean srcHasTransparentPixels,long dweight)
{
	register ushort	s,d,y,uv;
	ushort weight = dweight >> (kFract/2);

	if ( srcHasTransparentPixels )
		while ( width-- )
		{
			// should weight the transparency, but this just forces transparency if any
			// of the component scanlines have any transparent pixels
		
			d = *dst;
			s = *src++;
			if ( ((d>>8) == kTransparentY) || ((s>>8) == kTransparentY) ) 
			{
				*dst++ = (ushort)kTransparentYUYV;
			} 
			else 
			{
				y = (d>>8);
				AddWeightedLuminance(weight,(s>>8),y);
				s &= 0xff;
				uv = d & 0xff;
				AddWeightedChrominance(weight,s,uv);
				*dst++ = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
			}
		}
	else {
		if ( ((long)src & 2) == 0 && ((long)dst & 2) == 0 ) {
			ulong sl,dl,al;

			while ( width > 1 ) {
				dl = *(ulong *)dst;
				y = dl>>24;
				sl = *(ulong *)src;
				AddWeightedLuminance(weight,(sl>>24),y);
				uv = (dl>>16) & 0xff;
				AddWeightedChrominance(weight,((sl>>16) & 0xff),uv);
				al = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
				al <<= 16;
				y = (dl>>8) & 0xff;
				AddWeightedLuminance(weight,(0xff & (sl>>8)),y);
				uv = dl & 0xff;
				AddWeightedChrominance(weight,(sl & 0xff),uv);
				al |= (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
				*(ulong *)dst = al;
				src += 2;
				dst += 2;
				width -= 2;
			}
		}
		while ( width-- ) {
			d = *dst;
			s = *src++;
			y = d>>8;
			AddWeightedLuminance(weight,(s>>8),y);
			s &= 0xff;
			uv = d & 0xff;
			AddWeightedChrominance(weight,s,uv);
			*dst++ = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
		}
	}
}



void
BlendYUYVScanLine(ushort *src,ushort *dst,long width,Boolean srcHasTransparentPixels,long weight)
{
	register ushort	s,d,y,uv;

	if ( srcHasTransparentPixels )
		while ( width-- )
		{
			// should weight the transparency, but this just forces transparency if any
			// of the component scanlines have any transparent pixels
		
			s = *src++;
			d = *dst;
			if ( ((d>>8) == kTransparentY) || ((s>>8) == kTransparentY) ) 
			{
				*dst++ = (ushort)kTransparentYUYV;
			} 
			else 
			{
				y = (d>>8);
				y = BlendPixelsY((s>>8),y,weight);
				s &= 0xff;
				uv = d & 0xff;
				uv = BlendPixels(s,uv,weight);
				*dst++ = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
			}
		}
	else {
		if ( ((long)src & 2) == 0 && ((long)dst & 2) == 0 ) {
			ulong sl,dl,al;

			while ( width > 1 ) {
				dl = *(ulong *)dst;
				sl = *(ulong *)src;
				y = BlendPixelsY((sl>>24),(dl>>24),weight);
				uv = BlendPixels((sl>>16) & 0xff,(dl>>16) & 0xff,weight);
				al = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
				al <<= 16;
				y = (dl>>8) & 0xff;
				y = BlendPixelsY(0xff & (sl>>8),y,weight);
				uv = BlendPixels(sl & 0xff,dl & 0xff,weight);
				al |= (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
				*(ulong *)dst = al;
				src += 2;
				dst += 2;
				width -= 2;
			}
		}
		while ( width-- )
		{
			d = *dst;
			s = *src++;
			y = d>>8;
			y = BlendPixelsY((s>>8),y,weight);
			s &= 0xff;
			uv = d & 0xff;
			uv = BlendPixels(s,uv,weight);
			*dst++ = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(uv);
		}
	}
}



void
ResizeBlit(BlitParams2& p,ulong transparency)
{

	long 	dstHeight = p.dst.r.bottom - p.dst.r.top;
	long 	srcHeight = p.src.r.bottom - p.src.r.top;
	long 	dstWidth =  p.dst.r.right - p.dst.r.left;
	long	srcWidth = p.src.r.right - p.src.r.left;
	long	srcTop = p.src.r.top;
	long	srcBottom = p.src.r.bottom;
	long	srcLeft = p.src.r.left;
	long	srcBufLeftOffset = 0;
	long	resizeWidth = dstWidth;
	long	i,j;
	ResizeParams	rp;
	BlitParams2		srcParams = p;
	BlitParams2		dstParams = p;
	BlitProcPtr		srcBlitProc = nil;
	BlitProcPtr		dstBlitProc = nil;
	ushort	*bufA,*bufB;
	bufA = gResizeBuffer + kExtraSrcLeft;
	bufB = gResizeBuffer + kExtraSrcLeft + kMaxScanLineWidth;
	long	dstRowBytes,srcRowBytes;
	long	dstBump;
	long	srcBump;		// could be negative if flipped vertically	
	long 	srcPos,srcInc;
	uchar	*srcBase, *sb;
	uchar	*db;
	Boolean	srcHasTransparentPixels = (srcParams.src.device->transparentColor & kTransparent) != 0;
	Boolean needsClear;
	BitMapDevice bufferDevice;
	long	horizScale = IntToFrac(p.srcWidth) / p.dstWidth;
	long	horizScaleRecip = IntToFrac(p.dstWidth) / p.srcWidth;
	Boolean	reAlignUV = false;
	ushort 	*tmpResizeBuffer = nil;
	long	maxScanLineWidth = kMaxScanLineWidth;
	Boolean	doCheap = p.cheapFast;
	FilterType saveDstFilter = p.dst.device->filter;
		
	PostulateFinal(false);		// test this 
#if	1	
	if ( IsError(p.src.device->format == vqFormat) ) 
	{
		ImportantMessage(("Can't scale VQ format bitmaps yet"));		// need to figure out two-line rowbytes
		PostulateFinal(false);	// need to deal with this case
		goto done;
	}
#endif
	
	bufferDevice.format = kResizeBufferFormat;
	bufferDevice.filter = kNoFilter;
	srcBlitProc = LookupBlitProc(p.src.device,&bufferDevice,kNotTransparent);

	// can't do filterblit on resize, since we only have one scanline of source
	// еее if we want this we need a special blit loop
	
	p.dst.device->filter = kNoFilter;
	dstBlitProc = LookupBlitProc(&bufferDevice,p.dst.device,transparency);
	p.dst.device->filter = saveDstFilter;
	
	if ( IsError(srcBlitProc == nil) )
	{
		ImportantMessage(("Can't resize image from format %d",p.src.device->format));
		goto done;
	}
	if ( IsError(dstBlitProc == nil) )
	{
		ImportantMessage(("Can't resize image to format %d",p.dst.device->format));
		goto done;
	}
	
	if ( (p.dst.device->format == yuv422Format  
#ifdef SPOT1
		|| p.dst.device->format == yuv422BucherFormat
#endif
		)   &&
		 (p.dst.r.left & 1) == 1  ) 
		reAlignUV  = true;
	
	
	if ( horizScale > kFractOne ) {	
		long scaleF = FracToInt(horizScale) + 1;
		if (  srcWidth < (p.srcWidth+scaleF) ) {	
			resizeWidth = SCALE_ORD(srcWidth,p.dstWidth,p.srcWidth);
			srcWidth += scaleF;
			resizeWidth += scaleF;
		}
	}
	else if ( horizScale < kFractOne )
	{
		if (  srcWidth < (p.srcWidth+2) ) {	
			srcWidth += 2;
			resizeWidth = SCALE_ORD(srcWidth,p.dstWidth,p.srcWidth);
		}
	} 


	if (  resizeWidth > kMaxScanLineWidth ||  dstWidth > kMaxScanLineWidth || srcWidth >  kMaxScanLineWidth ) {
		maxScanLineWidth = MAX(dstWidth,srcWidth);
		maxScanLineWidth = MAX(maxScanLineWidth,resizeWidth);
		tmpResizeBuffer = (ushort *)AllocateTaggedMemoryNilAllowed(( maxScanLineWidth + kExtraSrcLeft+kMaxScaleFactor) * 2 * 2,"resize buf");
		if ( IsError(tmpResizeBuffer == nil) ) {
			ImportantMessage(("scanline too long for resize"));
			return;
		}
		bufA = tmpResizeBuffer + kExtraSrcLeft;
		bufB = tmpResizeBuffer + kExtraSrcLeft + maxScanLineWidth;
	}
	
	bufferDevice.baseAddress = (uchar*)(bufA-kExtraSrcLeft);
	bufferDevice.rowBytes = maxScanLineWidth*kResizeBufferPixelSize;
	bufferDevice.depth = kResizeBufferFormat & 0xff;
	bufferDevice.depthShift = kResizeBufferDepthShift;
	SetRectangle(bufferDevice.bounds,0,0,maxScanLineWidth,1);
	bufferDevice.transparentColor = 0;
	bufferDevice.colorTable = nil;


	srcParams.src.r.left -= kExtraSrcLeft;
	srcBufLeftOffset = -kExtraSrcLeft;
	srcWidth += kExtraSrcLeft;


	srcParams.src.r.right = srcParams.src.r.left + srcWidth;
	srcParams.src.base -= srcParams.src.leftBits >> 5;
	srcParams.src.leftBits =  srcParams.src.r.left << p.src.device->depthShift;
	srcParams.src.base += srcParams.src.leftBits >> 5;
	srcParams.src.xInc = 1;
	
	srcParams.dst.color = 0;
	srcParams.dst.bump = bufferDevice.rowBytes;
	srcParams.dst.device = &bufferDevice;
	srcParams.dst.r.top = 0;
	srcParams.dst.r.bottom = 1;
	srcParams.dst.r.left = 0;
	srcParams.dst.r.right =   srcParams.dst.r.left + srcWidth;
	srcParams.dst.leftBits =  0;
	srcParams.dst.rightBits = srcParams.dst.r.right<<kResizeBufferDepthShift;
	srcParams.dst.longCount = (srcParams.dst.rightBits-srcParams.dst.leftBits)>>5;
	srcParams.dst.leftMask = 0xffffffff << (srcParams.dst.leftBits & 31);
	srcParams.dst.rightMask = 0xffffffff;
	srcParams.dst.base = ((ulong *)(bufA + srcBufLeftOffset)) + (srcParams.dst.leftBits>>5);
	srcParams.dst.xInc = 1;
	
	{	
		ulong temp = (32 - (srcParams.dst.rightBits & 31));
		if (temp == 32)								// handle flush right edge case
			srcParams.dst.longCount--;
		else
			srcParams.dst.rightMask <<= temp;
	}
	
	srcBump = srcParams.src.bump;
	srcRowBytes = p.src.device->rowBytes;
	srcParams.src.r.top = 0;	
	srcParams.src.r.bottom = 1;
	srcBase = (uchar*)srcParams.src.base;
	
	bufferDevice.transparentColor = p.src.device->transparentColor;
	
	dstParams.src.device = &bufferDevice;
	dstParams.src.color = 0;
	dstParams.src.bump = bufferDevice.rowBytes;
	dstParams.src.r.top = 0;
	dstParams.src.r.bottom = 1;
	dstParams.src.r.left = reAlignUV ? 1 : 0;
	dstParams.src.r.right = dstWidth;
	dstParams.src.leftBits = dstParams.src.r.left << kResizeBufferDepthShift ;
	dstParams.src.rightBits = dstParams.src.r.right << kResizeBufferDepthShift;
	dstParams.src.base = ((ulong *)(bufB)) + (dstParams.src.leftBits>>5);
	dstParams.src.xInc = p.src.xInc;
	
	
	dstParams.dst.r.top = 0;
	dstParams.dst.r.bottom = 1;
	dstParams.dst.xInc = 1;
	
	needsClear = srcHasTransparentPixels || 
		(srcParams.src.device->format == antialias8Format) ||
		(srcParams.src.device->format == antialias4Format)
		;
		

	sb = srcBase;
	db = (uchar *)dstParams.dst.base;
	dstBump = dstParams.dst.bump;
	dstRowBytes = p.dst.device->rowBytes;



	rp.srcBase = bufA;
	rp.dstBase = bufB + ( reAlignUV ? 1 : 0) ;
	rp.leftSkipSrc = p.leftSkipSrc;
	rp.leftSkipDst = p.leftSkipDst;
	rp.srcWidth = p.srcWidth;
	rp.dstWidth = p.dstWidth;
	rp.srcInc = horizScale;
	rp.srcIncRecip = horizScaleRecip;
	rp.newNumPixels = resizeWidth; 
	rp.skipStart = srcLeft;
	rp.hasTransparent = srcHasTransparentPixels;

	
	if ( p.srcHeight == p.dstHeight ) 
	{	
	
		// no vertical scaling or no scaling just format conversion
		
		for ( i=dstHeight; i-- > 0; ) 
		{
			if ( needsClear )
				ClearYUYVScanLine(bufA-kExtraSrcLeft,srcWidth+kExtraSrcLeft,srcHasTransparentPixels);
			srcParams.src.base = (ulong *)sb;
			srcBlitProc(srcParams,kNotTransparent);
			if ( doCheap )
				ResizeYUYVScanlineCheap(&rp);
			else
				ResizeYUYVScanline(&rp);
			dstParams.dst.base = (ulong *)db;
			dstBlitProc(dstParams,transparency);
			sb += srcBump;
			db += dstBump;
		}
	} 
	
	else if ( p.srcHeight > p.dstHeight )
	{
	
		
	
		// shrunk vertically, need to average source scan lines

		long	srcNext;
		uchar *srcBaseAddr = (uchar*)srcParams.src.base;
		uchar *srcEnd = srcBase + srcHeight * srcRowBytes;
		Boolean inDst = false;
		
		long	weightScale = IntToFrac(p.dstHeight) / p.srcHeight;
		
		srcInc = IntToFrac(p.srcHeight) / p.dstHeight;
		if ( srcBump < 0 ) {		// reduced accuracy, but works for vertical flip
			srcPos = IntToFrac(srcBottom-1);
			srcBase = (uchar*)((ulong *)srcParams.src.device->baseAddress + (srcParams.src.leftBits >> 5)); 
			srcBaseAddr = srcBase + srcTop * srcRowBytes;
			srcEnd = srcBaseAddr + srcHeight * srcRowBytes;
			srcInc = -srcInc;
		} else {
			srcPos = 0;
			srcBase -= 	srcTop * srcRowBytes;
		}
		sb = srcBase + FracToInt(srcPos) * srcRowBytes;
		
		if ( doCheap ) {
			srcParams.dst.base = ((ulong *)(bufA +  srcBufLeftOffset)) + (srcParams.dst.leftBits>>5);
			while ( dstHeight > 0 ) {	
				Boolean gotFirstScanline = false;
				
				if ( srcInc < 0 ) 
				{
					if ( (FracToInt(srcPos + kRound) ) < srcTop )
						break;
				} 
				else 
				{
					if ( (FracToInt(srcPos + kRound) ) >= srcBottom )
						break;
				}
				srcNext = srcPos + srcInc;
				
				if ( srcInc < 0 ) 
				{
					if ( srcPos < IntToFrac(srcTop)  )
						srcPos = IntToFrac(srcTop-1);
					j = FracToInt(srcPos) - FracToInt(srcNext);
					if ( (FracToInt(srcPos + kRound)) <= srcBottom )
						inDst = true;
				}
				else
				{
					if ( srcPos > IntToFrac(srcBottom)  )
						srcPos = FracToInt(srcBottom);
					j = FracToInt(srcNext) - FracToInt(srcPos);
					if ( (FracToInt(srcPos + kRound)) >= srcTop )
						inDst = true;
				}
	
				// copy subsequent scanlines into buffer B, then average into buffer A
				
				for  ( ; j >= 0 ; j-- ) 
				{
					if ( sb >= srcBaseAddr && sb < srcEnd ) 
					{
						
						if ( !gotFirstScanline ) {
							srcParams.src.base = (ulong *)sb;
							srcBlitProc(srcParams,kNotTransparent);
							gotFirstScanline = true;
						}
					}
					if ( j )
						sb += srcBump;
				}
				if (  inDst  ) {
					
					// resize the accumulated scanline in buffer A into buffer B
				
					if ( doCheap )
						ResizeYUYVScanlineCheap(&rp);
					else
						ResizeYUYVScanline(&rp);
					
					// copy the averaged resized scanline from buffer B to dest
					
					dstParams.dst.base = (ulong *)db;
					dstBlitProc(dstParams,transparency);
					db += dstBump;
					--dstHeight;
				}
				srcPos = srcNext;
			}
		} else {
			srcParams.dst.base = ((ulong *)(bufB +  srcBufLeftOffset)) + (srcParams.dst.leftBits>>5);
			while ( dstHeight > 0 ) {	
				Boolean gotFirstScanline = false;
				long	totalWeight = 0;
				long	weight;
				
				if ( srcInc < 0 ) 
				{
					if ( (FracToInt(srcPos + kRound) ) < srcTop )
						break;
				} 
				else 
				{
					if ( (FracToInt(srcPos + kRound) ) >= srcBottom )
						break;
				}
				srcNext = srcPos + srcInc;
				
				if ( srcInc < 0 ) 
				{
					if ( srcPos < IntToFrac(srcTop)  )
						srcPos = IntToFrac(srcTop-1);
					j = FracToInt(srcPos) - FracToInt(srcNext);
					if ( (FracToInt(srcPos + kRound)) <= srcBottom )
						inDst = true;
				}
				else
				{
					if ( srcPos > IntToFrac(srcBottom)  )
						srcPos = FracToInt(srcBottom);
					j = FracToInt(srcNext) - FracToInt(srcPos);
					if ( (FracToInt(srcPos + kRound)) >= srcTop )
						inDst = true;
				}
	
				// copy subsequent scanlines into buffer B, then average into buffer A
				
				for  ( ; j >= 0 ; j-- ) 
				{
					if ( sb >= srcBaseAddr && sb < srcEnd ) 
					{
						srcParams.src.base = (ulong *)sb;
						
						if ( gotFirstScanline && !doCheap ) {
							srcBlitProc(srcParams,kNotTransparent);
							if ( j == 0 ) 
								weight = FracMul(weightScale,FracMan(srcNext));
							else
								weight = weightScale;
							totalWeight += weight;
							AddYUYVScanLine(bufB,bufA,srcWidth,srcHasTransparentPixels,weight);
						}
						else {
							if ( needsClear )
								ClearYUYVScanLine(bufB,srcWidth,srcHasTransparentPixels);
							totalWeight = FracMul(weightScale,kFractOne - FracMan(srcPos));
							ClearYUYVScanLine(bufA,srcWidth,false);
							srcBlitProc(srcParams,kNotTransparent);
							AddYUYVScanLine(bufB,bufA,srcWidth,srcHasTransparentPixels,totalWeight);
							gotFirstScanline = true;
						}
					}
					if ( j )
						sb += srcBump;
				}
				if (  inDst  ) {
					if ( ( totalWeight < kFractOne ) && (p.srcHeight > 1) ) 
						AddYUYVScanLine(bufB,bufA,srcWidth,srcHasTransparentPixels,kFractOne-totalWeight);
					
					// resize the accumulated scanline in buffer A into buffer B
				
					ResizeYUYVScanline(&rp);
					
					// copy the averaged resized scanline from buffer B to dest
					
					dstParams.dst.base = (ulong *)db;
					dstBlitProc(dstParams,transparency);
					db += dstBump;
					--dstHeight;
				}
				srcPos = srcNext;
			}
	
		}
	}
	else if ( p.srcHeight < p.dstHeight  )
	{
		// stretched vertically, just replicate source scan lines

		srcInc = IntToFrac(p.srcHeight) / p.dstHeight;
		srcPos = 0;
		
		if ( srcBump < 0 ) {		// reduced accuracy, but works for vertical flip
			p.topSkipSrc = 0;
			p.topSkipDst = 0;
		}
		sb = srcBase;
		sb -= p.topSkipSrc * srcRowBytes;
		db -= p.topSkipDst * dstRowBytes;
		i = 0;
		j = 0;
		if ( needsClear ) {
			ClearYUYVScanLine(bufA,srcWidth,srcHasTransparentPixels);
			ClearYUYVScanLine(bufB,srcWidth,srcHasTransparentPixels);
		}
		long	dstInc = IntToFrac(p.dstHeight) / p.srcHeight;
		long 	dstPos = 0;
		long 	dstY,dstNext;
		long 	dstLast = dstHeight + p.topSkipDst;
		
		dstY = FracToInt(dstPos);
		while ( dstY < dstLast ) {
			dstPos += dstInc;
			dstNext = FracToInt(dstPos);
			if ( dstNext >= p.topSkipDst  && i < p.srcHeight ) {
				srcParams.src.base = (ulong *)sb;
				srcParams.dst.base = ((ulong *)(bufA + srcBufLeftOffset)) + (srcParams.dst.leftBits>>5);
				srcBlitProc(srcParams,kNotTransparent);
				rp.srcBase = bufA;
				rp.dstBase = bufB + ( reAlignUV ? 1 : 0) ;
				if ( doCheap )
					ResizeYUYVScanlineCheap(&rp);
				else
					ResizeYUYVScanline(&rp);
				
			}
			++i;
			sb += srcBump;
			if ( dstNext > dstLast )
				dstNext = dstLast;
			for ( ; dstY < dstNext; ) {
				if ( dstY >= p.topSkipDst ) {
					dstParams.src.base = ((ulong *)(bufB)) + (dstParams.src.leftBits>>5);
					dstParams.dst.base = (ulong *)(db);
					dstBlitProc(dstParams,transparency);
				}
				db += dstRowBytes;
				dstY++;
			}
			long weight = FracMan(dstPos) ;
			if ( weight ) {
				if ( dstY >= p.topSkipDst && dstY < dstLast ) {
					srcParams.src.base = (ulong *)sb;
					srcParams.dst.base = ((ulong *)(bufB + srcBufLeftOffset)) + (srcParams.dst.leftBits>>5);
					srcBlitProc(srcParams,kNotTransparent);
					BlendYUYVScanLine(bufA,bufB,srcWidth,srcHasTransparentPixels,(weight* 256) >> kFract);
					rp.srcBase = bufB;
					rp.dstBase = bufA + ( reAlignUV ? 1 : 0) ;
					if ( doCheap )
						ResizeYUYVScanlineCheap(&rp);
					else
						ResizeYUYVScanline(&rp);
					dstParams.src.base = ((ulong *)(bufA)) + (dstParams.src.leftBits>>5);
					dstParams.dst.base = (ulong *)(db);
					dstBlitProc(dstParams,transparency);
				}
				db += dstRowBytes;
				dstY++;
			}
		}
	}
done:
	if ( tmpResizeBuffer )
		FreeTaggedMemory(tmpResizeBuffer,"resize buf");

}



void
NewInterlaceFilterYUYV(BlitParams& p, FilterType filterType)
{
	register uchar*		dst = (uchar*)p.base;
	long				pixelCount = (p.rightBits >> 4) - (p.leftBits >> 4) ;
	long				height = p.r.bottom - p.r.top;
	long				rowBytes;
	Boolean				noTop = false,noBottom = false;
	Boolean				doSlight = false;
	
	if ( filterType == 0 )
		return;
	if (  gOptionsPanel && gOptionsPanel->IsVisible()  ) {
		Rectangle optBounds;
		gOptionsPanel->GetBounds(&optBounds);
		if ( p.r.bottom >= optBounds.top ) {
			p.r.bottom = optBounds.top;
			noBottom = true;
		}
	}
	height = p.r.bottom - p.r.top;
	
	dst += (p.leftBits >> 3) & 3;	// advance to ragged byte boundary
	rowBytes  = p.bump;


	switch ( filterType )
	{
	case kTopFilterSlight:
		doSlight = true;
	case kTopFilter:
		noBottom = true;
		break;
	case kBottomFilterSlight:
		doSlight = true;
	case kBottomFilter:
		noTop = true;
		break;
	case kSliceFilterSlight:
		doSlight = true;
	case kSliceFilter:
		noTop = true;
		noBottom = true;
		break;
	case kNoFilter:
		return;
	case kFullFilterSlight:
		doSlight = true;
		break;
	default:
		Message(("unknown filter type"));
		break;
	}

	long	x,y;
//	uchar *dend = dst + rowBytes * height;
	
	if ( !doSlight ) {
		for (x=0; x < pixelCount; ) {
#if	0		
		// this would be cool if it got all the registers, but it doesn't
		
			if ( (((long)dst & 0x1f) == 0 )  && (x < (pixelCount-16)) ) {
				register ulong a0,a1,a2,a3,a4,a5,a6,a7;
				register ulong b0,b1,b2,b3,b4,b5,b6,b7;
				register ulong c0,c1,c2,c3,c4,c5,c6,c7;
				long  rowWords = rowBytes>>2;
				ulong *lp,*nlp;
		
				y=0;
				lp = (ulong *)dst;
				if ( !noTop ) {
					nlp = lp - rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					a4 = nlp[4]; a5 = nlp[5]; a6 = nlp[6]; a7 = nlp[7];
				} else {
					a0 = lp[0]; a1 = lp[1]; a2 = lp[2]; a3 = lp[3];
					a4 = lp[4]; a5 = lp[5]; a6 = lp[6]; a7 = lp[7];
				}
				b0 = lp[0]; b1 = lp[1]; b2 = lp[2]; b3 = lp[3];
				b4 = lp[4]; b5 = lp[5]; b6 = lp[6]; b7 = lp[7];
				while ( y < height-3) {
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					c4 = nlp[4]; c5 = nlp[5]; c6 = nlp[6]; c7 = nlp[7];
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
					lp[4] = FilterLongs(a4,b4,c4);
					lp[5] = FilterLongs(a5,b5,c5);
					lp[6] = FilterLongs(a6,b6,c6);
					lp[7] = FilterLongs(a7,b7,c7);
					lp += rowWords;
					nlp = lp+rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					a4 = nlp[4]; a5 = nlp[5]; a6 = nlp[6]; a7 = nlp[7];
					lp[0] = FilterLongs(b0,c0,a0);
					lp[1] = FilterLongs(b1,c1,a1);
					lp[2] = FilterLongs(b2,c2,a2);
					lp[3] = FilterLongs(b3,c3,a3);
					lp[4] = FilterLongs(b4,c4,a4);
					lp[5] = FilterLongs(b5,c5,a5);
					lp[6] = FilterLongs(b6,c6,a6);
					lp[7] = FilterLongs(b7,c7,a7);
					lp += rowWords;
					nlp = lp+rowWords;
					b0 = nlp[0]; b1 = nlp[1]; b2 = nlp[2]; b3 = nlp[3];
					b4 = nlp[4]; b5 = nlp[5]; b6 = nlp[6]; b7 = nlp[7];
					lp[0] = FilterLongs(c0,a0,b0);
					lp[1] = FilterLongs(c1,a1,b1);
					lp[2] = FilterLongs(c2,a2,b2);
					lp[3] = FilterLongs(c3,a3,b3);
					lp[4] = FilterLongs(c4,a4,b4);
					lp[5] = FilterLongs(c5,a5,b5);
					lp[6] = FilterLongs(c6,a6,b6);
					lp[7] = FilterLongs(c7,a7,b7);
					lp += rowWords;
				 	y += 3;
				}
				switch(height-y) {
				case 3:
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					c4 = nlp[4]; c5 = nlp[5]; c6 = nlp[6]; c7 = nlp[7];
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
					lp[4] = FilterLongs(a4,b4,c4);
					lp[5] = FilterLongs(a5,b5,c5);
					lp[6] = FilterLongs(a6,b6,c6);
					lp[7] = FilterLongs(a7,b7,c7);
					lp += rowWords;
					nlp = lp+rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					a4 = nlp[4]; a5 = nlp[5]; a6 = nlp[6]; a7 = nlp[7];
					lp[0] = FilterLongs(b0,c0,a0);
					lp[1] = FilterLongs(b1,c1,a1);
					lp[2] = FilterLongs(b2,c2,a2);
					lp[3] = FilterLongs(b3,c3,a3);
					lp[4] = FilterLongs(b4,c4,a4);
					lp[5] = FilterLongs(b5,c5,a5);
					lp[6] = FilterLongs(b6,c6,a6);
					lp[7] = FilterLongs(b7,c7,a7);
					lp += rowWords;
					nlp = lp+rowWords;
					if ( noBottom ) {
						b0 = a0; b1 = a1; b2 = a2; b3 = a3;
						b4 = a4; b5 = a5; b6 = a6; b7 = a7;
					}
					else {
						b0 = nlp[0]; b1 = nlp[1]; b2 = nlp[2]; b3 = nlp[3];
						b4 = nlp[4]; b5 = nlp[5]; b6 = nlp[6]; b7 = nlp[7];
					}
					lp[0] = FilterLongs(c0,a0,b0);
					lp[1] = FilterLongs(c1,a1,b1);
					lp[2] = FilterLongs(c2,a2,b2);
					lp[3] = FilterLongs(c3,a3,b3);
					break;
				case 2:
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					c4 = nlp[4]; c5 = nlp[5]; c6 = nlp[6]; c7 = nlp[7];
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
					lp[4] = FilterLongs(c4,a4,b4);
					lp[5] = FilterLongs(c5,a5,b5);
					lp[6] = FilterLongs(c6,a6,b6);
					lp[7] = FilterLongs(c7,a7,b7);
					lp += rowWords;
					nlp = lp+rowWords;
					if ( noBottom ) {
						a0 = b0; a1 = b1; a2 = b2; a3 = b3;
						a4 = b4; a5 = b5; a6 = b6; a7 = b7;
					}
					else {
						a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
						a4 = nlp[4]; a5 = nlp[5]; a6 = nlp[6]; a7 = nlp[7];
					}
					lp[0] = FilterLongs(b0,c0,a0);
					lp[1] = FilterLongs(b1,c1,a1);
					lp[2] = FilterLongs(b2,c2,a2);
					lp[3] = FilterLongs(b3,c3,a3);
					lp[4] = FilterLongs(b4,c4,a4);
					lp[5] = FilterLongs(b5,c5,a5);
					lp[6] = FilterLongs(b6,c6,a6);
					lp[7] = FilterLongs(b7,c7,a7);
					break;
				case 1:
					nlp = lp+rowWords;
					if ( noBottom ) {
						c0 = a0; c1 = a1; c2 = a2; c3 = a3;
						c4 = a4; c5 = a5; c6 = a6; c7 = a7;
					}
					else {
						c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
						c4 = nlp[4]; c5 = nlp[5]; c6 = nlp[6]; c7 = nlp[7];
					}
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
					lp[4] = FilterLongs(a4,b4,c4);
					lp[5] = FilterLongs(a5,b5,c5);
					lp[6] = FilterLongs(a6,b6,c6);
					lp[7] = FilterLongs(a7,b7,c7);
				}
				dst += 32;
				x += 16;
			}
			else 
#endif			
			if ( (((long)dst & 0xf) == 0 )  && (x < (pixelCount-8)) ) {
				register ulong a0,a1,a2,a3;
				register ulong b0,b1,b2,b3;
				register ulong c0,c1,c2,c3;
				long  rowWords = rowBytes>>2;
				register ulong *lp,*nlp;
		
				y=0;
				lp = (ulong *)dst;
				if ( !noTop ) {
					nlp = lp - rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
				} else {
					a0 = lp[0]; a1 = lp[1]; a2 = lp[2]; a3 = lp[3];
				}
				b0 = lp[0]; b1 = lp[1]; b2 = lp[2]; b3 = lp[3];
				while ( y < height-3) {
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
					lp += rowWords;
					nlp = lp+rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					lp[0] = FilterLongs(b0,c0,a0);
					lp[1] = FilterLongs(b1,c1,a1);
					lp[2] = FilterLongs(b2,c2,a2);
					lp[3] = FilterLongs(b3,c3,a3);
					lp += rowWords;
					nlp = lp+rowWords;
					b0 = nlp[0]; b1 = nlp[1]; b2 = nlp[2]; b3 = nlp[3];
					lp[0] = FilterLongs(c0,a0,b0);
					lp[1] = FilterLongs(c1,a1,b1);
					lp[2] = FilterLongs(c2,a2,b2);
					lp[3] = FilterLongs(c3,a3,b3);
					lp += rowWords;
				 	y += 3;
				}
				switch(height-y) {
				case 3:
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
					lp += rowWords;
					nlp = lp+rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					lp[0] = FilterLongs(b0,c0,a0);
					lp[1] = FilterLongs(b1,c1,a1);
					lp[2] = FilterLongs(b2,c2,a2);
					lp[3] = FilterLongs(b3,c3,a3);
					lp += rowWords;
					nlp = lp+rowWords;
					if ( noBottom ) {
						b0 = a0; b1 = a1; b2 = a2; b3 = a3;
					}
					else {
						b0 = nlp[0]; b1 = nlp[1]; b2 = nlp[2]; b3 = nlp[3];
					}
					lp[0] = FilterLongs(c0,a0,b0);
					lp[1] = FilterLongs(c1,a1,b1);
					lp[2] = FilterLongs(c2,a2,b2);
					lp[3] = FilterLongs(c3,a3,b3);
					break;
				case 2:
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
					lp += rowWords;
					nlp = lp+rowWords;
					if ( noBottom ) {
						a0 = b0; a1 = b1; a2 = b2; a3 = b3;
					}
					else {
						a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					}
					lp[0] = FilterLongs(b0,c0,a0);
					lp[1] = FilterLongs(b1,c1,a1);
					lp[2] = FilterLongs(b2,c2,a2);
					lp[3] = FilterLongs(b3,c3,a3);
					break;
				case 1:
					nlp = lp+rowWords;
					if ( noBottom ) {
						c0 = a0; c1 = a1; c2 = a2; c3 = a3;
					}
					else {
						c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					}
					lp[0] = FilterLongs(a0,b0,c0);
					lp[1] = FilterLongs(a1,b1,c1);
					lp[2] = FilterLongs(a2,b2,c2);
					lp[3] = FilterLongs(a3,b3,c3);
				}
				dst += 16;
				x += 8;
			}
			else 
			{
				uchar a,b,c;
				uchar	*cp;
				
				y=0;
				cp = dst;
				if ( !noTop ) {
					a = cp[-rowBytes];
				} else
					a = *cp;
				b = *cp;
				while ( y < height-3) {
					c = *(cp + rowBytes);
					*cp = FilterBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = FilterBytes(b,c,a);
					cp += rowBytes;
					b = *(cp + rowBytes);
					*cp = FilterBytes(c,a,b);
					cp += rowBytes;
				 	y += 3;
				}
				switch(height-y) {
				case 3:
					c = *(cp + rowBytes);
					*cp = FilterBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = FilterBytes(b,c,a);
					cp += rowBytes;
					b = noBottom ? c : *(cp + rowBytes);
					*cp = FilterBytes(c,a,b);
					break;
				case 2:
					c = *(cp + rowBytes);
					*cp = FilterBytes(a,b,c);
					cp += rowBytes;
					a = noBottom ? b : *(cp + rowBytes);
					*cp = FilterBytes(b,c,a);
					break;
				case 1:
					c = noBottom ? a : *(cp + rowBytes);
					*cp = FilterBytes(a,b,c);
				}
				dst++;
#ifndef	DONT_FILTER_CHROMA
				y=0;
				cp = dst;
				if ( !noTop ) {
					a = cp[-rowBytes];
				} else
					a = *cp;
				b = *cp;
				while ( y < height-3) {
					c = *(cp + rowBytes);
					*cp = FilterChromaBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = FilterChromaBytes(b,c,a);
					cp += rowBytes;
					b = *(cp + rowBytes);
					*cp = FilterChromaBytes(c,a,b);
					cp += rowBytes;
				 	y += 3;
				}
				switch(height-y) {
				case 3:
					c = *(cp + rowBytes);
					*cp = FilterChromaBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = FilterChromaBytes(b,c,a);
					cp += rowBytes;
					b = noBottom ? c : *(cp + rowBytes);
					*cp = SFilterChromaBytes(c,a,b);
					break;
				case 2:
					c = *(cp + rowBytes);
					*cp = FilterChromaBytes(a,b,c);
					cp += rowBytes;
					a = noBottom ? b : *(cp + rowBytes);
					*cp = FilterChromaBytes(b,c,a);
					break;
				case 1:
					c = noBottom ? a : *(cp + rowBytes);
					*cp = FilterChromaBytes(a,b,c);
				}
#endif
				dst++;
				x++;
			}
		}
	} else {
		for (x=0; x < pixelCount; ) {
			if ( (((long)dst & 0xf) == 0 )  && (x < (pixelCount-8)) ) {
				register ulong a0,a1,a2,a3;
				register ulong b0,b1,b2,b3;
				register ulong c0,c1,c2,c3;
				long  rowWords = rowBytes>>2;
				register ulong *lp,*nlp;
		
				y=0;
				lp = (ulong *)dst;
				if ( !noTop ) {
					nlp = lp - rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
				} else {
					a0 = lp[0]; a1 = lp[1]; a2 = lp[2]; a3 = lp[3];
				}
				b0 = lp[0]; b1 = lp[1]; b2 = lp[2]; b3 = lp[3];
				while ( y < height-3) {
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					lp[0] = SFilterLongs(a0,b0,c0);
					lp[1] = SFilterLongs(a1,b1,c1);
					lp[2] = SFilterLongs(a2,b2,c2);
					lp[3] = SFilterLongs(a3,b3,c3);
					lp += rowWords;
					nlp = lp+rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					lp[0] = SFilterLongs(b0,c0,a0);
					lp[1] = SFilterLongs(b1,c1,a1);
					lp[2] = SFilterLongs(b2,c2,a2);
					lp[3] = SFilterLongs(b3,c3,a3);
					lp += rowWords;
					nlp = lp+rowWords;
					b0 = nlp[0]; b1 = nlp[1]; b2 = nlp[2]; b3 = nlp[3];
					lp[0] = SFilterLongs(c0,a0,b0);
					lp[1] = SFilterLongs(c1,a1,b1);
					lp[2] = SFilterLongs(c2,a2,b2);
					lp[3] = SFilterLongs(c3,a3,b3);
					lp += rowWords;
				 	y += 3;
				}
				switch(height-y) {
				case 3:
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					lp[0] = SFilterLongs(a0,b0,c0);
					lp[1] = SFilterLongs(a1,b1,c1);
					lp[2] = SFilterLongs(a2,b2,c2);
					lp[3] = SFilterLongs(a3,b3,c3);
					lp += rowWords;
					nlp = lp+rowWords;
					a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					lp[0] = SFilterLongs(b0,c0,a0);
					lp[1] = SFilterLongs(b1,c1,a1);
					lp[2] = SFilterLongs(b2,c2,a2);
					lp[3] = SFilterLongs(b3,c3,a3);
					lp += rowWords;
					nlp = lp+rowWords;
					if ( noBottom ) {
						b0 = a0; b1 = a1; b2 = a2; b3 = a3;
					}
					else {
						b0 = nlp[0]; b1 = nlp[1]; b2 = nlp[2]; b3 = nlp[3];
					}
					lp[0] = SFilterLongs(c0,a0,b0);
					lp[1] = SFilterLongs(c1,a1,b1);
					lp[2] = SFilterLongs(c2,a2,b2);
					lp[3] = SFilterLongs(c3,a3,b3);
					break;
				case 2:
					nlp = lp+rowWords;
					c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					lp[0] = SFilterLongs(a0,b0,c0);
					lp[1] = SFilterLongs(a1,b1,c1);
					lp[2] = SFilterLongs(a2,b2,c2);
					lp[3] = SFilterLongs(a3,b3,c3);
					lp += rowWords;
					nlp = lp+rowWords;
					if ( noBottom ) {
						a0 = b0; a1 = b1; a2 = b2; a3 = b3;
					}
					else {
						a0 = nlp[0]; a1 = nlp[1]; a2 = nlp[2]; a3 = nlp[3];
					}
					lp[0] = SFilterLongs(b0,c0,a0);
					lp[1] = SFilterLongs(b1,c1,a1);
					lp[2] = SFilterLongs(b2,c2,a2);
					lp[3] = SFilterLongs(b3,c3,a3);
					break;
				case 1:
					nlp = lp+rowWords;
					if ( noBottom ) {
						c0 = a0; c1 = a1; c2 = a2; c3 = a3;
					}
					else {
						c0 = nlp[0]; c1 = nlp[1]; c2 = nlp[2]; c3 = nlp[3];
					}
					lp[0] = SFilterLongs(a0,b0,c0);
					lp[1] = SFilterLongs(a1,b1,c1);
					lp[2] = SFilterLongs(a2,b2,c2);
					lp[3] = SFilterLongs(a3,b3,c3);
				}
				dst += 16;
				x += 8;
			}
			else 
			{
				uchar a,b,c;
				uchar	*cp;
				
				y=0;
				cp = dst;
				if ( !noTop ) {
					a = cp[-rowBytes];
				} else
					a = *cp;
				b = *cp;
				while ( y < height-3) {
					c = *(cp + rowBytes);
					*cp = SFilterBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = SFilterBytes(b,c,a);
					cp += rowBytes;
					b = *(cp + rowBytes);
					*cp = SFilterBytes(c,a,b);
					cp += rowBytes;
				 	y += 3;
				}
				switch(height-y) {
				case 3:
					c = *(cp + rowBytes);
					*cp = SFilterBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = SFilterBytes(b,c,a);
					cp += rowBytes;
					b = noBottom ? c : *(cp + rowBytes);
					*cp = SFilterBytes(c,a,b);
					break;
				case 2:
					c = *(cp + rowBytes);
					*cp = SFilterBytes(a,b,c);
					cp += rowBytes;
					a = noBottom ? b : *(cp + rowBytes);
					*cp = SFilterBytes(b,c,a);
					break;
				case 1:
					c = noBottom ? a : *(cp + rowBytes);
					*cp = SFilterBytes(a,b,c);
				}
				dst += 1;
#ifndef	DONT_FILTER_CHROMA
				y=0;
				cp = dst;
				if ( !noTop ) {
					a = cp[-rowBytes];
				} else
					a = *cp;
				b = *cp;
				while ( y < height-3) {
					c = *(cp + rowBytes);
					*cp = SFilterChromaBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = SFilterChromaBytes(b,c,a);
					cp += rowBytes;
					b = *(cp + rowBytes);
					*cp = SFilterChromaBytes(c,a,b);
					cp += rowBytes;
				 	y += 3;
				}
				switch(height-y) {
				case 3:
					c = *(cp + rowBytes);
					*cp = SFilterChromaBytes(a,b,c);
					cp += rowBytes;
					a = *(cp + rowBytes);
					*cp = SFilterChromaBytes(b,c,a);
					cp += rowBytes;
					b = noBottom ? c : *(cp + rowBytes);
					*cp = SFilterChromaBytes(c,a,b);
					break;
				case 2:
					c = *(cp + rowBytes);
					*cp = SFilterChromaBytes(a,b,c);
					cp += rowBytes;
					a = noBottom ? b : *(cp + rowBytes);
					*cp = SFilterChromaBytes(b,c,a);
					break;
				case 1:
					c = noBottom ? a : *(cp + rowBytes);
					*cp = SFilterChromaBytes(a,b,c);
				}
#endif
				dst += 1;
				x++;
			}
		}
	}
}

	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
InterlaceFilterYUYV(BlitParams& p, FilterType filterType)
{
	register uchar*		dst = (uchar*)p.base;
	long				pixelCount = (p.rightBits >> 4) - (p.leftBits >> 4) ;
	long				height = p.r.bottom - p.r.top;
	long				rowBytes;
	Boolean				noTop = false,noBottom = false;
	Boolean				doSlight = false;
	
	if ( filterType == 0 )
		return;
	if (  gOptionsPanel && gOptionsPanel->IsVisible()  ) {
		Rectangle optBounds;
		gOptionsPanel->GetBounds(&optBounds);
		if ( p.r.bottom >= optBounds.top ) {
			p.r.bottom = optBounds.top;
			noBottom = true;
		}
	}
	height = p.r.bottom - p.r.top;
	
	dst += (p.leftBits >> 3) & 3;	// advance to ragged byte boundary
	rowBytes  = p.bump;


	switch ( filterType )
	{
	case kTopFilterSlight:
		doSlight = true;
	case kTopFilter:
		noBottom = true;
		break;
	case kBottomFilterSlight:
		doSlight = true;
	case kBottomFilter:
		noTop = true;
		break;
	case kSliceFilterSlight:
		doSlight = true;
	case kSliceFilter:
		noTop = true;
		noBottom = true;
		break;
	case kNoFilter:
		return;
	case kFullFilterSlight:
		doSlight = true;
		break;
	default:
		Message(("unknown filter type"));
		break;
	}
		
	
	register uchar		*dlast,*dnext;
	long				i;
	register short		t;
	register long		dstBump;
	dstBump = (p.bump - pixelCount * 2);


	if ( (dst + (height+1)*rowBytes) > p.device->baseAddress + p.device->rowBytes * 
			(p.device->bounds.bottom-p.device->bounds.top)  )
		noBottom = true;
	if ( (dst - rowBytes) < p.device->baseAddress)
		noTop = true;
		
	// partial row at top if previous scanline does not exist
	
	if ( noTop ) 
	{
		dnext = dst + rowBytes;
		for (i=pixelCount; i-- > 0; )
		{
			*dst = (short)(*dst * 3 + *dnext) >> 2;
			dst += 1;
			dnext += 1;
			*dst = (short)(*dst * 3 + *dnext) >> 2;
			dst += 1;
			dnext += 1;
		}
		dst += dstBump;
		height -= 1;
	}
	
	if (noBottom) 
		height -= 1;
	if ( height <= 0 )
		return;
		
	dnext = dst + rowBytes;
	dlast = dst - rowBytes;

//#define	READ_BYTES
	
#ifndef	READ_BYTES
	Boolean oddStart = (((ulong)dst) & 2) != 0;
	register ulong pix,pixlast,pixnext;
	register uchar l,n;
	register ulong *ldst,*ldnext,*ldlast;
#endif


	if ( doSlight ) 
		while (height-- > 0)
		{
#ifdef	READ_BYTES
			for (i=pixelCount; i-- > 0;  )
			{
				t = (*dst*6 + *dlast + *dnext)>>3;
				*dst = t;
				dst += 2;
				dlast += 2;
				dnext += 2;
			}
#else
			i = pixelCount;
			if ( oddStart ) 
			{
				*dst = (*dst * 6 + *dlast + *dnext)>>3;
				dst += 2;
				dlast += 2;
				dnext += 2;
				i--;
			}
			ldst = (ulong *)dst;
			ldnext = (ulong *)dnext;
			ldlast = (ulong *)dlast;
			
			dst += (i>>1)<<2;
			dlast += (i>>1)<<2;
			dnext += (i>>1)<<2;
			pix = *ldst;
			for (; i  >= 2;  i -= 2 )
			{
				pixlast = *ldlast++;
				l = pixlast>>24;
				t = pix>>24;
				pix &= 0x00ffffff;
				pixnext = *ldnext++;
				n = pixnext>>24;
				t = (t * 6 + l + n)>>3;
				pix |= ((ulong)t) << 24;

				t = 0xff & (pix>>8);
				n = 0xff & (pixnext>>8);
				l = 0xff & (pixlast>>8);
				pix &= 0xffff00ff;
				t = (t * 6 + l + n)>>3;
				pix |= ((ulong)t) << 8;
				*ldst++ = pix;
				pix = *ldst;
			}
			dst = (uchar *)ldst;
			dnext = (uchar *)ldnext;
			dlast = (uchar *)ldlast;
			if ( i == 1)
			{
				t = (*dst * 6 + *dlast + *dnext)>>3;
				*dst = t;
				dst += 2;
				dlast += 2;
				dnext += 2;
			}
#endif
			dst += dstBump;
			dlast += dstBump;
			dnext += dstBump;
		}		
	else
		while (height-- > 0)
		{
#ifdef	READ_BYTES
			t = *dst;
			for (i=pixelCount; i-- > 0;  )
			{
				t = (short)((*dst<<1) + *dlast + *dnext)>>2;
				*dst  = t;
				dst += 1;
				dlast += 1;
				dnext += 1;
				t = (short)((*dst<<1) + *dlast + *dnext)>>2;
				*dst  = t;
				dst += 1;
				dlast += 1;
				dnext += 1;
			}
#else
			i = pixelCount;
			if ( oddStart ) 
			{
				t = (short)((*dst<<1) + *dlast + *dnext)>>2;
				*dst  = t;
				dst += 2;
				dlast += 2;
				dnext += 2;
				i--;
			}
			ldst = (ulong *)dst;
			ldnext = (ulong *)dnext;
			ldlast = (ulong *)dlast;
			
			pix = *ldst;
			for (; i  >= 2;  i -= 2 )
			{
				pixlast = *ldlast++;
				l = 0xff & (pixlast>>24);
				t = 0xff & (pix>>24);
				pix &= 0x00ffffff;
				pixnext = *ldnext++;
				n = pixnext>>24;
				t = (short)((t<<1) + l + n)>>2;
				pix |= ((ulong)t) << 24;

				t = 0xff & (pix>>8);
				n = 0xff & (pixnext>>8);
				l = 0xff & (pixlast>>8);
				pix &= 0xffff00ff;
				t = (short)((t<<1) + l + n)>>2;
				pix |= ((ulong)t) << 8;
				*ldst++ = pix;
				pix = *ldst;
			}
			dst = (uchar *)ldst;
			dnext = (uchar *)ldnext;
			dlast = (uchar *)ldlast;
			
			if ( i == 1 )
			{
				t = (short)((*dst<<1) + *dlast + *dnext)>>2;
				*dst  = t;
				dst += 2;
				dlast += 2;
				dnext += 2;
			}
#endif
			dst += dstBump;
			dlast += dstBump;
			dnext += dstBump;
		}
	
	// partial row which doesn't include any component from next scan line
	
	if ( noBottom ) 
	{
		for (i=pixelCount; i-- > 0;  )
		{
			
			t = (short)(*dst * 3 + *dlast) >> 2;
			*dst  = t;
			dst += 1;
			dlast += 1;
			t = (short)(*dst * 3 + *dlast) >> 2;
			*dst  = t;
			dst += 1;
			dlast += 1;
		}
	}

}
#endif


BlitProcPtr
LookupBlitProc(const BitMapDevice *srcDevice,const BitMapDevice *dstDevice,ulong transparency)
{
	register BlitProcPtr p = nil;
	register long srcFormat = srcDevice->format;
	Boolean	 useFilterBlit = (dstDevice->filter & kCopyFilter) != 0;
	Boolean  isTransparent = transparency != kNotTransparent;
	
	if ( useFilterBlit && isTransparent )
		TrivialMessage(("transparent filter blit"));
		
	if ( !gFilterOnScreenUpdate )
		useFilterBlit = false;

	if ( !gSystem->GetUseFlickerFilter() )
		useFilterBlit = false;
	if ( dstDevice != &gOnScreenDevice )
		useFilterBlit = false;
	
	switch (dstDevice->format )
	{
#ifdef	SPOT1		
	case yuv422BucherFormat:								// fucked up SPOT1 frame buffer
	
		switch(srcFormat) 
		{
		case yuv422Format:
			if ( useFilterBlit )
				p = FilterBlitYUYVtoBucher;
			else
				p = ScaleBlitYUYVtoBucher;
			break;
		case yuv422BucherFormat:
			p = ScaleBlitBuchertoBucher;
			break;
		default:
			Message(("can't blit format %ld to yuv422BucherFormat",srcFormat));
			break;
		}
		break;
#endif

	case yuv422Format:								// target common mode
	
		switch(srcFormat) 
		{
		case index4Format:
			p = ScaleBlit4toYUYV;
			break;
		case antialias4Format:
			p = ScaleBlitAntiAlias4toYUYV;
			break;
		case alpha8Format: 
			p = ScaleBlitAlpha8toYUYV;
			break;
		case index8Format:
			if (srcDevice->colorTable != nil)
				if ( IsError((srcDevice->colorTable->version & 0xf) == kYUV32 ) ) {
					p = ScaleBlitAlpha8toYUYV;
					break;
				}
			p = ScaleBlit8toYUYV;
			break;
		case vqFormat:
			p = ScaleBlitVQtoYUYV;
			break;
		case antialias8Format:
			p = ScaleBlitAntiAlias8toYUYV;
			break;
		case yuv422Format:
			if ( useFilterBlit )
				p = FilterBlitYUYVtoYUYV;
			else
				p = ScaleBlitYUYVtoYUYV;
			break;
#ifdef	SIMULATOR
		case yuvFormat:
			p = ScaleBlitYUVtoYUYV;
			break;
		case rgb16Format:
			p = ScaleBlit16toYUYV;
			break;
		case rgb32Format:
			p = ScaleBlit32toYUYV;
			break;
#endif
		default:
			Message(("can't blit format %ld to YUYV",srcFormat));
			break;
		}
		break;
		
		
#ifdef	SIMULATOR

	case yuvFormat:
		switch(srcFormat) 
		{
		case index8Format:
			p = ScaleBlit8toYUV;
			break;
		case antialias8Format:
			p = ScaleBlitAntiAlias8toYUV;
			break;
		case yuvFormat:
			p = ScaleBlitYUVtoYUV;
			break;
		case yuv422Format:
			p = ScaleBlitYUYVtoYUV;
			break;
		default:
			Message(("can't blit format %d to YUV",srcFormat));
			break;
		}
		break;
		
	case rgb32Format:	
		switch(srcFormat) 
		{
		case index8Format:
			p = ScaleBlit8to32;
			break;
		case rgb32Format:
			p = ScaleBlit32to32;
			break;
		case antialias8Format:
			p = ScaleBlitAntiAlias8to32;
			break;
		case  rgb16Format:
			p = ScaleBlit16to32;
			break;
		case  yuvFormat:
			p = ScaleBlitYUVto32;
			break;
		case  yuv422Format:
			if ( useFilterBlit )
				p = FilterBlitYUYVto32;
			else
				p = ScaleBlitYUYVto32;
			break;
		default:
			Message(("can't blit format %d to 32",srcFormat));
			break;
		}
		break;
		
	case rgb16Format:
		switch(srcFormat) 
		{
		case vqFormat:
			p = ScaleBlitVQto16;
			break;
		case index4Format:
			p = ScaleBlit4to16;
			break;
		case antialias4Format:
			p = ScaleBlitAntiAlias4to16;
			break;
		case alpha8Format:
			p = ScaleBlitAlpha8to16;
			break;
		case index8Format:
			if (srcDevice->colorTable != nil)
				Assert ((srcDevice->colorTable->version & 0xf) != kYUV32 );	// color table has alpha should be alpha8Format
			p = ScaleBlit8to16;
			break;
		case rgb32Format:
			p = ScaleBlit32to16;
			break;
		case rgb16Format:
			p = ScaleBlit16to16;
			break;
		case antialias8Format:
			p = ScaleBlitAntiAlias8to16;
			break;
		case yuvFormat:
			p = ScaleBlitYUVto16;
			break;
		case yuv422Format:
			if ( useFilterBlit )
				p = FilterBlitYUYVto16;
			else
				p = ScaleBlitYUYVto16;
			break;
		default:
			Message(("can't blit format %d to 16",srcFormat));
			break;
		}
		break;
		
	case gray8Format:
		switch(srcFormat) 
		{
		case index8Format:
			p = ScaleBlit8toGray8;
			break;
		case rgb32Format:
			p = ScaleBlit32toGray8;
			break;
		case gray8Format:
			p = ScaleBlitGray8toGray8;
			break;
		default:
			Message(("can't blit format %d to gray",srcFormat));
			break;
		}
		break;		
#endif
	default:
		Message(("can't blit to format %d",dstDevice->format));
		break;
	}
	return p;	
}


// CopyRect


