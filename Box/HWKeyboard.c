#include "WTVTypes.h"
#include "SystemGlobals.h"
#include "Input.h"

#include "boxansi.h"
#include "Debug.h"
#include "Interrupts.h"

#include "HWKeyboard.h"
#include "HWRegs.h"

#include "HWKeymap.c"

#ifdef DEBUG
#include "HWDisplay.h"
#endif

#ifndef GOOBER
#include "Debug.h"
#endif


#define	kKBCtlTimeout	312000		/* ¥¥¥ÊFIXME: use the microsec timer for the timeout */
#define	kKBErr			0xff


uchar kbData[kKybdBufSize];
ulong kbHead;
ulong kbTail;

static uchar kbModifiers;
static uchar hwkbFlags;

#define	kCapsDownFlag	0x01

static uchar kbLEDs;

static uchar gStatus;		/* set by kb cmd funcs that don't care about or return status */

static ulong gReadDataTimeout = kKBCtlTimeout;
static ulong gWriteDataTimeout = kKBCtlTimeout;
static ulong gWriteCmdTimeout = kKBCtlTimeout;

static Boolean 	ResetKeyboard(void);
#if 0
static void 	SetTypematic(uchar newtm);
static void 	EnableKeyboard(void);
static uchar 	ReadCommandByte(void);
#endif
static void 	WriteCommandByte(uchar cmdbyte);
static Boolean 	ControllerSelfTestOK(void);
static void 	ControllerEnableKeyboard(void);


static uchar 	ReadData(void);
static void 	WriteData(uchar data);
static void 	WriteCommand(uchar data);

static Boolean 	TimeoutFindData(uchar match);

/* USER FUNCTIONS */


/* Unlike other devices, KB Init should be called AFTER interrupts are enabled */

void InitHWKeyboard(void)
{
	Message(("Initializing keyboard controller..."));

	kbHead = kbTail = 0;
	kbModifiers = 0;
	hwkbFlags = 0;
	kbLEDs = 0;
	gStatus = 0;
	
	if (ControllerSelfTestOK())
	{
		ControllerEnableKeyboard();		/* release the clk line */

		ResetKeyboard();
	
		WriteCommandByte( 0x4d );		/* scancode xlation, override keybd lock, */
										/*  sysflag, int enabled */
	}
	else
		Message(("*** Keyboard controller failure!"));
}



/* 	Decoding PC scancodes.
	
	The 8042 translates the AT scancodes -> PC scancodes.  Some keys generate
	multibyte scancodes (that sux).  Most keys don't.
	
	Here are the troublemakers, and the codes they generate:
	
				MAKE			BREAK
	
	Home		e0 47 			e0 c7
	End			e0 4f 			e0 cf
	PgUp		e0 49 			e0 c9
	PgDn		e0 51 			e0 d1

	R Alt		e0 38 			e0 b8

	Del			e0 53 			e0 d3
	Ins			e0 52 			e0 d2
	
	UpArr		e0 48 			e0 c8
	DownArr		e0 50 			e0 d0
	LeftArr		e0 4b 			e0 cb
	RightArr	e0 4d 			e0 cd

	Pause/Brk	e1 1d 45 		e1 9d c5
	PrtScr		e0 2a e0 37 	e0 b7 e0 aa			(egad!)
	
	For the most part, they consist of a code byte prefaced by an escape
	byte that is usually $e0.  Fortunately, there are no valid scancodes in
	the $eX range, so we can just check for an upper nybble of $E.
	
	As with normal keys, the break code is the make code with the high bit set.
	
	We can't just ignore the $E0, as the code bytes overlap with non-$E0 code
	bytes (e.g., $E047 = Home, while $47 = Number Pad 1).
	
	We can simplify the Pause & PrtScr codes by just looking for $EX1D and $EX37,
	respectively.  (And $EX9D and $EXAA for the breaks.)
	
	
	On a Mac, 		Shift + CapsLock yields Upper Case chars.
	On a PeeCee, 	Shift + CapsLock yields Lower Case chars.
	
	We do it the Mac way.
	
*/

Boolean gE0State = false;

/* Special Function Key Hack State Vars */
ulong vidToggle = 0;
extern Boolean gPrintEnable;

static ulong ParseScanCode(uchar sc);

