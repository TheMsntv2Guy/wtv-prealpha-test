// ===========================================================================
//	CelImage.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CELIMAGE_H__
#include "CelImage.h"
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
#ifndef __REGION_H__
#include "Region.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif




// ===========================================================================
//	implementation
// ===========================================================================

NewCelImage::NewCelImage()
{
	fBackwards = false;
	fNextIdle = Now();
	fFrameList.SetDataSize(sizeof(Frame));
	fImageData = nil;
	fRunning = true;
}

NewCelImage::~NewCelImage()
{
	fImageList.DeleteAll();
}

void NewCelImage::Draw(Rectangle* r, const Rectangle* invalid, ulong transparency)
{
	PushData();
	
	if (fState != kNewCelDone)
		return;

	if (SetCurrentFrame(fCurrentFrame) && invalid != nil)
	{
		PostulateFinal(false);	// need cleaner way to scale
		UnionRectangle((Rectangle*)invalid, r);	// make sure we force fit the invalid region
	}
	fImageData->Draw(r, invalid, transparency);
}

Boolean NewCelImage::DrawingIdle(Rectangle* r)
{
	PushData();
	
	if (fState != kNewCelDone)
		return false;
	
	Assert(fImageData != nil);
	return fImageData->DrawingIdle(r);
}

