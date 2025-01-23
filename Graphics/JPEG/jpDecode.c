// ===========================================================================
//	jpDecode.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __JPINCLUDES_H__
#include "jpIncludes.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif




// ===========================================================================
static Error InitJPEGDecoder(JPEGDecoder* j);
static void ResetJPEGDecoder(JPEGDecoder *j);
static void EndJPEGImage(JPEGDecoder *j);
static void PurgeJPEGDecoder(JPEGDecoder* j);
static void ResetScan(JPEGDecoder* j);
static void SetupScan(JPEGDecoder* j);
static void BeginInterval(JPEGDecoder* j);
static void SaveState(JPEGDecoder *j);
static Boolean RestoreState(JPEGDecoder *j);
static Error DecodeScan(JPEGDecoder *j, short marker);
static long SkipScan(JPEGDecoder* j, uchar* data, long dataLength);
static long ScanWrite(JPEGDecoder* j, uchar* data, long dataLength);
static long NextMarker(uchar *data,long dataLength,uchar *rMarker,ushort *rSize);
// ===========================================================================



#define	MHKMessage(x)	

//#define	MHKMessage(x)	Message(x)

// ===========================================================================

void DumpJPEGDecode(JPEGDecoder* j)
{
	j=j;	// compiler
	
//#define	I_LIKE_DUMPS
#ifdef	I_LIKE_DUMPS
	short	n;
	Message(("Width: %d, Height: %d, Restart interval: %d", j->Width, j->Height, j->RstInterval));
	Message(("First Slice: %d Last Slice %d", j->firstSlice,j->lastSlice));
	if ( j->isProgressive ) 
	{
		Message(("Slices Per Pass: %d Passes to go %d", j->slicesPerPass,j->anotherPass));
	}
	Message(("Clip: %d %d %d %d", j->fClipRectangle.left, j->fClipRectangle.top,j->fClipRectangle.right,j->fClipRectangle.bottom));
	Message(("MCUWidth: %d, MCUHeight: %d, MCU ( %d x %d )", j->WidthMCU, j->HeightMCU, j->MCUHPixels,j->MCUVPixels));
	Message((""));
	Message(("Components in Frame: %d", j->CompInFrame));
	Message(("Components in Scan: %d", j->CompInScan));
	for (n = 0; n < j->CompInFrame; n++) 
	{
		if ( j->Comp[n].inScan ) Message(("In Scan"));
		Message(("    Component identifier     (Ci): %d", j->Comp[n].Ci));
		Message(("    Horizontal blocks in MCU (Hi): %d", j->Comp[n].Hi));
		Message(("    Vertical blocks in MCU   (Vi): %d", j->Comp[n].Vi));
		Message(("    Blocks in MCU     	   (Ta): %d", j->Comp[n].blocksMCU));
		Message(("    Blocks in image     	   (Ta): %d", j->Comp[n].blockCount));
		Message((""));
	}
#endif
}


// ===========================================================================

static Error InitJPEGDecoder(JPEGDecoder* j)
{
	static ushort gInstanceCount = 0;
	
	Byte	*HTableData;
	uInt16	*QTableData;
	int		n;
	
	
	ZeroMemory((uchar *)j,sizeof(JPEGDecoder));
	
	// we could allocate these as we get them to save some memory 
															// Set everything to zero	
	QTableData = (uInt16*)AllocateTaggedMemory(kMCUSize*kMaxQuantizationTables*sizeof(uInt16), "JPEG Q Table");					// Up to 4 Q tables
	HTableData = (Byte*)AllocateTaggedMemory(sizeof(HuffmanTable)*kMaxHuffmanTables, "JPEG Huffman Table");	// 4 DC and 4 AC tables

	if (!HTableData || !QTableData) 
		return kLowMemory;
		
	for (n = 0; n < kMaxHuffmanTables; n++) 
	{											// Setup pointers to Q and H tables
		j->HTables[n] = HTableData;
		HTableData += sizeof(HuffmanTable);
	}
	for (n = 0; n < kMaxQuantizationTables; n++) 
	{											// Setup pointers to Q and H tables
		j->QTables[n] = QTableData;
		QTableData += kMCUSize;
	}
	j->inBuffer = 0;
	j->data = j->dataEnd = 0;
	j->CompInFrame = 0;
	j->CompInScan = 0;
	j->WriteBuffer = (Byte*)AllocateTaggedMemory(kJPEGBufferSize, "JPEG Write Buffer");
	if (j->WriteBuffer == nil)
		return kLowMemory;
	j->cacheName[0] = 'J';
	j->cacheName[1] = 'P';
	j->cacheName[2] = 'E';
	j->cacheName[3] = 'G';
	j->cacheName[4] = 'A' + (gInstanceCount & 0xf);
	j->cacheName[5] = 'A' + ((gInstanceCount>>4) & 0xf);
	j->cacheName[6] = 'A' + ((gInstanceCount>>8) & 0xf);
	j->cacheName[7] = 0;
	j->fWhiteLevel = 255;
	gInstanceCount++;
	return kNoError;
}


static void ResetJPEGDecoder(JPEGDecoder *j)
{
	j->data = j->dataEnd = 0;
	j->bits = 0;
	j->phase = kWaitingForNewImage;
	ResetScan(j);
}

static void EndJPEGImage(JPEGDecoder *j)
{
	MHKMessage(("JPEG end of image"));
	ResetJPEGDecoder(j);
	j->phase = kSkippingToEnd;
}


