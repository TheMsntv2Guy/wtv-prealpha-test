// ===========================================================================
//	OptionsPanel.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __OPTIONSPANEL_H__
#include "OptionsPanel.h"
#endif
#ifndef __TOURIST_H__
#include "Tourist.h"
#endif
#ifdef FOR_MAC
	#include "MacintoshUtilities.h" /* for CapsLockKeyDown */
#endif

#include "Animation.h"
#include "BoxUtils.h"
#include "ImageData.h"
#include "MemoryManager.h"
#include "Network.h"
#include "PageViewer.h"
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#include "Sound.h"
#include "SongData.h"
#include "System.h"
#include "SystemLogo.h"
#ifdef FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif
#ifdef HARDWARE
#include "TellyIO.h"
#endif /* HARDWARE */

// ===========================================================================
//	consts/#defines
// ===========================================================================

#define kTitleXFontFace				gSystem->GetFontProportional()
#define kTitleXFontSize				18
#define kTitleXFontStyle			kShadowStyle
#define kTitleXFont					(GetFont(kTitleXFontFace, kTitleXFontSize, kTitleXFontStyle))

#define kOdometerXFontFace			gSystem->GetFontProportional()
#define kOdometerXFontSize			12
#define kOdometerXFontStyle			0
#define kOdometerXFont				(GetFont( kOdometerXFontFace,  kOdometerXFontSize,  kOdometerXFontStyle))

const ulong	kTextLeftEdge = 7;
const ulong kTitleHMargin = 12;

#ifndef FOR_MAC
const ulong kOptionPanelScrollRate = 3;
const ulong kOptionPanelCloseScrollRate = 2;
#else
const ulong kOptionPanelScrollRate = 1;
const ulong kOptionPanelCloseScrollRate = 1;
#endif

#ifdef EXTERNAL_BUILD
#define kTitleTextColor kAmberColor
#else
#define kTitleTextColor kLightGrayColor
#endif

// ===========================================================================
//    implementation
// ===========================================================================

OptionsPanel::OptionsPanel()
{
	IsError(gOptionsPanel != nil);
	gOptionsPanel = this;

	fShouldPaintBackground = false;
	fRefreshSelections = true;
	fPercentDone = 0;
	fInProgress = false;
	fLastIdle = 0;
	fIsCapsLocked = false;
	fShown = true;
	fNoHWrapSelection = true;
	
	// Note: should rename to OptionBar.gif
	fOptionsBar = ImageData::NewImageData("file://ROM/Images/OptionsPanel.gif");
	fOptionsBar->SetFilterType(kNoFilter);
	fOptionsDrawer = ImageData::NewImageData("file://ROM/Images/Options/OptionsDrawer.gif");
	fOptionsDrawer->SetFilterType(kNoFilter);
	fAdvancedBar = ImageData::NewImageData("file://ROM/Images/Options/AdvancedBar.gif");
	fAdvancedBar->SetFilterType(kNoFilter);
	fPhoneInProgress = new(Animation);	
	fTitleField = ImageData::NewImageData("file://ROM/Images/Options/TitleField.gif");
	fTitleField->SetFilterType(kNoFilter);
	fPhoneConnected = ImageData::NewImageData("file://ROM/Images/PhoneLine/phone.0.gif");
	fPhoneDisconnected = ImageData::NewImageData("file://ROM/Images/PhoneLine/phone.none.gif");
	fScrollArrows = ImageData::NewImageData("file://ROM/Images/ScrollArrows.gif");
	fScrollDownLight = ImageData::NewImageData("file://ROM/Images/ScrollDownLight.gif");
	fScrollUpLight = ImageData::NewImageData("file://ROM/Images/ScrollUpLight.gif");
	fCapsLock = ImageData::NewImageData("file://ROM/Images/CapsLock.gif");
	
	IsError(fOptionsBar == nil);
	IsError(fAdvancedBar == nil);
	IsError(fOptionsDrawer == nil);
	
	IsError(fTitleField == nil);
	IsError(fPhoneInProgress == nil);
	IsError(fPhoneDisconnected == nil);
	IsError(fScrollArrows == nil);
	IsError(fScrollDownLight == nil);
	IsError(fScrollUpLight == nil);
	IsError(fCapsLock == nil);
	
	SetURL(kOptionsPanelURL);
	ExecuteURL(kOptionsPanelURL, nil);

	// Set up the phone progress Animation.
	fPhoneInProgress->SetAttributeStr(A_ANI, "file://ROM/Animations/PhoneLine.ani");
	fPhoneInProgress->Load(&fResource);
	fPhoneInProgress->Idle(this); // Idle once to load images in low memory.
	fPhoneInProgress->Layout(nil, nil);
	fPhoneInProgress->LayoutComplete(nil, nil);
	fPhoneInProgress->SetLeft(510);
	fPhoneInProgress->SetTop(5);
	fPhoneInProgress->SetShouldLoop(true);
	
	// set bounds	
	long screenHeight = gScreen->GetHeight();
	Rectangle barBounds;
	
	fOptionsBar->GetBounds(&barBounds);
	
	IsError(fBounds.top != 0 || fBounds.left != 0);
	
	fBounds.left = barBounds.left;
	fBounds.right = barBounds.right;	
	fBounds.top = screenHeight - barBounds.bottom;
	fBounds.bottom = screenHeight;
}

