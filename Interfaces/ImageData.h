// ===========================================================================
//	ImageData.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __IMAGEDATA_H__
#define __IMAGEDATA_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"			/* for Priority */
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"			/* for Rectangle */
#endif
#ifndef __LIST_H__
#include "List.h"				/* for Listable */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

class GIF;
class GIFImage;
struct JPEGDecoder;
class Region;
class Stream;


Boolean
ValidateImageType(const Resource *resource,DataType *actualType = nil);



enum KeptBitMapType {
	kNoBitMap = 0,
	kNormalBitMap,
	kCompressedBitMap
};



enum ImageDataPrepStatus {
	kPrepDataError,
	kPrepDataReset,
	kPrepDataResetReady,
	kPrepDataReady
};


// =============================================================================

class ImageData : public Listable {
public:
							ImageData();
	virtual					~ImageData();
	
	static ImageData*		NewImageData(Resource*);
	static ImageData*		NewImageData(const char *url);
	
	virtual Color			AverageColor(ulong backgroundColor);
	virtual void			Draw(Rectangle* r, const Rectangle* invalid, ulong transparency = kNotTransparent);
	virtual Boolean			DrawingIdle(Rectangle* r); 			// returns true if it drew something
#if defined FIDO_INTERCEPT
	virtual void 			Draw(const Rectangle* r, ulong transparency, class FidoCompatibilityState& fidoCompatibility) const;
	virtual Boolean			DrawingIdle(const Rectangle* r, class FidoCompatibilityState& fidoCompatibility) const; 			// returns true if it drew something
#endif
	virtual	void 			DrawTiled(Rectangle tileBounds,Coordinate origin,const Rectangle* invalid = nil,Boolean tileVertical = true,Boolean tileHorizontal= true, ulong transparency = kNotTransparent);
	virtual Boolean			PushData(Boolean sizeOnly = false);
	virtual void			SetBounds(const Rectangle* bounds);
	virtual void			SetFlip(Boolean horizontal,Boolean vertical);
	virtual	void			DrawUpdate();
#if defined FIDO_INTERCEPT
	virtual	void			DrawUpdate(class FidoCompatibilityState& fidoCompatibility) const;
#endif

	Boolean					IsDrawingComplete() const;				// returns true if DrawingIdle should be called
	Error					GetStatus();
	Priority				GetPriority() const;
	const Resource*			GetResource() const;
	Boolean					GetBounds(Rectangle*);	// returns true if bounds is known
	BitMapDevice*			GetBitMap();
	void					GetDrawnBounds(Rectangle *) const;	
	void					SetPriority(Priority);	
	void					SetStream(DataStream* stream = nil);
	void					SetKeepBitMap(KeptBitMapType keep);
	void					SetIsBackground(Boolean);
	void					SetFilterType(FilterType);
	void					SetGamma(Gamma gamma,uchar blackLevel,uchar whiteLevel);
	void					SetValidated(Boolean validData=true);
	const char*				GetName();
protected:

	virtual void			Reset(Boolean nukeBitmap = true);
	virtual void			DeleteBitMap();

	void					DeleteStream();
	ImageDataPrepStatus		PrepData();
	void					MapRectangle(Rectangle &r) const;

protected:
	Rectangle				fBounds;
	Rectangle				fClipRectangle;
	Rectangle				fDrawRectangle;
	Rectangle				fSourceBounds;
	Rectangle				fDrewRectangle;
	Rectangle				fValidBitmapBounds;
	Resource				fResource;
	Color					fAverageColor;
	
	BitMapDevice*			fBitMap;
	DataStream*				fStream;
	ulong					fTransparency;
		
	DataType				fDataType;
	FilterType				fFilterType;
	KeptBitMapType			fKeepBitMap;
	Gamma					fGamma;
	uchar					fBlackLevel;
	uchar					fWhiteLevel;
	unsigned				fDrawingComplete : 1;
	unsigned				fError : 1;
	unsigned				fFlipHorizontal : 1;
	unsigned				fFlipVertical : 1;
	unsigned				fIsBackground : 1;
	unsigned				fDidFilter : 1;
	unsigned				fInDrawIdle : 1;
	unsigned				fDataTypeValid : 1;
};

inline void ImageData::SetPriority(Priority priority)
{
	fResource.SetPriority(priority);
}

inline Priority ImageData::GetPriority() const
{
	return fResource.GetPriority();
}

inline const Resource* ImageData::GetResource() const
{
	return &fResource;
}

// =============================================================================

typedef enum {
	kBitMapCreateStream = 0,
	kBitMapReadHeader,
	kBitMapReadCLUT,
	kBitMapReadBits,
	kBitMapDone
} BitMapState;

class BitMapImage : public ImageData {
public:
	virtual Boolean			PushData(Boolean sizeOnly = false);
	void					SetBitMap(BitMapDevice*);
	void					Write(Stream*);
	void					Write(Stream*, const BitMapDevice*);
protected:
	BitMapArchive			fArchive;
	long					fBitsCount;
	long					fBitsSize;
	BitMapState				fState;
};

// =============================================================================

typedef enum {
	kGIFCreateStream,
	kGIFReadStream,
	kGIFDone
} GIFState;

