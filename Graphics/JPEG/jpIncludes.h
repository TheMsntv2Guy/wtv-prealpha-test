// ===========================================================================
//	jpIncludes.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __JPINCLUDES_H__
#define __JPINCLUDES_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"			/* for Priority */
#endif
#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"		/* for Error */
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"			/* for Rectangle */
#endif

//====================================================================
//
//
//
//
//
//
//
//
//====================================================================



#define kScaledDCT


// we can set I_SHIFT_F to 9 for better accuracy, but that
// means we need to do long multiplies in the IDCT code, which 
// cost one extra cycle - don't know if it's worth it...

const I_SHIFT_F = 10;					// scale factor for scaled quantization table
const O_SCALE_F = (16-I_SHIFT_F);		// unscale factor for transformed coefficients
const M_SCALE_F = (O_SCALE_F*2);		// intermediate scale factor for multiplies


const kMaxHuffmanTables= 8;
const kMaxQuantizationTables = 4;

const kMaxScanComponents = 3;			// standard allows 4 but we only support up to 3
const kMaxFrameComponents = 3;			// standard allows 4 but we only support up to 3
const kNumDecompressBuffers = kMaxFrameComponents;


// size of a MCU block 

const kMCUWidth = 8;
const kMCUHeight = 8;
const kMCUSize = (kMCUWidth * kMCUHeight);
const kMCUSizeShift	= 6;					// (1<<6) == kMCUWidth * kMCUHeight 



const kMaxMCUs = 6;								// the standard allows up to 10, but we only support up to 22:11:11 sampling so...

const kMDUMin=1024;								//		Minimum data in the buffer to safely decode an MDU

const kJPEGBufferSize = (kMaxMCUs + 2) * kMDUMin;		// read buffer size 


typedef	unsigned short uInt16;

//
//	JPEGDecoder contains all the state information for the decoder
//


typedef struct FastRec{		// Record for fast huffmand decode
	short	value;			// Decoded symbol value
	char	length;			// Length of decoded symbol or code (-ve for code)
	uchar	run;			// Zero run length or 0 for DC
} FastRec;

typedef struct HuffmanTable{
	FastRec	fast[256];		// for fast decoding of the huffman data
	long	mincode[17];	// smallest code of length (i+1)
	long	maxcode[18];	// largest code of length (i+1) (-1 if none)
	short	valptr[17+1];	// huffval[] index of 1st symbol of length (i+1)
	Byte	huffval[256];	// Symbols in increasing code length
	Byte	bits[17+3];		// bits[i] = # of symbols with codes of length (i+1) bits
} HuffmanTable;



typedef struct CompSpec{			// Component spec 

									// following are set once per image
	CacheEntry *blockBufferCacheEntry;
	short	*blockBuffer;			//		 block buffer ( for progressive )
	long	blockCount;				// 		blocks buffered

									// following set once per frame
	short	Ci;						// 		Component identifier
	short	Hi;						// 		Horizontal blocks in MCU
	short	Vi;						// 		Vertical blocks in MCU
	uInt16	*QTable;						// 		quantization table
	short	blocksMCU;				// 		blocks in a frame MCU ( MDU )

									// following set once per scan
	HuffmanTable	*DCH;			// 		dc huffman table
	HuffmanTable	*ACH;			// 		ac huffman table
	short	dcBitPos;				// 		successive approx bit position
	short	acBitPos;				// 		successive approx bit position
	short	dcPrevBitPos;			// 		dc successive approximation needed ( for progressive)
	short	acPrevBitPos;			// 		ac successive approximation needed ( for progressive)
	short	specStart;				// 		spectrum start (for scan, progressive )
	short	specEnd;				//		spectrum end (for scan, progressive )
	short	DC;						// 		dc value
	Boolean	inScan;					// 		true if included current scan
} CompSpec;


// for saving state between passes

#define	kMaxProgressiveScans	16	// reasonable guess

typedef struct {
	Boolean		set;
	short	DC[kMaxScanComponents];		
	long	bandSkip;
	long	bitsConsumed;
}DecoderScanState;


