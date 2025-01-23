// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#include "Headers.h"
#include "ImageData.h"
#include "GIF.h"
#include "Layer.h"

#if defined DEBUG && defined FOR_MAC && defined FIDO_INTERCEPT
#include "FidoCompatibility.h"

void AnimationImage::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency, FidoCompatibilityState& fidoCompatibility)
{
	if (fCurrentIndex < fImageList.GetCount())
	{
		ImageData* imagedata = (ImageData*)fImageList.At(fCurrentIndex);
		Assert(imagedata != nil);
		imagedata->Draw(r, invalid, transparency, fidoCompatibility);
	}
}

Boolean AnimationImage::DrawingIdle(Rectangle* r, FidoCompatibilityState& fidoCompatibility)
{
	if (fCurrentIndex >= fImageList.GetCount())
		return false;
	ImageData* imagedata = (ImageData*)fImageList.At(fCurrentIndex);
	Assert(imagedata != nil);
	return imagedata->DrawingIdle(r, fidoCompatibility);
}

// =============================================================================

void CelImage::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency, FidoCompatibilityState& fidoCompatibility)
{
	if (fState != kCelDone)
		return;
	Assert(fImageData != nil);
	fImageData->Draw(r, invalid, transparency, fidoCompatibility);
}


Boolean CelImage::DrawingIdle(Rectangle* r, FidoCompatibilityState& fidoCompatibility)
{
	if (fState != kCelDone)
		return false;	
	Assert(fImageData != nil);
	return fImageData->DrawingIdle(r, fidoCompatibility);
}

// =============================================================================

static long CLUTSize(short format)
{
	switch (format)
	{
		case index1Format:	return 3 * 2   + 4;
		case index2Format:	return 3 * 4   + 4;
		case index4Format:	return 3 * 16  + 4;
		case index8Format:	return 3 * 256 + 4; /*offsetof(CLUT,data)*/
		default:			break;
	}
	
	return 0;
}

void BitMapImage::Draw(Rectangle* r, const Rectangle* , ulong transparency, FidoCompatibilityState& fidoCompatibility)
{
	if (fState != kBitMapDone)
		return;
	fidoCompatibility.DrawImage(gScreenDevice, *fBitMap, *r, transparency, nil, nil);
}

// =============================================================================

void BorderImage::Draw(Rectangle* r, const Rectangle* , ulong transparency, FidoCompatibilityState& fidoCompatibility)
{
	if (fGIFFido == nil)
		return;		
	BitMapDevice* bitMap = fGIFFido->GetBitMap();
	if (bitMap == nil)
		return;	
	Assert(r->top > -30000 && r->bottom < 30000 && r->left > -30000 && r->right < 30000);
	Assert(r->bottom > r->top && r->right > r->left);
	fidoCompatibility.DrawBorderImage(gScreenDevice, *bitMap, *r, fInnerBounds, transparency, 0);
}

void BorderImage::Draw(const Region* region, const Rectangle* invalid, ulong transparency, FidoCompatibilityState& fidoCompatibility)
{
	if (region->IsRectangle())
	{
		Rectangle r = region->GetBounds();
		Assert(r.top > -30000 && r.bottom < 30000 && r.left > -30000 && r.right < 30000);
		::InsetRectangle(r, -fInnerBounds.left-1, -fInnerBounds.top+1);
		Draw(&r, invalid, transparency, fidoCompatibility);
		return;
	}

	Assert(region != nil);
	if (region->IsEmpty())
		return;
	
	if (fGIFFido == nil)
		return;
		
	BitMapDevice* bitMap = fGIFFido->GetBitMap();
	if (bitMap == nil)
		return;
	
	DataIterator* iterator = region->NewIterator();
	Rectangle* r = (Rectangle*)iterator->GetFirst();
	Assert(r != nil);

	for (; r; r = (Rectangle*)iterator->GetNext())
	{
		Assert(r->top > -30000 && r->bottom < 30000 && r->left > -30000 && r->right < 30000);
		::InsetRectangle(*r, -fInnerBounds.left-1, -fInnerBounds.top+1);
		fidoCompatibility.DrawBorderImage(gScreenDevice, *bitMap, *r, fInnerBounds, transparency, 0);
	}
	
	delete(iterator);
}

// =============================================================================

void GIFImage::DeleteGIF(FidoCompatibilityState&)
{
	if (fGIFFido == nil)
		return;
	
	delete(fGIFFido);
	fGIFFido = nil;
}

