#include "Headers.h"
#include "IR.h"
#include "SystemGlobals.h"
#include "HWRegs.h"
#include "Input.h"
#include "BoxAbsoluteGlobals.h"

#ifdef DEBUG
	#ifdef APPROM
		#ifndef __SYSTEM_H__
		#include "System.h"
		#endif
		#ifndef __SCREEN_H__
		#include "Screen.h"
		#endif
		#ifndef __SONGDATA_H__
		#include "SongData.h"
		#endif
		#ifndef __GRAPHICS_H__
		#include "Graphics.h"
		#endif
		#ifndef __OPTIONSPANEL_H__
		#include "OptionsPanel.h"
		#endif
		#ifndef __PAGEVIEWER_H__
		#include "PageViewer.h"
		#endif
	#endif
	#include "HWDisplay.h"
#endif

#ifdef GOOBER
#include "GooberHeader.h"
#endif

#include "IRKeymap.c"

ushort *sej2lcp;		/* Hack.  Const makes those tables static? */

rawIR irData[kIRBufSize];
ulong irHead;
ulong irTail;

uchar irkbModifiers;
uchar irremoteModifiers;

uchar gIRRemoteDelay;
uchar gIRRemoteRate;

uchar gIRKBDelay;
uchar gIRKBRate;

uchar irFlags;

#define	kCapsDownFlag	0x01

static Boolean KeyCanRepeat(uchar key);
static void ParseAndPost(ulong button, ulong when);

static void ParseIRKBData(uchar flags, uchar data, ulong time);
static void IRKBMake(uchar flags, uchar data, ulong when);
static void IRKBBreak(uchar flags, uchar data, ulong when);

 
void InitIR(void)
{
ulong iii;

	Message(("Initializing IR receiver..."));

	for(iii=0;iii!=kIRBufSize;iii++)
	{
		irData[iii].kbData = 0;
		irData[iii].category1 = 0;
		irData[iii].category2 = 0;
		irData[iii].button = 0;
		irData[iii].time = 0;
	}

	irHead = irTail = 0;
	
	irremoteModifiers = 0;
	irkbModifiers = 0;
	irFlags = 0;
	
	gIRRemoteDelay = kDefaultIRRemoteDelay;
	gIRRemoteRate = kDefaultIRRemoteRate;

	gIRKBDelay = kDefaultIRKBDelay;
	gIRKBRate = kDefaultIRKBRate;
	
	iii = *(volatile ulong *)kIRDataReg;
	if((iii & 0x00ff0000) == 0x00690000)
	{
		WRITE_AG(agIRMicroVersion, ((iii >> 8) & 0xff));
	}
	else
	{
		WRITE_AG(agIRMicroVersion, 1);
	}
	
	Message(("IR Micro Version %x", READ_AG(agIRMicroVersion)));
}



