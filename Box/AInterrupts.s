#include "iregdef.h"
#include "idtcpu.h"
#include "frame.h"

#include "HWRegs.h"


/* 
	void GetIntEnables(void) 
 */

#ifdef _INCLUDE_DEAD_CODE_
FRAME(GetIntEnables,sp,0,ra)
	.set	noreorder

	lw		v0,kBusIntEnableSetReg	

	j		ra
	nop
	
	.set	reorder
ENDFRAME(GetIntEnables)
#endif


/*
	ulong EnableInts(ulong ints) - returns int enable state AFTER enable
 */

FRAME(EnableInts,sp,0,ra)
	.set	noreorder

	sw		a0,kBusIntEnableSetReg
	lw		v0,kBusIntEnableSetReg		/* flush write buffer & read return value */
	
	j		ra
	nop

	.set	reorder
ENDFRAME(EnableInts)


/*
	ulong DisableInts(ulong ints) - returns int enable state BEFORE disable
 */

FRAME(DisableInts,sp,0,ra)
	.set	noreorder

	mfc0	t0,C0_SR			/* disable CPU SPOT ints (Goober ints still enabled!) */
 	li		t1,kCPUIntMask
	nor		t1,zero,t1			/* turn the mask into ~mask */
	and		t0,t0,t1
	mtc0	t0,C0_SR
	nop

	lw		v0,kBusIntEnableSetReg		/* return settings before disable */
	nop
	sw		a0,kBusIntEnableClearReg
	lw		t0,kBusIntEnableClearReg	/* flush write buffer */

	mfc0	t0,C0_SR			/* re-enable CPU ints */
	nor		t1,zero,t1			/* turn the ~mask into mask */
	or		t0,t0,t1			
	mtc0	t0,C0_SR

	j		ra
	nop
 
	.set	reorder
ENDFRAME(DisableInts)



/*
	void ClearAllInts(void)
 */
 
FRAME(ClearAllInts,sp,0,ra)
	.set	noreorder
 
	/* Bus Unit */
	
	li		t0,kVideoIntMask+kKeyboardIntMask+kModemIntMask+kIRIntMask+kSmartCardIntMask+kAudioIntMask
	sw		t0,kBusIntStatusClearReg
	
	/* Vid Unit */
	
	li		t0,kVidFIDOIntMask+kVidVSyncEvenIntMask+kVidVSyncOddIntMask+kVidHSyncIntMask+kVidDMAIntMask
	sw		t0,kVidIntStatusClearReg
	
	/* Fences */
	
	li		t0,kFence1ReadIntMask+kFence1WriteIntMask+kFence2ReadIntMask+kFence2WriteIntMask+kBusTimeoutIntMask
	sw		t0,kBusErrStatusClearReg
	
	lw		t0,kBusErrStatusClearReg		/* flush write buffer */
	
	j		ra
	nop
 
	.set	reorder
ENDFRAME(ClearAllInts)



/* 
	Disable all of the various interrupt sources.
 */

FRAME(DisableAllInts,sp,0,ra)
	.set	noreorder

	/* Bus Unit */
	
	li		t0,kVideoIntMask+kKeyboardIntMask+kModemIntMask+kIRIntMask+kSmartCardIntMask+kAudioIntMask
	sw		t0,kBusIntEnableClearReg
	
	/* Vid Unit */
	
	li		t0,kVidFIDOIntMask+kVidVSyncEvenIntMask+kVidVSyncOddIntMask+kVidHSyncIntMask+kVidDMAIntMask
	sw		t0,kVidIntEnableClearReg
	
	/* Fences */
	
	li		t0,kFence1ReadIntMask+kFence1WriteIntMask+kFence2ReadIntMask+kFence2WriteIntMask+kBusTimeoutIntMask
	sw		t0,kBusErrEnableClearReg
	
	lw		t0,kBusErrEnableClearReg		/* flush write buffer */
	
	j		ra
	nop

	.set	reorder
ENDFRAME(DisableAllInts)


/* LOGCNT assumes v0 is available.  It simply increments a global counter. */
#ifdef DEBUG
#define	LOGCNT(logvar)	lw	v0,logvar; nop; addiu	v0,v0,1; sw	v0,logvar
#else
#define	LOGCNT(var)
#endif



#
# Primary Interrupt Source      Service Priority
# 	
#          Video                       5
#       PC Keyboard					   4
#          Modem                       1	(Highest)
#           IR						   3
#        SmartCard                     6	(Lowest)
#          Audio					   2					
#
#	Expansion Devices - insert themselves via the SystemIntHook vector
#
#   Goober Interrupt Button is on a different CPU interrupt line.
#   We only check for it in DEBUG builds.
#
#	Nested interrupts are not allowed.  Each interrupt handler should 
#	do as little work as possible (typically move data to/from device
#	from/to buffer).
#

