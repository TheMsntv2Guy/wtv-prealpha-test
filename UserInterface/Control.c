// ===========================================================================
//	Control.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CONTROL_H__
#include "Control.h"
#endif

#include "Animation.h"
#include "ImageData.h"
#include "Keyboard.h"
#include "Menu.h"
#include "MemoryManager.h"
#include "PageViewer.h"
#include "Service.h"
#include "Stream.h"
#include "System.h"
#include "TextField.h"

// =============================================================================

Control::Control()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberControl;
#endif /* DEBUG_CLASSNUMBER */
}

Control::~Control()
{
	if (fAction != nil)
		FreeTaggedMemory(fAction, "Control::fAction");
		
	if (fID != nil)
		FreeTaggedMemory(fID, "Control::fID");

	if (fName != nil)
		FreeTaggedMemory(fName, "Control::fName");

	if (fValue != nil)
		FreeTaggedMemory(fValue, "Control::fValue");
}

void
Control::AddSubmission(DataStream* stream, Boolean UNUSED(force))
{
	if (fName != nil)
		stream->WriteQuery(fName, fValue);
}

Boolean 
Control::ExecuteInput()
{
	if (fAction != nil) {
		MemoryStream*	stream = new(MemoryStream);
		
		stream->WriteString(fAction);
		stream->WriteString("?");
		AddSubmission(stream, true);

		gPageViewer->ExecuteURL(stream->GetDataAsString());

		delete(stream);
	}
	
	return true;
}

AttributeValue
Control::GetAlign() const 
{
	return AV_ABSMIDDLE;
}

const char*
Control::GetID() const
{
	return fID;
}

const char*
Control::GetName() const
{
	return fName;
}

Displayable*
Control::GetParent() const
{
	return fParent;
}

void 
Control::GetSelectionRegion(Region* region) const
{
	if (!IsError(region == nil)) {
		region->Reset();

		Rectangle bounds;	
		GetBoundsTight(&bounds);
		region->Add(&bounds);
	}
}

ContentView*
Control::GetView() const
{
	if (IsError(fForm == nil))
		return nil;
		
	Document* document = fForm->GetDocument();
	
	if (IsError(document == nil))
		return nil;
		
	return document->GetView();
}

void 
Control::InvalidateBounds()
{
	ContentView* view = GetView();
	
	if (IsError(view == nil))
		return;
	
	Rectangle bounds;
	GetBounds(&bounds);
	
	view->ContentToScreen(&bounds);
	view->InvalidateBounds(&bounds);
}

Boolean 
Control::IsAutoSubmit() const
{
	return false;
}

Boolean 
Control::IsLayoutComplete() const
{
	return fLayoutComplete;
}

Boolean 
Control::IsSelectableInputImage() const
{
	return false;
}

Boolean 
Control::IsRadio() const
{
	return false;
}

Boolean 
Control::IsReset() const
{
	return false;
}

Boolean 
Control::IsSubmit() const
{
	return false;
}

Boolean 
Control::KeyboardInput(Input* input)
{
	if (input->data == '\t') {
		if (input->modifiers & kShiftModifier)
			MoveToPreviousControl();
		else
			MoveToNextControl();
		return true;
	}
			
	return false;
}

void 
Control::LayoutComplete(Document* document, Displayable*)
{
	document->AddSelectable(this);
	fLayoutComplete = true;
}

void 
Control::MoveToNextControl()
{
	ContentView* view = GetView();
	Control* control = fForm->NextControl(this);
	
	if (IsError(control == nil))
		return;

	// "Next" within forms with one field should close.	
	if (control == this) {
		gKeyboard->Close();
		return;
	}

	if (control->IsTextField()) {
		TextField* textfield = (TextField*)control;
		textfield->MakeActive();
		
	} else if (gKeyboard->IsVisible())
		// Only show keyboard for textfields.
		gKeyboard->Close();
			
	if (control->IsSelectableInputImage())
		view->SetCurrentSelectableWithScroll(((InputImage*)control)->GetSelectable());
	else
		view->SetCurrentSelectableWithScroll(control);
}

