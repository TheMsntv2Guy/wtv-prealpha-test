// ===========================================================================
//	jpMarkers.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __JPINCLUDES_H__
#include "jpIncludes.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif


// ===========================================================================
//	local functions
static void ResetMultiPassState(JPEGDecoder *j);
static Error AllocateBlockBuffer(JPEGDecoder *j);
static void MakeFastHuffmanTable(HuffmanTable *h);
static Error SetupHuffmanTable(HuffmanTable *h);



//====================================================================
// resets the state for doing multiple passes to render progressive JPEGs in slices

static void
ResetMultiPassState(JPEGDecoder *j)
{	
	long i;
	for ( i=0; i < kMaxProgressiveScans; i++ )
		j->scanState[i].set = false;
}



static Error
AllocateBlockBuffer(JPEGDecoder *j)
{
	long Nf;
	CompSpec *cspec;
	
#if	0	
// for debugging
	j->slicesPerPass = 1;
	if ( j->slicesPerPass >= heightMCU )
		j->slicesPerPass = heightMCU-1;
	TrivialMessage(("forcing JPEG slice progression to %d",j->slicesPerPass));	// for testing mulit-pass sliced progressive
#endif

	if ( j->priority > 3 ) 
	{
		long availMem = gRAMCache->GetFreeCount();
		availMem /= 8;

		long totalBlocks = 0;
		for (Nf = 0; Nf < j->CompInFrame; Nf++) 
		{
			cspec = &j->Comp[Nf];
			totalBlocks += j->WidthMCU * cspec->blocksMCU;
		}
	
		j->slicesPerPass = availMem / (totalBlocks * kMCUSize * sizeof(short));
		if ( j->slicesPerPass > 32 )
			j->slicesPerPass = 32;
		if ( j->slicesPerPass == 0 )
			 j->slicesPerPass = 1;			// try anyway
		if ( j->slicesPerPass > j->HeightMCU )
			j->slicesPerPass = 0;
	  	else
	   		j->slicesPerPass = 1;
   	} else
	   	j->slicesPerPass = 1;
   			

	for (Nf = 0; Nf < j->CompInFrame; Nf++) 
	{
		char name[2];
		name[0] = 'A' + Nf;
		name[1] = 0;
		cspec = &j->Comp[Nf];
		if ( j->slicesPerPass == 0 )	
			cspec->blockCount = j->HeightMCU * j->WidthMCU * cspec->blocksMCU;
		else 
			cspec->blockCount = j->slicesPerPass * j->WidthMCU * cspec->blocksMCU;
		long bsize = cspec->blockCount * kMCUSize * sizeof(short);
		
		if ( IsError(cspec->blockBufferCacheEntry != nil) ) {
			goto fail;
		}
		cspec->blockBufferCacheEntry = gRAMCache->Find(j->cacheName, name, strlen(name));
		if (IsWarning(cspec->blockBufferCacheEntry != nil) )
			goto fail;
		cspec->blockBufferCacheEntry = gRAMCache->Add(j->cacheName, name, strlen(name), bsize);
		if (IsWarning(cspec->blockBufferCacheEntry == nil) )
			goto fail;
		cspec->blockBufferCacheEntry->SetDataLength(bsize);
		cspec->blockBufferCacheEntry->BeginUseData();
		cspec->blockBufferCacheEntry->SetPriority(j->priority);
		if ( IsWarning(cspec->blockBufferCacheEntry->GetDataLength() < bsize ) )
			goto fail;
	}
	TrivialMessage(("Doing progressive JPEG with %d slices/pass",j->slicesPerPass));
	return kNoError;

fail:
	Complain(("Could not allocate memory for progressive jpeg block buffer"));
	for (Nf = 0; Nf < j->CompInFrame; Nf++) 
	{
		cspec = &j->Comp[Nf];
		if ( cspec->blockBufferCacheEntry ) 
		{
			cspec->blockBufferCacheEntry->EndUseData();
			gRAMCache->Remove(cspec->blockBufferCacheEntry);
			cspec->blockBufferCacheEntry = nil;
		}
	}
	return kLowMemory;

}


