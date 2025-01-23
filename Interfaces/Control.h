// ===========================================================================
//	Control.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CONTROL_H__
#define __CONTROL_H__

#ifndef __IMAGE_H__
#include "Image.h"
#endif

// ===========================================================================
class Animation;
class BorderImage;
class Control;
class Form;
class ImageMap;
class ImageMapSelectable;
class Input;

// ===========================================================================

class Control : public SpatialDisplayable {
public:
							Control();
	virtual 				~Control();
		
	virtual Boolean			ExecuteInput();
	virtual AttributeValue	GetAlign() const;
	virtual const char*		GetID() const;
	const char*				GetName() const;
	virtual Displayable*	GetParent() const;
	ContentView*			GetView() const;
	virtual Boolean			IsAutoSubmit() const;
	virtual Boolean			IsHighlightable() const;
	virtual Boolean			IsInitiallySelected() const;
	virtual Boolean			IsRadio() const;
	virtual Boolean			IsReset() const;
	virtual Boolean			IsSelectable() const;
	virtual Boolean			IsSelectableInputImage() const;
	virtual Boolean			IsSubmit() const;
	
	void					SetForm(Form*);
	
	virtual void			AddSubmission(DataStream*, Boolean force);	// parameter specifies whether to force a submission string, even for false value
	virtual void			GetSelectionRegion(Region* region) const;
	virtual Boolean			Idle(Layer*);
	void					InvalidateBounds();
	virtual Boolean			IsLayoutComplete() const;
	virtual Boolean			KeyboardInput(Input*);
	virtual void			LayoutComplete(Document*, Displayable* parent);
	virtual void			ResetLayout(Document*);
	virtual Boolean			Reset();
	virtual void			RestoreFillinData(const char* data);
	virtual const char*		SaveFillinData() const;
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
	virtual void			SetParent(Displayable*);
	
protected:
	void					MoveToNextControl();
	void					MoveToPreviousControl();
	
	char*					fAction;
	Form*					fForm;
	char*					fID;
	char*					fName;
	Displayable*			fParent;
	char*					fValue;
	
	short					fKnownWidth;

	unsigned				fChecked : 1;
	unsigned				fIsInitiallySelected : 1;
	unsigned				fIsWidthPercentage : 1;
	unsigned				fLayoutComplete : 1;
	unsigned				fNoHighlight : 1;
};

// ===========================================================================

class InputImage : public Control {
public:
							InputImage();
	virtual					~InputImage();

	virtual void			AddSubmission(DataStream*, Boolean force);
	virtual void			Draw(const Document*, const Rectangle*);	
	virtual Boolean			ExecuteInput();
	virtual AttributeValue	GetAlign() const;
	virtual void			GetBoundsTight(Rectangle*) const;
	virtual long			GetLeft() const;
	virtual long			GetHeight() const;
	virtual ImageMap*		GetImageMap() const;
	virtual Image*			GetMappedImage() const;
	ImageMapSelectable*		GetSelectable() const;
	virtual long			GetTop() const;
	virtual long			GetWidth() const;
	virtual Boolean			Idle(Layer*);
	virtual Boolean			IsFloating() const;
	virtual Boolean			IsSelectableInputImage() const;
	virtual Boolean			IsSubmit() const;

	virtual Boolean			IsLayoutComplete() const;
	virtual void			Layout(Document*, Displayable* parent);
	virtual void			LayoutComplete(Document*, Displayable* parent);
	virtual Boolean			ReadyForLayout(const Document*);
	virtual void			ResetLayout(Document*);
	
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
	virtual	void			SetLeft(long);
	virtual	void			SetParent(Displayable*);
	virtual	void			SetTop(long);
	
protected:
	Image*					fImage;
	ImageMapSelectable*		fSelectable;
	Animation*				fActionImage;
	
	unsigned				fStartedActionImage : 1;
	unsigned				fUseCursor : 1;
};

// ===========================================================================

class CheckBox : public Control {
public:
							CheckBox();

	virtual void			Commit(long value);
	virtual void			Draw(const Document* document, const Rectangle* invalid);
	virtual Boolean			ExecuteInput();
	virtual Boolean			Reset();
	virtual void			AddSubmission(DataStream*, Boolean force);

	virtual void			RestoreFillinData(const char* data);
	virtual const char*		SaveFillinData() const;
	
protected:
	virtual void			Layout(Document*, Displayable* parent);

protected:
	Boolean					fOn;
};

// ===========================================================================

class InputHidden : public Control {
public:
							InputHidden();
							~InputHidden();
							
	AttributeValue			GetAutoSubmitType() const;
	
	virtual Boolean			IsSelectable() const;
	virtual Boolean			IsAutoSubmit() const;
	virtual Boolean			IsSubmit() const;

protected:
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	
	AttributeValue			fAutoSubmit;
};

// ===========================================================================

inline AttributeValue
InputHidden::GetAutoSubmitType() const
{
	return fAutoSubmit;
}

// ===========================================================================
#endif /*__CONTROL_H__ */