void 
Control::MoveToPreviousControl()
{
	ContentView* view = GetView();
	Control* control = fForm->PreviousControl(this);
	
	if (IsError(control == nil))
		return;

	// "Previous" within forms with one field should close.	
	if (control == this) {
		gKeyboard->Close();
		return;
	}

	if (control->IsTextField()) {
		TextField* textfield = (TextField*)control;
		textfield->MakeActive();
		
	} else if (gKeyboard->IsVisible())
		// Only show keyboard for textfields.
		gKeyboard->Close();
			
	if (control->IsSelectableInputImage())
		view->SetCurrentSelectableWithScroll(((InputImage*)control)->GetSelectable());
	else
		view->SetCurrentSelectableWithScroll(control);
}

void 
Control::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	// The set attribute stuff is probably too general
	
	switch (attributeID) {		
		case A_CHECKED:		if (value != AV_OFF) fChecked = true;	break;
		case A_SELECTED:	fIsInitiallySelected = true; 			break;
		case A_NOHIGHLIGHT:	fNoHighlight = true;					break;
		case A_WIDTH:		if (value > 0) fKnownWidth = value;
							fIsWidthPercentage = isPercentage;
							if (isPercentage)
								fKnownWidth = MIN(value, 100);
							break;
		default: TrivialMessage(("Control::SetAttribute ignored attribute %d",attributeID)); break;
	}
}

void 
Control::SetAttributeStr(Attribute attributeID, const char* value)
{
	// Remember name and value
	
	switch (attributeID) {		
		case A_ACTION:	fAction = CopyStringTo(fAction, value, "Control::fAction"); break;
		case A_ID:		fID = CopyStringTo(fID, value, "Control::fID"); break;
		case A_NAME:	fName = CopyStringTo(fName, value, "Control::fName"); break;
		case A_VALUE:	fValue = CopyStringTo(fValue, value, "Control::fValue"); break;
		default: TrivialMessage(("Control::SetAttributeStr gnored attribute %d",attributeID)); break;
	}
}

Boolean 
Control::Idle(Layer*)
{
	return false;
}

Boolean 
Control::IsSelectable() const
{
	if (fForm == nil)
		return false;
		
	if (fAction != nil && fForm->GetDocument() != nil)
		return gServiceList->HasService(fForm->GetDocument()->GetBaseResource(), fAction);

	return (fForm->IsSelectable());
}

Boolean 
Control::IsHighlightable() const
{
	return (fNoHighlight ? false : IsSelectable());
}

Boolean 
Control::IsInitiallySelected() const
{
	return fIsInitiallySelected;
}

Boolean 
Control::Reset()
{
	// Reset to default values
	return false;	// no invalidation needed
}

void
Control::ResetLayout(Document*)
{
	fLayoutComplete = false;
}

void 
Control::RestoreFillinData(const char*)
{
}

const char* 
Control::SaveFillinData() const
{
	return nil;
}

void 
Control::SetForm(Form* value)
{
	fForm = value;
}

void
Control::SetParent(Displayable* parent)
{
	fParent = parent;
}

// =============================================================================
// InputImage - Uses a regular image..
// It does not draw itself, it lets the image do the work...

InputImage::InputImage()
{
	fImage = new(Image);
	fImage->SetAttribute(A_ISMAP, 0, 0);
	fUseCursor = true;

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberInputImage;
#endif /* DEBUG_CLASSNUMBER */
}

InputImage::~InputImage()
{
	delete(fImage);
}

void
InputImage::Draw(const Document* document, const Rectangle* invalid)
{
	fImage->Draw(document, invalid);
}

