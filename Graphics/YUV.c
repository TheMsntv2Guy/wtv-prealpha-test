#include	"Headers.h"

//====================================================================================
//====================================================================================

//	Filters using mpeg's recommended (-29   0  88  138  88   0  -29)
//	Not pretty, but works

Byte XX(long x)
{
	x += 127;
	if (x < 0) return 0;
	if (x > 0x0FFFF) return 0xFF;
	return x >> 8;
}

void FilterMPEG_1D(Byte *src, long width, Byte *dst, long dstStep)
{
	short	kernel[] = {-29,0,88,138,88,0,-29};
	short	*k = kernel + 3;
	short	n;

	if (width < 4) return;
	*dst = XX(src[ 0]*(k[ 0] + k[-1] + k[-3]) +
			src[ 1]*k[ 1] +
			src[ 3]*k[ 3]);
	dst += dstStep;
	src += 2;
	*dst = XX(src[-1]*(k[-1] + k[-3]) +
			src[ 0]*k[ 0] + 
			src[ 1]*k[ 1] +
			src[ 3]*k[ 3]);
	dst += dstStep;
	src += 2;
	for (n = 2; n < ((width >> 1) - 2); n++) 
	{		
		*dst = XX(src[-3]*k[-3] +
				src[-1]*k[-1] +
				src[ 0]*k[ 0] + 
				src[ 1]*k[ 1] +
				src[ 3]*k[ 3]);
		dst += dstStep;
		src += 2;
	}
	*dst = XX(src[-3]*k[-3] +
			src[-1]*k[-1] +
			src[ 0]*k[ 0] + 
			src[ 1]*(k[ 1] + k[ 3]));
	dst += dstStep;
	src += 2;
	*dst = XX(src[-3]*k[-3] +
			src[-1]*k[-1] +
			src[ 0]*(k[ 0] + k[ 1] + k[ 3]));
}

void FilterMPEG(long width,long height, Byte *data, long rowBytes, Byte *buffer)
{ 
	Byte	*dst,*src;
	short	i;

	src = data;									//	Filter along the horizontal
	dst = (Byte *)buffer;
	for (i = 0; i < height; i++) 
	{		
		FilterMPEG_1D(src,width,dst,height);	// Rotate from horizontal to vertical
		dst++;
		src += rowBytes;
	}
	src = (Byte *)buffer;						//	Filter along the vertical
	dst = data;
	for (i = 0; i < (width >> 1); i++) 
	{		
		FilterMPEG_1D(src,height,dst,rowBytes);	// Rotate from vertical to horizontal
		dst++;
		src += height;
	}
}

void FilterMPEGHorizontal(long width,long height, Byte *data, long rowBytes, Byte *buffer)
{ 
	Byte	*dst,*src;
	short	i;

	src = data;									//	Filter along the horizontal
	dst = (Byte *)buffer;
	for (i = 0; i < height; i++) 
	{		
		FilterMPEG_1D(src,width,dst,1);			// Rotate from horizontal to vertical
		dst++;
		src += rowBytes;
	}
}


//		Filters 1:3:3:1, coded for clarity, not speed

void Filter1331_1D(Byte *src, long width, Byte *dst, long dstStep)
{
	short	n;

	*dst = (src[0]*4 + src[1]*3 + src[2] + 3) >> 3;
	dst += dstStep;
	src += 2;
	for (n = 1; n < ((width >> 1) - 1); n++) 
	{		
		*dst = (src[-1] + src[0]*3 + src[1]*3 + src[2] + 3) >> 3;
		dst += dstStep;
		src += 2;
	}
	*dst = (src[-1] + src[0]*3 + src[1]*4 + 3) >> 3;
}

void Filter1331_2D(long width, long height, Byte *data, long rowBytes, Byte *buffer)
{ 
	Byte	*dst,*src;
	short	i;

	src = data;									//	Filter along the horizontal
	dst = (Byte *)buffer;
	for (i = 0; i < height; i++) 
	{		
		Filter1331_1D(src,width,dst,height);	// Rotate from horizontal to vertical
		dst++;
		src += rowBytes;
	}
	src = (Byte *)buffer;						//	Filter along the vertical
	dst = data;
	for (i = 0; i < (width >> 1); i++) 
	{		
		Filter1331_1D(src,height,dst,rowBytes);	// Rotate from vertical to horizontal
		dst++;
		src += height;
	}
}