JPEGDecoder* NewJPEGDecoder(const Rectangle* r, short thumbnail, DrawRowProcPtr drawProc , 
	const Rectangle* invalid, ulong transparency,BitMapDevice *device)
{
	JPEGDecoder* j;

	j = (JPEGDecoder*)AllocateTaggedMemory(sizeof(JPEGDecoder), "JPEG Decoder");
	if ( j ) 
	{ 
		if ( InitJPEGDecoder(j) != kNoError ) 
		{
			DisposeJPEGDecoder(j);
			return nil;
		}
		j->fDrawRect = *r;
		j->thumbnail = thumbnail;
		j->SingleRowProc = drawProc;
		j->fClipRectangle = (invalid == nil) ? *r : *invalid;
		j->fTransparency = transparency;
		j->fDrawDevice = device;
	}

	return j;
}


static void PurgeJPEGDecoder(JPEGDecoder* j)
{
	long i;
	
	if (j->DrawRefCon)
		DeleteBitMapDevice(j->DrawRefCon);
	if (j->fResizeBuffer)
		DeleteBitMapDevice(j->fResizeBuffer);
	if (j->fResizeAccumulateBuffer)
		DeleteBitMapDevice(j->fResizeAccumulateBuffer);
	if (j->QTables[0] != nil)
		FreeTaggedMemory(j->QTables[0], "JPEG Q Table");
	if (j->HTables[0] != nil)
		FreeTaggedMemory(j->HTables[0], "JPEG Huffman Table");
	for (i = 0; i < kMaxFrameComponents; i++) 
	{
		if ( j->Comp[i].blockBufferCacheEntry )
		{
			j->Comp[i].blockBufferCacheEntry->SetDataLength(0);
			j->Comp[i].blockBufferCacheEntry->EndUseData();
			gRAMCache->Remove(j->Comp[i].blockBufferCacheEntry);
			j->Comp[i].blockBufferCacheEntry = nil;
		}
	}
	for (i = 0; i < kNumDecompressBuffers; i++)
		if (j->buffer[i] != nil)
			FreeTaggedMemory(j->buffer[i], "JPEG Buffer");
	if (j->WriteBuffer != nil)
		FreeTaggedMemory(j->WriteBuffer, "JPEG Write Buffer");

}

//====================================================================
//		Don't call this on an uninitalized JPEGDecoder

void	DisposeJPEGDecoder(JPEGDecoder* j)
{
	PurgeJPEGDecoder(j);
	FreeTaggedMemory(j, "JPEG Decoder");
}



static void ResetScan(JPEGDecoder* j)
{
	j->inBuffer = 0;
	j->scanBitsToSkip = 0;
	j->scanBitsUsed = 0;
	j->currentSlice = 0;
	j->MCUCount = 0;
	j->bandSkip = 0;
	j->skipToFirstSlice = false;
	j->SkipToRestartMarker = 0;
	j->didDraw = 0;
	j->Interval = 0;
}

static void SetupScan(JPEGDecoder* j)
{
	ResetScan(j);
	if ( j->CompInScan == 1 ) 
	{	
		// non-interleaved unused MCU blocks not included
		
		long bh,bv;
		long sWidthMCU,sHeightMCU;
			
		bh = j->MCUHPixels;
		bv = j->MCUVPixels;
		
		if ( j->thumbnail ) 
		{
			bh <<= 3;
			bv <<= 3;
		}
		bh /= j->scanComps[0]->Hi;
		bv /= j->scanComps[0]->Vi;
		sWidthMCU = (j->Width + bh-1)/bh;
		sHeightMCU = (j->Height + bv-1)/bv;

		j->ScanMCUs = sWidthMCU * sHeightMCU;
		j->ScanWidth = sWidthMCU;
	}
	else 
	{
		// interleaved - full MCUs
		j->ScanWidth = j->WidthMCU;
		j->ScanMCUs = j->WidthMCU * j->HeightMCU;
	}
	j->multiPassScan++;
}

                                                                                                  

//====================================================================
//	Setup at beginning of restart interval

static void BeginInterval(JPEGDecoder* j)
{
	short	i;
	CompSpec **cspec;

	cspec = j->scanComps;
	for (i = 0; i < j->CompInScan; i++, cspec++ ) 
		(*cspec)->DC = 0;		// Reset DC prediction
	j->bandSkip = 0;

	if (!(j->Interval = j->RstInterval))
		j->Interval = j->ScanMCUs;			// Restart interval counter ( or full scan )

	MHKMessage(("Interval reset %d",j->Interval));
	j->last32 = *(long *)j->data;	// Prime the data pump
	j->data += 4;
	j->next32 = *(long *)j->data;	// Prime the data pump
	j->data += 4;
	j->bits = 32;
	j->RstMarker = 0;								// Reset restart marker
}


//====================================================================
//		
//		DecodeScan draws as much data as is in the buffer.
//		If a restart marker is detected, DecodeScan can draw
//		until the end of the scan without worring about checking
//		for underflows
//



#define skip_bits(_x)	if ((j->bits -= _x) < 0) {			\
							j->bits += 32;					\
							j->last32 = j->next32;			\
							j->next32 =  *(long *)j->data;	\
							j->data += 4;					\
						}

#define	BITCOUNT	( ((j->data - dataStart - 8) <<3)  +  (32 - j->bits))

static void SaveState(JPEGDecoder *j)
{
	long i;
	
	long scanIndex = j->multiPassScan;
	
	if ( scanIndex < kMaxProgressiveScans  ) 
	{
		MHKMessage(("save state for scan %d",scanIndex));
		j->scanState[scanIndex].bitsConsumed = j->scanBitsUsed;
		for ( i=0; i < j->CompInFrame; i++ )
			j->scanState[scanIndex].DC[i] = j->Comp[i].DC;
		j->scanState[scanIndex].bandSkip = j->bandSkip;
		j->scanState[scanIndex].set = true;
	}
}



