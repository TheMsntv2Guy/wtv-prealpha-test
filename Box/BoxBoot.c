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

#include "Caches.h"
#include "HWTimers.h"
#include "HWKeyboard.h"
#include "HWDisplay.h"
#include "HWModem.h"
#include "HWExpansion.h"
#include "HWAudio.h"
#include "SiliconSerial.h"
#include "IR.h"
#include "Fence.h"
#include "CrashLogC.h"

#include "ObjectStore.h"
#include "MemoryManager.h"

#include "BoxDebugger.h"

#include "FlashDownload.h"
#include "FlashStorage.h"


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


#ifdef __cplusplus
extern "C"  {
#endif

void InitSGRAM(ulong MEM_CNTL, ulong MEM_TIMING, ulong MEM_REFCNT, ulong MEM_CMD_MRS);

#ifdef __cplusplus
}
#endif

extern void LateBoot(void);

extern void ShowBuildInfo(void);

static void ReadCPUConfig(void);
static void GetClockSpeed(void);
static void SetRAMTiming(void);

#ifndef GOOBER
void SetROMTiming(ulong p1, ulong p2,ulong pagemode);
#endif



main()
{	
	SystemExceptionHook = (ulong)otherException;	/* install default exception handler early */

	InitCrashLog();
	
#ifdef GOOBER	
	SetBoxLEDs(GetBoxLEDs() | kBoxLEDPower);		/* turn on Power LED */
#endif

	InitDisplay(0);		/* do this early, as it wants to set up field buffers above data space */

	GetClockSpeed();	/* do this early, as lots of folks depend on these values */

	gBaudCount = (READ_AG(agCPUSpeed) >> 1) / 19200;

#ifdef APPROM
	InitAudio();		/* do this right after InitDisplay() */
#endif

#ifdef DEBUG
	ShowBuildInfo();
	
	Message(("EMAC: this bootrom is mostly unmodified from the original oldWebTVSrc. Changes were made so it could compile, fix some header issues and fix video out."));

	Message(("Calg = %x",FetchCalg()));
    Message(("CPU @ %d.%d MHz, BUS @ %d.%d MHz",
	       READ_AG(agCPUSpeed)/1000000,
	       (READ_AG(agCPUSpeed)%1000000)/1000,
	       READ_AG(agBusSpeed)/1000000,
	       (READ_AG(agBusSpeed)%1000000)/1000 ));

	Message(("This board has a SPOT%d", SPOTVersion()));

	ReadCPUConfig();
#endif

#ifndef GOOBER		
	SetROMTiming(6,12,0);
#endif

	SetRAMTiming();

	ROMCodeChecksumOK(ROMBASE, ROMSIZE);		/* should do something if these fail? */
	ROMFSChecksumOK(ROMBASE, ROMSIZE);

	InitIR();

	InitModem();

	InitSiliconSerialNumber();
	
	CheckSystemIntegrity();


#ifdef GOOBER
	
	Message(("Initializing interrupts..."));

	move_exc_code();
		
	SystemIntHook = (ulong)InterruptDecode;			/* install main int hook */	
		
	DisableAllInts();
	ClearAllInts();
	
	SetDisplayInterrupts(kVidHSyncIntMask);		/* enable hline ints at vid unit */

	EnableInts(kVideoIntMask + kIRIntMask);			

#ifdef SPOT3_REFRESH_HACK
	SetSR(0x10010000);			/* turn off BEV, counter interrupts */
#endif

	enable_cpu_ints(kCPUIntMask+kGooberIntMask);	/* enable ints to the cpu */

	
	InitHWKeyboard();			/* unlike other drivers, call kb init AFTER ints are enabled */

	EnableDisplay();

	InitializeMemoryManagement();			/* Normally done in LateBoot() */
	InitializeFilesystems();

	SetDisplayOverscanColor(0x2fcffd);		/* red border */
	SetDisplayOptions(GetDisplayOptions() | kBlankColorEnable);
										
	printf("\nJumping to debugger...\n");
	BoxDebugger();

#endif

#ifdef BOOTROM

	InitializeMemoryManagement();		/* NVSanityCheck() and Flash Downloader might need this */

	NVSanityCheck();					/* Be sure NV storage is cool */

	DealWithFlashDownload();			/* First, update app rom if needed */

	Message(("Jumping to app ROM..."));
	
	JumpToApp();						/* Boot ROM, jump into App ROM! */
	
#endif

	return 0;							/* never get here */
}