/* by now, we've saved at,v0,v1,a0,sp,ra */


FRAME(InterruptDecode,sp,0,ra)
	.set	noreorder

	mfc0	v0,C0_CAUSE
	li		v1,kCPUIntMask			/* SPOT interrupts are on INT0 for TrialSys */
	and		v1,v0,v1				/* SPOT? */
	beq		zero,v1,notSPOT

	lw		v1,kBusIntStatusReg		/* fetch status, already shifted up 2 bits */	
	la		v0,dispatch_int_table	/* load ptr to table in delay slot */
	addu	v0,v0,v1				/* v0 points at a vector in the table */
	lw		v0,0(v0)
	nop
	jr		v0
	nop
	

notSPOT:							/* not a level 0.  Perhaps Goober?  v0 = cause*/
	li		v1,kGooberIntMask
	and		v1,v0,v1				/* Goober? */
	beq		zero,v1,notGoober
	
	nop
	j		debuggerContext			/* go save the rest of the regs & enter the monitor */
	nop
	
notGoober:
	sw		v0,mysteryIntCause	
	mfc0	v0,C0_SR
	nop	
	sw		v0,mysteryIntSR	
	mfc0	v0,C0_EPC
	nop	
	sw		v0,mysteryIntEPC	
#ifdef DEBUG
#if 0
	break	0x70					/* stop & let us know */
#endif
#endif
	j		ra			
	nop



	/*************************** VIDEO INTERRUPT HANDLER *****************************/

video_int:
	lui		a0,kSPOTBaseHi				/* a0 -> SPOT Base */

	lw		v0,kBusChipID(a0)			/* get spot ID */
	li		v1,kRevMask
	and		v0,v0,v1					/* mask all but SPOT rev */
	bne		zero,v0,spot3VBL			
	nop


	/*
	SPOT1 Software Interlacing
	*/
	
spot1HBL:
	li		v0,kVidHSyncIntMask
	sw		v0,kVidIntStatusClearReg
	
	lw		v0,kVidCLineReg
	andi	v1,v0,0x3ff
	slti	v0,v1,10
	bne		zero,v0,isVBlank1
	slti	v0,v1,242
	bne		zero,v0,NotVBlank				/* process hline interrupts only */
isVBlank1:
	
	andi	v0,v0,0x0400					/* bit 10: 1 = odd, 0 = even */
	beq		zero,v0,itsEven
	nop
itsOdd:
	lw		v0,oddBuffer
	nop
	b		startVid
	sw		v0,kVidNStart(a0)
itsEven:
	lw		v0,evenBuffer
	nop
	b		startVid
	sw		v0,kVidNStart(a0)
	

	/*
	SPOT3 VBL 
	*/
	
spot3VBL:

#ifndef HW_INTERLACING	

	/*
	SPOT3 Software Interlacing 
	*/
	
spot3VBL:	
	lw		v1,kVidIntStatus(a0)
	nop
	
	lw		v0,kVidCLineReg
	andi	v0,v0,0x3ff
	slti	v1,v0,10
	bne		zero,v1,isVBlank3
	slti	v1,v0,242
	bne		zero,v1,NotVBlank					/* process hline interrupts only */
isVBlank3:


	andi	v0,v1,kVidVSyncEvenIntMask	/* even bit set? */
	beq		zero,v0,spot3_itsOdd
	nop
	
spot3_itsEven:
	lw		v0,oddBuffer
	nop
	b		spot3_startVid
	nop

spot3_itsOdd:
	lw		v0,evenBuffer
	nop	
	
spot3_startVid:
	sw		v1,kVidIntStatusClear(a0)	/* use saved copy to clear int */
	sw		v0,kVidNStart(a0)
	
#else 	/* HW_INTERLACING */



	lw		v1,kVidIntStatus(a0)
	nop
	sw		v1,kVidIntStatusClear(a0)	/* use saved copy to clear int */

	lw		v0,kVidCLineReg
	nop
	andi	v0,v0,0x3ff
	slti	v1,v0,10
	bne		zero,v1,isVBlank3
	slti	v1,v0,242
	bne		zero,v1,NotVBlank					/* process hline interrupts only */
	nop
		
isVBlank3:


#endif  /* HW_INTERLACING */

	/* 
	Common VBL Handler
	*/
	
startVid:

	lw		v0,gOverscanColorLineStart		/* if non-zero we want to change the color on that scanline */
	beq		zero,v0,noOverscanColorChange
	nop
	sw		v0,kVidHIntLineReg				/* set interrupt for the line we want to change on */
	lw		v0,gOverscanDefaultColor		/* set default color ( top of screen down to the change line ) */
	sw		v0,kVidBlnkColReg
	