void MakeYUV(Byte *src, long rowBytes, long width, long height, Byte *yData)
{
	long	yy,p,makeUp = rowBytes - (width <<  2);
	short	x,y;
	Byte	*buffer;

	Byte *yD = yData;
	Byte *uD = yData + width*height;
	Byte *vD = uD + width*height;
	for (y = 0; y < height; y++) 
	{		
		for (x = 0; x < width; x++) 
		{			
			p = *(long *)src;
			src += 4;
			yy = (long)(*yD++ = (Byte)((((p >> 16) & 0xFF)*77 + ((p >> 8) & 0xFF)*150 + (p & 0xFF)*29) >> 8));
			*uD++ = (Byte)((((p & 0xFF) - yy + 227) << 8) / 453);
			*vD++ = (Byte)((((p >> 16) - yy + 179) << 8) / 357);
		}
		src += makeUp;
	}
	
//	Filter down the u and v data using 1:3:3:1
	
	uD = yData + width*height;
	vD = uD + width*height;
	buffer = (Byte *)AllocateTaggedMemory(width*height, "YUV data", 0);
	if (!buffer) return;
	Filter1331_2D(width,height,uD,width,(Byte *)buffer);
	Filter1331_2D(width,height,vD,width,(Byte *)buffer);
	FreeTaggedMemory(buffer, "YUV data");
}

short PIN(short x)
{
	return x < 0 ? 0 : (x > 255 ? 255 : x);
}

//	Does the 1:3:3:1 expansion of U and V clumsily

void MakeRGB(Byte *dst, long rowBytes, long width, long height, Byte *yData)
{
	long	makeUp = rowBytes - (width <<  2);
	short	yy,u,v,uv;
	short	x,y;
	Byte	*uD;
	Byte	*vD;
	Byte	*yD = yData;

	for (y = 0; y < height; y++) 
	{		
		for (x = 0; x < width; x++) 
		{			
			uD = yData + width*height + (x >> 1) + (y >> 1)*width;	// Code Crime
			vD = uD + width*height;
			
			yy = *yD++;
			u = ((*uD++ * 453) >> 8) - 227;
			v = ((*vD++ * 357) >> 8) - 179;
			uv = (29*u + 77*v)/150;
			
			*dst++ = 0;
			*dst++ = PIN(yy + v);
			*dst++ = PIN(yy - uv);
			*dst++ = PIN(yy + u);
		}
		dst += makeUp;
	}
}

//====================================================================================
//====================================================================================

//	Create vectors for the YYYYUV vectors

void MakeType1Vectors(Byte *src, long width, long height, short *vectors)
{
	short	x,y;
	Byte *u = src + width*height;
	Byte *v = u + width*height;
	for (y = 0; y < (height >> 1); y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		for (x = 0; x < (width >> 1); x++) 
		{			
			*vectors++ = *s1++;
			*vectors++ = *s1++;
			*vectors++ = *s2++;
			*vectors++ = *s2++;
			*vectors++ = *u++;
			*vectors++ = *v++;
		}
		u += (width >> 1);
		v += (width >> 1);
		src += width << 1;
	}
}

//	Create vectors for the 2x2 separate Y and UV vectors, Y Vectors

void MakeType2YVectors(Byte *src, long width, long height, short *YVectors)
{
	short	x,y;
	for (y = 0; y < (height >> 1); y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		for (x = 0; x < (width >> 1); x++) 
		{			
			*YVectors++ = *s1++;
			*YVectors++ = *s1++;
			*YVectors++ = *s2++;
			*YVectors++ = *s2++;
		}
		src += width << 1;
	}
}

//	Create vectors for the 2x2 separate Y and UV vectors, UV Vectors

void MakeType2UVVectors(Byte *src, long width, long height, short *UVVectors)
{
	short	x,y;
	src += width*height;
	for (y = 0; y < (height >> 2); y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		Byte *s3 = src + width*height;
		Byte *s4 = s3 + width;
		for (x = 0; x < (width >> 2); x++) 
		{			
			*UVVectors++ = *s1++;	// U
			*UVVectors++ = *s1++;
			*UVVectors++ = *s2++;
			*UVVectors++ = *s2++;
			*UVVectors++ = *s3++;	// V
			*UVVectors++ = *s3++;
			*UVVectors++ = *s4++;
			*UVVectors++ = *s4++;
		}
		src += width << 1;
	}
}