//====================================================================
//	SOF defines Y and X line counts and frame components:
//	Defines h and v subsampling and Q table index for each component
//
//
//	The decoder also only supports several interleaving factors
//	so we check for those too
//
//
//

Error InterpretSOF(JPEGDecoder *j, Byte *data, short size)
{
	short	Nf,i,MaxHi,MaxVi;
	CompSpec	*cspec;
	long dstWidth,dstHeight;
	uchar	*dataEnd = data + size -2;
	long	totalMCUs = 0;
	Boolean	justSizeCheck;
	
	//TrivialMessage(("SOF Marker: %d bytes", size));

	dstWidth = j->fDrawRect.right - j->fDrawRect.left;
	dstHeight = j->fDrawRect.bottom - j->fDrawRect.top;
	justSizeCheck = dstWidth == nil;
	
	if ( IsWarning(*data++ != 8) ) 
	{
		Message(("Only 8-bit precision JPEG is supported"));
		return kBadSOFMarkerErr;				// Only understand 8 bit precision
	}
	
		
	j->Height = *data++;					// Number of vertical lines
	j->Height = (j->Height << 8) | *data++;
	
	
	if ( IsWarning(j->Height == 0) ) 
	{
		// height is defined by DNL marker at end of first scan, we can't handle that now
		Message(("JPEG Images with DNL markers not supported"));
		return kBadSOFMarkerErr;				// Only understand 8 bit precision
	}

	j->Width = *data++;						// Number of horizontal lines
	j->Width = (j->Width << 8) | *data++;

	j->CompInFrame = *data++;				// Number of components in this frame

	if ( IsWarning(j->CompInFrame < 1 || j->CompInFrame > kMaxFrameComponents) )
		return kBadSOFMarkerErr;
	
//	Read the component identifiers, sampling factors and Q table numbers

	MaxHi = -1;
	MaxVi = -1;

	for (Nf = 0; Nf < j->CompInFrame; Nf++) 
	{
		cspec = &j->Comp[Nf];
		
		cspec->Ci = *data++;		// Component identifier

		i = *data++;
		cspec->Hi = (i >> 4);		// Horizontal blocks in MCU
		cspec->Vi = (i & 0xF);		// Vertical blocks in MCU

		i = *data++;			// Q table to use
		
		if ( IsWarning(i >= kMaxQuantizationTables) )
			return kBadSOFMarkerErr;
		
		cspec->QTable = j->QTables[i];

		// per scan info

		cspec->inScan = false;
		cspec->blocksMCU = cspec->Hi*cspec->Vi;
		totalMCUs += cspec->blocksMCU;
		
		if ( IsWarning(totalMCUs > kMaxMCUs) ) 
		{
			Message(("JPEG Unsupported scan interleave > %d blocks",kMaxMCUs));
			return kBadMarkerErr;
		}

		if ( MaxHi < cspec->Hi )
			MaxHi = cspec->Hi;
		if ( MaxVi < cspec->Vi )
			MaxVi = cspec->Vi;
		
		if ( IsWarning(cspec->Hi > 2 || cspec->Vi > 2 ||  ( cspec->Hi == 1 && cspec->Vi == 2 )) ) 
		{
			Message(("JPEG Unsupported scan interleave cspec %d (%d x %d)",	cspec->Ci,cspec->Hi,cspec->Vi));
			return kBadMarkerErr;
		}
	}

	j->WidthMCU = ((j->Width + (MaxHi << 3)-1)/(MaxHi << 3));
	j->HeightMCU = ((j->Height + (MaxVi << 3)-1)/(MaxVi << 3));
	j->MCUHPixels = j->thumbnail ? MaxHi : MaxHi << 3;
	j->MCUVPixels = j->thumbnail ? MaxVi : MaxVi << 3;

	
	// dont allocate if we are just checking size
	
	if ( justSizeCheck ) 
	{
		j->phase = kSkippingToEnd;
		return kNoError;
	}
		
	if ( SetupDrawing(j) != kNoError )
		return kLowMemory;
	
	
	// if bounds is empty, we aren't really drawing yet, just getting the dimensions 
	// otherwise setup the params for clipping by slice and MCU 
	
	if ( dstWidth != 0 ) 
	{
		Rectangle srcClip = j->fClipRectangle;
	
		IntersectRectangle(&srcClip,&j->fDrawRect);

		OffsetRectangle(srcClip,-j->fDrawRect.left,-j->fDrawRect.top);
	
		// translate clip from dest space to source space
		
		if (dstWidth != j->Width || dstHeight != j->Height  ) 
		{
			long scaleH = (j->Width <<16 ) / dstWidth;
			long scaleV = (j->Height <<16) / dstHeight;
			srcClip.right =  (srcClip.right * scaleH) >> 16;
			srcClip.bottom =  (srcClip.bottom * scaleV) >> 16;
			srcClip.left =  (srcClip.left * scaleH) >> 16;
			srcClip.top =  (srcClip.top * scaleV) >> 16;
		}
		j->firstSlice = srcClip.top / j->MCUVPixels;
		j->lastSlice = (srcClip.bottom-1) / j->MCUVPixels;
		j->leftMCU = (srcClip.left / j->MCUHPixels);
		j->rightMCU  = ((srcClip.right / j->MCUHPixels) + 1);
		
			
		//Message(("mcu clipping first %d last %d left %d right %d",j->firstMCU,j->lastMCU,j->leftMCU,j->rightMCU));
		//Message(("slice clipping first %d last %d",j->firstSlice,j->lastSlice));

	
		// progressive JPEG requires the DCT coefficients from
		// all previous scans, so we have to buffer them until all passes
		// are drawn. If there isn't enough memory we have to draw
		// all scans of the image muliple times depending on how many
		// slices worth of DCT data we can buffer
	
		// based on number of blocks we can allocate
		// if we can buffer all the DCT blocks for an image we can 
		// draw it much better and make it look better so it would be 
		// nice to be able to get this memory if we can
		
				
		if ( j->isProgressive && !j->thumbnail ) 
		{
			j->firstSlice = 0;		// cant clip off top for progressive
			long heightMCU = j->HeightMCU;
			if ( (j->lastSlice+1) < heightMCU )
				heightMCU = j->lastSlice+1;

			// on first pass only, allocate the block buffer
			
			if ( j->slicesPerPass == 0 && j->anotherPass == 0 ) 
			{
				if ( AllocateBlockBuffer(j) != kNoError)
					return kLowMemory;
			}
			for (Nf = 0; Nf < j->CompInFrame; Nf++) 
			{
				cspec = &j->Comp[Nf];
				long l = cspec->blockBufferCacheEntry->GetDataLength();
				if (IsWarning(l != cspec->blockCount * kMCUSize * (long)sizeof(*cspec->blockBuffer) ) )
				{
					Complain(("size changed"));
					return kGenericError;
				}
				cspec->blockBuffer = (short *)cspec->blockBufferCacheEntry->GetData();
				if ( IsError(cspec->blockBuffer == nil ) )
				{
					Complain(("no buffer"));
					return kLowMemory;
				}
				ZeroMemory(cspec->blockBuffer,cspec->blockCount * kMCUSize * sizeof(*cspec->blockBuffer) );
			}
		    
	     	if ( j->slicesPerPass ) 
	     	{
	     	
				if ( j->anotherPass == 0 ) 
				{
					j->thisPassFirstSlice = j->firstSlice;
					if ( j->lastSlice > (j->firstSlice + (j->slicesPerPass-1)) ) 
					{ 
						ResetMultiPassState(j);
						j->multiPassScan = 0;
						j->anotherPass = (j->lastSlice-j->firstSlice +(j->slicesPerPass-1)) / j->slicesPerPass;
						j->lastPassSlice = j->firstSlice + (j->slicesPerPass-1);
						TrivialMessage(("progressive JPEG first pass %d to %d",j->firstSlice,j->lastPassSlice));
					}
				} else 
				{
					if ( j->lastPassSlice < j->lastSlice ) 
					{
						j->thisPassFirstSlice = j->firstSlice = j->lastPassSlice+1;
						j->multiPassScan = 0;
						if ( j->lastSlice > (j->firstSlice + (j->slicesPerPass-1)) ) 
						{ 
							j->lastPassSlice = j->firstSlice + (j->slicesPerPass-1);
							TrivialMessage(("progressive JPEG another pass %d to %d",j->firstSlice,j->lastPassSlice));
						} else 
						{
							j->anotherPass = 0;
							j->lastPassSlice = j->lastSlice;
							TrivialMessage(("progressive JPEG last pass %d to %d",j->firstSlice,j->lastPassSlice));
						}
					} 
					else 
					{
						j->multiPassScan = 0;
						j->lastPassSlice = 0;
						j->anotherPass = 0;
						TrivialMessage(("progressive JPEG last empty pass"));
					}
				}
			}
		}
	}
	
		
	// final slice clipping info
	
	j->firstMCU = (j->firstSlice * j->WidthMCU ) + j->leftMCU;
	j->lastMCU = (j->lastSlice * j->WidthMCU ) + j->rightMCU;
	

	j->phase = kWaitingForScan;					// Got the start of frame
	if ( data != dataEnd )
		Message(("whacky SOF marker"));

	DumpJPEGDecode(j);
	return kNoError;
}



