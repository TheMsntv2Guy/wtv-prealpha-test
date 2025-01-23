
#include "iregdef.h"
#include "idtcpu.h"
#include "frame.h"
#include "HWRegs.h"


/*  ------------------------------------
	CPU Coprocessor 0 register accessors
*/


/* ulong FetchConfig(void) - read CP0 Config register & pass it back */

#ifndef APPROM
FRAME(FetchConfig,sp,0,ra)
	.set	noreorder

	mfc0	v0,C0_CONFIG
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(FetchConfig)
#endif


/* ulong FetchCalg(void) - read CP0 Config register & pass it back */

FRAME(FetchCalg,sp,0,ra)
	.set	noreorder

	mfc0	v0,C0_CALG
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(FetchCalg)


/* void SetCalg(ulong) - set CP0 Config register */

FRAME(SetCalg,sp,0,ra)
	.set	noreorder
	
	la		a1,setit
	li		t0,0x20000000
	or		a1,a1,t0
	jr		a1
	nop
setit:
	mtc0	a0,C0_CALG
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(SetCalg)


/* ulong FetchIWatch(void) - read CP0 IWatch register & pass it back */

#ifdef _INCLUDE_DEAD_CODE_
FRAME(FetchIWatch,sp,0,ra)
	.set	noreorder

	mfc0	v0,C0_IWATCH
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(FetchIWatch)
#endif


/* void SetIWatch(ulong) - set CP0 IWatch register */

#ifdef _INCLUDE_DEAD_CODE_
FRAME(SetIWatch,sp,0,ra)
	.set	noreorder

	mtc0	a0,C0_IWATCH
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(SetIWatch)
#endif


/* ulong FetchDWatch(void) - read CP0 DWatch register & pass it back */

#ifdef _INCLUDE_DEAD_CODE_
FRAME(FetchDWatch,sp,0,ra)
	.set	noreorder

	mfc0	v0,C0_DWATCH
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(FetchDWatch)
#endif


/* void SetDWatch(ulong) - set CP0 DWatch register */

#ifdef _INCLUDE_DEAD_CODE_
FRAME(SetDWatch,sp,0,ra)
	.set	noreorder

	mtc0	a0,C0_DWATCH
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(SetDWatch)
#endif


/* ulong SetCounter(ulong value) - set CP0 Count register */

FRAME(SetCounter,sp,0,ra)
	.set	noreorder

	mtc0	a0,C0_COUNT
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(SetCounter)

/* ulong FetchCounter(void) - read CP0 Count register & pass it back */

FRAME(FetchCounter,sp,0,ra)
	.set	noreorder

	mfc0	v0,C0_COUNT
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(FetchCounter)


/* ulong SetSR(ulong value) - set CP0 SR register */

FRAME(SetSR,sp,0,ra)
	.set	noreorder

	mtc0	a0,C0_SR
	jr	ra
	nop
	
	.set	reorder
ENDFRAME(SetSR)


/*  ------------------------------------
	Hardware Utility Routines
*/

	
/* ulong GetRandom(void) */
	
FRAME(GetRandom,sp,0,ra)
	.set	noreorder

	lw		v1,kVidCLineReg
	mfc0	v0,C0_COUNT
	sll		v1,v1,6
	xor		v0,v0,v1		/* random is swizzled line count ^ CPU counter */

	jr		ra
	nop	
	
ENDFRAME(GetRandom)



/* ulong TimeVBL(void) - used to compute CPU clock speed */
#ifndef APPROM
FRAME(TimeVBL,sp,0,ra)
	.set	noreorder

#ifdef SPOT3_REFRESH_HACK
	li		a1,0x48000000
#endif

	/* get current VBL and wait until we increment */	
	lw		t0,kVidCLineReg
	nop
1:
	lw		t1,kVidCLineReg
	nop

#ifdef SPOT3_REFRESH_HACK
	sw		a1,kMemCmdReg
#endif

	beq		t1,t0,1b
	nop
	
	mfc0	v0,C0_COUNT		/* start the counter */
	nop
	
	/* now wait until this line comes back */	
2:
	lw		t1,kVidCLineReg
	nop

#ifdef SPOT3_REFRESH_HACK
	sw		a1,kMemCmdReg
#endif

	bne		t1,t0,2b
	nop
	
	mfc0	v1,C0_COUNT		/* stop the counter */
	nop

	subu	v0,v1,v0		/* compute time spent */


#ifdef SPOT3_REFRESH_HACK
	mtc0	zero,C0_COUNT		/* start anew */
	nop
	nop
	nop
	nop
	
	li		t0,kRefreshCompare
	mtc0	t0,C0_COMPARE		/* init compare */
	nop
	nop
	nop
	nop
#endif


	jr		ra
	nop
	
ENDFRAME(TimeVBL)
#endif /* !APPROM */


/* void CallDebugger(void) - drop into debugger */

FRAME(CallDebugger,sp,0,ra)
	.set	noreorder

	break	0x72
	j		ra			
	nop

ENDFRAME(CallDebugger)



/* void EnableWatchdog(void) */

