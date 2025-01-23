// ===========================================================================
//	Input.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __INPUT_H__
#define __INPUT_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

class Layer;

//==============================================================================
// Remote Input Codes
// (PC Keyboard can also generate these with remote button emulation keys)

#define kUpKey				0x8000
#define kLeftKey			0x8001
#define kRightKey			0x8002
#define kDownKey			0x8003
#define kOptionsKey			0x8004
#define kExecuteKey			0x8005
#define kPowerKey			0x8006
#define kForwardKey			0x8007
#define kBackKey			0x8008
#define kScrollUpKey		0x8009
#define kScrollDownKey		0x800a
#define kRecentKey			0x800b
#define kHomeKey			0x800c
#define kFavoriteKey		0x800d
#define	kRemoteEnterKey		0x800e

#define IsKeyboardInput(input)		(!(input->data & 0x8000))

//==============================================================================
// Function Key Codes (from IR or PC keyboard)

#define kF1					0x3b
#define kF2					0x3c
#define kF3					0x3d
#define kF4					0x3e
#define kF5					0x3f
#define kF6					0x40
#define kF7					0x41
#define kF8					0x42
#define kF9					0x43
#define kF10				0x44
#define kF11				0x57
#define kF12				0x58

//==============================================================================
// SmartCard Input Codes

#define kSmartCardNoEvent	0x1000
#define kSmartCardInserted	0x1001
#define kSmartCardRemoved	0x1002

//==============================================================================
// Standard Input Devices
// Other devices may become available - PC keyboard port joysticks, Expansion
//  Bus devices, etc.

#define	kWebTVIRRemote		1
#define	kWebTVIRKeyboard	2
#define	kPCKeyboard			3
#define	kWebTVSmartCard		4


#define	kSoftKeyboard		128		/* Virtual devices have the high bit set */

//==============================================================================
// Modifier Flags

#define	kShiftModifier		0x01
#define	kCapsModifier		0x02
#define	kControlModifier	0x04
#define	kAltModifier		0x08

#define	kKeyUp				0x10		// Does anyone need this?
#define	kAutoRepeat			0x20

//==============================================================================
// Input Queue Management

class Input {
public:
	uchar	device;			// who's it from?
	uchar	modifiers;		// device-independent modifiers
	ushort	data;			// 2 bytes of data
	ulong	rawData;		// Raw, device-specific event data
	ulong	time;			// timestamp from interrupt handler, in ticks (VBLs)
};


void	PostInput(Input* input);	// post one of the input codes above
Boolean	NextInput(Input* input);	// returns false if q is empty
void	FlushInput(void);				// clear the input queue

#ifdef SIMULATOR
void UserLoopIteration(void);
void CheckAbort(void);
#else
void UserTaskMain(void);
#define CheckAbort() 	 _void
#endif

//==============================================================================
// Global Modifier Key Management
//
// There may be several input devices, each with its own set of modifier keys.
// Posted inputs from each device carry the modifiers from that device.
// For some things (like updating the CapsLock indicator on the screen *and* on
// an attached PC keyboard) the union of all of the input devices' modifiers 
// is interesting.

uchar 	GetGlobalModifiers(void);
void 	UpdateGlobalModifiers(uchar device, uchar modifiers);

Boolean CapsLockKeyDown(void);

//==============================================================================
// HandlesInput

class HandlesInput {
public:
	virtual Boolean			BackInput();
	virtual Boolean			DispatchInput(Input*);
	virtual Boolean			DownInput(Input*);
	virtual Boolean			ExecuteInput();
	virtual Boolean			ForwardInput();
	virtual Boolean			HomeInput();
	virtual Boolean			KeyboardDownInput(Input*);
	virtual Boolean			KeyboardInput(Input*);
	virtual Boolean			KeyboardLeftInput(Input*);
	virtual Boolean			KeyboardRightInput(Input*);
	virtual Boolean			KeyboardUpInput(Input*);
	virtual Boolean			LeftInput(Input*);
	virtual Boolean			OptionsInput();
	virtual Boolean			PowerInput();
	virtual Boolean			RecentInput();
	virtual Boolean			RightInput(Input*);
	virtual Boolean			ScrollDownInput(Input*);
	virtual Boolean			ScrollUpInput(Input*);
	virtual Boolean			SearchInput();
	virtual Boolean			SmartCardInsertInput();
	virtual Boolean			SmartCardRemovedInput();
	virtual Boolean			UpInput(Input*);
};

//==============================================================================

#endif /* __INPUT_H__ */
