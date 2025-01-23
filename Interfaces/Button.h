// Copyright(c) 1995 Artemis Research, Inc. All rights reserved.

#ifndef __BUTTON_H__
#define __BUTTON_H__

#ifndef __CONTROL_H__
#include "Control.h"
#endif

// ===========================================================================

class Button : public Control {
public:
							Button();
							
	virtual const char*		GetTitle() const;

	virtual void			Draw(const Document* document, const Rectangle* invalid);

protected:
	virtual void			Layout(Document*, Displayable* parent);
};

class SubmitButton : public Button {
public:
							SubmitButton();
	virtual					~SubmitButton();
							
	virtual const char*		GetTitle() const;
	virtual Boolean			IsSubmit() const;
		
	virtual Boolean			ExecuteInput();
	
	virtual void			SetAttributeStr(Attribute, const char*);

protected:
	char*					fUseForm;
};

class ResetButton : public Button {
public:
							ResetButton();
							
	virtual const char*		GetTitle() const;
	virtual Boolean			IsReset() const;
		
	virtual Boolean			ExecuteInput();
};

// ===========================================================================

#endif /*__BUTTON_H__ */