#include "WTVTypes.h"
#include "ErrorNumbers.h"
#include "boxansi.h"
#include "chario.h"
#include "Debug.h"

#include "iregdef.h"
#include "BoxHWEquates.h"
#include "BoxAbsoluteGlobals.h"
#include "BoxBoot.h"
#include "Checksum.h"
#include "HWRegs.h"
#include "BoxUtils.h"
#include "Exceptions.h"
#include "Interrupts.h"

#include "HWKeyboard.h"
#include "HWDisplay.h"
#include "HWModem.h"
#include "HWExpansion.h"
#include "HWAudio.h"
#include "SiliconSerial.h"
#include "IR.h"
#include "Fence.h"
#include "Flash.h"
#include "Caches.h"
#include "CrashLogC.h"

#include "ObjectStore.h"
#include "MemoryManager.h"

#include "BoxDebugger.h"

#ifdef SPOT3_REFRESH_HACK
#include "idtcpu.h"
#endif

/*
 *	GLOBALS
 */
  
ulong	gSystemTicks;
ulong	gVideoShift;

ulong	except_regs[NREGS];		/* registers at time of exception */
ulong	audioIntRegs[NREGS];	/* reg save space for audio interrupt handler */
ulong 	SystemIntHook;			/* MOVE ME */
ulong	SystemExceptionHook;	/* MOVE ME */

ulong 	gBaudCount = 0;			/* used by BangSerial */

extern void LateBoot(void);
extern void ShowBuildInfo(void);




main()
{
	SystemExceptionHook = (ulong)otherException;	/* install default exception handler early */

	DetermineBootType();							/* decides if warm or cold, sets relevant state */

	InitCrashLog();

	InitDisplay(0);		/* do this early, as it wants to set up field buffers above data space */
	
	gBaudCount = (READ_AG(agCPUSpeed) >> 1) / 19200;

#ifndef GOOBER
	InitAudio();		/* do this right after InitDisplay() */
#endif

#ifdef DEBUG
	ShowBuildInfo();
#endif

	Message(("EMAC: this approm is mostly unmodified from the original oldWebTVSrc. Changes were made so it could compile, fix some header issues, fix audio, fix TellyScript result codes, fix video out, add basic PAP support (username: test, password: test)."));

	InitIR();

	InitModem();

	InitSiliconSerialNumber();

	Message(("Initializing interrupts...\n"));

	move_exc_code();

	if(SPOTVersion() == kSPOT3)
	{
		ulong alg = 0x21233333;
		SetCalg(alg);
		Message(("Flushing data cache..."));
		FlushDataCache();
	}

	SystemIntHook = (ulong)InterruptDecode;			/* install main int hook */	
		
	DisableAllInts();
	ClearAllInts();

	SetDisplayInterrupts(kVidHSyncIntMask);		/* enable hline ints at vid unit */

	EnableInts(kVideoIntMask + kKeyboardIntMask + kIRIntMask + kSmartCardIntMask);

#ifdef SPOT3_REFRESH_HACK
	SetSR(0x10010000);		/* turn off BEV, counter interrupts */
#endif

	enable_cpu_ints(kCPUIntMask+kGooberIntMask);	/* enable ints to the cpu */

	InitHWKeyboard();					/* unlike other drivers, call kb init AFTER ints are enabled */

	*(volatile ulong*)kBusErrEnableSetReg = kBusTimeoutIntMask;  /* enable timeouts (buserr) */

	SetFence1Range(0x00000000, 0x000000ff);			/* turn on fence to catch nasty address accesses */
	SetFence2Range(0x00000000, 0x000001ff);
	EnableFences(kFence1ReadIntMask | kFence2WriteIntMask);
	
	WriteEnableFlash(false);			/* write protect flash (requires kBusTimeoutIntMask set) */
	
	LateBoot();							/* crank up the browser */
	
	
	return 0;							/* never get here */
}




void UpdateAG(void)
{
	/*Message(("Updating Absolute Globals Checksum...\n"));*/
}




