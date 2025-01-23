 
#include "iregdef.h"
#include "idtcpu.h"
#include "BoxHWEquates.h"
#include "HWRegs.h"
#include "CrashLog.h"


#define		kStackUpper		0x8000

	.set noreorder

.ent	start
.globl	start
start:
.frame	sp,0,$31

resetentry:
	b		real_start
	nop
	
rom_checksum:	.word	0x00000000			/* checksum, NOT including the checksum itself */
rom_size:		.word	0x00000000			/* size in bytes */
code_size:		.word	0x00000000			/* size in bytes */
version:		.word	0x00000000			/* version number */
data_start:		.word	0x00000000			/* address of .data in ROM image */
data_len:		.word	0x00000000			/* .data len, in WORDS */
bss_len:		.word 	0x00000000			/* .bss len, in WORDS */
	

#ifdef SPOT3_REFRESH_HACK
	
/*	============================================================================
	EVIL FUCKED UP SPOT3 REFRESH HACK START
*/

/* 	Int handler needs to be at 0xbfc00400 (bfc00200 + 200 (vector)).
	By this point, we've chewed up ((2*4) + (7*4)) = 36 bytes.
	So, chew up 1024 - 36 = 988 bytes to get the int handler at the right place.
*/

	.space	988


/*	This only runs until the main system interrupts are installed and running.
	Only use k0 and k1 so we don't have to save/restore anything.
*/

Spot3_Refresh:
	
#if 0 /* for debugging */
	lw		k0,kDevLEDsReg
	nop
	xori	k0,k0,kMessageLED		/* toggle message led (alive) */
	sw		k0,kDevLEDsReg
#endif


	li		k0,1024					/* refresh 1024 times */
	li		k1,0x48000000
1:
	sw		k1,kMemCmdReg
	subu	k0,k0,1
	bne		zero,k0,1b
	nop
	lw		zero,kMemCmdReg			/* flush write buffer */
	
	mtc0	zero,C0_COUNT			/* start anew */
	nop
	nop
	nop
	nop
	
	li		k0,kRefreshCompare
	mtc0	k0,C0_COMPARE			/* clear interrupt */
	nop
	nop
	nop
	nop
		
	eret	
	nop								/* this never gets executed */

/*	EVIL FUCKED UP SPOT3 REFRESH HACK END
	============================================================================
*/

#endif /* SPOT3_REFRESH_HACK */

	
real_start:
	
	li		v0,0x10010000			/* Cu0, Disable Cache parity error exceptions */	
	mtc0	v0,C0_SR			


	#
	# Initialize Memory controller & CPU here
	#

	li		v1,0x7ffff000		/* init Bases to 0 and Bounds to top of addr space */
	mtc0	zero,C0_IBASE
	mtc0	v1,C0_IBOUND
	mtc0	zero,C0_DBASE
	mtc0	v1,C0_DBOUND


	lui		a0,kSPOTBaseHi		

	# Initialize ROM Timing
	
	li		t0,kROMCntl1_Init
	sw		t0,kROMCntl1(a0)
		
	# Initialize Audio clk divisor & Bus Timeout
	
	li		t0,(kAudClkDiv3 + 0x0000ffff)
	sw		t0,kBusChipCntl(a0)

	
	# Initialize SGRAM Controller - use safe default values.
	# Bus speed is calculated later & used to reinit SGRAM controller.

	sw		zero,kMemData(a0)

	lui		t0,0x8800
	sw		t0,kMemCmd(a0)
	
	lui		t0,0x4800
	sw		t0,kMemCmd(a0)		
	sw		t0,kMemCmd(a0)		
	sw		t0,kMemCmd(a0)		
	sw		t0,kMemCmd(a0)		
	sw		t0,kMemCmd(a0)	
	sw		t0,kMemCmd(a0)	
	sw		t0,kMemCmd(a0)	
	sw		t0,kMemCmd(a0)	
		
	li		t0,0x280000c0
	sw		t0,kMemCmd(a0)
		
	lui		t0,0x8800
	sw		t0,kMemCmd(a0)	

	lw		t1,kBusChipID(a0)
	nop
	beq		zero,t1,WeAreSPOT1

