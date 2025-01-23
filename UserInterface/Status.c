// ===========================================================================
//	Status.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __STATUS_H__
#include "Status.h"
#endif

#include "Animation.h"
#include "ImageData.h"
#include "MemoryManager.h"
#include "PageViewer.h"
#include "System.h"
#ifdef FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif

// ===========================================================================
//	local #defines
// ===========================================================================

#define kStatusXFontFace	gSystem->GetFontProportional()
#define kStatusXFontSize	18
#define kStatusXFontStyle	kShadowStyle

#define kStatusXFont		(GetFont(kStatusXFontFace, kStatusXFontSize, kStatusXFontStyle))
#define kTextLeftEdge		73

static const char kIndicatorMessageTag[] = "Indicator::fMessage";

// =============================================================================

Indicator::Indicator()
{
	fPercentDone = 0;
	fMessage = nil;
	fMessageCopied = false;
	fDisabled = false;
#ifdef DEBUG
	fMessage = "Finding Page...";
#endif
}

Indicator::~Indicator()
{
	if (fMessageCopied && (fMessage != nil)) {	
		FreeTaggedMemory((void*)fMessage, kIndicatorMessageTag);
	}
}

void 
Indicator::Draw(const Rectangle* UNUSED(invalid))
{
	Trespass();
}

#ifdef FIDO_INTERCEPT
void
Indicator::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	Trespass();
}
#endif

void
Indicator::GetPercentDoneBounds(Rectangle* UNUSED(bounds)) const
{
	Trespass();
}

Layer* 
Indicator::GetTopLayer() const
{
	Layer* next = (Layer*)Next();
	
	if (next == nil)
		return nil;
	
	return next->GetTopLayer();
}

void 
Indicator::Hide()
{
	if (!fIsVisible)
		return;
	
	InvalidateBounds();
	
	Layer::Hide();
}

Boolean 
Indicator::IsDisabled() const
{
	return fDisabled;
}

void 
Indicator::SetDisabled(Boolean value)
{
	fDisabled = value;
}

void
Indicator::SetMessage(const char* message, Boolean shouldCopy)
{
	if (fMessage != nil && fMessageCopied) {
		NoteObserving(&fMessage, nil, kIndicatorMessageTag);
		FreeTaggedMemory((void*)fMessage, kIndicatorMessageTag);
	}
	
	if (shouldCopy) {
		CharacterEncoding encoding = GuessCharacterEncoding(message, strlen(message));
		message = (const char*)NewTruncatedStringWithEllipsis(message, kStatusXFont, encoding,
															  fBounds.right - fBounds.left - 4 - kTextLeftEdge,
															  kIndicatorMessageTag);
	}
	
	fMessage = message;
	fMessageCopied = shouldCopy;
	NoteObserving(&fMessage, message, kIndicatorMessageTag);
	
	InvalidateBounds();
}

void
Indicator::SetPercentDone(long percent)
{
	Rectangle bounds;

	if (percent < 0)
		return;
	
	if (percent > 100)
		percent = 100;
		
	fPercentDone = percent;
	GetPercentDoneBounds(&bounds);
	InvalidateBounds(&bounds);
}

void 
Indicator::SetTarget(const Resource* resource)
{
	fTargetResource = *resource;
	fPercentDone = 0;
	InvalidateBounds();
}

void 
Indicator::Show()
{
	if (fDisabled)
		return;
	
	Layer::Show();
}

// ===========================================================================

StatusIndicator::StatusIndicator()
{
	IsError(gStatusIndicator != nil);
	gStatusIndicator = this;

	fBackground = ImageData::NewImageData("file://ROM/Images/StatusBackground.gif");
	IsError(fBackground == nil);
	if (!IsError(fBackground == nil)) {
		fBackground->GetBounds(&fBounds);
		fBackground->SetFilterType(kNoFilter);
		IsError(fBounds.top != 0 || fBounds.left != 0);
		OffsetRectangle(fBounds, 10, 10);
	}
	
	fImages = new(Animation);
	if (!IsError(fImages == nil)) {
		// Set up the progress Animation.
		fImages->SetAttributeStr(A_ANI, "file://ROM/Animations/Globes2.ani");
		fImages->Load(nil);
		fImages->Idle(this); // Idle once to load images in low memory and capture size.
		fImages->Layout(nil, nil);
		fImages->LayoutComplete(nil, nil);
		fImages->SetLeft(fBounds.left + 11);
		fImages->SetTop(fBounds.top + 7);
		fImages->SetShouldLoop(true);
	}

	fLastIdle = 0;
}

