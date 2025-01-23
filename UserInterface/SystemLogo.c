// ===========================================================================
//	SystemLogo.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __SYSTEMLOGO_H__
#include "SystemLogo.h"
#endif

#include "ImageData.h"
#include "MemoryManager.h"
#include "Screen.h"

// =============================================================================

SystemLogo::SystemLogo()
{
	IsError(gSystemLogo != nil);
	gSystemLogo = this;

	Rectangle imageBounds;	

	fLogoImage = ImageData::NewImageData("file://ROM/images/logo.gif");
	fLogoImage->GetBounds(&imageBounds);
	fBounds.left = 435;
	fBounds.top = 330;
	fBounds.right = fBounds.left + imageBounds.right;
	fBounds.bottom = fBounds.top + imageBounds.bottom;

	fPartiallyTransparent = true;
}

SystemLogo::~SystemLogo()
{
	IsError(gSystemLogo != this);
	gSystemLogo = nil;

	if (fLogoImage != nil) {
		delete(fLogoImage);
		fLogoImage = nil;
	}
}

Layer* 
SystemLogo::GetTopLayer() const
{
	Layer* next = (Layer*)Next();
	
	if (next == nil)
		return nil;
	
	return next->GetTopLayer();
}

#if 1
void 
SystemLogo::Draw(const Rectangle* invalid)
{
	if (!fIsVisible)
		return;

	Rectangle logoBounds;

	GetBounds(&logoBounds);
	fLogoImage->Draw(&logoBounds, invalid, fCurrentLogoTransparency);
}


#ifdef FIDO_INTERCEPT
void SystemLogo::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	if (!fIsVisible)
		return;

	Rectangle logoBounds;

	GetBounds(&logoBounds);
	fLogoImage->Draw(&logoBounds, fCurrentLogoTransparency, fidoCompatibility);
}
#endif
#else
void SystemLogo::Draw(const Rectangle* UNUSED(invalid))
{
}

#ifdef FIDO_INTERCEPT
void SystemLogo::Draw(class FidoCompatibilityState& UNUSED(fidoCompatibility)) const
{
}
#endif
#endif

void 
SystemLogo::StartFading()
{
	// always draw for now	
	fCurrentLogoTransparency = 0x60;
	fFadeStartTime = Now();
	Show();
	fLogoImage->SetKeepBitMap(kNormalBitMap);
	InvalidateBounds();
}

void 
SystemLogo::Idle()
{
	if (!fIsVisible)
		return;

	ulong deltaTime = Now() - fFadeStartTime;
	if (deltaTime >= 6 * kOneSecond) {
		fLogoImage->SetKeepBitMap(kNoBitMap);
		Hide();
	}
	else if (deltaTime > 5 * kOneSecond) {
		ulong	newTransparency = 0x60 + (deltaTime - 5 * kOneSecond)*0x9f/(kOneSecond);// /8*8;
		if (newTransparency != fCurrentLogoTransparency) {
			fCurrentLogoTransparency = newTransparency;
			InvalidateBounds();
		}
	}
}

// =============================================================================