Boolean 
InputImage::ExecuteInput()
{
	PostulateFinal(false);	// should share code w/ SubmitButton::ExecuteKeyInput()

	ContentView* view = GetView();
		
	if (fActionImage != nil && !fStartedActionImage) {
		Rectangle		bounds;

		fActionImage->Show();
		fActionImage->Start();
		fStartedActionImage = true;
		
		fActionImage->GetBounds(&bounds);
		view->ContentToScreen(&bounds);
		view->InvalidateBounds(&bounds);
		
		return true;
	}
	else if (fAction != nil)
		return Control::ExecuteInput();
		
	fForm->CreateSubmission(this);
	fForm->Submit();
	return true;
}

AttributeValue 
InputImage::GetAlign() const
{
	return GetMappedImage()->GetAlign();
}

void 
InputImage::GetBoundsTight(Rectangle* r) const
{
	GetMappedImage()->GetBoundsTight(r);
}

long 
InputImage::GetLeft() const
{
	return GetMappedImage()->GetLeft();
}

long 
InputImage::GetHeight() const
{
	return GetMappedImage()->GetHeight();
}

ImageMap* 
InputImage::GetImageMap() const
{
	if (!fUseCursor)
		return nil;
		
	return fActionImage != nil ? nil : Displayable::GetImageMap();
}

Image* 
InputImage::GetMappedImage() const
{
	return fImage;
}

ImageMapSelectable* 
InputImage::GetSelectable() const
{
	return fSelectable;
}

long 
InputImage::GetTop() const
{
	return GetMappedImage()->GetTop();
}

long 
InputImage::GetWidth() const
{
	return GetMappedImage()->GetWidth();
}

Boolean 
InputImage::Idle(Layer*)
{
	if (fStartedActionImage && fActionImage != nil && !fActionImage->IsRunning()) {
		ExecuteInput();
		fStartedActionImage = false;
	}
	
	return false;
}

Boolean 
InputImage::IsFloating() const
{
	return GetMappedImage()->IsFloating();
}

Boolean 
InputImage::IsLayoutComplete() const
{
	return GetMappedImage()->IsLayoutComplete();
}

Boolean 
InputImage::IsSelectableInputImage() const
{
	return fActionImage == nil && fUseCursor;
}

Boolean 
InputImage::IsSubmit() const
{
	// Input image pretends to be a submit button
	
	return fActionImage == nil;
}

void 
InputImage::Layout(Document* document, Displayable* parent)
{
	GetMappedImage()->Layout(document, parent);
	Control::Layout(document, parent);
	
	if (fActionImage != nil) {
		((Document*)document)->GetRootDisplayable()->AddChild(fActionImage);
		((Document*)document)->AddImage(fActionImage);
	}
}
	
void 
InputImage::LayoutComplete(Document* document, Displayable* parent)
{
	GetMappedImage()->LayoutComplete(document, parent);

	if (fActionImage == nil && fUseCursor) {
		// Treat this selectable as a server-side image map.
		fSelectable = new(ImageMapSelectable);
		fSelectable->SetSelectable(this);
		fSelectable->SetInitiallySelected(fIsInitiallySelected);
		document->AddSelectable(fSelectable);
	} else {
		Control::LayoutComplete(document, parent);
	}
}

Boolean	
InputImage::ReadyForLayout(const Document* document)
{
	return GetMappedImage()->ReadyForLayout(document);
}
	
void	
InputImage::ResetLayout(Document* document)
{
	GetMappedImage()->ResetLayout(document);
}
	
void 
InputImage::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	GetMappedImage()->SetAttribute(attributeID, value, isPercentage);
	Control::SetAttribute(attributeID, value, isPercentage);
}

void 
InputImage::SetAttributeStr(Attribute attributeID, const char* value)
{
	switch (attributeID) {
		case A_ONCLICK:
			fActionImage = new(Animation);
			fActionImage->SetAttributeStr(A_ANI, value);
			fActionImage->Hide();
			break;
		case A_LOOP:
			if (fActionImage != nil)
				fActionImage->SetAttributeStr(attributeID, value);
			break;
		case A_NOCURSOR:
			fUseCursor = false;
			break;
		default:
			GetMappedImage()->SetAttributeStr(attributeID, value);
			Control::SetAttributeStr(attributeID, value);
	}
}