void CheckSystemIntegrity(void)
{	
	/* Initialize the Absolute Globals */
					
	Message(("ROMBASE = %x",ROMBASE));
	Message(("kAbsGlobalsTop = %x",kAbsGlobalsTop));
	Message(("ROMBASE->data_length = %x",((ROMHeader *)ROMBASE)->data_length<<2));
	Message(("ROMBASE->bss_length = %x",((ROMHeader *)ROMBASE)->bss_length<<2));
	Message(("sys heap should start @ %x",READ_AG(agSysHeapBase)));
	
	WRITE_AG(agSysHeapSize, kInitSysHeapSize);

	WRITE_AG(agSysStackBase, (char*)(READ_AG(agRAMPresentSize) + 0x80000000));
	Message(("agSysStackBase = %x",READ_AG(agSysStackBase)));
}



void UpdateAG(void)
{
	/*Message(("Updating Absolute Globals Checksum..."));*/
}





#ifdef BOOTROM

ulong Now(void);

ulong Now(void)
{
	return gSystemTicks;
}

#endif



void GetClockSpeed(void)
{
	ulong mode;
	ulong speed;

	mode = ((FetchConfig()&0x70000000)>>28)+2;	/* find out what mode the CPU is in (2x,3x,etc) */

 	/* 
		To compute CPU pipeline clock speed note that:
			1) counter increments every other cycle 
		 	2) it took two fields to get back to the line number
		therefore:
			speed 	= cycles * (2 clks/cycle) * (2 fields) / (33.3333 ms) * (1000 ms/s)
					= cycles * 120
				
	*/


#ifdef SPOT3_REFRESH_HACK
	disable_cpu_ints(SR_IMASK7);		/* turn off counter interrupts */
#endif


	SetCounter(0);								/* initialize the counter */
	
	speed = TimeVBL()*120;						/* estimate clock speed (Hz) */
	
#ifdef SPOT3_REFRESH_HACK
	enable_cpu_ints(SR_IMASK7);			/* turn on counter interrupts */
#endif


	WRITE_AG(agCPUSpeed, speed);
	WRITE_AG(agBusSpeed, speed/mode);

	WRITE_AG(agCountsPerMicrosec, speed/2000000);
}
	



/* This will become algorithmic */


void SetRAMTiming(void)
{
ulong speed;
ulong MEM_CNTL;
ulong MEM_TIMING;
ulong MEM_REFCNT;
ulong MEM_CMD_MRS;

	speed = READ_AG(agBusSpeed)/1000000;

	if(SPOTVersion() == kSPOT1)
	{
		switch(speed)
		{
			case 31:	/* Hack until CPU/Bus speed calc code is more accurate */
			case 32:
						InitSGRAM(0x80000000, 0x60910812, 0x000001f4, 0x28000080);
						break;
			case 35:
			case 36:
						InitSGRAM(0x80000000, 0x60910813, 0x00000222, 0x28000080);
						break;
			case 37:
						InitSGRAM(0x80000000, 0x60910823, 0x00000242, 0x28000080);
						break;
			case 40:
						InitSGRAM(0x80000000, 0x60910823, 0x00000271, 0x28000080);
						break;
			default:									/* safe, 20MHz values */
						InitSGRAM(0x40000000, 0x40408001, 0x00000138, 0x28000040);
						break;
		}
	}
	else		/* SPOT3 */
	{
/*
		Freq (MHz)      Cas Latency
		----------      -----------
		0-27.7               1
		27.8-55.5            2
		55.6-83.3            3

		
		Freq (MHz)      MEM_TIMING Register
		----------      -------------------
		16.8-23.9           0x40000012
		24.0-28.0           0x60000023
		28.1-33.4           0x60000aa3
		33.5-35.9           0x60000aa4
		36.0-41.7           0x60000ab4
		41.8-47.7           0x60000ab5
		47.8-50.0           0x60000ac5
		50.1-55.5           0x62000ac6
*/

		Message(("Programming mem ctlr for %dMHz",speed));
#if 0
		MEM_REFCNT = 0x5;
#else
		MEM_REFCNT = 0x0;
#endif
		MEM_CMD_MRS = 0x28000080;

		if(speed < 24)
		{
			MEM_CNTL = 0x40000000;
			MEM_TIMING = 0x40000012;
		}
		else
		
		if((speed >= 24) && (speed <= 28))
		{
			MEM_CNTL = 0x40000000;
			MEM_TIMING = 0x60000023;
		}
		else
		
		if((speed > 28) && (speed < 34))
		{
			MEM_CNTL = 0x80000000;
			MEM_TIMING = 0x60000aa3;
		}
		else
		
		if((speed >= 34) && (speed < 36))
		{
			MEM_CNTL = 0x80000000;
			MEM_TIMING = 0x60000aa4;
		}
		else
			
		if((speed >= 36) && (speed < 42))
		{
			MEM_CNTL = 0x80000000;
			MEM_TIMING = 0x60000ab4;
		}
		else
		
		if((speed >= 42) && (speed < 48))
		{
			MEM_CNTL = 0x80000000;
			MEM_TIMING = 0x60000ab5;
		}
		else
		
		if((speed >= 48) && (speed <= 50))
		{
			MEM_CNTL = 0x80000000;
			MEM_TIMING = 0x60000ac5;
		}
		else

	//
	//	default to this if higher but somebody needs to find out the numbers for higher speeds ( like 56 )
	//	
	//	if(speed > 50)
		{
			MEM_CNTL = 0x80000000;
			MEM_TIMING = 0x62000ac6;
		}

		InitSGRAM(MEM_CNTL, MEM_TIMING, MEM_REFCNT, MEM_CMD_MRS);		
	}
}