OptionsPanel::~OptionsPanel()
{
	IsError(gOptionsPanel != this);
	gOptionsPanel = nil;

	if (fOptionsBar != nil) {	
		delete(fOptionsBar);
		fOptionsBar = nil;
	}
	if (fAdvancedBar != nil) {
		delete(fAdvancedBar);
		fAdvancedBar = nil;
	}
	if (fOptionsDrawer != nil) {	
		delete(fOptionsDrawer);
		fOptionsDrawer = nil;
	}
	
	if (fTitleField != nil) {	
		delete(fTitleField);
		fTitleField = nil;
	}
	if (fPhoneConnected != nil) {	
		delete(fPhoneConnected);
		fPhoneConnected = nil;
	}
	if (fPhoneDisconnected != nil) {	
		delete(fPhoneDisconnected);
		fPhoneDisconnected = nil;
	}
	if (fPhoneInProgress != nil) {	
		delete(fPhoneInProgress);
		fPhoneInProgress = nil;
	}
	if (fScrollArrows != nil) {	
		delete(fScrollArrows);
		fScrollArrows = nil;
	}
	if (fScrollDownLight != nil) {	
		delete(fScrollDownLight);
		fScrollDownLight = nil;
	}
	if (fScrollUpLight != nil) {	
		delete(fScrollUpLight);
		fScrollUpLight = nil;
	}
	if (fCapsLock != nil) {	
		delete(fCapsLock);
		fCapsLock = nil;
	}
	
	if(fMessage) {
		FreeTaggedMemory((void*)fMessage, "StatusIndicator::fMessage");
		fMessage = nil;
	}
}

void
OptionsPanel::SetIsPhoneInProgress(Boolean state)
{
	fIsPhoneInProgress = state;
}

Boolean
OptionsPanel::GetIsPhoneInProgress()
{
	return fIsPhoneInProgress;
}

Boolean
OptionsPanel::BackInput()
{
	// This is overridden so the closing sound can be played. We can't
	// play the sound in close because that is called from other places,
	// and would result in conflicting sounds being played.
	
	if (!fIsVisible)
		return false;

	gScroll1Sound->Play();
	return Panel::BackInput();
}

void 
OptionsPanel::ChangeIdleState(ViewIdleState newState, ViewIdleState oldState)
{
	ContentView::ChangeIdleState(newState, oldState);
}