ulong ParseScanCode(uchar sc)
{
ulong event;
uchar c;
Boolean isBreak;

	event = 0;
	c = 0;
				
	if (gE0State)						/* are we parsing a multibyte code? */
	{
			
		if (!(sc & 0x80))				/* ¥¥¥ÊFIXME ignore keyups for now - IMPORTANT! */
		{
			switch (sc)					/* this sux too, tablify once we know what's up */
			{
				case kUpArr:
						event = kUpKey;
						break;
				case kDownArr:
						event = kDownKey;
						break;
				case kRightArr:
						event = kRightKey;
						break;
				case kLeftArr:
						event = kLeftKey;
						break;
				case kPageUp:
						event = kScrollUpKey;
						break;
				case kPageDown:
						event = kScrollDownKey;
						break;
				case kHome:
						event = kHomeKey;
						break;
				case kInsert:
						event = kRecentKey;
						break;
				case kDelete:
						event = kOptionsKey;
						break;
				case kEnd:
						event = kBackKey;
						break;
				case kEnter:
						event = kExecuteKey;
						break;
				default:
						Message(("multibyte: e0 %x",sc));
						break;
			}
		}
				
		gE0State = false;				/* drop it on the floor */
		
	}
	else
	{
	
		isBreak = sc & 0x80;
			
		if (!isBreak)
		{	
		
			if (sc < kCvtTableSize)
			{
				if (kbModifiers & kShiftModifier)
					c = sc2uc[sc];
				else
				{
					c = sc2lc[sc];
						
					if (kbModifiers & kCapsModifier)
						if ((c >= 'a') && (c <= 'z'))
							c -= 0x20;
				}
						
				if (kbModifiers & kControlModifier)
				{
					if ((c >= 'a') && (c <= 'z'))
						c -= 0x60;
					else							
						if ((c >= 'A') && (c <= 'Z'))
							c -= 0x40;
				}
			}
					
			if (c == 0)					/* a special key? */
			{
				switch (sc)
				{
					case kLeftShift:
					case kRightShift:
							kbModifiers |= kShiftModifier;
							break;

					case kControl:
							kbModifiers |= kControlModifier;
							break;

					case kCapsLock:
							if (!(hwkbFlags & kCapsDownFlag))
							{
								if(GetGlobalModifiers() & kCapsModifier)
									kbModifiers &= ~kCapsModifier;
								else
									kbModifiers |= kCapsModifier;
								hwkbFlags |= kCapsDownFlag;
							}
							break;

#ifdef DEBUG
					case kF1:
							if(gPrintEnable)
							{
								   printf("\nPRINTF DISABLED\n");
								   gPrintEnable = !gPrintEnable;
							}
							else
							{
                                gPrintEnable = !gPrintEnable;
								printf("\nPRINTF ENABLED\n");
							}
							break;
					case kF2:
							if (vidToggle)
								EnableDisplay();
							else
								KillDisplay();
							vidToggle = !vidToggle;
							break;
					case kF3:
							printf("PowerKey pressed\n");
							event = kPowerKey;
							break;
#ifdef APPROM
					case kF4:
							printf("Debugger breaks enabled\n");
							gPreventDebuggerBreaks = false;
							break;
					case kF5:
							if(gMonkeyEnable)
								printf("Monkey mode disabled\n");
							else
								printf("Monkey mode enabled\n");
							gMonkeyEnable = !gMonkeyEnable;
							break;
					case kF6:
							printf("Slow ass ROM speed set\n");
							*(volatile ulong*)(kSPOTBase+kROMCntl1) = 0x0f0f0007;
							break;
					case kF7: {
							extern ulong gVBump;
							gVBump++;
							break;
					}

#endif
#endif
				}						
			}
			else
				event = (ulong)c;
				
		}
		else							/* for now, just look for meta keys */
		{
		
			if ((sc & 0xf0) == 0xe0)
				gE0State = true;
			else
			{
				switch (sc & 0x7f)
				{
					case kLeftShift:
					case kRightShift:
							kbModifiers &= ~kShiftModifier;
							break;
							
					case kControl:
							kbModifiers &= ~kControlModifier;
							break;

					case kCapsLock:
							hwkbFlags &= ~kCapsDownFlag;
							break;
				}
			}
			
		}
	}
		
	return event;
}


void HWKeyboardIdle(void)
{	
Input input; 

	while (kbTail != kbHead)
	{
		kbTail++;
		kbTail &= kKybdBufMask;
		
		input.data = ParseScanCode(kbData[kbTail]);
		if (input.data)
		{
			input.device = kPCKeyboard;
			input.modifiers = kbModifiers;		
			input.rawData = kbData[kbTail];
			input.time = gSystemTicks;			/* This sucks, add timestamp to kb posts */
			PostInput(&input);
		}

		UpdateGlobalModifiers(kPCKeyboard, kbModifiers);
	}
}



/* SUPPORT FUNCTIONS */


