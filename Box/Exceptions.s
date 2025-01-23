#include "iregdef.h"
#include "idtcpu.h"
#include "excepthdr.h"
#include "frame.h"
#include "HWRegs.h"



/*
  move_exc_code() - Copy exception handlers to RAM
*/

FRAME(move_exc_code,sp,0,ra)
	.set	noreorder

	move	t5,ra					/* assumes ClearCache doesnt use t5 */

	la	a0,GeneralException			/* GENERAL EXCEPTION HANDLER */
	li	a1,E_VEC
	li	t0, 0x34	/* copy the whole general exception handler */

copyGenExc:
	lw	t1,0(a0)
	addiu	a0,a0,4
	sw	t1,0(a1)
	
	subu	t0,t0,4
	bne	t0,zero,copyGenExc
	addiu	a1,a1,4
		
	li	a0,E_VEC
	jal	ClearCache
	li	a1, 0x34
	


	la	a0,InterruptException		/* INTERRUPT EXCEPTION HANDLER */
	li	a1,I_VEC
	li	t0, 0x60	/* copy the whole interrupt exception handler */

copyIntExc:
	lw	t1,0(a0)
	addiu	a0,a0,4
	sw	t1,0(a1)
	
	subu	t0,t0,4
	bne	t0,zero,copyIntExc
	addiu	a1,a1,4
		
	li	a0,I_VEC
	jal	ClearCache
	li	a1, 0x60
	
	jal	FlushDataCache
	
	mfc0	t0,C0_CAUSE
	li	t1,CAUSE_IV
	or	t0,t0,t1			/* set the IV bit so interrupts will vector thru 0x200 */
	mtc0	t0,C0_CAUSE
	
	move	ra,t5				/* restore ra */
	j	ra
	nop

	.set	reorder
ENDFRAME(move_exc_code)



/*
  enable_cpu_ints(mask) - enable CPU interrupt(s)
*/

FRAME(enable_cpu_ints,sp,0,ra)
	.set	noreorder
	
	mfc0	t0,C0_SR
	ori	a0,a0,1		/* set IEc */
	or	t0,t0,a0	/* set the int mask passed to us */
	mtc0	t0,C0_SR
	j	ra
	nop
	
	.set	reorder
ENDFRAME(enable_cpu_ints)



/*
  disable_cpu_ints(mask) - disable CPU interrupt(s)
*/

FRAME(disable_cpu_ints,sp,0,ra)
	.set	noreorder
	
	mfc0	t0,C0_SR
	nor	a0,zero,a0	/* turn the mask into ~mask */
	and	t0,t0,a0
	mtc0	t0,C0_SR
	j	ra
	nop
	
	.set	reorder
ENDFRAME(disable_cpu_ints)



/*
   Interrupt handler.
   Saves some CPU state, then revectors through SystemIntHook.  
   Expansion devices use this to hook themselves into the system interrupt chain.

   ��� OPTIMIZATION -- init k0 at startup & always assume it's cool.
 */
 
	.set	noreorder
	.set	noat			# keep assembler from complaining


FRAME(InterruptException,sp,0,ra)

INT_X_START:

	la	k0,except_regs		# point at exception storage

	sw	AT,R_AT*4(k0)		# save at
	sw	v0,R_V0*4(k0)		# save v0	
	sw	v1,R_V1*4(k0)		# save v1			
	sw	sp,R_SP*4(k0)		# save sp
	
	lw	v0,SystemIntHook	# fetch vector (No error checking - it had BETTER be valid!)	
	sw	ra,R_RA*4(k0)		# save ra in delay slot
	
	jal	v0					# user routine can use at,v0,v1,a0,sp,ra
	sw	a0,R_A0*4(k0)		# save a0 in delay slot

	lw	ra,R_RA*4(k0)
	lw	a0,R_A0*4(k0)
	lw	sp,R_SP*4(k0)
	lw	v1,R_V1*4(k0)
	lw	v0,R_V0*4(k0)
	lw	AT,R_AT*4(k0)
	nop	

	eret	
	nop						# this never gets executed
	
INT_X_END:
	.set	at
	
ENDFRAME(InterruptException)



/*
   General Xception handling code.
   k0 points to the exception register save area.
   
   ��� OPTIMIZATION -- init k0 at startup & always assume it's cool.
*/

	.set	noreorder
	.set	noat			# keep assembler from complaining

FRAME(GeneralException,sp,0,ra)		
GEN_X_START:
	la	k0,except_regs		# point at exception storage

	sw	AT,R_AT*4(k0)		# save at
	sw	v0,R_V0*4(k0)		# save v0	
	sw	v1,R_V1*4(k0)		# save v1			
	sw	sp,R_SP*4(k0)		# save sp
	
	lw	v0,SystemExceptionHook	# fetch vector (No error checking - it had BETTER be valid!)	
	sw	ra,R_RA*4(k0)		# save ra in delay slot
	
	jr	v0					# at entry to exception handler: at,v0,v1,a0,sp,ra have been saved
	sw	a0,R_A0*4(k0)		# save a0 in delay slot	
	
GEN_X_END:
	.set	at
	
ENDFRAME(GeneralException)



/* all non-ints come here */

FRAME(otherException,sp,0,ra)	/* by now, we've saved at,v0,v1,a0,sp,ra */
								/* k0 -> exception reg save area */
	
	nop				/* ����For debug builds, drop into debugger.  For real boxes in the field, */
	nop				/* ��� log the exception info */
	
ENDFRAME(otherException)		/* fall into rest of context save */


