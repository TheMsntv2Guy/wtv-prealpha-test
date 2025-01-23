// ===========================================================================
//	RadioButton.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __RADIOBUTTON_H__
#include "RadioButton.h"
#endif

#include "ImageData.h"
#include "PageViewer.h"
#include "System.h"

// ===========================================================================
// implementation
// ===========================================================================

RadioButton::RadioButton()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberRadioButton;
#endif /* DEBUG_CLASSNUMBER */
}

void 
RadioButton::Commit(long value)
{
	ContentView* view = GetView();
	
	fOn = value;
	view->InvalidateBounds();
}

void
RadioButton::AddSubmission(DataStream* stream, Boolean force)
{
	if (!force && !fOn)
		return;
	
	Control::AddSubmission(stream, force);
}

void 
RadioButton::Draw(const Document* document, const Rectangle* invalid)
{
	if (IsError(document == nil))
		return;
	
	Rectangle	r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
	
	(fOn ? gRadioButtonOnImage : gRadioButtonOffImage)->Draw(&r, invalid);
}

Boolean 
RadioButton::ExecuteInput()
{
	if (fForm != nil)
		fForm->RadioGroup(this);
	Commit(true);
	
	return Control::ExecuteInput();
}

Boolean 
RadioButton::IsRadio() const
{
	return true;
}

void 
RadioButton::Layout(Document*, Displayable*)
{
	Rectangle	r;
	
	gRadioButtonOnImage->GetBounds(&r);
	SetWidth(r.right - r.left);
	SetHeight(r.bottom - r.top);
}

