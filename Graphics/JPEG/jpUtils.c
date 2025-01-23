// ===========================================================================
//	jpMarkers.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __JPINCLUDES_H__
#include "jpIncludes.h"
#endif

//====================================================================


//	Look in the encoded image for its dimensions

Error JPEGBounds(Byte *image, long size, Rectangle* bounds)
{
	long		result,offset = 0;
	JPEGDecoder	*j;

	SetRectangle(*bounds,0,0,0,0);
	
	if (!(j = NewJPEGDecoder(bounds, 0, nil, nil, 0,0)))
		return kLowMemory;
	while ((result = JPEGWrite(j,image + offset,size - offset)) > 0)
	{
		if (j->CompInFrame > 0 )
		{
			SetRectangle(*bounds,0,0,j->Width,j->Height);
			DisposeJPEGDecoder(j);
			return kNoError;			// Leave after Start of Frame
		}
		offset += result;
	}
	DisposeJPEGDecoder(j);
	if ( result < 0 || j->phase > kWaitingForFrame )
		return kGenericError;

	return kCannotParse;				// Not enough data to reach kSOS
}

//	Loop to decode an image
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined

Error JPEGDecodeImage(JPEGDecoder *j, Byte *image, long size)
{
	long		result,offset = 0;
	
	//Message(("JPEG BeginDECODE width=%d, height=%d", j->fDrawRect.right - j->fDrawRect.left, j->fDrawRect.bottom - j->fDrawRect.top));

	// note - for progressive JPEG images in low memory cases we may need to 
	// display the entire image again if all the slices didnt get drawn
	

	while ((result = JPEGWrite(j,image + offset,size - offset)) > 0)
	{
		offset += result;
		if (size <= (offset + 2))
			break;						// Stopped decoding in a paricular phase
	}
	if (result < 0) {
		if ( result == kJPEGCompletedPass )
			result = 0;
		return (Error)result;
	}
	if (j->phase != kWaitingForNewImage) 
		return kCannotParse;
	return kNoError;
}
#endif

