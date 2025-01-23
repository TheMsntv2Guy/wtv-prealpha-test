
#include "WTVTypes.h"
#include "idtcpu.h"
#include "iregdef.h"
#include "excepthdr.h"
#include "BoxDebugger.h"


extern int 	except_regs[80];
extern ulong 	SystemIntHook;		/* MOVE ME */

extern "C" void debugErrEntry(void);

extern "C" void move_exc_code(void);

#if !defined __MWERKS__ || defined USING_METROWERKS_MIPS
extern "C" asm void enable_cpu_ints(void);
extern "C" asm void disable_cpu_ints(void);

asm void exc_norm_code(void);
asm void exc_utlb_code(void);
asm void exception(void);
asm void otherException(void);
asm void restoreContextAndReturn(void);
#elif defined __MWERKS__ && !defined USING_METROWERKS_MIPS
void exc_norm_code(void) {}
void exc_utlb_code(void) {}
void exception(void) {}
void otherException(void) {}
void restoreContextAndReturn(void) {}
#endif

/*
**	move_exc_code() - moves the exception code to the utlb and gen
**			exception vectors
*/

void move_exc_code(void)
{
ulong foobar;
ulong *src;
ulong *dst;
ulong *end;

	src = (ulong *)&exc_norm_code;
	dst = (ulong *)E_VEC;
	end = (ulong *)&exc_utlb_code;
	
	while (src != end)
	{
		*dst = *src;
		src++; dst++;
	}

	src = (ulong *)&exc_utlb_code;
	dst = (ulong *)UT_VEC;
	end = (ulong *)&exception;
	
	while (src != end)
	{
		*dst = *src;
		src++; dst++;
	}

	foobar = 0;

	/* dummy calls to keep code from being stripped */

	if (foobar)
	{	
		exc_norm_code();
		exc_utlb_code();
		exception();
		otherException();
		restoreContextAndReturn();	
	}
}


/*
  enable_cpu_ints(mask) - enable CPU interrupt(s)
*/


#if !defined __MWERKS__ || defined USING_METROWERKS_MIPS
asm void enable_cpu_ints(void)
{	
	mfc0	t0,C0_SR
	ori	a0,a0,1		/* set IEc */
	or	t0,t0,a0	/* set the int mask passed to us */
	mtc0	t0,C0_SR
	jr	ra
	nop	
}


/*
  disable_cpu_ints(mask) - disable CPU interrupt(s)
*/

asm void disable_cpu_ints(void)
{	
	mfc0	t0,C0_SR
	nor	a0,zero,a0	/* turn the mask into ~mask */
	and	t0,t0,a0
	mtc0	t0,C0_SR
	jr	ra
	nop
}



/*
** the following sections of code are copied to the vector area
**	at location 0x80000000 (utlb miss) and location 0x80000080
**	(general exception). 
**
*/

asm void exc_norm_code(void)		/* only uses k0 */
{
	la	k0,except_regs		/* point at exception storage */
	sw	at,R_AT*4(k0)		/* save at */
	la	at,exception
	jr	at			/* enter common exc code */
	nop
}


	
asm void exc_utlb_code(void)
{
	la	k0,except_regs		/* point at exception storage */
	sw	at,R_AT*4(k0)		/* save  at */
	la	at,exception		
	jr	at			/* enter common exc code */
	nop
}


/*
** common exception handling code
**	Reg.(k0) points to the exception register save area.
*/

asm void exception(void)
{
	sw	v0,R_V0*4(k0)		/* save v0 */
	sw	v1,R_V1*4(k0)		/* save v1 */
	mfc0	v0,C0_EPC
	mfc0	v1,C0_SR
	sw	v0,R_EPC*4(k0)		/* save EPC */
	sw	v1,R_SR*4(k0)		/* save SR */

	sw	gp,R_GP*4(k0)		/* save gp */
		
	mfc0	v0,C0_BADVADDR
	mfc0	v1,C0_CAUSE
	sw	v0,R_BADVADDR*4(k0)	/* save BADVADDR */
	sw	v1,R_CAUSE*4(k0)	/* save CAUSE */
	
	andi	v1,v1,EXCMASK		/* was it just an interrupt? */
	bne	v1,zero,otherExcept
	sw	sp,R_SP*4(k0)		/* save sp in the delay slot */

	lw	v0,SystemIntHook	
	
	sw	a0,R_A0*4(k0)		/* save a0 */
	sw	ra,R_RA*4(k0)		/* save ra */
	beq	v0,zero,noUserInt
	nop
	
//	jalr	ra,v0			/* user routine can use k0,v0,v1,a0,at */
	nop				/* we might want to switch to an interrupt stack, first */
	
noUserInt:
	la	v1,except_regs
	lw	ra,R_RA*4(v1)
	lw	AT,R_AT*4(v1)
	lw	gp,R_GP*4(v1)
	lw	v0,R_V0*4(v1)
	lw	sp,R_SP*4(v1)
	lw	a0,R_A0*4(v1)
	lw	k0,R_EPC*4(v1)
	lw	v1,R_V1*4(v1)
	
	jr	k0
	rfe
	
otherExcept:
}


/* all non-ints come here */

asm void otherException(void)		/* by now, we've saved at,gp,v0,v1,EPC,SR,BADVADDR,CAUSE, and SP */
{					/* k0 -> exception reg save area */
	lui	v0,0xa521
	li	v1,1
	sw	v1,0(v0)		/* LED on to indicate naughty exception */

	sw	a0,R_A0*4(k0)
	sw	ra,R_RA*4(k0)
	
					/* fall into rest of context save */


					/* the programmer's button wants to save all context and jump */
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
#endif

