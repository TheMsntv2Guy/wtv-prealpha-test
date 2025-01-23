// ===========================================================================
//	ImageData.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __GIF_H__
#include "GIF.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif
#ifndef __VECTORQUANTIZATION_H__
#include "VectorQuantization.h"
#endif
#ifndef __JPINCLUDES_H__
#include "jpIncludes.h"
#endif
#ifndef __LAYER_H__
#include "Layer.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __REGION_H__
#include "Region.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif
#ifndef __PPP_H__
#include "ppp.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif



// ===========================================================================
//	implementation
// ===========================================================================

static long CLUTSize(short format)
{
	switch (format) { 										/*offsetof(CLUT,data)*/
		case index1Format:	return 3 * 2   + 4;
		case index2Format:	return 3 * 4   + 4;
		case index4Format:	return 3 * 16  + 4;
		case index8Format:	return 3 * 256 + 4;
		case alpha8Format:	return 4 * 256 + 4; 
		case vqFormat:		return 8 * 256 + 4;
		default:			break;
	}
	return 0;
}


// =============================================================================


const FilterType kDefaultFilter = kNoFilter;
//const kDefaultGamma = 0;
const kDefaultGamma = 0;

ImageData::ImageData()
{
#ifdef FIDO_INTERCEPT
	fKeepBitMap = kNormalBitMap;
#endif
	fDrawingComplete = true;
	fAverageColor = (Color)-1;
	fFilterType = kDefaultFilter;	
	fWhiteLevel = 255;	
	fGamma = kDefaultGamma;	
}

ImageData::~ImageData()
{
	Reset();
}

Color ImageData::AverageColor(ulong backgroundColor)
{
	if ( fAverageColor == (Color)-1 ) {
		if ( fBitMap ) {
			fAverageColor = AverageImage(*fBitMap,backgroundColor);
	}
	}
	if ( fAverageColor == (Color)-1 ) 
		return 0x808080;
	return fAverageColor;
}

void ImageData::DeleteStream()
{
	if (fStream == nil)
		return;
	delete(fStream);
	fStream = nil;
}

void ImageData::DeleteBitMap()
{
	if (fBitMap )  {
		DeleteBitMapDevice(fBitMap);
		fBitMap = nil;
	}
	SetRectangle(fValidBitmapBounds,0,0,0,0);
}


void ImageData::SetBounds(const Rectangle* bounds)
{	
	fSourceBounds = *bounds;
}


void ImageData::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency)
{

	if (invalid != nil && !RectanglesIntersect(r, invalid))
		return;

	// If a draw is still in progress, add this clip rectangle with the previous

	Rectangle	newClipRectangle = (invalid == nil) ? *r : *invalid;	
	PostulateFinal(false); // Dave
	if (EqualRectangles(r,&fDrawRectangle) && !fDrawingComplete && !fIsBackground)
		UnionRectangle(&newClipRectangle, &fClipRectangle);	
	IntersectRectangle(&newClipRectangle, r);
	
	fClipRectangle = newClipRectangle;				
	fDrawRectangle = *r;
	fTransparency = transparency;
	Reset(fKeepBitMap == kNoBitMap);
	fError = false;
	fDrawingComplete = false;
	fDidFilter = false;
	DrawingIdle(r);
}


Boolean ImageData::DrawingIdle(Rectangle* r)
{
	SetRectangle(fDrewRectangle,0,0,0,0);
	if ( r == nil )
		 r = &fDrawRectangle;
	if ( EqualRectangles(r,&fDrawRectangle) ) {
		if ( fDrawingComplete ) 
			return false;
	}
	else
		Reset(fKeepBitMap == kNoBitMap);

	fDrawRectangle = *r;
	
	if ( fError && !fDrawingComplete ) {
		Reset();
		fDrawingComplete = true;
		return false;
	}
	
	
	fInDrawIdle = true;
	if (  ((fKeepBitMap == kNoBitMap || fBitMap == nil) && !fDrawingComplete ) || 
			fBitMap && !EqualRectangles(&fValidBitmapBounds,&fBitMap->bounds) )
		PushData();
	
	DrawUpdate();
	
//	FrameRectangle(gScreenDevice,fDrewRectangle,1,0xff0000,128);
	Rectangle filterRect = fDrewRectangle;
	IntersectRectangle(&filterRect,&fClipRectangle);
	if ( !fDidFilter && !fIsBackground ) {
		if ( EqualRectangles(&filterRect,&fClipRectangle)) { 
			FilterType filterType = kFullFilter;
			if ( filterRect.top > gScreenDevice.bounds.top ) 
				filterRect.top--;
			else
				filterType = kBottomFilter;
			if ( filterRect.bottom < gScreenDevice.bounds.bottom ) 
				filterRect.bottom++;
			else
				filterType = filterType == kBottomFilter ? kSliceFilter : kTopFilter;
			FlickerFilterBounds(gScreenDevice,filterRect,filterType,nil);
			fDidFilter = true;
		}
		else
			FlickerFilterBounds(gScreenDevice,fDrewRectangle,kSliceFilter,&fClipRectangle);
	}
	if ( fKeepBitMap && fBitMap )
		fDrawingComplete = true;
		
	// completed drawing, so do display and dispose data
	
	if ( fDrawingComplete  ) {
		Reset(fKeepBitMap == kNoBitMap);
	}
	*r = fDrewRectangle;
	fInDrawIdle = false;
	return !EmptyRectangle(&fDrewRectangle);
}



void ImageData::SetFlip(Boolean shouldFlipH,Boolean shouldFlipV)
{
	fFlipHorizontal = shouldFlipH;
	fFlipVertical = shouldFlipV;
}
	
void ImageData::SetKeepBitMap(KeptBitMapType keep)
{
#ifdef FIDO_INTERCEPT
	keep = kNormalBitMap;
#endif
	if ( fKeepBitMap != keep ) {
		fKeepBitMap = keep;
		if (!keep)
			Reset();
	}
}


void ImageData::MapRectangle(Rectangle &r) const
{
	if ( !EmptyRectangle(&r) && !EmptyRectangle(&fDrawRectangle) ) {
		long width = r.right - r.left;
		long height = r.bottom - r.top;
		long srcWidth = fBounds.right - fBounds.left;
		long srcHeight = fBounds.bottom - fBounds.top;
		long dstWidth = fDrawRectangle.right - fDrawRectangle.left;
		long dstHeight = fDrawRectangle.bottom - fDrawRectangle.top;
		
		if ( fSourceBounds.right != fSourceBounds.left ) {
			srcWidth = fSourceBounds.right - fSourceBounds.left;
			srcHeight = fSourceBounds.bottom - fSourceBounds.top;
		}
		if ( srcWidth == 0 || srcHeight == 0 ) 
			SetRectangle(r,0,0,0,0);
		else {
			if ( dstWidth != srcWidth )
				width = (dstWidth * width) / srcWidth;
			if ( dstHeight != srcHeight )
				height = (dstHeight * height) / srcHeight;

			r.left = fDrawRectangle.left;
			r.top = fDrawRectangle.top;
			r.right = r.left + width;
			r.bottom = r.top + height;
		}
	}
}

void ImageData::DrawUpdate()
{
	if ( fBitMap && !EmptyRectangle(&fValidBitmapBounds) ) {
		Rectangle refreshFromBitmapClip;
		Rectangle *srcRect = nil;

		if ( fSourceBounds.right != fSourceBounds.left )
			srcRect = &fSourceBounds;
			
		refreshFromBitmapClip = fValidBitmapBounds;
		MapRectangle(refreshFromBitmapClip);
		IntersectRectangle(&refreshFromBitmapClip,&fClipRectangle);
		UnionRectangle(&fDrewRectangle,&refreshFromBitmapClip);
		DrawImage(gScreenDevice, *fBitMap, fDrawRectangle, fTransparency, &refreshFromBitmapClip,srcRect,fFlipHorizontal, fFlipVertical);

	}
}