noOverscanColorChange:

	li		v1,kVideoIntMask
	sw		v1,kBusIntStatusClear(a0)

	/* 
	Increment System Ticks
	*/
	
	lw		v0,gSystemTicks
	li		v1,1						/* fill delay slot */
	addu	v0,v0,v1					/* bump ticks */
	sw		v0,gSystemTicks
	
	
	
	/*
	To flash the Connected LED with a regular cadence, its flashing is handled by
	the VBL handler.
	*/
		
	lw		v0,gVBLsPerConnectedFlashes
	lw		v1,gVBLsElapsed
	beq		zero,v0,NoLEDUpdates
	
	addiu	v1,v1,1						/* bump vbl count (in delay, happens regardless) */
	sw		v1,gVBLsElapsed				/* update count */
		
	bne		v1,v0,NoLEDUpdates
	nop
	
	lw		v0,kDevLEDs(a0)
	sw		zero,gVBLsElapsed
	xori	v0,v0,kConnectedLED
	sw		v0,kDevLEDs(a0)
	
NoLEDUpdates:



	/*
	
	!!! From this point on, a0 will be used for other things than SPOT Base !!!
	
	*/
	
	/* get SPOT version and branch to appropriate hacks */
	
	lw		v0,kSPOTBase				/* get spot ID */
 	li		v1,kRevMask
	and		v0,v1,v0					/* get SPOT id */
	bne		zero,v0,SPOT3Hacks			/* do SPOT3 hax */
    nop									/* fall through to SPOT1 hacks */
	
	/**************************** SPOT1 hacks *************************/
	
	/*
		SPOT1 Shiftage Hack
		If kVidCCntReg == 0x30 => video is OK and no need to fix it
		otherwise we need to correct the shift which we do by setting a global that is
		detected in UserLoopIteration in System.c which kills video, waits, then
		re-enables it
	*/
	
ShiftageHack:
   	lw      a0,kVidCCntReg              /* read current count */
    li      v0,0x30                     /* SPOT1 magic count number */
    beq     a0,v0,vidQFlush				/* if equal then no hack needed */
    nop
    lw      v0,(gVideoShift)			/* video has wigged out on us */
    li      v1,1						/* set the flag so we can correct it */
    addu    v0,v0,v1					/* it will be corrected next time through */
    sw      v0,(gVideoShift)			/* UserLoopIteration */
	b		vidQFlush					/* continue normal VBL handler */
	nop
	
	/**************************** SPOT3 hacks *************************/

SPOT3Hacks:

#ifdef SPOT3_SHIFT_HACK


	/*
		SPOT3 Shift Hack
		
		SPOT3 DMA goes wacko at boot time.  We detect shiftage by whether
		or not the CCnt is equal to 0x4a0. If not, we subtract the difference,
		and add it to the next DMA.  After that, we set DMA size to normal.
	*/



	/* if VBL is late, then dont restart the video for this field */
	
	lw		v0,kVidCLineReg
	andi	v0,v0,0x3ff
	beq		zero,v0,notlate
	slti	v0,v0,242
	beq		zero,v0,notlate			/* interrupt late, dont reload */
	li		v0,0x800000

#if	0	/* for debugging */
	lw		v1,kVidBlnkColReg
	xor		v1,v1,v0				/* invert color */
	sw		v1,kVidBlnkColReg
#endif

	b		SPOT3RefreshHack
	nop
notlate:
	

	lw		v1,dmaSize			
	lw		v0,gVBump
	beq		zero,v0,setNSize
	nop
	
shift:
	lw		v0,kVidCCntReg
	add		v1,v1,v1					/* two fields */		
	addi	v0,-1184					/* 0x4a0 */
	add		v1,v1,v0
	sw		zero,gVBump
	
setNSize:
	sw		v1,kVidNSizeReg
	lw		zero,kVidNSizeReg			/* flush write buf */

#endif

#ifdef SPOT3_REFRESH_HACK

	/*
		SPOT3 Refresh Hack
		
		SPOT3 has a really cool feature where the hardware can't refresh memory, so
		we get to do it in software. neat!
	*/
	
SPOT3RefreshHack:

   	lw      v0,kVidCLineReg             /* get current count */
	andi	v0,v0,0x3ff					/* mask off field bit */
	beq		zero,v0,doRefresh
	slti	v1,v0,242					/* end of active pixels */
	bne		zero,v1,skipRefresh			/* skip refresh if in active pixels */	

doRefresh:
	li		v0,1024						/* refresh 1024 times */
	li		v1,0x48000000