//====================================================================
//	SOS defines the number of image components in the scan
//	Defines which component will be the jth component in the scan,
//	along with the DC and AC huffman table number to use with each component

Error InterpretSOS(JPEGDecoder *j, uchar *data, short size)
{
	short	Ns,i,Cs;
	CompSpec*	cspec;
	uchar	*dataEnd = data + size-2;
	
	//TrivialMessage(("SOS Marker: %d bytes", size));
		
	j->CompInScan =	*data++;			// Number of components in this scan
	
	if ( IsWarning( j->CompInScan > kMaxScanComponents) )
		return kBadSOSMarkerErr;
	 
	for (i = 0; i < j->CompInFrame; i++) 
		j->Comp[i].inScan = false;
		
		
	for (Ns = 0; Ns < j->CompInScan; Ns++) 
	{
		Cs = *data++;		// Component selector
		
		cspec = nil;
		for ( i=0; i < j->CompInFrame; i++ )  
		{
			if ( j->Comp[i].Ci == Cs ) 
			{
				cspec = &j->Comp[i];
				break;
			}
		}
		if ( IsWarning(cspec == nil) ) 
			return kBadSOSMarkerErr;
		j->scanComps[Ns] = cspec;
		i = *data++;
		short htd = (i >> 4);		// DC Huffman table number
		short hta = (i & 0xF);		// AC Huffman table number
		if ( IsWarning(htd >= kMaxHuffmanTables) ) 
			return kBadSOSMarkerErr;
		if ( IsWarning(hta >= kMaxHuffmanTables) ) 
			return kBadSOSMarkerErr;
		
		cspec->DCH = (HuffmanTable* )j->HTables[htd];
		cspec->ACH = (HuffmanTable* )j->HTables[hta + 4];
		cspec->inScan = true;
	}
	
	// Progressive JPEG images have two types of progression -
	// spectrum selection
	//  	this encodes parts of the frequency spectrum in seperate scans
	// successive approximation 
	//		this encodes a base level for each DCT coefficient in a scan
	//		and then successive scans adjust each non-zero coefficient or 
	//		add new bits
	// both can be used at the same time
	
	if ( j->isProgressive ) 
	{
		uchar succLow,succHigh;
		uchar specStart,specEnd;
		
		specStart = *data++;
		specEnd = *data++;
		i = *data++;
		succHigh = i>>4;
		succLow = i & 0xf;
		if ( specStart == 0 ) 
		{
			if ( IsWarning(specEnd != 0) )	// Annex G.1.1.1.1		DC components are separate
				return kBadSOSMarkerErr;
				
			
			//Message(("Prog SOS %d DC components spectrum (bit %d to bit %d)",j->CompInScan,succHigh,succLow));
			for (Ns = 0; Ns < j->CompInScan; Ns++) 
			{
				cspec = j->scanComps[Ns];
				//Message(("SOS DC component %d spectrum %d to %d (bit %d to bit %d) last %d",cspec->Ci,specStart,specEnd,succHigh,succLow,cspec->dcBitPos));
				cspec->dcPrevBitPos = succHigh;
				cspec->dcBitPos = succLow;
				cspec->specStart = specStart;
				cspec->specEnd = specEnd;
			}
		} else 
		{
			if ( IsWarning(j->CompInScan != 1) )
				return kBadSOSMarkerErr;
											// Annex G.1.1.1.1		Only dc can be interleaved
			cspec = j->scanComps[0];
			//Message(("SOS AC component %d spectrum %d to %d (bit %d to bit %d) last %d",	cspec->Ci,specStart,specEnd,succHigh,succLow,cspec->acBitPos));
			cspec->acPrevBitPos = succHigh;
			cspec->acBitPos = succLow;
			cspec->specStart = specStart;
			cspec->specEnd = specEnd;
		}
	} else 
	{
		data += 3;		
	}
	if ( IsWarning(data != dataEnd) )
		return kBadSOSMarkerErr;		
		
	DumpJPEGDecode(j);
	
	j->phase = kProcessingScan;					// Now in the main scan
	j->RstMarker = kSOS;				// Trigger an interval for DecodeScan
	return kNoError;					
}