Boolean ImageData::GetBounds(Rectangle* bounds)
{
		
	if ( fSourceBounds.left != fSourceBounds.right ) {
		*bounds = fSourceBounds;
		return true;
	}
	if ( fBitMap && fBounds.right == fBounds.left ) {
		fBounds = fBitMap->bounds;
	}
	if ( fBounds.right != fBounds.left ) {
		*bounds = fBounds;
		return true;
	}
	PushData(true);
	*bounds = fBounds;
	Reset();
	return (bounds->right != bounds->left && bounds->bottom != bounds->top);
}

		
Error ImageData::GetStatus()
{
	if ( !fDataTypeValid ) {
		DataType at;
		fDataTypeValid = ::ValidateImageType(&fResource,&at);
		if ( fDataTypeValid ) {
			if ( at != fDataType ) {
				fResource.SetDataType(at);
				return kWrongImageType;
			}
		}
	}
	if ( fError )
		return kGenericError;
	return fResource.GetStatus();
}
	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void ImageData::GetDrawnBounds(Rectangle* bounds) const
{
	*bounds = fDrewRectangle;
}
#endif
		
Boolean ImageData::IsDrawingComplete() const
{
	return fDrawingComplete;
}


// returns true if data matches expected type

Boolean
ValidateImageType(const Resource *resource,DataType *actualType)
{
	DataType newDataType = resource->GetDataType();
	DataStream *stream = resource->NewStream();
	
	if ( stream == nil) {
		return false;
	}
	if ( stream->GetPending() > 0 ) {
		uchar c = *stream->GetData();
		switch (c) {
		case 0xff:
			newDataType = kDataTypeJPEG;
			break;
		case 'G':
			newDataType = kDataTypeGIF;
			break;
		case 'F':
			newDataType = kDataTypeFidoImage;
			break;
		case '#':
		case ' ':
		case '\t':
		case 0xd:
		case 0xa:
			newDataType = kDataTypeXBitMap;
			break;
		case 0:
			newDataType = kDataTypeBitmap;
			break;
		default:
			if ( IsWarning(c) ) {
				Message(("Unknown image data '%s' %x",resource->GetCacheEntry()->GetName(),c));
				delete(stream);
				newDataType = (DataType)0;
				return true;
			}
		}
	} 
	else {
		delete(stream);
		if ( actualType )
			*actualType = newDataType;
		return false;
	}
	delete(stream);
	if ( actualType )
		*actualType = newDataType;
	return true;
}

void
ImageData::SetValidated(Boolean dataTypeValid)
{
	fDataTypeValid = dataTypeValid;
}

ImageData* ImageData::NewImageData( Resource* resource)
{
	ImageData* 	imageData;
	DataType 	dtype,ndtype;
	Boolean		validated;
	
	if (IsError(resource == nil))
		return nil;
		
	dtype = resource->GetDataType();
	validated = ::ValidateImageType(resource,&ndtype);

	switch (dtype) {
	case kDataTypeBitmap:		imageData = new(BitMapImage); break;
	case kDataTypeGIF:			imageData = new(GIFImage); break;
	case kDataTypeJPEG:			imageData = new(JPEGImage); break;
	case kDataTypeXBitMap:		imageData = new(XBitMapImage); break;
	case kDataTypeFidoImage:	imageData = new(FidoImage); break;
	default:					
		switch ( ndtype ) {
			case kDataTypeBitmap:		imageData = new(BitMapImage); break;
			case kDataTypeJPEG:			imageData = new(JPEGImage); break;
			case kDataTypeXBitMap:		imageData = new(XBitMapImage); break;
			case kDataTypeFidoImage:	imageData = new(FidoImage); break;
			default:					ndtype = kDataTypeGIF; imageData = new(GIFImage); break;
		}
		TrivialMessage(("invalid image type %lx - defaulting to %lX",dtype,ndtype));
		resource->SetDataType(ndtype);
		break;
	}
	imageData->fDataType = resource->GetDataType();
	imageData->fResource = *resource;
	imageData->SetValidated(validated);
	return imageData;
}


ImageData* ImageData::NewImageData(const char* url)
{
	ImageData* imageData;
	Resource resource;
	Boolean		validated;
	DataType ndtype;
	
	if (IsError(url == nil))
		return nil;
	
	resource.SetURL(url);
	resource.SetDataType(GuessDataType(url));
	validated =  ::ValidateImageType(&resource,&ndtype);
	switch (resource.GetDataType()) {
		case kDataTypeBitmap:		imageData = new(BitMapImage); break;
		case kDataTypeGIF:			imageData = new(GIFImage); break;
		case kDataTypeJPEG:			imageData = new(JPEGImage); break;
		case kDataTypeXBitMap:		imageData = new(XBitMapImage); break;
		case kDataTypeFidoImage:	imageData = new(FidoImage); break;
		default:				
			switch ( ndtype ) {
				case kDataTypeBitmap:		imageData = new(BitMapImage); break;
				case kDataTypeJPEG:			imageData = new(JPEGImage); break;
				case kDataTypeXBitMap:		imageData = new(XBitMapImage); break;
				case kDataTypeFidoImage:	imageData = new(FidoImage); break;
				default:					ndtype = kDataTypeGIF; imageData = new(GIFImage); break;
			}
			TrivialMessage(("invalid image type %lx - defaulting to %lX",resource.GetDataType(),ndtype)); 
			resource.SetDataType(ndtype);
		break;
	}
	imageData->fDataType = resource.GetDataType();
	imageData->SetValidated(validated);
	imageData->fResource = resource;
	return imageData;
}


ImageDataPrepStatus ImageData::PrepData()
{
	if (fStream != nil) {
		Error status = fStream->GetStatus();
		if (status == kStreamReset) {
			Reset(fKeepBitMap == kNoBitMap);
		}
		else if (status == kTruncated) {
			fStream->SetStatus(kStreamReset);
			Reset(fKeepBitMap == kNoBitMap);
		}
		else if (TrueError(status)) {
			Reset();
			return kPrepDataError;
		}
	}

	if ( fStream == nil ) {
		SetStream();
		if ( fStream == nil ) {
			Reset();
			return kPrepDataReset;
		}
		return kPrepDataResetReady;
	}
	
	return kPrepDataReady;
}


Boolean ImageData::PushData(Boolean)
{
	return false;
}

void ImageData::SetIsBackground(Boolean isBackground)
{	
	if ( isBackground ) {
		FilterType filterType = kSliceFilter;
		if  ( fFilterType == kFullFilter || fFilterType == kTopFilter  ) 
			filterType = kSliceFilter;
		if  ( fFilterType == kFullFilterSlight || fFilterType == kTopFilterSlight  ) 
			filterType = kSliceFilterSlight;
		SetFilterType(filterType);
		fIsBackground = true;		// do this after, since SetFilterType wont work on background images 
	} else {
		fIsBackground = false; 
		SetFilterType(kDefaultFilter);
	}
}
		

void ImageData::SetStream(DataStream* stream)
{
	if (IsWarning(fStream != nil)) {
		delete(fStream);
		fStream = nil;
	}
	
	if ( stream == nil ) {
		stream = fResource.NewStream();
		if (stream == nil)
			return;
	}
	fStream = stream;
}


void ImageData::Reset(Boolean nukeBitmap)
{
	DeleteStream();
	if ( nukeBitmap )
		DeleteBitMap();
}


BitMapDevice* ImageData::GetBitMap()
{
	if ( fBitMap == nil )
		PushData();
	return fBitMap;
}


