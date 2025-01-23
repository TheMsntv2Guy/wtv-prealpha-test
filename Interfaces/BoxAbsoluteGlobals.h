
#ifndef __BOXABSOLUTEGLOBALS_H__
#define __BOXABSOLUTEGLOBALS_H__

#ifndef __OBJECTSTORE_H__
#include "ObjectStore.h"
#endif

#ifndef __BOXHWEQUATES_H__
#include "BoxHWEquates.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Absolute Globals 
	
	NOTE! The Absolute Globals should only be written to via the WRITE_AG() macro.
		  This will update the agAbsoluteGlobalsChecksum.
		  
		  
	*** ACHTUNG! ***
	If you modify this struct, modify the assembly counterpart in CrashLog.h!
	
*/

typedef struct 
{
	ulong	agRAMPresentSize;		/* set up right after reset */

	ulong	agCPUSpeed;
	ulong	agBusSpeed;
	ulong	agCountsPerMicrosec;	/* CPU C0_COUNT reg runs at 0.5x CPU speed */
	
	ulong	agSerialNumberHi;		/* DevID, Upper 24 bits of 48-bit Serial Number */
	ulong	agSerialNumberLo;		/*        Lower 24 bits of 48-bit Serial Number, CRC */
	
	ulong	agAbsoluteGlobalsChecksum;
	
	char	*agSysHeapBase;
	ulong	agSysHeapSize;
		
	char	*agSysStackBase;
	
	char	agIRMicroVersion;
	char	pad1;
	char	pad2;
	char	pad3;
		
	Filesystem	*agFirstFilesystem;

} AbsoluteGlobals;

#ifdef HARDWARE
#define		ADDR_AG(g)			( &(((AbsoluteGlobals *)((ulong)kAbsGlobalsBase))->g) )
#define		READ_AG(g)			(((AbsoluteGlobals *)((ulong)kAbsGlobalsBase))->g)
#define		WRITE_AG(g,val)		((((AbsoluteGlobals *)((ulong)kAbsGlobalsBase))->g) = val);	\
								UpdateAG();
#else
extern 		AbsoluteGlobals	gSimAbsoluteGlobals;
#define		ADDR_AG(g)			( &(gSimAbsoluteGlobals.g) )
#define		READ_AG(g)			(gSimAbsoluteGlobals.g)
#define		WRITE_AG(g,val)		((gSimAbsoluteGlobals.g) = val);	\
								UpdateAG();
#endif

#ifdef HARDWARE
void  UpdateAG(void);	
#else
#define UpdateAG()
#endif


#ifdef __cplusplus
}
#endif

#endif /* __BOXABSOLUTEGLOBALS_H__ */
