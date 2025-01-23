#ifndef __CRASHLOG_C__
#define	__CRASHLOG_C__

#ifndef __IREGDEF_H__
#include "iregdef.h"
#endif

#ifndef __BOXHWEQUATES_H__
#include "BoxHWEquates.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


void InitCrashLog(void);


/*
	BE SURE TO UPDATE struct in assembly header file (CrashLog.h), too!
*/


#define	kSaveStackWords		32

#define	kSCWordSize			2
#define	kStackCrawlDepth	8

#define	kPrintfBufSize		256
#define	kPrintfBufMask		0xff

#define	kLastReqURLBufSize	256

#define	kCrashLogVersion	1

#define	kValidCrashLogSig	0x43727368

typedef struct 
{
	ulong	crashSig;
	ulong	crashVersion;
	ulong	cpuRegs[NREGS];
	ulong	physicalRAM;
	ulong	cpuSpeed;
	ulong	busSpeed;
	ulong	romVersion;
	ulong	stackSave[kSaveStackWords];
	ulong	spotVersion;
	ulong	sysConfig;
	ulong	intEnable;
	ulong	intStatus;
	ulong	errEnable;
	ulong	errStatus;
	ulong	errAddress;
	ulong	stackCrawl[kSCWordSize*kStackCrawlDepth];
	char	printfBuf[kPrintfBufSize];
	ulong	printfBufTail;
	ulong	watchDogSet;
	char	lastReqURL[kLastReqURLBufSize];
	
} CrashLogStruct;


#ifdef __cplusplus
}
#endif


#endif /* __CRASHLOG_C__ */