static Boolean RestoreState(JPEGDecoder *j)
{
	long i;
	long scanIndex = j->multiPassScan;

	if ( scanIndex < kMaxProgressiveScans  ) 
	{
		if ( j->scanState[scanIndex].set ) 
		{
			MHKMessage(("restore state for scan %d",scanIndex));
			j->scanBitsToSkip = j->scanState[scanIndex].bitsConsumed;
			j->scanBitsUsed = 0;
			j->bandSkip = j->scanState[scanIndex].bandSkip;
			for ( i=0; i < j->CompInFrame; i++ )
				j->Comp[i].DC = j->scanState[scanIndex].DC[i];
			j->scanState[scanIndex].set = false;
			return true;
		}
	}
	return false;
}

/*
	Convert the current MCU from DCT to pixels, dequantizing as we go.
	The mcu is assumed to be in interleaved order starting at src.
	if src is nil thenmcuNum refers to the mcu index in the block buffer for each 
	component.

*/



static Error DecodeScan(JPEGDecoder *j, short marker)
{

	long 		i;
	long		xMCU,yMCU;
	CompSpec	*cspec;
	short		restartMarker = j->RstMarker;
	Boolean		skipScan = false;
	Boolean		visible = true;
	Boolean		useBlockBuffer = j->isProgressive && !j->thumbnail;
	uchar* 		dataStart = j->data;
	long		startBits = 0;
	long 		mcuBufferStart = 0;
	long 		mcuBufferSize = 0;
	long		MCUsPerSlice = 0,MCUsTillSlice = 0;
	short		*b;
	long		blockbufsize;
	Boolean 	scanIsNonInterleaved = (j->CompInScan == 1 && j->CompInFrame > 1 );
	Boolean 	isBaseScan = false;
	Boolean 	partialBlockBuffer;
	
	if ( j->dataEnd - j->data == 0 )
		return kNoError;
		
//	Start a new interval if there was a restart marker or start a new scan

	if (j->RstMarker) 
	{
		if (j->RstMarker == kSOS)	
		{
			SetupScan(j);
			MHKMessage(( j->isProgressive ?  "Start new progressive scan to %X": "Start new scan to %X",marker));
		}
		if (j->dataEnd - j->data > 8)	// Need at least 8 bytes to start interval
			BeginInterval(j);
		else 
			return kNoError;			// Wait for more data to come
	}
	
	startBits = BITCOUNT;
	

	if ( j->slicesPerPass ) 
	{
		MHKMessage(("%d slices per pass", j->slicesPerPass));

		isBaseScan =  ( !scanIsNonInterleaved && j->multiPassScan == 1 );


		if ( !isBaseScan ) 
		{
		
			RestoreState(j);
			
			if ( j->scanBitsToSkip ) {
			
				MHKMessage(("skipping %d bits in scan %d",j->scanBitsToSkip,j->multiPassScan));
				
				// align to word
				
				j->skipToFirstSlice = true;

				if ( j->scanBitsToSkip > 32 ) 
				{
					long curBits = BITCOUNT-startBits;
					curBits &= 31;
					skip_bits(curBits);
					j->scanBitsToSkip -= curBits;
				}

				while ( j->scanBitsToSkip > 8 ) 
				{
					skip_bits(8);
					j->scanBitsToSkip -= 8;
					if ( !marker ) 
					{
						if ( j->dataEnd - j->data < 8 )	 
						{	// Ran out of data	
							MHKMessage(("Ran out of data skipping bits"));
							goto done;
						}
					}
				}
				if ( j->scanBitsToSkip > 0 ) 
				{
					skip_bits(j->scanBitsToSkip);
				}
				j->scanBitsToSkip = 0;
				if (!marker)  {
					if (j->dataEnd - j->data < kMDUMin)	 
					{	// Ran out of data
						goto done;
					}
				}
			}
		}
	}
	

	if ( j->currentSlice > j->lastSlice ) 
	{
		MHKMessage(("skip entire scan current %d last %d",j->currentSlice,j->lastSlice));
		skipScan = true;
	}
	
	
	// the following may work with progressive but it's not tested

	if ( !j->isProgressive ) 
	{
		/* clip an entire restart interval if possible */

		if ( (restartMarker & 0xf8) == 0xd0 && (j->MCUCount + j->Interval) < j->firstMCU ) 
		{
			j->RstMarker = 0xd0 + ((restartMarker + 1) & 7);

			if ( marker == 0 ) 
				j->SkipToRestartMarker = j->RstMarker;
			MHKMessage(("skip entire interval marker %X next %X count %d, interval %d first %d",restartMarker,j->RstMarker,j->MCUCount,j->Interval,j->firstMCU));
			j->data = j->dataEnd;
			j->bits = 0;
			j->MCUCount += j->Interval;
			j->currentSlice = j->MCUCount / j->WidthMCU; 
			j->Interval = 0;
			j->bandSkip = 0;
//			j->scanBitsToSkip = 0;
//			j->scanBitsUsed = 0;
//			j->skipToFirstSlice = false;
			goto done;
		}
	}
	
	if ( skipScan ) 
	{
		MHKMessage(("Skipping to end of scan"));
		ResetScan(j);
		j->currentSlice = j->lastSlice+1;		// make sure we keep skipping
		j->phase = kSkippingScan;
		goto done;
	}
	
//	We can decode until the end of image/restart interval,
//	or until we run out of data

	xMCU = j->MCUCount % j->ScanWidth;
	yMCU = j->MCUCount / j->ScanWidth;




	MHKMessage(("start scan from %d %d scanline mdu %d, interval %d",xMCU,yMCU,j->MCUCount,j->Interval));
	
	partialBlockBuffer = j->slicesPerPass != 0;


	MHKMessage(("scan %d components firstslice %d currentslice %d lastSlice %d x=%d,y=%d",j->CompInScan,j->firstSlice,j->currentSlice,j->lastSlice,xMCU,yMCU));
			
	if ( scanIsNonInterleaved ) 
	{
		if ( !j->isProgressive  ) 
		{
			useBlockBuffer = false;
			partialBlockBuffer = false;
		}
		MCUsPerSlice =  j->ScanWidth * j->scanComps[0]->Vi;
		MCUsTillSlice = j->MCUCount % MCUsPerSlice;
		MHKMessage(("non-interleaved scan at mcu %d %d ( mcus/slice= %d)",j->MCUCount,MCUsTillSlice,MCUsPerSlice));
	}
	
	
	
	
	blockbufsize = 0;
	for ( i=0; i < j->CompInFrame; i++ ) 
	{
		cspec = &j->Comp[i];
		blockbufsize += cspec->blocksMCU * kMCUSize * sizeof(ushort);
		// Dave - Only check valid cache entry if using block buffers.
		if (useBlockBuffer) {
			cspec->blockBuffer = (short *)cspec->blockBufferCacheEntry->GetData();
			if ( IsError(cspec->blockBufferCacheEntry == nil || cspec->blockBuffer == nil) )
			{
				//Complain(("blockBufferCacheEntry->GetData failed"));
				j->error = kGenericError;
				goto done;
			}
		}
	}
	
	if ( partialBlockBuffer && j->slicesPerPass ) 
	{
		mcuBufferStart = j->WidthMCU * j->thisPassFirstSlice;
		mcuBufferSize = j->slicesPerPass * j->WidthMCU;
		MHKMessage(("mcubuffer start %d size %d",mcuBufferStart,mcuBufferSize));
	}
	
	
	if ( isBaseScan && j->currentSlice > j->lastPassSlice )
		useBlockBuffer = false;
	
	while (j->Interval--) 
	{			// Decode the next MCU
	
		if ( partialBlockBuffer && j->slicesPerPass == 0 ) 
		{
			mcuBufferStart = j->WidthMCU * j->currentSlice;
			mcuBufferSize = j->WidthMCU;
		}
		Boolean	completedSlice = false;
	
		if (!marker)  
		{
			if (j->dataEnd - j->data < kMDUMin)	 
			{	
				MHKMessage(("ran dry at %d (%d,%d)",j->MCUCount,xMCU,yMCU));
				j->Interval++;
				break;
			}
		}

		// Huffman decode

		if ( scanIsNonInterleaved ) 
		{
			// non interleaved scan ( in interleaved frame )

			CompSpec *nicomp = j->scanComps[0];
			Boolean skipMCU = false;
			long mcuNum,mcuRow = j->MCUCount/j->ScanWidth;
			
			// calculate the mcu number ( relative to place in frame ) based on position in scan
			// NOTE: this only supports interleaves of 1 or 2...
			
			if ( nicomp->Vi == 2 )
				mcuNum = (mcuRow>>1)  * j->WidthMCU;
			else
				mcuNum = mcuRow  * j->WidthMCU;
			if ( nicomp->Hi == 2 )
				mcuNum += xMCU >> 1;
			else
				mcuNum += xMCU;
			if ( partialBlockBuffer  ) 
			{
				mcuNum -= mcuBufferStart;
				if ( mcuNum < 0 || mcuNum >= mcuBufferSize )
					skipMCU = true;
			}
			
			if ( !skipMCU )  
			{
				if ( !j->isProgressive ) 
				{
					b =j->blocks;
					ZeroMemory(b,kMCUSize * sizeof(ushort));
					DecodeComponent(j,1,nicomp,b);
				} else 
				{
					b = nicomp->blockBuffer + nicomp->blocksMCU * (mcuNum << kMCUSizeShift);
					if ( nicomp->Hi == 2 )
						b += ( xMCU & 1 ) << kMCUSizeShift;
					if ( nicomp->Vi == 2 ) 
						b += (nicomp->Hi * (mcuRow & 1)) << kMCUSizeShift;
					j->skipToFirstSlice = false;
					if ( !j->isProgressive ) 
						ZeroMemory(b,kMCUSize * sizeof(ushort));
					DecodeComponent(j,1,nicomp,b);
				}
			} else 
			{
				if ( !j->skipToFirstSlice ) 
				{
					b = j->blocks;
					DecodeComponent(j,1,nicomp,b);
				}
			}
			if ( ++xMCU == j->ScanWidth ) 
			{
				xMCU = 0;
				yMCU++;
			}
		} else {

			// interleaved scan ( or grayscale )

			long decodeMCU = j->MCUCount;
			visible = ( j->currentSlice >= j->firstSlice && xMCU >= j->leftMCU && xMCU <= j->rightMCU);
	
			b = nil;
			if ( useBlockBuffer ) 
			{ 
				if ( partialBlockBuffer  ) 
				{
					decodeMCU -= mcuBufferStart;
					if ( decodeMCU < 0 || decodeMCU >= mcuBufferSize ) 
						visible = false;
				}
			} 
			else 
			{
				if (  visible) 
				{
					b = j->blocks;
					ZeroMemory(b,blockbufsize);
				}
			}
			
			if ( j->currentSlice == j->firstSlice ) 
				j->skipToFirstSlice = false;
			if ( !visible ) {
				if ( !j->skipToFirstSlice ) 
				{
					for ( i = 0; i < j->CompInFrame; i++ ) 
					{
						cspec = &j->Comp[i];
						if ( cspec->inScan ) 
							DecodeComponent(j,cspec->blocksMCU,cspec,nil);
					}
				}
			} else {
				for ( i = 0; i < j->CompInFrame; i++   ) 
				{
					cspec = &j->Comp[i];
					if ( cspec->inScan ) 
					{
						if ( useBlockBuffer  ) 
							b = cspec->blockBuffer + ((decodeMCU * cspec->blocksMCU)<<kMCUSizeShift);
						DecodeComponent(j,cspec->blocksMCU,cspec,b);
						b += cspec->blocksMCU<<kMCUSizeShift;
					}
				}
			}
		}

		// in case we went nuts in huffman decode

		if ( j->data-j->dataEnd > 8  )  
		{
			MHKMessage(("ran out of data"));
			j->error = kBadHuffmanErr;
			goto done;
		}	

		if ( j->error )	
		{					// Died or was cancelled
			TrivialMessage(("scan error at mcu %d %d,%d with %d bytes left",j->MCUCount,xMCU,yMCU,j->dataEnd-j->data));	
			goto done;		
		}
		
		// Draw the decoded MCU  if necessary
		
		if ( scanIsNonInterleaved ) 
		{
	
			// when we did enough to complete a slice of MCUs, then process it

			if ( !j->isProgressive ) 
			{
			
				// special case for multi-scan sequential ( just draw first scan directly )

				TransformBlocks(j,j->scanComps[0],1,j->blocks,j->blocks);
				DrawMCUPiece(j,j->blocks,xMCU == 0 ? j->ScanWidth-1 : xMCU-1,yMCU);
				j->didDraw = true;
			}
			
			if ( ++MCUsTillSlice == MCUsPerSlice || 
				j->MCUCount == (j->ScanMCUs-1) ) 
			{			// special case for possible partial slice at end of image
				MCUsTillSlice = 0;

				if ( j->currentSlice >= j->firstSlice && j->isProgressive) 
				{
					long rowX;
					long dispMCU = j->currentSlice * j->WidthMCU;
					if ( partialBlockBuffer  ) 
						dispMCU -= mcuBufferStart;
					for ( rowX= 0; rowX < j->WidthMCU; rowX++, dispMCU++ ) 
					{
						if ( rowX >= j->leftMCU && rowX <= j->rightMCU ) 
						{
							DoTransform(j,dispMCU,nil);
							DrawMCU(j,j->blocks,rowX,j->currentSlice);
							j->didDraw = true;
						}
					}
				}
				completedSlice = true;
			}
		} else {
			if ( visible )  
			{
				if ( !j->thumbnail ) 
				{ 
					if ( useBlockBuffer ) 
					{
						long idispMCU = j->MCUCount;
						if ( partialBlockBuffer  ) 
						{
							idispMCU -= mcuBufferStart;
						}
						DoTransform(j,idispMCU,nil);
					} else
						DoTransform(j,0,j->blocks);
				}
				DrawMCU(j,j->blocks,xMCU,yMCU);
				j->didDraw = true;
			}
			if ( ++xMCU == j->WidthMCU ) 
			{
				xMCU = 0;
				yMCU++;
				completedSlice = true;
			}
		}
		
		j->MCUCount++;
		
		if ( completedSlice ) 
		{
			//Message(("did slice %d draw %d",j->currentSlice,j->didDraw));

			if ( j->slicesPerPass == 0 ||  j->currentSlice <= j->lastPassSlice || isBaseScan ) 
			{
				if ( j->didDraw ) 
				{
					MHKMessage(("draw slice %d (y = %d)",j->currentSlice,j->currentSlice * j->MCUVPixels));
					if (j->SingleRowProc != nil)
						 j->error = (j->SingleRowProc)(j);
					j->didDraw = false;
				}
			}
			Boolean 	scanCompleted = false;
 
			if ( j->slicesPerPass ) 
			{
				if ( j->currentSlice == j->lastPassSlice ) 
				{
					MHKMessage(("MMMX save slice at %d with %d bits used",j->currentSlice,BITCOUNT - startBits));
					j->scanBitsUsed += BITCOUNT - startBits;
					
					// keep drawing  after slice if we can
					if ( isBaseScan ) 
					{
						if ( j->maxScansToDisplay == 0 ) 
						{
							MHKMessage(("Base scan at %d continue",j->currentSlice));
							useBlockBuffer = false;
						} else 
						{
							j->scanBitsUsed = 0;
							scanCompleted = true;
						}
					} 
					else 
					{
						SaveState(j);
						j->scanBitsUsed = 0;
						scanCompleted = true;
					}
				}
			}
			if ( j->currentSlice == j->lastSlice ) 
			{
				if ( j->multiPassScan == 0 || j->multiPassScan > j->scansProcessed )
					j->scansProcessed++;
				scanCompleted = true;
			}
			if ( scanCompleted ) 
			{
				MHKMessage(("MMMX ending slice at %d (%d)",j->currentSlice,j->MCUCount));
				j->scanBitsUsed = 0;
				if ( j->thumbnail || marker == kEOI  )
					j->phase =  kSkippingToEnd;	
				else
					j->phase =  kSkippingScan;		// next scan 
				break;
			}
			++j->currentSlice;
		}
		if ( j->error )	
		{					// Died or was cancelled
			goto done;		
		}

		if ( j->MCUCount > j->ScanMCUs ) 
		{
			Complain(("End of JPEG data reached"));
			EndJPEGImage(j);
			break;
		}
	}
	j->RstMarker = marker;
done:
	if ( j->slicesPerPass ) 
	{
		j->scanBitsUsed += BITCOUNT - startBits;
		MHKMessage(("partial slice with %d bits used (so far %d)",BITCOUNT - startBits,j->scanBitsUsed));
	}
	for ( i=0; i < j->CompInFrame; i++ ) 
	{
		cspec = &j->Comp[i];
//		if ( cspec->blockBuffer )	cspec->blockBufferCacheEntry->EndUseData();
		cspec->blockBuffer = nil;
	}
	return j->error;
}



