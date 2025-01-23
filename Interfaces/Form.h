// ===========================================================================
//	Form.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __FORM_H__
#define __FORM_H__

#ifndef __DISPLAYABLE_H__
#include "Displayable.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif

class Control;
class Layer;
class MemoryStream;
class Menu;
class TextField;

// =============================================================================

class Form : public HasHTMLAttributes {
public:
							Form();
	virtual 				~Form();
	
	short					GetControlCount() const;
	Menu*					GetSelect() const;
	const char*				GetFormData() const;
	const char*				GetFormAction() const;
	short					GetFormMethod() const;
	const char*				GetName() const;
	ulong					GetTextFieldCount() const;
	Document*				GetDocument() const;
	Boolean					IsSelectable() const;
	
	void					SetDocument(Document*);
	
	void					AddControl(Control*);
	Boolean					AddText(const char* text, long textCount);
	void					CloseSelect();
	void					CloseTextArea();
	void					CreateSubmission(Control* submitDisplayable);
	void					CreateSubmission(const char* submitName,
											 const char* submitValue);
	Control*				GetControl(short itemIndex) const;
	short					GetControlIndex(Control* item) const;
	void					Idle(Layer* layer);
	void					LeaveDocument();
	Control*				NextControl(Control* current) const;
	TextField*				NextTextField(TextField* current) const;
	void					OpenSelect(Menu* select);
	void					OpenTextArea(TextField* text);
	void					Option();
	Control*				PreviousControl(Control* current) const;
	TextField*				PreviousTextField(TextField* current) const;
	void					RadioGroup(Control* radio);
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
	void					Reset();
	void					Submit();
	void					AutoSubmit();

protected:
	ObjectList				fControlList;
	Document*				fDocument;
	char*					fAction;
	MemoryStream*			fFormData;
	char*					fName;
	Menu*					fSelect;
	char*					fSelectValue;

	AttributeValue			fMethod;

	unsigned				fExecuteURL : 1;
	unsigned				fIsSelectable : 1;
	unsigned				fSelected : 1;
	unsigned				fWaitingForOption : 1;
};

// =============================================================================

#endif /*__FORM_H__ */