//====================================================================
//
//	GenerateQScale is used to create QScale. QScale is a 8:8 fixed point
//	zig-zag ordered array that is multiplied by the incoming Q table to
//	compensate for the scaled IDCT
//


#if 0

#define ks0 0.176776695
#define ks1 0.127448894
#define ks2 0.135299025
#define ks3 0.150336221
#define ks4 0.176776695
#define ks5 0.224994055
#define ks6 0.326640741
#define ks7 0.640728861

void GenerateQScale()
{	
	double		v[8],scale[64];
	short		x,y;
	
	v[0] = 1/ks0;
	v[1] = 1/ks1;
	v[2] = 1/ks2;
	v[3] = 1/ks3;
	v[4] = 1/ks4;
	v[5] = 1/ks5;
	v[6] = 1/ks6;
	v[7] = 1/ks7;
	for (y = 0; y < kMCUHeight; y++) {
		for (x = 0; x < kMCUWidth; x++)
			scale[x + (y << 3)] = v[x];
	}
	for (x = 0; x < kMCUWidth; x++) {
		for (y = 0; y < kMCUHeight; y++)
			scale[x + (y << 3)] = scale[x + (y << 3)] * v[y];
	}
	for (y = 0; y < kMCUHeight; y++) {
		for (x = 0; x < kMCUWidth; x++)
		{	Message(("%5d,",(long)(256*scale[ZZ[x + (y << 3)]]))); }
		Message((""));
	}
}