#if 0	
	li		t0,0x5				/* for SPOT3 */
#else
	li		t0,0x0				/* FIXME for SPOT3 workaround */
#endif

 	li		t6,0x21211111		/* FIXME - should be write back, write allocate */
	b		KeepGoing
	nop

WeAreSPOT1:
	li		t0,0x9c				/* for SPOT1 */
  	li		t6,0x21211111		/* write through, write allocate */
	
KeepGoing:
	sw		t0,kMemRefCnt(a0)	
	lw		zero,kMemRefCnt(a0)	/* flush write buffer */
	nop
	mtc0	t6,C0_CALG 			/* Initialize CPU Cache Algorithm for each segment */
	nop



#ifdef SPOT3_REFRESH_HACK

/*	============================================================================
	EVIL FUCKED UP SPOT3 REFRESH HACK INIT START
	
	Hack assumes system is running with 30MHz osc (that should be slower than these
	SPOT3 boards ever run).  Typically run with 36MHz osc.
	
	30MHz * 3 = 90MHz CPU instruction issue rate
	Count reg incs at half that, so 45MHz.
	
	1/45MHz = 22ns.
	
	We need to whack the RAM every 15ms, so that's:  15ms/22ns = 675,000 = 0xa4cb8
	counts.
	
	This value is defined in HWRegs.h.
*/

	mfc0	v0,C0_CAUSE
	li		v1,CAUSE_IV
	or		v0,v0,v1			/* set IV bit */
	mtc0	v0,C0_CAUSE
	
	mtc0	zero,C0_COUNT		/* start anew */
	nop
	nop
	nop
	nop
	
	li		k0,kRefreshCompare
	mtc0	k0,C0_COMPARE		/* init compare */
	nop
	nop
	nop
	nop
	
	li		v0,0x10418001		/* BEV = 1, IM = 80 (counter), IE */	
	mtc0	v0,C0_SR			
		
	
/*	EVIL FUCKED UP SPOT3 REFRESH HACK INIT END
	============================================================================
*/

#endif /* SPOT3_REFRESH_HACK */


	
	#
	# Initialize CPU caches
	#
	
	# DATA
	
	mtc0	zero,C0_TAGLO		/* Cache state == 00 --> Invalid */
	li		a0,0x9fe00000		/* a0 -> base of cacheable seg */
	li		t0,255				/* 8KB, 32bytes/line -> 256 lines */
	
DInitLoop:
	cache	0x9,0(a0)			/* Index Store Tag - Set 0 (bit 13 selects set) */
	cache	0x9,0x2000(a0)		/* Index Store Tag - Set 1 (bit 13 selects set) */
	
	lw		zero,0(a0)			/* load good data to clear dirty bits & set good parity */
	lw		zero,0x2000(a0)
	
	cache	0x9,0(a0)			/* re-invalidate the tags */
	cache	0x9,0x2000(a0)
	
	addu	a0,32				/* next line... */
	bgtz	t0,DInitLoop
	addi	t0,-1
	
	# INSTRUCTION
	
	mtc0	zero,C0_TAGLO
	li		a0,0x9fe00000
	li		t0,255
	
IInitLoop:
	cache	0x8,0(a0)			/* Index Store Tag - Set 0 (bit 13 selects set) */		
	cache	0x8,0x2000(a0)		/* Index Store Tag - Set 1 (bit 13 selects set) */
	
	cache	0x14,0(a0)			/* fill I cache from memory */
	cache	0x14,0(a0)
	
	cache	0x8,0(a0)			/* re-invalidate the tags */		
	cache	0x8,0x2000(a0)		

	addu	a0,32				/* next line... */
	bgtz	t0,IInitLoop
	addi	t0,-1
	
	#
	# CPU/Memory controller setup end
	#

	

#if 0

	#
	# Size RAM
	#
								/* System memory is always added in 1MB increments.		*/
	li		t0,0xa0700000		/* 8MB max */
	li		t1,0xdeadc0ed	
