// ===========================================================================
//	Keyboard.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __KEYBOARD_H__
#include "Keyboard.h"
#endif

#include "ImageData.h"
#include "MemoryManager.h"
#include "OptionsPanel.h"
#include "PageViewer.h"
#include "SystemLogo.h"
#include "TextField.h"
#include "SongData.h"
#include "System.h"

// ===========================================================================
//	local constants
// ===========================================================================

const short kFieldMargin = 6;

#ifndef FOR_MAC
const ulong kKeyboardScrollRate = 6;
#else
const ulong kKeyboardScrollRate = 2;
#endif

// =============================================================================

Keyboard::Keyboard()
{
	IsError(gKeyboard != nil);
	gKeyboard = this;

	fCapsOn = ImageData::NewImageData("file://ROM/Images/CapsOn.gif");
	fCapsOff = ImageData::NewImageData("file://ROM/Images/CapsOff.gif");
	fKeyboard = kAlphabeticKeyboard;
	
	IsError(fCapsOn == nil);
	IsError(fCapsOff == nil);
	
	fNoHWrapSelection = true;
	fShowActiveSelection = false;
}

Keyboard::~Keyboard()
{
	IsError(gKeyboard != this);
	gKeyboard = nil;

	if (fCapsOn != nil) {	
		delete(fCapsOn);
		fCapsOn = nil;
	}
	if (fCapsOff != nil) {	
		delete(fCapsOff);
		fCapsOff = nil;
	}
}

Boolean 
Keyboard::BackInput()
{
	if (!fIsVisible)
		return false;
		
	Close();
	return true;
}

void 
Keyboard::ChangeIdleState(ViewIdleState newState, ViewIdleState oldState)
{
	ContentView::ChangeIdleState(newState, oldState);
	
	if (newState != kDocumentLoadingComplete)
		return;
		
	TextField* textField = gScreen->GetKeyboardTarget();
	ContentView* targetView = nil;
	
	if (fIsVisible)
		return;

	if (textField != nil)
		targetView = textField->GetContainer();
	
	Show();
	
	if (targetView != nil)
		targetView->RefreshSelection();
	
	Rectangle	viewBounds;
	targetView->GetBounds(&viewBounds);

	// Make sure we don't obscure the target text field.
	if (targetView == gPageViewer || targetView == gScreen->GetCurrentPanel()) {	
		Rectangle	fieldBounds;
		textField->GetTextBounds(&fieldBounds);
		targetView->ContentToScreen(&fieldBounds);
		
		// If keyboard will obscure entry field, need to scroll down PageViewer or move up panel.
		fTargetViewOffset = fieldBounds.bottom + kFieldMargin - fBounds.top;
			
		if (fTargetViewOffset > 0 ) {
			if (targetView == gPageViewer)
				gPageViewer->ScrollBy(fTargetViewOffset);
			else {
				Rectangle	newViewBounds = viewBounds;
				OffsetRectangle(newViewBounds, 0, -fTargetViewOffset);
				targetView->SetBounds(&newViewBounds);
				targetView->InvalidateBounds();
			}
		}
		else
			fTargetViewOffset = 0;
	}
	
	InvalidateBounds();		

	Draw(nil);

	gScroll2Sound->Play();
	gScreen->SlideAreaUp(&fBounds, kKeyboardScrollRate, 
						 RectangleHeight(fBounds) - fTargetViewOffset);
	if (fTargetViewOffset > 0)
	{
		Rectangle	areaBounds = fBounds;
		viewBounds.bottom = areaBounds.top + fTargetViewOffset;
		gScreen->SlideViewAndAreaUp(&viewBounds, &areaBounds, kKeyboardScrollRate, 
									fTargetViewOffset);
	}

	Rectangle	pageBounds;
	gPageViewer->GetBounds(&pageBounds);
	pageBounds.bottom = fBounds.top;
	gPageViewer->SetBounds(&pageBounds);
	
	gOptionsPanel->ScrollStatusChanged();
}