void GIFImage::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency, FidoCompatibilityState& fidoCompatibility)
{
	PushGIFData(false, fidoCompatibility);
	
#ifdef KEEP_STREAMS_NOT_PIXELS
	if (fStreamFido)
	{
		fStreamFido->Rewind();
		fDrawingCompleteFido = false;
	}
	DrawingIdle(r, fidoCompatibility);
#else

	if (fGIFFido != nil)
		fGIFFido->Draw(r, invalid, transparency, fidoCompatibility);
	fDrawingCompleteFido = (fStateFido >= kGIFDone);
#endif
}

Boolean GIFImage::DrawingIdle(Rectangle* r, FidoCompatibilityState& fidoCompatibility)
{
	// See if we moved
	if (fDrawRectangle.top != r->top || fDrawRectangle.left != r->left)
	{
		fDrawingCompleteFido = true;
		return false;
	}
	
	// If an error occurred, invalidate and stop drawing
	if (!fDrawingCompleteFido && fStateFido == kGIFError)
	{
		fDrawingCompleteFido = true;
		return true;
	}

	if (!PushGIFData(false, fidoCompatibility))
		return false;
	
	fGIFFido->Draw(&fDrawRectangle, &fClipRectangle, fTransparency, fidoCompatibility);
	return true;
}

Boolean GIFImage::PushGIFData(Boolean once, FidoCompatibilityState& fidoCompatibility)
{
	// Push the data that has not been used so far. Returns whether any data
	// was consumed.
	
	long pending;
	DataStream* stream;

	switch (fStateFido)
	{
		case kGIFCreateStream:
			if ((stream = fResource.NewStream()) == nil) 
				return false;
			SetStream(stream, fidoCompatibility);
			fStateFido = kGIFReadStream;
		
		case kGIFReadStream:
			Error status = fStreamFido->GetStatus();
			
			if (status == kStreamReset)
			{
				Reset(fidoCompatibility);
				fStateFido = kGIFCreateStream;
				return false;
			}
			
			if (TrueError(status))
			{
				DeleteGIF(fidoCompatibility);
				DeleteStream(fidoCompatibility);
				fStateFido = kGIFError;
				return false;
			}
			
			if ((pending = fStreamFido->GetPending()) == 0)
			{
				// Check whether we are finished with the stream.
				if (status == kComplete)
				{
					Assert(fGIFFido);
					Assert(fGIFFido->fPhase == kGlobalImageEnd);
					Assert(fStreamFido->GetPending() == 0);
			#ifndef KEEP_STREAMS_NOT_PIXELS
					DeleteStream(fidoCompatibility);
			#endif
					fDrawingCompleteFido = true;
					fStateFido = kGIFDone;
				}
				return false;
			}
	
			if (fGIFFido == nil)
				fGIFFido = new(GIF);
		
			for (; pending != 0; pending = fStreamFido->GetPending())
			{
				long used = fGIFFido->Write((Byte*)fStreamFido->ReadNext(0), pending);
				
				// Check whether there was enough data available.
				if (used == 0)
					break;
				
				// Check for bad data.
				if (used < 0) 
				{
					// If we reach an end of image marker, then consume the rest
					if (used == -1)
						used = fStreamFido->GetPending();
					else
					{
						fStreamFido->SetStatus(kAborted);
						DeleteGIF(fidoCompatibility);
						DeleteStream(fidoCompatibility);
						fStateFido = kGIFError;
						return true;
					}
				}
				
				// Note how much data was used.
				fStreamFido->ReadNext(used);
				if (once)					// just want to get the size parsed
					break;
			}
			break;
	}

	return true;
}

void GIFImage::Reset(FidoCompatibilityState& fidoCompatibility)
{
	DeleteGIF(fidoCompatibility);
	DeleteStream(fidoCompatibility);
}

// =============================================================================

void ImageData::DeleteStream(FidoCompatibilityState&)
{
	if (fStreamFido == nil)
		return;
	delete(fStreamFido);
	fStreamFido = nil;
}

void ImageData::Draw(Rectangle*, const Rectangle* , ulong , FidoCompatibilityState& )
{
	Trespass();
}

Boolean ImageData::DrawingIdle(Rectangle*, FidoCompatibilityState& )
{
	return false;
}

Boolean ImageData::IsDrawingComplete(FidoCompatibilityState& )
{
	return fDrawingCompleteFido;
}

void ImageData::SetStream(DataStream* stream, FidoCompatibilityState&)
{
	Assert(stream != nil && fStreamFido == nil);
	fStreamFido = stream;
}

// =============================================================================

extern Error DrawSingleRow(void* j);

