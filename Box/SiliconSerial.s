
#include "iregdef.h"
#include "idtcpu.h"
#include "frame.h"

#include "HWRegs.h"

/* 
	Interrupts are disabled for about 7ms, C0_COUNT is altered.

	Boolean ReadSiliconSerialNumber(ulong countsPerMicro, uchar *databuf);

entry:
	a0 = # counts/microsec
	a1 = ptr to 8 bytes to store raw data in
	
used:
	a2 = SPOT base
	
	t0 = arg to DelayUS
	t1 = scratch2
	t2 = bit count for reads/writes to device
	
	t4 = SR before we started
	t5 = saved ra
	t6 = scratch
		
returns:
	v0 = Boolean, 0 if we couldn't find the part, 1 if we did.
	
*/

FRAME(ReadSiliconSerialNumber,sp,0,ra)
	.set 	noreorder

	move	t5,ra	
	move	v0,zero						/* if no part, return 0 */

	lui		a2,kSPOTBaseHi				/* a2 -> SPOT */

	mfc0	t4,C0_SR
	li		t6,0xfffffffe
	and		t6,t4,t6					/* clear IE, keep old SR */
	mtc0	t6,C0_SR

	
	/* RESET PULSE/PRESENCE DETECT */
	
	sw		zero,kDevIDChip(a2)			/* drive low for reset pulse */
	lw		zero,kDevIDChip(a2)			/* flush */
	
	jal		DelayUS
	li		t0,500						/* Reset Pulse is >480us */
	
	li		t0,2
	sw		t0,kDevIDChip(a2)			/* let it go, should hear back from part */
	lw		zero,kDevIDChip(a2)			/* flush */


	jal		DelayUS
	li		t0,60+7						/* delay into presence pulse */
	
	lw		t6,kDevIDChip(a2)			/* sample, should be LOW */
	nop
	andi	t6,t6,1	
	bne		t6,zero,noIDChip
	nop
	
	jal		DelayUS
	li		t0,500						/* delay for AT LEAST 480us after reset */

	
	/* ISSUE COMMAND WORD (0x0f for DS2400 compatibility) */

	li		t1,0x0f	
	li		t2,8
	
cmdIssue:
	andi	t6,t1,1						/* xmt lsb 1st */
	beq		t6,zero,writeAZero
	srl		t1,t1,1				
	
writeAOne:
	sw		zero,kDevIDChip(a2)			/* drive low */
	lw		zero,kDevIDChip(a2)			/* flush */

	jal		DelayUS
	li		t0,8						/* 1us < Tlow1 < 15us */

	li		t0,2
	sw		t0,kDevIDChip(a2)			/* let it go */
	lw		zero,kDevIDChip(a2)			/* flush */

	jal		DelayUS
	li		t0,100-8					/* 60us < Tslot < 120us */
		
	j		writeCommon
	nop
	
writeAZero:
	sw		zero,kDevIDChip(a2)			/* drive low */
	lw		zero,kDevIDChip(a2)			/* flush */

	jal		DelayUS
	li		t0,100						/* 60us < Tlow0 < 120us */

	li		t0,2
	sw		t0,kDevIDChip(a2)			/* let it go */
	lw		zero,kDevIDChip(a2)			/* flush */

	jal		DelayUS
	li		t0,105-100					/* 60us < Tslot < 120us */

writeCommon:	
	jal		DelayUS
	li		t0,1						/* 1us delay between slots */

	subu	t2,t2,1
	bne		t2,zero,cmdIssue
	nop
	
	
	/* Read 8-bit device ID, 48-bit serial number, and 8-bit CRC */
	
	jal		Read1WireWord				/* read a word into t1 */
	nop
	sw		t1,0(a1)					/* stash in 1st 4 bytes */

	jal		Read1WireWord				/* read a word into t1 */
	nop
	sw		t1,4(a1)					/* stash in 2nd 4 bytes */


	li		v0,1						/* return success */


noIDChip:

#ifdef SPOT3_REFRESH_HACK

	/* === EVIL FUCKED UP SPOT3 REFRESH HACK START === */
	/* We've turned off interrupts for about 7ms.  Do a
	   refresh, then set back up the count/compare regs.
	   
	   Only happens at boot, doesn't hurt to do it for SPOT 1 & 3.
	*/

	li		a0,1024						/* refresh 1024 times */
	li		a1,0x48000000
1:
	sw		a1,kMemCmdReg
	subu	a0,a0,1
	bne		zero,a0,1b
	nop
	lw		zero,kMemCmdReg				/* flush write buffer */

	mtc0	zero,C0_COUNT				/* start anew */
	nop
	nop
	nop
	nop
	
	li		a0,kRefreshCompare
	mtc0	a0,C0_COMPARE				/* init compare */
	nop
	nop
	nop
	nop

	/* ==== EVIL FUCKED UP SPOT3 REFRESH HACK END ==== */

#endif /* SPOT3_REFRESH_HACK */

	mtc0	t4,C0_SR					/* put old SR back */
	
	jr		t5							/* return w/ptr to nums in v0 */
	nop
	
ENDFRAME(ReadSiliconSerialNumber)





/* 	
entry:
	a0 = counts/microsec
	a2 = SPOT base
	
used:
	t0 = arg to DelayUS
	
	t2 = bit count for reads/writes to device
	t3 = saved ra
	
	t6 = scratch

returns:
	t1 = byte read
*/

FRAME(Read1WireWord,sp,0,ra)
	.set	noreorder
	
	move	t3,ra
	
	move	t1,zero	
	li		t2,32
	
read1WireBit:
	jal		DelayUS
	li		t0,1						/* 1us delay between slots */

	sw		zero,kDevIDChip(a2)			/* drive low */
	lw		zero,kDevIDChip(a2)			/* flush */

	jal		DelayUS
	li		t0,2						/* 1us < Tsu */

	li		t0,2
	sw		t0,kDevIDChip(a2)			/* let it go */
	lw		zero,kDevIDChip(a2)			/* flush */

	jal		DelayUS
	li		t0,9-2						/* Trdv < 15us */

	lw		t6,kDevIDChip(a2)			/* sample */
	nop
	andi	t6,t6,1	
	bne		t6,zero,readAOne
	nop
										/* recv lsb 1st */										
readAZero:
	srl		t1,t1,1						/* bump next bit into position */
										/* insert a 0 */
	j		readCommon
	nop
	
readAOne:
	srl		t1,t1,1						/* bump next bit into position */
	lui		t6,0x8000
	or		t1,t1,t6					/* insert a 1 */
	
readCommon:	
	jal		DelayUS
	li		t0,105-9					/* 60us < Tslot < 120us */

	subu	t2,t2,1
	bne		t2,zero,read1WireBit
	nop

	jr		t3
	nop
	
ENDFRAME(Read1WireWord)



	
	
/* 	
entry:
	a0 = counts/microsec
	
	t0 = # us to delay
	
used:
	t6 = scratch
*/
	
FRAME(DelayUS,sp,0,ra)
	.set 	noreorder

	multu	a0,t0		
	mflo	t0
							
	mtc0	zero,C0_COUNT			/* clear counter */
	nop
	nop
	nop
	nop

waitDelay:
	mfc0	t6,C0_COUNT
	nop
	blt		t6,t0,waitDelay
	nop

	jr	ra
	nop

ENDFRAME(DelayUS)
