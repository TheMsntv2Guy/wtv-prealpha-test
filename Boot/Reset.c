#include "idtcpu.h"
#include "iregdef.h"
#include "BoxHWEquates.h"
#include "excepthdr.h"
#include "Exceptions.h"

#define	ROM_BASE		0x80200000
#define	rom_checksum	ROM_BASE + (2*4)
#define	rom_size		ROM_BASE + (3*4)
#define	version			ROM_BASE + (4*4)

#define	data_start		ROM_BASE + (5*4)
#define	data_addr		ROM_BASE + (6*4)
#define	data_len		ROM_BASE + (7*4)

#define	sdata_addr		ROM_BASE + (8*4)
#define	sdata_len		ROM_BASE + (9*4)

#define	sbss_addr		ROM_BASE + (10*4)
#define	sbss_len		ROM_BASE + (11*4)

#define	bss_addr		ROM_BASE + (12*4)
#define	bss_len			ROM_BASE + (13*4)


extern "C" void main(void);
extern "C" void __static_init(void);

extern "C" void __start(void);
extern "C" void move_reset_stack(void);

extern "C" void exc_utlb_code(void);
extern "C" void exc_norm_code(void);
extern "C" void exception(void);

extern "C" void debugErrEntry(void);
extern "C" void otherException(void);
extern "C" void noUserInt(void);

extern "C" void clear_cache(void);

extern unsigned long 	SystemIntHook;

asm void __start(void)
{
	b		real_start
PASCAL:
	or		zero,zero,zero
		
	or		zero,zero,zero				/* checksum, NOT including the checksum itself */
	or		zero,zero,zero				/* size in bytes */
	or		zero,zero,zero				/* version number */
	
	or		zero,zero,zero				/* address of .data in ROM image */
	or		zero,zero,zero				/* phys address of .data in mem */
	or		zero,zero,zero				/* .data len, in WORDS */
	
	or		zero,zero,zero				/* phys address of .sdata in mem */
	or		zero,zero,zero				/* .sdata len, in WORDS */
	
	or		zero,zero,zero				/* .sbss phys address in mem */
	or		zero,zero,zero				/* .sbss len, in WORDS */

	or		zero,zero,zero				/* .bss phys address in mem */
	or		zero,zero,zero				/* .bss len, in WORDS */
	
real_start:


	//
	// Initialize CPU & Memory controller here
	//

	li		v0,SR_PE
	mtc0	v0,C0_SR			/* clear any parity error */
			
	//
	// CPU/Memory controller setup end
	//
	
								/* System memory is always added in 1MB increments.		*/
	li		t0,0xa0700000		/* 8MB max */
	li		t1,0xdeadc0ed	
sizeLoop:
	lw		t2,0(t0)			/* save it */
	nop
	sw		t1,0(t0)			/* write signature */
	lw		t3,0(t0)			/* read it back */
	nop
	sw		t2,0(t0)			/* put it back */
	beq		t1,t3,ramSized
	nop
	subu	t0,t0,0x00100000
	b		sizeLoop
	nop

ramSized:
	subu	t0,t0,0xa0000000
	addu	t0,t0,0x00100000

	//
	li		t0,0x00200000		/* Fake it for now - always size to 2MB (idt board has more mem) */
	//
	
	sw		t0,kAbsGlobalsBase	/* Stash mem size.  This will trigger a reinit if mem size has changed. */
	
	lui		t1,0x8000
	or		t0,t0,t1
	subu	t0,t0,kResetStackOffset
	move	sp,t0				/* set up the stack under any patches that are present */
		
	//
	// Now copy data section to RAM, initialize bss
	//

								/* copy data to data area */
	lw		t0,data_start
	lw		t1,data_len
	lw		t3,data_addr		/* where data goes at runtime */
copyDataArea:
	lw		t2,0(t0)
	addu	t0,t0,4
	subu	t1,t1,1
	sw		t2,0(t3)
	bne		zero,t1,copyDataArea
	addu	t3,t3,4

	lw		t3,sdata_addr		/* copy sdata to sdata area */
	lw		t1,sdata_len
copySDataArea:
	lw		t2,0(t0)
	addu	t0,t0,4
	subu	t1,t1,1
	sw		t2,0(t3)
	bne		zero,t1,copySDataArea
	addu	t3,t3,4
	
								/* zero sbss area */
	lw		t3,sbss_addr
	lw		t1,sbss_len
zeroSBssArea:
	sw		zero,0(t3)
	subu	t1,t1,1
	bne		zero,t1,zeroSBssArea
	addu	t3,t3,4
	
								/* zero bss area */
	lw		t3,bss_addr
	lw		t1,bss_len
zeroBssArea:
	sw		zero,0(t3)
	subu	t1,t1,1
	bne		zero,t1,zeroBssArea
	addu	t3,t3,4
		
	li		gp,_gp				/* set up the globals pointer */

	la		t0,__static_init	/* call static initializers */
	jalr	ra,t0
	nop
		
	la		t0,main				/* t0 -> our C entry point */
	jalr	zero,t0				/* here we go... (we won't be back!) */
	nop
	
	b		PASCAL
}