static long SkipScan(JPEGDecoder* j, uchar* data, long dataLength)
{	
	uchar marker;
	uchar *dataStart = data;
	uchar *dataEnd = dataStart + dataLength;
	
	// only handles one scan per image for non-interleaved sequential JPEG ( rare )
	// those images need to be transcoded to see all the components 
	
	if ( j->thumbnail || !j->isProgressive 
		||  ( j->maxScansToDisplay != 0 && j->multiPassScan > j->maxScansToDisplay )
	) 
	{			
		j->phase = kSkippingToEnd;
		return dataLength;
	}
	if ( dataLength == 0 )
		return 0;
		
	MHKMessage(("skipping scan data %d bytes",dataLength));
	
	while ( data < dataEnd ) 
	{
		if ( *data++ == 0xff ) 
		{
			// make sure a marker doesn't get split from its 0xff header
			// or else we wont recognize it next time around
			
			if ( data == dataEnd ) 
			{	
				MHKMessage(("split marker during skip"));
				return dataLength-1;		
			}
			
			// multiple fill bytes are allowed
			
			while ( (marker=*data++) == 0xff ) 
			{
				if ( data == dataEnd ) 
				{
					MHKMessage(("split marker during skip"));
					return dataLength-1;		// dont split markers
				}
			}
			
							
			if ( marker != 0  ) 
			{		// if it's real
				if ( marker == kEOI ) 
				{
					MHKMessage(("end of image reached skipping scan"));
					j->phase =  kSkippingToEnd;	
					return data-dataStart;
				} 
				else 
				{	
					// ignore restarts
					if ( (marker & 0xf8) != 0xd0 ) 
					{
						MHKMessage(("found marker skipping %X %d bytes used",marker,(data-2) - dataStart));
						switch ( marker ) 
						{
						case kDQT:
						case kDHT:
						case kDRI:
						case kSOS:
						case kEOI:
						case kCOM:
							break;
						default:
							Message(("Invalid marker found in scan"));
							return -1;
						}
						j->phase = kWaitingForScan;
						return (data-2) - dataStart;
					}
				}
			}
		}
	}
	return dataLength;
}


