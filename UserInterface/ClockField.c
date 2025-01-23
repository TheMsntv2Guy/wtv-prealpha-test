// Copyright(c) 1996 Artemis Research, Inc. All rights reserved.

#include "Headers.h"

#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __CLOCKFIELD_H__
#include "ClockField.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif

// =============================================================================

static PackedStyle gClockStyle = {3, 0, 0, 0, 0};

// =============================================================================

ClockField::ClockField()
{
	fCurrentTime = Now();
	fLayer = nil;
	fType = kTimeField;
}

ClockField::~ClockField()
{
}

void ClockField::IClockField(int type)
{
	fType = type;
}

void ClockField::Draw(const Document* document, const Rectangle* invalid)
{
	ContentView* view = document->GetView();
	Rectangle r;
	const char* timeStr = "gClock==nil";
	
	if (gClock != nil) {
		switch(fType) {
			case kTimeField:
				timeStr = gClock->GetTimeString();
				break;
			case kDateField:
				timeStr = gClock->GetDateString();
				break;
		}
	}
	
	GetBounds(&r);
	view->ContentToScreen(&r);

	XFont font = document->GetFont(gClockStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ulong ascent = ::GetFontAscent(font,encoding);
	Color color = document->GetColor(gClockStyle);
	::PaintText(gScreenDevice, font,encoding, timeStr, strlen(timeStr), 
		color, r.left, r.top + ascent, 0, false, invalid);
}

Boolean 
ClockField::Idle(Layer* layer)
{
	Rectangle r;
	GetBounds(&r);
	
	ContentView* view = (ContentView*)layer;
	
	// NOTE: this is a hack to allow other methods to get at
	// the layer. Controls should probably have a layer field
	// which is set when the control is created.
	fLayer = layer;
	if ((gScreen->GetTopLayer() != view))
		return false;
		
	fCurrentTime = Now();	
	if ((fCurrentTime % (60)) == 0) {
		view->ContentToScreen(&r);
		fLayer->InvalidateBounds(&r);
	}
	
	return true;
}

void 
ClockField::Layout(Document* document, Displayable*)
{
	const char* time = "00:00:00 PM";
	const char* date = "XXXXXXXXX, XXXXXXXX 00, 0000";
	ulong textWidth = 0;

	XFont font = document->GetFont(gClockStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ulong ascent = ::GetFontAscent(font,encoding);
	ulong descent = ::GetFontDescent(font,encoding);
	ulong leading = ::GetFontLeading(font,encoding);
	
	switch(fType) {
		case kTimeField:
			textWidth = TextMeasure(gScreenDevice, font,encoding, time, strlen(time));
			break;
		case kDateField:
			textWidth = TextMeasure(gScreenDevice, font,encoding, date, strlen(date));
			break;
	}
	
	SetWidth(textWidth);
	SetHeight(ascent + descent + leading);
}

Boolean	
ClockField::IsSelectable() const
{
	return false;
}