1:
	sw		v1,kMemCmdReg
	bne		zero,v0,1b
	subu	v0,v0,1
	
	lw		zero,kMemCmdReg				/* flush write buffer */
	nop
	b		vidQFlush
	nop

skipRefresh:
	lw		v0,gRefreshSkips
	nop
	addi	v0,v0,1
	sw		v0,gRefreshSkips

	
#endif /* SPOT3_REFRESH_HACK */

	/**************************** end of hacks *************************/

	

	/*
	Queued VidUnit Writes
	*/
	
vidQFlush:	
	lw		v0,vidqtail				
	lw		v1,vidqhead				
	nop
	beq		v0,v1,vidQDone		
	addiu	v0,v0,1						/* read next element */
	andi	v0,v0,0xf					/* circular buffer */
	sw		v0,vidqtail
	
	sll		v0,v0,3						/* struct is 8 bytes, turn index -> offset */

	la		v1,vidq
	addu	v1,v1,v0					/* add offset to base */
	
	lw		a0,0(v1)					/* fetch address */
	nop
	lw		v0,4(v1)					/* fetch data */
	nop
	sw		v0,0(a0)					/* write to chip */
		
	b		vidQFlush	
	nop

vidQDone:	
	lw		v1,kVidFCntlReg				/* Flush write buffer */


	j		ra			
	nop



	/*********************** NON-VBLANK  HLINE INTERRUPT HANDLER ****************/

	/* entered from common video interrupt handler above if not at vblank */
	
NotVBlank:

	/*  Process horizontal line interrupt for overscan color changes */

	lw		v0,gOverscanColorLineStart		/* if non-zero we want to change the color on that scanline */
	beq		zero,v0,noChange
	nop
	lw		v0,gOverscanColor
	sw		v0,kVidBlnkColReg
	nop
	
noChange:

	li		v0,244					/* restore default line interrupt for vblank */			
	sw		v0,kVidHIntLineReg
	nop
	
	j		ra			
	nop
	





	/*************************** IR INTERRUPT HANDLER *****************************/

ir_int:
	li		v0,kIRIntMask
	sw		v0,kBusIntStatusClearReg
		
	lw		v0,irHead				/* v0 = index of last data written */
	lw		v1,irTail				/* (delay) if inc'ed head == tail, we just wrapped.  blow this one off. */
	addiu	v0,v0,1					/* write to next element */
	andi	v0,v0,kIRBufMask		/* circular buffer */
	beq		v0,v1,irfifofull		
	la		v1,irData				/* (partially) load v1 whether we wrapped or not (most often not) */
	sw		v0,irHead				/* update head */
	sll		v0,v0,3					/* struct is 8 bytes, so mult index by 8 */
	addu	v1,v1,v0
	
	lw		v0,kIRDataReg			/* fetch the data from the HW */
	nop
	sw		v0,0(v1)				/* store data */
	lw		v0,gSystemTicks
	j		ra
	sw		v0,4(v1)				/* store timestamp */

irfifofull:
	j		ra						/* throw away overflows */
	nop



	/*************************** KEYBOARD INTERRUPT HANDLER *****************************/

keyboard_int:
	li		v0,kKeyboardIntMask
	sw		v0,kBusIntStatusClearReg

	lui		a0,kSPOTBaseHi			/* a0 -> SPOT */
	
stashkcode:	
	lw		v0,kbHead				/* v0 = index of last data written */	
	lw		v1,kbTail				/* if inc'ed head == tail, we just wrapped.  blow this one off. */
	addiu	v0,v0,1					/* write to next element */
	andi	v0,v0,kKybdBufMask		/* circular buffer */
	beq		v0,v1,kbfifofull		/* this could be nasty if we're losing a keyup, huh? */
	la		v1,kbData				/* load v1 whether we wrapped or not (most often not) */	
	sw		v0,kbHead				/* update head */
	addu	v1,v1,v0				/* turn offset into address */
	
	lw		v0,kKBDataOffset(a0)	/* fetch the data from the interface chip */
	nop
	sb		v0,0(v1)				/* and store */
	
	lw		v0,kKBCtlOffset(a0)		/* any more? */
	nop
	andi	v0,v0,0x01				/* test OBF */
	bne		zero,v0,stashkcode
	nop

	j		ra
	nop
	
kbfifofull:							/* the fifo full loop is suboptimal.  OK, 'cos we shouldn't do it */
	lw		v0,kKBDataOffset(a0)	/* fetch the data from the interface chip, but don't do anything with it */
	nop

	lw		v0,kKBCtlOffset(a0)		/* any more? */
	nop
	andi	v0,v0,0x01				/* test OBF */
	bne		zero,v0,stashkcode
	nop
		
	j		ra
	nop



	/***************************** MODEM INTERRUPT HANDLER ******************************/
	
