// ===========================================================================
//	Animation.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __AUDIO_SCOPE_H__
#include "AudioScope.h"
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

#ifndef G_SOUND
#include "GenSnd.h"
#endif
#ifndef G_PRIVATE
#include "GenPriv.h"
#endif


// ===========================================================================
//	implementation
// ===========================================================================

AudioScope::AudioScope()
{	
	fRunning = false;
	fDrawCalled = false;
	fState = kWaitForImageLayout;
	fSizeKnown = true;
	
	fBounds.top = 0;
	fBounds.left = 0;
	fBounds.bottom = 80;
	fBounds.right = 100;
	
	fKnownWidth = 100;
	fKnownHeight = 80;
	
	fBGColor = 0x007f7f7f;
	fLeftColor = 0x00c00000;
	fRightColor = 0x000000c0;

	fLeftOffset = 0;
	fRightOffset = 1;
	fGain = 1;
	
	fMaxLevel = false;
	
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberAudioScope;
#endif /* DEBUG_CLASSNUMBER */
}

AudioScope::~AudioScope()
{
	if(fRunning)
	{
		DeleteBitMapDevice(fBitmapDevice);		/* this will toss the CLUT for us */
		fRunning = false;
	}
}

void AudioScope::SetColor(long index, long color)
{
	fCLUT->data[index*3] = color>>16;
	fCLUT->data[index*3+1] = color>>8;
	fCLUT->data[index*3+2] = color;
}

void AudioScope::Setup(void)
{	
	fCLUT = NewColorTable(kRGB24, 16);
	if(fCLUT)
	{			
		fBitmapDevice = NewBitMapDevice(fBounds, index4Format, fCLUT, 0);
		if(fBitmapDevice) {
			fBitmapDevice->colorTable = fCLUT;
			fBitmapDevice->filter = kFullFilter;
			SetColor(kBGIndex, fBGColor);
			SetColor(kLeftIndex, fLeftColor);
			SetColor(kRightIndex, fRightColor);
			
			SetColor(kWhiteIndex, 	0x00ffffff);
			SetColor(kGrayIndex, 	0x007f7f7f);
			SetColor(kLtGrayIndex,	0x003f3f3f);
			
			fVel = 1;

			fSettle = (fBounds.bottom - fBounds.top)>>1;
			
			if(fLeftOffset > fRightOffset)
				fSettle += fRightOffset;
			else
				fSettle += fLeftOffset;

			fMax = fSettle;
				
			fPos = fSettle;
										
			fRunning = true;						/* only set if we got our bufs */
		} else
			DeleteColorTable(fCLUT);
	}	
}


void AudioScope::Draw(const Document* document, const Rectangle* invalid)
{
	short left[MAX_CHUNK_SIZE];
	short right[MAX_CHUNK_SIZE];

	fDrawCalled = true;
	
	if (!fIsVisible)
		return;

	if(fRunning)
	{
		BlankScope(kBGIndex);
		
		GM_GetAudioSampleFrame(left, right);	
			
		DrawTrace(left, kLeftIndex, fLeftOffset);
		DrawTrace(right, kRightIndex, fRightOffset);
		
		if(fMaxLevel)
			DrawMax();
	
		Rectangle bounds;
		GetBoundsTight(&bounds);
		document->GetView()->ContentToScreen(&bounds);

		// create an ImageData, and stick the bitmap device into it.
		// then call draw on the ImageData.
		
		CopyImage(*fBitmapDevice, gScreenDevice, fBounds, bounds, 0, invalid, 0, 0, false); 
		DrawBorder(document, invalid);		
	}
}

#ifdef FIDO_INTERCEPT
void AudioScope::Draw(const Document* document, FidoCompatibilityState& fidoCompatibility) const
{
	if (fAnimationState != kAnimationReady)
		return;

	Image::Draw(document, fidoCompatibility);
}
#endif

void AudioScope::BlankScope(uchar index)
{
ulong xxx,yyy;
ulong height;
Byte *pix;
Byte color;

	color = index | (index << 4);
	
	pix = fBitmapDevice->baseAddress;
	
	height = fBounds.bottom - fBounds.top;
	
	for(yyy=0;yyy!=height;yyy++)
		for(xxx=0;xxx!=fBitmapDevice->rowBytes;xxx++)
			*(pix + xxx + (yyy*fBitmapDevice->rowBytes)) = color;
}


