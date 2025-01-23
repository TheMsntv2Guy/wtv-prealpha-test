// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#include "Headers.h"
#include "Layer.h"

#ifndef __INPUT_H__
#include "Input.h"
#endif

#ifndef __TELLYIO_H__
#include "TellyIO.h"
#endif

#ifdef HARDWARE
#include "HWKeyboard.h"
#endif

#include "BoxUtils.h"

#include "System.h"

// =============================================================================

#define kInputQueueSize 32

static Input gInputQueue[kInputQueueSize];

static ulong gHeadIndex = 0, gTailIndex = 0;

#if kInputQueueSize == 32
#define ADVANCE_COUNTER(counter) counter++; counter &= (kInputQueueSize - 1)
#else
#define ADVANCE_COUNTER(counter) counter++; counter %= kInputQueueSize
#endif

#define InputQueueNotEmpty()	(gHeadIndex != gTailIndex)
#define InputQueueEmpty()		(gHeadIndex == gTailIndex)

// =============================================================================

Boolean 
NextInput(Input *input)
{
	PostulateFinal(false);						// needs to be made to block	
	
	if (InputQueueNotEmpty()) {
		*input = gInputQueue[gTailIndex];		// copy input event struct
		ADVANCE_COUNTER(gTailIndex);
		return true;
	} else {
		input->rawData = 0;
		input->data = 0;
		input->device = 0;
		input->modifiers = 0;		
		return false;
	}
}

// =============================================================================

void 
PostInput(Input *input)
{
	// If the queue is not empty, and the current input is repeated, 
	// don't add it. We do not want to queue up repeated keys.
	if (InputQueueNotEmpty() && (input->modifiers & kAutoRepeat))
		return;
		
	gInputQueue[gHeadIndex] = *input;	// copy input event struct
	ADVANCE_COUNTER(gHeadIndex);

	if (gHeadIndex == gTailIndex)	{	// drop oldest input if wrap because queue full
		ADVANCE_COUNTER(gTailIndex);
	}
	
#ifdef FOR_MAC
	PostEvent(nullEvent, 0);			// wake up out of WaitNextEvent
#endif
}
	
// =============================================================================

void 
FlushInput(void)
{
	gHeadIndex = 0;
	gTailIndex = 0;
}

// =============================================================================
// For Shift, Control, and Alt, the global modifiers are just the union.
// Caps is different - we have to catch a 0->1 transition by any device and only
// update the global caps state on that.
//

uchar irKBMods = 0;
uchar pcKBMods = 0;

uchar globalMods = 0;

uchar GetGlobalModifiers(void)
{
	return globalMods;
}

void UpdateGlobalModifiers(uchar USED_FOR_HARDWARE(device), uchar USED_FOR_HARDWARE(modifiers))
{
#ifdef HARDWARE
uchar oldMods;
uchar capsChanged = false;

	oldMods = globalMods;
	
	switch(device)
	{
		case kWebTVIRKeyboard:
						capsChanged = ((irKBMods ^ modifiers) & kCapsModifier);
						irKBMods = modifiers;
						break;
		
		case kPCKeyboard:
						capsChanged = ((pcKBMods ^ modifiers) & kCapsModifier);
						pcKBMods = modifiers;
						break;
	}
	
	globalMods = ((irKBMods | pcKBMods) & ~kCapsModifier);
	
	if(capsChanged)
		globalMods |= (modifiers & kCapsModifier);
	else
		globalMods |= (oldMods & kCapsModifier);
			
	/* If we have a PC KB, update its capslock led */
	
	if ((oldMods ^ globalMods) & kCapsModifier)
	{
		if (modifiers & kCapsModifier)
			SetHWKBLEDs(GetHWKBLEDs() | kCapLED);
		else
			SetHWKBLEDs(GetHWKBLEDs() & ~kCapLED);
	}
#endif
}


#ifdef HARDWARE
Boolean CapsLockKeyDown(void)
{
	if(globalMods & kCapsModifier)
		return true;
	else
		return false;
}
#endif


// =============================================================================
// class HandlesInput

#ifdef HARDWARE
#ifdef APPROM
#include "HWRegs.h"
#include "FlashStorage.h"

void Cylon(void);

void Cylon(void)
{
ulong oldLEDs = GetBoxLEDs();
ulong cyl;
ulong xxx,yyy;
ulong oldTix;

	for(yyy=0;yyy!=2;yyy++)
	{
		cyl = 1;
		
		for(xxx=0;xxx!=4;xxx++)
		{
			SetBoxLEDs(cyl);
			cyl<<=1;
				
			oldTix = (Now() + 5);
			while(Now() < oldTix)
				;
		}
	}
	
	SetBoxLEDs(oldLEDs);
}

#endif /* APPROM */
#endif /* HARDWARE */


#include "HWAudio.h"		/* part of the hack to be cleaned up, described below */
#include "HWDisplay.h"
#include "Exceptions.h"
#include "BoxHWEquates.h"
#include "CrashLogC.h"


