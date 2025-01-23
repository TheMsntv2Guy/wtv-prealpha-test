// ===========================================================================
//	RadioButton.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __RADIOBUTTON_H__
#define __RADIOBUTTON_H__

#ifndef __CONTROL_H__
#include "Control.h"
#endif

// ===========================================================================

class RadioButton : public CheckBox {
public:
							RadioButton();
							
	virtual Boolean			IsRadio() const;

	virtual void			AddSubmission(DataStream*, Boolean force);
	virtual void			Commit(long value);
	virtual void			Draw(const Document* document, const Rectangle* invalid);
	virtual Boolean			ExecuteInput();

protected:
	virtual void			Layout(Document*, Displayable* parent);
};

// ===========================================================================

#endif /*__RADIOBUTTON_H__ */