Byte MakeType3Vector(Byte *src, long width, short *YVectors)
{
	short	x,y;
	short	mean = 0;
	for (y = 0; y < 2; y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		for (x = 0; x < 2; x++) 
		{			
			mean += (*YVectors++ = *s1++);
			mean += (*YVectors++ = *s1++);
			mean += (*YVectors++ = *s2++);
			mean += (*YVectors++ = *s2++);
		}
		src += width << 1;
	}
	YVectors -= 16;
	mean = (mean + 7) >> 4;
	for (x = 0; x < 16; x++)
		YVectors[x] = (YVectors[x] - mean) >> 1;
	return mean;
}

//	Create vectors for the 4x4 mean separated Y and UV vectors, Y Vectors

void MakeType3YVectors(Byte *src, long width, long height, short *YVectors, Byte *mean)
{
	short	x,y;
	for (y = 0; y < (height >> 2); y++) 
	{		
		for (x = 0; x < (width >> 2); x++) 
		{			
			*mean++ = MakeType3Vector(src + (x << 2),width,YVectors);
			YVectors += 16;
		}
		src += width << 2;
	}
}

//====================================================================================
//====================================================================================

void DrawType1Vectors(Byte *src, long width, long height, Byte *codeBook, Byte *match)
{
	short	x,y;
	Byte *u = src + width*height;
	Byte *v = u + width*height;
	for (y = 0; y < (height >> 1); y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		for (x = 0; x < (width >> 1); x++) 
		{			
			Byte *c = codeBook + (*match++ * 6);
			*s1++ = *c++;
			*s1++ = *c++;
			*s2++ = *c++;
			*s2++ = *c++;
			*u++ = *c++;
			*v++ = *c++;
		}
		u += (width >> 1);
		v += (width >> 1);
		src += width << 1;
	}
}

void DrawType2UV(Byte *src, long width, long height, Byte *UVCodeBook, Byte *UVMatch)
{
	short	x,y;
	Byte	*c;
	
	for (y = 0; y < (height >> 2); y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		Byte *s3 = src + width*height;
		Byte *s4 = s3 + width;
		for (x = 0; x < (width >> 2); x++) 
		{			
			c = UVCodeBook + (*UVMatch++ << 3);
			*s1++ = *c++;	// U
			*s1++ = *c++;
			*s2++ = *c++;
			*s2++ = *c++;
			*s3++ = *c++;
			*s3++ = *c++;
			*s4++ = *c++;
			*s4++ = *c++;
		}
		src += width << 1;
	}
}

//	Test 'em, draw em

void DrawType2Vectors(Byte *src, long width, long height, 
		Byte *YCodeBook, Byte *YMatch, Byte *UVCodeBook, Byte *UVMatch)
{
	short	x,y;
	Byte	*c;
	
	for (y = 0; y < (height >> 1); y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		for (x = 0; x < (width >> 1); x++) 
		{			
			c = YCodeBook + (*YMatch++ << 2);
			*s1++ = *c++;
			*s1++ = *c++;
			*s2++ = *c++;
			*s2++ = *c++;
		}
		src += width << 1;
	}
	DrawType2UV(src,width,height,UVCodeBook,UVMatch);
}

void DrawType3Vector(Byte *src, long width, Byte *YCodeBook, Byte *YMatch, short mean)
{
	short	x,y;
	for (y = 0; y < 2; y++) 
	{		
		Byte *s1 = src;
		Byte *s2 = s1 + width;
		for (x = 0; x < 2; x++) 
		{			
			char *c = (char *)(YCodeBook + (*YMatch++ << 2));
			*s1++ = PIN(mean + (*c++ << 1));
			*s1++ = PIN(mean + (*c++ << 1));
			*s2++ = PIN(mean + (*c++ << 1));
			*s2++ = PIN(mean + (*c++ << 1));
		}
		src += width << 1;
	}
}

void DrawType3Vectors(Byte *src, long width, long height,
		Byte *YCodeBook, Byte *YMatch, Byte *UVCodeBook, Byte *UVMatch, Byte *mean)
{
	short	x,y;
	for (y = 0; y < (height >> 2); y++) 
	{		
		for (x = 0; x < (width >> 2); x++) 
		{			
			DrawType3Vector(src + (x << 2),width,YCodeBook,YMatch,*mean++);
			YMatch += 4;
		}
		src += width << 2;
	}
	DrawType2UV(src,width,height,UVCodeBook,UVMatch);
}
