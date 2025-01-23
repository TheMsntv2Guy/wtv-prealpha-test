#include "iregdef.h"
#include "idtcpu.h"
#include "frame.h"

#include "HWRegs.h"


#define	kFlashEraseErr		1
#define	kFlashWriteErr		2


FRAME(DupeFlasher,sp,0,ra)
	.set	noreorder

	la		a0,FlashBlockRAM			
	lui		a1,0xc000					/* base of write-thru cached RAM */
	li		t0, 0x11c	
	
copyFlasher:
	lw		t1,0(a0)
	addiu	a0,a0,4
	sw		t1,0(a1)
	
	subu	t0,t0,4
	bne		t0,zero,copyFlasher
	addiu	a1,a1,4
	
	/* NEED TO FLUSH D CACHE HERE */
	
	jr		ra
	nop

	.set	reorder
ENDFRAME(DupeFlasher)



FRAME(CallRAMFlasher,sp,0,ra)
	.set	noreorder

	lui		t0,0xc000					/* base of write-thru cached RAM */
	jr		t0
	nop

	.set	reorder
ENDFRAME(CallRAMFlasher)



/*
	ulong FlashBlockRAM(ulong *src, ulong *dst)

	Erase some flash, then copy data to it.
	
	Unfortunately, the flash ROMs can only be erased in 64KB chunks, and the ROM
	is 2 flash ROMs wide.  That means that the smallest eraseable chunk is 128KB.
	
	The NV storage won't (can't) be 128KB, so the higher-level code must copy 128KB to RAM, 
	modify the NV storage part, then call this routine to flash the new NV data and re-flash
	the remainder of the 128KB block.
	
	Flash download code will always flash 128KB at a time.
		
	Before Calling:
		+ Flash must be write-enabled.
		+ Fences must be disabled.
		
	Returns an error if the Flash erase or write times out.

	FOR TRIAL 1
	-----------
	
	On these systems, there is only one set of ROMs, and it is Flash.  To program
	these devices, this code is copied to RAM, and executed from there for the
	duration of the flashing operation.
	
	A convenient place to stick this code is the low 256 bytes of RAM, which are
	normally unused (hence the fence disable requirement).  This means that this
	routine can be no more than 64 instructions long!


	a0 = src
	a1 = dst

*/


FRAME(FlashBlockRAM,sp,0,ra)
	.set 	noreorder

FLASH_START:

	/* ***********
	   Erase Block
	   *********** */
	
eraseBlock:
	li		t0,0x20202020			/* single block erase cmd, both parts */
	sw		t0,0(a1)
	li		t0,0xd0d0d0d0			/* confirm cmd, both parts */
	sw		t0,0(a1)
	lw		zero,0(a0)				/* flush write buf */
	
	li		t1,0x00800080			/* WSM ready, both parts */

#ifdef SPOT3_REFRESH_HACK
	move	t3,zero
	li		t4,28					/* this causes a refresh roughly every 16us */
#endif

waitErase:	

#ifdef SPOT3_REFRESH_HACK
	addiu	t3,t3,1
	bne		t4,t3,noRefYet
	nop
	
	move	t3,zero
	li		t0,0x48000000			
	sw		t0,kMemCmdReg			
	
noRefYet:
#endif

	lw		t0,0(a1)				/* fetch flash status */
	nop
	and		t0,t0,t1
	bne		t0,t1,waitErase			/* ### NO TIMEOUT FOR NOW */
	nop
	
eraseTimeout:	
	move	v0,t0					/* copy status to return val reg */
	
	li		t0,0x50505050			/* clear status reg cmd */
	sw		t0,0(a1)
	li		t0,0xffffffff			/* read array mode */
	sw		t0,0(a1)
	lw		zero,0(a0)				/* flush write buf */

	li		t0,0x00200020
	and		t0,t0,v0				/* mask all but erase err bits */
	bne		zero,t0,ReturnEraseErr	/* if err bits set, return 'em */
	nop
	

	/* ***********
	   Write Block
	   *********** */	
	
writeBlock:
	li		t1,0x10101010			/* t1 = write word cmd */
	li		t2,0x00800080			/* t2 = WSM ready, both parts */
	li		t3,131072				/* t3 = byte count to write */
	move	a2,a1					/* a2 = copy of block base */
						
writeLoop:
	lw		v1,0(a0)				/* fetch src word */
	addiu	a0,a0,4					/* bump src offset (delay) */
	
	sw		t1,0(a1)				/* write word cmd */
	sw		v1,0(a1)				/* write data */
	
waitWrite:
	lw		t0,0(a1)				/* fetch flash status (also flushes write buffer) */
	nop
	and		t0,t0,t2				/* mask all but WSM bits */
	bne		t0,t2,waitWrite			/* ### NO TIMEOUT FOR NOW */
	nop

#ifdef SPOT3_REFRESH_HACK
	li		t0,0x48000000			/* programming takes ~340ms.  340ms/32768words = ~10us/word */
	sw		t0,kMemCmdReg			/* 1024*10us = ~10ms, only need 1024/16ms */
#endif

	addiu	t3,t3,-4
	bne		zero,t3,writeLoop
	addiu	a1,a1,4					/* bump dst offset */

	move	v0,zero					/* return no err */

	li		t0,0x50505050			/* clear status reg cmd */
	sw		t0,0(a2)
	li		t0,0xffffffff			/* read array mode */
	sw		t0,0(a2)
	lw		zero,0(a2)				/* flush write buf */

	lw		zero,kVidCSizeReg
	nop

	/* ********************
	   Exit, w/ or w/o Error
	   ******************** */	

ReturnEraseErr:	
	
	j		ra						/* errs in v0 */
	nop

ReturnProgramErr:
	move	v0,t0					/* what was flash status? */
	b		ReturnEraseErr
	nop
	
FLASH_END:

	.set	reorder
ENDFRAME(FlashBlockRAM)
