// ===========================================================================
//	TextField.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __TEXTFIELD_H__
#define __TEXTFIELD_H__

#ifndef __CONTROL_H__
#include "Control.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif

// ===========================================================================

class BorderImage;
class Control;
class Form;

// ===========================================================================

class TextField : public Control {
public:
							TextField();
	virtual 				~TextField();

	ContentView*			GetContainer() const;
	Boolean					GetExecuteURL() const;
	ulong					GetLength() const;
	const char*				GetText() const;
	Boolean					HasNumbersFirst() const;
	virtual Boolean			IsSelectable() const;
	Boolean					IsShifted() const;
	virtual Boolean			IsTextField() const;

	void					AddDefaultText(const char*);
	virtual void			AddSubmission(DataStream*, Boolean force);
	void					AddText(const char*);
	void					Backspace();
	void					ClearText();
	virtual void			Deselect();
	virtual void			Draw(const Document* document, const Rectangle*);
	virtual Boolean			ExecuteInput();
	void					Enter();
	void					GetCursorBounds(ContentView*, Rectangle* cursorBounds) const;
	void					GetTextBounds(Rectangle*) const;
	virtual Boolean			Idle(Layer* layer);
	void					InsertChar(uchar c);
	void					InvalidateBounds(Boolean drawNow);
	virtual Boolean			KeyboardInput(Input*);
	virtual void			Layout(Document*, Displayable* parent);
	void					MakeActive();
	Boolean					MoveCursorDown();
	Boolean					MoveCursorLeft();
	Boolean					MoveCursorRight();
	Boolean					MoveCursorUp();
	void					Next();
	void					Previous();
	virtual Boolean			Reset();
	void					Return();
	virtual void			Select();
	virtual void			SelectFirst();
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);

	virtual void			RestoreFillinData(const char* data);
	virtual const char*		SaveFillinData() const;
	void					SetExecuteURL(Boolean);
	void					SetIsTextArea(Boolean);
	void					SetShifted(Boolean);
	
protected:
	virtual void			DrawText(XFont font,CharacterEncoding encoding, char* text, ulong length, Ordinate x, Ordinate y, 
											const Rectangle* invalid);
	virtual void			DrawTextInBox(XFont font,CharacterEncoding encoding, char* text, ulong Length, Ordinate x, Ordinate y, 
											ulong lineHeight, const Rectangle* invalid);

	BorderImage*			GetBorder() const;
	XFont					GetFont(const Document*) const;
	long					GetLeftSpace() const;
	long					GetTopSpace() const;
	long					GetRightSpace() const;
	long					GetBottomSpace() const;
	long					GetRowCount() const;
	char*					GetVisibleText() const;
	Boolean					HandleEnterKey();
	void					HandleRowChanged();
	void					HandleWordWrap();
	void					MakeSpaceAvailable(ulong count);
	void					MoveToNextField();
	void					RefreshAfterInput();
	void					RefreshCapsIndicator(Boolean firstTime = false);
	void					UpdateFirstPosition();
	
protected:
	Color					fBackgroundColor;
	BorderImage*			fBorderImage;
	Color					fTextColor;
	Color					fCursorColor;
	long					fCursorPosition;
	long					fFirstPosition;
	ulong					fLastIdle;
	long					fMaxLength;
	char*					fText;
	ulong					fTextLength;

	short					fKnownColumns;
	short					fKnownRows;
	short					fRows;
	
	unsigned				fAllCaps : 1;
	unsigned				fAutoCaps : 1;
	unsigned				fExecuteURL : 1;
	unsigned				fGrowable : 1;
	unsigned				fHasBorder : 1;
	unsigned				fHasNumbersFirst : 1;
	unsigned				fIsTextArea : 1;
	unsigned				fNoSoftBreaks : 1;
	unsigned				fProportionalFont : 1;
	unsigned				fShift : 1;
	unsigned				fShouldDrawCursor : 1;
	unsigned				fShowKeyboard : 1;
};

// ===========================================================================

inline ulong 	TextField::GetLength() const	{ return (fText != nil ? strlen(fText) : 0); }
inline void		TextField::SetIsTextArea(Boolean isArea)	{ fIsTextArea = isArea; }

// ===========================================================================

class InputPassword : public TextField {
public:
							InputPassword();
	virtual					~InputPassword();
	
protected:
	virtual void			DrawText(XFont font,CharacterEncoding encoding, char* text, ulong length, Ordinate x, Ordinate y, 
									 const Rectangle* invalid);
};

// ===========================================================================
#endif /*__TEXTFIELD_H__ */