// ===========================================================================
//	Animation.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __ANIMATION_H__
#include "Animation.h"
#endif
#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __GIF_H__
#include "GIF.h"
#endif
#ifndef __LAYER_H__
#include "Layer.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
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
#ifndef __URLPARSER_H__
#include "URLParser.h"
#endif

#ifdef DEBUG
#define DEBUG_ANIMATION_C
#endif


ulong	gPendingAnimationDissolvePeriod = 0;
TransitionEffect gAnimationTransitionType = kTransitionNone;


// ===========================================================================
//	implementation
// ===========================================================================

Animation::Animation()
{
	fBackwards = false;
	fDirectionX = 1;
	fDirectionY = 1;
	fNextIdle = Now();
	fFrameList.SetDataSize(sizeof(Frame));
	fImageData = nil;
	fRunning = true;
}

Animation::~Animation()
{
	if(fSRC) {
		FreeTaggedMemory(fSRC,"Image::fSRC");
		fSRC = nil;
	}
	fImageList.DeleteAll();
	fImageData = nil;
}

void Animation::Draw(const Document* document, const Rectangle* invalid)
{
	if (fAnimationState != kAnimationReady)
		return;

	Image::Draw(document, invalid);
}

#ifdef FIDO_INTERCEPT
void Animation::Draw(const Document* document, FidoCompatibilityState& fidoCompatibility) const
{
	if (fAnimationState != kAnimationReady)
		return;

	Image::Draw(document, fidoCompatibility);
}
#endif

long
Animation::GetLayoutHeight() const
{
	return fSkipLayout ? 0 : Displayable::GetLayoutHeight();
}

long
Animation::GetLayoutWidth() const
{
	return fSkipLayout ? 0 : Displayable::GetLayoutWidth();
}

Boolean Animation::SetCurrentFrame(ulong newFrame, Rectangle *destinationBounds)
{
	ulong		count = fFrameList.GetCount();
	
	if (count == 0)
		return false;

Next:
	if (newFrame >= count) {
		if (!fShouldLoop) {
			Stop();
			return false;
		}
		newFrame = 0;
	}
	fCurrentFrame = newFrame;
	Frame*	frame = (Frame*)fFrameList.At(newFrame);
	
	switch (frame->fType)
	{
		case kMotionFrame:
			fImageData = (ImageData*)fImageList.At(frame->u.motion.fImageNumber);
			break;
		case kLoopFrame:
			if (frame->u.loop.fCurrentCount == -1 || --frame->u.loop.fCurrentCount >= 0)
				newFrame = frame->u.loop.fLoopTopFrame;
			else
			{
				newFrame++;
				frame->u.loop.fCurrentCount = frame->u.loop.fMaxCount; 
			}
			goto Next;
		default:
			Trespass();
	};
	
	Assert(fImageData != nil);
	fImageData->PushData(false);
	frame = (Frame*)fFrameList.At(fCurrentFrame);
	Assert(frame->fType == kMotionFrame);
	fImageData->SetBounds(&frame->u.motion.fSourceBounds);
	if (destinationBounds != nil && (frame->u.motion.fScaleX != 100 || frame->u.motion.fScaleY != 100))
	{
#if 0
		long		xCenter = (destinationBounds->left + destinationBounds->right)/2;
		long		yCenter = (destinationBounds->top + destinationBounds->bottom)/2;
		long		midWidth = (destinationBounds->right - destinationBounds->left)*frame->u.motion.fScaleX/200;
		long		midHeight = (destinationBounds->bottom - destinationBounds->top)*frame->u.motion.fScaleY/200;
		
		destinationBounds->top = yCenter - midHeight;
		destinationBounds->left = xCenter - midWidth;
		destinationBounds->bottom = yCenter + midHeight;
		destinationBounds->right = xCenter + midWidth;
#else
		destinationBounds->bottom = destinationBounds->top + (destinationBounds->bottom - destinationBounds->top)*frame->u.motion.fScaleX/100;
		destinationBounds->right = destinationBounds->left + (destinationBounds->right - destinationBounds->left)*frame->u.motion.fScaleY/100;
#endif
	}
	return true;
}

