// Copyright(c) 1995 Artemis Research, Inc. All rights reserved.

#include "Headers.h"

#ifndef __BUTTON_H__
#include "Button.h"
#endif

#include "ImageData.h"
#include "MemoryManager.h"
#include "PageViewer.h"

// =============================================================================
// Button

static PackedStyle gButtonStyle = { 2, 0, 0, 0, 0 };

Button::Button()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberButton;
#endif /* DEBUG_CLASSNUMBER */
}

void 
Button::Draw(const Document* document, const Rectangle* invalid)
{
	Rectangle r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
	
	// look for quick out
	if (invalid != nil && !RectanglesIntersect(&r, invalid))
		return;

	//::InsetRectangle(r, 2, 2);
	
	// Paint background.
	if (!gButtonBorder->GetDrawCenter())
		PaintRectangle(gScreenDevice, r, kLightGrayColor, 0, invalid);

	long oldBackColor = gPageBackColor;
	gPageBackColor = kLightGrayColor;	// ¥¥¥¥ HACK FOR NOTIFYING XDRAWTEXT OF BackgroundColor ¥¥¥¥

	// Draw border.
	gButtonBorder->Draw(&r, invalid);

	// Paint text.
	const char* title = GetTitle();
	if (title != nil && *title != 0) {
		char* shortTitle = nil;
		XFont font = document->GetFont(gButtonStyle);
		CharacterEncoding encoding = document->GetCharacterEncoding();
		ulong ascent = ::GetFontAscent(font,encoding);

		ulong availableTextWidth = GetWidth() - gButtonBorder->PadBounds().left - gButtonBorder->PadBounds().right;
		ulong textWidth = TextMeasure(gScreenDevice, font, encoding, title, strlen(title));
		
		if (textWidth > availableTextWidth) {
			shortTitle = NewTruncatedStringWithEllipsis(title, font, encoding, availableTextWidth, "Button::Draw");
			title = shortTitle;
			textWidth = TextMeasure(gScreenDevice, font, encoding, title, strlen(title));
		}
		
		// Center horizontally.
		Ordinate x = r.left + gButtonBorder->PadBounds().left + (availableTextWidth - textWidth) / 2;
		Ordinate y = r.top + gButtonBorder->PadBounds().top + ascent;
		
		::PaintText(gScreenDevice, font,encoding, title, strlen(title), kBlackColor, x, y, 0, false, invalid);
		
		if (shortTitle != nil)
			FreeTaggedMemory(shortTitle, "Button::Draw");
	}
	
	gPageBackColor = oldBackColor;
}

const char* 
Button::GetTitle() const
{
	if (fValue == nil)
		return "Button";
	
	return fValue;
}

void 
Button::Layout(Document* document, Displayable* parent)
{
	const char* title = GetTitle();
	if (title == nil || *title == 0)
		return;
		
	XFont font = document->GetFont(gButtonStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ulong ascent = ::GetFontAscent(font,encoding);
	ulong descent = ::GetFontDescent(font,encoding);
	ulong leading = ::GetFontLeading(font,encoding);
	long width;
	
	if (fKnownWidth > 0) {
		if (fIsWidthPercentage)
			width = (fKnownWidth * parent->GetWidth() + 50) / 100;
		else
			width = fKnownWidth;
	}
	else {
		width = TextMeasure(gScreenDevice, font, encoding, title, strlen(title)) +
			 	gButtonBorder->PadBounds().left + gButtonBorder->PadBounds().right;
	}
	
	SetWidth(MIN(width, ((Page*)parent)->GetDefaultMarginWidth()));
	SetHeight(ascent + descent + leading*2  + gButtonBorder->PadBounds().top + gButtonBorder->PadBounds().bottom);
}

// =============================================================================
// SubmitButton

SubmitButton::SubmitButton()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberSubmitButton;
#endif /* DEBUG_CLASSNUMBER */
}

SubmitButton::~SubmitButton()
{
	if (fUseForm != nil)
		FreeTaggedMemory(fUseForm, "SubmitButton::fUseForm");
}

Boolean 
SubmitButton::ExecuteInput()
{
	if (fAction != nil)
		return Control::ExecuteInput();
		
	if (IsError(fForm == nil))
		return true;
	
	// If a named form was specified, find and submit it.	
	if (fUseForm != nil) {
		Document*	document = fForm->GetDocument();
		if (!IsError(document == nil)) {
			Form*	form = document->FindForm(fUseForm);
			if (form != nil) {
				form->CreateSubmission(this);
				form->Submit();
				return true;
			}
		}
	}
	
	// Otherwise submit our contained form.
	fForm->CreateSubmission(this);
	fForm->Submit();
	return true;
}

const char* 
SubmitButton::GetTitle() const
{
	if (fValue)
		return fValue;

	return "Submit";
}

Boolean 
SubmitButton::IsSubmit() const
{
	return fAction == nil;
}

void 
SubmitButton::SetAttributeStr(Attribute attributeID, const char* value)
{
	switch (attributeID) {
		case A_USEFORM:
			fUseForm = CopyString(value, "SubmitButton::fUseForm");
			break;
		default:
			Control::SetAttributeStr(attributeID, value);
			break;
	}
}

// =============================================================================
// ResetButton

ResetButton::ResetButton()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberResetButton;
#endif /* DEBUG_CLASSNUMBER */
}

Boolean 
ResetButton::ExecuteInput()
{
	ImportantMessage(("Resetting form..."));
	
	if (fForm != nil)
		fForm->Reset();
	return true;
}

const char* 
ResetButton::GetTitle() const
{
	if (fValue != nil)
		return fValue;
	return "Reset";
}

Boolean 
ResetButton::IsReset() const
{
	return true;
}