Boolean 
HandlesInput::DispatchInput(Input* input)
{
#ifdef PRERELEASE
	// Allow non-blocking for now.
	if (input->data == 0)
		return false;
#endif

#ifdef HARDWARE
#ifdef APPROM
	{
	static long check;
	static long expn = 1;
	static ulong oldtime;
	long m = 1;
	long i;

	if (!gSystem->GetIsOn() && input->data != kPowerKey)
		{
		PostulateFinal(false);

		switch (input->data)
			{
#ifdef DEBUG
			case kExecuteKey:
				/* ugly hack to force the box to ask the user what number should be
				   dialed.  useful for when we're doing am international roadshow */
				FlashBoxLED(kBoxLEDPower,2);
				gSystem->SetUsingConnectSetup(true);
				break;
			case kUpKey:
				// special hack to turn off printf
				FlashBoxLED(kBoxLEDConnect,2);
				gPrintEnable = false;
				break;
#endif				
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (input->time > (oldtime + 120))		/* reset after 2 seconds of silence */
					m = 1, check = 0, expn = 1;

				oldtime = input->time;

				for (i = 1; i <= expn; i++)
					m *= input->data - 46;				/* be careful to avoid 0's and 1's */
				check += m;
				expn++;

				switch (check)
					{
					case 0x1998e:											/* 32768 */
						Message(("Resetting flash!!"));
						m = 1, check = 0, expn = 1;
						Cylon();
						NVInit(kNVPhaseSaveFB);				/* alloc buf, zero */		
						NVCommit();							/* update checksum, write, free buf */
						Reboot(kWarmBoot);
						break;
					case 0x2e6:												/* 217 */
						Message(("Phone settings triggered..."));
						m = 1, check = 0, expn = 1;
						Cylon();
						gSystem->SetUsingPhoneSetup(true);
						break;
					case 0x3b9e:											/* 7869 */
						Message(("Flash download triggered..."));
						m = 1, check = 0, expn = 1;
						Cylon();
	
						/* this is a hack, clean it up, Joe! */
						KillDisplay();
	
						NVSetFlags(NVGetFlags() | kUserReqFlashUpdate);
						
						Reboot(kColdBoot);	/* here we go! */
						break;
					}
				break;
			default:
				break;
			}
		// Always gobble up the input (except kPowerKey) when the system is off.
		return true;
		}
	}
#endif /* APPROM */
#endif /* HARDWARE */

	if (input->device != kWebTVIRRemote && !(input->modifiers & kAltModifier)) {
		switch (input->data) {
			case kDownKey:			return KeyboardDownInput(input);
			case kLeftKey:			return KeyboardLeftInput(input);
			case kRightKey:			return KeyboardRightInput(input);
			case kUpKey:			return KeyboardUpInput(input);
		}
	}
	
	switch (input->data) {
		case kBackKey:			return BackInput();
		case kDownKey:			return DownInput(input);
		case kExecuteKey:		return ExecuteInput();
		case kForwardKey:		return ForwardInput();
		case kHomeKey:			return HomeInput();
		case kLeftKey:			return LeftInput(input);
		case kOptionsKey:		return OptionsInput();
		case kPowerKey:			return PowerInput();
		case kRecentKey:		return RecentInput();
		case kRightKey:			return RightInput(input);
		case kScrollDownKey:	return ScrollDownInput(input);
		case kScrollUpKey:		return ScrollUpInput(input);
		case kUpKey:			return UpInput(input);
			
		case kSmartCardInserted: return SmartCardInsertInput();
		case kSmartCardRemoved:  return SmartCardRemovedInput();
		
		default:
			if (IsKeyboardInput(input))
				return KeyboardInput(input);
	}
	
	return false;
}

// Default input handlers:
Boolean HandlesInput::BackInput()						{ return false;}
Boolean HandlesInput::DownInput(Input*)					{ return false;}
Boolean HandlesInput::ExecuteInput()					{ return false;}
Boolean HandlesInput::ForwardInput()					{ return false;}
Boolean HandlesInput::HomeInput()						{ return false;}
Boolean HandlesInput::KeyboardDownInput(Input* input)	{ return DownInput(input);}
Boolean HandlesInput::KeyboardInput(Input*) 			{ return false;}
Boolean HandlesInput::KeyboardLeftInput(Input* input)	{ return LeftInput(input);}
Boolean HandlesInput::KeyboardRightInput(Input* input)	{ return RightInput(input);}
Boolean HandlesInput::KeyboardUpInput(Input* input)		{ return UpInput(input);}
Boolean HandlesInput::LeftInput(Input*)					{ return false;}
Boolean HandlesInput::OptionsInput()					{ return false;}
Boolean HandlesInput::PowerInput()						{ return false;}
Boolean HandlesInput::RecentInput()						{ return false;}
Boolean HandlesInput::RightInput(Input*)				{ return false;}
Boolean HandlesInput::ScrollDownInput(Input*)			{ return false;}
Boolean HandlesInput::ScrollUpInput(Input*)				{ return false;}
Boolean HandlesInput::SearchInput()						{ return false;}
Boolean HandlesInput::SmartCardInsertInput()			{ Message(("HandlesInput::SmartCardInsertInput")); return false;}
Boolean HandlesInput::SmartCardRemovedInput()			{ Message(("HandlesInput::SmartCardRemovedInput")); return false;}
Boolean HandlesInput::UpInput(Input*)					{ return false;}

	
// =============================================================================