//====================================================================
//
//		ScanWrite writes vlc data, stripping huffman codes as it goes
//		We can always write into the start of our buffer, since the
//		caller is taking care of how much data is being used.
//

static long ScanWrite(JPEGDecoder* j, uchar* data, long dataLength)
{
	uchar	*dst,*dataStart = data;
	short	marker = 0;
	Error	error;
	long	count;

//	Write data until our buffer is full, they ran out of data or we hit a marker

	count = MIN(kJPEGBufferSize - j->inBuffer,dataLength);
	if ( IsError(count <= 0 ) )
		return -1;

	dst = j->WriteBuffer + j->inBuffer;

	MHKMessage(("process scan %d bytes",count));
	while (count-- > 0) {
		if ( (*dst++ = *data++) == 0xFF ) 
		{
			// multiple fill bytes are allowed, but dont copy them
			do {
				if ( !count--) 
				{
					MHKMessage(("split marker"));
					data--;			// Didn't get this one, get it next time
					dst--;
					j->inBuffer = dst - j->WriteBuffer;
					return data-dataStart;
				}
			} while ( (marker=*data++) == 0xff );

			// it's either a byte stuffed zero or a real marker

			if (marker != 0) 
			{
				MHKMessage(("marker in scan %X",marker));
				if ( (marker & 0xF8) == kRST ) 
				{
					dst--;		// dont include the restart marker
					break;
				} 
				else 
				{
					if ( marker == kEOI ) 
					{
						j->phase = kSkippingToEnd;
						MHKMessage(("end of image in scan"));
						dst--;
						break;
					} 
					else 
					{
						dst--;			// dont want the 0xff
						data -= 2;		// process the marker next time	
						break;
					}
				}
			}
		}
	}

	if ( j->SkipToRestartMarker ) 
	{
		MHKMessage(("waiting for marker %X",j->SkipToRestartMarker));
		if ( marker == j->SkipToRestartMarker )
			j->SkipToRestartMarker = 0;
		return data-dataStart;
	}
	
	//Message(("processed %d bytes from %d - marker %X, total avail = %d",dst - (j->WriteBuffer + j->inBuffer),data-dataStart,marker,dst-j->WriteBuffer));


	if ( dst == j->WriteBuffer ) 
	{
		j->phase = kWaitingForScan;
		ResetScan(j);
		return 0;
	} 
	else 
	{
		if ( marker == 0 && (dst-j->WriteBuffer) < kMDUMin ) 
		{
			j->inBuffer = dst - j->WriteBuffer;
			return data-dataStart;
		}
		
		j->data = j->WriteBuffer;	// Read from here ...
		j->dataEnd = dst;			// use to here
	
		//	Data is all set up, use it
	
		if ((error = DecodeScan(j,marker)) != kNoError)
		{
			ImportantMessage(("JPEG DecodeScan: Error %d", (int)error));
			ResetJPEGDecoder(j);
			return error;				// Error
		}
		
		//Message(("decoded %d bytes of %d",j->data-j->WriteBuffer,j->dataEnd-j->WriteBuffer));

		//	if we dont need the rest of the scan ( or the image )
		//	just skip over the rest of the data here

		if ( j->phase == kSkippingScan )
		{
			if ( marker == kEOI )
				j->phase =  kSkippingToEnd;	
			else 
			{
				ResetScan(j);
				long skipped = 0;
				long used = (data - dataStart);
				if ( (dataLength-used)  > 0 && used > 0 )
				{
					MHKMessage(("JPEG skipping rest of scan %d bytes used %d left",used,dataLength-used));
					skipped = SkipScan(j,data,dataLength - used);
					MHKMessage(("skipped %d bytes",skipped));
					if ( skipped < 0 )
						return skipped;
				}
				return used + skipped;
			}
		}
	
		if ( marker == kEOI || j->phase == kSkippingToEnd ) 
		{
			ResetScan(j);
			j->phase =  kSkippingToEnd;	
			return dataLength;
		}

	//	Copy Unused data to buffer head
		
		if (marker) 
		{
			j->inBuffer = 0;
		}
		else
		{
			j->inBuffer = j->dataEnd - j->data;
			MHKMessage(("%d kept in buffer",j->inBuffer));
			if ( IsError(j->inBuffer < 0) )
				return -1;
			if ( IsError(j->inBuffer > kJPEGBufferSize) )
				return -1;
			if (j->inBuffer)
				CopyMemory(j->data,j->WriteBuffer,j->inBuffer);
		}
		if (data < dataStart)
			Message(("Underflow In ScanWrite: %d",data - dataStart));
	}
	
	return data - dataStart;	// Return how much we used
}




