#ifndef __CRASHLOG__
#define	__CRASHLOG__


/* CRASH LOG 

	BE SURE TO UPDATE struct in C header file (CrashLogC.h), too!

	If crashSig == kValidCrashLogSig, there is a valid crash log present.

	All absolute system struct base addresses (like for the crash log) are
	defined in BoxHWEquates.h.
*/

#define	kSaveStackWords		32

#define	kSCWordSize			2
#define	kStackCrawlDepth	8

#define	kPrintfBufSize		256
#define	kPrintfBufMask		0xff

#define	kLastReqURLBufSize	256

#define	kCrashLogVersion	1

#define	kValidCrashLogSig	0x43727368

#define	crashSig			0
#define	crashVersion		crashSig		+ 4
#define	cpuRegs				crashVersion	+ 4
#define	physicalRAM			cpuRegs			+ (4*NREGS)
#define	cpuSpeed			physicalRAM		+ 4
#define	busSpeed			cpuSpeed		+ 4
#define	romVersion			busSpeed		+ 4
#define	stackSave			romVersion		+ 4
#define	spotVersion			stackSave		+ (4*kSaveStackWords)
#define	sysConfig			spotVersion		+ 4
#define	intEnable			sysConfig		+ 4
#define	intStatus			intEnable		+ 4
#define	errEnable			intStatus		+ 4
#define	errStatus			errEnable		+ 4
#define	errAddress			errStatus		+ 4
#define	stackCrawl			errAddress		+ 4
#define	printfBuf			stackCrawl 		+ (4*kSCWordSize*kStackCrawlDepth)
#define	printfBufTail		printfBuf 		+ kPrintfBufSize	
#define	watchDogSet			printfBufTail	+ 4
#define	lastReqURL			watchDogSet		+ 4

#define	crashLogSize		lastReqURL		+ kLastReqURLBufSize



/* ABSOLUTE GLOBALS
								>>> HEY YOU!!! <<<

	If you modify this struct, modify the C counterpart in BoxAbsolateGlobals.h!
*/

#define	agRAMPresentSize			0

#define	agCPUSpeed					agRAMPresentSize 		+ 4
#define	agBusSpeed					agCPUSpeed				+ 4				
#define	agCountsPerMicrosec			agBusSpeed				+ 4
	
#define	agSerialNumberHi			agCountsPerMicrosec		+ 4
#define	agSerialNumberLo			agSerialNumberHi		+ 4
	
#define	agAbsoluteGlobalsChecksum	agSerialNumberLo		+ 4
	
#define	agSysHeapBase				agAbsoluteGlobalsChecksum + 4
#define	agSysHeapSize				agSysHeapBase			+ 4
		
#define	agSysStackBase				agSysHeapSize			+ 4
	
#define	agIRMicroVersion			agSysStackBase			+ 4
#define	pad1						agIRMicroVersion		+ 1
#define	pad2						pad1					+ 1
#define	pad3						pad2					+ 1
		
#define	agFirstFilesystem			pad3					+ 1




#endif /* __CRASHLOG__ */