asm void move_reset_stack(void)
{
	move	sp,a0
	
	jr		ra
	nop
}

/* tim's hacks */

asm void move_exc_code(void)
{
	la		t1,exc_utlb_code
	la		t2,exc_norm_code
	li		t3,UT_VEC
	li		t4,E_VEC
	li		t5,VEC_CODE_LENGTH
	
temp:

	lw		t6,0(t1)
	lw		t7,0(t2)
	sw		t6,0(t3)
	sw		t7,0(t4)
	addu	t1,t1,4
	addu	t3,t3,4
	addu	t4,t4,4
	subu	t5,t5,4
	bne		t5,zero,temp
	addiu	t2,t2,4
	
	move	t5,ra		
	
	li		a0,UT_VEC
	la		t0,clear_cache
	jalr	ra,t0
	li		a1,VEC_CODE_LENGTH
	nop
	
	li		a0,E_VEC
	la		t0,clear_cache
	jalr	ra,t0
	li		a1,VEC_CODE_LENGTH
	
	move	ra,t5			// restore ra
	jr		ra
	nop
}

/*
** the following sections of code are copied to the vector area
**	at location 0x80000000 (utlb miss) and location 0x80000080
**	(general exception). 
**
*/

asm void exc_norm_code(void)
{
	la		k0,except_regs			// point at exception storage
	sw		AT,R_AT*4(k0)			// save at
	la		AT,exception
	jr		AT						// enter common exc code
	nop
}
	
asm void exc_utlb_code(void)
{
	la		k0,except_regs		// point at exception storage
	sw		AT,R_AT*4(k0)		// save at
	la		AT,exception		
	jr		AT					// enter common exc code
	nop
}

/*
** common exception handling code
**	Reg.(k0) points to the exception register save area.
*/
asm void exception(void)
{
	sw		v0,R_V0*4(k0)		// save v0	
	sw		v1,R_V1*4(k0)		// save v1
	mfc0	v0,C0_EPC
	mfc0	v1,C0_SR
	sw		v0,R_EPC*4(k0)		// save EPC
	sw		v1,R_SR*4(k0)		// save SR

	sw		gp,R_GP*4(k0)		// save gp
	
	mfc0	v0,C0_BADVADDR
	mfc0	v1,C0_CAUSE
	sw		v0,R_BADVADDR*4(k0)	// save BADVADDR
	sw		v1,R_CAUSE*4(k0)	// save CAUSE

	andi	v1,v1,EXCMASK		// was it just an interrupt?
	bne		v1,zero,otherException
	sw		sp,R_SP*4(k0)		// save sp in the delay slot
	lw		v0,SystemIntHook	
	
	sw		a0,R_A0*4(k0)		// save a0
	sw		ra,R_RA*4(k0)		// save ra
	beq		v0,zero,noUserInt
	nop

	jalr	ra,v0		// user routine can use k0,v0,v1,a0,at
	nop					// we might want to switch to an interrupt stack, first
	
noUserInt:
	la		v1,except_regs
	lw		ra,R_RA*4(v1)
	lw		AT,R_AT*4(v1)
	lw		gp,R_GP*4(v1)
	lw		v0,R_V0*4(v1)
	lw		sp,R_SP*4(v1)
	lw		a0,R_A0*4(v1)
	lw		k0,R_EPC*4(v1)
	lw		v1,R_V1*4(v1)
	jr		k0
	rfe

/* all non-ints come here */

otherException:
					/* by now, we've saved at,gp,v0,v1,EPC,SR,BADVADDR,CAUSE, and SP */
					/* k0 -> exception reg save area */
	lui	v0,0xa521
	li	v1,1
	sw	v1,0(v0)	/* LED on to indicate naughty exception */

	sw	a0,R_A0*4(k0)
	sw	ra,R_RA*4(k0)
}

