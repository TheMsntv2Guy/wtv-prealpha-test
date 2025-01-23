
#include "iregdef.h"
#include "idtcpu.h"
#include "frame.h"
#include "HWRegs.h"

#undef DEBUG_MSG

#define kGooberRead 	0x0				/* read some stuff from mac */
#define kGooberWrite 	0x1				/* write some stuff to mac */
#define kGooberGo		0x2				/* go someplace (execute at an address) */

#define kGooberCodeBase	0x801F0000		/* we live in top 64k of CACHEABLE second meg */

#define kEPROMEnable	0
#define kSRAMEnable		1

#define SetGooberMode(mode)		li t0,mode ; sw t0,kGooberControl ; lw zero,kGooberControl ;
#define KillVideo()				sw	zero,0xa4003018 ; sw zero,0xa4003014 ; lw zero,0xa4003014 ;


/*
	basic protocol: 
 		- read command byte 
		- read count
		- read address
		- do it
*/

/* this never exits */
FRAME(GooberDownloader,sp,0,ra)
	.set	noreorder
GOOBER_START:

	/* first thing we do is copy ourselves into RAM */

	la		a0,GooberDownloader				/* get address of this func */
	li		a1,kGooberCodeBase				/* get RAM dst addr */
	li		t0,GOOBER_END-GOOBER_START		/* get the size, too */
1:
	lw		t1,0(a0)						/* get a word */
	addiu	a0,a0,4							/* bump src ptr */
	sw		t1,0(a1)						/* store a word */
	subu	t0,t0,4							/* dec counter */
	bne		t0,zero,1b						/* go back if more to do */
	addiu	a1,a1,4							/* bump dst ptr */
	
	la		s0,printf						/* keep address of printf around */
	la		s1,clear_cache					/* keep address of clear_cache around */

	/* now jump to entry point in RAM */
	
	li 		t0,GooberEntry-GOOBER_START
	li 		t1,kGooberCodeBase
	addu	t0,t0,t1
	jalr	ra,t0
	nop

	/* main downloader code */
	/* monitors the STROBE bit to determine if there's anything to do */
	
GooberEntry:

	mfc0 	t0,C0_SR
	li 		a0,0xFFFFFFFE					/* clear the IE bit to disable interrupts */
	and 	t0,t0,a0
	mtc0	t0,C0_SR

	/* 
		make sure ACK is not asserted (really it is bit 4 of parallel cntl out) 
		set direction to be in
	*/
	
	li		a1,kAckBit						/* set ack bit */
	sw		a1,kGooberParallelCntrlOut		/* clear the register */
	lw		zero,kGooberParallelCntrlOut	
	
GooberLoop:

	la		a0,gGooberHello					/* announce our presence */
	jalr	ra,s0							/* call printf in ROM */
	nop	

	bal		ReadByte						/* get command byte */
	nop

	bne		zero,v0,1f						/* check if cmd 0 */ 
	nop
	bal		GooberRead
	nop
	b		GooberLoop						/* get next command */
	nop
1:	
	li		t0,1							/* check if cmd 1 */
	bne		t0,v0,2f
	nop
	bal		GooberWrite
	nop
	b		GooberLoop						/* get next command */
	nop
2:	
	li		t0,2							/* check if cmd 2 */
	bne		t0,v0,3f
	nop
	bal		GooberGo
	nop
	b		GooberLoop						/* get next command */
	nop
3:	
	bal		TestCmd
	nop
	b		GooberLoop						/* get next command */
	nop
	


/***********************************************************************************
	read a byte from parallel port
	returns byte in v0
***********************************************************************************/

ReadByte:

	li		a1,kStrobeBit					/* check strobe bit */
waitForStb:
	lw		a0,kGooberParallelCntrlIn	
	nop
	and		a0,a0,a1						
	beq		zero,a0,waitForStb				/* wait for Stb SET */
	nop

fetchData:
	lw		v0,kGooberParallelIn			/* fetch the data byte */						
	nop

ackIt:
	li		a0,kAckBit						
	sw		a0,kGooberParallelCntrlOut		/* set ACK bit */
	lw		zero,kGooberParallelCntrlOut	/* flush write buf */
	
											/* a1 still == kStrobeBit */
waitNotStb:
	lw		a0,kGooberParallelCntrlIn
	nop
	and		a0,a0,a1						
	bne		zero,a0,waitNotStb				/* wait for Stb CLEAR */
	nop

releaseAck:
	sw		zero,kGooberParallelCntrlOut	/* clear ACK bit */
	lw		zero,kGooberParallelCntrlOut	/* flush write buf */
	
	
	jr		ra								/* return */
	nop
	
	
	