/*
	Non ZigZag version of QScale
	
	 8192,11362,10703, 9632, 8192, 6436, 4433, 2260,
	11362,15760,14845,13361,11362, 8927, 6149, 3134,
	10703,14845,13984,12585,10703, 8409, 5792, 2953,
	 9632,13361,12585,11326, 9632, 7568, 5213, 2657,
	 8192,11362,10703, 9632, 8192, 6436, 4433, 2260,
	 6436, 8927, 8409, 7568, 6436, 5057, 3483, 1775,
	 4433, 6149, 5792, 5213, 4433, 3483, 2399, 1223,
	 2260, 3134, 2953, 2657, 2260, 1775, 1223,  623
*/

#endif

#ifdef kScaledDCT


ushort	QScale[64] = {
	 8192,11362,11362,10703,15760,10703, 9632,14845,
	14845, 9632, 8192,13361,13984,13361, 8192, 6436,
	11362,12585,12585,11362, 6436, 4433, 8927,10703,
	11326,10703, 8927, 4433, 2260, 6149, 8409, 9632,
	 9632, 8409, 6149, 2260, 3134, 5792, 7568, 8192,
	 7568, 5792, 3134, 2953, 5213, 6436, 6436, 5213,
	 2953, 2657, 4433, 5057, 4433, 2657, 2260, 3483,
	 3483, 2260, 1775, 2399, 1775, 1223, 1223, 623
};

#endif

//====================================================================
//	DQT defines the quantization tables
//	A common encoder error is to omit the Q table number