static long NextMarker(uchar *data,long dataLength,uchar *rMarker,ushort *rSize)
{
	long used = 0;
	uchar marker = 0;
	Boolean head = false;
	ushort s;

	*rMarker = 0;
	*rSize = 0;
	if ( dataLength <= 2 ) 
	{
		//MHKMessage(("no room for marker"));
		return 0;
	}
	while ( used < dataLength ) 
	{
		if ( marker == 0 ) 
		{
			if ( head ) 
			{
				if ( *data != 0xff )
					marker = *data;
				data++;
				used++;
			}
			else 
			{
				if ( *data++ == 0xff ) 
					head = true;
				used++;
			}
		} else 
		{
			if ( marker == kSOI || marker == kEOI ) 
			{
				*rSize = 0;
				*rMarker = marker;
				break;
			} 
			else if ( (marker & 0xf8) == kRST ) 
			{
				marker = 0;
				head = false;
				continue;
			} 
			else 
			{
				if ( dataLength-used < 2 ) 
				{
					//MHKMessage(("no room for marker header %X",marker));
					return used-2;		// reget from start of marker
				}
				used += 2;
				s = *data++;
				s <<= 8;
				s |= *data++;
				*rSize = s;
				*rMarker = marker;
				break;
			}
		}
	}
	return used;
}


