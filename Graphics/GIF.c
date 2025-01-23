// ===========================================================================
//	GIF.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __GIF_H__
#include "GIF.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif
#ifndef	__VECTORQUANTIZATION__
#include "VectorQuantization.h"
#endif

#if defined FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif

#include "PageViewer.h"
#include "ContentView.h"
#include "System.h"

#ifndef __SCREEN_H__
#include "Screen.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif



extern void VerifyMemory();

const 	kScaleShift=9;

#define	SCALE_ORD(_x,_n,_d)	(((((long)((_n) * (_x)) << kScaleShift)) / (_d) + ( 1<<(kScaleShift-1))) >> kScaleShift)


// ===========================================================================
//	implementation
// ===========================================================================

GIF::GIF()
{
	fPhase = kGlobalImageHeader;	
	fFilterType = kFullFilter;	
	fWhiteLevel = 255;
}

GIF::~GIF()
{
	Reset(false);
}

void
GIF::Rewind()
{
	Reset(true);
	fPhase = kGlobalImageHeader;
}


void GIF::Reset(Boolean keepBuffer)
{

	if ( !keepBuffer )
	{
		if ( fBitMap )
		{
			// the color table is shared with the bitmap, only one needs to destroy it.
			fBitMap->colorTable = nil;
			DeleteBitMapDevice(fBitMap);
			fBitMap = nil;
		}
		if (fGlobalColorTable != nil)
			DeleteColorTable(fGlobalColorTable);
		fGlobalColorTable = nil;
		if (fLocalColorTable != nil)
			DeleteColorTable(fLocalColorTable);
		fLocalColorTable = nil;
		fTop = fLeft = fWidth = fHeight =0;
		fLocalHeight =0;
		fLocalWidth = 0;
	}
	if ( fResizeBuffer )
		DeleteBitMapDevice(fResizeBuffer);
	fResizeBuffer = nil;
	if ( fResizeAccumulateBuffer )
		DeleteBitMapDevice(fResizeAccumulateBuffer);
	fResizeAccumulateBuffer = nil;
	
	fPhase = kGlobalImageHeader;
	fFrameCount = 0;
	fLoopCount = 0;
	fFormat = 0;
	fDepth =0;
	fTransparencyIndex = 0;
	fTransparent = false;
	fInterlaced = false;
	fSkippingExtensionData = false;
	fDataComplete = false;
	fDidLastFrame = false;
	fWaitTime = 0;
	fDisposalMethod = 0;
	fDelayTime = 0;
	ResetDrawing();
}

void GIF::ResetLZW()
{
	if ( fBuffer )
		FreeTaggedMemory(fBuffer,"GIF Read Buffer");
	fBuffer = nil;
	if (fPrefix != nil)
		FreeTaggedMemory(fPrefix, "GIF Prefix Table"); 
	fPrefix = nil;		
	if (fSuffix != nil)
		FreeTaggedMemory(fSuffix, "GIF Suffix Table");
	fSuffix = nil;
	if (fStack != nil)
		FreeTaggedMemory(fStack, "GIF Stack");  
	fSP = fStack = nil;
	fBits = 0;
	fBufferCount = 0;
	fData = 0;
	fLast32 =0;
	fStartedLZW = false;
	fEndOfLZW = false;
	fSetCodeSize = 0;
	fCodeSize = 0;
	fClearCode = 0;
	fEndCode = 0;
	fMaxCodeSize = 0;
	fMaxCode = 0;
	fFirstCode = 0;
	fOldCode = 0;
}

void GIF::ResetDrawing()
{
	ResetLZW();
	fXPos = fYPos = 0;
	fPass = 0;
	fDstYPos = 0;
	fDstYInc = 0;
	fDstYLast = 0;
	fFilterTop = 0;
	fFilterBottom = 0;
	fNotDrawn = true;
	fDidFilter = false;
}

void GIF::GetBounds(Rectangle* bounds) const
{
	SetRectangle(*bounds, 0, 0, fWidth, fHeight);
}

void GIF::SetBounds(const Rectangle* bounds)
{
	fSrcBounds = *bounds;
}

void GIF::SetKeepBitMap(Boolean keep)
{
#ifdef FIDO_INTERCEPT
	keep = true;
#endif
	fKeepBitMap = keep;
}

void GIF::SetIsBackground(Boolean is)
{
	fFilterType = kNoFilter;
	fIsBackground = is;
}

void GIF::SetGamma(Gamma gamma,uchar blackLevel,uchar whiteLevel)
{
	fGamma = gamma;
	fBlackLevel = blackLevel;
	fWhiteLevel = whiteLevel;
}