Error InterpretDQT(JPEGDecoder *j, uchar *data, short size)
{
	short		q, i;
	ushort		*QTable;
	ushort		scaledQ;
	
	//Message(("DQT Marker: %d bytes",size));

	uchar *end = data + size - 2;
	while (data < end) 
	{
		q = *data++;
		
		//Message(("DQT id %d size %d",q & 0x03,q & 0x10 ? 16 : 8));
		
		i =  ( q & 0xf);
		if ( IsError(i > 3) ) 
		{
			Message(("Bad DQT Marker"));
			return kBadMarkerErr;
		}
		QTable = j->QTables[i];			// Quantization table number

		i =  (q & 0xf0);
		if ( i == 0 ) 
		{
			for (i = 0; i < kMCUSize; i++)  
			{
				scaledQ = *data++;
#ifdef kScaledDCT
				scaledQ = (((long)(scaledQ * QScale[i])) + (1<<(I_SHIFT_F-1)) ) >> I_SHIFT_F;		// 8 by 8
#endif
				QTable[i] = scaledQ;
			}
		} 
		else if ( i == 0x10 ) 
		{
		
			// these are illegal for 8-bit JPEG, but try to deal with them anyway
			// since some encoders make them
			
			for (i = 0; i < kMCUSize; i++)  
			{
			
				scaledQ = *data++;
				scaledQ <<= 8;
				scaledQ |= *data++;
	
				if ( scaledQ > 0xff )
					scaledQ = 0xff;
				
				//Message(("16 bit qtab %X -> %X",scaledQ,(((long)(scaledQ * QScale[i])) + (1<<(I_SHIFT_F-1)) ) >> I_SHIFT_F));

#ifdef kScaledDCT
				scaledQ = (((long)(scaledQ * QScale[i])) + (1<<(I_SHIFT_F-1)) ) >> I_SHIFT_F;		// 8 by 8
#endif
				QTable[i] = scaledQ;
			}
		} 
		else 
		{
			if ( IsError(1) ) 
			{
				Message(("Bad DQT Marker"));
				return kBadMarkerErr;
			}
		} 
	}
	return kNoError;
}

//==================================================================
//		
//	MakeFastHuffmanTable
//
//	Creates a 256 entry table that speeds up the huffman decode
//	Each entry in the table may describe an entire symbol, it may
//	describe an full code, of it may fail ot describe either.
//
// еее note: could extend this for progressive JPEG successive approximation codes

static void
MakeFastHuffmanTable(HuffmanTable *h)
{
	int	l,p,v;
	ushort 	code8,code,s,r;
	
	for (code8 = 0; code8 < 256; code8++) 
	{
		l = 1;
		code = code8 >> 7;								// GetBit from code8
		while ((code > h->maxcode[l]) && (l < 8)) 
		{
			l++;
			code = code8 >> (8-l);
		}
		if ((code > h->maxcode[l]) || (code == 0xFF)) 	// 8 bits did not make a code
		{	
			h->fast[code8].length = 0;					// Unknown code
			h->fast[code8].run = 0;						// Unknown run
			h->fast[code8].value = 0;					// Unknown value
		} 
		else 
		{
			p = h->valptr[l] + code - h->mincode[l];	// code was <= 8 bits long
			code = h->huffval[p];
			s = (code & 0xF);							// scale of symbol
			r = (code >> 4);							// run length
			if ((l + s) <= 8) 							// See if symbol fits in 8 bits
			{						
				v = (code8 >> (8 - (l + s))) & (0xFF >> (8 - s));
				h->fast[code8].length = l + s;			// Total length of symbol
				h->fast[code8].run = r;					// Zero run
				h->fast[code8].value = Extend(v,s);		// Decoded symbol value
			} 
			else 
			{
				h->fast[code8].length = -l;				// -ve Total length of symbol
				h->fast[code8].run = r;					// Zero run
				h->fast[code8].value = s;				// value set to scale
			}
		}
	//	Message(("FastHuffTable %X = %d %d %d",code8,h->fast[code8].length,	h->fast[code8].run,h->fast[code8].value));
	}
}

//====================================================================

