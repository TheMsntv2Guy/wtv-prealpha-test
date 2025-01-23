// ===========================================================================
//	Macintosh.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MACINTOSH_H__
#define __MACINTOSH_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// constants
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enum {
	kOSTrapBit				= (1<<11),	/* bit 11 is the OS Trap bit */
	kTrapNumberMask			= 0x07FF,	/* bits used by the A-Trap mechanism */

	kHFSNameMax				= 31,		/* maximum length of an HFS file name */
	
	kPollingSleepTime		= 60,		//MultiFinderÕs sleep while sound is playing

	kNumberOfMasters		= 3,		//number of master pointer blocks we expect
	kSizeOfReserve			= 32 * 1024, //size of reserve memory for grow zone proc
	kMinSpace				= 32 * 1024, //minimum available memory I allow in heap

	rAppSignature			= 'WBTV',	//applicaitonÕs OS signature

	kScrollbarAdjust		= 16-1,		//the width of the scrollbar in the list
	kListFrameInset			= -1,		//inset rectangle adjustment for list frame
	kStandardWhiteSpacing	= 13,		//inset rectangle adjustment for dialog items

	kHiliteControlSelect	= 1,		//select the control
	kHiliteControlDeselect	= 0,		//deselect the control
	kCntlOn					= 1,		//controlÕs value when truned on
	kCntlOff				= 0,		//controlÕs value when truned off

	kRecordTop				= 40,		//40,50 is topLeft for SndRecord dialog
	kRecordLeft				= 50,		//40,50 is topLeft for SndRecord dialog

	kWindowPosStartPt		= 2,		//offset from the topLeft of the screen
	kWindowPosStaggerH		= 16,		//staggering amounts for new windows
	kWindowPosStaggerV		= 3,		//not including the windowÕs title height

	kMinWindowTitleHeight	= 19,
	kButtonFrameSize		= 3,		//buttonÕs frame pen size
	kButtonFrameInset		= -4,		//inset rectangle adjustment around button
	kButtonSizeH			= 17,		//standard height of buttons
	kDefaultButSizeH		= kButtonSizeH - kButtonFrameInset - kButtonFrameInset
};

//refer to the Simulator.r file for the explaination about these numbers
enum {
	kNumOfButtons			= 6,		//the number of buttons in the window

	//standard position for icons in alerts
	kIconWidthOrHeight		= 32,
	kStdAlertIconTop		= kStandardWhiteSpacing,
	kStdAlertIconLeft		= 23,
	kStdAlertIconBottom		= kStdAlertIconTop + kIconWidthOrHeight,
	kStdAlertIconRight		= kStdAlertIconLeft + kIconWidthOrHeight,

	kFSAsynch				= true,		//asynchronous File Manager call

	kEnterKey				= '\3', 	//the keys IÕm looking for
#ifdef applec
	kReturnKey				= '\n',		//MPW C is wrong, the return key is "\r" not "\n"
#else
	kReturnKey				= '\r',
#endif
	kEscape					= '\33',
	kUpArrow				= '\36',
	kDownArrow				= '\37',
	kPeriod					= '.',
	kBackspace				= '\b',

//This bit set in the ioFlAttrib field if the fileÕs resource fork is open.
	kResForkOpenBit			= (1 << 2),

//For the delay time when flashing the menubar and highlighting a button.
	kDelayTime				= 8,		// 8/60ths of a second

//The minimal number of ticks the ShowWindow needs to be visible.
	kShowTimeDelay			= 20
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// macros
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// accessing a rectangle's points
#define TopLeft(r)			(* (MacPoint *) &(r).top)
#define BottomRight(r)		(* (MacPoint *) &(r).bottom)

#if GENERATINGCFM
#define CreateRoutineDescriptor(info, proc)									\
 RoutineDescriptor g##proc##RD = BUILD_ROUTINE_DESCRIPTOR(info, proc)

#define GetRoutineAddress(proc)	(&g##proc##RD)

#else
#define GetRoutineAddress(proc)	proc
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// inlines
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*
This is a handy routine to copy pascal strings. It's smaller than the
library call pstrcopy, and faster too.
*/

#if GENERATING68K
#pragma parameter PStringCopy(__A0,__A1)
void PStringCopy(StringPtr source, StringPtr destination)
 FIVEWORDINLINE (0x7000, 0x1010, 0x12D8, 0x51C8, 0xFFFC);
//					moveq	#0,d0
//					move.b	(a0),d0
//			loop	move.b	(a0)+,(a1)+
//					dbra	d0,loop
#else
#define PStringCopy(source, dest) strncpy((char *)dest, (char *)source, StrLength(source) + 1)
#endif


// ---------------------------------------------------------------------------
//	prototypes
//
void DoEvent(EventRecord *event);

Boolean IsDAWindow(WindowPtr);
Boolean IsDocWindow(WindowPtr);
Boolean IsModalWindow(WindowPtr);

MacBoolean RequestSaveFile(FSSpec* destSpec, const char* defaultCName);
void SavePageToDisk(FSSpec* saveFile, const char* urlName);
void SetWindowHeight(WindowPtr window, short newHeight);


// ---------------------------------------------------------------------------
// unsorted prototypes


void HandleAbout(void);
void HandleOpen(void);
void HandleReload(void);
void HandleReloadAll(void);
void DoCloseWindow(WindowPtr window);
void Terminate(void);

// ---------------------------------------------------------------------------



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// prototypes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//void MyParafText(StringHandle text, StringPtr cite0, StringPtr cite1);
//WindowPtr GetCenteredWindow(short id, Ptr p, WindowPtr behind);
//short CenteredAlert(short alertID);
//DialogPtr GetCenteredDialog(short id, Ptr p, WindowPtr behind);
//void DoButtonOutline(ControlHandle button);
//void SelectButton(ControlHandle button);
//Boolean IsDocWindow(WindowPtr window);
//Boolean IsModalWindow(WindowPtr window);
//void KillSound(void);
//void DrawSimulatorWindow(void);
//void ShowSimulatorWindow(void);
//void InitSimulatorWindow(void);
//pascal void DoErrorSound(short soundNo);
//void CheckSoundVolume(void);
//Boolean PositionAvailable(MacPoint newPt, short wTitleHeight);
//WindowPtr NewStackedWindow(short windID, Ptr windStorage);
//OSErr CreateDocument(short resRef, FSSpecPtr file);
//void OpenDocument(FSSpecPtr file);
//pascal Boolean SFFilter(CInfoPBPtr p, Boolean *sndFilesOnly);
//pascal short SFGetHook(short mySFItem, DialogPtr dialog, void *sndFilesOnly);
//void DrawAboutWindow(WindowPtr window);
//pascal void DefaultOutline(WindowPtr window, short theItem);
//void AdjustMenus(ushort modifiers);
//void DoMenuCommand(long menuResult, ushort modifiers);
//void DoKeyDown(char key, WindowPtr window);
//void DoAboutClick(WindowPtr window, EventRecord *event);
//void HandleSimulatorClick(WindowPtr window, EventRecord *event);
//void DoUpdate(WindowPtr window);
//void DoActivate(WindowPtr window, Boolean becomingActive);
//pascal OSErr QuitApplicationEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
//pascal OSErr OpenDocumentsEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
//pascal OSErr PrintDocumentsEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
//pascal OSErr OpenApplicationEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
//void DoEvent(EventRecord *event);
//void AdjustCursor(RgnHandle region);
//void EventLoop(void);
//void Initialize(void);
//void main(void);

#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include Macintosh.h multiple times"
#endif
#endif /* __MACINTOSH_H__ */