void ImageData::SetFilterType(FilterType filterType)
{
	if ( !fIsBackground )
		fFilterType = filterType;
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void ImageData::SetGamma(Gamma gamma,uchar blackLevel,uchar whiteLevel)
{
	fGamma = gamma;
	fBlackLevel = blackLevel;
	fWhiteLevel = whiteLevel;
}
#endif


void ImageData::DrawTiled(Rectangle tileBounds,Coordinate origin,const Rectangle* invalid,  Boolean tileVertical,Boolean tileHorizontal,ulong transparency)
{
	long		x,y;
	long 		width,height;
	Rectangle	bounds;
	long		repeats = 0;
	Rectangle	drawnRectangle = {0,0,0,0};
	
	if (invalid != nil && !RectanglesIntersect(&tileBounds, invalid))
		return;
	if ( !GetBounds(&bounds) )
		return;

	if ( invalid )
		IntersectRectangle(&tileBounds,invalid);
		
	width = bounds.right - bounds.left;
	height = bounds.bottom - bounds.top;
	
	if ( origin.y > tileBounds.top )
		origin.y -= height;
	if ( origin.x > tileBounds.left )
		origin.x -= width;
	
	fIsBackground = true;
		
	// count repeats to see if we should keep bitmap

	for ( y=origin.y; y < tileBounds.bottom;  ) {
		if ( y >= tileBounds.top-height ) {
			for ( x=origin.x; x < tileBounds.right; ) {
				if ( x >= tileBounds.left-width ) {
					repeats++;
				}
				if ( tileHorizontal )
					x += width;
				else
					break;
			}
		}
		if ( tileVertical )
			y += height;
		else
			break;
	}
	if ( repeats > 0 ) {
	
		TrivialMessage(("Background tile drawn %d times",repeats));
		
		if ( repeats > 1 ) {
			SetKeepBitMap(kNormalBitMap);
//			GetBitMap();
		}
			
		// draw the tiles
		
		for ( y=origin.y; y < tileBounds.bottom;  ) {
			if ( y >= tileBounds.top-height ) {
				for ( x=origin.x; x < tileBounds.right; ) {
					if ( x >= tileBounds.left-width ) {
						SetRectangle(bounds,x,y,x+width,y+height);
						Draw(&bounds,&tileBounds,transparency);
						UnionRectangle(&drawnRectangle,&fDrewRectangle);
					}
					if ( tileHorizontal )
						x += width;
					else
						break;
				}
			}
			if ( tileVertical )
				y += height;
			else
				break;
		}
		fDrewRectangle = drawnRectangle;
		SetKeepBitMap(kNoBitMap);
		FlickerFilterBounds(gScreenDevice,tileBounds,kSliceFilter,nil);
	}
}

const char *
ImageData::GetName()
{
	return fResource.GetCacheEntry()->GetName();
}


// =============================================================================




Boolean BitMapImage::PushData(Boolean justHeader)
{
	long		pending;
	long		clutSize;
	CLUT*		clut;
	
	if ( !fDataTypeValid ) 
		return false;
	switch ( PrepData() ) {
	case kPrepDataError:
		fError = true;
		return false;
	case kPrepDataReset:
		return false;
	case kPrepDataResetReady:
		fState = kBitMapCreateStream;
		break;
	case kPrepDataReady:
		break;
	}
		
	
	switch (fState) {
		case kBitMapCreateStream:
			fError = false;
			fBitsCount = 0;
			fState = kBitMapReadHeader;
					
		case kBitMapReadHeader:
			if (fStream->GetPending() < (long)sizeof(fArchive))
				return false;
			fStream->Read(&fArchive, sizeof (fArchive));
			fState = kBitMapReadCLUT;
			fBounds = fArchive.bounds;
			
		case kBitMapReadCLUT:
			if ( justHeader )
				return true;
			clutSize = fArchive.clutSize;

			if ((pending = fStream->GetPending()) < clutSize)
				return false;
			
			if (clutSize != 0) {
				clut = (CLUT*)AllocateTaggedMemory(clutSize, "Color Table");
				fStream->Read(clut, clutSize);
			} else
				clut = nil;
			
			fBitMap = NewBitMapDevice(fArchive.bounds, fArchive.format, clut, fArchive.transparentColor);
			if ( IsWarning(fBitMap == nil)) {
				Message(("failed to allocate bitmap for bmi image"));
				Reset();
				fError = true;
				fDrawingComplete = true;
				break;
			}
			else {
				fBitsSize = BitMapPixelBufferSize(fBitMap);
				fState = kBitMapReadBits;
			}
					
		case kBitMapReadBits:
			if ( justHeader )
				return true;
			if ((pending = fStream->GetPending()) <= 0)
				return false;
			
			pending = MIN(pending, fBitsSize - fBitsCount);
			fStream->Read(fBitMap->baseAddress + fBitsCount, pending);
			fBitsCount += pending;
			
			if (fBitsCount == fBitsSize) {
				fState = kBitMapDone;
				fBitsCount = 0;
			}
			fValidBitmapBounds = fArchive.bounds;
		case kBitMapDone:
			fDrawingComplete = true;
			break;
	}

	return true;
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void BitMapImage::SetBitMap(BitMapDevice* value)
{
	fAverageColor = (Color)-1;
	fBitMap = value;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void BitMapImage::Write(Stream* stream)
{
	if (IsError(fBitMap == nil || stream == nil))
		return;
		
	Write(stream, fBitMap);
}
#endif

void BitMapImage::Write(Stream* stream, const BitMapDevice* bitMap)
{
	if (IsError(stream == nil || bitMap == nil))
		return;
	
	fArchive.version	= 0;
	fArchive.format		= bitMap->format;
	fArchive.rowBytes	= bitMap->rowBytes;
	fArchive.bounds		= bitMap->bounds;
	fArchive.transparentColor = bitMap->transparentColor;
	fArchive.clutSize 	= 0;
	if ( bitMap->colorTable && CLUTSize(bitMap->format) )
		fArchive.clutSize = 4 + bitMap->colorTable->size;
	stream->Write(&fArchive, sizeof (fArchive));

	if ( fArchive.clutSize )
		stream->Write(bitMap->colorTable,fArchive.clutSize);
	stream->Write(bitMap->baseAddress, BitMapPixelBufferSize(bitMap));
}

// =============================================================================

BorderImage::BorderImage()
{
	fImageData = nil;
	fFilterType = kDefaultFilter;
	fBitMap = nil;
}

BorderImage::~BorderImage()
{
	fImageList.DeleteAll();
	fImageData = nil;
}


BorderImage *BorderImage::NewBorderImage(const char *url)
{
	BorderImage *image = new(BorderImage);
	if ( IsError(image==nil) )
		return nil;
	image->SetFilterType(kFullFilter);
	if ( IsWarning(image->ReadBIF(url) != kNoError) ) {
		delete(image);
		image = nil;
	}
	return image;
}

Error BorderImage::ReadBIF(const char *url)
{
	Resource *resource = nil;
	DataStream *stream = nil;
	const char *s,*ns;
	int		i;
	const kMaxTag = 256;
	char tag[kMaxTag];
	Error error = kGenericError;
	Boolean gotBounds = false;
	static const char kImageTag[] 		= "IMAGE";			// url
	static const char kInnerBoundsTag[] = "INNERBOUNDS";	// rectangle
	static const char kPadBoundsTag[] 	= "PADBOUNDS";		// rectangle
	static const char kDrawCenterTag[] 	= "DRAWCENTER";
	static const char kScaleCenterTag[] = "SCALECENTER";
	static const char kInnerCornerTag[] = "HASINNERCORNERS";
	static const char kNoFilterTag[] 	= "NOFILTER";
	static const char kAnimateTag[] 	= "ANIMATE";
	static const char kBounceTag[] 		= "BOUNCE";
	static const char kDelayTag[] 		= "DELAY";
	static const char kFirstFrameOnceTag[] 		= "FIRSTFRAMEONCE";
	static const char kLoopTag[] 		= "LOOP";
	Rectangle	r;
	
	if (IsError(url == nil))
		return kGenericError;
	resource = new(Resource);
	resource->SetURL(url);
	stream = resource->NewStream();
	if ( IsError(stream == nil) )
		return kGenericError;
	s = stream->GetDataAsString();
	while ( 1 ) {
		while ( isspace(*s) )
			s++;
		i = 0;
		while ( *s != '\n' && *s != '\r'  ) {
			if ( *s == 0 )
				goto done;
			tag[i++] = *s++;
			if ( i == kMaxTag ) {
				Message(("Error parsing bif '%s'",url));
				goto done;
			}
		}
		if ( i == 0 )
			continue;
		if ( tag[0] == '#' )		// comment
			continue;	
		tag[i] = 0;
		ns = tag;
		if ( EqualStringN(tag,kImageTag,sizeof(kImageTag)-1)  ) {
			ns += sizeof(kImageTag)-1;
			while ( isspace(*ns) )
				ns++;
			if ( (error=AddImage(ns)) != kNoError ) 
				goto done;
		}
		else if ( EqualStringN(tag,kInnerBoundsTag,sizeof(kInnerBoundsTag)-1) ) {
			ns += sizeof(kInnerBoundsTag)-1;
			if ( sscanf(ns,"%ld%*[ ,]%ld%*[ ,]%ld%*[ ,]%ld",&r.left,&r.top,&r.right,&r.bottom) != 4 ) {
				error = kGenericError;
				goto done;
			}
			SetInnerBounds(&r);
			gotBounds = true;
		}
		else if ( EqualStringN(tag,kPadBoundsTag,sizeof(kPadBoundsTag)-1) ) {
			ns += sizeof(kPadBoundsTag)-1;
			if ( sscanf(ns,"%ld%*[ ,]%ld%*[ ,]%ld%*[ ,]%ld",&r.left,&r.top,&r.right,&r.bottom) != 4 ) {
				error = kGenericError;
				goto done;
			}
			SetPadBounds(&r);
		}
		else if ( EqualStringN(tag,kDrawCenterTag,sizeof(kDrawCenterTag)-1)  ) {
			SetDrawCenter(true);
		}	
		else if ( EqualStringN(tag,kScaleCenterTag,sizeof(kScaleCenterTag)-1)  ) {
			SetDrawStretched(true);
		}	
		else if ( EqualStringN(tag,kInnerCornerTag,sizeof(kInnerCornerTag)-1)  ) {
			SetHasInnerCorners(true);
		}	
		else if ( EqualStringN(tag,kNoFilterTag,sizeof(kNoFilterTag)-1)  ) {
			SetFilterType(kNoFilter);
		}	
		else if ( EqualStringN(tag,kAnimateTag,sizeof(kAnimateTag)-1)  ) {
			fAnimate = true;
		}	
		else if ( EqualStringN(tag,kBounceTag,sizeof(kBounceTag)-1)  ) {
			fBounce = true;
		}	
		else if ( EqualStringN(tag,kLoopTag,sizeof(kLoopTag)-1)  ) {
			fLoop = true;
		}	
		else if ( EqualStringN(tag,kFirstFrameOnceTag,sizeof(kFirstFrameOnceTag)-1)  ) {
			fFirstFrameOnce = true;
		}	
		else if ( EqualStringN(tag,kDelayTag,sizeof(kDelayTag)-1)  ) {
			long l;
			ns += sizeof(kDelayTag)-1;
			if ( sscanf(ns,"%ld",&l) == 1 ) 
				fDelay = l;
			else
				fDelay = 0;
		}	
		else {
			Message(("Ignoring bif line '%s'",tag));
		}
	}
	if ( fImageData && gotBounds )
		error = kNoError;
done:
	fFirstFrame = 0;
	fLastFrame = fImageList.GetCount() - 1;
	delete(stream);
	delete(resource);
	return error;
}



Error BorderImage::AddImage(const char *url)
{
	ImageData *img = ImageData::NewImageData(url);
	if ( img == nil )
		return kGenericError;
	AddImage(img);
	return kNoError;
}

void BorderImage::AddImage(ImageData *imageData)
{
	if ( IsError(imageData == nil) )
		return;
	fImageList.Add(imageData);
	imageData->SetFilterType(fFilterType);
	if ( fImageData == nil )
		PrepDraw();
}


Color BorderImage::AverageColor(ulong backgroundColor)
{
	if (fImageData != nil)
		return fImageData->AverageColor(backgroundColor);
	return 0x808080;
}

void BorderImage::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency  )
{
	if ( fImageData == nil )
		PrepDraw();
	
	if ( fImageData == nil || fBitMap == nil)
		return;
	if ( r == nil || EmptyRectangle(r) )
		return;		
	
	if ( fDrawRegion )
		delete(fDrawRegion);
	fDrawRegion = nil;
	fDrawRectangle = *r;

	if ( invalid ) {
		Rectangle t = *r;
		GetOuterBounds(&t);
		IntersectRectangle(&t,invalid);
		if ( t.right == t.left || t.top == t.bottom )
			return;
		if ( !fDrawCenter ) {
			t = *r;
			IntersectRectangle(&t,invalid);
			if ( t.left > r->left && t.right < r->right &&  t.top > r->top && t.bottom < r->bottom )
				return;
		}
	}
	fTransparency = transparency;
	if ( invalid )
		fClipRectangle = *invalid;
	else
		fClipRectangle = fDrawRectangle;
	::DrawBorderImage(gScreenDevice, *fBitMap, *r, fInnerBounds, transparency, invalid, fDrawStretched, fDrawCenter);
}

void BorderImage::Draw(const Region* region, const Rectangle* invalid,ulong transparency )
{
	if ( fImageData == nil )
		PrepDraw();
	if ( fImageData == nil || fBitMap == nil)
		return;
	if ( region == nil || region->IsEmpty() )
		return;		
	if ( fDrawRegion )
		delete(fDrawRegion);
	fDrawRegion = nil;
	SetRectangle(fDrawRectangle,0,0,0,0);
	if (region->IsEmpty())
		return;
	fDrawRegion = region->NewCopy();
	if ( invalid ) {
		Rectangle t = *invalid;
		t.top -= fInnerBounds.top;
		t.left -= fInnerBounds.left;
		t.right += fInnerBounds.right;
		t.bottom += fInnerBounds.bottom;
		if  ( !region->Intersects(&t) )
			return;
	}
	fTransparency = transparency;
	if ( invalid )
		fClipRectangle = *invalid;
	else
		fDrawRegion->GetBounds(&fClipRectangle);
	::DrawBorderImage(gScreenDevice, *fBitMap, region, fInnerBounds, transparency, invalid,fDrawStretched,fDrawCenter,fHasInnerCorners);
}

void BorderImage::SetVisible(Boolean visible)
{
	fNextDisplayTime = Now();
	fVisible = visible;
}
		
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void BorderImage::SetBounce(Boolean bounce)
{
	fBounce = bounce;
}
#endif

void BorderImage::ReDraw()
{
	if ( fVisible ) {
		if ( fDrawRegion && !fDrawRegion->IsEmpty() )
			::DrawBorderImage(gScreenDevice, *fBitMap, fDrawRegion, fInnerBounds, fTransparency, &fClipRectangle,fDrawStretched,fDrawCenter,fHasInnerCorners);
		else if ( !EmptyRectangle(&fDrawRectangle) )
			::DrawBorderImage(gScreenDevice, *fBitMap, fDrawRectangle, fInnerBounds, fTransparency, &fClipRectangle,fDrawStretched,fDrawCenter);
	}
}

long BorderImage::GetFrameCount() const
{
	return fImageList.GetCount();
}

void BorderImage::GetInnerBounds(Rectangle* bounds)
{
	// Bounds is outer bounds to start.

	Rectangle sourceBounds;
	if ( IsWarning(fImageData== nil) ) {
		SetRectangle(*bounds,0,0,0,0);
		return;
	}
	fImageData->GetBounds(&sourceBounds);

	if (IsError(fInnerBounds.top < sourceBounds.top
		|| fInnerBounds.left < sourceBounds.left
		|| fInnerBounds.bottom > sourceBounds.bottom
		|| fInnerBounds.right > sourceBounds.right)) {
		return;
	}
		
	bounds->top = bounds->top + (fInnerBounds.top - sourceBounds.top);
	bounds->left = bounds->left + (fInnerBounds.left - sourceBounds.left);
	bounds->bottom = bounds->bottom - (sourceBounds.bottom - fInnerBounds.bottom);
	bounds->right = bounds->right - (sourceBounds.right - fInnerBounds.right);
}


void BorderImage::GetOuterBounds(Rectangle* bounds)
{
	// Bounds is inner bounds to start.

	Rectangle sourceBounds;
	if ( IsWarning(fImageData== nil) ) {
		SetRectangle(*bounds,0,0,0,0);
		return;
	}
	fImageData->GetBounds(&sourceBounds);

	if (IsError(fInnerBounds.top < sourceBounds.top
		|| fInnerBounds.left < sourceBounds.left
		|| fInnerBounds.bottom > sourceBounds.bottom
		|| fInnerBounds.right > sourceBounds.right)) {
		return;
	}
		
	PostulateFinal(false);	// John, why do we need the +/- 1?
	bounds->top = bounds->top - (fInnerBounds.top - sourceBounds.top) - 1;
	bounds->left = bounds->left - (fInnerBounds.left - sourceBounds.left) - 1;
	bounds->bottom = bounds->bottom + (sourceBounds.bottom - fInnerBounds.bottom) + 1;
	bounds->right = bounds->right + (sourceBounds.right - fInnerBounds.right) + 1;
}

Rectangle BorderImage::InnerBounds() const
{
	return fInnerBounds;
}

Rectangle BorderImage::PadBounds() const
{
	return fPadBounds;
}


void BorderImage::Idle(Boolean forceUpdate)
{
	if ( fAnimate && fVisible && fNextDisplayTime != 0 ) {
		if ( Now() < fNextDisplayTime )
			return;
		fNextDisplayTime = Now() + fDelay;

		if ( fBounce ) {
			if ( fBackwards ) {
				if ( fFrameNumber-- == fFirstFrame ) {
					fFrameNumber++;
					if ( fLoop )  {
						fBackwards = false;
					}
					else
						fNextDisplayTime = 0;
				}
			} else {
				if ( fFrameNumber++ == fLastFrame ) {
					fFrameNumber--;
					if ( fLoop )  {
						fBackwards = true;
					}
					else
						fNextDisplayTime = 0;
				}
			}
		} else {
			if ( fBackwards ) {
				if ( fFrameNumber-- == fFirstFrame ) {
					fFrameNumber = fLastFrame;
					if ( !fLoop ) 
						fNextDisplayTime = 0;
				}
			} else {
				if ( fFrameNumber++ == fLastFrame ) {
					fFrameNumber = fFirstFrame;
					if ( !fLoop ) 
						fNextDisplayTime = 0;
				}
			}
		}
		if ( !forceUpdate && fFirstFrameOnce && fFrameNumber == fFirstFrame && fFirstFrame != fLastFrame )
			fFrameNumber += 1;
		PrepDraw();
		ReDraw();
	}
}
	
void BorderImage::PrepDraw()
{
		
	if ( fImageData ) {
		fImageData->SetKeepBitMap(kNoBitMap);
	}
	fImageData = (ImageData*)fImageList.At(fFrameNumber);
	if ( IsWarning(fImageData == nil) )
		return;
	fImageData->SetFilterType(fFilterType);
	fImageData->SetKeepBitMap(kNormalBitMap);
	fBitMap = fImageData->GetBitMap();
	if ( IsWarning(fBitMap == nil) ) {
#ifdef	DEBUG
		const char *s = fImageData->GetName();
		Message(("Nil border image '%s' frame %ld",s,fFrameNumber));
#endif
		fImageData = nil;
	}
}

void BorderImage::SetFrame(long frameNum) 
{
	if ( fAnimate )
		return;
	if ( IsWarning(frameNum < 0 ) )
		return;
	if ( IsWarning(frameNum >= fImageList.GetCount()) ) {
		frameNum = fImageList.GetCount();
		if ( frameNum == 0 )
			return;
		frameNum--;
	}
	fFrameNumber = frameNum;
	PrepDraw();
}

void BorderImage::SetAnimationRange(long firstFrame,long lastFrame) 
{
	if ( IsWarning(firstFrame < 0 || firstFrame >= fImageList.GetCount()) )
		return;
	if ( IsWarning(lastFrame < 0 || lastFrame >= fImageList.GetCount()) )
		return;
		
	if ( fFirstFrame == fLastFrame ) {
		fAnimate  = false;
		SetFrame(fFirstFrame);
	}
	if ( firstFrame > lastFrame )
		fBackwards = true;
	fFirstFrame = firstFrame;
	fLastFrame = lastFrame;
	fFrameNumber = fFirstFrame;
	fNextDisplayTime = Now();
	PrepDraw();
	Idle(true);
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void BorderImage::SetDelay(long delay)
{
	fDelayCount = fDelay = delay;
}
#endif

void BorderImage::SetInnerBounds(const Rectangle* value)
{
	fInnerBounds = *value;
}


void BorderImage::SetPadBounds(const Rectangle* value)
{
	fPadBounds = *value;
}

void BorderImage::SetFilterType(FilterType filterType)
{
	int i;
	ImageData *image;
	if ( fFilterType != filterType ) {
		
	fFilterType = filterType;
		for  (i=0; i < fImageList.GetCount(); i++ ) {
			image = (ImageData*)fImageList.At(i);
			if ( image )
				image->SetFilterType(fFilterType);
		}
	}
}


void BorderImage::SetHasInnerCorners(Boolean hasEm)
{
	fHasInnerCorners = hasEm;
}


void BorderImage::SetDrawCenter(Boolean drawCenter)
{
	fDrawCenter = drawCenter;
}

Boolean BorderImage::GetDrawCenter() const
{
	 return fDrawCenter;
}


void BorderImage::SetDrawStretched(Boolean drawStretched)
{
	fDrawStretched = drawStretched;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean BorderImage::GetDrawStretched() const
{
	 return fDrawStretched;
}
#endif

// =============================================================================



GIFImage::~GIFImage()
{
	Reset();
	if (fGIF)
		delete(fGIF);
}

void GIFImage::Reset(Boolean nukeBitmap)
{
	DeleteStream();
	if ( nukeBitmap )
		DeleteBitMap();
	else if ( fGIF )
		fGIF->Rewind();
}

void GIFImage::DrawUpdate()
{
	if (fGIF != nil) {
		Rectangle drewRectangle;
		Rectangle *srcRect = nil;

		if ( fSourceBounds.right != fSourceBounds.left ) {
			fGIF->SetBounds(&fSourceBounds);
			srcRect = &fSourceBounds;
		}
		if ( fKeepBitMap && fBitMap ) {
			Rectangle refreshFromBitmapClip;
	
			refreshFromBitmapClip = fValidBitmapBounds;
			MapRectangle(refreshFromBitmapClip);
			IntersectRectangle(&refreshFromBitmapClip,&fClipRectangle);
			UnionRectangle(&fDrewRectangle,&refreshFromBitmapClip);
			if ( !EmptyRectangle(&refreshFromBitmapClip) )
				fGIF->Draw(&fDrawRectangle, &refreshFromBitmapClip, fTransparency, &drewRectangle);
		}
		else {
			fGIF->Draw(&fDrawRectangle, &fClipRectangle, fTransparency, &drewRectangle);
			UnionRectangle(&fDrewRectangle,&drewRectangle);
		}
	}
}


void GIFImage::SetFlip(Boolean shouldFlipH,Boolean shouldFlipV)
{
	fFlipHorizontal = shouldFlipH;
	fFlipVertical = shouldFlipV;
	if ( fGIF )
		fGIF->SetFlip(fFlipHorizontal,fFlipVertical);
}



Boolean GIFImage::PushData(Boolean justHeader)
{
	// Push the data that has not been used so far. Returns whether any data
	// was consumed.
	
	long pending;
	long used,totalUsed = 0;
	Rectangle updatedRectangle = {0,0,0,0};


	if ( !fDataTypeValid ) 
		return false;

	if ( fKeepBitMap && fBitMap && fDrawingComplete )
		return false;

	switch ( PrepData() ) {
		case kPrepDataError:
			fError = true;
			return false;
		case kPrepDataReset:
			return false;
		case kPrepDataResetReady:
			fState = kGIFCreateStream;
			break;
		case kPrepDataReady:
			break;
	}
		
		
	switch (fState) {
				
		case kGIFCreateStream:
			fError = false;
			fState = kGIFReadStream;
				
			// fall thru to kGIFReadStream
				
		case kGIFReadStream:
		
			if (fGIF == nil) {
					fGIF = new(GIF);
				if ( IsError(fGIF == nil ) ) {
					Complain(("gif alloc failed"));
					Reset();
					fError = true;
					fState = kGIFDone;
					goto done;
				}
			}
				fGIF->SetDrawRectangle(&fDrawRectangle);
				fGIF->SetClipRectangle(&fClipRectangle);
				fGIF->SetTransparency(fTransparency);
			fGIF->SetIsBackground(fIsBackground);
			fGIF->SetKeepBitMap(fKeepBitMap);
			fGIF->SetFlip(fFlipHorizontal,fFlipVertical);
		fGIF->SetFilterType(fFilterType);
			fGIF->SetGamma(fGamma,fBlackLevel,fWhiteLevel);
			if  ( fSourceBounds.right != fSourceBounds.left )
				fGIF->SetBounds(&fSourceBounds);
				
			
			for (; (pending = fStream->GetPending()) > 0; pending = fStream->GetPending()) {
			
				Rectangle drewRectangle;
				used = fGIF->Write((Byte*)fStream->ReadNext(0), pending,fStream->GetStatus() == kComplete,
							  justHeader, &drewRectangle);
				UnionRectangle(&updatedRectangle,&drewRectangle);
					
					
				TCPIdle(false);
				
				// Check whether there was enough data available.
				if (used == 0)
					break;
				
				// Check for bad data.
				
				if (used < 0)  {
						// If we reach an end of image marker, then consume the rest
					if (used == -1) {
						if ( fGIF->GetLoopCount() ) {
							fStream->Rewind();
						}
						else {
							used = fStream->GetPending();
							totalUsed += used;
							fStream->ReadNext(used);
							if ( fKeepBitMap )
								fBitMap = fGIF->GetBitMap();
							if ( fBitMap ) 
								UnionRectangle(&fValidBitmapBounds,&fBitMap->bounds);
							else
								UnionRectangle(&fDrewRectangle,&updatedRectangle);
							fState = kGIFDone;
							fDrawingComplete = true;
						}
					}
					else if ( IsWarning(true) ) {
						if (used != kLowMemory)
							fStream->SetStatus(kAborted);
						Reset();
						fError = true;
						fState = kGIFDone;
					}
					goto done;
				}
					
				// Note how much data was used.
				fStream->ReadNext(used);
				totalUsed += used;
				
				
				if (justHeader)	{				// just want to get the size parsed
					fGIF->GetBounds(&fBounds);
					if ( !EmptyRectangle(&fBounds) ) {
						goto done;
					}
				}

			}
			if ( fStream->GetPending() <= 0 && fStream->GetStatus() == kComplete ) {
				if (  fGIF->GetLoopCount() )
					fStream->Rewind();
				else {
					fState = kGIFDone;
					fDrawingComplete = true;
					}
			}
			if ( fKeepBitMap )
				fBitMap = fGIF->GetBitMap();
			if ( fBitMap ) 
				UnionRectangle(&fValidBitmapBounds,&updatedRectangle);
			else
				UnionRectangle(&fDrewRectangle,&updatedRectangle);
			break;
		case kGIFDone:
			break;
		}
done:
	return totalUsed > 0;
}


void GIFImage::DeleteBitMap()
{
	if ( fGIF ) {
		delete(fGIF);
		fGIF = nil;
	}
	fBitMap = nil;			// gifs bitmap is in fGIF so it is gone now
	SetRectangle(fValidBitmapBounds,0,0,0,0);
}


void GIFImage::SetBounds(const Rectangle* bounds)
{
	fSourceBounds = *bounds;
	if (fGIF )
		fGIF->SetBounds(bounds);
}

	
// =============================================================================


JPEGImage::~JPEGImage()
{
	Reset();
	if (fJPEG != nil)
		DisposeJPEGDecoder(fJPEG);
}


void JPEGImage::DeleteJPEGDecoder()
{
	if (fJPEG != nil) {
		DisposeJPEGDecoder(fJPEG);
		fJPEG = nil;
	}
}


Boolean JPEGImage::PushData(Boolean justHeader)
{
	long pending,used = 0;

	if ( !fDataTypeValid ) 
		return false;

	switch ( PrepData() ) {
	case kPrepDataError:
		fError = true;
	return false;
	case kPrepDataReset:
		return false;
	case kPrepDataResetReady:
		fState = kJPEGCreateStream;
		break;
	case kPrepDataReady:
		break;
	}
	
	switch (fState) {
		case kJPEGCreateStream:
			fError = false;
			fState = kJPEGStarted;
			break;
		case kJPEGStarted:
			break;
		case kJPEGDone:	
			break;
	}


	

	if ( justHeader ) {
		if ( fBounds.left != fBounds.right )
			return false;

		long len = fStream->GetDataLength();

		if ( len == 0 )
			return false;


		Error e = JPEGBounds((Byte*)fStream->GetData(), len, &fBounds);
		
		if ( e == kCannotParse )		// need more data 
			return false;
	
		if ( IsWarning(e != kNoError)  )	{	// bad jpeg data
			Reset();
			fError = true;
			fState = kJPEGDone;
		}
		return true;
	}

	// See if we should abort

	if (fState != kJPEGStarted )
		return false;

	if (fJPEG == nil) 
	{

//fKeepBitMap = kCompressedBitMap;	// ¥¥¥Êvq test hack

		if ( fKeepBitMap != kNoBitMap && fBitMap == nil ) 
		{
			if ( JPEGBounds((Byte*)fStream->GetData(), fStream->GetDataLength(), &fBounds) != 0 )
				return false;
			fBitMap = NewBitMapDevice(fBounds, yuv422Format, nil, kNoTransparentColor);
			if ( fBitMap ) 
				fBitMap->filter = fFilterType;
			else {
				Message(("Drawing JPEG image without bitmap (low memory)"));
				if ( EmptyRectangle(&fDrawRectangle) ) {
					Reset();
					fDrawingComplete = true;
					fError = true;
					fState = kJPEGDone;
					return false;
				}
			}
		} 
		if ( fBitMap )
			fJPEG = NewJPEGDecoder(&fBitMap->bounds, 0, DrawSingleJPEGRow, nil, 0, fBitMap );
		else
			fJPEG = NewJPEGDecoder(&fDrawRectangle, 0, DrawSingleJPEGRow, &fClipRectangle, fTransparency, nil );
		if ( IsWarning(fJPEG == nil) ) {
			Reset();
			fError = true;
			fState = kJPEGDone;
			fDrawingComplete = true;
			return false;
		}
		if ( fGamma )
			SetJPEGGamma(fJPEG,fGamma,fBlackLevel,fWhiteLevel);
	}

	//Message(("JPEG PROGRESSIVE width=%d, height=%d at %d, %d", r->right - r->left, r->bottom - r->top, r->left, r->top));
	
	for (; (pending = fStream->GetPending()) > 0; pending = fStream->GetPending() ) {
		PostulateFinal(false);		// priority needs to be checked
		fJPEG->priority = GetPriority();
		Rectangle drewRectangle;
		used = JPEGWrite(fJPEG, (Byte*)fStream->ReadNext(0), pending, &drewRectangle);
		
		// doesn't draw to screen when there's a bitmap
		
		if ( fBitMap ) 
			UnionRectangle(&fValidBitmapBounds,&drewRectangle);
		else
			UnionRectangle(&fDrewRectangle,&drewRectangle);
			
		TCPIdle(false);
			
		// if we are deadlocked, then bail for now 
		// for progressive images with multiple passes, reset the decoder max scan variable
		// so that it displays the full image for the amount of data we have so far.
		
		if ( used == 0 && pending == fStream->GetPending() )  {
			if ( fJPEG->anotherPass && fJPEG->scansProcessed > fJPEG->maxScansToDisplay ) {
				fStream->Rewind();
				SetMaxScansToDisplay(fJPEG,fJPEG->scansProcessed);
			}	
			break;
		}
		
		// completed the image ( good or bad )
		
		if (used < 0)  {
			if ( used == kJPEGCompletedPass )  {
				if ( fJPEG->anotherPass )  {
					fStream->Rewind();
					//Message(("projpeg end of pass - needs another"));
				} else  {
					Reset(fKeepBitMap == kNoBitMap);
					fState = kJPEGDone;
					fDrawingComplete = true;
					if ( fBitMap && (fKeepBitMap != kNoBitMap) ) {
					
						// this should be done incrementally as the image is read in
						// or converserly, we could run an idle task which checks for bitmaps
						// hanging out in memory and compresses them while nothing else is
						// happening
						
						if ( (fKeepBitMap == kCompressedBitMap )  ) {
							long pixCount = (fBounds.bottom - fBounds.top) * (fBounds.right - fBounds.left);
							if ( pixCount <= 512*512 )  {

//			long t = TickCount();
			//		GVVectorQuantizer *vq = new(GVVectorQuantizer);
								VectorQuantizer *vq = new(VectorQuantizer);
								
								
								if ( vq )  {
									short iterations = 6;
									short codeBookSize = 256;
									if ( pixCount < 256*64 )
										codeBookSize = 128;
									if ( pixCount < 256*32 )
										codeBookSize = 64;
									
									vq->Start(fBitMap,nil,nil,codeBookSize,iterations);	
									
									while ( !vq->Continue(false) )
										TCPIdle(false);
																			
//			Message(("vq compress took %d ticks",TickCount()-t));

									
									BitMapDevice *compressedBM = vq->GetBitMap(true);		// we own it now
									if ( compressedBM ) 	{
										DeleteBitMap();
										fBitMap = compressedBM;
									}
									delete(vq);
								}
							}
						}
					}
					fDrawingComplete = true;
				}
			} 
			else if ( IsWarning(used < 0) )  {
				if (used != kLowMemory)
					fStream->SetStatus(kAborted);
				Reset();
				fDrawingComplete = true;
				fError = true;
				fState = kJPEGDone;
			}
			break;
		} else
			fStream->ReadNext(used);
	}
	return used > 0;
}
			
void JPEGImage::Reset(Boolean nukeBitmap)
{
	DeleteJPEGDecoder();
	DeleteStream();
	if ( nukeBitmap )
		DeleteBitMap();
}



// =============================================================================




long XBitMapImage::ParseDimension(const char *buf,long len,Boolean widthNotHeight)
{
	int	 i;
	const char *bp = buf;
	long v = 0;
	char	nb[10];
	long	startLen = len;
	const char	*defstr = widthNotHeight ? "width" : "height";
	
	while ( len > 0 )  {
		len--;
		if ( *bp++ == '#' )
			break;
	}
	while ( len > 0 )  {
		if ( !isspace(*bp) )
			break;
		len--;
		bp++;
	}
	if ( len < 6 ) 
		return 0;
	if ( IsWarning(strncmp(bp,"define",6) != 0 ) ) {
		Message(("Wierd pragma in x-bitmap '%s'",GetName()));
		return -1;
	}
	bp += 6;
	len -= 6;
	while ( len > 0 )  {
		if ( !isspace(*bp) )
			break;
		len--;
		bp++;
	}
	
	while ( 1 )  {
		if ( IsWarning(isspace(*bp)) ) {
			Message(("bad line in x-bitmap '%s'",GetName()));
			return -1;
		}
		if ( len-- == 0 )
			return 0;
		if ( *bp++ == '_' )
			break;
	}
	i = strlen(defstr);
	if ( len < i )
		return 0;
	if ( strncmp(bp,defstr,i) != 0 ) 
		return 0;
	bp += i;
	len -= i;
	while ( len > 0 )  {
		if ( !isspace(*bp) )
			break;
		len--;
		bp++;
	}
	i = 0;
	while ( 1 )  {
		if ( len == 0 )
			return 0;
		if ( isspace(*bp) ) {
			break;
		}
		nb[i++] = *bp++;
		len--;
		if ( IsWarning(i == 10) ) {
			Message(("too many digits x-bitmap '%s'",GetName()));
			return -1;
		}
	}
	nb[i] = 0;
	v = strtol(nb,0,10);
	if ( IsWarning(i == 0 || v == 0)  ) {
		Message(("error parsing x-bitmap '%s'",GetName()));
		return -1;
	}
	if ( widthNotHeight )
		fBounds.right = v;
	else
		fBounds.bottom = v;
	return startLen - len;
}


const	kXWhite = 15;
const 	kXBlack = 0;


long XBitMapImage::DecodeXBM(const char *bp,long len)
{
	long	startLen = len;
	int 	i,v,l;
	long	 width,height;
	unsigned char	*rbp,*baseAddr;
	char	c,nb[8];
	uchar	pix;
	

	baseAddr = fBitMap->baseAddress + fCurY * fBitMap->rowBytes;	// 4 bits/pixel
	width = fBounds.right - fBounds.left;
	height = fBounds.bottom - fBounds.top;
	
	while ( len > 0 ) {
		rbp = baseAddr + (fCurX>>1);
		while (  fCurX < width  ) {
			while ( len > 0 ) {	
				if ( !isspace(*bp) )
					break;
				bp++;
				len--;
			}
			if ( len == 0 )
				goto done;
			l = len;						// don't stop in the middle of parsing a byte
			for ( i =0; i < 6; ) {
				if ( l-- == 0 )
					goto done;
				c = nb[i++] = bp[i];
				if ( c == ',' || c == '}' || isspace(c) )
					break;
			}
			if ( IsWarning(i==6 || i == 0 )  ) {
				Message(("Bad parse x-bitmap data at %ld,%ld",fCurX,fCurY));
				return -1;
			}
			len -= i;
			bp += i;
			nb[i] = 0;
			
			// this can be either hex (with leading 0x) or decimal ( or octal ? ) sscanf should do the right thing
			
			if ( IsWarning(sscanf(nb,"%i",&v) != 1) ) {			
				Message(("Failed to parse x-bitmap data at %ld,%ld '%s'",fCurX,fCurY,nb));
				return -1;
			}
			
			for ( i=8; fCurX < width && i > 0 ; i -= 2 ) {
				if  (fCurX++ < width )  {
					pix = (v & 1) == 0 ? kXWhite : kXBlack;
					v >>= 1;
					pix <<= 4;
					if  (fCurX++ < width )  {
						pix |= (v & 1) == 0 ? kXWhite : kXBlack;
						v >>= 1;
					}
					*rbp++ = pix;
				}
			}
		}
		if ( fCurX >= width )  {
			fCurX = 0;
			if ( ++fCurY == height ) {
				fState = kXBMDone;
				return startLen;		// suck up all remaining data
			}
			baseAddr += fBitMap->rowBytes;
		}
	}
done:
	return startLen - len;
}


void XBitMapImage::Reset(Boolean nukeBitmap )
{
	fCurX = 0;
	fCurY = 0;
	DeleteStream();
	if ( nukeBitmap )
		DeleteBitMap();
}

Boolean XBitMapImage::PushData(Boolean sizeOnly)
{
	long		used = 0,pending;
		
	if ( !fDataTypeValid ) 
		return false;
	if ( fKeepBitMap && fBitMap && fState == kXBMDone )
		return false;
		
	switch ( PrepData() ) {
		case kPrepDataError:
			fState = kXBMDone;
			fError = true;
			return false;
		case kPrepDataReset:
			return false;
		case kPrepDataResetReady:
			fState = kXBMCreateStream;
			break;
		case kPrepDataReady:
			break;
	}
	
	switch (fState)
	{
		case kXBMCreateStream:
			fState = kXBMReadHeader;
			fError = false;
			
		case kXBMReadHeader:

			while ( fBounds.right == 0 || fBounds.bottom == 0 ) {
			
				pending = fStream->GetPending();
				if ( pending == 0 && (fStream == nil || fStream->GetStatus() != kPending) ) {
					Message(("lost getting xbm bounds '%s'",GetName()));
					Reset();
					fError = true;
					return false;
				}
				if ( pending < 6 ) {
					if ( IsWarning(fStream->GetStatus() == kComplete) ) {
						Message(("error parsing xbm image '%s'",GetName()));
						Reset();
						fError = true;
					}
					return false;
				}
				used = ParseDimension(fStream->ReadNext(0),pending,fBounds.right == 0);
				if ( IsWarning(used < 0) ) {
					Message(("error parsing xbm image '%s'",GetName()));
					Reset();
					fError = true;
					return false;
				} else  {
					if ( used )
						fStream->ReadNext(used);
					else if ( IsWarning(fStream->GetStatus() == kComplete) ) {
						Message(("error parsing xbm image '%s'",GetName()));
						Reset();
						fError = true;
						return false;
					}
				}
			}
			if ( sizeOnly ) 
				return true;
				
			while ( fStream->GetPending() > 0 ) {
				if (  *fStream->ReadNext(0) == '{' ) 	{
					CLUT *clut;
					Color transparentColor = kTransparent + kXWhite;
					
					fStream->ReadNext(1);
					fState = kXBMReadBits;
					fCurX = 0;
					fCurY = 0;
					DeleteBitMap();
					
					clut = NewColorTable(kRGB24,16);
					if ( clut ) {
						uchar r,g,b;
						Color bcolor = kWhiteColor;
						if (gPageViewer != nil) {
							Document *doc = gPageViewer->GetDocument();
							if ( doc ) {
								bcolor = gPageViewer->GetDocument()->GetBackgroundColor();
							}
						}
						
						clut->data[kXBlack*3] = 0;
						clut->data[kXBlack*3+1] = 0;
						clut->data[kXBlack*3+2] = 0;
						
						// if the background is too dark default to white so it is at least visible
						
						Color lum = bcolor;
						MapColor(gray8Format,lum);
						if ( (lum & 0xff) < 40 ) {
							transparentColor = kNoTransparentColor;
							clut->data[kXWhite*3] = 0xff;
							clut->data[kXWhite*3+1] = 0xff;
							clut->data[kXWhite*3+2] = 0xff;
						}
						else {
							r = (bcolor>>16) & 0xff;
							g = (bcolor>>8) & 0xff;
							b = (bcolor) & 0xff;
							clut->data[kXWhite*3] = r;
							clut->data[kXWhite*3+1] = g;
							clut->data[kXWhite*3+2] = b;
						}
					}
					fBitMap = NewBitMapDevice(fBounds, index4Format, 0, transparentColor);
					if ( IsWarning(fBitMap == nil) ) {
						DeleteColorTable(clut);
						Message(("failed to allocate bitmap for xbm image"));
						Reset();
						fError = true;
						return true;
					}
					fBitMap->colorTable = clut;
					fBitMap->filter = fFilterType;
					SetRectangle(fValidBitmapBounds,0,0,0,0);
					break;
				}
				fStream->ReadNext(1);
				
			}
			if ( IsWarning(fStream->GetStatus() == kComplete && fStream->GetPending() == 0 ) ) {
				Message(("error parsing xbm image '%s'",GetName()));
				Reset();
				fError = true;
				return true;
			}
			

		case kXBMReadBits:
		
			{
				pending = fStream->GetPending();
				used = DecodeXBM(fStream->ReadNext(0),pending);
				if ( IsWarning(used < 0) ) {
					Message(("error decoding xbm image '%s'",GetName()));
					Reset();
					fError = true;
					return true;
				} else if ( used ) {
					long height = fCurY;
					if ( height > 0 && fCurX != 0 )
						height--;
					SetRectangle(fValidBitmapBounds,fBounds.left,0,fBounds.right,height);
					fStream->ReadNext(used);
				}
				if ( fState != kXBMDone )
					break;
			}
			
		case kXBMDone:
			fDrawingComplete = true;
	}
	return used > 0;
}

	

// =============================================================================


void FidoImage::Reset(Boolean nukeBitmap )
{
	DeleteStream();
	if ( nukeBitmap )
		DeleteBitMap();
	ZeroMemory(&fHeader,sizeof(fHeader));
	fCodebookSize = 0;
	fDataSize = 0;
	fBandHeight = 0;
	fBandStart = 0;
	fDstBandStart = 0;
}


Boolean FidoImage::PushData(Boolean sizeOnly)
{
	long		pending;
	Rectangle 	srcBandRect,dstBandRect;
	
	if ( !fDataTypeValid ) 
		return false;
	switch ( PrepData() ) {
	case kPrepDataError:
		fState = kFidoDone;
		fError = true;
		return false;
	case kPrepDataReset:
		return false;
	case kPrepDataResetReady:
		fState = kFidoCreateStream;
		break;
	case kPrepDataReady:
		break;
	}
	
	switch (fState) {
		case kFidoCreateStream:
			fError = false;
			fState = kFidoReadHeader;

		case kFidoReadHeader:

			{
				long headerSize = 4 + 2 + 2 + 2 + 2;
				
				pending = fStream->GetPending();
			
				if ( pending < headerSize+4 ) {
				return false;
				}
				fHeader.fSignature = *(ulong  *)fStream->ReadNext(4);
				fHeader.fType = 	 (FidoImageType)*(ushort *)fStream->ReadNext(2);
				fHeader.fVersion = 	 *(ushort *)fStream->ReadNext(2);
				fHeader.fWidth =     *(ushort *)fStream->ReadNext(2);
				fHeader.fHeight =    *(ushort *)fStream->ReadNext(2);

				fState = kFidoReadCodeBook;
			
				if ( IsWarning(fHeader.fSignature != 0x4649444F || fHeader.fVersion != 0) ) {
#ifdef	DEBUG
					Message(("bad fido image '%s' sig %lx vers %x",GetName(),fHeader.fSignature,fHeader.fVersion));
#endif
					Reset();
					fError = true;
					return true;
				}
				
				if ( fBounds.right == fBounds.left )
					SetRectangle(fBounds,0,0,fHeader.fWidth,fHeader.fHeight);
				fState = kFidoReadBand;
				fBandStart = 0;
				fDstBandStart = 0;
				if ( sizeOnly )
					return true;
			}
			
		case kFidoReadBand:
fidoReadNextBand:
			if ( sizeOnly )
				return false;
			pending = fStream->GetPending();
			if ( pending < 4 )
				return false;
			fBandHeight = *(uchar *)fStream->ReadNext(1);
			fBandHeight <<= 8;
			fBandHeight |= *(uchar *)fStream->ReadNext(1);
			if (  fBandHeight == 0 ) {
				fState = kFidoDone;
				return true;
			}
			fCodebookSize = *(uchar *)fStream->ReadNext(1);
			fCodebookSize <<= 8;
			fCodebookSize |= *(uchar *)fStream->ReadNext(1);
			fState = kFidoReadCodeBook;

		case kFidoReadCodeBook:
			if ( sizeOnly )
				return false;
			pending = fStream->GetPending();
			if ( pending < (fCodebookSize+4) )
				return false;

			DeleteBitMap();
			SetRectangle(srcBandRect,0,0,fHeader.fWidth,fBandHeight*2);
			fBitMap = NewBitMapDevice(srcBandRect, vqFormat, 0, kNoTransparentColor);
			if ( IsWarning(fBitMap == nil) ) {
				Message(("failed to allocate bitmap for fido image"));
				Reset();
				fError = true;
				return true;
			}
			fBitMap->colorTable = (CLUT *)AllocateTaggedMemory(4 + fCodebookSize,"Color Table");
			if ( IsWarning(fBitMap->colorTable == nil) ) {
				Reset();
				fError = true;
				return true;
			}
			fBitMap->filter = fFilterType;
			if ( (fBandStart+fBandHeight) < fHeader.fHeight ) {
				if ( fBandStart == 0 ) {
					if ( fFilterType )
						fBitMap->filter = (FilterType)((fFilterType & ~0xf) | kTopFilter);
				} else { 
					if ( fFilterType )
						fBitMap->filter = (FilterType)((fFilterType & ~0xf) | kSliceFilter);
				}
			} else {
				if ( fBandStart == 0 ) {
					if ( fFilterType )
						fBitMap->filter = (FilterType)((fFilterType & ~0xf) | kFullFilter);
				} else { 
					if ( fFilterType )
						fBitMap->filter = (FilterType)((fFilterType & ~0xf) | kBottomFilter);
				}
			}
			fBitMap->colorTable->version = kYUV64;
			fBitMap->colorTable->size = fCodebookSize;
			CopyMemory(fStream->ReadNext(fCodebookSize),&fBitMap->colorTable->data,fCodebookSize);
			if ( fGamma )
				GammaCorrect(fBitMap->colorTable, fGamma, fBlackLevel, fWhiteLevel);
			fDataSize = *(uchar *)fStream->ReadNext(1);
			fDataSize <<= 8;
			fDataSize |= *(uchar *)fStream->ReadNext(1);
			fDataSize <<= 8;
			fDataSize |= *(uchar *)fStream->ReadNext(1);
			fDataSize <<= 8;
			fDataSize |= *(uchar *)fStream->ReadNext(1);
			fState = kFidoReadData;

		case kFidoReadData:
			{
				if ( sizeOnly )
					return false;
				pending = fStream->GetPending();
				if ( pending < fDataSize ) 				// ¥¥¥ should draw partial data if we have it
					return false;
				long used = 0;
				long rowSize = (fHeader.fWidth + 1)/2;
	
				uchar *dp = (uchar *)fBitMap->baseAddress;
				for ( long i=0; i < fBandHeight; i++ ) {
					CopyMemory(fStream->ReadNext(rowSize),dp,rowSize);
					used += rowSize;
					dp += fBitMap->rowBytes;
				}
				fStream->ReadNext(fDataSize-used);
				
				SetRectangle(srcBandRect,0,fBandStart*2,fHeader.fWidth, fBandStart*2 + fBandHeight*2);
				Rectangle srcBounds;
				if ( fSourceBounds.right != fSourceBounds.left )
					srcBounds = fSourceBounds;
				else
					srcBounds = fBounds;
				IntersectRectangle(&srcBandRect,&srcBounds);
				if ( !EmptyRectangle(&srcBandRect) ) {
					srcBandRect.top -= fBandStart*2;
					srcBandRect.bottom -= fBandStart*2;
					SetRectangle(dstBandRect,fDrawRectangle.left,fDrawRectangle.top + fDstBandStart,
						fDrawRectangle.right,fDrawRectangle.top + fDstBandStart + (srcBandRect.bottom-srcBandRect.top));
					fDstBandStart += (srcBandRect.bottom-srcBandRect.top);
					Rectangle r = fDrawRectangle;
					IntersectRectangle(&r,&fClipRectangle);
					if ( fInDrawIdle )
					DrawImage(gScreenDevice, *fBitMap, dstBandRect, fTransparency, &fClipRectangle,&srcBandRect,fFlipHorizontal,fFlipVertical);
					UnionRectangle(&fDrewRectangle,&r);
					UnionRectangle(&fValidBitmapBounds,&srcBandRect);
					fBitMap->filter = fFilterType;
				}
				DeleteBitMap();
				pending = fStream->GetPending();
				if ( fStream->GetStatus() != kComplete || pending > 4 ) {
					fBandStart += fBandHeight;
					fState = kFidoReadBand;
					TCPIdle(false);
					goto fidoReadNextBand;
				}
				fState = kFidoDone;
			}
		case kFidoDone:
			fDrawingComplete = true;
			return true;
	}
	return true;
}


// =============================================================================