class GIFImage : public ImageData {
public:
	virtual					~GIFImage();
	virtual Boolean			PushData(Boolean sizeOnly = false);
	virtual void			SetFlip(Boolean horizontal,Boolean vertical);
	virtual void			SetBounds(const Rectangle* bounds);
	virtual	void			DrawUpdate();
#if defined FIDO_INTERCEPT
	virtual	void			DrawUpdate(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual void			Reset(Boolean nukeBitmap = true);
protected:
	virtual void			DeleteBitMap();
	void					DeleteGIF();
protected:
	GIF*					fGIF;
	GIFState				fState;
};

	


// =============================================================================

typedef enum {
	kJPEGCreateStream,
	kJPEGStarted,
	kJPEGDone
} JPEGState;
	
class JPEGImage : public ImageData {
public:
	virtual					~JPEGImage();
	virtual Boolean			PushData(Boolean sizeOnly = false);

protected:
	virtual void			Reset(Boolean nukeBitmap=true);
	void					DeleteJPEGDecoder();
	
protected:
	JPEGDecoder*			fJPEG;
	JPEGState				fState;
};

// =============================================================================

typedef enum {
	kXBMCreateStream,
	kXBMReadHeader,
	kXBMReadBits,
	kXBMDone
} XBMState;


class XBitMapImage : public ImageData {
public:
	virtual Boolean			PushData(Boolean sizeOnly = false);
protected:
	long					ParseDimension(const char *buf,long pending,Boolean widthNotHeight);
	long					DecodeXBM(const char *buf,long pending);
protected:
	virtual void			Reset(Boolean nukeBitmap=true);
	XBMState				fState;
	long					fCurX;
	long					fCurY;
};

// =============================================================================


const kFidoSignature = 0x4649444F;

enum FidoImageType {
	kSimpleVQ
} ;

typedef struct {
	ulong			fSignature;
	FidoImageType	fType;	
	short			fVersion;
	ushort			fWidth;
	ushort			fHeight;
} FidoHeader;


typedef enum {
	kFidoCreateStream,
	kFidoReadHeader,
	kFidoReadBand,
	kFidoReadCodeBook,
	kFidoReadData,
	kFidoDone
} FidoState;


class FidoImage : public ImageData {
public:
	virtual Boolean			PushData(Boolean sizeOnly = false);
protected:
	virtual void			Reset(Boolean nukeBitmap=true);
	FidoHeader				fHeader;
	long					fCodebookSize;
	long					fDataSize;
	long					fBandHeight;
	long					fBandStart;
	long					fDstBandStart;
	FidoState				fState;
};


// =============================================================================


class BorderImage : public Listable {
public:
							BorderImage();
							~BorderImage();
	static BorderImage*		NewBorderImage(const char *url);
	Error					ReadBIF(const char *url);
	Error					AddImage(const char *url);
	void					AddImage(ImageData *data);
	virtual Color			AverageColor(ulong backgroundColor);
	void					Draw(Rectangle* r, const Rectangle* invalid, ulong transparency = kNotTransparent);
#if defined FIDO_INTERCEPT
	void 					Draw(const Rectangle* r, ulong transparency, class FidoCompatibilityState& fidoCompatibility) const;
	void					Draw(const Region* r, ulong transparency, class FidoCompatibilityState& fidoCompatibility) const;
#endif
	void					Draw(const Region* r, const Rectangle* invalid, ulong transparency = kNotTransparent);
	Rectangle				InnerBounds() const;
	Rectangle				PadBounds() const;
	Boolean					GetDrawCenter() const;
	Boolean					GetDrawStretched() const;
	long					GetFrameCount() const;
	void					GetInnerBounds(Rectangle*);
	void					GetOuterBounds(Rectangle*);
	
	void					SetInnerBounds(const Rectangle*);
	void					SetPadBounds(const Rectangle*);
	void					SetDrawCenter(Boolean);
	void					SetDrawStretched(Boolean);
	void					SetFrame(long frame);
	void					SetFilterType(FilterType);
	void					SetHasInnerCorners(Boolean);
	void					SetVisible(Boolean);
	void					SetAnimationRange(long firstFrame,long lastFrame);
	void					SetBounce(Boolean);
	void					SetDelay(long delay);
	void					Idle(Boolean forceUpdate=false);
	void					ReDraw();
protected:
	void					PrepDraw();
	
	BitMapDevice			*fBitMap;
	Rectangle				fBounds;
	Rectangle				fInnerBounds;
	Rectangle				fPadBounds;
	Rectangle				fDrawRectangle;
	Region*					fDrawRegion;
	ulong					fTransparency;
	Rectangle				fClipRectangle;
	long					fFrameNumber;
	long					fFirstFrame;
	long					fLastFrame;
	ObjectList				fImageList;
	ImageData				*fImageData;
	FilterType				fFilterType;
	long					fDelay;
	ulong					fNextDisplayTime;
	unsigned				fDrawCenter : 1;
	unsigned				fDrawStretched : 1;
	unsigned				fHasInnerCorners : 1;
	unsigned				fBounce : 1;
	unsigned				fAnimate : 1;
	unsigned				fBackwards : 1;
	unsigned				fVisible : 1;
	unsigned				fFirstFrameOnce : 1;
	unsigned				fLoop : 1;
};






#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include ImageData.h multiple times"
	#endif
#endif /* __IMAGEDATA_H__ */