asm void debuggerContext(void)
{	/* the programmer's button wants to save all context and jump */
					/* into the monitor. */
	sw	a1,R_A1*4(k0)
	sw	a2,R_A2*4(k0)
	sw	a3,R_A3*4(k0)

	sw	t0,R_T0*4(k0)
	sw	t1,R_T1*4(k0)
	sw	t2,R_T2*4(k0)
	sw	t3,R_T3*4(k0)
	sw	t4,R_T4*4(k0)
	sw	t5,R_T5*4(k0)
	sw	t6,R_T6*4(k0)
	sw	t7,R_T7*4(k0)

	sw	s0,R_S0*4(k0)
	sw	s1,R_S1*4(k0)
	sw	s2,R_S2*4(k0)
	sw	s3,R_S3*4(k0)
	sw	s4,R_S4*4(k0)
	sw	s5,R_S5*4(k0)
	sw	s6,R_S6*4(k0)
	sw	s7,R_S7*4(k0)

	sw	t8,R_T8*4(k0)
	sw	t9,R_T9*4(k0)

	sw	fp,R_FP*4(k0)

	j	debugErrEntry
	nop				/* leave interrupts OFF so the vbls, etc. won't bother us */
}	

/* 
   Do a full context restore & return.
   Interrupt enables are in whatever state they were in at the time of the context save.
 */
 
asm void restoreContextAndReturn(void)
{
	la	v1,except_regs

	lw	a0,R_A0*4(v1)
	lw	a1,R_A1*4(v1)
	lw	a2,R_A2*4(v1)
	lw	a3,R_A3*4(v1)

	lw	t0,R_T0*4(v1)
	lw	t1,R_T1*4(v1)
	lw	t2,R_T2*4(v1)
	lw	t3,R_T3*4(v1)
	lw	t4,R_T4*4(v1)
	lw	t5,R_T5*4(v1)
	lw	t6,R_T6*4(v1)
	lw	t7,R_T7*4(v1)

	lw	s0,R_S0*4(v1)
	lw	s1,R_S1*4(v1)
	lw	s2,R_S2*4(v1)
	lw	s3,R_S3*4(v1)
	lw	s4,R_S4*4(v1)
	lw	s5,R_S5*4(v1)
	lw	s6,R_S6*4(v1)
	lw	s7,R_S7*4(v1)

	lw	t8,R_T8*4(v1)
	lw	t9,R_T9*4(v1)

	lw	fp,R_FP*4(v1)
	lw	ra,R_RA*4(v1)

	lw	AT,R_AT*4(v1)
	lw	gp,R_GP*4(v1)
	lw	v0,R_V0*4(v1)
	lw	sp,R_SP*4(v1)
	lw	k0,R_EPC*4(v1)
	lw	v1,R_V1*4(v1)
	
	jr	k0
	rfe
}

/*
  enable_cpu_ints(mask) - enable CPU interrupt(s)
*/

asm void enable_cpu_ints(unsigned long mask)
{
	mfc0	t0,C0_SR
	ori		a0,a0,1		/* set IEc */
	or		t0,t0,a0	/* set the int mask passed to us */
	mtc0	t0,C0_SR
	jr		ra
	nop
}


/*
  disable_cpu_ints(mask) - disable CPU interrupt(s)
*/

asm void disable_cpu_ints(unsigned long mask)
{
	mfc0	t0,C0_SR
	nor		a0,zero,a0	/* turn the mask into ~mask */
	and		t0,t0,a0
	mtc0	t0,C0_SR
	jr		ra
	nop
}


