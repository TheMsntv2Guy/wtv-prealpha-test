#include "WTVTypes.h"
#include "Debug.h"
#include "BoxHWEquates.h"
#include "HWRegs.h"
#include "HWDisplay.h"
#include "HWAudio.h"
#include "Interrupts.h"
#include "Exceptions.h"
#include "SystemGlobals.h"
#include "BoxUtils.h"
#include "CrashLogC.h"


volatile ulong gVBLsPerConnectedFlashes = 0;
volatile ulong gVBLsElapsed = 0;


#define FLASH_DELAY 10


void SetBoxLEDs(ulong bits)
{
ulong oldints;

	oldints = DisableInts( kVideoIntMask );		/* critical section since VBL updates led reg */
	*(volatile ulong *)kDevLEDsReg = ~bits;	
	EnableInts( oldints );
}


ulong GetBoxLEDs(void)
{	
	return( ~(*(volatile ulong *)kDevLEDsReg) );
}


void FlashBoxLED(uchar led,int nFlashes)
{
	int		i;
	
	for(i=0;i<nFlashes;i++)
	{
		/* turn on LED */
		SetBoxLEDs(GetBoxLEDs() | led);
		
		/* wait a bit */
		for(i=0;i<10000;i++)
			*(volatile ulong*)kDevLEDsReg;
			
		/* turn off LED */
		SetBoxLEDs(GetBoxLEDs() & ~led);
		
		/* wait a bit */
		for(i=0;i<10000;i++)
			*(volatile ulong*)kDevLEDsReg;
	}
}


ulong SPOTVersion(void)
{
	// EMAC: always return kSPOT3 to fix screen
	return kSPOT3;
ulong chipid;

	chipid = *(volatile ulong *)(kSPOTBase + kBusChipID);
	
	switch((chipid & kRevMask) >> 20)
	{
		case 0:
			return kSPOT1;
		case 3:
			return kSPOT3;
		default:
			return 0;		// the unknown spot
	}
}

ulong  gNumSymbols = 0;
Symbol *gSymbols = 0;

ulong OpenSymbolFile(void)
{
	/* look for magic cookie 'timn' */
	if(*(volatile ulong*)0x9ff00000 != 0x74696d6e) {
		printf("OpenSymbolFile : symbol file not found\n");
		return 0;
	}
	
	/* get number of symbols and the data */
	gNumSymbols = *(ulong*)0x9ff00004;
	gSymbols = (Symbol *)0x9ff00008;
	
	return 1;
}

Symbol *FindSymbol(ulong addr)
{
	int mid = 0;
	int low = 0;
	int high = 0;
	
	high = gNumSymbols - 1;
	
	while(low <= high)
	{
		mid = (low+high)/2;
		if(gSymbols[mid].address > addr)
			high = mid - 1;
		else if(gSymbols[mid].address < addr)
			low = mid + 1;
		else
			break;
	}
	
	if((long)(addr - gSymbols[mid].address) < 0)
		mid--;
	
	return &gSymbols[mid];
}

Symbol *FindSymbolByName(char *name)
{
	ulong i;
	
	for(i=0;i<gNumSymbols;i++) {
		if(strcmp(gSymbols[i].symbol,name) == 0)
			return &gSymbols[i];
	}
	return 0;
}



void Reboot(ulong type)
{
	KillDisplay();						/* black screen */

#ifdef APPROM							/* bootrom has no audio code */
	if(SPOTVersion() != kSPOT1)			/* don't gronk when we reboot */
		StopAudioDMA();
#endif

	disable_cpu_ints(1);				/* clear IEc, stops bugs dead */
	
	*(vulong *)kModemSCR = type;		/* modem scratch reg holds warmboot flag (decays FAST) */
	
	EnableWatchdog();
	Message(("Watchdog enabled, rebooting..."));
	
	while(1)							/* wait for armageddon */ 
		; 
}


static Boolean coldBoot = true;

void DetermineBootType(void)			/* call early to set up */
{
	if(*(vulong *)kModemSCR == kWarmBoot)
		coldBoot = false;
	else
		coldBoot = true;

	*(vulong *)kModemSCR = kWarmBoot;	/* set warm flag */
}


Boolean ColdBooted(void)
{
	return coldBoot;
}



#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong ChecksumMem(uchar *addr,long count)
{
	ulong 	sum = 0;
	uchar 	*p;
	int		i;
	
	p = addr;
	for(i=0;i<count;i++)
		sum += *p++;
	return sum;
}
#endif




/* 	Will be called at an unpleasant time (we just crashed).
	Only called if sp looks sane.
	Only use locals.
*/

void CrashStackCrawl(ulong *sp, ulong depth, ulong *buf)
{
ulong *addr;
ulong op;
int coolAddr = 0;
			
	while( depth && ((ulong)sp > 0x80000000) )
	{
		if( (*sp & 0x9fc00003) == 0x9fc00000 )
		{
			/* 
				it looks like a ROM address, check if it's a candidate
				for being a return address.
				we'll look two instructions back, and if it's JAL then
				we'll assume it's what we'ere looking at is a return
				address. 
			 */
			 
			addr = (ulong*)*sp;
			addr -= 2;
			op = (*addr & 0xfc000000)  >> 26;
			
			switch(op)
			{
				case 0x0:
					op = *addr & 0x0000003f;				/* check if JALR */
					if(op == 0x09)
						coolAddr = 1;
				case 0x1:
					op = (*addr & 0x001f0000) >> 16;		/* check if BLTZAL or BGEZAL */
					if( (op == 0x10) || (op == 0x11) )
						coolAddr = 1;
				case 0x3: 
					coolAddr = 1;							/* JAL */
					break;
				default:
					coolAddr = 0;
					break;
			}
			
			if(coolAddr)
			{
				depth--;					/* found one */
				*buf++ = (ulong)sp;			/* stash sp addr */
				*buf++ = *sp;				/* and ra */
			}
			
		}
		
		sp++;		/* climb stack */
	}
}