void 
Keyboard::Close(Boolean closePanel)
{
	TextField* textField = gScreen->GetKeyboardTarget();
	ContentView* targetView = nil;
	Rectangle viewBounds;
	
	if (!fIsVisible)
		return;

	if (textField != nil)
		targetView = textField->GetContainer();
	
	Hide();

	// NOTE: Change PageViewer size so scrolling is correct.
	Rectangle	pageBounds;
	gPageViewer->GetBounds(&pageBounds);
	if (gOptionsPanel->IsVisible())
		pageBounds.bottom = gOptionsPanel->GetTop();
	else
		pageBounds.bottom = gScreen->GetBottom();
	gPageViewer->SetBounds(&pageBounds);
	
	// If the document fits on the screen, always restore scroll to top.
	if (targetView == gPageViewer && gPageViewer->GetDocument() != nil && 
		gPageViewer->GetDocument()->GetHeight() <= RectangleHeight(pageBounds))
		fTargetViewOffset = gPageViewer->GetScrollPosition();
		
	// If we still have a target view offset, restore the original scroll or panel position.
	if (fTargetViewOffset != 0) {
		targetView->GetBounds(&viewBounds);

		if (targetView == gPageViewer)
			fTargetViewOffset = - gPageViewer->ScrollBy(-fTargetViewOffset);	
		else {
			Rectangle	newViewBounds = viewBounds;
			OffsetRectangle(newViewBounds, 0, fTargetViewOffset);
			targetView->InvalidateBehind(&viewBounds);
			targetView->SetBounds(&newViewBounds);
			
			// If the target is a panel, and we need to close it, close the
			// panel without sliding, and then slide the entire keyboard and
			// panel off screen.
			if (closePanel && targetView == gScreen->GetCurrentPanel()) {
				gScreen->GetCurrentPanel()->Close(false);
				fTargetViewOffset = fBounds.bottom - viewBounds.top;
			}
		}
	}
	InvalidateBehind(&fBounds);
	targetView->RefreshSelection();
	
	DrawBehind();

	Rectangle	areaBounds = fBounds;
	
	gScroll2Sound->Play();
	if (fTargetViewOffset != 0) {
		if (targetView == gPageViewer)
			gScreen->SlidePageAndAreaDown(&pageBounds, kKeyboardScrollRate, fTargetViewOffset);
		else {
			viewBounds.bottom = areaBounds.top;
			gScreen->SlideViewAndAreaDown(&viewBounds, &areaBounds, kKeyboardScrollRate, fTargetViewOffset);
		}
	}
	
	if (!closePanel) {
		areaBounds.top += fTargetViewOffset;	
		gScreen->SlideAreaDown(&areaBounds, kKeyboardScrollRate, RectangleHeight(areaBounds));
	}
	
	fTargetViewOffset = 0;
	
	// Delete the document, it will be regenerated next time.	
	DeleteDocument();
	fIdleState = kViewIdle;
	
	gOptionsPanel->ScrollStatusChanged();
}

Boolean
Keyboard::ContainsTypingCommand(const char* url)
{
	return (strstr(url, "InsertChar") || strstr(url, "TextCommand"));
}

void 
Keyboard::Draw(const Rectangle* invalid)
{
	Rectangle capsBounds;
	TextField* target = GetKeyboardTarget();
	
	ContentView::Draw(invalid);
	
	fCapsOff->GetBounds(&capsBounds);
	OffsetRectangle(capsBounds, fBounds.left + 18, fBounds.top + 23);

	if (target != nil && target->IsShifted())
		fCapsOn->Draw(&capsBounds, invalid);
	else	
		fCapsOff->Draw(&capsBounds, invalid);
}

#ifdef FIDO_INTERCEPT
void 
Keyboard::Draw(FidoCompatibilityState& fidoCompatibility) const
{
	Rectangle capsBounds;
	TextField* target = GetKeyboardTarget();
	
	ContentView::Draw(fidoCompatibility);
	
	fCapsOff->GetBounds(&capsBounds);
	OffsetRectangle(capsBounds, fBounds.left + 18, fBounds.top + 23);

	if (target != nil && target->IsShifted())
		fCapsOn->Draw(&capsBounds, 0, fidoCompatibility);
	else	
		fCapsOff->Draw(&capsBounds, 0, fidoCompatibility);
}
#endif

void 
Keyboard::InsertChar(char c)
{
	TextField* target = GetKeyboardTarget();

	gTypeSound->Play();

	if (target == nil)
		return;

	target->InsertChar(c);
}

void 
Keyboard::InstallKeyboard(KeyboardType keyboard)
{
	fKeyboard = keyboard;
}

Boolean
Keyboard::IsNumeric() const
{
	TextField* target = GetKeyboardTarget();
	
	if (target == nil)
		return false;
		
	return target->HasNumbersFirst();
}

Boolean
Keyboard::KeyboardInput(Input* input)
{
	TextField* target = GetKeyboardTarget();
	uchar c = (uchar)input->data;

	if (target == nil)
		return true;

	// Pass through non-control characters to the textfield
	// so that the first letter of a word is not lost.
	if (!iscntrl(c) && target != nil)
		target->InsertChar(c);

	if (input->device != kWebTVIRRemote) {
		Close();

		// Let the system know that a hardware keyboard is being
		// used so that it doesn't automatically bring up the
		// software keyboard for textfields.		
		gSystem->SetUsingHardKeyboard(true);
	}
	
	return true;
}

