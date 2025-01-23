#ifndef __BOXUTILS_H__
#define __BOXUTILS_H__

#include "ObjectStore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Front Panel LEDs */
/* (in FOR_MAC build, Set/GetBoxLEDs() are defined in MacSimulator.c) */

extern volatile ulong gVBLsPerConnectedFlashes;		/* to regularly flash Connected LED */
extern volatile ulong gVBLsElapsed;

void	SetBoxLEDs(ulong bits);
ulong	GetBoxLEDs(void);
void 	FlashBoxLED(uchar led,int nFlashes);

#define	kBoxLEDPower				(1<<2)
#define	kBoxLEDConnect				(1<<1)
#define	kBoxLEDMessage				(1<<0)



/* Hardware Utility Routines */

ulong 	GetRandom(void);
ulong	TimeVBL(void);

void 	EnableWatchdog(void);
void	DisableWatchdog(void);

ulong 	ChecksumMem(uchar *addr,long count);



/* Software-controlled Reset */

#define	kWarmBoot					0x00000069
#define	kColdBoot					0x00000000

void 	Reboot(ulong type);			/* kColdBoot makes it look like a power-on reset */

void 	DetermineBootType(void);	/* called early in boot process to analyze boot type */
Boolean ColdBooted(void);			/* tells you if we lost power or just reset */



/* SPOT Version Detection */

#define kRevMask 0x00F00000

#define	kSPOT1						1
#define	kSPOT2						2		/* not really used */
#define	kSPOT3						3

ulong 	SPOTVersion(void);



/* CPU Coprocessor 0 Accessors */

ulong	FetchConfig(void);

ulong	FetchCalg(void);
void 	SetCalg(ulong);

ulong 	FetchIWatch(void);
void 	SetIWatch(ulong);

ulong 	FetchDWatch(void);
void 	SetDWatch(ulong);

ulong 	FetchCounter(void);
void 	SetCounter(ulong);

void 	SetSR(ulong);



/* Smart Card Software Serial Port */

void BangSerial(uchar c);



/* Symbolics */

typedef struct {
	ulong 	address;
	char	symbol[64];
} Symbol;

Symbol *FindSymbol(ulong addr);
Symbol *FindSymbolByName(char *name);
ulong OpenSymbolFile(void);
extern Symbol *gSymbols;



/* Misc */

extern Boolean gPrintEnable;

void CrashStackCrawl(ulong *sp, ulong depth, ulong *buf);




#ifdef __cplusplus
}
#endif

#endif