void 
OptionsPanel::Close(Boolean)
{
	if (!IsOpen())
		return;
		
	Rectangle barBounds;
	Rectangle advancedBounds;
	Rectangle drawerBounds;
	Rectangle scrollBounds = fBounds;
	Ordinate scrollHeight;

	fOptionsBar->GetBounds(&barBounds);
	fOptionsDrawer->GetBounds(&drawerBounds);
	scrollHeight = RectangleHeight(drawerBounds);

	if (fShowAdvanced) {
		fAdvancedBar->GetBounds(&advancedBounds);
	 	scrollHeight += RectangleHeight(advancedBounds);
	 }
	 	
	if (!fShown)
		scrollHeight += RectangleHeight(barBounds);

	// Update page viewer where the options panel used to be...
	Rectangle	pageBounds;
	gPageViewer->GetBounds(&pageBounds);
	pageBounds.bottom += scrollHeight;
	gPageViewer->SetBounds(&pageBounds);
	pageBounds.top = fBounds.top - 2;	// Clean up flick filter also.
	gPageViewer->InvalidateBounds(&pageBounds);
	
	fBounds.top += scrollHeight;

	DrawBehind();

	// Clean up flicker filtering before sliding down.
	pageBounds.bottom = pageBounds.top + 2;
	CopyImage(gScreenDevice, gOnScreenDevice, pageBounds, pageBounds);

	gScreen->SlideAreaDown(&scrollBounds, kOptionPanelCloseScrollRate, scrollHeight);

	if (fRefreshSelections)
		gPageViewer->RefreshSelection();

	InvalidateBounds();
	SetIsOpen(false);
}

#ifdef DEBUG_TOURIST
void
OptionsPanel::DrawOdometer(const Rectangle* invalid)
{
	Rectangle bounds;
	fOptionsBar->GetBounds(&bounds);

	char	buffer[12]; /* 11 for %ld, 1 for NULL */
	int		bufferLength = snprintf(buffer, sizeof(buffer),
								"%ld", (long)gPageViewer->GetPageCount());

	Assert(bufferLength < sizeof(buffer));
	PaintText(gScreenDevice, kOdometerXFont, kUSASCII, buffer, bufferLength,
		kGreenColor, fBounds.left + 424, fBounds.top + 24, 0, true, invalid);
}

#ifdef FIDO_INTERCEPT
void
OptionsPanel::DrawOdometer(class FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle bounds;
	fOptionsBar->GetBounds(&bounds);

	char	buffer[12]; /* 11 for %ld, 1 for NULL */
	int		bufferLength = snprintf(buffer, sizeof(buffer),
								"%ld", (long)gPageViewer->GetPageCount());

	Assert(bufferLength < sizeof(buffer));
	fidoCompatibility.PaintText(gScreenDevice, kOdometerXFont, buffer, bufferLength,
		kRedColor, fBounds.left + 424, fBounds.top + 24, 0, true, nil);
}
#endif /* FIDO_INTERCEPT */
#endif /* DEBUG_TOURIST */

void 
OptionsPanel::Draw(const Rectangle* invalid)
{
	PushDebugChildURL("<OptionsPanel::Draw>");

	Rectangle bounds;

	// Draw the options bar first...	
	// Special case optimization: If invalid area is contained by the PhoneInProgress image,
	// don't draw the options bar.
	fPhoneInProgress->GetBoundsTight(&bounds);
	ContentToScreen(&bounds);
	if (invalid == nil ||
		invalid->left < bounds.left || invalid->right > bounds.right ||
		invalid->top < bounds.top || invalid->bottom > bounds.bottom) {
		fOptionsBar->GetBounds(&bounds);
		IsError(bounds.top != 0 || bounds.left != 0);
		OffsetRectangle(bounds, fBounds.left, fBounds.top);	
		fOptionsBar->Draw(&bounds, invalid);
	}
	
	// Draw the title field...	
	fTitleField->GetBounds(&bounds);
	IsError(bounds.top != 0 || bounds.left != 0);
	
	OffsetRectangle(bounds, fBounds.left + 7, fBounds.top + 7);
	if (invalid == nil || RectanglesIntersect(&bounds, invalid)) {
		fTitleField->Draw(&bounds, invalid);
				
		if (fInProgress)
			DrawProgress(invalid);
			
		DrawTitle(invalid);
	}

	if (fIsCapsLocked) {
		fCapsLock->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 430, fBounds.top + 15);
		fCapsLock->Draw(&bounds, invalid);
	}		

	if (!IsOpen())
		DrawScrollArrows(invalid);