Boolean NewCelImage::GetBounds(Rectangle* bounds)
{
	((NewCelImage*)this)->PushData();
	
	if (fState != kNewCelDone)
		return false;
	
	Assert(fImageData != nil);
	if (!fImageData->GetBounds(bounds))
		return false;	/* not yet ready */
		
	Assert(((Frame*)fFrameList.At(fCurrentFrame))->fType == kMotionFrame);
	*bounds = ((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fSourceBounds;
	return true;
}

Error NewCelImage::GetStatus() const
{
	if (fImageData == nil)
		return ImageData::GetStatus();
	
	return fImageData->GetStatus();	
}

Boolean NewCelImage::SetCurrentFrame(ulong newFrame, Rectangle *destinationBounds)
{
	ulong		count = fFrameList.GetCount();
	
	if (count == 0)
		return false;

Next:
	if (newFrame >= count)
	{
		newFrame = 0;
		if (!fShouldLoop)
			Stop();
	}
	fCurrentFrame = newFrame;
	Frame*	frame = (Frame*)fFrameList.At(newFrame);
	
	switch (frame->fType)
	{
		case kMotionFrame:
			fImageData = (GIFImage*)fImageList.At(frame->u.motion.fImageNumber);
			break;
		case kLoopFrame:
			if (frame->u.loop.fCurrentCount == (ulong)-1 || --frame->u.loop.fCurrentCount >= 0)
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
	fImageData->PushData();
	frame = (Frame*)fFrameList.At(fCurrentFrame);
	Assert(frame->fType == kMotionFrame);
	fImageData->SetBounds(&frame->u.motion.fSourceBounds);
	if (destinationBounds != nil && frame->u.motion.fScale != 100)
	{
#if 0
		long		xCenter = (destinationBounds->left + destinationBounds->right)/2;
		long		yCenter = (destinationBounds->top + destinationBounds->bottom)/2;
		long		midWidth = (destinationBounds->right - destinationBounds->left)*frame->u.motion.fScale/200;
		long		midHeight = (destinationBounds->bottom - destinationBounds->top)*frame->u.motion.fScale/200;
		
		destinationBounds->top = yCenter - midHeight;
		destinationBounds->left = xCenter - midWidth;
		destinationBounds->bottom = yCenter + midHeight;
		destinationBounds->right = xCenter + midWidth;
#else
		destinationBounds->bottom = destinationBounds->top + (destinationBounds->bottom - destinationBounds->top)*frame->u.motion.fScale/100;
		destinationBounds->right = destinationBounds->left + (destinationBounds->right - destinationBounds->left)*frame->u.motion.fScale/100;
#endif
	}
	return true;
}

Boolean NewCelImage::Idle(Layer*, Coordinate *delta, ulong* scale)
// function returns whether any change occurred,
// and if true delta is filled in w/ locomotion amount
{
	PushData();
	
	if (fState != kNewCelDone)
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
	if (scale != nil)
		*scale = ((Frame*)fFrameList.At(fCurrentFrame))->u.motion.fScale;
	return true;
}

void NewCelImage::PushLoopFrame(DataList* loopFrames, ulong maxCount)
{
	Frame		frame;

	ZeroStruct(&frame);
	frame.fType = kLoopFrame;
	frame.u.loop.fMaxCount = maxCount;
	frame.u.loop.fCurrentCount = maxCount;
	frame.u.loop.fLoopTopFrame = fFrameList.GetCount();
	loopFrames->Add(&frame);
}

Boolean NewCelImage::PopLoopFrame(DataList* loopFrames)
{
	ulong		count = loopFrames->GetCount();
	
	if (count == 0)
		return false;
	
	Frame*		loopFrame = (Frame*)loopFrames->At(count - 1);
	fFrameList.Add(loopFrame);
	loopFrames->RemoveAt(count - 1);

	return true;
}

Boolean NewCelImage::ParseData()
{
	const char*	data;
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
	long		scale;
	
	TrivialMessage(("Parsing data for Cel Image"));

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
		dataOffset = data - base;
		
		/* note: this could all be handled in GetNextLine */
		while (*data == ' ' || *data == '\t' || *data == '\n' || *data == '\r')
			data++;
		if (*data == 0)
			break;
		if (*data == '#')
			continue;		// comment
				
		bufferLength = strlen(data);
		if ((buffer = (char*)AllocateBuffer(bufferLength)) == nil)
			{	result = false; goto Done; }
		
		if ((scanCount = sscanf(data, "IMAGE = %s", buffer)) != 0)
		{
			fImageData = GIFImage::NewGIFImage(buffer);
#ifdef DEBUG
			data = (char*)-1;	// no longer valid
#endif
			fImageData->SetPriority(GetPriority());
			fImageList.Add(fImageData);
			fImageData->SetKeepBitMap(kNormalBitMap);
		}
		else if (sscanf(data, "PERIOD = %d", &period) != 0)
			;
		else if ((scanCount = sscanf(data, "FRAME = %d %d %d %d",
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
				frame.u.motion.fScale = 100;
				frame.u.motion.fImageNumber = imageCount - 1;
				fFrameList.Add(&frame);
#ifdef DEBUG
				data = (char*)-1;	// no longer valid
#endif
			}
		}
		else if ((scanCount = sscanf(data, "FRAME %s = %d %d %d %d",
			buffer, &bounds.top, &bounds.left,
			&bounds.bottom , &bounds.right)) == 5)
		{
			imageCount = fImageList.GetCount();
			if (!IsWarning(imageCount == 0 || imageCount >= kMaxCelImages))
			{
				frameNames->Add(buffer);
#ifdef DEBUG
				data = (char*)-1;	// no longer valid
#endif
				frameSizes->Add(&bounds);
				frameImageCounts->Add(&imageCount);
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
				frame.u.motion.fScale = 100;
				frame.u.motion.fImageNumber = imageCount - 1;
				fFrameList.Add(&frame);
#ifdef DEBUG
				data = (char*)-1;	// no longer valid
#endif
			}
		}
		else if ((scanCount = sscanf(data, "ZOOM = %d", &scale)) == 1)
		{
			ulong	count = fFrameList.GetCount();
			if (count == 0 || scale < 1)
			{	result = false; goto Done; }
			Frame*		currentFrame = (Frame*)fFrameList.At(count - 1);
			Assert(currentFrame != nil && currentFrame->fType == kMotionFrame);
			currentFrame->u.motion.fScale = scale;
		}
		else if ((scanCount = sscanf(data, "ADVANCE = %d %d", &bounds.left, &bounds.top)) == 2)
		{
			ulong	count = fFrameList.GetCount();
			if (count == 0)
			{	result = false; goto Done; }
			Frame*		currentFrame = (Frame*)fFrameList.At(count - 1);
			Assert(currentFrame != nil && currentFrame->fType == kMotionFrame);
			currentFrame->u.motion.fTranslation.x = bounds.left;
			currentFrame->u.motion.fTranslation.y = bounds.top;
		}
		else if ((scanCount = sscanf(data, "LOOP %d", &loopCount)) == 1)
		{
			PushLoopFrame(loopFrames, loopCount);
#ifdef DEBUG
			data = (char*)-1;	// no longer valid
#endif
		}
		else if (FindString(data, "LOOP", true))
		{
			PushLoopFrame(loopFrames, (ulong)-1);
#ifdef DEBUG
			data = (char*)-1;	// no longer valid
#endif
		}
		else if (FindString(data, "END", true))
		{
			if (!PopLoopFrame(loopFrames))
			{
				ImportantMessage(("Too many ENDs in Cel parsing"));
				result = false;
				goto Done;
			}
		}
		else
			ImportantMessage(("Ignoring Cel line \"%s\"", data));
				
		FreeBuffer(buffer, bufferLength);
		buffer = nil;
	}

Done:
	if (buffer != nil)
		FreeBuffer(buffer, bufferLength);
	if (!result) {
		ImportantMessage(("Failed parsing Cel line \"%s\"", data));
	} else if (fImageData == nil)
	{
		ImportantMessage(("No image data found in Cel parsing"));
		result = false;
	}
	else if (loopFrames->GetCount() != 0)
	{
		ImportantMessage(("Missing END in Cel parsing"));
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

Boolean NewCelImage::PushData(Boolean, Boolean)
{
	DataStream* stream;
	
	switch (fState)
	{
		case kNewCelCreateStream:
			if ((stream = fResource.NewStream()) == nil) 
				return false;
			SetStream(stream);
			fState = kNewCelReadHeader;
		
		case kNewCelReadHeader:
			Error status = fStream->GetStatus();
			
			if (status == kStreamReset)
			{
				Reset();
				fState = kNewCelCreateStream;
				return false;
			}
			
			if (TrueError(status))
			{
HandleError:
				DeleteStream();
				fState = kNewCelError;
				return false;
			} 
			
			if (status == kComplete)
			{
				if (!ParseData())
					goto HandleError;
				DeleteStream();
				SetCurrentFrame(fCurrentFrame);
				fState = kNewCelDone;
			}
	}
	
	return true;
}

void NewCelImage::Reset()
{
	Assert(fImageData != nil);
	delete(fImageData);
	fImageData = nil;
	
	fFrameList.RemoveAll();
	fCurrentFrame = 0;
	DeleteStream();
}

void NewCelImage::SetFlip(Boolean h, Boolean v)
{
	ulong		count = fImageList.GetCount();
	
	for (ulong i = 0; i < count; i++)
		((GIFImage*)fImageList.At(i))->SetFlip(fFlipHorizontal = h, fFlipVertical = v);
}

void NewCelImage::SetShouldLoop(Boolean newValue)
{
	fShouldLoop = newValue;
}

void NewCelImage::Start()
{
	fRunning = true; 
}

void NewCelImage::Stop()
{
	fRunning = false; 
}