typedef enum JPEGDecodePhase {
	kWaitingForNewImage = 0,
	kWaitingForFrame,
	kWaitingForScan,
	kProcessingScan,
	kSkippingScan,
	kSkippingToEnd
} JPEGDecodePhase;

typedef Error (*DrawRowProcPtr)(void* j);


// returns 1 to cancel decode



typedef struct JPEGDecoder
{
	Byte			*WriteBuffer;
	long			inBuffer;
	Byte			*data;
	Byte			*dataEnd;
	
	JPEGDecodePhase			phase;			// Phase of the decode (0,kSOI,kSOS,kEOI)
	uchar			RstMarker;				// Last restart maker detected
	uchar			SkipToRestartMarker;	// skip all scan data up to and including this restart marker
	
	long			Width;			// number of horizontal lines defined in frame header
	long			Height;			// number of vertical lines defined in frame header
	short			RstInterval;	// 16 bit restart interval

	uchar			CompInFrame;						// Number of components in frame
	uchar			CompInScan;							// Number of components in scan

	CompSpec		Comp[kMaxFrameComponents];			// Up to 4 components per frame
	CompSpec*		scanComps[kMaxScanComponents];	
	ushort			*QTables[kMaxQuantizationTables];	// QTables
	uchar			*HTables[kMaxHuffmanTables];		// Huffman tables (DC 0-3, AC 0-3)


	long			bits;			// For reading bit by bit
	long			last32;
	long			next32;
	Error			error;			// Error during decode
	
	Boolean			thumbnail;		// Creates a 1/8 size thumbnail image instead	
	DrawRowProcPtr	SingleRowProc;	
	BitMapDevice*	DrawRefCon;
	BitMapDevice*	fDrawDevice;
	Rectangle		fDrawRect;		// drawing position and size
	Rectangle		fClipRectangle;
	ulong			fTransparency;
	
	short			blocks[kMCUSize*kMaxMCUs];		// Decoded MCU ( if not buffered )

	ulong			sampling;			// sampling interval code ( for display )
	
	Boolean			isProgressive;
	
	long			multiPassScan;
	DecoderScanState	scanState[kMaxProgressiveScans];
	long			scanBitsToSkip;
	long			scanBitsUsed;
	Boolean			skipToFirstSlice;
	Boolean			didDraw;
	
	ushort			WidthMCU;		// Width of image in MCU's
	ushort			ScanWidth;		// actual number of mcus in scan ( may differ if not interleaved )
	ushort			HeightMCU;		// Height of image in MCU's
	uchar			MCUHPixels;		// # of horizontal pixels in MCU
	uchar			MCUVPixels;		// # of vertical pixels in MCU
	
	long			Interval;		// Number of MCU's left in restart interval
	long			MCUCount;		// Number of MCU's drawn so far
	long			ScanMCUs;		// Number of MCU's in this scan

	Byte			*buffer[kNumDecompressBuffers];		// Decompression image buffers
	long			rowBytes[kNumDecompressBuffers];

									// clipping information by MCU
	long			firstMCU;		// 		first visible MCU in image 
	long			lastMCU;		// 		last visible MCU in image
	long			leftMCU;		// 		first visible MCU in each row 
	long			rightMCU;		// 		last visible MCU in each row
		
									// clipping information by slice
	long			currentSlice;	//		current slice being decoded
	long			firstSlice;		//		first visible slice
	long			lastSlice;		//		last visible slice

									// progressive decode stat
	long			bandSkip;		// number of blocks to skip in the current spectrum band

									// multi-pass progression
	long			anotherPass;	// needs another full display pass on image
	long			slicesPerPass;	// number of slices per display pass
	long			lastPassSlice;	// last slice in this pass
	long			thisPassFirstSlice;	// first slice in this pass
									// whacky undithering code
	long			ditherEnergy;
	long			unusedSkip;		// unused marker data to ignore
	
	Boolean			didClearDrawBuffer;
	
	Priority		priority;
	
	Rectangle		fDrewRectangle;
	
	long			scansProcessed;
	
	long			maxScansToDisplay;
	char			cacheName[8];
	BitMapDevice 	*fResizeBuffer;
	BitMapDevice	*fResizeAccumulateBuffer;
	long 			fDstYInc;
	long			fDstYPos;
	long			fDstYLast;
	
	
	Gamma			fGamma;
	uchar			fBlackLevel;
	uchar			fWhiteLevel;
	
} JPEGDecoder;

