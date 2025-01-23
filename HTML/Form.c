// ===========================================================================
//	Form.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include "Form.h"
#include "MemoryManager.h"
#include "Menu.h"
#include "PageViewer.h"
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#include "Service.h"
#include "SongData.h"
#include "TextField.h"

// ===========================================================================

Form::Form()
{
	fIsSelectable = true;
	fMethod = AV_GET;
}

Form::~Form()
{
	fControlList.RemoveAll();
	
	if (fAction != nil)
		FreeTaggedMemory(fAction, "Form::fAction");
	
	if (fSelectValue != nil)
		FreeTaggedMemory(fSelectValue, "Form::fSelectValue");
	
	if (fFormData != nil)
		delete(fFormData);
		
	if (fName != nil)
		FreeTaggedMemory(fName, "Form::fName");
}

void 
Form::AddControl(Control* control)
{
	fControlList.Add(control);
	if (!IsError(fDocument == nil))
		fDocument->AddControl(control);
	control->SetForm(this);
	control->Reset();
}

Boolean 
Form::AddText(const char* text, long UNUSED(textCount))
{
	// Let the form have the first crack at the input text
	
	if (fSelect && fWaitingForOption) {
		fSelect->AddItem(text, fSelectValue, fSelected);
		fWaitingForOption = false;
		fSelected = false;
		return true;
	}

	return false;
}

void 
Form::CloseSelect()
{
	fSelect = nil;
	fWaitingForOption = false;
	if (fSelectValue != nil) {
		FreeTaggedMemory(fSelectValue, "Form::fSelectValue");
		fSelectValue = nil;
	}
}

#define kHTTPShortPrefix		"http://"
#define kHTTPLongPrefix			"http://www."
#define kHTTPSuffix				".com"

static Boolean
FillFormDataFromSubmission(MemoryStream* formData, TextField* textField)
// return if got marked up
{
	formData->Reset();
	
	const char*		text = textField->GetText();
	if (strrchr(text, ':') != nil) {
		formData->WriteString(text);
		return false;
	}
	
	// missing info, so fill it in ourselves
	Boolean			longForm = (strrchr(text, '.') == nil);
	
	formData->WriteString(longForm ? kHTTPLongPrefix : kHTTPShortPrefix);
	formData->WriteString(text);
	if (longForm)
		formData->WriteString(kHTTPSuffix);
	return true;
}

void 
Form::CreateSubmission(Control* submitDisplayable)
{
	// Build the form data
	Boolean	foundSubmit = false;
	
	fExecuteURL = false;

	if (fFormData != nil)
		delete(fFormData);
	fFormData = new(MemoryStream);

	for (int i = 0; i < GetControlCount(); i++) {		
		Control*	control = GetControl(i);
		
		// Special case: Handle ExecuteURL mode on text fields. Throw away form
		// data just execute the url.
		if (control->IsTextField()) {
			TextField*	textField = (TextField*)control;
			if (textField->GetExecuteURL()) {
				fExecuteURL = true;
				if (FillFormDataFromSubmission(fFormData, textField)) {
					textField->SetAttributeStr(A_VALUE, fFormData->GetDataAsString());
					textField->InvalidateBounds(true);
				}
				return;
			}
		}
		
		// ignore *all* submit buttons (but already submitted submitDisplayable)
		if ((submitDisplayable == control || !control->IsSubmit()) &&
			!control->IsReset())
			control->AddSubmission(fFormData, false);
		if (submitDisplayable == control)
			foundSubmit = true;
	}
	
	// If we didn't find the submit control, it must be outside the form.
	// Add it now.
	if (!foundSubmit && submitDisplayable != nil)
		submitDisplayable->AddSubmission(fFormData, false);
}

void 
Form::CreateSubmission(const char* submitName, const char* submitValue)
{
	CreateSubmission(nil);
	
	// Submit current values
	if (submitName != nil && submitValue != nil)
		fFormData->WriteQuery(submitName, submitValue);
}