modem_int:
	LOGCNT(modemIntCnt)

	li		v0,kModemIntMask
	sw		v0,kBusIntStatusClearReg

	lui		a0,kSPOTBaseHi					/* a0 -> SPOT */
	
checkModemIntType:
	lw		v0,kIIR(a0)						/* v0 = int status bits */
	la		v1,dispatch_modem_int			/* load ptr to table during delay slot */
	andi	v0,v0,0x0f						/* use lo nybble as index into table */
	sll		v0,v0,2							/* make it an offset */
	addu	v0,v0,v1						/* v0 points at a vector in the table */
	lw		v0,0(v0)
	nop
	jr		v0
	nop
	
	
modemStatus:
	LOGCNT(modemStatusIntCnt)
	lw		v0,kMSR(a0)						/* clear the interrupt by reading MSR */	
	nop

	sb		v0,gMSR							/* stash it away for the driver */

	andi	v0,v0,kMSR_CTS					/* only thing I care about is CTS becoming ASSERTED */
	beq		v0,zero,notCTSAsserted
	
	lw		v1,kIER(a0)						/* got it, now we want to shut off MSR change interrupts. */
	andi	v1,v1,(~kIER_MSRInt & 0xff)
	sw		v1,kIER(a0)						/* gets flushed by upcoming iir read */
	
	j		modemTxBufEmpty					/* alright, let's go send some damn data! */

notCTSAsserted:	
	j		checkModemIntType				/* any more ints pending? */
	nop
	
	
modemLineStatus:							/* ¥¥¥Êthis needs work */
	LOGCNT(modemLineStatusIntCnt)
	lw		v0,kLSR(a0)						/* clear the interrupt by reading LSR */
	nop
	
	break	89			/* DEBUG - tell joe if you get here! */
	
	sb		v0,gLSR							/* stash it away for the driver */
	j		checkModemIntType				/* any more ints pending? */
	nop
	
	
modemTxBufEmpty:					/* for each tx buf empty int, we stash up to 16 chars into the tx fifo */
	LOGCNT(modemTxBufEmptyCnt)
		
	li		sp,16					/* sp has been saved, so we can use it as a counter */
									/* it counts fifo entries, of which there are 16 */
stashTxModem:
	LOGCNT(modemTxStashCnt)
	
	lw		v0,kMSR(a0)				/* this also clears any pending MSR interrupt */
	nop
	andi	v0,v0,kMSR_CTS			/* OK to xmit? */
	beq		v0,zero,modemCTSCleared
	
	lw		v0,modemXmtTail			/* v0 = index of last data read */
	lw		v1,modemXmtHead			/* (delay) if head == tail, we're done.  Disable Tx ints. */
	nop
	beq		v0,v1,noMoreToTx		
	addiu	v0,v0,1					/* read next element */
	andi	v0,v0,kModemXmtBufMask	/* circular buffer */
	sw		v0,modemXmtTail			/* update tail */

	la		v1,modemXmtData
	addu	v1,v1,v0				/* add offset to base */
	
	lb		v0,0(v1)				/* fetch byte to send */
	nop
	sw		v0,kRxTx(a0)			/* write data to the modem */
		
	addiu	sp,sp,-1				/* dec free fifo entry counter */
	bne		zero,sp,stashTxModem
	nop

	j		checkModemIntType		/* any more ints pending? */
	nop

noMoreToTx:
	LOGCNT(modemNoMoreToTxCnt)
	j		checkModemIntType		/* any more ints pending? */
	nop
	

modemCTSCleared:
	lw		v1,kIER(a0)
	ori		v1,v1,kIER_MSRInt				/* turn on MSR ints to catch CTS asserting */
	sw		v1,kIER(a0)						/* gets flushed by upcoming iir read */

	j		checkModemIntType
	nop
	

modemRxBufFull:
	LOGCNT(modemRxBufFullCnt)
stashRxModem:
	LOGCNT(modemRxStashCnt)
	lw		v0,modemRcvHead			/* v0 = index of last data written */
	lw		v1,modemRcvTail			/* (delay) if inc'ed head == tail, we just wrapped.  pause the line. */
	addiu	v0,v0,1					/* write to next element */
	andi	v0,v0,kModemRcvBufMask	/* circular buffer */
	beq		v0,v1,modemRxBufOvf		
	la		v1,modemRcvData			/* (partially) load v1 whether we wrapped or not (most often not) */
	sw		v0,modemRcvHead			/* update head */

	addu	v1,v1,v0				/* add offset to base */

	lw		v0,kRxTx(a0)			/* fetch the data from the modem */
	nop
	sb		v0,0(v1)				/* and store in rx buf */
	
	lw		v0,kLSR(a0)				/* any more? */
	nop
	andi	v0,v0,kLSR_DataReady	/* test Rx Data Ready.  1 = byte available. */
	bne		zero,v0,stashRxModem
	nop
	
	j		checkModemIntType		/* any more ints pending? */
	nop
	
