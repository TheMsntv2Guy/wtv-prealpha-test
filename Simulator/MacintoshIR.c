/* ===========================================================================
	MacintoshIR.c
	
	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#include "Headers.h"

#ifndef __SERIAL__
#include <serial.h>
#endif
#ifndef __INPUT_H__
#include "Input.h"
#endif
#ifndef __IR_H__
#include "IR.h"
#endif
#ifndef __MACINTOSHIR_H__
#include "MacintoshIR.h"
#endif
#ifndef __SERIAL_H__
#include "Serial.h"
#endif




/* ===========================================================================
	structs/#defines
=========================================================================== */

typedef struct
{
	long			a5;
	IOParam			pb;
} AsyncParameterBlock;


typedef struct {	
	uchar pad1;
	uchar button;
	ulong time;
} simIR;

/*
 *	Data we'll get from the IR receiver
 *
 *	The Serial IR receiver filters out all buttons not from a WebTV remote.
 *
 */
 
#define	kUpButton			0x74
#define	kDownButton			0x75
#define	kLeftButton			0x34
#define	kRightButton		0x33

#define	kOptionsButton		0x61
#define	kExecuteButton		0x65

#define	kBackButton			0x4C
#define	kRecentButton		0x4B
#define	kScrollUpButton		0x58
#define	kScrollDownButton	0x59

#define	kHomeButton			0x62
	
#define	kPowerButton		0x15

/*
 *	External Stuff
 */
	
extern Boolean		gTerminate;
extern short		gIROutRefNum, gIRInRefNum;		/* defined in Serial.c */

/*
 *	Protos
 */
 
static	OSErr 	SerialCompletionProc(IOParam *pb);
static	void 	ParseAndPost(ulong button, ulong time);

Boolean KeyCanRepeat(uchar key);
void 	IRIdle(void);
void 	MacintoshIRInitialize(void);
void 	MacintoshIRFinalize(void);
Boolean MacintoshIRInitialized(void);

/* 
 * 	Globals
 */
	
AsyncParameterBlock	gAsyncPB;

#define kNumIRBufEntries	128
#define	kIRDataBufferMask	0x7f

simIR 		irData[kNumIRBufEntries];
ulong 		irHead;
ulong 		irTail;

uchar		gIRBuffer;
ulong		gNexIRTime = 0;
Boolean		gMacintoshIREnabled = false;

#define Ticks	0x16a

IOCompletionUPP  serCompUPP = NewIOCompletionProc((void *)SerialCompletionProc);





/* ===========================================================================
	implementation
=========================================================================== */

#if USES68KINLINES
#pragma parameter __D0 SerialCompletionProc(__A0)
#endif
	
OSErr SerialCompletionProc(IOParam *pb)
{
long	SavedA5 = SetA5(((AsyncParameterBlock *)((char *)pb - 4))->a5);

	if (gTerminate)							/* are we shutting down? */
		goto Done; 
		
	if (pb->ioResult == readErr)			/* did that last read suck? */
	{ 
		KillIO(gIRInRefNum); 
		goto Done; 
	}
	
	if (pb->ioResult == noErr)
	{
		irData[irHead].button = gIRBuffer;
		irData[irHead].time = *(ulong *)Ticks;

		irHead++;						/* head points to next free slot */
		irHead &= kIRDataBufferMask;
			
		if (irHead == irTail)			/* buffer wrapped? */
		{
			irHead--;
			irHead &= kIRDataBufferMask;
		}
	}
	
	PBReadAsync((ParmBlkPtr)pb);			/* fire off another read, same params as b4 */

Done:		
	(void)SetA5(SavedA5);
	return(pb->ioResult);
}



Boolean KeyCanRepeat(uchar key)
{
	switch (key)
	{
		case kOptionsButton:
		case kExecuteButton:
		case kBackButton:
		case kRecentButton:
		case kHomeButton:
		case kPowerButton:
							return false;
		default:
							return true;
	}
}


/*
 *	Idle Task
 */