short 
Form::GetControlCount() const
{
	return fControlList.GetCount();
}

Document*
Form::GetDocument() const
{
	return fDocument;
}

void 
Form::Submit()
{
	char* url;
	const char* formData;
	
	if (fExecuteURL && fFormData != nil) {
#ifndef DEBUG
		if (gServiceList->HasService(fDocument->GetBaseResource(), fFormData->GetDataAsString()))
#endif
		{
			SetLastExecuteURL(fFormData->GetDataAsString());
			fDocument->GetView()->ExecuteURL(fFormData->GetDataAsString());
		}
		return;
	}
	
	if ((url = CopyString(GetFormAction(), "URL")) == nil)
		return;
	formData = nil;

	switch (GetFormMethod()) {
		case AV_POST:
			formData = GetFormData();
			Message(("POST '%s'", formData));
			break;
		
		case AV_GET:
			url = CatStringTo(url, "?", "URL");
			url = CatStringTo(url, GetFormData(), "URL");
			Message(("GET '%s'", url));
			break;
		
		default:
			FreeTaggedMemory(url, "URL");			
			Complain(("Form::Submit: unexpected form method %d", GetFormMethod()));
			return;
	}

	gSubmitSound->Play();	
	fDocument->GetView()->ExecuteURL(url, formData);
	FreeTaggedMemory(url, "URL");
}

void 
Form::AutoSubmit()
{
	char* url;
	const char* formData;
	
	if ((url = CopyString(GetFormAction(), "URL")) == nil)
		return;
	formData = nil;

	switch (GetFormMethod()) {
		case AV_POST:
			formData = GetFormData();
			Message(("POST '%s'", formData));
			break;
		
		case AV_GET:
			url = CatStringTo(url, "?", "URL");
			url = CatStringTo(url, GetFormData(), "URL");
			Message(("GET '%s'", url));
			break;
		
		default:
			FreeTaggedMemory(url, "URL");			
			Complain(("Form::Submit: unexpected form method %d", GetFormMethod()));
			return;
	}

	Resource	resource;
	long		formDataLength = 0;
	
	if (formData != nil && *formData != 0)
		formDataLength = strlen(formData);
	resource.SetURL(url, GetDocument()->GetBaseResource(), formData, formDataLength);
	resource.SetPriority(kImmediate);
	
	FreeTaggedMemory(url, "URL");
}

const char* 
Form::GetFormAction() const
{
	return fAction;
}

const char* 
Form::GetFormData() const
{
	if (fFormData == nil)
		return nil;
		
	return fFormData->GetDataAsString();
}

short 
Form::GetFormMethod() const
{
	return fMethod;
}

const char* 
Form::GetName() const
{
	return fName;
}

Control* 
Form::GetControl(short itemIndex) const
{
	// Handy for counting and getting input items
	
	return (Control*)fControlList.At(itemIndex);
}

short 
Form::GetControlIndex(Control* item) const
{
	for (short i = 0; i < GetControlCount(); i++)
		if (GetControl(i) == item)
			return i;
	IsError(true);
	return -1;
}

Menu* 
Form::GetSelect() const
{
	return fSelect;
}

ulong 
Form::GetTextFieldCount() const
{
	ulong count = fControlList.GetCount();
	ulong textFieldCount = 0;
	ulong i;
	
	for (i=0; i < count; i++) {
		Control* control = (Control*)fControlList.At(i);
		if (control->IsTextField())
			textFieldCount++;
	}
	
	return textFieldCount;
}

void 
Form::Idle(Layer* layer)
{
	PerfDump perfdump("Form::Idle");

	// Idle the form, blink carets
	
	for (short i = 0; i < GetControlCount(); i++)
		GetControl(i)->Idle(layer);
}

Boolean
Form::IsSelectable() const
{
	return fIsSelectable;
}