modemRxBufOvf:
	lw		v0,kMCR(a0)					/* we need to clear RTS to hold off the other guy */
	lw		v1,kIER(a0)					/* and we need to shut off Rx interrupts. */
	
	andi	v0,v0,(~kMCR_RTS & 0x00ff)
	sw		v0,kMCR(a0)
	
	andi	v1,v1,(~kIER_RxFullInt & 0xff)
	sw		v1,kIER(a0)
	
	li		v0,kModemRcvRTSEnableCnt	/* non-int code counts this down as it pulls chars */
	sw		v0,gRTSEnableCount			/*  from the buf, enables ints & sets RTS when it hits 0 */
	
	LOGCNT(modemRcvBufOvf)

	j		checkModemIntType		/* any more ints pending? */
	nop
	
	
modemInvalid:
#ifdef DEBUG
	break	0x69					/* debug - stop & let us know */
#endif
modemNoInt:	
	LOGCNT(modemNoIntCnt)
	j		ra
	nop

dispatch_modem_int:
	.word	modemStatus		/*  0	0000	Modem Status - read MSR to clear		*/		
	.word	modemNoInt		/*  1	0001 	No Interrupt - Clear & Ignore			*/		
	.word	modemTxBufEmpty	/*  2	0010 	Tx Buffer Empty	- IIR read will clear 	*/		
	.word	modemInvalid	/*  3	0011 	Invalid - should never see this			*/		
	.word	modemRxBufFull	/*  4	0100 	Rx Data Avail - read Rx buf til clear	*/		
	.word	modemInvalid	/*  5	0101 	Invalid - should never see this			*/		
	.word	modemLineStatus	/*  6	0110 	Rx Line Status - read LSR to clear		*/		
	.word	modemInvalid	/*  7	0111 	Invalid - should never see this			*/		
	.word	modemInvalid	/*  8	1000 	Invalid - should never see this			*/		
	.word	modemInvalid	/*  9	1001 	Invalid - should never see this			*/		
	.word	modemInvalid	/* 10	1010 	Invalid - should never see this			*/		
	.word	modemInvalid	/* 11	1011 	Invalid - should never see this			*/		
	.word	modemRxBufFull	/* 12	1100 	Char Timeout - read Rx buf to clear		*/		
	.word	modemInvalid	/* 13	1101 	Invalid - should never see this			*/		
	.word	modemInvalid	/* 14	1110 	Invalid - should never see this			*/		
	.word	modemInvalid	/* 15	1111 	Invalid - should never see this			*/		



	/*************************** SMARTCARD INTERRUPT HANDLER ****************************/
	
smartcard_int:

	li		v0,kSmartCardIntMask		/* clear interrupt */
	sw		v0,kBusIntStatusClearReg

	lw 		v0,kDevSmartCardReg			/* check if card inserted/removed */
	li		v1,kSmartCardInsert
	and		v1,v1,v0
	beq		zero,v1,cardRemoved			/* zero => card removed */
	li		v0,2						/* 2 => card removed event */
	li		v0,1						/* 1 => card inserted event */
cardRemoved:	
	sw		v0,gSmartCardEvent			/* note event for later handling */
	
	j		ra							/* bail */	
	nop
	
	
	/***************************** AUDIO INTERRUPT HANDLER ******************************/
	/* 	
	The audio interrupt handler is written in C.  It is the only C interrupt handler in the
	system.  To call it, we must save away the rest of the CPU state that C code can alter.
	
	By the time we get here, we've saved at,v0,v1,a0,sp,ra in the system register save
	area.  
	
	k0 -> system reg save area
	*/

audio_int:
	mfc0	v0,C0_CAUSE
	mfc0	v1,C0_SR
	sw		v0,R_CAUSE*4(k0)		# save CAUSE
	sw		v1,R_SR*4(k0)			# save SR

	sw		a1,R_A1*4(k0)
	sw		a2,R_A2*4(k0)
	sw		a3,R_A3*4(k0)

	sw		t0,R_T0*4(k0)
	sw		t1,R_T1*4(k0)
	sw		t2,R_T2*4(k0)
	sw		t3,R_T3*4(k0)
	sw		t4,R_T4*4(k0)
	sw		t5,R_T5*4(k0)
	sw		t6,R_T6*4(k0)
	sw		t7,R_T7*4(k0)
	sw		t8,R_T8*4(k0)
	sw		t9,R_T9*4(k0)

	sw		fp,R_FP*4(k0)

	sw		s0,R_S0*4(k0)			/* we need some temps */
	sw		s1,R_S1*4(k0)
	sw		s2,R_S2*4(k0)

	move	s0,ra					/* save ra */
	mflo	s1						/* save mult result regs */
	mfhi	s2	