/* 	Should be called periodically (like once per main loop).
	Parses the interrupt IR data buffer & posts appropriate input.
*/

ulong dupcnt = 0;
ulong reptratedups = 0;

#define	DUPS_TO_REPEAT		4
#define	REPEAT_RATE_DUPS	1
#define	OLD_AGE				7


void IRIdle(void)
{
ulong irHeadCopy;
ulong behind;


	irHeadCopy = irHead;
	behind = (irTail-1) & kIRDataBufferMask;

	while (irTail != irHeadCopy)
	{		
		if (irData[behind].button != irData[irTail].button)					/* is this key same as last? */
		{
			ParseAndPost(irData[irTail].button, irData[irTail].time);		/* new key, post it */
			dupcnt = 0;														/* new key, so no duplicates yet */
			reptratedups = 0;												
		}
		else
			if (irData[irTail].time >= irData[behind].time + OLD_AGE)		/* how long since last (same) key? */
				ParseAndPost(irData[irTail].button, irData[irTail].time);	/* long enough to be a nonrepeat */
			else
				if (KeyCanRepeat(irData[irTail].button))					/* can this key repeat? */
				{		
					if (dupcnt == DUPS_TO_REPEAT)							/* have we seen enough to start repeating? */	
						if (reptratedups == REPEAT_RATE_DUPS)	
						{
							ParseAndPost(irData[irTail].button, irData[irTail].time);		
							reptratedups = 0;
						}
						else
							reptratedups++;						
					else
						dupcnt++;			
				}
				
		behind = irTail;
		irTail = (irTail + 1) & kIRDataBufferMask;						/* next... */
	}		

}



static void ParseAndPost(ulong button, ulong time)
{
	Input input;

	input.rawData = button;
	input.device = kWebTVIRRemote;
	input.modifiers = 0;
	input.time = time;
	
	switch(button)
	{
		case kUpButton:			input.data = kUpKey;
								break;
		case kDownButton:		input.data = kDownKey;
								break;
		case kLeftButton:		input.data = kLeftKey;
								break;
		case kRightButton:		input.data = kRightKey;
								break;
		case kOptionsButton:	input.data = kOptionsKey;
								break;
		case kExecuteButton:	input.data = kExecuteKey;
								break;
		case kBackButton:		input.data = kBackKey;
								break;
		case kRecentButton:		input.data = kRecentKey;
								break;
		case kScrollUpButton:	input.data = kScrollUpKey;
								break;
		case kScrollDownButton:	input.data = kScrollDownKey;
								break;
		case kHomeButton:		input.data = kHomeKey;
								break;
		case kPowerButton:		input.data = kPowerKey;
								break;
								
		case 9:
								input.data = '0';
								break;
		case 0:
								input.data = '1';
								break;
		case 1:
								input.data = '2';
								break;
		case 2:
								input.data = '3';
								break;
		case 3:
								input.data = '4';
								break;
		case 4:
								input.data = '5';
								break;
		case 5:
								input.data = '6';
								break;
		case 6:
								input.data = '7';
								break;
		case 7:
								input.data = '8';
								break;
		case 8:
								input.data = '9';
								break;
		case 11:
								input.data = kRemoteEnterKey;
								break;
								
		default:				return;
	}

	PostInput(&input);
}



/*
 *	Setup stuff
 */
  
void MacintoshIRInitialize(void)
{
long myA5;

	SerialInitialize(kIR_PORT);			
	gMacintoshIREnabled = true;

	myA5 = SetA5(0);
	SetA5(myA5);
	gAsyncPB.a5 = myA5;

	/* Kickstart it */
	SerialReadAsync(kIR_PORT, &gAsyncPB.pb, 1, &gIRBuffer, serCompUPP);

	Message(("IR Enabled."));
}


void MacintoshIRFinalize(void)
{
	SerialFinalize(kIR_PORT);
	gMacintoshIREnabled = false;
}


Boolean MacintoshIRInitialized(void)
{
	return gMacintoshIREnabled;
}
	