void
Form::LeaveDocument()
{
	// Look for an autosubmit control and submit the form if found.
	long count = fControlList.GetCount();
	for (long i = 0; i < count; i++) {
		Control*	control = (Control*)fControlList.At(i);
		
		if (control->IsAutoSubmit() && ((InputHidden*)control)->GetAutoSubmitType() == AV_ONLEAVE) {
			CreateSubmission(control);
			AutoSubmit();
		}
	}
}

Control* 
Form::NextControl(Control* current) const
{
	Control*	next = current;
	long count = fControlList.GetCount();

	do {
		long currentIndex = GetControlIndex(next);
		
		if (currentIndex < 0 || count == 0)
			return current;
	
		if (++currentIndex >= count)
			currentIndex = 0;
				
		next = (Control*)fControlList.At(currentIndex);
	} while (!next->IsSelectable() && next != current);
	
	return next;
}

TextField* 
Form::NextTextField(TextField* current) const
{
	Control*	next = current;
	
	do {
		next = NextControl(next);
	} while (!next->IsTextField() && next != current);
	

	return (TextField*)next;	
}

void 
Form::OpenSelect(Menu* menu)
{
	CloseSelect();
	fSelect = menu;
	AddControl(menu);
}

void 
Form::Option()
{
	if (fSelect != nil)
		fWaitingForOption = true;
}

Control* 
Form::PreviousControl(Control* current) const
{
	Control*	previous = current;
	ulong count = fControlList.GetCount();

	do {
		long currentIndex = GetControlIndex(previous);
		
		if (currentIndex < 0 || count == 0)
			return current;
	
		if (--currentIndex < 0)
			currentIndex = count - 1;
				
		previous = (Control*)fControlList.At(currentIndex);
	} while (!previous->IsSelectable() && previous != current);
	
	return previous;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
TextField*
Form::PreviousTextField(TextField* current) const
{
	Control*	previous = current;
	
	do {
		previous = PreviousControl(previous);
	} while (!previous->IsTextField() && previous != current);
	

	return (TextField*)previous;	
}
#endif

void 
Form::RadioGroup(Control* radio)
{
	// Deselect any other radio button in group....
	// .. when a radio button is selected
	
	const char* s = radio->GetName();
	for (short i = 0; i < GetControlCount(); i++)  {		
		Control* control = GetControl(i);
		if (control != radio && control->IsRadio() && !strcmp(s, control->GetName()))
			control->Commit(0);
	}
}

void 
Form::Reset()
{
	Control* control;
	Rectangle bounds;
	ContentView* view = fDocument->GetView();
	
	for (short i = 0; i < GetControlCount(); i++)
		if ((control = GetControl(i))->Reset()) {
			control->GetBounds(&bounds);
			view->ContentToScreen(&bounds);
			view->InvalidateBounds(&bounds);
		}
}

void 
Form::SetAttribute(Attribute attributeID, long value, Boolean)
{
	switch (attributeID) {				
		case A_METHOD:		if (value > 0 ) fMethod = (AttributeValue)value;		break;
		case A_SELECTED:	if (fSelect != nil) fSelected = true;					break;
		default:			break;
	}
}

void 
Form::SetAttributeStr(Attribute attributeID, const char* value)
{	
	switch (attributeID) {
		case A_ACTION:
			fAction = CopyStringTo(fAction, value, "Form::fAction");
			if (fDocument != nil)
				fIsSelectable = gServiceList->HasService(fDocument->GetBaseResource(), fAction);
			break;
		case A_ID:
		case A_NAME:
			fName = CopyStringTo(fName, value, "Form::fName");
			break;
		case A_VALUE:
			if (fSelect != nil)
				fSelectValue = CopyStringTo(fSelectValue, value, "Form::fSelectValue");
			break;
		default:
			break;
	}
}

void
Form::SetDocument(Document* document)
{
	fDocument = document;
}

// =============================================================================