Boolean JPEGImage::DrawingIdle(Rectangle* r, FidoCompatibilityState& fidoCompatibility)
{
	PushData(fidoCompatibility);
	
	// See if we should abort
	if (fStateFido != kJPEGDone)
		return false;
		
	if (fJPEGFido->fDrawRect.top != r->top || fJPEGFido->fDrawRect.left != r->left)
	{
		fDrawingCompleteFido = true;
		return false;
	}

	if (fJPEGFido == nil)
		fJPEGFido = NewJPEGDecoder(r, 0, DrawSingleRow, nil, 0);

//Message(("JPEG PROGRESSIVE width=%d, height=%d at %d, %d", r->right - r->left, r->bottom - r->top, r->left, r->top));

	long pending;
	long used;
		
	for (pending = fStreamFido->GetPending(); pending != 0; pending -= used)
	{
		if ((used = JPEGWrite(fJPEGFido, (Byte*)fStreamFido->ReadNext(0), pending)) <= 0) {
			if ( used == 0 || used == -42 ) {
				if ( fJPEGFido->anotherPass ) {
//					fStreamFido->SetStatus(kPending);	// еее Dave - Rewind is all we need here. This confuses status
													// on whether all stream data has been received.
					fStreamFido->Rewind();
//					Message(("JPEG draw complete do another pass for progressive rewind %d",fStreamFido->GetPosition() ));
				}
			}
			break;
		}
		fStreamFido->ReadNext(used);
	}

	// Check whether we are finished with the stream.
	if (fStreamFido->GetStatus() == kComplete)
	{
		if ( !fJPEGFido->anotherPass ) {
			DeleteJPEGDecoder(fidoCompatibility);
			fDrawingCompleteFido = true;
		} else {
//			fStreamFido->SetStatus(kPending);			// еее Dave - see above.
			fStreamFido->Rewind();
		}
	}
	return true;								// actually drew something
}

void JPEGImage::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency, FidoCompatibilityState& fidoCompatibility)
{	
	fidoCompatibility.display.fillColor.setAlpha(transparency);
	PushData(fidoCompatibility);

#if 0	
	if ( !fDrawingCompleteFido ) {			// еее Mark, is this correct? 
		DrawingIdle(r, fidoCompatibility);
		return;
	}
#endif									// еее Dave, I am fairly certain this was not 
										// correct. It resulted in every other draw failing.
											
	if (fStateFido != kJPEGDone)
		return;		

	if (invalid != nil && !RectanglesIntersect(r, invalid))
		return;

	Message(("JPEG DRAW width=%d, height=%d at %d, %d", r->right - r->left, r->bottom - r->top, r->left, r->top));
	if (r->top == r->bottom || r->left == r->right)
		return;
	
	if (fJPEGFido != nil)
		DisposeJPEGDecoder(fJPEGFido);	// dispose anything in progress
		
	fJPEGFido = NewJPEGDecoder(r, 0, DrawSingleRow, invalid, 0);
	fStreamFido->Rewind();
	fDrawingCompleteFido = false;
	DrawingIdle(r, fidoCompatibility);
}


void JPEGImage::DeleteJPEGDecoder(FidoCompatibilityState& )
{
	if (fJPEGFido != nil)
	{
		DisposeJPEGDecoder(fJPEGFido);
		fJPEGFido = nil;
	}
}

void JPEGImage::PushData(FidoCompatibilityState& fidoCompatibility)
{
	DataStream* stream;

	switch (fStateFido)
	{
		case kJPEGCreateStream:
			if ((stream = fResource.NewStream()) == nil) 
				return;
			SetStream(stream, fidoCompatibility);
			fStateFido = kJPEGDone;
		
		case kJPEGDone:
			Error status = fStreamFido->GetStatus();
			
			if (status == kStreamReset)
			{
				Reset(fidoCompatibility);
				fStateFido = kJPEGCreateStream;
				return;
			}
			
			if (TrueError(status))
			{
				DeleteJPEGDecoder(fidoCompatibility);
				DeleteStream(fidoCompatibility);
				fStateFido = kJPEGError;
				fDrawingCompleteFido = true;
				return;
			}
			break;
	}
}
			
void JPEGImage::Reset(FidoCompatibilityState& fidoCompatibility)
{
	DeleteJPEGDecoder(fidoCompatibility);
	fStateFido = kJPEGCreateStream;
	DeleteStream(fidoCompatibility);
}

// =============================================================================

#endif // defined DEBUG && defined FOR_MAC && defined FIDO_INTERCEPT