/***********************************************************************************
	read four bytes from parallel port
	returns value in v0
	returns checksum in v1
	trashes s7
***********************************************************************************/
ReadFourBytes:

	move	t0,ra							/* save our RA */
	move	v1,zero							/* zero checksum */
	
	bal		ReadByte						/* get first byte */
	nop
	addu	v1,v1,v0						/* update checksum */
	sll		v0,v0,24						/* shift it left 24 bits */
	move	s7,v0							/* save it */
	
	bal		ReadByte						/* get next byte */
	nop
	addu	v1,v1,v0						/* update checksum */
	sll		v0,v0,16						/* shift it left 16 bits */
	or		s7,s7,v0						/* update value */
	
	bal		ReadByte						/* get next byte */
	nop
	addu	v1,v1,v0						/* update checksum */
	sll		v0,v0,8							/* shift it left 8 bits */
	or		s7,s7,v0						/* update value */
	
	bal		ReadByte						/* get next byte */
	nop
	addu	v1,v1,v0						/* update checksum */
	or		s7,s7,v0						/* update value */
	
	move	v0,s7							/* put return value in proper place */
	
	jr		t0								/* return */
	nop



/***********************************************************************************
	READ <count> <address>
	
	read <count> bytes starting from <address> 
	(used in response to Mac "squirt" command)
	trashes s4,s5,s6
	NOTE: assumes <count> is multiple of 4
***********************************************************************************/
GooberRead:

	/*
		s3 = saved RA
		s4 = checksum
		s5 = address
		s6 = count
	*/

	move	s3,ra							/* save ra */
		
	bal		ReadFourBytes					/* get the count (4 bytes) */
	nop
	move	s6,v0							/* store count */
	
	bal		ReadFourBytes					/* get the address (4 bytes) */
	nop
	move	s5,v0							/* store address */
	
	la		a0,gGooberReadStr				/* print info */
	move	a1,s6							/* count */
	move	a2,s5							/* address */
	jalr	ra,s0							/* call printf in ROM */
	nop

	/* write <count> bytes to <address> */
	
	SetGooberMode(kSRAMEnable)				/* turn on access to SRAM */

	move	s4,zero							/* initialize checksum */
readAllWords:
	bal		ReadFourBytes					/* get a long */
	subu	s6,s6,4							/* decr counter */
	sw		v0,0(s5)						/* store it */
	addu	s4,s4,v1						/* update checksum */
	bne		s6,zero,readAllWords			/* go back if more */
	addiu	s5,s5,4							/* incr address */

	SetGooberMode(kEPROMEnable)				/* turn on access to EPROM */

	la		a0,gCheckSum					/* print checksum */
	move	a1,s4
	jalr	ra,s0							/* call printf in ROM */
	nop

	/* clean up and return */
	
	jr		s3								/* return */
	nop
	
	
	
/***********************************************************************************
	WRITE <count> <address>
	
	write <count> bytes starting from <address>
	(used in response to Mac "slurp" command)
***********************************************************************************/
GooberWrite:

	/*
		s4 = checksum
		s5 = address
		s6 = count
	*/

	subu	sp,sp,0x20						/* make room on stack */
	sw		ra,0x1c(sp)						/* save ra */
		
	li		s4,0							/* initialize checksum */
	
	bal		ReadFourBytes					/* get the count (4 bytes) */
	nop
	or		s6,v0,v0						/* store count */
	
	bal		ReadFourBytes					/* get the address (4 bytes) */
	nop
	or		s5,v0,v0						/* store address */
	
	la		a0,gGooberWriteStr
	or		a1,s6,s6
	or		a2,s5,s5
	jalr	ra,s0							/* call printf in ROM */
	nop
	
	/* read <count> bytes from address */
	
	/* FIXME - need to implement this */
	
	/* clean up and return */
		
	lw		ra,0x1c(sp)						/* restore ra */
	addu	sp,sp,0x20						/* clean up stack */
	jr		ra								/* return */
	nop



/***********************************************************************************
	GO <address>
	
	executing starting from <address>
	(used in response to Mac "go" command)
	
	note that we probably will not come back
***********************************************************************************/
GooberGo:

	move	s3,ra							/* save ra in s3 */

	KillVideo()

	bal		ReadFourBytes					/* get the address (4 bytes) */
	nop
	move	s7,v0							/* save result */
	
#if 0	
	/* FIXME - why isn't this working? */
	move	a0,v0							/* clear the caches */
	li		a1,0x2000
	jalr	ra,s1
	nop
#endif	

	SetGooberMode(kSRAMEnable)				/* turn on access to SRAM */
	
	jalr	ra,v0							/* jump to it */
	nop
	
	/* note that we're probably not coming back! */
	
	jr		s3								/* return */
	nop
	
	
	
/***********************************************************************************
	dummy command for testing
***********************************************************************************/
TestCmd:

	subu	sp,sp,0x20						/* make room on stack */
	sw		ra,0x1c(sp)						/* save ra */
	
	la		a0,gTestStr
	jalr	ra,s0							/* call printf in ROM */
	nop

	lw		ra,0x1c(sp)						/* restore ra */
	addu	sp,sp,0x20						/* clean up stack */
	jr		ra								/* return */
	nop
	
/***********************************************************************************/
DispatchTable:
	.word	GooberRead						/* 0x0 read data from mac */
	.word	GooberWrite						/* 0x1 write data to mac */
	.word	GooberGo						/* 0x2 jump someplace */
	.word	TestCmd							/* 0x3 testing */
	
GOOBER_END:	
	.set	reorder
ENDFRAME(GooberDownloader)

