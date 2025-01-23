// ===========================================================================
//	GIF.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __GIF_H__
#define __GIF_H__

#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"
#endif

#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif

enum GIFPhase
{
	kGlobalImageHeader = 0,
	kReadExtension,
	kLocalImageHeader,
	kLocalImageBodySetup,
	kLocalImageBody,
	kGlobalImageEnd,
	kError
};
typedef enum GIFPhase GIFPhase;

class GIF {
public:
							GIF();
							~GIF();

	BitMapDevice*			GetBitMap() const;
	void					GetBounds(Rectangle* bounds) const;						

	void					Draw(Rectangle* r, const Rectangle* invalid, ulong transparency = 0,Rectangle *drewRectangle = nil);
#if defined FIDO_INTERCEPT
	void					Draw(const Rectangle* r, const Rectangle* invalid, ulong transparency, class FidoCompatibilityState& fidoCompatibility,Rectangle *drewRectangle) const;
#endif
	void					SaveBehind(const Rectangle* behindBounds);	
	long					Write(Byte* data, long dataLength,Boolean dataComplete,Boolean justGetBounds,Rectangle *drewRectangle = nil);
	void					SetBounds(const Rectangle* bounds);
	void					SetDrawRectangle(const Rectangle *drawRectangle);
	void					SetClipRectangle(const Rectangle *clipRectangle);
	Boolean					GetKeepBitMap() const;
	void					SetKeepBitMap(Boolean keep);
	void					SetFlip(Boolean flipHorizontal,Boolean flipVertical);
	void					SetTransparency(ulong transparency);
	void					SetIsBackground(Boolean);
	long					GetLoopCount() const;
	void					SetFilterType(FilterType);
	void					SetGamma(Gamma,uchar blackLevel,uchar whiteLevel);
	void					DoDraw();
	void					Rewind();

protected:
	long					GetCode();
	short					DecodeLZW();
	long					InitLZW(Byte *data,long dataLength);
	long					ReadComment(Byte *data, long dataLength);
	long					ReadExtension(Byte *data, long dataLength);
	long					ReadGlobalHeader(Byte *data, long dataLength);
	long					ReadGraphicControl(Byte *data, long dataLength);
	long					ReadImageBody(Byte *data, long dataLength);
	long					ReadImageHeader(Byte *data, long dataLength);
	long					SetupImageBody(Byte *data, long dataLength);
	long					SkipData(Byte *data,long dataLength);
	void					DrawStrip();
	long					FillBuffer(const uchar *src,long dataLength);
	void					Reset(Boolean keepBuffer);
	Boolean					Animate();
	void					DisposeImage();
	Boolean					GetBackgroundColor(Color *) const;
	void					ResetLZW();
	void					ResetDrawing();
	void					SetupDrawRectangles(Rectangle &src,Rectangle &dst);
protected:
	GIFPhase				fPhase;
	ushort					fTop, fLeft, fWidth, fHeight;
	uchar					fFormat;
	uchar					fDepth;
	uchar					fPass;
	
	// pixel by pixel transparency -- total transparency kept elsewhere
	uchar					fTransparencyIndex;		// index of it
	
	Gamma					fGamma;
	uchar					fBlackLevel;
	uchar					fWhiteLevel;
	
	// Buffers for LZW vlc decode
	Byte					*fBuffer;
	short					fBufferCount;
	Byte*					fData;
	ulong					fLast32;
	char					fBits;

	// LZW Decode
	char					fSetCodeSize;
	char					fCodeSize;
	short					fClearCode;
	short					fEndCode;
	short					fMaxCodeSize;
	short					fMaxCode;
	short					fFirstCode;
	short					fOldCode;
	
	short*					fPrefix;
	uchar*					fSuffix;
	uchar*					fStack;
	uchar*					fSP;

	// Drawing
	Rectangle				fDrawRectangle;
	Rectangle				fDrewRectangle;
	Rectangle				fClipRectangle;
	Rectangle				fImageRectangle;
	Rectangle				fSrcBounds;
	Rectangle				fLastDrawRectangle;
	BitMapDevice* 			fBitMap;
	BitMapDevice*			fResizeAccumulateBuffer;
	BitMapDevice*			fResizeBuffer;
	ushort					fYPos;
	ushort					fXPos;
	
	short					fBackGroundColor;
	CLUT*					fGlobalColorTable;
	ulong					fTransparency;
	ushort					fDelayTime;
	ushort					fHoldFrameTime;
	uchar					fDisposalMethod;
	ushort					fLocalWidth;
	ushort					fLocalHeight;
	CLUT*					fLocalColorTable;
	ulong					fWaitTime;
	unsigned long			fLoopCount;
	long					fFrameCount;
	short					fFilterTop;
	short					fFilterBottom;
	FilterType				fFilterType;
	long					fDstYPos;
	long					fDstYInc;
	long					fDstYLast;
	
	unsigned				fInterlaced : 1;			
	unsigned				fTransparent : 1;			
	unsigned				fNotDrawn : 1;
	unsigned				fKeepBitMap : 1;
	unsigned				fFlipHorizontal : 1;
	unsigned				fFlipVertical : 1;
	unsigned				fCacheBitMap : 1;
	unsigned				fDidFilter : 1;
	unsigned				fStartedLZW : 1;
	unsigned				fSkippingExtensionData : 1;
	unsigned				fDataComplete : 1;
	unsigned				fEndOfLZW: 1;
	unsigned				fDidLastFrame: 1;
	unsigned				fIsBackground: 1;
};


enum GIFDisposalMethod {
	kNothingToDispose = 0,
	kDisposalNotSpecified = 1,
	kDoNotDispose = 2,
	kRestoreToBackgroundColor = 3,
	kRestoreToPrevious = 4
};

typedef enum GIFDisposalMethod GIFDisposalMethod;


#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include GIF.h multiple times"
	#endif
#endif /* __GIF_H__ */
