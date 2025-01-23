
#include	"GraphicsPrivate.h"

#ifdef	SIMULATOR



#ifdef FOR_MAC
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
#endif



void
ScaleBlit8to32(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register const Byte*		cTable = GetCLUT24(p.src.device);

	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	long 				srcInc = p.src.xInc;
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary

	if ( srcInc < 0 )
		srcBump = p.src.bump+1;
	else
		srcBump = p.src.bump - longCount - 1;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register Byte transIndex = (Byte)p.src.device->transparentColor;
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (index != transIndex)
					{
						const Byte* rgb = &cTable[index * 3];
						dst += 1;			// alpha
						*dst++ = *rgb++;	// red
						*dst++ = *rgb++;	// green
						*dst++ = *rgb;		// blue
					}
					else
						dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparent case
		
			while (height > 0)
			{
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (index != transIndex)
					{
						const Byte* rgb = &cTable[index * 3];
						dst += 1;
						*dst++ = TransparencyBlend(transparency,*rgb,*dst);
						rgb++;
						*dst++ = TransparencyBlend(transparency,*rgb,*dst);
						rgb++;
						*dst++ = TransparencyBlend(transparency,*rgb,*dst);
					}
					else
						dst += 4;
				}
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
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{
					const Byte* rgb = &cTable[*src * 3];
					src += srcInc;
					dst += 1;			// alpha
					*dst++ = *rgb++;	// red
					*dst++ = *rgb++;	// green
					*dst++ = *rgb;		// blue
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
			
		else	// transparency case
			
			while (height > 0)
			{
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{
					const Byte* rgb = &cTable[*src * 3];
					src += srcInc;
					dst += 1;			// alpha
					*dst++ = TransparencyBlend(transparency,*rgb,*dst);
					rgb++;
					*dst++ = TransparencyBlend(transparency,*rgb,*dst);
					rgb++;
					*dst++ = TransparencyBlend(transparency,*rgb,*dst);
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}

 void
ScaleBlitAntiAlias8to32(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;
	register long		srcBump;
	register long		dstBump;
	ulong 				fColor= p.src.device->foregroundColor;
	uchar				r,g,b;
	uchar				unTransparent = 258-transparency;
	
	PIXEL32TORGB(fColor,r,g,b);
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary

	srcBump = p.src.bump - longCount - 1;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	while (height > 0)
	{
		for (long i=0; i <= longCount; i++)
		{
			uchar alpha = *src++;
			if (transparency != kNotTransparent)
				alpha = (unTransparent * alpha) >> 8;
			if (alpha == 0)
				dst += 4;
			else
			{
				dst += 1;
				*dst++ = AlphaBlend(alpha,r,*dst);
				*dst++ = AlphaBlend(alpha,g,*dst);
				*dst++ = AlphaBlend(alpha,b,*dst);
			}
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}

 void
ScaleBlitAntiAlias8to16(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	ulong				fColor = p.src.device->foregroundColor;
	uchar				r,g,b;
	uchar				unTransparent = kFullyTransparent-transparency;
	
	PIXEL32TORGB(fColor,r, g, b );
	MapColor(rgb16Format,fColor);
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)

	srcBump = p.src.bump - pixelCount - 1;
	dstBump = (p.dst.bump - pixelCount * 2 - 2) / 2;
	
	while (height > 0)
	{
		for (long i=0; i <= pixelCount; i++)
		{
			uchar alpha = *src++;
			if ( transparency != kNotTransparent )
				alpha = (alpha * unTransparent) >> 8;	// apply transparency
			if ( alpha == kAlphaOpaque )
				*dst = fColor;
			else if (alpha != kAlphaTransparent)
			{
				uchar dr, dg, db;
				ushort pixel = *dst;
				PIXEL16TORGB(pixel, dr, dg, db);
				RGBTOPIXEL16( AlphaBlend(alpha,r,dr), AlphaBlend(alpha,g,dg), AlphaBlend(alpha,b,db), pixel);
				*dst = pixel;
			}
			dst++;
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}



 void
ScaleBlitAntiAlias4to16(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits>>4);
	register long		height = p.dst.r.bottom - p.dst.r.top;
	register long		srcBump;
	register long		dstBump;
	ulong				fColor= p.src.device->foregroundColor;
	uchar				indices,index;
	uchar 				dr, dg, db,r,g,b;
	ushort				pixel;
	uchar				alpha;
	Boolean				oddStart = false;
	Boolean				oddEnd = false;
	uchar				unTransparent = kFullyTransparent-transparency;
	
	PIXEL32TORGB(fColor,r, g, b );
	MapColor(rgb16Format,fColor);
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)
	oddStart = (p.src.leftBits >> 2) & 1;
	
	srcBump = p.src.bump;
	dstBump = (p.dst.bump - pixelCount * 2) >> 1;

	Assert(p.src.xInc > 0);
	
	while (height > 0)
	{
		Boolean skipFirst = oddStart;
		uchar *sp = src;
		long i;
		
		for ( i= 0; i < pixelCount; i++ )
		{
			indices = *sp++;
			if ( !skipFirst )
			{
				index = (indices>>4) & 0xf;
				alpha = (index<<4) | index;
				if ( transparency != kNotTransparent )
					alpha = (alpha * (uchar)unTransparent) >> 8;
				if (alpha == kAlphaOpaque)
					*dst = fColor;
				else if (alpha != kAlphaTransparent)
				{
					pixel = *dst;
					PIXEL16TORGB(pixel, dr, dg, db);
					dr = AlphaBlend(alpha,r,dr);
					dg = AlphaBlend(alpha,g,dg);
					db = AlphaBlend(alpha,b,db);
					RGBTOPIXEL16(dr,dg,db,*dst);
				}
				dst++;
				if ( ++i == pixelCount )
					break;
			}
			skipFirst = false;
			index = indices  & 0xf;
			alpha = (index<<4) | index;
			if ( transparency != kNotTransparent )
				alpha = (alpha * (uchar)unTransparent) >> 8;
			if (alpha == kAlphaOpaque)
				*dst = fColor;
			else if (alpha != 0)
			{
				pixel = *dst;
				PIXEL16TORGB(pixel, dr, dg, db);
				dr = AlphaBlend(alpha,r,dr);
				dg = AlphaBlend(alpha,g,dg);
				db = AlphaBlend(alpha,b,db);
				RGBTOPIXEL16(dr,dg,db,*dst);
			}
			dst++;
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}


 void
ScaleBlit32toGray8(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register ulong*		src = (ulong*)p.src.base;
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 3) - (p.dst.leftBits >> 3) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register ulong		pixel = 0;
	uchar 				r,g,b;
	
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary

	srcBump = (p.src.bump - pixelCount * 4 - 4) / 4;
	dstBump = p.dst.bump - pixelCount - 1;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register ulong transIndex = (Byte)p.src.device->transparentColor & kTransColorMask;
		
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex)
						*dst++ = ((pixel>>16 & 0xff) * 5 + (pixel>>8 & 0xff) * 9 + (pixel<<1 & 0x1fe)) >> 4 ;		// Y = 5R + 9G + 2B
					else
						dst += 1;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex)
					{	
						PIXEL32TORGB(pixel,r,g,b);
						uchar y = (r * 5 + g *9 + b*2)>>4;
						*dst++ = TransparencyBlend(transparency,y,*dst);
					}
					else
						dst += 1;
				}
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
				for (long i=0; i <= pixelCount; i++)
				{
					pixel = *src++;
					PIXEL32TORGB(pixel,r,g,b);
					*dst++ = (r * 5 + g *9 + b*2)>>4;;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency
			
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{
					PIXEL32TORGB(pixel,r,g,b);
					uchar y = (r * 5 + g *9 + b*2)>>4;
					*dst++ = TransparencyBlend(transparency,y,*dst);
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}


 void
ScaleBlit8toGray8(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register const Byte*		cTable = GetCLUT24(p.src.device);

	register long		pixelCount = (p.dst.rightBits >> 3) - (p.dst.leftBits >> 3) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	long				srcInc = p.src.xInc;
	Boolean				hasTransparentSource = (p.src.device->transparentColor & kTransparent) != 0;
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary


	if ( srcInc < 0 )
		srcBump = p.src.bump+1;
	else
		srcBump = p.src.bump - pixelCount - 1;
	dstBump = p.dst.bump - pixelCount - 1;
	
	{
		register Byte transIndex = (Byte)p.src.device->transparentColor;
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				if ( srcInc < 0 )
					src += pixelCount;
				for (long i=0; i <= pixelCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (!hasTransparentSource || index != transIndex)
					{
						const Byte* rgb = &cTable[index * 3];
						*dst++ = (*rgb++ * 5 + *rgb++ * 9 + *rgb * 2) >> 4 ;		// Y = 5R + 9G + 2B
					}
					else
						dst += 1;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparent case
		
			while (height > 0)
			{
				if ( srcInc < 0 )
					src += pixelCount;
				for (long i=0; i <= pixelCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (!hasTransparentSource || index != transIndex)
					{
						const Byte* rgb = &cTable[index * 3];
						uchar y = (rgb[0] * 5 + rgb[1] *9 + rgb[2]*2)>>4;
						*dst++ = TransparencyBlend(transparency,y,*dst);
					}
					else
						dst += 1;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}

 void
ScaleBlitAlpha8to16(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;
	register uchar		*clut = p.src.device->colorTable->data;
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;
	short				transIndex = -1;
	register long		srcBump;
	register long		dstBump;
	register ushort		pixel;
	long				srcInc = p.src.xInc;
	char*				debugBase = (char*)p.dst.base;
	char*				debugMax = debugBase + (p.dst.r.bottom - p.dst.r.top)*p.dst.device->rowBytes;
	uchar				r,g,b;
	short				y,u,v,dy,du,dv;
	short				a,na = 0;
	uchar				unTransparency = (kFullyTransparent-transparency);
	
	if ( IsWarning((p.src.device->colorTable->version & 0xf) != kYUV32) )
	{
		Complain(("incompatible color table"));
		ScaleBlit8to16(p,transparency);
		return;
	}
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)

	if ( srcInc < 0 )
		srcBump = p.src.bump + 1;
	else
		srcBump = p.src.bump - pixelCount - 1;

	dstBump = (p.dst.bump - pixelCount * 2 - 2) / 2;
	Assert(dstBump >= 0);
		
	if (p.src.device->transparentColor & kTransparent)
		transIndex = (Byte)p.src.device->transparentColor;
	while (height > 0)
	{
		if ( srcInc < 0 )
			src += pixelCount;
		for (long i=0; i <= pixelCount; i++)
		{	
			Byte  index = *src;
			src += srcInc;
			if (index != transIndex)
			{
				uchar *cp = clut + (index<<2);
				y = *cp++;
				u = *cp++;
				a = *cp++;
				v = *cp++;
				pixel = *dst;
				PIXEL16TORGB(pixel, r, g, b);
				RGBTOYUV(r,g,b,dy,du,dv);
				a = (a * unTransparency) >> 8;
				y = AlphaBlend(a,y,dy);
				u = AlphaBlend(a,u,du);
				v = AlphaBlend(a,v,dv);
				YUVTORGB(y,u,v,r,g,b);
				RGBTOPIXEL16(r,g,b,*dst);
			}
			dst++;
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}


 void
ScaleBlitGray8toGray8(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 3) - (p.dst.leftBits >> 3) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary

	srcBump = p.src.bump - pixelCount - 1;
	dstBump = p.dst.bump - pixelCount - 1;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register Byte transIndex = (Byte)p.src.device->transparentColor;
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					Byte  index = *src++;
					if (index != transIndex)
						*dst++ = index;
					else
						dst += 1;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparent case
		
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					Byte  index = *src++;
					if (index != transIndex)
						*dst++ = TransparencyBlend(transparency,index,*dst);
					else
						dst += 1;
				}
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
				for (long i=0; i <= pixelCount; i++)
					*dst++ = *src++;
				dst += dstBump;
				src += srcBump;
				height--;
			}
			
		else	// transparency case
			
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{
					*dst++ = TransparencyBlend(transparency,*src,*dst);
					src++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}



 void
ScaleBlitYUVtoYUV(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;

	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
		
	srcBump = p.src.bump - longCount * 4 - 4;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		if (transparency == kNotTransparent)
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{	
					ulong  pixel = *(long *)src;
					src += 4;
					if ((pixel & 0x0000ff00) != 0 )
						*(long *)dst = pixel;
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparent case
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{	
					ulong  pixel = *(long *)src;
					if ((pixel & 0x0000ff00) != 0)
					{
						*dst++ = TransparencyBlendY(transparency,*src,*dst);
						src++;
						*dst++ = TransparencyBlend(transparency,*src,*dst);
						src++;
						*dst++ = *src++;  // alpha
						*dst++ = TransparencyBlend(transparency,*src,*dst);
						src++;
					}
					else
						dst += 4;	src += 4;
				}
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
				for (long i=0; i <= longCount; i++)
				{
					*(long*)dst = *(long*)src;
					dst += 4;
					src += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
			
		else 
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{
					*dst++ = TransparencyBlendY(transparency,*src,*dst);
					src++;
					*dst++ = TransparencyBlend(transparency,*src,*dst);
					src++;
					*dst++ = *src++;  // alpha
					*dst++ = TransparencyBlend(transparency,*src,*dst);
					src++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}




 void
ScaleBlitYUVto16(BlitParams2& p, ulong transparency)
{


	// important inner loop variables
	register ulong*		src = (ulong*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register ulong		pixel;
	short				y,u,v;
	uchar				r,g,b;
	
	
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)

	srcBump = (p.src.bump - pixelCount * 4 - 4) / 4;
	dstBump = (p.dst.bump - pixelCount * 2 - 2) / 2;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if ( (pixel & 0x0000ff00) != 0) {
						PIXEL32TOYUV(pixel,y,u,v);
						YUVTORGB(y,u,v,r,g,b);
						RGBTOPIXEL16(r,g,b,*dst);
					}
					dst++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				uchar dr,dg,db;
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if ((pixel & 0x0000ff00) != 0)
					{
						PIXEL32TOYUV(pixel,y,u,v);
						YUVTORGB(y,u,v,r,g,b);
						PIXEL16TORGB(*dst, dr, dg, db);
						dr = TransparencyBlend(transparency,r,dr);
						dg = TransparencyBlend(transparency,g,dg);
						db = TransparencyBlend(transparency,b,db);
						RGBTOPIXEL16(dr,dg,db,*dst);
					}
					dst++;
				}
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
				for (long i=0; i <= pixelCount; i++)
				{
					pixel = *src++;
					PIXEL32TOYUV(pixel,y,u,v);
					YUVTORGB(y,u,v,r,g,b);
					RGBTOPIXEL16(r,g,b,*dst);
					dst++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency
			
			while (height > 0)
			{
				uchar dr,dg,db;
				for (long i=0; i <= pixelCount; i++)
				{
					pixel = *src++;
					PIXEL32TOYUV(pixel,y,u,v);
					YUVTORGB(y,u,v,r,g,b);
					PIXEL16TORGB(*dst, dr, dg, db);
					dr = TransparencyBlend(transparency,r,dr);
					dg = TransparencyBlend(transparency,g,dg);
					db = TransparencyBlend(transparency,b,db);
					RGBTOPIXEL16(dr,dg,db,*dst);
					dst++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}

}





 void
ScaleBlit8toYUV(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register const Byte*		cTable = GetCLUTYUV(p.src.device);

	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	uchar	y,u,v;
	long				srcInc = p.src.xInc;
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary

	if ( srcInc < 0 )
		srcBump = p.src.bump+1;
	else
		srcBump = p.src.bump - longCount - 1;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register Byte transIndex = (Byte)p.src.device->transparentColor;
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (index != transIndex)
					{
						const Byte* yuv = &cTable[index * 3];
						y = *yuv++;
						u = *yuv++;
						v = *yuv++;
						YUVTOPIXEL32(y,u,v,*(long *)dst);
						dst += 4;
					}
					else
						dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparent case
		
			while (height > 0)
			{
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (index != transIndex)
					{
						const Byte* yuv = &cTable[index * 3];
						y = *yuv++;
						u = *yuv++;
						v = *yuv++;
						*dst++ = TransparencyBlendY(transparency,y,*dst);
						*dst++ = TransparencyBlend(transparency,u,*dst);
						*dst++;
						*dst++ = TransparencyBlend(transparency,v,*dst);
					}
					else
						dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
	else
	{
		if (transparency == kNotTransparent )
			while (height > 0)
			{
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{
					const Byte* yuv = &cTable[*src * 3];
					src += srcInc;
					y = *yuv++;
					u = *yuv++;
					v = *yuv++;
					YUVTOPIXEL32(y,u,v,*(long *)dst);
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
			
		else	// transparency case
			
			while (height > 0)
			{
				if ( srcInc < 0)
					src += longCount;
				for (long i=0; i <= longCount; i++)
				{
					const Byte* yuv = &cTable[*src * 3];
					src += srcInc;
					y = *yuv++;
					u = *yuv++;
					v = *yuv++;
					*dst++ = TransparencyBlendY(transparency,y,*dst);
					*dst++ = TransparencyBlend(transparency,u,*dst);
					dst++;
					*dst++ = TransparencyBlend(transparency,v,*dst);
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}




 void
ScaleBlitAntiAlias8toYUV(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;

	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	short				y,u,v;
	ulong 				fYUV;
	ulong				fColor = p.src.device->foregroundColor;
	uchar				r,g,b;
	
	
	PIXEL32TORGB(fColor,r,g,b);
	RGBTOYUV(r,g,b,y,u,v);
	YUVTOPIXEL32(y,u,v,fYUV);
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary

	srcBump = p.src.bump - longCount - 1;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	while (height > 0)
	{
		for (long i=0; i <= longCount; i++)
		{
			uchar alpha = *src++;
			alpha = (alpha * (uchar)transparency) >> 8;	// apply transparency
			if (alpha == 0)
				dst += 4;
			else
			{
				*dst++ = TransparencyBlendY(alpha,y,*dst);
				*dst++ = TransparencyBlend(alpha,u,*dst);
				dst++;
				*dst++ = TransparencyBlend(alpha,v,*dst);
			}
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}







 void
ScaleBlitYUVto32(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;

	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register short		y,u,v;
	register uchar 		r,g,b;
	uchar dr,dg,db;
	ulong		pixel;
	
	srcBump = p.src.bump - longCount * 4 - 4;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{	
					pixel = *(long *)src;
					src += 4;
					if ( (pixel & 0xff000000) != 0xff000000)
					{
						PIXEL32TOYUV(pixel,y,u,v);
						YUVTORGB(y,u,v,r,g,b);
						RGBTOPIXEL32(r,g,b,*(long *)dst);
					}
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparent case
		
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{	
					pixel = *(long *)src;
					src += 4;
					if ( (pixel & 0xff000000) != 0xff000000)
					{
						PIXEL32TOYUV(pixel,y,u,v);
						YUVTORGB(y,u,v,r,g,b);
						pixel = *(long *)dst;
						PIXEL32TORGB(pixel,dr,dg,db);
						r = TransparencyBlend(transparency,r,dr);
						g = TransparencyBlend(transparency,g,dg);
						b = TransparencyBlend(transparency,b,db);
						RGBTOPIXEL32(r,g,b,*(long *)dst);
					}
					dst += 4;
				}
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
				for (long i=0; i <= longCount; i++)
				{
					pixel = *(long *)src;
					src += 4;
					PIXEL32TOYUV(pixel,y,u,v);
					YUVTORGB(y,u,v,r,g,b);
					RGBTOPIXEL32(r,g,b,*(long *)dst);
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else 
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{
					pixel = *(long *)src;
					src += 4;
					PIXEL32TOYUV(pixel,y,u,v);
					YUVTORGB(y,u,v,r,g,b);
					pixel = *(long *)dst;
					PIXEL32TORGB(pixel,dr,dg,db);
					r = TransparencyBlend(transparency,r,dr);
					g = TransparencyBlend(transparency,g,dg);
					b = TransparencyBlend(transparency,b,db);
					RGBTOPIXEL32(r,g,b,*(long *)dst);
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}



 void
ScaleBlit32to32(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;

	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	
	srcBump = p.src.bump - longCount * 4 - 4;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register ulong transIndex = (Byte)p.src.device->transparentColor & kTransColorMask;
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{	
					ulong  pixel = *(long *)src;
					src += 4;
					if (pixel != transIndex)
						*(long *)dst = pixel;
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparent case
		
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{	
					ulong  pixel = *(long *)src;
					if (pixel != transIndex)
					{
						dst += 1;	src += 1;
						*dst = TransparencyBlend(transparency,*src,*dst);
						src++;
						dst++;
						*dst = TransparencyBlend(transparency,*src,*dst);
						src++;
						dst++;
						*dst = TransparencyBlend(transparency,*src,*dst);
						src++;
						dst++;
					}
					else
						dst += 4;	src += 4;
				}
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
				for (long i=0; i <= longCount; i++)
				{
					*(long*)dst = *(long*)src;
					dst += 4;
					src += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
			
		else if (transparency != 128)
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{
					dst += 1;	src += 1;
					*dst = TransparencyBlend(transparency,*src,*dst);
					src++;
					dst++;
					*dst = TransparencyBlend(transparency,*src,*dst);
					src++;
					dst++;
					*dst = TransparencyBlend(transparency,*src,*dst);
					src++;
					dst++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else  // transparency == 128
		{
			while (height > 0)
			{
				for (long i=0; i <= longCount; i++)
				{
					ulong	s = *(ulong *)src;
					ulong	d = *(ulong *)dst;
					
					*(ulong *)dst = ((d & ~kLowBitsMask8) >> 1) + ((s & ~kLowBitsMask8) >> 1) + (s & d & kLowBitsMask8);
					dst += 4; src += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		}
	}
}


 void
ScaleBlit32to16(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register ulong*		src = (ulong*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register ulong		pixel;
	
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)

	srcBump = (p.src.bump - pixelCount * 4 - 4) / 4;
	dstBump = (p.dst.bump - pixelCount * 2 - 2) / 2;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register ulong transIndex = (Byte)p.src.device->transparentColor & kTransColorMask;
		
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex)
						*dst++ = ((pixel>>9) & 0x7C00) + ((pixel>>6) & 0x03E0) + ((pixel>>3) & 0x001F);
					else
						dst += 1;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				uchar dr,dg,db,sr,sg,sb;
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex)
					{
						PIXEL16TORGB(*dst, dr, dg, db);
						PIXEL32TORGB(pixel, sr, sg, sb);
						dr = TransparencyBlend(transparency,sr,dr);
						dg = TransparencyBlend(transparency,sg,dg);
						db = TransparencyBlend(transparency,sb,db);
						RGBTOPIXEL16(dr,dg,db,*dst);
					}
					else
						dst += 1;
				}
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
				for (long i=0; i <= pixelCount; i++)
				{
					pixel = *src++;
					*dst++ = ((pixel>>9) & 0x7C00) + ((pixel>>6) & 0x03E0) + ((pixel>>3) & 0x001F);
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency
			
			while (height > 0)
			{
				uchar dr,dg,db,sr,sg,sb;
				for (long i=0; i <= pixelCount; i++)
				{
					pixel = *src++;
					PIXEL16TORGB(*dst, dr, dg, db);
					PIXEL32TORGB(pixel, sr, sg, sb);
					dr = TransparencyBlend(transparency,sr,dr);
					dg = TransparencyBlend(transparency,sg,dg);
					db = TransparencyBlend(transparency,sb,db);
					RGBTOPIXEL16(dr,dg,db,*dst);
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}

 void
ScaleBlit16to32(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 5) - (p.dst.leftBits >> 5) - 1;
	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register ulong		pixel;
	uchar				r,g,b;
	
	src += (p.src.leftBits >> 4) & 1;	// advance to ragged word boundary (src is a ushort*)

	srcBump = (p.src.bump - pixelCount * 2 - 2) / 2;
	dstBump = p.dst.bump - longCount * 4 - 4;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register ulong transIndex = (Byte)p.src.device->transparentColor & kTransColorMask;
		
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex) 
					{
						PIXEL16TORGB(pixel, r, g, b);
						dst++;
						*dst++ = r;
						*dst++ = g;
						*dst++ = b;
					}
					else
						dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				uchar sr,sg,sb;
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex)
					{
						PIXEL16TORGB(pixel, sr, sg, sb);
						PIXEL32TORGB(pixel, r, g, b);
						r = TransparencyBlend(transparency,sr,r);
						g = TransparencyBlend(transparency,sg,g);
						b = TransparencyBlend(transparency,sb,b);
						RGBTOPIXEL32(r,g,b,*(long *)dst);
						dst += 4;
					}
					else
						dst += 4;
				}
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
				for (long i=0; i <= pixelCount; i++) 
				{
					PIXEL16TORGB(*src, r, g, b);
					src++;
					dst++;
					*dst++ = r;
					*dst++ = g;
					*dst++ = b;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else 
			while (height > 0)
			{
				uchar sr,sg,sb;
				for (long i=0; i <= pixelCount; i++)
				{
					pixel = *src++;
					PIXEL16TORGB(pixel, sr, sg, sb);
					pixel = *(long *)dst;
					PIXEL32TORGB(pixel, r, g, b);
					r = TransparencyBlend(transparency,sr,r);
					g = TransparencyBlend(transparency,sg,g);
					b = TransparencyBlend(transparency,sb,b);
					RGBTOPIXEL32(r,g,b,*(long *)dst);
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}


 void
InterlaceFilterRGB32(BlitParams& p, FilterType filterType)
{


	register Byte*		dst = (Byte*)p.base;
	register uchar		*dlast,*dnext;
	register uchar		t;
	register long		dstBump;
	long				i;
	long				pixelCount = (p.rightBits >> 5) - (p.leftBits >> 5);
	long				height = p.r.bottom - p.r.top;
	long				rowBytes;
	Boolean				noTop = false,noBottom = false;
	Boolean				doSlight = false;
	uchar 				r,g,b;
	ulong 				pix;


	if ( filterType == kNoFilter )
		return;
	dst += (p.leftBits >> 3) & 3;	// advance to ragged byte boundary
	rowBytes  = p.bump;
	dstBump = (p.bump - pixelCount * 4);

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
	}
		
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
			pix = *(long *)dst;
			r = (pix>>16) & 0xff;
			g = (pix>>8) & 0xff;
			b = pix & 0xff;
			r = ((r << 1) + dnext[1])>>2;
			g = ((g << 1) + dnext[2])>>2;
			b = ((b << 1) + dnext[3])>>2;
			*(long *)dst = (r<<16) + (g<<8) + b;
			dst += 4;
			dnext += 4;
		}
		dst += dstBump;
		dnext += dstBump;
		height -= 1;
	}
	
	if (noBottom) 
		height -= 1;
	if ( height <= 0 )
		return;
		
	dnext = dst + rowBytes;
	dlast = dst - rowBytes;
	
	while (height-- > 0)
	{
		t = *dst;
		for (i=pixelCount; i-- > 0;  )
		{
			pix = *(long *)dst;
			r = (pix>>16) & 0xff;
			g = (pix>>8) & 0xff;
			b = pix & 0xff;
			r = ((r << 1) + dlast[1] + dnext[1])>>2;
			g = ((g << 1) + dlast[2] + dnext[2])>>2;
			b = ((b << 1) + dlast[3] + dnext[3])>>2;
			*(long *)dst = (r<<16) + (g<<8) + b;
			dst += 4;
			dlast += 4;
			dnext += 4;
		}
		dst += dstBump;
		dlast += dstBump;
		dnext += dstBump;
	}
	
	// partial row which doesn't include any component from next scan line
	
	if ( noBottom ) 
	{
		dlast = dst - rowBytes;
		for (i=pixelCount; i-- > 0;  )
		{
			pix = *(long *)dst;
			r = (pix>>16) & 0xff;
			g = (pix>>8) & 0xff;
			b = pix & 0xff;
			r = ((r << 1) + dlast[1])>>2;
			g = ((g << 1) + dlast[2])>>2;
			b = ((b << 1) + dlast[3])>>2;
			*(long *)dst = (r<<16) + (g<<8) + b;
			dst += 4;
			dlast += 4;
		}
	}

}



 void FilterBlitYUYVto32(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register ushort*	lsrc = (ushort*)((uchar*)p.src.base - p.src.device->rowBytes);
	register ushort*	nsrc = (ushort*)((uchar*)p.src.base + p.src.device->rowBytes);
	ushort*				srcEnd = (ushort*)((uchar*)p.src.device->baseAddress + (p.src.device->bounds.bottom-p.src.device->bounds.top) * p.src.device->rowBytes);
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 5) - (p.dst.leftBits >> 5) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;
	int					firstLine = lsrc < (ushort *)p.src.device->baseAddress;
	int					nextLine = 0;
	register long		srcBump;
	register long		dstBump;
	uchar				r,g,b;
	short				y1,y2,u,v;
	ulong				pixel;
	ulong				borderColor;
	
	borderColor = ((gScreenBorderColor>>16)  & 0xff) << 8;		// 0000yy00
	borderColor |= (borderColor<<16);							// yy00yy00
	borderColor |= ((gScreenBorderColor>>8)  & 0xff) << 16;		// yyuuyy00
	borderColor |= gScreenBorderColor & 0xff;					// yyuuyyvv

	src += (p.src.leftBits >> 4) & 1;
	nsrc += (p.src.leftBits >> 4) & 1;
	lsrc += (p.src.leftBits >> 4) & 1;

	srcBump = (p.src.bump - pixelCount * 2 - 2) >>1;
	dstBump = p.dst.bump - pixelCount * 4 - 4;

	if ( p.dst.device->filter == kCopyFilterOffset ) {			// slice filter
		firstLine = 2;
	}
	while (height > 0)
	{
		if  ( height == 1 &&  ( nsrc >= srcEnd || p.dst.device->filter == kCopyFilterOffset ) ) 
			nextLine = p.dst.device->filter == kCopyFilterOffset ? 2 : 1;
		for (long i=0; i <= pixelCount; i++)
		{
			pixel = *(ulong *)src;
			PIXELS16TOYUYV(pixel, y1, u, y2, v);
			{
				short	y1N,y2N,uN,vN;
				short	y1L,y2L,uL,vL;
				ulong	pixelN,pixelL;
				
				if ( firstLine == 0  )
					pixelL = *(ulong *)lsrc;
				else if ( firstLine == 1  )
					pixelL = borderColor;
				else
					pixelL = pixel;
				PIXELS16TOYUYV(pixelL, y1L, uL, y2L, vL);
				y1 <<= 1;
				u <<= 1;
				v <<= 1;
				y1 += y1L;
				u += uL;
				v += vL;
				if ( nextLine == 0 ) 
					pixelN = *(ulong *)nsrc;
				else if ( nextLine == 1 )
					pixelN = borderColor;
				else
					pixelN = pixel;
				PIXELS16TOYUYV(pixelN, y1N, uN, y2N, vN);
				y1 += y1N;
				u += uN;
				v += vN;
				y1 >>= 2;
				u >>= 2;
				v >>= 2;
			}
			if ( (((ulong)src) & 2)  ) 		// swap u and v
			{
				short t = u;
				u = v;
				v = t;
			}
			lsrc++;
			src++;
			nsrc++;
			YUVTORGB(y1,u,v,r,g,b);
			if ( transparency ) {
				uchar dr,dg,db;
				ulong dpix = *(ulong *)dst;
				PIXEL32TORGB(dpix,dr,dg,db);
				r = TransparencyBlend(transparency,r,dr);
				g = TransparencyBlend(transparency,g,dg);
				b = TransparencyBlend(transparency,b,db);
			}
			RGBTOPIXEL32(r,g,b,*(ulong *)dst);
			dst += 4;
		}
		dst += dstBump;
		src += srcBump;
		lsrc += srcBump;
		nsrc += srcBump;
		height--;
		firstLine = 0;
	}
}



 void
ScaleBlitYUYVto32(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register uchar*	dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 5) - (p.dst.leftBits >> 5) - 1;
	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	uchar				r,g,b;
	short				y1,y2,u,v,uvg,lu,lv;
	ulong				pixel;
	int					uvphase = 0;
	int					uvInitialPhase = 0;
	
	src += (p.src.leftBits >> 4) & 1;	// advance to ragged word boundary (src is a ushort*)

	if ( ( p.src.leftBits>>4) & 1 )
		uvInitialPhase++;
	srcBump = (p.src.bump - pixelCount * 2 - 2) / 2;
	dstBump = p.dst.bump - pixelCount * 4 - 4;

	if (p.src.device->transparentColor & kTransparent)
	{		
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				lu = -1;
				lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++, uvphase++)
				{	
					if (*(uchar*)src != kTransparentY) 		
					{
						pixel = *(long *)src;
						PIXELS16TOYUYV(pixel, y1, u, y2, v);
						src++;
						if ( (uvphase & 1) ) 
						{
							uvg = u;
							u = v;
							v = uvg;
						}
						if ( lu == -1 ) 
							lu = u;
						if ( lv == -1 )
							lv = v;
						uvg = (lu + u)>> 1;
						lu = u;
						u = uvg;
						uvg = (lv + v)>> 1;
						lv = v;
						v = uvg;
						PREPUV(u,v,uvg);
						DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
						RGBTOPIXEL32(r,g,b,*(long *)dst);
						dst += 4;
					}
					else 
					{
						lu = -1;
						lv = -1;
						src++;
						dst += 4;
					}
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				lu = -1;
				lv = -1;
				uvphase = uvInitialPhase;
				uchar dr,dg,db;
				for (long i=0; i <= pixelCount; i++,uvphase++)
				{	
					if (*(uchar*)src != kTransparentY)
					{
						pixel = *(long *)src;
						
						PIXELS16TOYUYV(pixel, y1, u, y2, v);
						src++;
						if ( (uvphase & 1 ) ) 
						{
							uvg = u;
							u = v;
							v = uvg;
						}
						if ( lu == -1 )
							lu = u;
						if ( lv == -1 )
							lv = v;
						uvg = (lu + u)>> 1;
						lu = u;
						u = uvg;
						uvg = (lv + v)>> 1;
						lv = v;
						v = uvg;
						PREPUV(u,v,uvg);
						DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
						pixel = *(long *)dst;
						PIXEL32TORGB(pixel,dr,dg,db);
						r = TransparencyBlend(transparency,r,dr);
						g = TransparencyBlend(transparency,g,dg);
						b = TransparencyBlend(transparency,b,db);
						RGBTOPIXEL32(r,g,b,*(long *)dst);
						dst += 4;
					}
					else
					{	
						src++;
						dst += 4;
						lu = -1;
						lv = -1;
					}
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
	else
	{
		if ( transparency == kNotTransparent)
			while (height > 0)
			{
				lu = -1;
				lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++,uvphase++) 
				{
					pixel = *(long *)src;
					PIXELS16TOYUYV(pixel, y1, u, y2, v);
					src++;
					if ( (uvphase & 1) ) 
					{
						uvg = u;
						u = v;
						v = uvg;
					}
					if ( lu == -1 )
						lu = u;
					if ( lv == -1 )
						lv = v;
					uvg = (lu + u)>> 1;
					lu = u;
					u = uvg;
					uvg = (lv + v)>> 1;
					lv = v;
					v = uvg;
					PREPUV(u,v,uvg);
					DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
					RGBTOPIXEL32(r,g,b,*(long *)dst);
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else 
			while (height > 0)
			{
				uchar dr,dg,db;
				lu = -1;
				lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++,uvphase++)
				{
					pixel = *(long *)src;
					PIXELS16TOYUYV(pixel, y1, u, y2, v);
					src++;
					if ((uvphase & 1) ) 
					{
						uvg = u;
						u = v;
						v = uvg;
					}
					if ( lu == -1 )
						lu = u;
					if ( lv == -1 )
						lv = v;
					uvg = (lu + u)>> 1;
					lu = u;
					u = uvg;
					uvg = (lv + v)>> 1;
					lv = v;
					v = uvg;
					PREPUV(u,v,uvg);
					DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
					pixel = *(long *)dst;
					PIXEL32TORGB(pixel,dr,dg,db);
					r = TransparencyBlend(transparency,r,dr);
					g = TransparencyBlend(transparency,g,dg);
					b = TransparencyBlend(transparency,b,db);
					RGBTOPIXEL32(r,g,b,*(long *)dst);
					dst += 4;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}

}

 void
FilterBlitYUYVto16(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register ushort*	lsrc = (ushort*)((uchar*)p.src.base - p.src.device->rowBytes);
	register ushort*	nsrc = (ushort*)((uchar*)p.src.base + p.src.device->rowBytes);
	ushort*				srcEnd = (ushort*)((uchar*)p.src.device->baseAddress + (p.src.device->bounds.bottom-p.src.device->bounds.top) * p.src.device->rowBytes);
	register ushort*	dst = (ushort*)p.dst.base;
	
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;
	int					firstLine = lsrc < (ushort *)p.src.device->baseAddress;
	int					nextLine = 0;
	
	register long		srcBump;
	register long		dstBump;
	uchar				r,g,b;
	short				y,u,v,uvg,t,lu,lv;
	ushort				pixel,pixelL,pixelN;
	int					uvphase = 0;
	int					uvInitialPhase = 0;
	ulong				borderColor;
	
	borderColor = ((gScreenBorderColor>>16)  & 0xff) << 8;		// 0000yy00
	borderColor |= (borderColor<<16);							// yy00yy00
	borderColor |= ((gScreenBorderColor>>8)  & 0xff) << 16;		// yyuuyy00
	borderColor |= gScreenBorderColor & 0xff;					// yyuuyyvv
	
	src += (p.src.leftBits >> 4) & 1;	
	dst += (p.dst.leftBits >> 4) & 1;	
	
	if  ( ((ulong)src) & 2 ) {
		Complain(("misaligned screen blit"));
		return;
	}
	
	srcBump = (p.src.bump>>1) - pixelCount;
	dstBump = (p.dst.bump>>1) - pixelCount;

	if ( p.dst.device->filter == kCopyFilterOffset ) {			// slice filter
		firstLine = 2;
	}
	while (height > 0) {
		if  ( height == 1 &&  ( nsrc >= srcEnd || p.dst.device->filter == kCopyFilterOffset ) ) 
			nextLine = p.dst.device->filter == kCopyFilterOffset ? 2 : 1;
		uvphase = uvInitialPhase;
		if ( (uvphase & 1) )
		{	
			lv = *(((uchar *)src) + 1);
			lu = *(((uchar *)src) + 3);
		}
		else 
		{
			lu = *(((uchar *)src) + 1);
			lv = *(((uchar *)src) + 3);
		}
		for (long i=0; i < pixelCount; i++, uvphase++) 
		{
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
		
			y = FilterBytes((pixelL>>8),(pixel>>8),(pixelN>>8));
			u = FilterChromaBytes((pixelL&0xff),(pixel&0xff),(pixelN&0xff));
			src++;
			lsrc++;
			nsrc++;
			pixel = *src;
			if ( firstLine == 0 )
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
			v = FilterChromaBytes((pixelL&0xff),(pixel&0xff),(pixelN&0xff));
			
			if ( (uvphase & 1) ) 
			{
				uvg = (lu + v)>> 1;
				lu = v;
				t = (lv + u) >> 1;
				lv = u;
				u = uvg;
				v = t;
			}
			else
			{
				uvg = (lu + u)>> 1;
				lu = u;
				u = uvg;
				uvg = (lv + v)>> 1;
				lv = v;
				v = uvg;
			}
			YUVTORGB(y,u,v,r,g,b);
			
			if ( transparency ) {
				uchar dr,dg,db;
				pixel = *dst;
				PIXEL16TORGB(pixel,dr,dg,db);
				r = TransparencyBlend(transparency,r,dr);
				g = TransparencyBlend(transparency,g,dg);
				b = TransparencyBlend(transparency,b,db);
			}
			RGBTOPIXEL16(r,g,b,*dst);
			dst++;
		}
		dst += dstBump;
		src += srcBump;
		lsrc += srcBump;
		nsrc += srcBump;
		height--;
		firstLine = 0;
	}
}



 void
ScaleBlitYUYVto16(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	uchar				r,g,b;
	short				y1,y2,u,v,uvg,lu,lv;
	ulong				pixel;
	int					uvphase = 0;
	int					uvInitialPhase = 0;
	
	src += (p.src.leftBits >> 4) & 1;	// advance to ragged word boundary (src is a ushort*)
	dst = (uchar*)( (ushort *)dst + ((p.dst.leftBits >> 4) & 1));

	if ( ((ulong)src) & 2 )	
		uvInitialPhase++;


	srcBump = (p.src.bump - pixelCount * 2 - 2) / 2;
	dstBump = p.dst.bump - pixelCount * 2 - 2;

	if (p.src.device->transparentColor & kTransparent)
	{
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				lu = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++, uvphase++)
				{	
					if (*(uchar *)src != kTransparentY) 			
					{
						pixel = *(long *)src;
						PIXELS16TOYUYV(pixel, y1, u, y2, v);
						src++;
						if ( (uvphase & 1) ) 
						{
							uvg = u;
							u = v;
							v = uvg;
						}
						if ( lu == -1 )
						{
							lu = u;
							lv = v;
						}
						uvg = (lu + u)>> 1;
						lu = u;
						u = uvg;
						uvg = (lv + v)>> 1;
						lv = v;
						v = uvg;
						PREPUV(u,v,uvg);
						DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
						RGBTOPIXEL16(r,g,b,*(short *)dst);
						dst += 2;
					}
					else 
					{
						lu = -1;
						src++;
						dst += 2;
					}
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				lu = -1;
				uvphase = uvInitialPhase;
				uchar dr,dg,db;
				for (long i=0; i <= pixelCount; i++,uvphase++)
				{	
					if (*(uchar *)src != kTransparentY)
					{
						pixel = *(long *)src;
						
						PIXELS16TOYUYV(pixel, y1, u, y2, v);
						src++;
						if ( (uvphase & 1 ) ) 
						{
							uvg = u;
							u = v;
							v = uvg;
						}
						if ( lu == -1 )
						{
							lu = u;
							lv = v;
						}
						uvg = (lu + u)>> 1;
						lu = u;
						u = uvg;
						uvg = (lv + v)>> 1;
						lv = v;
						v = uvg;
						PREPUV(u,v,uvg);
						DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
						pixel = *(ushort *)dst;
						PIXEL16TORGB(pixel,dr,dg,db);
						r = TransparencyBlend(transparency,r,dr);
						g = TransparencyBlend(transparency,g,dg);
						b = TransparencyBlend(transparency,b,db);
						RGBTOPIXEL16(r,g,b,*(short *)dst);
						dst += 2;
					}
					else
					{	
						src++;
						dst += 2;
						lu = -1;
					}
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
	else
	{
		if ( transparency == kNotTransparent)
			while (height > 0)
			{
				uvphase = uvInitialPhase;
				if ( (uvphase & 1) )
				{	
					lv = *(((uchar *)src) + 1);
					lu = *(((uchar *)src) + 3);
				}
				else 
				{
					lu = *(((uchar *)src) + 1);
					lv = *(((uchar *)src) + 3);
				}
				for (long i=0; i <= pixelCount; i++,uvphase++) 
				{
					pixel = *(long *)src++;
					PIXELS16TOYUYV(pixel, y1, u, y2, v);
#if	0

					if ( !(i & 1) ) 
					{
						if ( (uvphase & 1) ) 
						{
							lu = v;
							lv = u;
						}
						else
						{
							lu = u;
							lv = v;
						}
						PREPUV(lu,lv,uvg);
					}
					DEBUGYUVPTORGB(y1,lu,lv,uvg,r,g,b);
#else
					if ( (uvphase & 1) ) 
					{
						uvg = (lu + v)>> 1;
						lu = v;
						uchar t = (lv + u) >> 1;
						lv = u;
						u = uvg;
						v = t;
					}
					else
					{
						uvg = (lu + u)>> 1;
						lu = u;
						u = uvg;
						uvg = (lv + v)>> 1;
						lv = v;
						v = uvg;
					}
					PREPUV(u,v,uvg);
					DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
#endif
					RGBTOPIXEL16(r,g,b,*(short *)dst);
					dst += 2;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else 
			while (height > 0)
			{
				uchar dr,dg,db;
				uvphase = uvInitialPhase;
				if ( (uvphase & 1) )
				{	
					lv = *(((uchar *)src) + 1);
					lu = *(((uchar *)src) + 3);
				}
				else 
				{
					lu = *(((uchar *)src) + 1);
					lv = *(((uchar *)src) + 3);
				}
				for (long i=0; i <= pixelCount; i++,uvphase++)
				{
					pixel = *(long *)src++;
					PIXELS16TOYUYV(pixel, y1, u, y2, v);
					if ((uvphase & 1) ) 
					{
						uvg = u;
						u = v;
						v = uvg;
					}
					uvg = (lu + u)>> 1;
					lu = u;
					u = uvg;
					uvg = (lv + v)>> 1;
					lv = v;
					v = uvg;
					PREPUV(u,v,uvg);
					DEBUGYUVPTORGB(y1,u,v,uvg,r,g,b);
					pixel = *(ushort *)dst;
					PIXEL16TORGB(pixel,dr,dg,db);
					r = TransparencyBlend(transparency,r,dr);
					g = TransparencyBlend(transparency,g,dg);
					b = TransparencyBlend(transparency,b,db);
					RGBTOPIXEL16(r,g,b,*(short *)dst);
					dst += 2;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}

}



 void
ScaleBlit16toYUYV(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register ushort		pixel;
	uchar 				A = transparency;
	uchar 				oneMinusA = kFullyTransparent-A;
	int					uvphase;
	int					uvInitialPhase = 0;
	uchar				r,g,b;
	short				y,u,v,lu,lv,t;
	
	PostulateFinal(false);

	src += (p.src.leftBits >> 4) & 1;	// advance to ragged word boundary (src is a ushort*)
	dst += ((p.dst.leftBits >> 4) & 1)*2;	// advance to ragged word boundary (dst is a ushort*)

	uvInitialPhase += (p.dst.leftBits >> 4) & 1;
	
	srcBump = (p.src.bump - pixelCount * 2 - 2) / 2;
	dstBump = (p.dst.bump - pixelCount * 2 - 2) ;
	
	while (height > 0)
	{
		uvphase = uvInitialPhase;
		lu = lv = -1;
		for (long i=0; i <= pixelCount; i++,uvphase++) 
		{
			pixel = *src++;
			PIXEL16TORGB(pixel,r,g,b);
			RGBTOYUV(r,g,b,y,u,v);
			if ( lu == -1 )
				lu = u;
			if ( lv == -1 )
				lv = v;
			t = (v + lv) >> 1;
			lv = v;
			v = t;
			t = (u + lu) >>1;
			lu = u;
			u = t;
			*dst++ = TransparencyBlendY(transparency,y,*dst);
			*dst++ = (uvphase & 1 ) ? TransparencyBlend(transparency,v,*dst) : TransparencyBlend(transparency,u,*dst);
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}


// cheesy loop just for thumbnails

 void
ScaleBlit32toYUYV(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register uchar*		src = (uchar*)p.src.base;
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	int 				uvphase = 0;
	int 				uvInitialPhase = 0;
	ulong 				pixel;
	short				y,u,v,t,lu,lv;
	uchar				r,g,b;
	
	
	PostulateFinal(false);
	src += ((p.src.leftBits >> 5) & 1) * 4;	// advance to ragged word boundary
	dst += ((p.dst.leftBits >> 4) & 1) *2;	// advance to ragged word boundary

	if ( (p.dst.leftBits >> 4) & 1 )
		uvInitialPhase++;
	
	srcBump = (p.src.bump - pixelCount * 4 - 4);
	dstBump = (p.dst.bump - pixelCount * 2 - 2);
	
	while (height > 0)
	{
		lv = -1;
		uvphase = uvInitialPhase;
		for (long i=0; i <= pixelCount; i++, uvphase++)
		{	
			pixel = *(long *)src;
			src += 4;
			PIXEL32TORGB(pixel,r,g,b);
			RGBTOYUV(r,g,b,y,u,v);
			if ( lv == -1 )	
			{
				lv = v;
				lu = u;
			}
			t = (u+lu)>>1;
			lu = u;
			u = t;
			t = (v+lv)>>1;
			lv = v;
			v = t;
			if ( transparency != kNotTransparent  ) 
			{
				*dst++ = TransparencyBlendY(transparency,y,*dst);
				if (  (uvphase & 1 )  )
					*dst++ = TransparencyBlend(transparency,v,*dst);
				else
					*dst++ = TransparencyBlend(transparency,u,*dst);
			}
			else 
			{
				*dst++ = y;
				*dst++  = (uvphase & 1 ) ? v : u;
			}
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}

 void
ScaleBlit4to16(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	Byte*				src = (Byte*)p.src.base;
	short*				dst = (short*)p.dst.base;
	register short*		dp;
	register const Byte*		cTable = GetCLUT24(p.src.device);
	register uchar		*sp;
	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;
	register long		srcBump;
	register long		dstBump;
	uchar				index,index2;
	short				transIndex = -1;
	short				r,g,b,dr,dg,db;
	short				pixel;
	long				dstInc = p.src.xInc;		// do dst instead it's easier
	Boolean				oddStart;
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged byte boundary

	srcBump = p.src.bump;
	dstBump = p.dst.bump>>1;
	oddStart = (p.src.leftBits >> 2) & 1;
	

	if ( (p.src.device->transparentColor & kTransparent) != 0 )
		transIndex = (Byte)p.src.device->transparentColor;

	
	// loop assumes 1 cycle short mul ( no special case for transparency == 0 or 255 )
	
	
	while (height > 0)
	{
		Boolean skipFirst = oddStart;
		dp = dst;
		if ( dstInc < 0 )
			dp += pixelCount-1;
		sp = src;

		for (long i=0; i < pixelCount; i++)
		{
			index2 = *sp++;
			if ( !skipFirst ) 
			{
				index = (index2 >> 4);
				if (index != transIndex) 
				{
					const Byte* rgb = &cTable[index * 3];
					r = *rgb++;
					g = *rgb++;
					b = *rgb++;
					pixel = *dp;
					PIXEL16TORGB(pixel,dr,dg,db);
					r = TransparencyBlend(transparency,r,dr);
					g = TransparencyBlend(transparency,g,dg);
					b = TransparencyBlend(transparency,b,db);
					RGBTOPIXEL16(r,g,b,*dp);
				}
				dp += dstInc;
				if ( ++i == pixelCount )
					break;
			}
			skipFirst = false;
				
			// second pixel
			index = index2  & 0xf;
			if (index != transIndex) 
			{
				const Byte* rgb = &cTable[index * 3];
				r = *rgb++;
				g = *rgb++;
				b = *rgb++;
				pixel = *dp;
				PIXEL16TORGB(pixel,dr,dg,db);
				r = TransparencyBlend(transparency,r,dr);
				g = TransparencyBlend(transparency,g,dg);
				b = TransparencyBlend(transparency,b,db);
				RGBTOPIXEL16(r,g,b,*dp);
			}
			dp += dstInc;
		}
		dst += dstBump;
		src += srcBump;
		height--;
	}
}






 void
ScaleBlit8to16(BlitParams2& p, ulong transparency)
{
	static const Boolean	newCode = true;
	PostulateFinal(false);	// newCode or not?

	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register short		color;
	long				srcInc = p.src.xInc;
	char*				debugBase = (char*)p.dst.base;
	char*				debugMax = debugBase + (p.dst.r.bottom - p.dst.r.top)*p.dst.device->rowBytes;
	
	src += (p.src.leftBits >> 3) & 3;	// advance to ragged byte boundary
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)

	if ( srcInc < 0 )
		srcBump = p.src.bump + 1;
	else
		srcBump = p.src.bump - pixelCount - 1;

	dstBump = (p.dst.bump - pixelCount * 2 - 2) / 2;
	Assert(dstBump >= 0);
		
	if (p.src.device->transparentColor & kTransparent)
	{
		register Byte transIndex = (Byte)p.src.device->transparentColor;
		if ( transparency == kNotTransparent)
		{
			register const ushort* cTable16 = GetCLUT16(p.src.device);
			while (height > 0)
			{
				if ( srcInc < 0 )
					src += pixelCount;
				for (long i=0; i <= pixelCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (index != transIndex)
						*dst = cTable16[index];
					dst += 1;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		}
		else	// transparent case
		{
			register const Byte*	cTable24 = GetCLUT24(p.src.device);
			while (height > 0)
			{
				uchar r,g,b;
				if ( srcInc < 0 )
					src += pixelCount;
				for (long i=0; i <= pixelCount; i++)
				{	
					Byte  index = *src;
					src += srcInc;
					if (index != transIndex)
					{
						const Byte* rgb = &cTable24[index * 3];
						
						color = *dst;
						PIXEL16TORGB(color, r, g, b);
						r = TransparencyBlend(transparency,*rgb,r);
						rgb++;
						g = TransparencyBlend(transparency,*rgb,g);
						rgb++;
						b = TransparencyBlend(transparency,*rgb,b);
						rgb++;
						RGBTOPIXEL16(r,g,b,color);
						*dst++ = color;
					}
					else
						dst += 1;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		}

	}
	else
	{
		if ( transparency == kNotTransparent)
		{
			if (newCode && (srcInc > 0) )
			{
				register const ushort*	cTable16 = GetCLUT16(p.src.device);
				
				while (height > 0)
				{
					Assert(debugBase <= (char*)dst && (char*)dst < debugMax);
					ulong pixels = pixelCount + 1;
					
					if (((ulong)dst & 0x2) != 0)
					{
						*(ushort*)dst = cTable16[*src++];
						dst++;
						Assert(((ulong)dst & 0x3) == 0);
						pixels--;
					}
					Assert(((ulong)dst & 0x3) == 0);
					long quads = pixels>>2;
					
					for (long i = 0; i < quads; i++)
					{
						*(ulong*)dst = (cTable16[*src]<<16) + cTable16[src[1]];
						src += 2; dst += 2;
						*(ulong*)dst = (cTable16[*src]<<16) + cTable16[src[1]];
						src += 2; dst += 2;
					}
						
					// clean up the odd ones
					switch (pixels & 0x3)
					{
						case 3: *dst++ = cTable16[*src++];
						case 2: *dst++ = cTable16[*src++];
						case 1: *dst++ = cTable16[*src++];
					}
						
					Assert(debugBase <= (char*)dst && (char*)dst <= debugMax);

					dst += dstBump;
					src += srcBump;
					height--;
				}
			}
			else
			{
				register const ushort*	cTable16 = GetCLUT16(p.src.device);
				while (height > 0)
				{
					if ( srcInc < 0 )
						src += pixelCount;
					for (long i = 0; i <= pixelCount; i++)
					{
						*dst++ = cTable16[*src];
						src += srcInc;
					}
					dst += dstBump;
					src += srcBump;
					height--;
				}
			}
		}
		else	// transparency
		{
			register const Byte*		cTable24 = GetCLUT24(p.src.device);
			while (height > 0)
			{
				uchar r,g,b;
				if ( srcInc < 0 )
					src += pixelCount;
				for (long i=0; i <= pixelCount; i++)
				{
					const Byte* rgb = &cTable24[*src * 3];
					src += srcInc;

					color = *dst;
					PIXEL16TORGB(color, r, g, b);
					r = TransparencyBlend(transparency,*rgb,r);
					rgb++;
					g = TransparencyBlend(transparency,*rgb,g);
					rgb++;
					b = TransparencyBlend(transparency,*rgb,b);
					rgb++;
					RGBTOPIXEL16(r,g,b,*dst);
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		}
	}
}

void
ScaleBlit16to16(BlitParams2& p, ulong transparency)
{
	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register ushort*	dst = (ushort*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	register ushort		pixel,dpixel;
	
	src += (p.src.leftBits >> 4) & 1;	// advance to ragged word boundary (src is a ushort*)
	dst += (p.dst.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)

	srcBump = (p.src.bump - pixelCount * 2 - 2) / 2;
	dstBump = (p.dst.bump - pixelCount * 2 - 2) / 2;
	
	if (p.src.device->transparentColor & kTransparent)
	{
		register ulong transIndex = (Byte)p.src.device->transparentColor & kTransColorMask;
		
		if (transparency == kNotTransparent)
		
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex)
						*dst = pixel;
					dst++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				uchar dr,dg,db,sr,sg,sb;
				for (long i=0; i <= pixelCount; i++)
				{	
					pixel = *src++;
					if (pixel != transIndex)
					{
						dpixel = *dst;
						PIXEL16TORGB(dpixel, dr, dg, db);
						PIXEL16TORGB(pixel, sr, sg, sb);
						dr = TransparencyBlend(transparency,sr,dr);
						dg = TransparencyBlend(transparency,sg,dg);
						db = TransparencyBlend(transparency,sb,db);
						RGBTOPIXEL16(dr,dg,db,*dst);
					}
					dst++;
				}
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
				for (long i=0; i <= pixelCount; i++)
					*dst++ = *src++;
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else if (transparency != 128)
			while (height > 0)
			{
				uchar dr,dg,db,sr,sg,sb;
				for (long i=0; i <= pixelCount; i++)
				{
					pixel = *src++;
					dpixel = *dst;
					PIXEL16TORGB(dpixel, dr, dg, db);
					PIXEL16TORGB(pixel, sr, sg, sb);
					dr = TransparencyBlend(transparency,sr,dr);
					dg = TransparencyBlend(transparency,sr,dr);
					db = TransparencyBlend(transparency,sr,dr);
					RGBTOPIXEL16(dr,dg,db,*dst);
					dst++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else if (((ulong)src & 3) == ((ulong)dst & 3) == 0 == (pixelCount & 1))	// long word boundaries, even pixel count

		{
			pixelCount >>= 1;
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{
					ulong	srcLow = *(ulong *)src & kLowBitsMask5;
					ulong	dstLow = *(ulong *)dst & kLowBitsMask5;
					
					*(ulong *)dst = ((*(ulong *)dst & ~kLowBitsMask5) >> 1) + ((*(ulong *)src & ~kLowBitsMask5) >> 1) + (srcLow & dstLow);
					dst += 2; src += 2;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		}
		else
			while (height > 0)
			{
				for (long i=0; i <= pixelCount; i++)
				{
					ushort	srcLow = *src & kLowBitsMask5;
					ushort	dstLow = *dst & kLowBitsMask5;
					
					*dst = ((*dst & ~kLowBitsMask5) >> 1) + ((*src++ & ~kLowBitsMask5) >> 1) + (srcLow & dstLow);
					dst++;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}
}



 void
ScaleBlitYUYVtoYUV(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register ushort*	src = (ushort*)p.src.base;
	register uchar*	dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 5) - (p.dst.leftBits >> 5) - 1;
	register long		longCount = p.dst.longCount;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	short				y1,y2,y,u,v,lu,lv,uvg;
	ulong				pixel;
	uchar				nt;
	int					uvphase = 0;
	int					uvInitialPhase = 0;
	Boolean				srcHasTransparent = (p.src.device->transparentColor & kTransparent) != 0;
	
	src += (p.src.leftBits >> 4) & 1;	// advance to ragged word boundary (src is a ushort*)

	if ( ( p.src.leftBits>>4) & 1 )
		uvInitialPhase++;
	srcBump = (p.src.bump - pixelCount * 2 - 2) / 2;
	dstBump = p.dst.bump - pixelCount * 4 - 4;

	nt = kFullyTransparent - (uchar)transparency;
	{
		if (transparency == kNotTransparent)
			while (height > 0)
			{
				lu = -1;
				lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++, uvphase++)
				{	
					if (!srcHasTransparent || *(uchar *)src != kTransparentY) 	
					{
						pixel = *(long *)src;
						PIXELS16TOYUYV(pixel, y1, u, y2, v);
						src++;
						if ( (uvphase & 1) ) 
						{
							uvg = u;
							u = v;
							v = uvg;
						}
						if ( lu == -1 ) 
							lu = u;
						if ( lv == -1 )
							lv = v;
						uvg = (lu + u)>> 1;
						lu = u;
						u = uvg;
						uvg = (lv + v)>> 1;
						lv = v;
						v = uvg;
						YUVTOPIXEL32(y1,u,v,*(long *)dst);
						dst += 4;
					}
					else 
					{
						lu = -1;
						lv = -1;
						src++;
						dst += 4;
					}
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				lu = -1;
				lv = -1;
				uvphase = uvInitialPhase;
				short dy,du,dv;
				for (long i=0; i <= pixelCount; i++,uvphase++)
				{	
					if (!srcHasTransparent || *(uchar *)src != kTransparentY)
					{
						pixel = *(long *)src;
						
						PIXELS16TOYUYV(pixel, y1, u, y2, v);
						src++;
						if ( (uvphase & 1 ) ) 
						{
							uvg = u;
							u = v;
							v = uvg;
						}
						if ( lu == -1 )
							lu = u;
						if ( lv == -1 )
							lv = v;
						uvg = (lu + u)>> 1;
						lu = u;
						u = uvg;
						uvg = (lv + v)>> 1;
						lv = v;
						v = uvg;
						y = y1;
						pixel = *(long *)dst;
						PIXEL32TOYUV(pixel,dy,du,dv);
						y = TransparencyBlendY(transparency,y,dy);
						u = TransparencyBlend(transparency,u,du);
						v = TransparencyBlend(transparency,v,dv);
						YUVTOPIXEL32(y,u,v,*(long *)dst);
						dst += 4;
					}
					else
					{	
						src++;
						dst += 4;
						lu = -1;
						lv = -1;
					}
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
	}

}


 void
ScaleBlitYUVtoYUYV(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register uchar*		src = (uchar*)p.src.base;
	register uchar*		dst = (uchar*)p.dst.base;

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4) - 1;
	register long		height = p.dst.r.bottom - p.dst.r.top;

	register long		srcBump;
	register long		dstBump;
	Boolean				alignedUV = true;
	int 				uvphase = 0;
	int 				uvInitialPhase = 0;
	ulong 				pixel;
	short				y,u,v,t,lu,lv;
	
	
	src += ((p.src.leftBits >> 5) & 1) * 4;	// advance to ragged word boundary
	dst += ((p.dst.leftBits >> 4) & 1) *2;	// advance to ragged word boundary

	if ( (p.dst.leftBits >> 4) & 1 )
		uvInitialPhase++;
	
	srcBump = (p.src.bump - pixelCount * 4 - 4);
	dstBump = (p.dst.bump - pixelCount * 2 - 2);
	
	if (p.src.device->transparentColor & kTransparent)
	{
		if (transparency == kNotTransparent)
			while (height > 0)
			{
				lu = lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++,uvphase++)
				{	
					pixel = *(long *)src;
					src += 4;
					PIXEL32TOYUV(pixel,y,u,v);
					if (y != kTransparentY)
					{
						if ( lv == -1 )	
							lv = v;
						if ( lu == -1 )	
							lu = u;
						t = (u+lu)>>1;
						lu = u;
						u = t;
						t = (v+lv)>>1;
						lv = v;
						v = t;
						*(ushort*)dst = (y << 8) + ((uvphase & 1 ) ? v : u);
					}
					else
						lu = lv = -1;
					dst += 2;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		else	// transparency case

			while (height > 0)
			{
				lu = lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++, uvphase++)
				{	
					pixel = *(long *)src;
					src += 4;
					PIXEL32TOYUV(pixel,y,u,v);
					if (y != kTransparentY)
					{	
						if ( lv == -1 )	
							lv = v;
						if ( lu == -1 )	
							lu = u;
						t = (u+lu)>>1;
						lu = u;
						u = t;
						t = (v+lv)>>1;
						lv = v;
						v = t;
						y = TransparencyBlendY(transparency,y,*dst);
						t = (uvphase & 1 ) ? v : u;
						t = TransparencyBlend(transparency,t,dst[1]);
						*(ushort*)dst = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(t);
					}
					else
						lv = lu = -1;
					dst += 2;
				}
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
				lu = lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++, uvphase++) 
				{
					pixel = *(long *)src;
					src += 4;
					PIXEL32TOYUV(pixel,y,u,v);
					if ( lv == -1 )	
						lv = v;
					if ( lu == -1 )	
						lu = u;
					t = (u+lu)>>1;
					lu = u;
					u = t;
					t = (v+lv)>>1;
					lv = v;
					v = t;
					*(ushort*)dst = (y << 8) + ((uvphase & 1 ) ? v : u);
					dst += 2;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		}
		else
		{
			while (height > 0)
			{
				lu = lv = -1;
				uvphase = uvInitialPhase;
				for (long i=0; i <= pixelCount; i++,uvphase++)
				{
					pixel = *(long *)src;
					src += 4;
					PIXEL32TOYUV(pixel,y,u,v);
					if ( lv == -1 )	
						lv = v;
					if ( lu == -1 )	
						lu = u;
					t = (u+lu)>>1;
					lu = u;
					u = t;
					t = (v+lv)>>1;
					lv = v;
					v = t;
					y = TransparencyBlendY(transparency,y,*dst);
					t = (uvphase & 1 ) ? v : u;
					t = TransparencyBlend(transparency,t,dst[1]);
					*(ushort*)dst = (CLIPLUMINANCE(y) << 8) + CLIPCHROMA(t);
					dst += 2;
				}
				dst += dstBump;
				src += srcBump;
				height--;
			}
		}
	}
}




void
PaintBlitBlend32(BlitParams& p, ulong transparency)
{
	// important inner loop variables (consider reading sequentially)
	register ulong*		dst			= (ulong*)p.base;
	register long		rowBump		= p.bump;
	register long		longCount	= p.longCount;
	register ulong		height		= p.r.bottom - p.r.top;
	register Color		color		= p.color;
	register ulong pixel;
	long i;
	ushort	red   = (short)((color>>16) & 0xff) ;
	ushort	green = (short)((color>>8)  & 0xff);
	ushort	blue  = (short)( color      & 0xff);

	rowBump >>= 2;
	rowBump -= longCount;
	
	if ( transparency == kHalfTransparent ) {
		ulong	sp,spm;
		RGBTOPIXEL32(red,green,blue,sp);
		sp = (sp<<16) | sp;
		spm = sp & kLowBitsMask8;
		sp = ((sp & ~kLowBitsMask8) >> 1);
		while (height > 0) {
			for (i=0; i < longCount; i++) {
				ulong  dp = *dst;
				ulong dpm =  dp & kLowBitsMask8;
				*dst++ = ((dp & ~kLowBitsMask8) >> 1) + sp + (spm & dpm);
			}
			dst += rowBump;
			height--;
		}
	} else {
		register short a = transparency;
		register ushort	r,g,b;
		red   *= (short)(kFullyTransparent-a);
		green *=(short)(kFullyTransparent-a);
		blue  *= (short)(kFullyTransparent-a);
		while (height > 0) {
			for (i=0; i < longCount; i++) {
				pixel = *dst;
				PIXEL32TORGB(pixel,r,g,b);
				r = (r * a + red)   >> 8;
				g = (g * a + green) >> 8;
				b = (b * a + blue)  >> 8;
				RGBTOPIXEL32(r,g,b,pixel);
				*dst++ = pixel;
			}
			dst += rowBump;
			height--;
		}
	}
}



void
PaintBlitBlend16(BlitParams& p, ulong transparency, Color color)
{
	// important inner loop variables (consider reading sequentially)
	register ushort*	dst			= (ushort*)p.base;
	register long		rowBump		= p.bump;
	register long		pixelCount = (p.rightBits >> 4) - (p.leftBits >> 4);
	register ulong		height		= p.r.bottom - p.r.top;
	register short a = transparency;
	long		i;
	
	dst += (p.leftBits >> 4) & 1;	// advance to ragged word boundary (dst is a ushort*)
	rowBump >>= 1;
	rowBump -= pixelCount;
	
	ushort	red   = ((color>>16) & 0xff);
	ushort	green = ((color>>8)  & 0xff);
	ushort	blue  = ( color      & 0xff);
	uchar	r,g,b;
	
	if ( transparency == kHalfTransparent ) {
		ulong	sp,spm;
		RGBTOPIXEL16(red,green,blue,sp);
		sp = (sp<<16) | sp;
		spm = sp & kLowBitsMask5;
		sp = ((sp & ~kLowBitsMask5) >> 1);
		while (height > 0) {
			for (i=0; i < pixelCount>>1; i++) {
				ulong  dp = *(ulong *)dst;
				ulong dpm =  dp & kLowBitsMask5;
				*(ulong *)dst = ((dp & ~kLowBitsMask5) >> 1) + sp + (spm & dpm);
				dst += 2;
			}
			dst += rowBump;
			height--;
		}
	} else {
		red   *= (short)(kFullyTransparent-a);
		green *= (short)(kFullyTransparent-a);
		blue  *= (short)(kFullyTransparent-a);
		while (height > 0) {
			for (i=0; i < pixelCount; i++) 	{
				ushort pixel = *dst;
				PIXEL16TORGB(pixel, r, g, b);
				*dst++ = (((r*a + red  )>>1) & 0x7C00) 
					   + (((g*a + green)>>6) & 0x03E0) 
					   + (((b*a + blue)>>11) & 0x001F);
			}
			dst += rowBump;
			height--;
		}
	}
}


void
PaintBlitBlendGray8(BlitParams& p, ulong transparency, Color color)
{
	// important inner loop variables (consider reading sequentially)
	register uchar*		dst			= (uchar*)p.base;
	register long		rowBump		= p.bump;
	register long		pixelCount = (p.rightBits >> 3) - (p.leftBits >> 3) - 1;
	register ulong		height		= p.r.bottom - p.r.top;

	dst += (p.leftBits >> 3) & 3;	// advance to ragged byte boundary

	rowBump -= pixelCount + 1;
	ushort	gray  = (color & 0xff) * (kFullyTransparent-transparency);
	while (height > 0) {
		for (long i=0; i <= pixelCount; i++)
			*dst++ = (*dst * transparency + gray) >> 8;
		dst = dst + rowBump;
		height--;
	}
}




void
PaintBlitBlendYUV(BlitParams& p, ulong transparency)
{
	// important inner loop variables (consider reading sequentially)
	register uchar*		dst			= (uchar*)p.base;
	register long		rowBump		= p.bump;
	register long		longCount	= p.longCount;
	register ulong		height		= p.r.bottom - p.r.top;
	register Color		color		= p.color;
	register short a = transparency;

	rowBump -= longCount * 4 + 4;
	ushort	y  = (short)((color>>24) & 0xff) * (short)(kFullyTransparent-a);
	ushort	u = (short)((color>>16)  & 0xff) * (short)(kFullyTransparent-a);
	ushort	v  = (short)( color      & 0xff) * (short)(kFullyTransparent-a);
	while (height > 0) {
		for (long i=0; i <= longCount; i++) {
			*dst++ = (*dst * a + y) >> 8;
			*dst++ = (*dst * a + u) >> 8;
			dst++;
			*dst++ = (*dst * a + v) >> 8;
		}
		dst = dst + rowBump;
		height--;
	}
}
	
	


// ===========================================================================
//	static functions
// ===========================================================================

#ifdef FOR_MAC


static void CopyBoundsToMacPort(GrafPtr port, const Rectangle& srcBounds);
static void MakeMacDevice(BitMapDevice& device);
static void MakePixMap(PixMap& pixMap, Byte* base, ulong depth, ulong rowBytes, Rect& bounds, CLUT* colorTable);
static void SetQDFont(XFont font);
static void SetQDColor(Color color);
static void SetQDBackColor(Color color);
static void SetQDDevice(BitMapDevice& device, const Rectangle* clip);
static void UnMakeMacDevice(BitMapDevice& device);
static void UnSetQDDevice(BitMapDevice& device);

void SetRectangle(Rectangle& r, const Rect& mac)
{
	r.left = mac.left;
	r.top = mac.top;
	r.right = mac.right;
	r.bottom = mac.bottom;
}

void MacRect(const Rectangle& r, Rect& mac)
{
	mac.left = r.left;
	mac.top = r.top;
	mac.right = r.right;
	mac.bottom = r.bottom;
}


static void CopyBoundsToMacPort(GrafPtr port, const Rectangle& srcBounds)
{
	
	Rect srcRect;
	Rect clipRect;

	GrafPtr savePort;
	GetPort(&savePort);
	
	SetPort(port);
	MacRect(srcBounds, clipRect);
	MacRect(gScreenDevice.bounds, srcRect);

	ClipRect(&clipRect);
	ForeColor(blackColor);
	BackColor(whiteColor);
	CopyBits((BitMap *)*gScreenDevice.pixmap, &port->portBits, &srcRect, &srcRect, 0, nil);

	ClipRect(&port->portRect);
	SetPort(savePort);
}


void UpdateMacPort()
{
	extern ulong		gPendingAnimationDissolvePeriod;
	
	if (gPendingAnimationDissolvePeriod != 0)
		return;
	DrawingComplete(gScreenDevice.bounds, nil);
	UpdateScreenBits();
}

#define kUnusedStyles	(outline|condense|extend)

static void SetQDFont(XFont font)
{
	short 	theFont = font>>16;
	short	theStyle = (font>>8) & 0xff;
	short	theSize = font & 0xff;
	
	TextFont(theFont);
	TextSize(theSize);
	Assert((theStyle & kUnusedStyles) == 0);	// should never see funny style fonts
	TextFace(theStyle);
}

static void SetQDColor(Color color)
{
	RGBColor	rgb = { (color>>8) & 0xFF00, color & 0xFF00, (color<<8) & 0xFF00 };
	
	RGBForeColor(&rgb);
}

static void SetQDBackColor(Color color)
{
	RGBColor	rgb = { (color>>8) & 0xFF00, color & 0xFF00, (color<<8) & 0xFF00 };
	
	RGBBackColor(&rgb);
}

static void SetQDDevice(BitMapDevice& device, const Rectangle* clip)
{
	Assert(device.saveDevice == nil);
	GetGWorld(&device.savePort, &device.saveDevice);
	SetGWorld(&device.cport, device.gdevice);
	if (clip == nil)
		clip = &device.bounds;
	Rect r;
	MacRect(*clip, r);
	ClipRect(&r);
}

static void UnSetQDDevice(BitMapDevice& device)
{
	SetGWorld(device.savePort, device.saveDevice);
	device.saveDevice = nil;
}




static void MakePixMap(PixMap& pixMap, Byte* base, ulong depth, ulong rowBytes, Rect& bounds, CLUT* colorTable)
	{
	Boolean		grayscale = false;
	if (depth == 40) {
		grayscale = true;
		depth = 8;
	}
	pixMap.baseAddr = (char*)base;
#ifdef FIDO_INTERCEPT
	Assert((rowBytes & 3) == 0);
#endif
	pixMap.rowBytes = rowBytes | 0x8000;
	pixMap.pixelType = (depth <= 8) ? 0 : 16;
	pixMap.pixelSize = depth;
	pixMap.bounds = bounds;
	DisposeCTable(pixMap.pmTable);
	pixMap.pmTable = (pixMap.pixelSize <= 8) ? GetCTable(depth) : 0;

	if (colorTable && colorTable->version == kRGB24)
	{
		int i;
		short* p = (short *)&(**pixMap.pmTable).ctTable[0];
		uchar* rgb24 = (uchar*)(&colorTable->data);

		(**pixMap.pmTable).ctSeed = GetCTSeed();
		for (i=0; i<1<<depth; i++)
		{
			*p++ = i; 
			*p++ = *rgb24++ << 8;
			*p++ = *rgb24++ << 8;
			*p++ = *rgb24++ << 8;
		}
	}
	else if (colorTable && colorTable->version == kYUV24)
	{
		int i;
		short* p = (short *)&(**pixMap.pmTable).ctTable[0];
		uchar* rgb24 = (uchar*)(&colorTable->data);

		(**pixMap.pmTable).ctSeed = GetCTSeed();
		for (i=0; i<1<<depth; i++)
		{
			uchar r,g,b;
			short y,u,v;
			
			y = *rgb24++;
			u = *rgb24++;
			v = *rgb24++;
			yuvtorgb(y,u,v,&r,&g,&b);
			*p++ = i; 
			*p++ = r << 8;
			*p++ = g << 8;
			*p++ = b << 8;
		}
	}
	else if (colorTable && colorTable->version == kRGB15)
	{
		int i;
		short* p = (short *)&(**pixMap.pmTable).ctTable[0];
		short* rgb15 = (short*)(&colorTable->data);

		(**pixMap.pmTable).ctSeed = GetCTSeed();
		for (i=0; i<1<<depth; i++)
		{
			uchar r, g, b;
			ushort pixel = *rgb15++;
			
			PIXEL16TORGB(pixel, r, g, b);
			*p++ = i; 
			*p++ = r << 8;
			*p++ = g << 8;
			*p++ = b << 8;
		}
	}
	else if (grayscale)
	{
		int i;
		short* p = (short *)&(**pixMap.pmTable).ctTable[0];

		(**pixMap.pmTable).ctSeed = GetCTSeed();
		for (i=0; i<1<<depth; i++)
		{
			*p++ = i;
			*p++ = (i << 8) + i;
			*p++ = (i << 8) + i;
			*p++ = (i << 8) + i;
		}
	}

	pixMap.pmVersion = 0;
	pixMap.packType = 0;
	pixMap.packSize = 0;
	pixMap.cmpCount = (depth <= 8) ? 1 : 3;
	switch (depth)
	{
		case 32: pixMap.cmpSize = 8; break;
		case 16: pixMap.cmpSize = 5; break;
		default: pixMap.cmpSize = 0; break;
	}
	pixMap.planeBytes = 0;
}

static void UnMakeMacDevice(BitMapDevice& device)
{
	if (device.pixmap != nil)
	{	DisposePixMap(device.pixmap);
		device.pixmap = nil;
	}
	if (device.gdevice != nil)
	{	DisposeGDevice(device.gdevice);
		device.gdevice = nil;
	}
}

static void MakeMacDevice(BitMapDevice& device) {
	Rect r;
	GDHandle saveDevice;
	CGrafPtr savePort;
	
	GetGWorld(&savePort, &saveDevice);
	SetRect(&r, device.bounds.left, device.bounds.top, device.bounds.right, device.bounds.bottom);
	device.pixmap = NewPixMap();
	device.gdevice = NewGDevice(0,-1);
	device.saveDevice = nil;
	MakePixMap(**device.pixmap, device.baseAddress, device.format != gray8Format ? device.depth : 32 + device.depth, device.rowBytes, r, device.colorTable);
	(**device.gdevice).gdType = (device.depth <= 8) ? clutType : directType;
	(**device.gdevice).gdPMap = device.pixmap;
#if defined FIDO_INTERCEPT
	device.clutType = (CLUTType) -1;
	device.clutBase = 0;
	device.textureType = (BitMapFormat) -1;
	device.textureBase = 0;
#endif
	SetGDevice(device.gdevice);
	OpenCPort(&device.cport);
	SetGWorld(savePort, saveDevice);
}
	
void
InitializeSimulatorGraphics(void) 
{
	Rectangle		screenBounds;
	
	SetRectangle(screenBounds,0, 0,GetDisplayWidth(), GetDisplayHeight());
	MakeBitMapDevice(gScreenDevice, screenBounds,
				(uchar *)gSimulator->GetScreenBaseAddress(),  
				gMacSimulator->GetScreenRowBytes(), 
				gMacSimulator->GetScreenBitMapFormat() , nil, kNoTransparentColor);
	MakeMacDevice(gScreenDevice);
	MakeBitMapDevice(gOnScreenDevice, (CGrafPtr)gMacSimulator->GetWindow());
	MakeMacDevice(gOnScreenDevice);
	gOnScreenDevice.bounds = screenBounds;
}


void
ScaleBlitVQto16(BlitParams2& p, ulong transparency)
{

	// important inner loop variables
	register Byte*		src = (Byte*)p.src.base;
	register Byte*		dst = (Byte*)p.dst.base;
	register ulong*		codeBooks = (ulong*)&p.src.device->colorTable->data[0];

	register long		pixelCount = (p.dst.rightBits >> 4) - (p.dst.leftBits >> 4);
	register long		height = p.dst.r.bottom - p.dst.r.top;

	short				index;
	int					vertMisAligned;
	int					horzMisAligned;
	long				srcInc = p.src.xInc;
	register ushort		*dpa,*dpb;
	short				transIndex = -1;
	short				y1,y2,u,v,uvg;
	ulong				pix;
	register ulong 		*cbp;
	uchar				r,g,b,dr,dg,db;
	Byte				*sp;
	
	src -= (p.src.leftBits >> 5)<<2;
	src += (p.src.leftBits >> 3);
	horzMisAligned = (p.src.leftBits >> 2) & 1;
	
	
	dst += (p.dst.leftBits >> 3) & 3;	// advance to ragged byte boundary

	vertMisAligned = (p.src.r.top & 1);
	srcInc = 1; //еее	should handle flips
	
	if ( (p.src.device->transparentColor & kTransparent) != 0)
		transIndex = (Byte)p.src.device->transparentColor;

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
				vertMisAligned = 2;
				height += 1;		// really only doing one line this time
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
		long i=pixelCount;

		if ( horzMisAligned ) {
			index = *sp;
			sp += srcInc;
			cbp = codeBooks + (index<<1);
			if ( dpa ) {
				pix = *cbp;
				PIXELS16TOYUYV(pix,y1,u,y2,v);
				PREPUV(u,v,uvg);
				YUVPTORGB(y1,u,v,uvg,r,g,b);
				RGBTOPIXEL16(r,g,b,*dpa);
				dpa++;
			}
			if (dpb ) {
				cbp++;
				pix = *cbp;
				PIXELS16TOYUYV(pix,y1,u,y2,v);
				PREPUV(u,v,uvg);
				YUVPTORGB(y1,u,v,uvg,r,g,b);
				RGBTOPIXEL16(r,g,b,*dpb);
				dpb++;
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
				if ( dpa ) 
				{
					pix = *cbp;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					PREPUV(u,v,uvg);
					YUVPTORGB(y1,u,v,uvg,r,g,b);
					if ( transparency ) {
						pix = *dpa;
						PIXEL16TORGB(pix,dr,dg,db);
						r = TransparencyBlend(transparency,r,dr);
						g = TransparencyBlend(transparency,g,dg);
						b = TransparencyBlend(transparency,b,db);
					}
					RGBTOPIXEL16(r,g,b,*dpa);
					dpa++;
					YUVPTORGB(y2,u,v,uvg,r,g,b);
					if ( transparency ) {
						pix = *dpa;
						PIXEL16TORGB(pix,dr,dg,db);
						r = TransparencyBlend(transparency,r,dr);
						g = TransparencyBlend(transparency,g,dg);
						b = TransparencyBlend(transparency,b,db);
					}
					RGBTOPIXEL16(r,g,b,*dpa);
					dpa++;
				}
				cbp++;
				if ( dpb ) 
				{
					pix = *cbp++;
					PIXELS16TOYUYV(pix,y1,u,y2,v);
					PREPUV(u,v,uvg);
					YUVPTORGB(y1,u,v,uvg,r,g,b);
					if ( transparency ) {
						pix = *dpb;
						PIXEL16TORGB(pix,dr,dg,db);
						r = TransparencyBlend(transparency,r,dr);
						g = TransparencyBlend(transparency,g,dg);
						b = TransparencyBlend(transparency,b,db);
					}
					RGBTOPIXEL16(r,g,b,*dpb);
					dpb++;
					YUVPTORGB(y2,u,v,uvg,r,g,b);
					if ( transparency ) {
						pix = *dpb;
						PIXEL16TORGB(pix,dr,dg,db);
						r = TransparencyBlend(transparency,r,dr);
						g = TransparencyBlend(transparency,g,dg);
						b = TransparencyBlend(transparency,b,db);
					}
					RGBTOPIXEL16(r,g,b,*dpb);
					dpb++;
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
				PREPUV(u,v,uvg);
				YUVPTORGB(y1,u,v,uvg,r,g,b);
				RGBTOPIXEL16(r,g,b,*dpa);
				dpa++;
			}
			if ( dpb ) {
				cbp++;
				pix = *cbp;
				PIXELS16TOYUYV(pix,y1,u,y2,v);
				PREPUV(u,v,uvg);
				YUVPTORGB(y1,u,v,uvg,r,g,b);
				RGBTOPIXEL16(r,g,b,*dpb);
				dpb++;
		}
		}		
		dst += p.dst.device->rowBytes * 2;
		src += p.src.device->rowBytes;
		height -= 2;
	}
}


void
UpdateMacScreen(Rectangle dirty,const Rectangle *clip)
{
	Assert(gMacSimulator->GetWindow() == qd.thePort);
	if ( clip )
		;
	MakeBitMapDevice(gOnScreenDevice, (CGrafPtr)gMacSimulator->GetWindow());
	if (gMacSimulator->GetUseBackBuffer() == false && !(gScreenDevice.format & 0x8300) )
		CopyBoundsToMacPort((GrafPtr)gMacSimulator->GetWindow(), dirty);
}



void FastTextBounds(BitMapDevice& device, XFont font,CharacterEncoding encoding, const char* text, ulong length, Ordinate x, Ordinate y, Rectangle* bounds)
{
	ulong width = TextMeasure(device, font, encoding, text, length);
	
	bounds->left   = x - GetFontKern(font,encoding);
	bounds->right  = x + width;
	bounds->top    = y - GetFontAscent(font,encoding);
	bounds->bottom = y + GetFontDescent(font,encoding);
}


void
CopyImageQD(const BitMapDevice& srcDevice, BitMapDevice& dstDevice,
		const Rectangle& srcBounds, const Rectangle& dstBounds, ulong , const Rectangle* clip )
{

	Rect	dstRect, srcRect, clipRect;
	RgnHandle theClip;
		
	// set up the world for the upcoming copy
	static PixMapHandle srcMap = nil;
	static PixMapHandle dstMap = nil;
	static RgnHandle clipRgn = nil;
	if (srcMap == nil)
	{ 
		srcMap = NewPixMap();
		dstMap = NewPixMap();
		clipRgn = NewRgn();
	}
	
	MacRect(srcDevice.bounds, srcRect);
	MakePixMap(**srcMap, srcDevice.baseAddress, srcDevice.depth, srcDevice.rowBytes, srcRect, srcDevice.colorTable);
//	OffsetRect(&srcRect, src.bounds.left, src.bounds.top);
	MacRect(dstDevice.bounds, dstRect);
	MakePixMap(**dstMap, dstDevice.baseAddress, dstDevice.depth, dstDevice.rowBytes, dstRect, dstDevice.colorTable);
//	OffsetRect(&dstRect, dst.bounds.left, dst.bounds.top);

	MacRect(srcBounds, srcRect);
	MacRect(dstBounds, dstRect);
	
	if (clip)
	{
		MacRect(*clip, clipRect);
		RectRgn(clipRgn, &clipRect);
		theClip = clipRgn;
	}
	else
		theClip = nil;
	
	
	SetQDDevice(dstDevice, clip);
	CopyBits((BitMap *)*srcMap, (BitMap *)*dstMap, &srcRect, &dstRect, 0, theClip);
	UnSetQDDevice(dstDevice);

}

#if 0
// Saving this for when we support alpha channels

void DrawMaskedImage(BitMapDevice& device, const BitMapDevice& mask, const BitMapDevice& source, const Rectangle& destR, ulong transparency = 0, const Rectangle* clip);

void DrawMaskedImage(BitMapDevice& device, const BitMapDevice& mask, const BitMapDevice& source, const Rectangle& destR, ulong transparency, const Rectangle* clip)
{
	if (!(device.format & 0x8300) && !(source.format & 0x8300) && !(mask.format & 0x8300) >= -8 && ( gMacSimulator->GetUseQuickdraw()))
	{
		Rect			dstRect, srcRect;
		PixMapHandle	srcPixMapHandle, maskPixMapHandle;
		
		// set up the world for the upcoming copy
		SetQDDevice(device, clip);
		MacRect(destR, dstRect);
		MacRect(source.bounds, srcRect);
	
		// make the mask pixmap on the fly each time
		maskPixMapHandle = NewPixMap();
		MakePixMap(**maskPixMapHandle, mask.baseAddress, mask.depth, mask.rowBytes, srcRect, mask.colorTable);
	
		// make the source pixmap on the fly each time
		srcPixMapHandle = NewPixMap();
		MakePixMap(**srcPixMapHandle, source.baseAddress, source.depth, source.rowBytes, srcRect, source.colorTable);
	
		// copy its bits over to the destination
		if (dstRect.left != dstRect.right || dstRect.top != dstRect.bottom)
			//CopyDeepMask((BitMap *)*srcPixMapHandle, (BitMap *)*maskPixMapHandle, (BitMap *)*device.pixmap, &srcRect, &srcRect, &dstRect, 0, nil);
			CopyDeepMask((BitMap *)*srcPixMapHandle, (BitMap *)*maskPixMapHandle, (BitMap *)&(qd.thePort->portBits), &srcRect, &srcRect, &dstRect, 0, nil);
	
		// destroy the source and mask pixmaps we just created
		DisposePixMap(maskPixMapHandle);
		DisposePixMap(srcPixMapHandle);
	
		// restore the world and notify the simulator that new bits have arrived
		// note: the notify is unnecessary if device != gScreenDevice
		UnSetQDDevice(device);
	}
	DrawingComplete(destR, clip);
}

#endif


void PaintRectangleQD(BitMapDevice& device, const Rectangle& r, Color color, ulong , const Rectangle* clip)
{
	
	Rect	macRect;

	SetQDDevice(device, clip);
	SetQDColor(color);
	MacRect(r, macRect);
	PaintRect(&macRect);
	SetQDColor(0x000000);
	UnSetQDDevice(device);
}

void FrameRectangleQD(BitMapDevice& device, const Rectangle& r, Color color, ulong , const Rectangle* clip)
{
	
	Rect	macRect;

	SetQDDevice(device, clip);
	SetQDColor(color);
	MacRect(r, macRect);
	FrameRect(&macRect);
	SetQDColor(0x000000);
	UnSetQDDevice(device);
}

ulong TextMeasureQD(BitMapDevice& , XFont font, CharacterEncoding ,const char* text, ulong length)
{
	SetQDFont(font);
	return TextWidth(text, 0, length);
}

ulong GetFontLeadingQD(const XFont font,CharacterEncoding )
{
	FontInfo	finfo;
	SetQDFont(font);
	GetFontInfo(&finfo);					
	return finfo.leading;


}

ulong GetFontDescentQD(const XFont font,CharacterEncoding )
{
	FontInfo	finfo;
	SetQDFont(font);
	GetFontInfo(&finfo);					
	return finfo.descent;
}

ulong GetFontAscentQD(const XFont font,CharacterEncoding )
{
	FontInfo	finfo;
	SetQDFont(font);
	GetFontInfo(&finfo);					
	return finfo.ascent;
}

long PaintTextQD(BitMapDevice& device, XFont font,CharacterEncoding encoding,const char* text, ulong length, Color color, Ordinate x, Ordinate y, ulong, Boolean , const Rectangle* clip)
{
	long width;
	Rectangle tBounds;
	
	SetQDDevice(device, clip);
	SetQDFont(font);
	SetQDColor(color);
	MoveTo(x,y);
	DrawText(text, 0, length);
	FastTextBounds(device, font, encoding,text, length, x, y, &tBounds);
	width = tBounds.right - x;
	SetQDColor(0x000000);
	UnSetQDDevice(device);
	DrawingComplete(tBounds, clip);
	return width;
}

// Mac-ONLY routine for making a BitMapDevice that corresponds to a window

void MakeBitMapDevice(BitMapDevice& device, CGrafPtr port)
	{
	Assert((port->portVersion & 0xC000) == 0xC000);	// must truly be a CGrafPtr
	
	Rect	globalRect = port->portRect;
	LocalToGlobal((MacPoint*)&globalRect.top);
	LocalToGlobal((MacPoint*)&globalRect.bottom);
	
	GDHandle deepestDevice = GetMaxDevice(&globalRect);


	if (deepestDevice == nil)
	{
		device.bounds.top = device.bounds.left = device.bounds.bottom = device.bounds.right = 0;
		return;
	}
	
	Rect clippedRect = globalRect;
	SectRect(&globalRect, &(*deepestDevice)->gdRect, &clippedRect);
	OffsetRect(&globalRect, -(*deepestDevice)->gdRect.left, -(*deepestDevice)->gdRect.top);
	OffsetRect(&clippedRect, -(*deepestDevice)->gdRect.left, -(*deepestDevice)->gdRect.top);

	device.rowBytes = (*(*deepestDevice)->gdPMap)->rowBytes & 0x7FFF;
#if defined FIDO_INTERCEPT
	Assert((device.rowBytes & 3) == 0);
#endif
	device.format = (BitMapFormat)(*(*deepestDevice)->gdPMap)->pixelSize;
	device.depth = device.format;
	device.depthShift = kDepthToShiftTable[device.depth];
	device.baseAddress = (unsigned char*) (*(*deepestDevice)->gdPMap)->baseAddr;
	device.transparentColor = 0;
	device.colorTable = nil;
	device.filter = kNoFilter;

	device.bounds.top  = -globalRect.top;	device.bounds.bottom = globalRect.bottom - globalRect.top;
	device.bounds.left = -globalRect.left;	device.bounds.right  = globalRect.right - globalRect.left;
	
	device.deviceBounds.top = clippedRect.top - globalRect.top;
	device.deviceBounds.left = clippedRect.left - globalRect.left;
	device.deviceBounds.bottom = clippedRect.bottom - globalRect.top;
	device.deviceBounds.right = clippedRect.right - globalRect.left;
#if defined FIDO_INTERCEPT
	device.clutType = (CLUTType) -1;
	device.clutBase = 0;
	device.textureType = (BitMapFormat) -1;
	device.textureBase = 0;
#endif
}

#endif /* FOR_MAC */



#endif		// SIMULATOR
