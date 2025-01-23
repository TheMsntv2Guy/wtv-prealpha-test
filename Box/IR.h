/* ===========================================================================
	IR.h

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __IR_H__
#define __IR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {			/* NOTE: int handler depends on this being 8 bytes */
	uchar kbData;
	uchar category1;
	uchar category2;
	uchar button;
	ulong time;
} rawIR;


/* Public functions */

void InitIR(void);
void IRIdle(void);

void SetIRRemoteRepeat(uchar delay, uchar rate);
void SetIRKBRepeat(uchar delay, uchar rate);

void GetIRRemoteRepeat(uchar *delay, uchar *rate);
void GetIRKBRepeat(uchar *delay, uchar *rate);

ulong PollIR(void);


/* Private stuff */

#ifdef HARDWARE			/* ¥¥¥ÊFIXME!  This should really stay sync'ed with the codes in MacintoshIR.c */

#define	kMaxTicksForAutoRepeat		7		/* Remote autorepeats typically come 5 or 6 ticks apart */
											/* (6 jives with min frame time of 90ms.  If other interrupt activity      */
											/*  holds off IR interrupts, this could stretch & screw up autorepeating.) */

#define	kDefaultIRRemoteDelay 		7		/* 7 * (6ticks*16ms) = 0.7sec */
#define	kDefaultIRRemoteRate		1		/* 1 * (6ticks*16ms) = 0.1sec */

#define	kDefaultIRKBDelay 			4
#define	kDefaultIRKBRate			0

#define	kIRKBDebounce				0


/* -------- REMOTE --------- */

#define	kWebTVCategory1		0x0f			/* Real, Sony-assigned WebTV Category Code */
#define	kWebTVCategory2		0x3a			

#define	kUpButton			0x74			/* Real, Sony-assigned WebTV Button Codes */
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


/* ------- Sejin KB -------- */

#define	kSejinIDMask		0x30
#define	kSejinKeyboardID	0x10

#define	kSejinMakeBreakMask	0x40			/* 0 = MAKE, 1 = BREAK */


#endif


#ifdef __cplusplus
}
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include IR.h multiple times"
	#endif
#endif /* __IR_H__ */