static Error
SetupHuffmanTable(HuffmanTable *h)
{
  	uInt16		p,size,code,i;
	uInt16		huffcode[257];
	Byte		huffsize[257];
  
//	Figure C.1 - Generation of Huffman code sizes

	p = 0;
	for (size = 1; size <= 16; size++) 
	{
		for (i = 1; i <= h->bits[size]; i++) 
		{
			if ( IsWarning(p > 256) )
				return kBadMarkerErr;
			huffsize[p++] = size;
		}
	}
	if ( IsWarning(p > 256) )
		return kBadMarkerErr;
	huffsize[p] = 0;
	
//	Figure C.2 - Generation of Huffman codes

	code = 0;
	size = huffsize[0];
	p = 0;
	while (huffsize[p]) 
	{
		while (huffsize[p] == size) 
		{
			if ( IsWarning(p > 256) )
				return kBadMarkerErr;
			huffcode[p++] = code;
			code++;
		}
		code <<= 1;
		size++;
	}

//	Figure F.15: Decoder table generation

	p = 0;
	for (size = 1; size <= 16; size++) 
	{
		if (h->bits[size])
		{
			h->valptr[size] = p;				// huffval[] index of first value of this size
			h->mincode[size] = huffcode[p];		// minimum code this size
			p += h->bits[size];
			h->maxcode[size] = huffcode[p-1];	// maximum code this size
		} 
		else 
			h->maxcode[size] = -1;
	}
	h->maxcode[17] = 0xFFFFF;
	MakeFastHuffmanTable(h);
	return kNoError; 
}


//====================================================================
//	DHT defines the huffman tables

Error InterpretDHT(JPEGDecoder *j, Byte *data, short size)
{
	short			n,i,c,Tc,Th;
	HuffmanTable	*H;

	//Message(("DHT Marker: %d bytes",size));
	size -= 2;				// Total size of huffman table data
	while (size > 0) 
	{
		i = *data++;
		Tc = (i >> 4);					// Table class
		Th = (i & 0xF);					// Table identifier
		
		//Message(("DHT class %d id %d",Tc,Th));
		
		if ( IsError(Tc > 1) )  
		{
			Message(("invalid huffman table"));
			return kBadMarkerErr;
		}
		if ( j->isProgressive ) 
		{
			if ( IsError(Th > 3) ) 
			{
				Message(("invalid huffman table id"));
				return kBadMarkerErr;
			}
		} 
		else 
		{ 
			if ( IsError(Th > 1) ) 
			{
				Message(("invalid huffman table id"));
				return kBadMarkerErr;
			}
		}
		
		
		H = (HuffmanTable *)j->HTables[(Tc << 2) + Th];	// Huffman tables (DC 0, DC 1, AC 0, AC 1)
		c = 0;
		for (n = 1; n < 17; n++) 
		{
			i = *data++;				// Number of huffman codes of length i
			H->bits[n] = i;				// fill in bits
			c += i;						// Add to total number of codes				
		}
		if ( IsWarning(c > 255) )
			return kBadMarkerErr;
		for (n = 0; n < c; n++) 
			H->huffval[n] = *data++;	// Write it to the huffman table
		size -= (17 + c);				// Size of this table
		if ( SetupHuffmanTable(H) != kNoError )		// Create tables for decode
			return kBadMarkerErr;
	}
	return kNoError;
}

//====================================================================

//	DRI defines the restart interval

Error InterpretDRI(JPEGDecoder *j, Byte *data, short UNUSED(size))
{
	//Message(("DRI Marker: %d bytes",size));
	j->RstInterval = *data++;			// 16 bit restart interval
	j->RstInterval = (j->RstInterval << 8) | *data++;
	TrivialMessage(("JPEG restart interval %d",j->RstInterval));
	
	return kNoError;
}

//	APP is an application specific marker

Error InterpretAPP(JPEGDecoder*, Byte* UNUSED(data), short UNUSED(size))
{
	// could check for JFIF and use the thumbnail if it's there - but
	// its probably better to strip that during transcoding since
	// we can't depend on it anyway and it takes up space
	//Message(("APP Marker: %d bytes",size));
	return kNoError;
}

//	COM is a comment

Error InterpretCOM(JPEGDecoder*, Byte* UNUSED(data), short UNUSED(size))
{
	//Message(("COM Marker: %d bytes",size));
	return kNoError;
}