#ifdef APPROM
	jal		audioInterruptHandler
	nop
#endif

	lw		zero,kAudDMACntl		/* flush write buf */

	move 	ra,s0					/* restore ra */
	mtlo	s1						/* restore mult result regs */
	mthi	s2
	
	lw		s0,R_S0*4(k0)
	lw		s1,R_S1*4(k0)
	lw		s2,R_S2*4(k0)

	lw		fp,R_FP*4(k0)

	lw		t0,R_T0*4(k0)
	lw		t1,R_T1*4(k0)
	lw		t2,R_T2*4(k0)
	lw		t3,R_T3*4(k0)
	lw		t4,R_T4*4(k0)
	lw		t5,R_T5*4(k0)
	lw		t6,R_T6*4(k0)
	lw		t7,R_T7*4(k0)
	lw		t8,R_T8*4(k0)
	lw		t9,R_T9*4(k0)

	lw		a1,R_A1*4(k0)
	lw		a2,R_A2*4(k0)
	lw		a3,R_A3*4(k0)

	lw		v0,R_CAUSE*4(k0)		
	lw		v1,R_SR*4(k0)			
	mtc0	v0,C0_CAUSE
	mtc0	v1,C0_SR

	j		ra
	nop


	/**************************** UNKNOWN INTERRUPT HANDLER *****************************/

notUs:								/* not one we recognize.  Shouldn't happen. */
#if DEBUG
	j		debuggerContext
	nop
#else
	break	0x71
	j		ra			
	nop
#endif

/* First level interrupt dispatch table */

