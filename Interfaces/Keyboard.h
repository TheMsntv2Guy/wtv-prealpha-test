// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif

class GIFImage;

// =============================================================================

enum CommandName {
	kBackspaceCmd,
	kClearCmd,
	kEnterCmd,
	kShiftCmd,
	kNextCmd,
	kPreviousCmd
};

enum CursorDirection {
	kCursorDown,
	kCursorLeft,
	kCursorRight,
	kCursorUp
};

enum KeyboardType {
	kAlphabeticKeyboard = 1,
	kStandardKeyboard = 2
};

// =============================================================================

class Keyboard : public ContentView {
public:
							Keyboard();
	virtual					~Keyboard();
	
	Boolean					IsNumeric() const;

	virtual Boolean			BackInput();
	virtual void			ChangeIdleState(ViewIdleState newState, ViewIdleState oldState);
	void					ClearTargetViewOffset();
	void					Close(Boolean closePanel = false);
	Boolean					ContainsTypingCommand(const char*);
	virtual void			Draw(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	long					GetKeyboardType();
	void					InsertChar(char);	
	void					InstallKeyboard(KeyboardType keyboard);
	Boolean					KeyboardInput(Input*);
	void					MoveCursor(CursorDirection);
	void					Open();
	virtual Boolean			ScrollDownInput(Input*);
	virtual Boolean			ScrollUpInput(Input*);
	void					TextCommand(CommandName);
	
protected:
	virtual Selection		PreviousVisibleAnchorAbove(Selection current, Boolean wrap = false,
													   Boolean requireSibling = true);

	ImageData*				fCapsOff;
	ImageData*				fCapsOn;
	long					fTargetViewOffset;
	KeyboardType			fKeyboard;
	Boolean					fNotDrawnYet;
};

// =============================================================================

inline void Keyboard::ClearTargetViewOffset()
{
	fTargetViewOffset = 0;
}

inline long Keyboard::GetKeyboardType()
{
	return (long)fKeyboard;
}

// =============================================================================

#endif /* __KEYBOARD_H__ */