sizeLoop:
	lw		t2,0(t0)			/* save it */
	nop
	sw		t1,0(t0)			/* write signature */
	lw		t3,0(t0)			/* read it back */
	nop
	beq		t1,t3,ramSized
	sw		t2,0(t0)			/* put it back */
	subu	t0,t0,0x00100000
	b		sizeLoop
	nop

ramSized:
	subu	t0,t0,0xa0000000
	addu	t0,t0,0x00100000

#else

	#
	# Fake it for now - 2MB sys RAM
	#
	li		t0,0x00200000		

#endif


	#
	# Test/Clear RAM
	# (but DO NOT alter crashlog space!)
	#

	li		t2,0xc0edbabe					/* init value */

	li		t1,(kCrashLogBase&0x0fffffff)	/* init up to crashlog */	
	lui		a0,0xa000						/* a0 -> base of RAM */
initLowRAMloop:
	sw		t2,0(a0)
	subu	t1,t1,4
	bne		zero,t1,initLowRAMloop
	addiu	a0,a0,4


	subu	t1,t0,(kCrashLogTop&0x0fffffff)	/* RAM size - what we've just inited */
	li		a0,kCrashLogTop					/* a0 -> just above crashlog */
initHiRAMloop:
	sw		t2,0(a0)
	subu	t1,t1,4
	bne		zero,t1,initHiRAMloop
	addiu	a0,a0,4

	
	#
	# Init stack, stash ram size
	#

	sw		t0,kAbsGlobalsBase	/* Stash mem size */
		
	lui		t1,kStackUpper		/* Init stack at top of available RAM */		
	or		t0,t0,t1
	subu	t0,t0,kStackOffset
	subu	t0,t0,4
	move	sp,t0				/* set up the stack under any patches that are present */
		
		
	#
	# Now copy data section to RAM, initialize bss
	#

								/* copy data to data area */
	lw		t0,data_start
	lw		t1,data_len
	li		t3,kDataSegmentBase		/* where data goes at runtime */
copyDataArea:
	lw		t2,0(t0)
	addu	t0,t0,4
	subu	t1,t1,1
	sw		t2,0(t3)
	bne		zero,t1,copyDataArea
	addu	t3,t3,4
	
								/* zero bss area, bss placed right after data */
	lw		t1,bss_len
zeroBssArea:
	sw		zero,0(t3)
	subu	t1,t1,1
	bne		zero,t1,zeroBssArea
	addu	t3,t3,4

	la		gp,_gp				/* set up the globals pointer */
				
	la		t0,main				/* t0 -> our C entry point */
	j		t0					/* here we go... (we won't be back!) */
	nop
	
.end	start


#
# Move the stack to wherever the caller tells us.
# This should only be called by main() after determining where the system stack should go.
#

.ent	move_reset_stack
.globl	move_reset_stack
move_reset_stack:
.frame	sp,0,$31

	move	sp,a0
	
	j		ra
	nop

.end	move_reset_stack



#
# main() will call this first
#

.ent	__main
.globl	__main
__main:
.frame	sp,0,$31

	j		ra
	nop

.end	__main



#
#	InitSGRAM - used by BoxBoot.c to program the SGRAM timings
#
#	a0 = MEM_CNTL
#	a1 = MEM_TIMING
#	a2 = MEM_REFCNT
#	a3 = MEM_CMD_MRS
#

.ent	InitSGRAM
.globl	InitSGRAM
InitSGRAM:
.frame	sp,0,$31

	.set	noreorder
	
	lui		t1,kSPOTBaseHi
		
	sw		a0,kMemCntl(t1)		
	
	sw		a1,kMemTiming(t1)		
			
	sw		a2,kMemRefCnt(t1)		/* For SPOT3, need to set AFTER SGRAM init */

	sw		a3,kMemCmd(t1)	
	
	lw		zero,kMemCmd(t1)		/* flush write buffer */
	
	jr		ra
	nop
	
	.set	reorder

.end	InitSGRAM



#
#	Jump to start of App ROM image (never returns!)
#

.ent	JumpToApp
.globl	JumpToApp
JumpToApp:
.frame	sp,0,$31

	.set	noreorder

	li		t0,kAppROMBase	
	jr		t0
	nop

	.set	reorder

.end	JumpToApp