dispatch_int_table:
	.word	notUs			/*  0	xx00 0000 ..	   	 	  		  						*/		
	.word	audio_int		/*  1	xx00 0001 .. 		 	  		  	 		   + Audio	*/		
	.word	smartcard_int	/*  2	xx00 0010 .. 		 	  		  		SmCard			*/		
	.word	audio_int		/*  3	xx00 0011 ..    	 	  		  	    SmCard + Audio	*/		
	.word	ir_int			/*  4	xx00 0100 ..		 			   IR					*/		
	.word	audio_int		/*  5	xx00 0101 .. 			   		   IR 		   + Audio	*/		
	.word	ir_int			/*  6	xx00 0110 .. 		 	   		   IR + SmCard			*/		
	.word	audio_int		/*  7	xx00 0111 .. 		 			   IR + SmCard + Audio	*/		
	.word	modem_int		/*  8	xx00 1000 .. 		 	   Modem	 					*/		
	.word	modem_int		/*  9	xx00 1001 .. 		 	   Modem	 		   + Audio	*/		
	.word	modem_int		/* 10	xx00 1010 ..			   Modem	  + SmCard			*/		
	.word	modem_int		/* 11	xx00 1011 .. 			   Modem	  + SmCard + Audio	*/		
	.word	modem_int		/* 12	xx00 1100 ..    		   Modem + IR					*/		
	.word	modem_int		/* 13	xx00 1101 .. 			   Modem + IR		   + Audio	*/		
	.word	modem_int		/* 14	xx00 1110 .. 			   Modem + IR + SmCard			*/		
	.word	modem_int		/* 15	xx00 1111 .. 			   Modem + IR + SmCard + Audio	*/		

	.word	keyboard_int	/* 16	xx01 0000 .. 		 Kbd 								*/		
	.word	audio_int		/* 17	xx01 0001 .. 		 Kbd		 			   + Audio	*/		
	.word	keyboard_int	/* 18	xx01 0010 .. 		 Kbd			  + SmCard			*/		
	.word	audio_int		/* 19	xx01 0011 .. 		 Kbd		 	  + SmCard + Audio	*/		
	.word	ir_int			/* 20	xx01 0100 .. 		 Kbd		 + IR					*/		
	.word	audio_int		/* 21	xx01 0101 .. 		 Kbd		 + IR 		   + Audio	*/		
	.word	ir_int			/* 22	xx01 0110 .. 		 Kbd		 + IR + SmCard			*/		
	.word	audio_int		/* 23	xx01 0111 .. 		 Kbd 		 + IR + SmCard + Audio	*/		
	.word	modem_int		/* 24	xx01 1000 .. 		 Kbd + Modem						*/		
	.word	modem_int		/* 25	xx01 1001 .. 		 Kbd + Modem 			   + Audio	*/		
	.word	modem_int		/* 26	xx01 1010 .. 		 Kbd + Modem 	  + SmCard			*/		
	.word	modem_int		/* 27	xx01 1011 .. 		 Kbd + Modem 	  + SmCard + Audio	*/		
	.word	modem_int		/* 28	xx01 1100 .. 		 Kbd + Modem + IR 					*/		
	.word	modem_int		/* 29	xx01 1101 .. 		 Kbd + Modem + IR 		   + Audio	*/		
	.word	modem_int		/* 30	xx01 1110 .. 		 Kbd + Modem + IR + SmCard			*/		
	.word	modem_int		/* 31	xx01 1111 .. 		 Kbd + Modem + IR + SmCard + Audio	*/		

	.word	video_int		/* 32	xx10 0000 .. Vid	  									*/		
	.word	audio_int		/* 33	xx10 0001 .. Vid	  	 					   + Audio	*/		
	.word	smartcard_int	/* 34	xx10 0010 .. Vid	  				  + SmCard			*/		
	.word	audio_int		/* 35	xx10 0011 .. Vid	  				  + SmCard + Audio	*/		
	.word	ir_int			/* 36	xx10 0100 .. Vid	  	 		 + IR					*/		
	.word	audio_int		/* 37	xx10 0101 .. Vid	  	 		 + IR		   + Audio	*/		
	.word	ir_int			/* 38	xx10 0110 .. Vid	  	 		 + IR + SmCard			*/		
	.word	audio_int		/* 39	xx10 0111 .. Vid	  	 		 + IR + SmCard + Audio	*/		
	.word	modem_int		/* 40	xx10 1000 .. Vid	  	 + Modem						*/		
	.word	modem_int		/* 41	xx10 1001 .. Vid	  	 + Modem	 		  + Audio	*/		
	.word	modem_int		/* 42	xx10 1010 .. Vid	  	 + Modem	  + SmCard			*/		
	.word	modem_int		/* 43	xx10 1011 .. Vid	  	 + Modem	  + SmCard + Audio	*/		
	.word	modem_int		/* 44	xx10 1100 .. Vid	  	 + Modem + IR					*/		
	.word	modem_int		/* 45	xx10 1101 .. Vid	  	 + Modem + IR 		   + Audio	*/		
	.word	modem_int		/* 46	xx10 1110 .. Vid	  	 + Modem + IR + SmCard			*/		
	.word	modem_int		/* 47	xx10 1111 .. Vid	  	 + Modem + IR + SmCard + Audio	*/		

	.word	keyboard_int	/* 48	xx11 0000 .. Vid + Kbd									*/		
	.word	audio_int		/* 49	xx11 0001 .. Vid + Kbd 					   		+ Audio	*/		
	.word	keyboard_int	/* 50	xx11 0010 .. Vid + Kbd 			  	+ SmCard 			*/		
	.word	audio_int		/* 51	xx11 0011 .. Vid + Kbd 			  	+ SmCard + Audio	*/		
	.word	ir_int			/* 52	xx11 0100 .. Vid + Kbd 		 + IR						*/		
	.word	audio_int		/* 53	xx11 0101 .. Vid + Kbd 		 + IR 		   	 + Audio	*/		
	.word	ir_int			/* 54	xx11 0110 .. Vid + Kbd		 + IR 	+ SmCard			*/		
	.word	audio_int		/* 55	xx11 0111 .. Vid + Kbd 	     + IR 	+ SmCard + Audio	*/		
	.word	modem_int		/* 56	xx11 1000 .. Vid + Kbd + Modem							*/		
	.word	modem_int		/* 57	xx11 1001 .. Vid + Kbd + Modem			   	 + Audio	*/		
	.word	modem_int		/* 58	xx11 1010 .. Vid + Kbd + Modem	  	+ SmCard			*/		
	.word	modem_int		/* 59	xx11 1011 .. Vid + Kbd + Modem	  	+ SmCard + Audio	*/		
	.word	modem_int		/* 60	xx11 1100 .. Vid + Kbd + Modem + IR 					*/		
	.word	modem_int		/* 61	xx11 1101 .. Vid + Kbd + Modem + IR		     + Audio	*/		
	.word	modem_int		/* 62	xx11 1110 .. Vid + Kbd + Modem + IR + SmCard			*/		
	.word	modem_int		/* 63	xx11 1111 .. Vid + Kbd + Modem + IR + SmCard + Audio	*/		


ENDFRAME(InterruptDecode)


FRAME(returnFromDebugger,sp,0,ra)
	
#if 0
	li		v0,kButtonIntMask
	sw		v0,kIntClearReg				/* ensure no pesky button ints are lying around */
#endif /* ¥¥¥Goober */

	j		restoreContextAndReturn
	nop
	
ENDFRAME(returnFromDebugger)