void GIF::SetFilterType(FilterType filt)
{
	if ( !fIsBackground )
		fFilterType = filt;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean GIF::GetKeepBitMap() const
{
	return fKeepBitMap;
}
#endif


BitMapDevice* GIF::GetBitMap() const
{
	if ( fKeepBitMap || fCacheBitMap )
	return fBitMap;
	else
		return nil;
}


void GIF::SetTransparency(ulong transparency)
{
	fTransparency = transparency;
}



long GIF::Write(Byte* data, long dataLength,Boolean dataComplete,Boolean justBounds,Rectangle *drewRectangle)
{
	//	Write a byte int the GIF decoder, see what comes out
	long l = 0;
	
	fDataComplete = dataComplete;
	SetRectangle(fDrewRectangle,0,0,0,0);
	if ( dataLength ) {
	switch (fPhase)
	{
		case kGlobalImageHeader:	
			l = ReadGlobalHeader(data, dataLength);
			break;
		case kReadExtension:
			l = ReadExtension(data, dataLength);
			break;
		case kLocalImageHeader:	
			l = ReadImageHeader(data, dataLength);
			break;
		case kLocalImageBodySetup:
			l = SetupImageBody(data, dataLength);
			break;
		case kLocalImageBody:
			if ( justBounds ) 
				l = SkipData(data, dataLength);
			else 
				l = ReadImageBody(data, dataLength);
			break;
		case kGlobalImageEnd:		
			Reset(fKeepBitMap||fCacheBitMap);
			l = -1;
			break;
		default:
			Message(("GIF::Write bad phase? %d",fPhase));
			return kCannotParse;
	}
	}
	*drewRectangle = fDrewRectangle;
	return l;
}


//	Read the header info in from the GIF file

long GIF::ReadGlobalHeader(Byte *data, long dataLength)
{
	if (dataLength < 13) return 0;	// Didn't read the header
	
	fFormat = 0;
	if (!strncmp((char *)data,"GIF87a",6))
	{
		//Message(("GIF is 87a"));
		fFormat = 87;
	}
	if (!strncmp((char *)data,"GIF89a",6))
	{
		//Message(("GIF is 89a"));
		fFormat = 89;
	}
	if (fFormat == 0)
	{
		Message(("Bad GIF header"));
		return kCannotParse;
	}
	
	fWidth = data[6] + (data[7] << 8);
	fHeight = data[8] + (data[9] << 8);


	//Message(("Global image: %d x %d",fLocalWidth,fLocalHeight));

	uchar packed = data[10];
	fBackGroundColor = data[11];	
	// aspectRatio = (data[12] + 15) / 64;		
	
	//	Read global color table

	short numberOfColors = 1 << ((packed & 0x7) + 1);
	//short colorResolution = ((packed >> 4) & 7) + 1;
	
	if ( fBackGroundColor > ((packed & 0x7) + 1) )		// check for invalid background color indices
		fBackGroundColor = 0;
	
	fDepth = ((packed) & 0x7) + 1;
	
	if ( (packed & 0x80) != 0) {		// global color table is present

		{
			if (dataLength < (13 + 3 * numberOfColors))		// Read header, not enough for color table
				return 0;
				
			if ( IsError(fGlobalColorTable != nil && fGlobalColorTable->size !=  3*numberOfColors) )
			{
				Complain(("hosed color table"));
				DeleteColorTable(fGlobalColorTable);
				fGlobalColorTable = nil;
			}
			if ( fGlobalColorTable == nil )
				fGlobalColorTable = NewColorTable(kRGB24,numberOfColors);
			else
				fGlobalColorTable->version = kRGB24;
				
			if ( IsError(fGlobalColorTable == nil) )
			{
				Message(("No memory for color table"));
				return kLowMemory;
			}
			CopyMemory( data + 13,&fGlobalColorTable->data[0], 3*numberOfColors);
			if ( fBitMap )
				fBitMap->colorTable = fGlobalColorTable;

		}

		//Message(("Got Global Color table: Size %d, %dbpp",
		//	colorTableSize/3,colorResolution));
		

#if	0
//		Want to have the image fade in from black
		uchar *p;
		for (p = &fGlobalColorTable->data[0]; p < fGlobalColorTable->data + colorTableSize; p+=3)
		{
			if (*p==0 && *(p+1)==0 && *(p+2)==0)	// hunt for a black entry
			{
				fBackGroundColor = (p-(fGlobalColorTable->data))/3;
				break;
			}
		}
#endif


	} else
		numberOfColors = 0;
	
	fPhase = kReadExtension;
	return 13 + 3 * numberOfColors;
}


long GIF::ReadComment(Byte *data, long dataLength)
{
	// Skip a comment

	short count;
	
	if ( dataLength < 2 )
		return 0;
		
	Byte*	src = data + 2;
	dataLength -= 2;
	
	while ((count = *src++) != 0)
	{
		// Not enough data here to read extension?
		if (dataLength <= count + 1)
			return 0;
		src += count;
		dataLength -= count;
	}

	// Note: could do something useful with the comment ....

	return src - data;
}


// draw the next frame of animation, returns true if not ready yet

Boolean GIF::Animate()
{

	if ( fWaitTime )
	{
		if (  Now() > fWaitTime )			
			fWaitTime = 0;
		return true;
	}
	
	DisposeImage();

	// should it erase just where it is going to draw, or the entire last frame ??? 

	fLastDrawRectangle = fDrawRectangle;

	long dw = fDrawRectangle.right - fDrawRectangle.left;
	long dh = fDrawRectangle.bottom - fDrawRectangle.top;
	
	
	Rectangle srcRect = fImageRectangle;
	if ( fFlipHorizontal ) {
		long t = srcRect.left;
		srcRect.left = fWidth - srcRect.right;
		srcRect.right = fWidth - t;
	}
	if ( fFlipVertical ) {
		long t = srcRect.top;
		srcRect.top = fHeight - srcRect.bottom;
		srcRect.bottom = fHeight - t;
	}
	if ( dw == fWidth && dh == fHeight )
	{
		srcRect.right = srcRect.left + fLocalWidth;
		srcRect.bottom = srcRect.top + fLocalHeight;
		fLastDrawRectangle.left += srcRect.left;
		fLastDrawRectangle.top += srcRect.top;
		fLastDrawRectangle.right = fLastDrawRectangle.left + fLocalWidth;
		fLastDrawRectangle.bottom = fLastDrawRectangle.top + fLocalHeight;
	}
	else
	{
	
		srcRect.right = srcRect.left + fLocalWidth;
		srcRect.bottom = srcRect.top + fLocalHeight;
		fLastDrawRectangle.left += SCALE_ORD(srcRect.left,dw,fWidth);
		fLastDrawRectangle.right = fLastDrawRectangle.left + SCALE_ORD(fLocalWidth,dw,fWidth);
		fLastDrawRectangle.top += SCALE_ORD(srcRect.top,dh,fHeight);
		fLastDrawRectangle.bottom = fLastDrawRectangle.top + SCALE_ORD(fLocalHeight,dh,fHeight);
	}

	DoDraw();



//	UpdateScreenBits();

	if ( fHoldFrameTime )
		fWaitTime = Now() + fHoldFrameTime;
	else
		fWaitTime = 0;
	fHoldFrameTime = 0;	
	return false;
}


 
Boolean GIF::GetBackgroundColor(Color *bcp) const
{
	Rectangle r = fDrawRectangle;
	IntersectRectangle(&r,&fClipRectangle);
	Color color = AverageImage(gScreenDevice,*bcp,&r,true);
	if ( color == (Color)-1 )
		return false;
	*bcp = color;
	return true;	
}

//	Read Graphic control extension

long GIF::ReadGraphicControl(Byte *data, long dataLength)
{
	
	if (dataLength < 8) return 0;	// Total size of Graphic extension
	
	
	fTransparent = (data[3] & 1) != 0;
	// fUserInputWait = (data[3] & 2) != 0;
	fDisposalMethod = ((data[3]>>2) & 7) +1;
	fDelayTime = data[4] + (data[5]<<8);
	if ( fDelayTime == 0 )
		fDelayTime = 1;
	fTransparencyIndex = data[6];
	
	if (fGlobalColorTable == nil)
		return 8;

	if (fTransparent ) {
	
		// check for invalid transparent color indices, if present treat as non-transparent
		
		if ( (fGlobalColorTable->version == kYUV32 && fTransparencyIndex >= fGlobalColorTable->size / 4 ) || 
			(fGlobalColorTable->version == kRGB24 && fTransparencyIndex >= fGlobalColorTable->size / 3 ) ) {
			fTransparent = false;
			return 8;
		}
	}
//	if (fTransparent) TrivialMessage(("GIF is Transparent: Index %d",fTransparencyIndex));
	return 8;
}


long GIF::GetLoopCount() const
{
	return fLoopCount;

}

//	Read the next extension

long GIF::ReadExtension(Byte *data, long dataLength)
{
	long count,used  = 0;
	Byte *src = data;

	if ( fSkippingExtensionData ) {
		while ((count = *src) != 0)
		{
			if (dataLength <= (count + 1))	// Not enough data here to read extension
				return used;
			src += count+1;
			dataLength -= count+1;
			used += count+1;
		}
		used++;	// count the last zero
		fSkippingExtensionData = 0;
		return used;
	}
	
	switch (*data)
	{
		case 0x3B:	// end of image
			if ( fFrameCount > 1 ) 					// multi-frame animation
			{
				if ( !fDidLastFrame )
				{
					if ( Animate() )
						return used;
					fDidLastFrame = true;
				} 
				if ( fWaitTime &&  Now() < fWaitTime )			
					return used;
				fDidLastFrame = false;
				fWaitTime = 0;
			}
			if ( fLoopCount > 0 )
			{
				fLoopCount--;
				fFrameCount = 0;
				SetRectangle(fLastDrawRectangle,0,0,0,0);
				fPhase = kGlobalImageHeader;		// start from first frame
			}
			else
			{
				Reset(fKeepBitMap||fCacheBitMap);
				fPhase = kGlobalImageEnd;
			}
			return dataLength;			// ignore anything else in stream
		case 0x2C:	// start of image
			return ReadImageHeader(data, dataLength);
		case 0x21:							// Control Extension
			{
				if (dataLength < 3)
					return used;	// Not enough data for the smalllest extension
				switch (data[1])
				{
					case 0xFE:	return ReadComment(data, dataLength);
					case 0xF9:	return ReadGraphicControl(data, dataLength);
					case 0x01:	TrivialMessage(("Plain Text Extension"));			break;
					case 0xFF:
					
						if ( dataLength < (3 + data[2]+1) )
							return used;
							
						src += 3 + data[2];
						used += 3 + data[2];
						dataLength -= 3 + data[2];
						
						if ( strncmp((const char *)data+3,"Netscape2.0",11) == 0  || strncmp((const char *)data+3,"NETSCAPE2.0",11) == 0 )
						{
							TrivialMessage(("Netscape2.0 Extension"));	
							
							// only understand loop count
							uchar size = src[0];
							uchar code = src[1];
							if ( size == 3 && code == 1 ) {
								fLoopCount = (src[3]<<8) + src[2];
								if ( fLoopCount == 0 )
									fLoopCount = 0xffffffff;
						}
						}
						else {
							Boolean newAlphaFormat = strncmp((const char *)data+3,"Artemis ALP",11) == 0;
							
							if ( newAlphaFormat || strncmp((const char *)data+3,"Artemis ALF",11) == 0 ) {
								uchar numAlphas = src[0];
								uchar *alphaVals = src + 1;
								if ( dataLength < (2 + numAlphas) )
								return 0;
									
							TrivialMessage(("Artemis alpha extension %d alpha values",numAlphas));	

								int nc = 1 << fDepth;
								
							{
									if ( IsWarning(nc < numAlphas) || fGlobalColorTable == nil || fGlobalColorTable->version !=  kRGB24 ) {
									Message(("invalid alpha gif %d %d",nc,numAlphas));
								} else {
									uchar r,g,b;
									short i,j,k,m;
									short y,u,v;
									CLUT *rgbClut = fGlobalColorTable;
									fGlobalColorTable = NewColorTable(kYUV32,nc);
									if ( IsError(fGlobalColorTable == nil) )
									{
										Message(("No memory for color table"));
										return kLowMemory;
									}
									for ( i=0, j =0, k =0, m =0; i < nc; i++)
									{
										if ( i < numAlphas ) {
											r = rgbClut->data[j++];
											g = rgbClut->data[j++];
											b = rgbClut->data[j++];
											rgbtoyuv(r,g,b,&y,&u,&v);
											fGlobalColorTable->data[k++] = y;
											fGlobalColorTable->data[k++] = u;
												if ( newAlphaFormat && i == 0 ) 
												fGlobalColorTable->data[k++] = 0;
											else
												fGlobalColorTable->data[k++] = alphaVals[m++];
											fGlobalColorTable->data[k++] = v;
										} else {
											fGlobalColorTable->data[k++] = 0;
											fGlobalColorTable->data[k++] = kUVOffset;
											fGlobalColorTable->data[k++] = 0;
											fGlobalColorTable->data[k++] = kUVOffset;
										}
									}
										fDepth = 8;
									if ( rgbClut )
										DeleteColorTable(rgbClut);
								}
							}
						}
						else
							TrivialMessage(("GIF: Unhandled application extension '%s'",data+3));		
						}	
						break;
					default:	Message(("GIF: bad control extension: 0x%0x,ignored",(int)data[1])); break;
				}
				fSkippingExtensionData = true;
				while ((count = *src) != 0)
				{
					if (dataLength <= (count + 1))	// Not enough data here to read extension
						return used;
					dataLength -= count+1;
					used += count+1;
					src += count+1;
				}
				used++;	// count the last zero
				fSkippingExtensionData = false;
				return used;
			}
		default:
			Message(("GIF: bad extension 0x%0x, ignored",(int)*data));
			fSkippingExtensionData = true;
			while ((count = *src) != 0)
			{
				if (dataLength <= (count + 1))	// Not enough data here to read extension
					return used;
				dataLength -= count+1;
				used += count+1;
				src += count+1;
			}
			used++;	// count the last zero
			fSkippingExtensionData = false;
			return used;
	}
}

long GIF::ReadImageHeader(Byte *data, long dataLength)
// Read local image
{
	uchar packed;

	if (dataLength < 10)
		return 0;				// Didn't read the local image header
	if (data[0] != 0x2C)
	{
		Message(("Bad image separator: 0x%X",data[0]));
		return kCannotParse;
	}

	packed = data[9];
	fInterlaced = (packed & 0x40) != 0;
	
#if	1

	/*
	
	 if a GIF is interlaced AND transparent, AND on a solid background 
	 then make it non-transparent so we can draw it in the chunk style 
	 which is less prone to flickering.
	
	*/
	
	if ( fTransparent && fInterlaced  ) {
		Color bcolor = kBlackColor;
		if (gPageViewer != nil)
			bcolor = gPageViewer->GetDocument()->GetBackgroundColor();
		if ( fIsBackground ||  GetBackgroundColor(&bcolor) && (fGlobalColorTable->version == kYUV32 ||
			fGlobalColorTable->version == kRGB24) ) {
			uchar r,g,b;
			r = (bcolor>>16) & 0xff;
			g = (bcolor>>8) & 0xff;
			b = (bcolor) & 0xff;
			if ( fGlobalColorTable->version == kYUV32 ) {
				short y,u,v;
				rgbtoyuv(r,g,b,&y,&u,&v);
 				fGlobalColorTable->data[fTransparencyIndex*4] = 0;
				fGlobalColorTable->data[fTransparencyIndex*4+1] = y;
				fGlobalColorTable->data[fTransparencyIndex*4+2] = u;
				fGlobalColorTable->data[fTransparencyIndex*4+3] = v;
			} else {
				fGlobalColorTable->data[fTransparencyIndex*3] = r;
				fGlobalColorTable->data[fTransparencyIndex*3+1] = g;
				fGlobalColorTable->data[fTransparencyIndex*3+2] = b;
			}
			fTransparent = false;
		}
	}	
#endif
	short numberOfColors = 0;
	if ( ((packed & 0x80) != 0 ) ) {
		numberOfColors = 1 << ((packed & 0x07) + 1);
		if (dataLength < (10 + 3*numberOfColors))		// Read header, not enough for color table
			return 0;
	}
	
	if ( fFrameCount )
	{
		if ( Animate() )
			return 0;
	}
	else if ( fLastDrawRectangle.right-fLastDrawRectangle.left != 0 ) 
		DisposeImage();

	fFrameCount++;

	fLeft = data[1] + (data[2] << 8);
	fTop = data[3] + (data[4] << 8);
	fLocalWidth = data[5] + (data[6] << 8);
	fLocalHeight = data[7] + (data[8] << 8);
	
	if ( fWidth == 0 || fHeight == 0 ) {
		TrivialMessage(("GIF had zero global width and height, using local bounds %d %d",fLocalWidth,fLocalHeight));
		fWidth = fLocalWidth;
		fHeight = fLocalHeight;
	}
	
	//Message(("Local image: %d x %d",fLocalWidth,fLocalHeight));
	SetRectangle(fImageRectangle,fLeft,fTop,fLeft+fLocalWidth,fTop+fLocalHeight);
	
	
	if ( fLocalColorTable )
	{
		if ( fBitMap && fBitMap->colorTable == fLocalColorTable )
			fBitMap->colorTable = fGlobalColorTable;
		DeleteColorTable(fLocalColorTable);
		fLocalColorTable = nil;
	}
	
	if ( ((packed & 0x80) == 0 ) )
		numberOfColors = 0;
	else
	{
		TrivialMessage(("Local color table present: %d",numberOfColors));

		// not doing anything w/ local color tables yet, so don't include cxde to do nothing
		fLocalColorTable = (CLUT*)NewColorTable(kRGB24,numberOfColors);
		if  ( IsError(fLocalColorTable == nil)  )
			return kLowMemory;
		CopyMemory(data+10,&fLocalColorTable->data[0],3*numberOfColors);
		if ( fGlobalColorTable == nil )
		{
			fGlobalColorTable = fLocalColorTable;
			fLocalColorTable = nil;
		}
	}
	fPhase = kLocalImageBodySetup;
	return 10 + 3*numberOfColors;
}

//===========================================================================



#define	BITMASK(_n)	((1 << (_n)) - 1)

inline long  GIF::GetCode()
{
	long count = fCodeSize;
	int shift;
	

	if ( count < fBits ) {
		fBits -= count;
		shift = 32 - count - fBits;
		if ( shift == 32 )
			return 0;
		else
			return (fLast32>>shift) & BITMASK(count);
	} else { 
		long	a = 0,b;
		

		//	Load up the next 32 bits

		if ( fBits )
		a = fLast32 >> (32 - fBits);
		count -= fBits;
		shift = fBits;
		if ( fBufferCount >= 4 )
		{
			b = *fData++;
			b |= (*fData++ << 8);
			b |= (*fData++ << 16);
			b |= (*fData++ << 24);
			fBufferCount -= 4;
		}
		else
		{
			b = 0;
			while ( --fBufferCount >= 0 )
			{
				b <<= 8;
				b |= *fData++;
			}
		}
		fLast32 = b;
		fBits = 32 - count;
		if ( shift )
			return a | (fLast32 & BITMASK(count)) << shift;
		else
			return fLast32 & BITMASK(count);
	}
}




//	Alocate tables for decode

long
GIF::InitLZW(Byte *src, long dataLength)
{
	long used = 1;
	long bytesRead;
	
	// Start the LZW decode
	short initCodeSize = *src++;
	fBuffer = (Byte *)AllocateTaggedMemory(256+4,"GIF Read Buffer");
	
	if ( IsError(fBuffer == nil) )
		return kLowMemory;
	fBits = 0;
	fBufferCount = 0;
	bytesRead = FillBuffer(src,dataLength);
	if ( bytesRead < 0 )
		return 0;
	used += bytesRead;
	
	fSetCodeSize = initCodeSize;
	fCodeSize = fSetCodeSize + 1;
	fClearCode = 1 << fSetCodeSize;
	fEndCode = fClearCode + 1;
	fMaxCodeSize = fClearCode << 1;
	fMaxCode = fClearCode + 2;
	
	fPrefix = (short *)AllocateTaggedZeroNilAllowed(4096*sizeof(short), "GIF Prefix Table");
	fSuffix = (uchar *)AllocateTaggedZeroNilAllowed(4096*sizeof(uchar), "GIF Suffix Table");
	fStack  = (uchar *)AllocateTaggedMemoryNilAllowed((4096 + 1)*sizeof(uchar), "GIF Stack");

	if ( IsWarning( fPrefix == nil || fSuffix == nil || fStack == nil) )
	{
		if ( fPrefix )
			FreeTaggedMemory(fPrefix,"GIF Prefix Table");
		fPrefix = nil;
		if ( fSuffix )
			FreeTaggedMemory(fSuffix,"GIF Suffix Table");
		fSuffix = nil;
		if ( fBuffer )
			FreeTaggedMemory(fBuffer,"GIF Read Buffer");
		fBuffer = nil;
		return kLowMemory;
	}
	for (int i = 0; i < fClearCode; i++)
		fSuffix[i] = i;
	fSP = fStack;
	fStartedLZW = false;
	return used;
}

//	Decode the next pixel. Usually on stack .. make a macro


#define LZWPixel() ((fSP > fStack) ? *--fSP : DecodeLZW())

inline
short GIF::DecodeLZW()
{
	long	i,code,thisCode;

	if ((code = GetCode()) < 0)
		goto done;
	if ( !fStartedLZW ) {
		while ( code == fClearCode ) {
			code = GetCode();
			if ( code < 0 )
				goto done;
		}
		fFirstCode = fOldCode = code;
		fStartedLZW = true;
		goto done;
	}

	// Clear code resets tables
	if (code == fClearCode)
	{				
		fCodeSize = fSetCodeSize + 1;
		fMaxCodeSize = fClearCode << 1;
		fMaxCode = fClearCode + 2;
		code =  GetCode();
		ZeroMemory(fPrefix,4096*sizeof(short));
		for (i = 0; i < fClearCode; i++)
    		fSuffix[i] = i;
		ZeroMemory(fSuffix+i,4096-i);
			
		fSP = fStack;
		fFirstCode = fOldCode = code;
		if ( code == fClearCode ) {
			TrivialMessage(("GIF repeated clears"));
		}
		goto done;
	}
    
	if (code == fEndCode)					// End code
		return -998;

	thisCode = code;
	if (code >= fMaxCode)
	{
		*fSP++ = fFirstCode;
		code = fOldCode;
	}

	while (code >= fClearCode)
	{
		short pcode = fPrefix[code];
		*fSP++ = fSuffix[code];
		if ( IsError(code == pcode) ) 
			return kCannotParse;
		code = pcode;
	}
	*fSP++ = fFirstCode = fSuffix[code];

	if ((code = fMaxCode) < 4096)
	{
		fPrefix[code] = fOldCode;
		fSuffix[code] = fFirstCode;
		fMaxCode++;
  		if ((fMaxCode >= fMaxCodeSize) && (fMaxCodeSize < 4096))
		{
			fMaxCodeSize <<= 1;
			fCodeSize++;
		}
	}
	fOldCode = thisCode;
	if (fSP > fStack)
		code = *--fSP;
done:
#ifdef	DEBUG
	if ( code >= (1<<fDepth) ) {
		Message(("Bogus GIF data"));
		return kCannotParse;
		code = (1<<fDepth)-1;
	}
#endif
	return code;
}


//================================================================================
//================================================================================

//	Setup the LZW decode and allocate the image buffer and pixmap

long GIF::SetupImageBody(Byte *data, long dataLength)
{
	size_t	size;
	Rectangle	bounds;
	long	used;
	
	if (dataLength < (1 + 1 + data[1]))		// codeSize, blockCount, count
		return 0;

	if ( fWidth == 0 || fHeight == 0 )
		return kGenericError;
	bounds.left = 0; 
	bounds.right = fWidth;
	bounds.top = 0; 
	bounds.bottom = 1;

	if ( !fKeepBitMap && ( fFlipVertical || (fWidth <= 64 && fHeight <= 64) ) )			// if small or flipped draw the whole thing
		fCacheBitMap = true;
	
	if ( fFrameCount > 1 )
		fCacheBitMap = true;
		
	if ( fDataComplete && fInterlaced && ((fDrawRectangle.bottom-fDrawRectangle.top) < fHeight) )
		fCacheBitMap = true;

	if ( fKeepBitMap || fCacheBitMap )
		bounds.bottom = fHeight; 
	
	fNotDrawn = true;
	
	CLUT *clut = fLocalColorTable ? fLocalColorTable : fGlobalColorTable;
	BitMapFormat format = fDepth <= 4 ? index4Format : index8Format;
	if (clut != nil) {
		if ( fDepth == 8 && clut->version == kYUV32 )
			format = alpha8Format;
	
		if ( fGamma )
			GammaCorrect(clut, fGamma, fBlackLevel, fWhiteLevel);
	}
	
tryAgain:
	if ( fBitMap )
	{
		if ( format != fBitMap->format || fBitMap->bounds.right-fBitMap->bounds.left < fWidth 
			|| fBitMap->bounds.bottom-fBitMap->bounds.top < fHeight )
		{
			fBitMap->colorTable = nil;
			DeleteBitMapDevice(fBitMap);
			fBitMap = nil;
		}
		else {
			fBitMap->colorTable = clut;
			fBitMap->transparentColor = fTransparent ? kTransparent+fTransparencyIndex : kNoTransparentColor;
		}
	}
	if ( fBitMap == nil )
	{
		if ((fBitMap = NewBitMapDevice(bounds, format,clut, 
				fTransparent ? kTransparent+fTransparencyIndex : kNoTransparentColor)) == nil) {
			if ( (fCacheBitMap || fKeepBitMap) && !fFlipVertical && !(bounds.top == 0 && bounds.bottom == 1) ) {
				bounds.top = 0;
				bounds.bottom = 1;
				fCacheBitMap = false;
				fKeepBitMap = false;
				goto tryAgain;
			} else
				return kLowMemory;
		}
	}
	if ( fHeight > 1 && !(fCacheBitMap || fKeepBitMap) &&
		fDrawRectangle.bottom-fDrawRectangle.top != fHeight ) {
		fResizeBuffer = NewBitMapDevice(bounds, yuv422Format,nil,fTransparent ? kTransparent : kNoTransparentColor);
		if ( fResizeBuffer == nil ) {
			Message(("No memory for GIF resize buffer"));
		} else {
			fResizeAccumulateBuffer = NewBitMapDevice(bounds, yuv422Format,nil,fTransparent ? kTransparent : kNoTransparentColor);
			if ( fResizeAccumulateBuffer == nil ) {
				DeleteBitMapDevice(fResizeBuffer);
				fResizeBuffer = nil;
				Message(("No memory for GIF resize buffer"));
			}
		}
	}
	if ( bounds.bottom-bounds.top > 1 )
		fBitMap->filter = fFilterType;	// anti-flicker filter
	
	size = fBitMap->rowBytes * (fBitMap->bounds.bottom - fBitMap->bounds.top);

	uchar fillIndex = fTransparent ? fTransparencyIndex : fBackGroundColor;
	
	if ( fBitMap->format == index4Format )
		fillIndex = fillIndex | (fillIndex<<4);

	memset(fBitMap->baseAddress,fillIndex,size);
	fPass = fYPos = 0;
	fXPos = 0;
	fFilterTop = fFilterBottom = 0;
	used = InitLZW(data,dataLength);

	if ( used > 0 )
		fPhase = kLocalImageBody;		// Start reading the body
	return used;
}


void GIF::SetDrawRectangle(const Rectangle *r)
{
	fDrawRectangle = *r;
	SetRectangle(fDrewRectangle,0,0,0,0);
}

void GIF::SetClipRectangle(const Rectangle *r)
{
	fClipRectangle = *r;
}

void GIF::DrawStrip()
{
	Rectangle src;
	Rectangle dst;
	long 	h = 0;
	long	srcHeight;
	long	fullDstHeight = fDrawRectangle.bottom-fDrawRectangle.top;
	
	if ( fSrcBounds.right-fSrcBounds.left )
		srcHeight = fSrcBounds.bottom - fSrcBounds.top;
	else
		srcHeight = fImageRectangle.bottom - fImageRectangle.top;
		
	if ( fullDstHeight == 0 )
		return;
	if ( fKeepBitMap  || fCacheBitMap )
		return;
	
	// if interlaced, but not transparent then draw as interlaced

	if (  fInterlaced && !fDataComplete  ) 
	{
		switch ( fPass )
		{
		case 0:
			h = 7;
			break;
		case 1:
			h = 3;
			break;
		case 2:
			h = 1;
			break;
		case 3:
			h = 0;
			break;
		}
		if ( (h+fYPos) > fLocalHeight-1 )
			h = (fLocalHeight-1) - fYPos;
	}
	
	// if not scaled, then draw as we get the data

	fDidFilter = false;
	fBitMap->filter = kNoFilter;	// filter is done at end
		
	SetupDrawRectangles(src,dst);

	if ( src.bottom-src.top == dst.bottom-dst.top )
	{
		src.top = 0;
		src.bottom = 1;
		dst.top += fYPos;
		dst.bottom = dst.top + 1 + h;
		DrawImage(gScreenDevice, *fBitMap, dst, fTransparency, &fClipRectangle, &src,fFlipHorizontal,fFlipVertical);
		IntersectRectangle(&dst,&fClipRectangle);
		UnionRectangle(&fDrewRectangle,&dst);
		fNotDrawn = false;
	} else {
		src.top = 0;
		src.bottom = 1;
		dst.bottom = dst.top + SCALE_ORD(fYPos+1+h,fullDstHeight,fHeight);
		dst.top += SCALE_ORD(fYPos,fullDstHeight,fHeight);
		if ( fResizeBuffer && h == 0  && !fInterlaced ) {
			if ( fDstYInc == 0 ) {
				fDstYPos = IntToFrac(dst.top);
				fDstYInc = IntToFrac(fullDstHeight) / srcHeight;
				fDstYLast = fDstYPos;
				ClearYUYVScanLine((ushort *)fResizeAccumulateBuffer->baseAddress,fWidth,false);
			}
			fDstYLast = fDstYPos;
			fDstYPos += fDstYInc;
			if  ( srcHeight > fullDstHeight  ) {
				if ( fTransparent )
					ClearYUYVScanLine((ushort *)fResizeBuffer->baseAddress,fWidth,true);
				DrawImage(*fResizeBuffer,*fBitMap, fResizeBuffer->bounds,0,nil,&src);
				if ( FracToInt(fDstYPos) > FracToInt(fDstYLast) ) {
					Rectangle lastLine = dst;
					lastLine.top = FracToInt(fDstYLast);
					lastLine.bottom = lastLine.top+1;
					AddYUYVScanLine((ushort *)fResizeBuffer->baseAddress,(ushort *)fResizeAccumulateBuffer->baseAddress,fWidth,fTransparent,kFractOne - FracMan(fDstYLast));
					DrawImage(gScreenDevice,*fResizeAccumulateBuffer, lastLine,fTransparency,&fClipRectangle,&fResizeAccumulateBuffer->bounds,fFlipHorizontal,fFlipVertical);
						IntersectRectangle(&lastLine,&fClipRectangle);
						UnionRectangle(&fDrewRectangle,&lastLine);
					ClearYUYVScanLine((ushort *)fResizeAccumulateBuffer->baseAddress,fWidth,false);
					AddYUYVScanLine((ushort *)fResizeBuffer->baseAddress,(ushort *)fResizeAccumulateBuffer->baseAddress,fWidth,fTransparent,FracMan(fDstYPos));
				} else {
					AddYUYVScanLine((ushort *)fResizeBuffer->baseAddress,(ushort *)fResizeAccumulateBuffer->baseAddress,fWidth,fTransparent,fDstYInc);
				}
			} else {				
				Rectangle updateRect;			
				dst.top = FracToInt(fDstYLast);
				dst.bottom = dst.top+1;
				long weight = FracMan(fDstYLast);
				if ( weight ) {
					weight *= 256;
					weight >>= kFract;
					if ( fTransparent )
						ClearYUYVScanLine((ushort *)fResizeAccumulateBuffer->baseAddress,fWidth,true);
					DrawImage(*fResizeAccumulateBuffer,*fBitMap, fResizeAccumulateBuffer->bounds,0,nil,&src);
					BlendYUYVScanLine((ushort *)fResizeBuffer->baseAddress,(ushort *)fResizeAccumulateBuffer->baseAddress,fWidth,fTransparent,weight);
					DrawImage(gScreenDevice,*fResizeAccumulateBuffer,dst,fTransparency,&fClipRectangle,&fResizeAccumulateBuffer->bounds,fFlipHorizontal,fFlipVertical);
					updateRect = dst;
					IntersectRectangle(&updateRect,&fClipRectangle);
					UnionRectangle(&fDrewRectangle,&updateRect);
					dst.top++;
				}
				if ( fTransparent )
					ClearYUYVScanLine((ushort *)fResizeBuffer->baseAddress,fWidth,true);
				DrawImage(*fResizeBuffer,*fBitMap, fResizeBuffer->bounds,0,nil,&src);
				while ( dst.top < FracToInt(fDstYPos) ) {
					dst.bottom = dst.top+1;
					DrawImage(gScreenDevice,*fBitMap, dst,fTransparency,&fClipRectangle,&src,fFlipHorizontal,fFlipVertical);
					updateRect = dst;
					IntersectRectangle(&updateRect,&fClipRectangle);
					UnionRectangle(&fDrewRectangle,&updateRect);
					dst.top++;
				}
			}
		} 
		else  {
			DrawImage(gScreenDevice,*fBitMap, dst,fTransparency,&fClipRectangle,&src,fFlipHorizontal,fFlipVertical);
			IntersectRectangle(&dst,&fClipRectangle);
			UnionRectangle(&fDrewRectangle,&dst);
		}
		fNotDrawn = false;
	}
}


void GIF::DisposeImage()
{
	uchar r,g,b;
	Color eraseColor = 0;		
	
	if ( fDisposalMethod == kDisposalNotSpecified )		// really ?? 
		return;
	if ( fDisposalMethod == kRestoreToPrevious )
		return;
	if ( fDisposalMethod && fDisposalMethod != kDoNotDispose && (fLastDrawRectangle.right - fLastDrawRectangle.left) > 0 ) 
	{
		Rectangle eraseRect = fLastDrawRectangle;
		
		if ( !fTransparent )
		{
			if ( fLocalColorTable ) 
				LookUpRGB(fBackGroundColor,fLocalColorTable,r,g,b);
			else if ( fGlobalColorTable ) 
				LookUpRGB(fBackGroundColor,fGlobalColorTable,r,g,b);
			else
				r = g = b = 0;
			eraseColor = ((ulong)r) << 16;
			eraseColor |= ((ulong)g) << 8;
			eraseColor |= b;
			PaintRectangle(gScreenDevice,eraseRect,eraseColor,0,&fClipRectangle);
			IntersectRectangle(&eraseRect,&fClipRectangle);
			UnionRectangle(&fDrewRectangle,&eraseRect);
		} 
		else
		{
			IntersectRectangle(&eraseRect,&fClipRectangle);

			if ( !fIsBackground && gPageViewer && gPageViewer->GetDocument() ) 
				gPageViewer->GetDocument()->DrawBackground(&eraseRect);
			else {
				eraseColor = kBlackColor;
				if (gPageViewer != nil)
					eraseColor = gPageViewer->GetDocument()->GetBackgroundColor();
				PaintRectangle(gScreenDevice,eraseRect,eraseColor,0,&fClipRectangle);
				IntersectRectangle(&eraseRect,&fClipRectangle);
			}
			UnionRectangle(&fDrewRectangle,&eraseRect);
		}
		SetRectangle(fLastDrawRectangle,0,0,0,0);
	}
}

long GIF::FillBuffer(const uchar *src,long dataLength)
{
	short i;
	long added = 0;
	
	
	if ( fEndOfLZW )
		return 0;
	
	if ( dataLength < 1 )
		return -1;

	// If Buffer is nearly empty .. put more data into it
	if (fBufferCount < 4) {				
		if (dataLength <= *src)
			return -1;				// Run out of data, return
		for (i = 0; i < fBufferCount; i++)
			fBuffer[i] = *fData++;
		added = fBufferCount = *src++;
		added += 1;
		if (fBufferCount == 0)
		{
			fBufferCount = 8;		// Lie so writing will finish
			fEndOfLZW = true;
		}
		else
		{
//			Assert(fBuffer + i+fBufferCount < fBuffer+260);
			CopyMemory(src,fBuffer + i,fBufferCount);
		}
		fBufferCount += i;
		fData = fBuffer;
	}
	return added;
}


long GIF::SkipData(Byte *data,long dataLength )
{
	long count;
	uchar *src = data;
	
	if ( dataLength < 1 )
		return 0;
	while ( 1 ) {
		if ( dataLength < 1  )
			break; 
		if ( dataLength < *src  )
			break; 
		count = *src++;
		if ( count == 0 ) {
			fEndOfLZW = true;
			Reset(fKeepBitMap||fCacheBitMap);
			fPhase = kReadExtension;
			break;
		}
		src += count;
		dataLength -= count-1;
	}
	return src - data;
}



long GIF::ReadImageBody(Byte *data, long dataLength)
{
	// Read and draw the image body

	short	p = 0,q;
	long	bytesRead = 0;
	Byte	*src = data;
	ushort  	maxY = MIN(fLocalHeight,fHeight);
	Rectangle 	updatedRectangle;
	
	SetRectangle(updatedRectangle,0,0,0,0);
	if (fDidFilter)
		fFilterTop = fFilterBottom = fYPos;
		
	//ImportantMessage(("IN: fYPos == %d", fYPos));
	
	while (fYPos < maxY)
	{
		// Write as many pixels as we can with the data in the buffer
		
		if ( fBufferCount >= 4 ) {
			if ( (fYPos + fTop) < updatedRectangle.top )
				updatedRectangle.top = (fYPos + fTop);
			if ( (fYPos + fTop - 1) > updatedRectangle.bottom )
				updatedRectangle.bottom = (fYPos + fTop-1);
			updatedRectangle.left = fLeft;
			updatedRectangle.right = fLeft + fLocalWidth;
			if ( fBitMap->format == index4Format )
			{
				Byte *dst = fBitMap->baseAddress + (fXPos + fLeft)/2;
				if ( fBitMap->bounds.bottom != 1 ) 
					dst += (fYPos + fTop) * fBitMap->rowBytes;
				
				if ( ((fXPos + fLeft)&1) != 0 ) 
				{
					if ((q=LZWPixel()) < 0) 
					{
						Complain(("lz decode error"));
						goto bail;
					}
#ifdef	SIMULATOR
#define	BLAST_CHECK
#endif
#ifdef BLAST_CHECK
					Assert( (dst-1) >= fBitMap->baseAddress );
					Assert( (dst-1) < fBitMap->baseAddress + fBitMap->rowBytes * ( fBitMap->bounds.bottom- fBitMap->bounds.top));
#endif
					dst[-1] |= q;
					fXPos++;
				}
				for ( ;fXPos < fLocalWidth && (fBufferCount >= 4) ; )
				{
					if ((p = LZWPixel()) < 0) 
					{
						Complain(("lz decode error"));
						goto bail;
					}
					fXPos++;
					q = 0;
					if ( fXPos < fLocalWidth )
					{
						if ((q = LZWPixel()) < 0)
						{
							Complain(("lz decode error"));
							goto bail;
						}
						fXPos++;
					}
					if ( fXPos <= fWidth )
					{
#ifdef BLAST_CHECK
						Assert( dst >= fBitMap->baseAddress );
						Assert( dst < fBitMap->baseAddress + fBitMap->rowBytes * ( fBitMap->bounds.bottom- fBitMap->bounds.top));
#endif
						*dst++ = q + (p<<4);
					}
				}
			}
			else
			{
				Byte *dst = fBitMap->baseAddress + fXPos + fLeft;
				if ( fBitMap->bounds.bottom != 1 ) 
					dst += (fYPos + fTop) * fBitMap->rowBytes;
				for (;fXPos < fLocalWidth && (fBufferCount >= 4); fXPos++)
				{
					if ((p = LZWPixel()) < 0) 
						goto bail;
					if ( fXPos < fWidth )
					{
#ifdef BLAST_CHECK
						Assert( dst >= fBitMap->baseAddress );
						Assert( dst < fBitMap->baseAddress + fBitMap->rowBytes * ( fBitMap->bounds.bottom- fBitMap->bounds.top));
#endif
	 					*dst++ = p;
	 				}
				}
			}
		}
		else if ( fEndOfLZW ) {
			goto bail;
		}
	
		if (fXPos == fLocalWidth) {
			
			if (fYPos < fFilterTop)
				fFilterTop = fYPos;
			if (fYPos > fFilterBottom)
				fFilterBottom = fYPos;
			if ( !(fKeepBitMap || fCacheBitMap) )
			{
				Boolean drawIt;
				
				if ( fSrcBounds.right-fSrcBounds.left )
					drawIt =  ( fYPos >= fSrcBounds.top && fYPos < fSrcBounds.bottom );
				else
					drawIt =  ( fYPos >= fImageRectangle.top && fYPos < fImageRectangle.bottom );
				if ( drawIt )
					DrawStrip();
	
			}
			fXPos = 0;						// Move to the next line
			if ( fInterlaced )
			
			// interlaced GIF
			
			{
				if (fPass < 3 && fYPos < (fLocalHeight-1))
				{
					if ( fKeepBitMap || fCacheBitMap )
					{
						CopyMemory(fBitMap->baseAddress + (fYPos + fTop)*fBitMap->rowBytes,fBitMap->baseAddress + ((fYPos + fTop)+1)*fBitMap->rowBytes,  fBitMap->rowBytes);
						if ( (fYPos + fTop+1) > updatedRectangle.bottom )
							updatedRectangle.bottom = (fYPos + fTop+1);
					}
				}
				switch (fPass)
			
				{			// Draw interlaced
					case 0:
					case 1:	fYPos += 8;	break;
					case 2:	fYPos += 4;	break;
					case 3: fYPos += 2;	break;
				}
				if (fYPos >= fLocalHeight)
				{
					switch (++fPass)
					{
						case 1:	fYPos = 4; break;
						case 2:	fYPos = 2; break;
						case 3:	fYPos = 1; break;
					}
				}
				if ( (fYPos + fTop) > updatedRectangle.bottom )
					updatedRectangle.bottom = (fYPos + fTop);
			}
			else {
				fYPos++;					// Draw Progressive
				if ( (fYPos + fTop) > updatedRectangle.bottom )
					updatedRectangle.bottom = (fYPos + fTop);
			}
			if (fYPos < fFilterTop)
				fFilterTop = fYPos;
			if (fYPos > fFilterBottom)
				fFilterBottom = fYPos;
		}
		bytesRead = FillBuffer(src,dataLength);
		if ( bytesRead < 0 )
			goto done;
		src += bytesRead;
		dataLength -= bytesRead;
	}

bail:
	if ( fBitMap && fKeepBitMap )
		UnionRectangle(&fDrewRectangle,&updatedRectangle);

	if ( fEndOfLZW || p < 0  )
	{
		if ( IsError(p < 0) ) {
			Reset(true);
			return p;
		}
		ResetDrawing();
		fPhase = kReadExtension;
		if ( fDelayTime )
			fHoldFrameTime = fDelayTime;
	}
done:
	return src - data;
}



void 
GIF::SetupDrawRectangles(Rectangle &srcRect,Rectangle &drawRect)
{
	long dw,dh,t;
	
	drawRect = fDrawRectangle;
	dw = fDrawRectangle.right - fDrawRectangle.left;
	dh = fDrawRectangle.bottom - fDrawRectangle.top;

	if ( fSrcBounds.right-fSrcBounds.left )	
	{	
	
		// for animations ignore gif local image frame
		srcRect = fSrcBounds;
		if ( fFlipHorizontal ) {
			t = srcRect.left;
			srcRect.left = fWidth - srcRect.right;
			srcRect.right = fWidth - t;
		}
		if ( fFlipVertical ) {
			t = srcRect.top;
			srcRect.top = fHeight - srcRect.bottom;
			srcRect.bottom = fHeight - t;
		}
	}
	else
	{
		srcRect = fImageRectangle;
		if ( fFlipHorizontal ) {
			t = srcRect.left;
			srcRect.left = fWidth - srcRect.right;
			srcRect.right = fWidth - t;
		}
		if ( fFlipVertical ) {
			t = srcRect.top;
			srcRect.top = fHeight - srcRect.bottom;
			srcRect.bottom = fHeight - t;
		}
		if ( dw == fWidth && dh == fHeight )
		{
			srcRect.right = srcRect.left + fLocalWidth;
			srcRect.bottom = srcRect.top + fLocalHeight;
			drawRect.left += srcRect.left;
			drawRect.top += srcRect.top;
			drawRect.right = drawRect.left + fLocalWidth;
			drawRect.bottom = drawRect.top + fLocalHeight;
		}
		else
		{
		
			srcRect.right = srcRect.left + fLocalWidth;
			srcRect.bottom = srcRect.top + fLocalHeight;
			drawRect.left += SCALE_ORD(srcRect.left,dw,fWidth);
			drawRect.right = drawRect.left + SCALE_ORD(fLocalWidth,dw,fWidth);
			drawRect.top += SCALE_ORD(srcRect.top,dh,fHeight);
			drawRect.bottom = drawRect.top + SCALE_ORD(fLocalHeight,dh,fHeight);
		}
	}
}

void GIF::DoDraw()
{
	if ( fBitMap && (fKeepBitMap || fCacheBitMap) )
	{
		Rectangle srcRect,drawRect;
		SetupDrawRectangles(srcRect,drawRect);
		DrawImage(gScreenDevice, *fBitMap,drawRect, fTransparency, &fClipRectangle, &srcRect,fFlipHorizontal,fFlipVertical);
		if ( fBitMap->filter != kNoFilter )
			fDidFilter = true;
		fLastDrawRectangle = drawRect;
		IntersectRectangle(&drawRect,&fClipRectangle);
		UnionRectangle(&fDrewRectangle,&drawRect);
	}
}

void GIF::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency,Rectangle *drewRectangle)
// Draw the gif using a transparent mask if required....
{	
	SetRectangle(fDrewRectangle,0,0,0,0);
	if ( invalid == nil  ) 
		invalid = r;
	if ( !EqualRectangles(&fClipRectangle,invalid) )
		fNotDrawn = true;
	fClipRectangle = *invalid;
	if ( !EqualRectangles(&fDrawRectangle,r) )
		fNotDrawn = true;
	fDrawRectangle = *r;
	if ( fTransparency != transparency )
		fNotDrawn = true;
	fTransparency = transparency;

	if ( fFilterType != kNoFilter && !fKeepBitMap && !fCacheBitMap && !fNotDrawn && (!fInterlaced  || fPass == 4) )
	{
		Rectangle		filterBounds = *r;
		static Boolean	specialFilter = true;
		
		if (specialFilter)
		{
			filterBounds.bottom = filterBounds.top + fFilterBottom;
			filterBounds.top += fFilterTop;
		}
	
		if (fFilterTop > fFilterBottom)
		{
			ImportantMessage(("%08x.Filter(%d,%d)", (int)this, fFilterTop, fFilterBottom));
		}
		
		FlickerFilterBounds(gScreenDevice,filterBounds,fFilterType,invalid);
		fDidFilter = true;
		goto done;
	}
	if (fBitMap == nil)
		goto done;
		
#if	0
	if ( fKeepBitMap && fBitMap->format == index8Format )
	{
		long osize= BitMapPixelBufferSize(fBitMap) + fBitMap->colorTable->size;
		long area = (fBitMap->bounds.bottom-fBitMap->bounds.top) * (fBitMap->bounds.right-fBitMap->bounds.left);
		
		if ( osize > 2048 && area < 40000 )
		{
			VectorQuantizer *vq = new VectorQuantizer;
			if ( vq ) 
			{
				vq->Start(fBitMap,nil,nil,area < 4000 ? 128 : 256 );				// 128 codebooks - can tune for image quality

				while ( !vq->Continue(false) )
					;
				
				BitMapDevice *vqbitmap = vq->GetBitMap(true);		// we own it now
				if ( vqbitmap ) 
				{
					long vsize= BitMapPixelBufferSize(vqbitmap) + vqbitmap->colorTable->size;
					if ( osize < vsize )
					{
						DeleteBitMapDevice(fBitMap);
						fBitMap = vqbitmap;
					}
					else
						DeleteBitMapDevice(vqbitmap);
				} else
					DeleteBitMapDevice(vqbitmap);
				delete vq;
			}
		}
	}
#endif

	if ( fNotDrawn && fFrameCount <= 1  ) {
		DoDraw();
//		fNotDrawn = false;
	}
done:
#if	0
	if ( fCacheBitMap && fFrameCount < 1 )
	{
		if ( fBitMap )
		{
			fBitMap->colorTable = nil;
			DeleteBitMapDevice(fBitMap);
		}
		fBitMap = nil;
	}
#endif
	if ( drewRectangle )
		*drewRectangle = fDrewRectangle;
}



#if defined FIDO_INTERCEPT
void GIF::Draw(const Rectangle* bounds, const Rectangle*, ulong transparency, FidoCompatibilityState& fidoCompatibility) const
{
	if (fBitMap == nil)
		return;

	Rectangle		srcBounds;
	GetBounds(&srcBounds);
	fidoCompatibility.DrawImage(gScreenDevice, *fBitMap, *bounds, transparency, &srcBounds);
}
#endif


void GIF::SetFlip(Boolean flipHorizontal,Boolean flipVertical)
{
	fFlipHorizontal = flipHorizontal;
	fFlipVertical = flipVertical;
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GIF::SaveBehind(const Rectangle */*behindBounds*/)
{
}
#endif