#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void SetIRRemoteRepeat(uchar delay, uchar rate)
{
	gIRRemoteDelay = delay;
	gIRRemoteRate = rate;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void SetIRKBRepeat(uchar delay, uchar rate)
{
	gIRKBDelay = delay;
	gIRKBRate = rate;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GetIRRemoteRepeat(uchar *delay, uchar *rate)
{
	*delay = gIRRemoteDelay;
	*rate = gIRRemoteRate;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GetIRKBRepeat(uchar *delay, uchar *rate)
{
	*delay = gIRKBDelay;
	*rate = gIRKBRate;
}
#endif


Boolean KeyCanRepeat(uchar key)
{		
	/* return false;	*/

	switch (key)
	{
		case kOptionsButton:
		case kExecuteButton:
		case kBackButton:
		case kRecentButton:
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

static uchar dupcnt = 0;
static uchar reptratedups = 0;

#define	OLD_AGE				7


void IRIdle(void)
{
ulong irHeadCopy;
ulong behind;

	irHeadCopy = irHead;
	behind = irTail;

	while (irTail != irHeadCopy)
	{		
		irTail = (irTail + 1) & kIRBufMask;		/* int handler bumps, then stores */

		if ( !irData[irTail].kbData )			/* from remote or IR keyboard? */
		{
			irremoteModifiers &= ~kAutoRepeat;
		
			if ( (irData[irTail].category1 == kWebTVCategory1) && 
						(irData[irTail].category2 == kWebTVCategory2) )
			{
				if ( (irData[behind].category1 == kWebTVCategory1) && 
								(irData[behind].category2 == kWebTVCategory2) )
				{
					if (irData[behind].button != irData[irTail].button)
					{
						ParseAndPost(irData[irTail].button, irData[irTail].time);
						dupcnt = 0;
						reptratedups = 0;												
					}
					else
						if (irData[irTail].time > irData[behind].time + kMaxTicksForAutoRepeat)
						{
							ParseAndPost(irData[irTail].button, irData[irTail].time);
							dupcnt = 0;
							reptratedups = 0;												
						}
						else
							if (KeyCanRepeat(irData[irTail].button))	/* WRONG - toss keys that can't repeat */
							{	
								if (dupcnt == gIRRemoteDelay)
									if (reptratedups == gIRRemoteRate)	
									{
										irremoteModifiers |= kAutoRepeat;
										ParseAndPost(irData[irTail].button, irData[irTail].time);
										reptratedups = 0;
									}
									else
										reptratedups++;						
								else
									dupcnt++;			
							}
				}		
				else
					ParseAndPost(irData[irTail].button, irData[irTail].time);
			}

			UpdateGlobalModifiers(kWebTVIRRemote, irremoteModifiers);
		}
		else
		{
			ParseIRKBData(irData[irTail].kbData, irData[irTail].category1, irData[irTail].time);
			UpdateGlobalModifiers(kWebTVIRKeyboard, irkbModifiers);
		}

		behind = irTail;
	}		

}



static uchar irkbdupcnt = 0;
static uchar irkbreptratedups = 0;

static uchar lastKBFlags = 0;
static uchar lastKBData = 0;
static ulong lastKBTime = 0;

static void ParseIRKBData(uchar flags, uchar data, ulong time)
{
	if( ((flags & kSejinIDMask) == kSejinKeyboardID) &&	(data < kIRKeyMapTableSize) && data )				
	{
		irkbModifiers &= ~kAutoRepeat;

		if( !(flags & kSejinMakeBreakMask) )				/* Make (0) or Break (1)? */
		{
			if( data == lastKBData )						/* repeat? (break zeroes lastKBData) */
			{
				if( time < lastKBTime )						/* debounce */
					return;
			
				if( irkbdupcnt == gIRKBDelay )
				{
					if( irkbreptratedups == gIRKBRate )
					{
						irkbreptratedups = 0;				/* zero, let it post */
						irkbModifiers |= kAutoRepeat;
					}
					else
					{
						irkbreptratedups++;
						return;
					}
				}
				else
				{
					irkbdupcnt++;
					return;									/* don't post anything yet */
				}
			}
			else
			{
				irkbdupcnt = 0;
				irkbreptratedups = 0;
			}
	
			lastKBFlags = flags;
			lastKBData = data;
			lastKBTime = time + kIRKBDebounce;
			lastKBTime = time + 8;

			IRKBMake(flags, data, time);
		}
		else 								
			IRKBBreak(flags, data, time);
	}
}




/* Makes are decoded (with the metakeys' state taken into account) and posted.
   Autorepeat has already been handled by this point.
   
   Currently only ctl-A thru clt-Z are supported.  Does the browser need support for
   other combinations?
 */

static void IRKBMake(uchar flags, uchar data, ulong when)
{
Input input;
ushort d;

	input.rawData = (((ushort)flags<<8) | data);
	input.device = kWebTVIRKeyboard;
	input.modifiers = irkbModifiers;
	input.time = when;

	if( irkbModifiers & kShiftModifier )
		d = sej2uc[data];
	else
	{
		d = sej2lc[data];
			
		if ( irkbModifiers & kCapsModifier )
			if ((d >= 'a') && (d <= 'z'))
				d -= 0x20;
	}
				
	if( d & kIRKBSpecialKeyFlag )
	{
		switch(d)
		{
			case kIRKBLeftShift:
			case kIRKBRightShift:
							irkbModifiers |= kShiftModifier;
							break;
							
			case kIRKBLeftCmd:
			case kIRKBRightCmd:
							irkbModifiers |= kAltModifier;
							break;
							
			case kIRKBLeftControl:
			case kIRKBRightControl:
							irkbModifiers |= kControlModifier;
							break;
							
			case kIRKBCapsLock:
							if (!(irFlags & kCapsDownFlag))			/* toss autorepeats */
							{
								if(GetGlobalModifiers() & kCapsModifier)
									irkbModifiers &= ~kCapsModifier;
								else
									irkbModifiers |= kCapsModifier;
								irFlags |= kCapsDownFlag;
							}
							break;
		}
		
		return; 	/* nothing to post, just a modifier */
	}
		
	if( irkbModifiers & kControlModifier )
	{
		if ((d >= 'a') && (d <= 'z'))
			d -= 0x60;
		else							
			if ((d >= 'A') && (d <= 'Z'))
				d -= 0x40;
	}


	input.data = d;
	PostInput(&input);
}



/* Currently Breaks are not posted, only used to update metakey state.
   Does any part of the browser need keyups?
 */
 
static void IRKBBreak(uchar flags, uchar data, ulong when)
{
ushort d;

	if( data == lastKBData )		/* keep autorepeat logic from kicking in */
		lastKBData = 0;

	d = sej2uc[data];
	
	if( d & kIRKBSpecialKeyFlag )
	{
		switch(d)
		{
			case kIRKBLeftShift:
			case kIRKBRightShift:
							irkbModifiers &= ~kShiftModifier;
							break;
							
			case kIRKBLeftCmd:
			case kIRKBRightCmd:
							irkbModifiers &= ~kAltModifier;
							break;
							
			case kIRKBLeftControl:
			case kIRKBRightControl:
							irkbModifiers &= ~kControlModifier;
							break;

			case kIRKBCapsLock:
							irFlags &= ~kCapsDownFlag;
							break;
		}
	}
}


static void ParseAndPost(ulong button, ulong when)
{
Input input;

	input.rawData = button;
	input.device = kWebTVIRRemote;
	input.modifiers = irremoteModifiers;
	input.time = when;
	
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
#if defined(APPROM) && defined(DEBUG)
								gSystem->SetUseFlickerFilter(!gSystem->GetUseFlickerFilter());
								
								gPageViewer->InvalidateBounds();
								gOptionsPanel->InvalidateBounds();
								gScreen->RedrawNow();
								
								if (gSystem->GetUseFlickerFilter())
									gSaveSound->Play();
								else
									gLimitSound->Play();
								return;
#else /* of #if defined(APPROM) && defined(DEBUG) */
								input.data = kRemoteEnterKey;
								break;
#endif /* of #else of #if defined(APPROM) && defined(DEBUG) */
		default:				return;
	}

	PostInput(&input);
}

#if !defined(BOOTROM) && !defined(APPROM)
ulong PollIR(void)
{
ulong retval = 0;

	*(ulong *)kBusIntEnableSetReg 		= kIRIntMask;			

	if(*(ulong *)kBusIntStatusReg & kIRIntMask)
	{
		retval = *(ulong *)kIRDataReg;
		*(ulong *)kBusIntStatusClearReg = kIRIntMask;		
	}
	
	*(ulong *)kBusIntEnableClearReg 	= kIRIntMask;							
	
	return retval;
}
#endif