/*	HW Keyboard interface is a PC-AT style keyboard interface microcontroller.

	CAVEATS: 	- disable interrupts before sending keyboard or controller a 
				  cmd.  The interrupt handler always assumes it's fair game.
				  
	Keyboard presence is detected dynamically.
*/


/* Keyboard Controller Command Functions */
#if 0
static uchar ReadCommandByte(void)
{
uchar data;
ulong ints;

	ints = DisableInts( kKeyboardIntMask );
	
	WriteCommand( kReadCommandByte );
	data = ReadData();
	
	EnableInts( ints );

	return data;
}
#endif

static void WriteCommandByte(uchar cmdbyte)
{
ulong ints;

	ints = DisableInts( kKeyboardIntMask );
	
	WriteCommand( kWriteCommandByte );
	WriteData( cmdbyte );
	
	EnableInts( ints );
}

static Boolean ControllerSelfTestOK(void)
{
uchar data;
ulong ints;

	ints = DisableInts( kKeyboardIntMask );
	
	WriteCommand( kTestController );
	data = ReadData();
	
	EnableInts( ints );

	if ( data == 0x55 )
		return true;
	else
		return false;
}

static void ControllerEnableKeyboard(void)
{
ulong ints;

	ints = DisableInts( kKeyboardIntMask );
	
	WriteCommand( kEnableKeyboard );
	
	EnableInts( ints );
}



/* Keyboard Command Functions */

void SetHWKBLEDs(uchar newleds)
{
ulong ints;

	ints = DisableInts( kKeyboardIntMask );
	
	WriteData( 0xed );	
	if ( ReadData() == 0xfa )
	{
		WriteData( newleds );
		gStatus = ReadData();			/* we don't care about status, but someone might */
	}
	
	kbLEDs = newleds;
	
	EnableInts( ints );
}

uchar GetHWKBLEDs(void)
{
	return kbLEDs;
}

#if 0
static void SetTypematic(uchar UNUSED(newtm))
{
}

static void EnableKeyboard(void)
{
}
#endif
static Boolean ResetKeyboard(void)
{
uchar data;
ulong ints;

	ints = DisableInts( kKeyboardIntMask );
	
	TimeoutFindData( kKBErr );			/* flush anything waiting */
	
	WriteData( 0xff );					/* kb reset cmd */
	data = ReadData();
	
	if (data == 0xfa)					/* successful cmd? */
	{
		data = ReadData();				/* kb OK? */
	}
	
	EnableInts( ints );
	
	if (data == 0xaa)
		return true;
	else
		return false;
}


/* pass match == kKBErr to just flush the ctlr fifo */

static Boolean TimeoutFindData(uchar match)
{
ulong maxreads = 16;
uchar result;

	do
		result = ReadData();
	while ( (result != match) && maxreads-- );
		
	if (result == match)
		return true;
	else
		return false;
}



/* Poll assumes interrupts are OFF! */

uchar PollHWKeyboard(void)
{
ulong result;
	
	if (*(volatile ulong *)kKybdControlReg & kKStat_OutputFull)
	{	
		result = *(volatile ulong *)kKybdDataReg;
		return (ParseScanCode((uchar)result));
	}
	
	return 0;
}



/* These functions assume that it's safe to fondle the hardware (ints are OFF) */

static uchar ReadData(void)
{
ulong timeout = kKBCtlTimeout;
ulong result = kKBErr;

	while (timeout--)
	{
		if (*(volatile ulong *)kKybdControlReg & kKStat_OutputFull)
		{	
			result = *(volatile ulong *)kKybdDataReg;
			goto Done;
		}
	}
	
Done:
	if (timeout < gReadDataTimeout)	
		gReadDataTimeout = timeout;
	return (uchar)result;	
}


static void WriteData(uchar data)
{
ulong timeout = kKBCtlTimeout;

	while (timeout--)
	{
		if (!(*(volatile ulong *)kKybdControlReg & kKStat_InputFull))
		{
			*(volatile ulong *)kKybdDataReg = data;
			goto Done;
		}
	}
	
Done:
	if (timeout < gWriteDataTimeout)
		gWriteDataTimeout = timeout;
}


static void WriteCommand(uchar data)
{
ulong timeout = kKBCtlTimeout;

	while (timeout--)
	{
		if (!(*(volatile ulong *)kKybdControlReg & kKStat_InputFull))
		{
			*(volatile ulong *)kKybdControlReg = data;
			goto Done;
		}
	}
	
Done:
	if (timeout < gWriteCmdTimeout)
		gWriteCmdTimeout = timeout;
}