FRAME(debuggerContext,sp,0,ra)	/* the programmer's button wants to save all context and jump */
								/* into the monitor. */
	mfc0	v0,C0_EPC
	mfc0	v1,C0_SR
	sw	v0,R_EPC*4(k0)		# save EPC
	sw	v1,R_SR*4(k0)		# save SR
		
	mfc0	v0,C0_BADVADDR
	mfc0	v1,C0_CAUSE
	sw	v0,R_BADVADDR*4(k0)	# read-only, just here for debugger
	sw	v1,R_CAUSE*4(k0)	# save CAUSE

	mfc0	v0,C0_IBASE
	mfc0	v1,C0_IBOUND
	sw	v0,R_IBASE*4(k0)
	sw	v1,R_IBOUND*4(k0)
	
	mfc0	v0,C0_DBASE
	mfc0	v1,C0_DBOUND
	sw	v0,R_DBASE*4(k0)
	sw	v1,R_DBOUND*4(k0)
	
	mfc0	v0,C0_COUNT
	mfc0	v1,C0_COMPARE
	sw	v0,R_COUNT*4(k0)	# read-only, just here for debugger
	sw	v1,R_COMPARE*4(k0)
	
	mfc0	v0,C0_PRID			# read-only, just here for debugger
	mfc0	v1,C0_CONFIG		# read-only, just here for debugger
	sw	v0,R_PRID*4(k0)
	sw	v1,R_CONFIG*4(k0)
	
	mfc0	v0,C0_CALG
	mfc0	v1,C0_ECC
	sw	v0,R_CALG*4(k0)
	sw	v1,R_ECC*4(k0)
	
	mfc0	v0,C0_IWATCH
	mfc0	v1,C0_DWATCH
	sw	v0,R_IWATCH*4(k0)
	sw	v1,R_DWATCH*4(k0)
	
	mfc0	v0,C0_CACHEERR
	mfc0	v1,C0_TAGLO
	sw	v0,R_CACHEERR*4(k0)	# read-only, just here for debugger
	sw	v1,R_TAGLO*4(k0)
	
	mfc0	v0,C0_ERROREPC
	sw	gp,R_GP*4(k0)		# save gp
	sw	v0,R_ERROREPC*4(k0)

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

	sw	k0,R_K0*4(k0)
	sw	k1,R_K1*4(k0)

	sw	fp,R_FP*4(k0)

#if 1
	j	LogCrash
#else
	j	debugErrEntry
#endif
	nop					/* leave interrupts OFF so the vbls, etc. won't bother us */

	.set	at
	
ENDFRAME(debuggerContext)
	

/* 
   Do a full context restore & return.
   Interrupt enables are in whatever state they were in at the time of the context save.
 */
 
FRAME(restoreContextAndReturn,sp,0,ra)
	la	k0,except_regs

	.set	noat

	lw	a0,R_A0*4(k0)
	lw	a1,R_A1*4(k0)
	lw	a2,R_A2*4(k0)
	lw	a3,R_A3*4(k0)

	lw	t0,R_T0*4(k0)
	lw	t1,R_T1*4(k0)
	lw	t2,R_T2*4(k0)
	lw	t3,R_T3*4(k0)
	lw	t4,R_T4*4(k0)
	lw	t5,R_T5*4(k0)
	lw	t6,R_T6*4(k0)
	lw	t7,R_T7*4(k0)

	lw	s0,R_S0*4(k0)
	lw	s1,R_S1*4(k0)
	lw	s2,R_S2*4(k0)
	lw	s3,R_S3*4(k0)
	lw	s4,R_S4*4(k0)
	lw	s5,R_S5*4(k0)
	lw	s6,R_S6*4(k0)
	lw	s7,R_S7*4(k0)

	lw	t8,R_T8*4(k0)
	lw	t9,R_T9*4(k0)

	lw	fp,R_FP*4(k0)
	lw	ra,R_RA*4(k0)

	lw	AT,R_AT*4(k0)
	lw	gp,R_GP*4(k0)
	lw	sp,R_SP*4(k0)

	lw	v0,R_EPC*4(k0)
	lw	v1,R_SR*4(k0)
	mtc0	v0,C0_EPC
	mtc0	v1,C0_SR
	
	lw	v0,R_CAUSE*4(k0)
	lw	v1,R_ECC*4(k0)
	mtc0	v0,C0_CAUSE
	mtc0	v1,C0_ECC
		
	lw	v0,R_IBASE*4(k0)
	lw	v1,R_IBOUND*4(k0)
	mtc0	v0,C0_IBASE
	mtc0	v1,C0_IBOUND
	
	lw	v0,R_DBASE*4(k0)
	lw	v1,R_DBOUND*4(k0)
	mtc0	v0,C0_DBASE
	mtc0	v1,C0_DBOUND
	
	lw	v0,R_COMPARE*4(k0)
	lw	v1,R_CALG*4(k0)
	mtc0	v0,C0_COMPARE
	mtc0	v1,C0_CALG
	
	lw	v0,R_IWATCH*4(k0)
	lw	v1,R_DWATCH*4(k0)
	mtc0	v0,C0_IWATCH
	mtc0	v1,C0_DWATCH
	
	lw	v0,R_TAGLO*4(k0)
	lw	v1,R_ERROREPC*4(k0)
	mtc0	v0,C0_TAGLO
	mtc0	v1,C0_ERROREPC
	
	
	lw	v0,R_V0*4(k0)
	lw	v1,R_V1*4(k0)

	lw	k1,R_K1*4(k0)
	lw	k0,R_K0*4(k0)		/* restore this one last, for obvious reason... */

	eret	
	nop
	
	.set	at

ENDFRAME(restoreContextAndReturn)