StatusIndicator::~StatusIndicator()
{
	IsError(gStatusIndicator != this);
	gStatusIndicator = nil;

	if (fBackground != nil) {	
		delete(fBackground);
		fBackground = nil;
	}

	if (fImages != nil) {	
		delete(fImages);
		fImages = nil;
	}

}

void 
StatusIndicator::Draw(const Rectangle* invalid)
{
	TrivialMessage(("Status Indicator::Draw w/ %d%%", fPercentDone));
	
	Rectangle		bounds;
	fBackground->GetBounds(&bounds);
	IsError(bounds.top != 0 || bounds.left != 0);
	OffsetRectangle(bounds, fBounds.left, fBounds.top);

	if (fDrawShadowOnShow) {
		// draw background w/ drop shadow
		Rectangle		shadowBounds;
		
		// first draw the right shadow
		shadowBounds.top = bounds.top + 6;
		shadowBounds.left = bounds.right;
		shadowBounds.bottom = bounds.bottom;
		shadowBounds.right = bounds.right + 6;
		PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
		
		// then draw the bottom shadow
		shadowBounds.top = bounds.bottom;
		shadowBounds.left = bounds.left + 6;
		shadowBounds.bottom = bounds.bottom + 6;
		shadowBounds.right = bounds.right + 6;
		PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
		
		fDrawShadowOnShow = false;
	}

	fBackground->Draw(&bounds, invalid);
	
	// paint the proper globe
	fImages->Draw(nil, invalid);
	
	// percentange done...	
	GetPercentDoneBounds(&bounds);	
	InsetRectangle(bounds, 2, 2);
	bounds.right = bounds.left + RectangleWidth(bounds) * fPercentDone / 100;
	PaintRectangle(gScreenDevice, bounds, kGreenColor, 0, invalid);

	// message...
	
	if (fMessage != nil) {
		ulong	saveBackgroundColor = gPageBackColor;
		
		gPageBackColor = fBackground->AverageColor(0);
		CharacterEncoding encoding = GuessCharacterEncoding(fMessage,strlen(fMessage));
		
		PaintText(gScreenDevice, kStatusXFont,encoding,  fMessage, strlen(fMessage),
			kAmberColor, fBounds.left + kTextLeftEdge, bounds.bottom + 30, 0, false, invalid);
		gPageBackColor = saveBackgroundColor;
	}
}

#ifdef FIDO_INTERCEPT
void
StatusIndicator::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle		bounds;
	fBackground->GetBounds(&bounds);
	IsError(bounds.top != 0 || bounds.left != 0);
	OffsetRectangle(bounds, fBounds.left, fBounds.top);
	
	if (fDrawShadowOnShow)
	{
		Rectangle		shadowBounds;
		
		// first draw the right shadow
		shadowBounds.top = bounds.top + 6;
		shadowBounds.left = bounds.right;
		shadowBounds.bottom = bounds.bottom;
		shadowBounds.right = bounds.right + 6;
		fidoCompatibility.PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
		
		// then draw the bottom shadow
		shadowBounds.top = bounds.bottom;
		shadowBounds.left = bounds.left + 6;
		shadowBounds.bottom = bounds.bottom + 6;
		shadowBounds.right = bounds.right + 6;
		fidoCompatibility.PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
		
		fDrawShadowOnShow = false;
	}

	fBackground->Draw(&bounds, 0, fidoCompatibility);
	
	// paint the proper globe
	fImages->Draw(nil, fidoCompatibility);
	
	// percentange done...	
	GetPercentDoneBounds(&bounds);	
	InsetRectangle(bounds, 2, 2);
	bounds.right = bounds.left + RectangleWidth(bounds) * fPercentDone / 100;
	fidoCompatibility.PaintRectangle(gScreenDevice, bounds, kGreenColor, 0, nil);

	// message...
	
	if (fMessage != nil) {
		fidoCompatibility.PaintText(gScreenDevice, kStatusXFont, fMessage, strlen(fMessage),
			kAmberColor, fBounds.left + kTextLeftEdge, bounds.bottom + 30, 0, false, nil);
	}
}
#endif

