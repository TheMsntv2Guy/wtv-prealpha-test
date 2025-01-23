#include "iregdef.h"
#include "idtcpu.h"
#include "excepthdr.h"
#include "frame.h"
#include "HWRegs.h"
#include "CrashLog.h"
#include "BoxHWEquates.h"


FRAME(InitCrashLog,sp,0,ra)	
	.set 	noreorder

	li		a0,kCrashLogBase
	
	lw		t0,crashSig(a0)
	li		t1,kValidCrashLogSig
	beq		t0,t1,logIsValid
	
	sw		zero,printfBufTail			/* init fields that other folks depend on */
	
logIsValid:
	jr		ra
	nop

ENDFRAME(InitCrashLog)




/* 	By the time we get here, we've saved at,v0,v1,a0,sp,ra in the system register save
	area.  
	
	We've only trashed AT, v0, and v1, so we ONLY copy those from the normal sys reg save
	area.
	
	k0 -> system reg save area
	k1 is UNUSED, so here it -> crashlog reg save area
	(Well, the VID_HACK uses k1, but by this time, we don't care.  A reboot is imminent.)
*/

FRAME(LogCrash,sp,0,ra)	
	.set 	noreorder
	
	/* 	#########################
		Crash Log CPU Reg Save
	*/

	li		k1,kCrashLogBase
	
	li		v0,kValidCrashLogSig
	sw		v0,crashSig(k1)		/* indicate that a valid crashlog is present */
	
	li		v0,kCrashLogVersion
	sw		v0,crashVersion(k1)		/* set version */
	
	addiu	k1,cpuRegs
	
	lw		v0,R_AT*4(k0)		/* move at to crashlog */
	lw		v1,R_V0*4(k0)		/* move v0 to crashlog */
	sw		v0,R_AT*4(k1)
	sw		v1,R_V0*4(k1)
	lw		v0,R_V1*4(k0)		/* move v1 to crashlog */
	sw		sp,R_SP*4(k1)
	sw		v0,R_V1*4(k1)
	
	sw		a0,R_A0*4(k1)
	sw		ra,R_RA*4(k1)
	
	mfc0	v0,C0_EPC
	mfc0	v1,C0_SR
	sw		v0,R_EPC*4(k1)		# save EPC
	sw		v1,R_SR*4(k1)		# save SR
		
	mfc0	v0,C0_BADVADDR
	mfc0	v1,C0_CAUSE
	sw		v0,R_BADVADDR*4(k1)	# read-only, just here for debugger
	sw		v1,R_CAUSE*4(k1)	# save CAUSE

	mfc0	v0,C0_IBASE
	mfc0	v1,C0_IBOUND
	sw		v0,R_IBASE*4(k1)
	sw		v1,R_IBOUND*4(k1)
	
	mfc0	v0,C0_DBASE
	mfc0	v1,C0_DBOUND
	sw		v0,R_DBASE*4(k1)
	sw		v1,R_DBOUND*4(k1)
	
	mfc0	v0,C0_COUNT
	mfc0	v1,C0_COMPARE
	sw		v0,R_COUNT*4(k1)	# read-only, just here for debugger
	sw		v1,R_COMPARE*4(k1)
	
	mfc0	v0,C0_PRID			# read-only, just here for debugger
	mfc0	v1,C0_CONFIG		# read-only, just here for debugger
	sw		v0,R_PRID*4(k1)
	sw		v1,R_CONFIG*4(k1)
	
	mfc0	v0,C0_CALG
	mfc0	v1,C0_ECC
	sw		v0,R_CALG*4(k1)
	sw		v1,R_ECC*4(k1)
	
	mfc0	v0,C0_IWATCH
	mfc0	v1,C0_DWATCH
	sw		v0,R_IWATCH*4(k1)
	sw		v1,R_DWATCH*4(k1)
	
	mfc0	v0,C0_CACHEERR
	mfc0	v1,C0_TAGLO
	sw		v0,R_CACHEERR*4(k1)	# read-only, just here for debugger
	sw		v1,R_TAGLO*4(k1)
	
	mfc0	v0,C0_ERROREPC
	sw		gp,R_GP*4(k1)		# save gp
	sw		v0,R_ERROREPC*4(k1)

	sw		a1,R_A1*4(k1)
	sw		a2,R_A2*4(k1)
	sw		a3,R_A3*4(k1)

	sw		t0,R_T0*4(k1)
	sw		t1,R_T1*4(k1)
	sw		t2,R_T2*4(k1)
	sw		t3,R_T3*4(k1)
	sw		t4,R_T4*4(k1)
	sw		t5,R_T5*4(k1)
	sw		t6,R_T6*4(k1)
	sw		t7,R_T7*4(k1)

	sw		s0,R_S0*4(k1)
	sw		s1,R_S1*4(k1)
	sw		s2,R_S2*4(k1)
	sw		s3,R_S3*4(k1)
	sw		s4,R_S4*4(k1)
	sw		s5,R_S5*4(k1)
	sw		s6,R_S6*4(k1)
	sw		s7,R_S7*4(k1)

	sw		t8,R_T8*4(k1)
	sw		t9,R_T9*4(k1)

	sw		fp,R_FP*4(k1)
	
	/* 	#########################
		Crash Log Misc Save
	*/

	li		k1,kCrashLogBase		/* k1 -> crash log */
	li		k0,kAbsGlobalsBase		/* k0 -> abs globs */
	lui		a0,kSPOTBaseHi			/* a0 -> SPOT */

	li		v0,0x00200000			/* FAKE IT FOR NOW - need to stash real RAM size! */
	sw		v0,physicalRAM(k1)
	
	lw		v0,agCPUSpeed(k0)
	lw		v1,agBusSpeed(k0)
	sw		v0,cpuSpeed(k1)
	sw		v1,busSpeed(k1)
	
	lw		v0,kROMBaseUncached+kROMVersion
	lw		v1,kBusChipID(a0)
	sw		v0,romVersion(k1)
	sw		v1,spotVersion(k1)
	
	lw		v0,kROMSysConfig(a0)
	lw		v1,kBusIntEnableSet(a0)
	sw		v0,sysConfig(k1)
	sw		v1,intEnable(k1)
	
	lw		v0,kBusIntStatus(a0)
	lw		v1,kBusErrEnableSet(a0)
	sw		v0,intStatus(k1)
	sw		v1,errEnable(k1)
	
	lw		v0,kBusErrStatus(a0)
	lw		v1,kBusErrAddress(a0)
	sw		v0,errStatus(k1)
	sw		v1,errAddress(k1)


	/* 	#########################
		Crash Log Last Requested URL Save
	*/

	/* 	This is done by ContentView::ShowResource() whenever the URL changes.
		gLastRequestedURL always points to the crashlog lastReqURL field. 
		The last byte is always 0 to protect folks who access it as a string. */	
		

	/* 	#########################
		Crash Log TOS Save
		
		NOTE: Sanity check will have to change once we have RAM expansion modules.
	*/

	lui		t0,0xffff
	and		t0,t0,sp			/* check sanity of SP */
	
	lui		t1,0x801f			/* should be in upper meg of RAM */
	
	bne		t0,t1,BadSP
	
	move	a0,sp
	li		v0,kSaveStackWords
	
	move	a1,k1
	addiu	a1,a1,stackSave
	
tosLoop:
	lw		v1,0(a0)
	addiu	a0,a0,4
	sw		v1,0(a1)
	subu	v0,v0,1
	bne		v0,zero,tosLoop
	addiu	a1,a1,4
	

	/* 	#########################
		Crash Log Stack Crawl Save
		
		SP sanity has been verified by this point.
	*/
	
	move	a0,sp
	li		a1,kStackCrawlDepth
	move	a2,k1
	addiu	a2,a2,stackCrawl
	
	jal		CrashStackCrawl
	nop
	
BadSP:

#ifdef EXTERNAL_BUILD
	lui		t0,0xffff
	and		t0,t0,sp			/* check sanity of SP */
	
	lui		t1,0x801f			/* should be in upper meg of RAM */
	
	bne		t0,t1,dontJerkSP
	
	li		t0,0xdbba0			/* point SP into offscreen fb */
	move	sp,t0
	
dontJerkSP:
#endif

	/* 	#########################
		Reboot!
	*/
#ifdef EXTERNAL_BUILD
	j	EnableWatchdog
#else
	j	debugErrEntry
#endif
	nop
	
ENDFRAME(LogCrash)