#ifdef DEBUG_TOURIST
	if (gTourist != nil && gTourist->GetIsActive())
		DrawOdometer(invalid);
#endif
	
	// Draw the phone connection status...
	if (gNetwork->IsActive()) {
		fPhoneInProgress->Draw(fDocument, invalid);
	} else {
		(void)fPhoneDisconnected->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 510, fBounds.top + 5);
		fPhoneDisconnected->Draw(&bounds, invalid);
	}
	
	// Only draw the drawer if it is open...
	if (IsOpen())
		ContentView::Draw(invalid);
	
	PopDebugChildURL();
}

#ifdef FIDO_INTERCEPT
void 
OptionsPanel::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle bounds;

	// Draw the options bar first...	
	fOptionsBar->GetBounds(&bounds);
	IsError(bounds.top != 0 || bounds.left != 0);
	OffsetRectangle(bounds, fBounds.left, fBounds.top);
	fOptionsBar->Draw(&bounds, 0, fidoCompatibility);

	// Draw the title field...	
	fTitleField->GetBounds(&bounds);
	IsError(bounds.top != 0 || bounds.left != 0);
	
	OffsetRectangle(bounds, fBounds.left + 7, fBounds.top + 7);
	{
		fTitleField->Draw(&bounds, 0, fidoCompatibility);
		
		if (fInProgress)
			DrawProgress(fidoCompatibility);
			
		DrawTitle(fidoCompatibility);
	}
	
	if (fIsCapsLocked) {
		fCapsLock->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 430, fBounds.top + 15);
		fCapsLock->Draw(&bounds, 0, fidoCompatibility);
	}		

	// NOTE: Scroll arrows could be shown as disabled on the titlebar if
	// this check is removed. In order to make this work correctly, the
	// DrawScrollArrows  code must be changed to use some stored variables
	// that record whether scroll up/down was possible in the pageviewer
	// before the options panel was opened.
	
	if (!IsOpen())	
		DrawScrollArrows(fidoCompatibility);
	
#ifdef DEBUG_TOURIST
	if (gTourist != nil && gTourist->GetIsActive())
		DrawOdometer(fidoCompatibility);
#endif
	
	// Draw the phone connection status...
	if (gNetwork->IsActive()) {
		fPhoneInProgress->Draw(fDocument, fidoCompatibility);
	} else {
		(void)fPhoneDisconnected->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 510, fBounds.top + 5);
		fPhoneDisconnected->Draw(&bounds, 0, fidoCompatibility);
	}
	
	// Only draw the drawer if it is open...
	if (IsOpen())
		ContentView::Draw(fidoCompatibility);
}
#endif

void 
OptionsPanel::DrawScrollArrows(const Rectangle* invalid)
{
	Rectangle bounds;

	// If we can scroll in either direction, we show both scroll arrows. We
	// only light up the direction(s) that can be scrolled.
	
	if (gPageViewer->CanScrollDown() || gPageViewer->CanScrollUp()) {	
		(void)fScrollArrows->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 450, fBounds.top + 10);
		fScrollArrows->Draw(&bounds, invalid);
	}			

	// Scroll arrows are not lit when any panel is open.
	if (IsOpen() || (gScreen->GetCurrentPanel() != nil && gScreen->GetCurrentPanel()->IsOpen()))
		return;
		
	if (gPageViewer->CanScrollDown()) {
		(void)fScrollDownLight->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 450, fBounds.top + 10);
		fScrollDownLight->Draw(&bounds, invalid);
	}
	
	if (gPageViewer->CanScrollUp()) {
		(void)fScrollUpLight->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 475, fBounds.top + 10);
		fScrollUpLight->Draw(&bounds, invalid);
	}
}