//====================================================================
//	Useful macros
//====================================================================

//	Figure F.12 - Extending the sign bit of a decoded value in v

#define Extend(v, t) ((v) < (1 << ((t)-1)) ? (v) + (-1 << (t)) + 1 : (v))



const	kJPEGCompletedPass	= -42;		// returned from JPEGWrite when done with pass


//====================================================================
//	Markers
//====================================================================

#define	kSOF0	0xC0		// Baseline DCT
#define	kSOF2	0xC2		// Progressive DCT
#define kDHT	0xC4		// Define Huffman tables
#define kRST	0xD0		// kRSTm (0xD0 - 0xD7)*
#define	kSOI	0xD8		// Start of image*
#define	kEOI	0xD9		// End of image*
#define kSOS	0xDA		// Start of Scan
#define kDQT	0xDB		// Define quantization tables
#define kDNL	0xDC		// Define number of lines
#define kDRI	0xDD		// Define restart intervals
#define kAPP	0xE0		// Application specific (0xE0 - 0xEF)
#define kJPG	0xF0		// JPEG extentions (0xF0 - 0xFD)
#define kCOM	0xFE		// Comment



//====================================================================
//	Routines
//====================================================================

//		Handy utils

#ifdef FOR_MAC
struct CGrafPort* JPEGToWorld(Byte* image, long size, short thumbnail);
#endif
Error		JPEGBounds(Byte* image, long size, Rectangle* bounds);
Error		JPEGDecodeImage(JPEGDecoder* j, Byte* image, long size);

//		Create and dispose JPEG decoders

JPEGDecoder* NewJPEGDecoder(const Rectangle* r, short thumbnail, DrawRowProcPtr drawProc, const Rectangle* invalid, ulong transparency,BitMapDevice *drawDevice);
void		DisposeJPEGDecoder(JPEGDecoder* j);


//====================================================================
//
//		JPEGWrite
//			Write data into the decoder
//			Returns number of bytes used or
//			0 if size is too small or
//			-ve if there has been an error

long	JPEGWrite(JPEGDecoder* j, Byte* data, long dataLength,Rectangle *drewRectangle = nil);

//		Interpret Q tables, huffman tables, frame, scan and APP info

Error	InterpretSOF(JPEGDecoder* j, Byte* data, short size);
Error	InterpretSOS(JPEGDecoder* j, Byte* data, short size);
Error	InterpretDQT(JPEGDecoder* j, Byte* data, short size);
Error	InterpretDHT(JPEGDecoder* j, Byte* data, short size);
Error	InterpretDRI(JPEGDecoder* j, Byte* data, short size);
Error	InterpretAPP(JPEGDecoder* j, Byte* data, short size);
Error	InterpretCOM(JPEGDecoder* j, Byte* data, short size);


void	SetMaxScansToDisplay(JPEGDecoder* j,long scansToDisplay);


void	SetJPEGGamma(JPEGDecoder* j,Gamma gamma,uchar blackLevel,uchar whiteLevel);

//		Calls to draw decoded blocks

Error	SetupDrawing(JPEGDecoder* j);
void	DrawMCU(JPEGDecoder* j,short	*blocks,long x,long y);

//		VLC low level calls

long	DecodeComponent(JPEGDecoder* j, short count,CompSpec* cspec, short* blocks);

extern uchar ZZ[kMCUSize];

// ===========================================================================
//	needed the following...

void 	DoTransform(JPEGDecoder *j,long mcuNum,short *src);
void 	DumpJPEGDecode(JPEGDecoder* j);
Error	DrawSingleJPEGRow(void *i);
void	DrawMCUPiece(JPEGDecoder *j, short	*blocks,long h,long v);
void	TransformBlocks(JPEGDecoder *j,CompSpec *cspec,long count,register short *block,register short *dqblock);


#endif /* __JP_INCLUDES_H__ */
