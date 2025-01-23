
#ifndef __BOXHWEQUATES_H__
#define __BOXHWEQUATES_H__


/* ROM Header.  C equivalent is in BoxBoot.h. */

#define 	kROMBase 					0x9fc00000
#define		kROMBaseUncached			0xbfc00000

#define		kAppROMBase					0x9fe00000			/* App ROM base (and entry point) */

#define		kResetBranch				0
#define		kResetNop					kResetBranch 	+ 4
#define		kROMChecksum				kResetNop		+ 4
#define		kROMSize					kROMChecksum	+ 4
#define		kROMCodeSize				kROMSize		+ 4
#define		kROMVersion					kROMCodeSize	+ 4
#define		kROMDataStart				kROMVersion		+ 4
#define		kROMDataLen					kROMDataStart	+ 4
#define		kROMBSSLen					kROMDataLen		+ 4

/* RAM Layout. */

#define		kAbsGlobalsBase				0x80000280			/* Exception handlers are below this. */
#define		kAbsGlobalsTop				0x80000300

#define		kCrashLogBase				0xa0000300			/* Always reference in NON-CACHED space. */
#define		kCrashLogTop				0xa0000700

#define		kDataSegmentBase			0x80000700			/* Data/BSS start.  Heap start above this. */

/* Stack Initialization. */

#define		kStackOffset				0x00000000			/* Currently unused - crt0.s uses this to 
															   offset the initial stack.  Useful if
															   we need to place some chunk of stuff
															   above the stack. */

/* Heap Size.  Heap starts dynamically right after global data/bss. */

#define		kInitSysHeapSize			0x000f0000			/* ~1MB for heap, ~64KB for stack */



#endif /* __BOXHWEQUATES_H__ */