#ifdef FIDO_INTERCEPT
void 
OptionsPanel::DrawScrollArrows(class FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle bounds;

	// If we can scroll in either direction, we show both scroll arrows. We
	// only light up the direction(s) that can be scrolled.
	
	if (gPageViewer->CanScrollDown() || gPageViewer->CanScrollUp()) {	
		(void)fScrollArrows->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 450, fBounds.top + 10);
		fScrollArrows->Draw(&bounds, 0, fidoCompatibility);
	}			

	if (gPageViewer->CanScrollDown()) {
		(void)fScrollDownLight->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 450, fBounds.top + 10);
		fScrollDownLight->Draw(&bounds, 0, fidoCompatibility);
	}
	
	if (gPageViewer->CanScrollUp()) {
		(void)fScrollUpLight->GetBounds(&bounds);
		OffsetRectangle(bounds, fBounds.left + 475, fBounds.top + 10);
		fScrollUpLight->Draw(&bounds, 0, fidoCompatibility);
	}	
}
#endif

void 
OptionsPanel::DrawTitle(const Rectangle* invalid)
{
	Document* document;
	const char* message;

	if (fInProgress && fMessage != nil)
		message = fMessage;
		
	else if ((document=gPageViewer->GetDocument()) != nil)
		message = document->GetTitle();
	else
		return;
	
	if (message == nil)
		return;
	
	Rectangle	bounds;
	CharacterEncoding encoding = GuessCharacterEncoding(message,strlen(message));
	fTitleField->GetBounds(&bounds);
	char*	banner = NewTruncatedStringWithEllipsis(message, kTitleXFont,encoding, 
											bounds.right - bounds.left - kTitleHMargin*2,
											"OptionsPanel");
	
	PaintText(gScreenDevice, kTitleXFont, encoding, banner, strlen(banner),
		kTitleTextColor, fBounds.left + kTitleHMargin, fBounds.top + 24, 0, false, invalid);
	FreeTaggedMemory(banner, "OptionsPanel");
}

#ifdef FIDO_INTERCEPT
void 
OptionsPanel::DrawTitle(class FidoCompatibilityState& fidoCompatibility) const
{
	Document* document;
	const char* message;

	if (fInProgress && fMessage != nil)
		message = fMessage;
		
	else if ((document=gPageViewer->GetDocument()) != nil)
		message = document->GetTitle();
	else
		return;
	
	if (message == nil)
		return;
	
	Rectangle	bounds;
	CharacterEncoding encoding = GuessCharacterEncoding(message,strlen(message));
	fTitleField->GetBounds(&bounds);
	char*	banner = NewTruncatedStringWithEllipsis(message, kTitleXFont, encoding, 
											bounds.right - bounds.left - kTitleHMargin*2,
											"OptionsPanel");
	
	fidoCompatibility.PaintText(gScreenDevice, kTitleXFont, banner, strlen(banner),
		kTitleTextColor, fBounds.left + kTitleHMargin, fBounds.top + 24, 0, false, nil);
	FreeTaggedMemory(banner, "OptionsPanel");
}
#endif

void 
OptionsPanel::DrawProgress(const Rectangle* invalid)
{
	Rectangle bounds;

	bounds.top = fBounds.top + 12;
	bounds.left = fBounds.left + 74;
	bounds.bottom = fBounds.top + 23;
	bounds.right = fBounds.left + 209;
	
	OffsetRectangle(bounds, 180, -1);
	
	InsetRectangle(bounds, 2, 2);
	bounds.right = bounds.left + RectangleWidth(bounds) * fPercentDone / 100;
	PaintRectangle(gScreenDevice, bounds, kGreenColor, 0, invalid);
}

#ifdef FIDO_INTERCEPT
void 
OptionsPanel::DrawProgress(class FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle bounds;

	bounds.top = fBounds.top + 12;
	bounds.left = fBounds.left + 74;
	bounds.bottom = fBounds.top + 23;
	bounds.right = fBounds.left + 209;
	
	OffsetRectangle(bounds, 180, -1);
	
	InsetRectangle(bounds, 2, 2);
	bounds.right = bounds.left + RectangleWidth(bounds) * fPercentDone / 100;
	fidoCompatibility.PaintRectangle(gScreenDevice, bounds, kGreenColor, 0, nil);
}

#endif