void 
InputImage::SetLeft(long left)
{
	GetMappedImage()->SetLeft(left);
}

void 
InputImage::SetParent(Displayable* parent)
{
	// Delegate parent to image so that image uses correct max-size.
	Control::SetParent(parent);
	GetMappedImage()->SetParent(parent);
}

void 
InputImage::SetTop(long top)
{
	GetMappedImage()->SetTop(top);
}

void
InputImage::AddSubmission(DataStream* stream, Boolean UNUSED(force))
{
	Coordinate	point;
	
	if (fName == nil)
		return;
		
	ContentView*	view = GetView();

	if (view == nil) {
		Rectangle	bounds;
		GetMappedImage()->GetBounds(&bounds);	
		point.x = (bounds.right - bounds.left)/2;
		point.y = (bounds.bottom - bounds.top)/2;
	} else {
		ImageMapSelectable* mapSelectable = (ImageMapSelectable*)view->GetCurrentSelectable();
		point = mapSelectable->GetTargetPoint(view->GetMapCursorPosition());
	}
	// +2 for ".x" or ".y", +1 for null terminator.
	long allocSize = strlen(fName) + 2 + 1 ;
	char* s = (char*)AllocateTaggedMemory(allocSize, "InputImage::AddSubmission");
	snprintf(s, allocSize, "%s.x", fName);
	stream->WriteQuery(s, (long)point.x);
	snprintf(s, allocSize, "%s.y", fName);
	stream->WriteQuery(s, (long)point.y);
	
	FreeTaggedMemory(s, "InputImage::AddSubmission");
}

// =============================================================================
// CheckBox
static const char kOnString[] = "On";
static const char kOffString[] = "Off";

CheckBox::CheckBox()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberCheckBox;
#endif /* DEBUG_CLASSNUMBER */
}

void 
CheckBox::Commit(long /*value*/)
{
	ContentView* view = GetView();
	
	fOn = !fOn;
	view->InvalidateBounds();
}

void 
CheckBox::Draw(const Document* document, const Rectangle* invalid)
{
	if (IsError(document == nil || gCheckBoxImage == nil || gCheckedBoxImage == nil))
		return;
	
	Rectangle	r;
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
	
	(fOn ? gCheckedBoxImage : gCheckBoxImage)->Draw(&r, invalid);
}

Boolean 
CheckBox::ExecuteInput()
{
	Commit(!fOn);
	return Control::ExecuteInput();
}

void 
CheckBox::Layout(Document*, Displayable*)
{
	if (IsError(gCheckedBoxImage == nil))
		return;
		
	Rectangle	r;
	
	gCheckedBoxImage->GetBounds(&r);
	SetWidth(r.right - r.left);
	SetHeight(r.bottom - r.top);
}

Boolean 
CheckBox::Reset()
{
	Boolean		result = (fOn != fChecked);
	
	fOn = fChecked;
	return result;
}

void 
CheckBox::RestoreFillinData(const char* data)
{
	fOn = strcmp(data, kOnString) == 0 ? true : false;	
}

const char* 
CheckBox::SaveFillinData() const
{
	if (fOn != fChecked)
		return fOn ? kOnString : kOffString;
		
	return nil;
}

void
CheckBox::AddSubmission(DataStream* stream, Boolean force)
{
	if (fOn)
		Control::AddSubmission(stream, force);
	else if (force && fName != nil)
		stream->WriteQuery(fName, "0");
}

// =============================================================================
InputHidden::InputHidden()
{
}

InputHidden::~InputHidden()
{
}

Boolean 
InputHidden::IsAutoSubmit() const
{
	return fAutoSubmit != 0;
}

Boolean 
InputHidden::IsSubmit() const
{
	return fAutoSubmit != 0;
}
Boolean 
InputHidden::IsSelectable() const
{
	return false;
}

void 
InputHidden::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) {		
		case A_AUTOSUBMIT:	if (value > 0) fAutoSubmit = (AttributeValue)value; break;
		default:			Control::SetAttribute(attributeID, value, isPercentage);
	}
}