void
StatusIndicator::GetPercentDoneBounds(Rectangle* bounds) const
{
	bounds->top = fBounds.top + 12;
	bounds->left = fBounds.left + 76;
	bounds->bottom = fBounds.top + 23;
	bounds->right = fBounds.left + 231;
}

void 
StatusIndicator::Hide()
{
	if (!fIsVisible)
		return;
		
	// Invalidate shadow.
	Rectangle bounds = fBounds;
	OffsetRectangle(bounds, 6, 6);
	InvalidateBehind(&bounds);
	fBackground->SetKeepBitMap(kNoBitMap);
	Indicator::Hide();
}

void 
StatusIndicator::Idle()
{
	if (!fIsVisible)
		return;
		
	ulong	now = Now();
	
	if (fImages)
		fImages->Idle(this);
			
	long	delta = now - fLastIdle;
	if (fLastIdle != 0 && delta < (kOneSecond/10)) 
		return;
	
	fLastIdle = now;
		
	// guarantee monotonically increasing, but <= 100
	uchar	newPercentDone = fPercentDone;
	if (gPageViewer->GetDocument()  != nil &&
			fTargetResource == *gPageViewer->GetDocument()->GetResource())
		newPercentDone = gPageViewer->GetPercentComplete();
	if (newPercentDone < fPercentDone)
		newPercentDone = fPercentDone;
	if (IsWarning(newPercentDone > 100))
		newPercentDone = 100;
		
	if (newPercentDone != fPercentDone) {
		Rectangle	bounds;
		
		fPercentDone = newPercentDone;
		GetPercentDoneBounds(&bounds);	
		InvalidateBounds(&bounds);
	}
}

void 
StatusIndicator::Show()
{
	if (fDisabled /* || fIsVisible*/)
		return;
	
	fBackground->SetKeepBitMap(kNormalBitMap);
	if (!fIsVisible) {
		fDrawShadowOnShow = true;
	}
	
	Indicator::Show();
}

// =============================================================================

ConnectIndicator::ConnectIndicator()
{
	IsError(gConnectIndicator != nil);
	gConnectIndicator = this;

	SetRectangle(fBounds, 10, gScreen->GetHeight() - 100, gScreen->GetWidth() - 290, gScreen->GetHeight());
}

ConnectIndicator::~ConnectIndicator()
{
	IsError(gConnectIndicator != this);
	gConnectIndicator = nil;

	if (fMessageCopied && (fMessage != nil)) {	
		FreeTaggedMemory((void*)fMessage, kIndicatorMessageTag);
	}
}

void 
ConnectIndicator::Draw(const Rectangle* invalid)
{
	TrivialMessage(("Status Indicator::Draw w/ %d%%", fPercentDone));

	PaintRectangle(gScreenDevice, fBounds, kBlackColor, 0, invalid);
	
	Rectangle bounds;
	bounds = fBounds;

	OffsetRectangle(bounds, fBounds.left, fBounds.top);

	// percentange done...	
	GetPercentDoneBounds(&bounds);

	// indicator frame
	Rectangle frameBounds;
	frameBounds = bounds;
	
	InsetRectangle(frameBounds, 1, 1);	
	PaintRectangle(gScreenDevice, frameBounds, 0x00222222, 0, invalid);
	
	InsetRectangle(bounds, 2, 2);
	bounds.right = bounds.left + RectangleWidth(bounds) * fPercentDone / 100;
	PaintRectangle(gScreenDevice, bounds, kGreenColor, 0, invalid);

	// message...
	
	if (fMessage != nil) {
		gPageBackColor = kBlackColor;
		CharacterEncoding encoding = GuessCharacterEncoding(fMessage,strlen(fMessage));
		PaintText(gScreenDevice, kStatusXFont,encoding,  fMessage, strlen(fMessage),
			0x00505050, fBounds.left + 20, fBounds.top + 30, 0, false, invalid);
	}
}

#ifdef FIDO_INTERCEPT
void
ConnectIndicator::Draw(class FidoCompatibilityState& UNUSED(fidoCompatibility)) const
{
}
#endif

void
ConnectIndicator::GetPercentDoneBounds(Rectangle* bounds) const
{
	bounds->top = fBounds.top + 40;
	bounds->left = fBounds.left + 20;
	bounds->bottom = fBounds.top + 51;
	bounds->right = fBounds.left + 220;
}