void AudioScope::DrawTrace(short *audio, uchar index, long offset)
{
ushort xScale,yScale;
ushort sample;
ulong x, nextsamp;
ulong width,height;
uchar pair;
Byte *pix;

	pix = fBitmapDevice->baseAddress;

	xScale = (MAX_CHUNK_SIZE<<2)/(fBounds.right - fBounds.left);
		
	yScale = 65536/(fBounds.bottom - fBounds.top);
	
	width = (fBounds.right - fBounds.left);
	height = (fBounds.bottom - fBounds.top) - 1;
	
	pix = fBitmapDevice->baseAddress;
		
	nextsamp = 0;
		
	for(x = 0; x != width; x++)
	{
		sample = audio[nextsamp>>2];
		nextsamp += xScale;
		
		sample *= fGain;		/* variable gain */
		sample += 0x8000;		/* normalize */
		sample /= yScale;	
		sample += offset;		/* so we can slide the waves apart */
				
		if(sample > height)
			sample = height; 	/* pin */

		if(sample < fMax)
			fMax = sample;

		pair = *(pix + (sample*(fBitmapDevice->rowBytes)) + (x>>1));
		
		if(x&1)
			pair = ((pair&0xf0) | index);
		else
			pair = ((pair&0x0f) | (index<<4));
			
		*(pix + (sample*(fBitmapDevice->rowBytes)) + (x>>1)) = pair;
	}
}

void AudioScope::DrawHLine(long pos, uchar index)
{
ulong x;
Byte *pix;

	if(pos > 0)
	{
		index = index | (index << 4);
		pix = fBitmapDevice->baseAddress + (pos*(fBitmapDevice->rowBytes));
		
		for(x = 0; x != fBitmapDevice->rowBytes; x++)
			*(pix + x) = index;
	}
}


void AudioScope::DrawMax(void)
{
	if(fPos > fMax)
	{	
		fPos = fMax;

		if(fMax != fSettle)
		{
			DrawHLine(fPos+4, kGrayIndex);	
			DrawHLine(fPos+8, kLtGrayIndex);	
		}
	}
	else
	{
		if(fMax != fSettle)
		{
			DrawHLine(fPos-4, kGrayIndex);	
			DrawHLine(fPos-8, kLtGrayIndex);	
		}
	}

	DrawHLine(fPos, kWhiteIndex);				
	
	fPos += fVel;

	fMax = fSettle;
}


/*	Render the current audio buf
*/

Boolean AudioScope::Idle(Layer* layer)
{
Boolean changed = false;

	PerfDump perfdump("AudioScope::Idle");

	if(fRunning)
	{
		if (fState == kWaitForImageLayout)
			if (fLayoutComplete)
				fState = kDrawImage;
			else
				return changed;
	
		if(fDrawCalled)
		{
			Document* document = ((ContentView*)layer)->GetDocument();
			
			Rectangle bounds;
			GetBoundsTight(&bounds);

			document->GetView()->ContentToScreen(&bounds);

			Draw(document, &bounds);

			layer->InvalidateAbove(&bounds);
		}
	}
	
	return changed;
}


void AudioScope::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{	
		case A_BORDER:			fBorder = (value == -1 ? 1 : MAX(value,0));		break;
		case A_MAXLEVEL:		fMaxLevel = true;								break;
		case A_LEFTOFFSET:		fLeftOffset = value;							break;
		case A_RIGHTOFFSET:		fRightOffset = value;							break;
		case A_GAIN:			fGain = value;									break;
		case A_LEFTCOLOR:		fLeftColor = value;								break;
		case A_RIGHTCOLOR:		fRightColor = value;							break;
		case A_BGCOLOR:			fBGColor = value;								break;
		case A_WIDTH:			fBounds.right = MAX(value, 0);				
								fKnownWidth = MAX(value, 0);					break;
		case A_HEIGHT:			fBounds.bottom = MAX(value, 0);				
								fKnownHeight = MAX(value, 0);					break;
		default:				Image::SetAttribute(attributeID, value, isPercentage);
	}
}


void AudioScope::SetAttributeStr(Attribute attributeID, const char* value)
{	
	switch (attributeID) {		
	case A_ANI:
		fSRC = CopyStringTo(fSRC, value, "Image::fSRC");
		break;
	default:
		Image::SetAttributeStr(attributeID, value);
	}
}