Boolean 
OptionsPanel::ExecuteInput()
{
	if (!IsOpen() || fDocument == nil)
		return false;
	
	Displayable* current = fDocument->GetSelectable(fCurrentSelection);
	if (current == nil || !current->HasURL())
		return false;

	fRefreshSelections = false;	
	Close();
	fRefreshSelections = true;
	gScreen->RedrawNow();
	
	char*	url = current->NewURL(&fMapCursorPosition, "URL");	
	if (url == nil)
		return false;
	
	gPageViewer->ExecuteURL(url, nil);
		
	FreeTaggedMemory(url, "URL");		
	return true;
}

Layer* 
OptionsPanel::GetTopLayer() const
{
	if (IsOpen())
		return (Layer*)this;
		
	Layer* next = (Layer*)Next();
	
	if (next == nil)
		return nil;
	
	return next->GetTopLayer();
}

void 
OptionsPanel::Idle()
{
	PerfDump perfdump("OptionsPanel::Idle");

	// Allow animation to advance.
	if (GetIsPhoneInProgress() && gSystem->GetIsOn())
		fPhoneInProgress->Idle(this);

	ContentView::Idle();
	
	// Update caps lock.	
	if (CapsLockKeyDown() != fIsCapsLocked) {
		fIsCapsLocked = !fIsCapsLocked;
		InvalidateBounds();
	}
	
	if (!fInProgress)
		return;
		
	ulong	now = Now();
	
	long delta = now - fLastIdle;
	if (fLastIdle != 0 && delta < (kOneSecond/10)) 
		return;
		
	TrivialMessage(("Status @ 0x%x", now));

	// guarantee monotonically increasing, but <= 100
	uchar oldPercentDone = fPercentDone;
	
	if (gPageViewer->GetDocument()  != nil &&
			fTargetResource == *gPageViewer->GetDocument()->GetResource())
		fPercentDone = gPageViewer->GetPercentComplete();
		
	if (fPercentDone < oldPercentDone)
		fPercentDone = oldPercentDone;
	IsError(oldPercentDone > 100);
	
	if (fPercentDone > 100)
		fPercentDone = 100;
	
	PostulateFinal(false);
	InvalidateBounds();
	
	fLastIdle = now;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean
OptionsPanel::ShowAdvanced() const
{
	return fShowAdvanced;
}
#endif

Boolean
OptionsPanel::IsShown() const
{
	return fShown;
}

void 
OptionsPanel::Open()
{
	Rectangle barBounds;
	Rectangle drawerBounds;
	Ordinate scrollHeight;

	gPageViewer->RefreshSelection();

	fOptionsDrawer->GetBounds(&drawerBounds);
	fOptionsBar->GetBounds(&barBounds);
	
	scrollHeight = RectangleHeight(drawerBounds);
	
	if (fShowAdvanced) {
		Rectangle advancedBounds;
		fAdvancedBar->GetBounds(&advancedBounds);
		scrollHeight += RectangleHeight(advancedBounds);
	}
	
	if (!fShown)
		scrollHeight += RectangleHeight(barBounds);

	fBounds.top -= scrollHeight;
	
	Rectangle	pageBounds;
	gPageViewer->GetBounds(&pageBounds);
	pageBounds.bottom = fBounds.top;
	gPageViewer->SetBounds(&pageBounds);
	
	// This view is not visible when first built, so no selection is set.
	if (SelectionsEqual(fCurrentSelection, kInvalidSelection))
		SetCurrentSelection(NextVisibleAnchor(kInvalidSelection));
		
	// If an advanced option was last set, use the default selection.
	else {
		Rectangle visibleBounds;
		Rectangle currentBounds;
		
		fDocument->GetSelectable(fCurrentSelection)->GetBounds(&currentBounds);
		VisibleContentBounds(visibleBounds);
		
		if (currentBounds.top >= visibleBounds.bottom)
			SetCurrentSelection(NextVisibleAnchor(kInvalidSelection));
	}


	InvalidateBounds();
	SetIsOpen(true);

	Draw(nil);
	gScreen->SlideAreaUp(&fBounds, kOptionPanelScrollRate, 
						 scrollHeight, 
						 RectangleHeight(fBounds) - scrollHeight);
}

void 
OptionsPanel::PhoneStatusChanged()
{
	Rectangle bounds;
	
	fPhoneInProgress->GetBounds(&bounds);
	ContentToScreen(&bounds);
	InvalidateBounds(&bounds);
}

void
OptionsPanel::SetShowAdvanced(Boolean value)
{
	fShowAdvanced = value;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
OptionsPanel::SetMessage(const char* message, Boolean shouldCopy)
{
	if (fMessage != nil && fMessageCopied) {
		NoteObserving(&fMessage, nil, "StatusIndicator::fMessage");
		FreeTaggedMemory((void*)fMessage, "StatusIndicator::fMessage");
	}
		
	if (shouldCopy) {
		message = (const char*)CopyString(message, "StatusIndicator::fMessage");
		ulong		length = strlen(message);
		ulong		proposedLength;
		Rectangle	bounds;
		Boolean		backedUp = false;
		
		GetBounds(&bounds);
		CharacterEncoding encoding = GuessCharacterEncoding(message,strlen(message));
		while (TextMeasure(gScreenDevice, kTitleXFont, encoding, message, length) > 
							RectangleWidth(bounds) - 4 - kTextLeftEdge) {	
			length--; 
			backedUp = true; 
		}
		if (backedUp) {
			/* just pick some min size to look backwards for space, so we break cleanly */
			for (proposedLength = length; proposedLength > 4; proposedLength--)
				if (message[proposedLength-1] == ' ')
					break;
			length = proposedLength;
		}
		
		((char*)message)[length] = 0;
	}
	
	fMessage = message;
	fMessageCopied = shouldCopy;
	NoteObserving(&fMessage, message, "StatusIndicator::fMessage");
}
#endif

void 
OptionsPanel::ScrollStatusChanged()
{
	Rectangle	bounds;
	
	fScrollArrows->GetBounds(&bounds);
	OffsetRectangle(bounds, fBounds.left + 450, fBounds.top + 10);
	InvalidateBounds(&bounds);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
OptionsPanel::SetPercentDone(ulong percent)
{
	fPercentDone = percent;
}
#endif

void 
OptionsPanel::SetShown(Boolean show)
{
	if (fShown == show)
		return;

	fShown = show;
		
	if (fShown) {
		Rectangle	barBounds;
		fOptionsBar->GetBounds(&barBounds);
		fBounds.top = gScreen->GetHeight() - barBounds.bottom;
		InvalidateBounds();
	}
	else {
		InvalidateBounds();
		fBounds.top = gScreen->GetHeight();
	}
	
	gPageViewer->SetBounds(0, 0, gScreen->GetWidth(), fBounds.top);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
OptionsPanel::SetTarget(const Resource* resource)
{
	fTargetResource = *resource;
	fPercentDone = 0;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
OptionsPanel::StartProgress()
{
	fInProgress = true;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
OptionsPanel::StopProgress()
{
	fInProgress = false;
	InvalidateBounds();
}
#endif

void 
OptionsPanel::StartPhoneProgress()
{
	SetIsPhoneInProgress(true);

#ifdef HARDWARE
	if (GetScriptResult() == kTellyLinkConnected)
		{
		gVBLsElapsed = 0;
		gVBLsPerConnectedFlashes = 3;
		}
#endif
	fPhoneInProgress->Start();
}

void 
OptionsPanel::StopPhoneProgress()
{
	SetIsPhoneInProgress(false);

#ifdef HARDWARE
	if (gVBLsPerConnectedFlashes == 3)		/* only stop it if we started it */
		{
		gVBLsPerConnectedFlashes = 0;
		SetBoxLEDs(GetBoxLEDs() | kBoxLEDConnect);
		}
#endif
	fPhoneInProgress->Stop();
}

void 
OptionsPanel::TitleChanged()
{
	Rectangle	bounds;
	
	fScrollArrows->GetBounds(&bounds);
	OffsetRectangle(bounds, fBounds.left + 7, fBounds.top + 7);
	InvalidateBounds(&bounds);
}

// =============================================================================