Boolean Animation::Idle(Layer* layer)
{
	PerfDump perfdump("Animation::Idle");
	
	Boolean		changed = false;
	Rectangle	r;
	Error		status;
	long		count;
	
	switch (fAnimationState)
	{
		case kAnimationCreateStream:
			if (TrueError(fResource.GetStatus())) {
				Reset();
				fAnimationState = kAnimationError;
			}
			
			if ((fStream = fResource.NewStream()) == nil) 
				return false;
			fAnimationState = kAnimationReadHeader;
		
		case kAnimationReadHeader:
		{
			if ((status = fStream->GetStatus()) == kStreamReset) {
				Reset();
				fAnimationState = kAnimationCreateStream;
				return false;
			}
			
			if (TrueError(status)) {
HandleError:
				Reset();
				fAnimationState = kAnimationError;
				return false;
			}
			
			if (status == kComplete) {
				if (!ParseData())
					goto HandleError;
				
				delete(fStream);
				fStream = nil;
				SetCurrentFrame(fCurrentFrame);
				fAnimationState = kAnimationWaitForImages;
				fState = kDetermineImageSize;			// Let Image determine its size.
			}
			else
				break;
		}
		
		case kAnimationWaitForImages:
			count = fImageList.GetCount();

			fAnimationState = kAnimationReady;					// Assume we can move on.
			for (long i = 0; i < count; i++) {
				ImageData*	data = (ImageData*)fImageList.At(i);
				
				data->PushData();
				if ((status = data->GetStatus()) != kComplete) {
					if (TrueError(status))
						goto HandleError;						// If any image is bad then we're bad
					fAnimationState = kAnimationWaitForImages;	// Hold myself in check because image[i] not in yet
					break;										// no use looking at anyone else
				}
			}

			if (fAnimationState != kAnimationReady)
				break;
			// otherwise just fall through and don't waste another round trip to idle
			
		case kAnimationReady:
		{
			Coordinate	delta;
				
			changed =  Image::Idle(layer);
							
			if (!fIsVisible || !fIdleDrawEnabled)
				break;
							
			// Give animations a chance to move -- and clean up if they do.
			GetBounds(&r);
			Coordinate	scale;
			ulong	dissolveIntoPeriod = 0;
			
			scale.x = scale.y = 100;
			if (AnimationIdle(layer, &delta, &scale, &dissolveIntoPeriod)) {
				InvalidateBounds(layer);
				
				r.left += delta.x*fDirectionX;
				r.top += delta.y*fDirectionY;
				
				Rectangle	dataBounds = ((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fSourceBounds;
				Boolean		saveSkip = fSkipLayout;
				
				fSkipLayout = false;
				SetLeft(r.left);
				SetTop(r.top);
				SetWidth((dataBounds.right-dataBounds.left)*scale.x/100);
				SetHeight((dataBounds.bottom-dataBounds.top)*scale.y/100);
				fSkipLayout = saveSkip;
				
				GetBoundsTight(&r);
				InvalidateBounds(layer);
				

				if ( dissolveIntoPeriod ) {
					gPendingAnimationDissolvePeriod = dissolveIntoPeriod;
					gAnimationTransitionType = kTransitionCrossFade;
				} else
					gAnimationTransitionType = kTransitionNone;

                // Bounce off parent bounds
                if (fParent != nil) {
                    Rectangle       parentBounds;
                    fParent->GetBounds(&parentBounds);
                    if (r.left < parentBounds.left && fShouldHBounce && delta.x*fDirectionX < 0) {
                        fImageData->SetFlip(delta.x < 0, fShouldVFlip && fDirectionY == -1);
                        fDirectionX = (delta.x >=0) ? 1 : -1;
                    }
                    else if (r.right > parentBounds.right && fShouldHBounce && delta.x*fDirectionX > 0) {
                    	fImageData->SetFlip(fShouldHFlip && delta.x >= 0, fShouldVFlip && fDirectionY == -1);
                        fDirectionX = (delta.x >= 0) ? -1 : 1;
                    }
                    if (r.top < parentBounds.top && fShouldVBounce && delta.y*fDirectionY < 0) {
						fImageData->SetFlip(fShouldHFlip && fDirectionX == -1, false);
						fDirectionY = (delta.y >= 0) ? 1 : -1;
                    }
                    else if (r.bottom > parentBounds.bottom && fShouldVBounce && delta.y*fDirectionY > 0) {
                   		fImageData->SetFlip(fShouldHFlip && fDirectionX == -1, fShouldVFlip);
                        fDirectionY = (delta.y >= 0) ? -1 : 1;
                    }
                }
			}
			break;
		}
		case kAnimationError:
			changed = Image::Idle(layer);
			break;					
		default:
			break;
	}

	return changed;
}
Boolean Animation::IsAnimation() const
{
	return true;
}

Boolean Animation::AnimationIdle(Layer*, Coordinate *delta, Coordinate* scale, ulong* dissolveIntoPeriod)
// function returns whether any change occurred,
// and if true delta is filled in w/ locomotion amount
{
	if (fAnimationState != kAnimationReady)
		return false;
	if (fImageData->GetStatus() != kComplete)
	{
		Rectangle	unused;
		fImageData->GetBounds(&unused);
		return false;
	}

	if (!fRunning)
		return false;
		
	const long kAcceptableDelta = 0x40000000;
	ulong now = Now();

	// ensure it is in the past, but not too far
	if (now - fNextIdle > kAcceptableDelta)
		return false;
		
	if (!SetCurrentFrame(fCurrentFrame + 1))
		return false;
		
	Assert(((Frame*)fFrameList.At(fCurrentFrame))->fType == kMotionFrame);
	
	fNextIdle = now + ((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fTicksToNext;
	if (delta != nil)
		*delta = ((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fTranslation;
	if (scale != nil) {
		scale->x = (long)((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fScaleX;
		if (scale->x < 0)
			scale->x = 100;
		scale->y = (long)((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fScaleY;
		if (scale->y < 0)
			scale->y = 100;
	}
	if (dissolveIntoPeriod != nil)
		*dissolveIntoPeriod = ((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fDissolveIntoPeriod;
	return true;
}

void Animation::PushLoopFrame(DataList* loopFrames, ulong maxCount)
{
	Frame		frame;

	ZeroStruct(&frame);
	frame.fType = kLoopFrame;
	frame.u.loop.fMaxCount = maxCount;
	frame.u.loop.fCurrentCount = maxCount;
	frame.u.loop.fLoopTopFrame = fFrameList.GetCount();
	loopFrames->Add(&frame);
}

Boolean Animation::PopLoopFrame(DataList* loopFrames)
{
	ulong		count = loopFrames->GetCount();
	
	if (count == 0)
		return false;
	
	Frame*		loopFrame = (Frame*)loopFrames->At(count - 1);
	fFrameList.Add(loopFrame);
	loopFrames->RemoveAt(count - 1);

	return true;
}

Boolean Animation::ParseData()
{
	const char*	data = nil;
	const char*	base;
	ulong		dataOffset = 0;
	Boolean		result = false;
	ulong		period = 12;	/* default of 12 ticks */
	Rectangle	bounds;
	Frame		frame;
	ulong		scanCount;
	ulong		imageCount;
	char*		buffer = nil;
	ulong		bufferLength;
	ulong		loopCount;
	long		value, auxValue;
	
	TrivialMessage(("Parsing data for Ani Image"));

	StringList*	frameNames = new(StringList);
	DataList*	frameImageCounts = new(DataList);
	DataList*	frameSizes = new(DataList);
	DataList*	loopFrames = new(DataList);
	if (frameNames == nil || frameImageCounts == nil || frameSizes == nil || loopFrames == nil)
		goto Done;
	frameImageCounts->SetDataSize(sizeof(ulong));
	frameSizes->SetDataSize(sizeof(Rectangle));
	loopFrames->SetDataSize(sizeof(Frame));
	result = true;
	
	for (data = base = fStream->GetDataAsString(); data != nil; data = GetNextLine((base = fStream->GetDataAsString()) + dataOffset))
	{
		/* note: this could all be handled in GetNextLine */
		while (isspace(*data))
			data++;

		dataOffset = data - base;
		
		if (*data == 0)
			break;
		if (*data == '#')
			continue;		// comment
				
		bufferLength = strlen(data);
		if ((buffer = (char*)AllocateBuffer(bufferLength)) == nil)
			{	result = false; goto Done; }
		
		if ((scanCount = sscanf(data, "IMAGE = %s", buffer)) != 0)
		{
			URLParser urlParser;
			urlParser.SetURL((const char*)buffer);
			fImageData = ImageData::NewImageData(urlParser.GetURL());
#ifdef DEBUG_ANIMATION_C
			data = (char*)-1;	// no longer valid
#endif
			fImageData->SetPriority(GetPriority());
			fImageList.Add(fImageData);
			fImageData->SetKeepBitMap(kNormalBitMap);
		}
		else if (sscanf(data, "PERIOD = %ld", &period) != 0)
			;
		else if ((scanCount = sscanf(data, "FRAME = %ld %ld %ld %ld",
			 &bounds.top, &bounds.left,
			&bounds.bottom , &bounds.right)) == 4)
		{
			imageCount = fImageList.GetCount();
			if (!IsWarning(imageCount == 0 || imageCount >= kMaxCelImages))
			{
				ZeroStruct(&frame);
				frame.fType = kMotionFrame;
				frame.u.motion.fTicksToNext = period;
				frame.u.motion.fSourceBounds = bounds;
				frame.u.motion.fScaleX = 100;
				frame.u.motion.fScaleY = 100;
				//frame.u.motion.fDissolveIntoPeriod = 0;
				frame.u.motion.fImageNumber = imageCount - 1;
				fFrameList.Add(&frame);
#ifdef DEBUG_ANIMATION_C
				data = (char*)-1;	// no longer valid
#endif
			}
		}
		else if ((scanCount = sscanf(data, "FRAME %s = %ld %ld %ld %ld",
			buffer, &bounds.top, &bounds.left,
			&bounds.bottom , &bounds.right)) == 5)
		{
			imageCount = fImageList.GetCount();
			if (!IsWarning(imageCount == 0 || imageCount >= kMaxCelImages))
			{
				frameNames->Add(buffer);
#ifdef DEBUG_ANIMATION_C
				data = (char*)-1;	// no longer valid
#endif
				frameSizes->Add(&bounds);
				frameImageCounts->Add(&imageCount);
			}
		}
		else if (sscanf(data, "FRAME = %s", buffer) != 0)
		{
			if (EqualString(buffer, "ALL") && !IsWarning(fImageData == nil)) {
				fImageData->GetBounds(&bounds);
				fImageData->SetKeepBitMap(kNoBitMap);
				
				ZeroStruct(&frame);
				frame.fType = kMotionFrame;
				frame.u.motion.fTicksToNext = period;
				frame.u.motion.fSourceBounds = bounds;
				frame.u.motion.fScaleX = 100;
				frame.u.motion.fScaleY = 100;
				//frame.u.motion.fDissolveIntoPeriod = 0;
				frame.u.motion.fImageNumber = fImageList.GetCount() - 1;
				fFrameList.Add(&frame);
			}
		}
		else if (sscanf(data, "FRAME %s", buffer) != 0)
		{
			long		nameIndex = frameNames->Find(buffer);
			if (nameIndex == -1)
			{	result = false; goto Done; }
			
			imageCount = *(ulong*)frameImageCounts->At(nameIndex);
			if (!IsWarning(imageCount == 0 || imageCount >= kMaxCelImages))
			{
				ZeroStruct(&frame);
				frame.fType = kMotionFrame;
				frame.u.motion.fTicksToNext = period;
				frame.u.motion.fSourceBounds = *(Rectangle*)frameSizes->At(nameIndex);
				frame.u.motion.fScaleX = 100;
				frame.u.motion.fScaleY = 100;
				//frame.u.motion.fDissolveIntoPeriod = 0;
				frame.u.motion.fImageNumber = imageCount - 1;
				fFrameList.Add(&frame);
#ifdef DEBUG_ANIMATION_C
				data = (char*)-1;	// no longer valid
#endif
			}
		}
		else if ((scanCount = sscanf(data, "DISSOLVE = %ld", &value)) == 1)
		{
			ulong	count = fFrameList.GetCount();
			if (count == 0 || value < 1)
			{	result = false; goto Done; }
			Frame*		currentFrame = (Frame*)fFrameList.At(count - 1);
			Assert(currentFrame != nil && currentFrame->fType == kMotionFrame);
			currentFrame->u.motion.fDissolveIntoPeriod = value;
		}
		else if ((scanCount = sscanf(data, "ZOOM = %ld %ld", &value, &auxValue)) >= 1)
		{
			ulong	count = fFrameList.GetCount();
			if (count == 0 || value < 1)
			{	result = false; goto Done; }
			Frame*		currentFrame = (Frame*)fFrameList.At(count - 1);
			Assert(currentFrame != nil && currentFrame->fType == kMotionFrame);
			currentFrame->u.motion.fScaleX = value;
			currentFrame->u.motion.fScaleY = (scanCount == 2) ? auxValue : value;
		}
		else if ((scanCount = sscanf(data, "ADVANCE = %ld %ld", &bounds.left, &bounds.top)) == 2)
		{
			ulong	count = fFrameList.GetCount();
			if (count == 0)
			{	result = false; goto Done; }
			Frame*		currentFrame = (Frame*)fFrameList.At(count - 1);
			Assert(currentFrame != nil && currentFrame->fType == kMotionFrame);
			currentFrame->u.motion.fTranslation.x = bounds.left;
			currentFrame->u.motion.fTranslation.y = bounds.top;
		}
		else if ((scanCount = sscanf(data, "LOOP %ld", &loopCount)) == 1)
		{
			PushLoopFrame(loopFrames, loopCount);
#ifdef DEBUG_ANIMATION_C
			data = (char*)-1;	// no longer valid
#endif
		}
		else if (FindString(data, "LOOP", true))
		{
			PushLoopFrame(loopFrames, (ulong)-1);
#ifdef DEBUG_ANIMATION_C
			data = (char*)-1;	// no longer valid
#endif
		}
		else if (FindString(data, "END", true))
		{
			if (!PopLoopFrame(loopFrames))
			{
				ImportantMessage(("Too many ENDs in Ani parsing"));
				result = false;
				goto Done;
			}
		}
		else if (FindString(data, "HBOUNCE", true))
		{
			fShouldHBounce = true;
#ifdef DEBUG_ANIMATION_C
			data = (char*)-1;	// no longer valid
#endif
		}
		else if (FindString(data, "VBOUNCE", true))
		{
			fShouldVBounce = true;
#ifdef DEBUG_ANIMATION_C
			data = (char*)-1;	// no longer valid
#endif
		}
		else if (FindString(data, "HFLIP", true))
		{
			fShouldHBounce = true;
			fShouldHFlip = true;
#ifdef DEBUG_ANIMATION_C
			data = (char*)-1;	// no longer valid
#endif
		}
		else if (FindString(data, "VFLIP", true))
		{
			fShouldVBounce = true;
			fShouldVFlip = true;
#ifdef DEBUG_ANIMATION_C
			data = (char*)-1;	// no longer valid
#endif
		}
		else
			ImportantMessage(("Ignoring Ani line \"%s\"", data));
				
		FreeBuffer(buffer, bufferLength);
		buffer = nil;
	}

Done:
	if (buffer != nil)
		FreeBuffer(buffer, bufferLength);
	if (!result && data != nil) {
		ImportantMessage(("Failed parsing Ani line \"%s\"", data));
	} else if (fImageData == nil)
	{
		ImportantMessage(("No image data found in Ani parsing"));
		result = false;
	}
	else if (loopFrames->GetCount() != 0)
	{
		ImportantMessage(("Missing END in Ani parsing"));
		result = false;
	}
	
	if (loopFrames != nil)
		delete(loopFrames);
	if (frameImageCounts != nil)
		delete(frameImageCounts);
	if (frameNames != nil)
		delete(frameNames);
	if (frameSizes != nil)
		delete(frameSizes);
	
	return result;
}

void Animation::Reset()
{
	fFrameList.RemoveAll();
	fCurrentFrame = 0;
	fImageList.DeleteAll();
	fImageData = nil;
	delete(fStream);
	fStream = nil;
}

void Animation::PurgeResource()
{
	Image::PurgeResource();
	
	long count = fImageList.GetCount();
	for (long i = 0; i < count; i++) {		
		ImageData* imageData = (ImageData *)fImageList.At(i);
		if (!IsError(imageData == nil)) {
			Resource resource = *imageData->GetResource();
			resource.Purge();
		}
	}
}

void Animation::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{		
		case A_ANISTARTX:		fSkipLayout = true; fLeft = MAX(value, 0);	break;
		case A_ANISTARTY:		fSkipLayout = true; fTop = MAX(value, 0);	break;
		case A_LAYER:			ImportantMessage(("Would be putting in layer %ld.", value));
								break;
		default:				Image::SetAttribute(attributeID, value, isPercentage);
	}
}

void Animation::SetAttributeStr(Attribute attributeID, const char* value)
{	
	switch (attributeID) {		
	case A_ANI:
		fSRC = CopyStringTo(fSRC, value, "Image::fSRC");
		break;
	case A_LOOP:
		SetShouldLoop(true);
		break;
	default:
		Image::SetAttributeStr(attributeID, value);
	}
}

void Animation::SetShouldLoop(Boolean newValue)
{
	fShouldLoop = newValue;
}

void Animation::SetTop(long top)
{
	if (!fSkipLayout)
		SpatialDisplayable::SetTop(top);
}

void Animation::SetLeft(long left)
{
	if (!fSkipLayout)
		SpatialDisplayable::SetLeft(left);
}

void Animation::Start()
{
	fRunning = true; 
}

void Animation::Stop()
{
	fRunning = false; 
}