FRAME(EnableWatchdog,sp,0,ra)
	.set	noreorder

	lui		a0,kSPOTBaseHi
	
	lw		t0,kBusChipCntl(a0)		/* preserve AudClkDiv & TOCount */
	li		t1,0x3fffffff			/* wd enable bits are 31:30 */

	and		t0,t0,t1				
	sw		t0,kBusChipCntl(a0)		/* 00 */
	
	lui		t2,0x4000
	or		t1,t0,t2
	sw		t1,kBusChipCntl(a0)		/* 01 */
	
	lui		t2,0x8000
	or		t1,t0,t2
	sw		t1,kBusChipCntl(a0)		/* 10 */

	lui		t2,0xc000
	or		t1,t0,t2
	sw		t1,kBusChipCntl(a0)		/* 11 */

	j		ra			
	nop
	
ENDFRAME(EnableWatchdog)



/* void DisableWatchdog(void) */

#ifdef _INCLUDE_DEAD_CODE_
FRAME(DisableWatchdog,sp,0,ra)
	.set	noreorder

	lui		a0,kSPOTBaseHi
	
	lw		t0,kBusChipCntl(a0)		/* preserve AudClkDiv & TOCount */
	li		t1,0x3fffffff			/* wd enable bits are 31:30 */

	and		t0,t0,t1	
	
	lui		t2,0xc000
	or		t1,t0,t2
	sw		t1,kBusChipCntl(a0)		/* 11 */

	lui		t2,0x8000
	or		t1,t0,t2
	sw		t1,kBusChipCntl(a0)		/* 10 */

	lui		t2,0x4000
	or		t1,t0,t2
	sw		t1,kBusChipCntl(a0)		/* 01 */
	
	sw		t0,kBusChipCntl(a0)		/* 00 */
	
	lw		t0,kBusChipCntl(a0)		/* flush write buffer (disable wd NOW) */
	
	j		ra			
	nop
	
ENDFRAME(DisableWatchdog)
#endif




/*
	Interrupts are disabled for 52us*10 = 520us.
	
	void BangSerial(uchar c);
	
	a1 -> SPOT Base
	t0 = count for 19200 baud (calculated at startup)
	t4 = SR before we started
	t5 = kSmartCardClk
	
	t6 = saved counter for SPOT3 refresh hack
*/


FRAME(BangSerial,sp,0,ra)
	.set	noreorder

#ifdef SPOT3_REFRESH_HACK
	mfc0	t6,C0_COUNT
	nop
#endif

	mfc0	t4,C0_SR
	li		t1,0xfffffffe
	and		t1,t4,t1					/* clear IE, keep old SR */
	mtc0	t1,C0_SR


	lw		t0,gBaudCount
	lui		a1,kSPOTBaseHi
	beq		zero,t0,exitBangSerial		/* if BR count has not been set up, just exit */
	li		t5,kSmartCardClk			/* t5 = clk bit set */

										/* --- STOP BIT --- */
	sw		zero,kDevSmartCard(a1)		/* clear serial bit (marking) */
	lw		zero,kDevSmartCard(a1)		/* flush wbuf */

	mtc0	zero,C0_COUNT				/* clear counter */
	nop
	nop
	nop
	nop
waitStopTime:
	mfc0	t1,C0_COUNT					/* fetch count */
	nop
	blt		t1,t0,waitStopTime
	nop
	
	
										/* --- START BIT --- */
	sw		t5,kDevSmartCard(a1)		/* set serial bit */	
	lw		zero,kDevSmartCard(a1)		/* flush wbuf */

	mtc0	zero,C0_COUNT				/* clear counter */
	nop
	nop
	nop
	nop
waitStartTime:
	mfc0	t1,C0_COUNT					/* fetch count */
	nop
	blt		t1,t0,waitStartTime
	nop
	
	
										/* --- DATA BITS, 0->7 --- */
	li		t2,8						/* 8 databits, no parity */

bitloop:
	andi	t3,a0,1						/* check a bit */									
	beq		zero,t3,ItsAZero
	nop
	
ItsAOne:
	sw		zero,kDevSmartCard(a1)		/* mark */
	lw		zero,kDevSmartCard(a1)		/* flush wbuf */

	b		bitwait
	nop

ItsAZero:
	sw		t5,kDevSmartCard(a1)		/* space */
	lw		zero,kDevSmartCard(a1)		/* flush wbuf */

bitwait:
	mtc0	zero,C0_COUNT				/* clear counter */
	nop
	nop
	nop
	nop
waitBitTime:
	mfc0	t1,C0_COUNT					/* fetch count */
	nop
	blt		t1,t0,waitBitTime
	nop

	subu	t2,t2,1						/* one less bit to send */
	srl		a0,a0,1						/* shift next bit down */
	bne		zero,t2,bitloop				/* send em all */
	nop
										
										/* --- DONE --- */
	sw		zero,kDevSmartCard(a1)		/* exit with serial bit marking */
	lw		zero,kDevSmartCard(a1)		/* flush wbuf */
	

exitBangSerial:

#ifdef SPOT3_REFRESH_HACK
	mtc0	t6,C0_COUNT			/* restore - lost 520us, but that's oK */
	nop
	nop
	nop
	nop
	
	li		t0,kRefreshCompare
	mtc0	t0,C0_COMPARE		/* init compare */
	nop
	nop
	nop
	nop
#endif

	mtc0	t4,C0_SR					/* put old SR back */

	jr		ra
	nop

	.set	reorder
ENDFRAME(BangSerial)