#ifndef GOOBER		
void SetROMTiming(ulong p1, ulong p2,ulong pagemode)
{
	ulong speed;
	ulong x;
	ulong y;
	
	speed = READ_AG(agBusSpeed)/1000000;
	
	/* 
		magic numbers from Bob :
		x = (p1 * speed) + 2 => fastest access
		y = (p2 * speed) + 2 => page mode emulation
	*/
	
	x = (p1 * speed)/100 + 2;
	y = (p2 * speed)/100 + 2;
	
	if(pagemode)
		speed = 0x80000007 | ((x & 0xf) << 24) | ((y & 0xf) << 16);
	else
		speed = 0x00000007 | ((x & 0xf) << 24) | ((y & 0xf) << 16);

	Message(("Setting ROM timing to %#x\n",speed));
	
	*(volatile ulong*)(kSPOTBase+kROMCntl1) = speed;
}
#endif

/* ===========================================================================
	Stuff that is only used for debugging
*/

		
/* This is ONLY used for diagnostic output */

#ifdef DEBUG

void ReadCPUConfig(void)
{
ulong config;

	config = FetchConfig();
	
	Message(("Config = 0x%x", config));

	Message(("EC: Pipeline clk = CPU clk * %d.", ((config&0x70000000)>>28)+2));
#ifdef VERBOSE	
	{
	switch((config&0x0f000000)>>24)
	{
		case 0:
				words = 1;
				cycles = 1;
				break;
		case 1:
				words = 2;
				cycles = 3;
				break;
		case 2:
				words = 2;
				cycles = 4;
				break;
		case 3:
				words = 2;
				cycles = 4;
				break;
		case 4:
				words = 2;
				cycles = 5;
				break;
		case 5:
				words = 2;
				cycles = 6;
				break;
		case 6:
				words = 2;
				cycles = 6;
				break;
		case 7:
				words = 2;
				cycles = 7;
				break;
		case 8:
				words = 2;
				cycles = 8;
				break;
		default:
				words = 0;
				Message(("### ILLEGAL WRITE-BACK DATA RATE!"));
	}
	
	if(words)
		Message(("EP: Writeback data rate: %d word(s) every %d cycle(s).", words, cycles));

	if((config&0x000c0000)>>18 == 1)
		Message(("EW = 01: 32-bit mode"));
	else
		if(config&0x000c0000 == 0)
			Message(("Eeek!  EW = 00: 64-bit mode!"));
		else
			Message(("Eeek!  EW = %d: INVALID VALUE!",(config&0x000c0000)>>18));

	if(config&0x00008000)
		Message(("BE = 1: Big Endian"));
	else
		Message(("Eeek!  BE = 0: Little Endian!"));

	temp = (config&0x00000E00)>>9;		/* isolate & shift IC */
	Message(("IC = %d, I-Cache is 0x%x (%d) bytes",temp, (1<<(12+temp)), (1<<(12+temp))));

	temp = (config&0x000001c0)>>6;		/* isolate & shift DC */
	Message(("DC = %d, D-Cache is 0x%x (%d) bytes",temp, (1<<(12+temp)), (1<<(12+temp))));

	Message(("IB = %d, DB = %d", (config&0x00000020)>>5, (config&0x00000010)>>4));
	}
#endif /* VERBOSE */

}

#endif /* DEBUG */