void 
Keyboard::Open()
{
	const char* kKeyboardFile1 = "file://ROM/Keyboard1.html";
	const char* kKeyboardFile2 = "file://ROM/Keyboard2.html";
	const char* kKeyboardImage1 = "file://ROM/Images/Keyboard1.gif";
	const char* kKeyboardImage2 = "file://ROM/Images/Keyboard2.gif";
	
	const char* kNumericKeyFile1 = "file://ROM/NumericKeypad1.html";
	const char* kNumericKeyFile2 = "file://ROM/NumericKeypad2.html";
	
	const char* keyboardFile = kKeyboardFile1;
	const char* keyboardImage = kKeyboardImage1;
	
	switch (fKeyboard) {
		case kAlphabeticKeyboard:
			keyboardFile = kKeyboardFile1;
			keyboardImage = kKeyboardImage1;
			
			if (IsNumeric())
				keyboardFile = kNumericKeyFile1;
				
			break;

		case kStandardKeyboard:
			keyboardFile = kKeyboardFile2;
			keyboardImage = kKeyboardImage2;
			
			if (IsNumeric())
				keyboardFile = kNumericKeyFile2;
				
			break;
			
		default:
			return;
	}
		
	ImageData* image = ImageData::NewImageData(keyboardImage);
	ExecuteURL(keyboardFile);
	
	Rectangle imageBounds;
	long screenHeight = gScreen->GetHeight();
	
	image->GetBounds(&imageBounds);
	fBounds.left = imageBounds.left;
	fBounds.right = imageBounds.right;	
	fBounds.top = screenHeight - imageBounds.bottom;
	fBounds.bottom = screenHeight;

	if (gOptionsPanel->IsVisible())	
		OffsetRectangle(fBounds, 0, -gOptionsPanel->GetHeight() + 1);
		
	fNotDrawnYet = true;
	gSystem->SetUsingHardKeyboard(false);

	delete(image);
}

void
Keyboard::MoveCursor(CursorDirection direction)
{
	TextField* target = GetKeyboardTarget();

	gTypeSound->Play();

	if (target == nil)
		return;

	switch (direction) {
		case kCursorDown:		target->MoveCursorDown(); break;
		case kCursorLeft:		target->MoveCursorLeft(); break;
		case kCursorRight:		target->MoveCursorRight(); break;
		case kCursorUp:			target->MoveCursorUp(); break;
		
		default:
			IsError(true);
	}
}

Selection 
Keyboard::PreviousVisibleAnchorAbove(Selection current, Boolean wrap, Boolean requireSibling)
{
	Selection	selection = ContentView::PreviousVisibleAnchorAbove(current, wrap, requireSibling);

	// If we found another selection, return it.
	if (fDocument == nil || !SelectionsEqual(selection, kInvalidSelection) ||
		SelectionsEqual(current, kInvalidSelection))
		return selection;

	// Look for the "Close" button by looking for a selectable after this
	// one that is above by a smaller amount than "standard above" test used
	// in ContentView. If we find one below, exit early.
	ulong	count = fDocument->GetSelectableCount(current.fPageIndex);
	Rectangle	originalBounds;
	fDocument->GetSelectable(current)->GetBounds(&originalBounds);
	for (; current.fSelectableIndex < count; current.fSelectableIndex++) {
		Rectangle	currentBounds;
		fDocument->GetSelectable(current)->GetBounds(&currentBounds);
		if (originalBounds.top - currentBounds.top > 3)
			return current;
		if (currentBounds.top - originalBounds.top > 3)
			break;
	}
	
	return selection;
}

Boolean 
Keyboard::ScrollUpInput(Input*)
{
	if (!IsVisible())
		return false;
		
	InsertChar(' ');
	return true;
}

Boolean 
Keyboard::ScrollDownInput(Input*)
{
	if (!IsVisible())
		return false;
		
	TextCommand(kBackspaceCmd);
	return true;
}

void 
Keyboard::TextCommand(CommandName command)
{
	TextField* target = GetKeyboardTarget();
	ContentView* container;

	gTypeSound->Play();
	
	if (target == nil || (container=target->GetContainer()) == nil)
		return;	

	switch (command) {
		case kBackspaceCmd:
			target->Backspace();
			break;
		case kClearCmd:
			target->ClearText();
			break;
		case kEnterCmd:
			target->Return();
			break;
		case kShiftCmd:
			target->SetShifted(!target->IsShifted());
			break;
		case kNextCmd:
			target->Next();
			break;
		case kPreviousCmd:
			target->Previous();
			break;
	}
}