//====================================================================
//
//		JPEGWrite
//			Write data into the decoder
//			Returns number of bytes used or
//			0 if size is to small or
//			-ve if there has been an error

long JPEGWrite(JPEGDecoder* j, Byte* data, long dataLength,Rectangle *drewRectangle)
{
	uchar	marker;
	ushort	s;
	Error	error;
	long	l,bytesConsumed = 0;
	long	markerOffset = 0;
	JPEGDecodePhase thePhase;
	long	result = 0;
	
	SetRectangle(j->fDrewRectangle,0,0,0,0);

	marker = 0;

	if  ( j->phase == kSkippingToEnd ) 
	{
		TrivialMessage(("End of image at first"));
		ResetJPEGDecoder(j);
		j->phase = kWaitingForNewImage;
	}
	
	if ( dataLength == 0 )
	{
		goto done;
	}
	
	// we have to deal with skipping larger markers we dont care about like
	// comments and or app markers 
	

	if ( j->unusedSkip ) 
	{
		if ( dataLength > j->unusedSkip )
			j->unusedSkip = 0;
		else
			goto done;
	}


newPhase:
	if ( j->phase == kSkippingToEnd ) 
	{
		TrivialMessage(("End of image reached"));
		goto completed;
	}
	//TrivialMessage(("JPEGWrite len %d phase %d consumed %d",dataLength,j->phase,bytesConsumed));

	thePhase = j->phase;

	if ( dataLength < 8 && bytesConsumed != 0 ) 
	{
		MHKMessage(("only %d bytes left, returning with %d",dataLength,bytesConsumed));
		if ( IsError(bytesConsumed < 0) )
			result = kGenericError;
		else
			result = bytesConsumed;
		goto done;
	}

	if ( IsError(dataLength <= 0 ) ) {
		result = kGenericError;
		goto done;
	}

	switch ( j->phase ) 
	{
	case kSkippingScan:
//		MHKMessage(("kSkippingScan %d",dataLength));
		l = SkipScan(j,data,dataLength);
		if ( l < 0 ) 
		{
			Message(("SkipScan error %ld",l));
			ResetJPEGDecoder(j);
			result = l;
			goto done;
		}
		bytesConsumed += l;
		dataLength -= l;
		data += l;
			
		if (l != 0 && j->phase != thePhase )	
			goto newPhase;
		if ( IsError(bytesConsumed < 0) )
			result = kGenericError;
		else
			result = bytesConsumed;
		goto done;
		
	case kProcessingScan:

//		MHKMessage(("process scan %x %d",data,dataLength));
		l = ScanWrite(j,data,dataLength);
		if ( l < 0 ) 
		{
			Message(("ScanWrite error %ld",l));
			ResetJPEGDecoder(j);
			result = l;
			goto done;
		}
		bytesConsumed += l;
		dataLength -= l;
		data += l;
		if ( l != 0 && j->phase != thePhase )	
			goto newPhase;
		if ( IsError(bytesConsumed < 0) )
			result = kGenericError;
		else
			result = bytesConsumed;
		goto done;
			
	case kSkippingToEnd:	// Image is complete, accept no more data
completed:
//		MHKMessage(("kSkippingToEnd %d",dataLength));

		ResetJPEGDecoder(j);
		j->phase = kWaitingForNewImage;
		if ( j->error == 0 ) 
		{
			if ( j->anotherPass ) 
			{					// requires another display pass on full stream
				MHKMessage(("end of image - need another pass"));
				j->phase = kWaitingForNewImage;
				result = kJPEGCompletedPass;							// magic code
				goto done;
			}
		} else
			j->anotherPass = 0;
		result = kJPEGCompletedPass;
		goto done;
		
	case kWaitingForNewImage:
	
		MHKMessage(("kWaitingForNewImage %d",dataLength));
		l = NextMarker(data,dataLength,&marker,&s);
		markerOffset = l;
		bytesConsumed += l;
		if ( IsError(l > dataLength) ) {
			result = kGenericError;
			goto done;
		}
		if ( marker == 0 ) {
			result = bytesConsumed;
			goto done;
		}
		else if (marker != kSOI) {				// Need to start with SOI
			result = kNoSOIMarkerErr;
			goto done;
		}
		j->phase = kWaitingForFrame;
		if ( IsError(bytesConsumed < 0) )
			result = kGenericError;
		else
			result = bytesConsumed;
		goto done;
	
	case kWaitingForFrame:
	case kWaitingForScan:
	
		MHKMessage(("kWaitingForScan %d",dataLength));
		markerOffset = l = NextMarker(data,dataLength,&marker,&s);
		if ( IsError(l > dataLength) ) {
			result = kGenericError;
			goto done;
		}
		bytesConsumed += l;
		data += l;
		dataLength -= l;
		if ( marker == 0 )
		{
			if ( IsError(bytesConsumed < 0) )
				result = kGenericError;
			else
				result = bytesConsumed;
			goto done;
		}
		
		MHKMessage(("process marker %X size %d",marker,s));
	
		if ( IsError(marker < kSOF0 || s <= 0) ) {
			result = kGenericError;
			goto done;
		}
	
		if ( s > (dataLength-2) ) 
		{
		
			MHKMessage(("not enough data to process marker need %d",s));
			j->unusedSkip = s;
	
			// go back to just before the marker
			result = bytesConsumed - markerOffset;
			goto done;
		}
		
	//	Interpret the marker
	
		switch (marker)
		{
			case kEOI:		EndJPEGImage(j); return kJPEGCompletedPass;
			case kSOF0:	  error = InterpretSOF(j,data,s);	break;	// Baseline DCT
			case kSOF0+1: error = InterpretSOF(j,data,s);	break;	// Baseline DCT
			case kSOF2:  
					j->isProgressive = true;
					error = InterpretSOF(j,data,s);
					if ( error == kNoError ) 
					{
						TrivialMessage(("Progressive JPEG Image"));
					}
					break;	// Progressive DCT
			case kSOS:	  error = InterpretSOS(j,data,s);	break;	// Start of Scan
			case kDHT:	  error = InterpretDHT(j,data,s);	break;	// Define Huffman tables
			case kDQT:	  error = InterpretDQT(j,data,s);	break;	// Define quantization tables
			case kDRI:	  error = InterpretDRI(j,data,s);	break;	// Define restart intervals
			case kCOM:	  
					if ( dataLength < (s-2) ) 
					{			// marker doesnt fit	
						TrivialMessage(("JPEG Ignoring big marker %X",marker));
					//	j->unusedSkip = s-2;
						result = l;
						goto done;
					}
					error = InterpretCOM(j,data,s);	
					break;	// Comment
			default:	
					if ((marker >= kAPP) && (marker <= (kAPP + 0xF))) 
					{	// Application specific
						if ( dataLength < (s-2) ) 
						{			// marker doesnt fit	
							TrivialMessage(("JPEG Ignoring big marker %X",marker));
					//		j->unusedSkip = s-2;
							result = l;
							goto done;
						}
						error = InterpretAPP(j,data,s);
					}
					else 
					{
						Message(("Unexpected JPEG marker %X",marker));
						error = kBadMarkerErr;
					}
					break;
		}
		if (error != kNoError) 
		{
			MHKMessage(("Marker decode error %d",error));
			ResetJPEGDecoder(j);
			result = error;
			goto done;
		}
		bytesConsumed += (s-2);
		if ( IsError(bytesConsumed < 0) )
			result = kGenericError;
		else
			result = bytesConsumed;		// Not even enough data for a marker and size!
		goto done;
	default:
		Complain(("bad phase"));
		break;
	}
	result = 0;
done:
	if ( drewRectangle )
		*drewRectangle = j->fDrewRectangle;
	return result;
}



	// this resets everything ( including multipass state ) and sets the max scans
	// to display to the parameter given. This is useful when the amount of data
	// for a progressive JPEG image is not yet available ( coming over the wire ) 
	// but partial display is desired. Not needed if displaying in one pass 
	// 

void	
SetMaxScansToDisplay(JPEGDecoder* j,long scansToDisplay)
{
	Rectangle	saveDrawRect = j->fDrawRect;
	Rectangle	saveClipRect = j->fClipRectangle;
	ulong	saveTransparency = j->fTransparency;
	DrawRowProcPtr saveDrawProc = j->SingleRowProc;
	Boolean saveThumbNail = j->thumbnail;
	
	
	PurgeJPEGDecoder(j);
	InitJPEGDecoder(j);
	j->fDrawRect = saveDrawRect;
	j->fClipRectangle = saveClipRect;
	j->fTransparency = saveTransparency;
	saveDrawProc = j->SingleRowProc = saveDrawProc;
	j->thumbnail = saveThumbNail;

	j->maxScansToDisplay = scansToDisplay;
}

//====================================================================
//
//
