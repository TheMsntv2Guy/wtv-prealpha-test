#include "WTVTypes.h"
#include "Debug.h"
#include "BoxUtils.h"
#include "MemoryManager.h"
#include "Flash.h"
#include "FlashStorage.h"
#include "Fence.h"
#include "HWRegs.h"
#include "HWDisplay.h"
#include "Log.h"



/* 	*******************
	Simulator-Only Code
	******************* */

#ifdef SIMULATOR

#ifdef FOR_MAC
	#ifndef __MACINTOSHUTILITIES_H__
	#include "MacintoshUtilities.h"
	#endif
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
	#ifndef __SIMULATOR_RSRC_H__
	#include "Simulator.rsrc.h"
	#endif
#endif /* FOR_MAC */

void* kNVStorageBase = nil;
void* kNVFlashBase = nil;

#define kPrefsNVContents "Non-volatile contents"

static void NVInit_Simulator(void);

static void
NVInit_Simulator(void)
{
	/* allocate make-believe buffer in simulator heap so that */
	/* kNVFlashBase and kNVStorageBase have something to point to */

	if(kNVFlashBase != nil)
		return;
		
	kNVFlashBase = (void*)NewSimulatorMemory(kNVBufSize);
	if (IsError(kNVFlashBase == nil)) {
		#ifdef FOR_MAC
			EmergencyExit(sLowMemory);
		#else /* of #ifdef FOR_MAC */
			#error "If not 'FOR_MAC', then what's the platform-savvy way to exit?"
		#endif /* of #else of #ifdef FOR_MAC */
	}
	kNVStorageBase = (void*)((ulong)kNVFlashBase + kNVBufSize - kNVSize);
	memset(kNVFlashBase, 0, kNVBufSize);

	if (gSimulator->GetUseNVStore ()) {
		/* restore contents from simulator prefs file */
		gSimulator->OpenPreferences();
		void* savedNVContents = nil;
		size_t savedNVLength = 0;
		if (gSimulator->GetPreference(kPrefsNVContents, &savedNVContents, &savedNVLength)) {
			if (savedNVLength != kNVSize) {
				gSimulator->RemovePreference(kPrefsNVContents);
				Message(("NVContents in Prefs file is %d bytes, but now NVContents are %d bytes, "
						 "so ignoring Prefs file's NVContents", savedNVLength, kNVSize));
			} else {
				memcpy(kNVStorageBase, savedNVContents, savedNVLength);
				Message(("Restored NVContents from Prefs file"));
			}
		}
		gSimulator->ClosePreferences();
	}
}

#endif /* SIMULATOR */




/* 	********************
	Box & Simulator Code
	******************** */

static ulong 	*gNVBuffer = nil;	/* buffer for flash we need to save + nv storage */
static NVNode 	*gNext = nil;		/* place where next node goes */
static NVHeader	*gNVHeader = nil;	/* start of NV storage */
static NVNode 	*gFirstNode = nil;	/* first node in chain */

static Boolean	usingFBRAM;

#define NEXT(n) (NVNode*)( ( (ulong)&n->data + n->len + kNVFlashAlignMask ) & ~kNVFlashAlignMask )




/* 	************************************
	Non-Volatile Initialization/Flashing 
	************************************ */

/* Validate checksum, zero NV storage if mismatch.  Called early in boot. */

#if defined BOOTROM || defined SIMULATOR
Boolean NVSanityCheck(void)
{

#ifdef SIMULATOR
	if (kNVStorageBase == nil) 
		NVInit_Simulator();
#endif

	NVHeader *h = (NVHeader*)kNVStorageBase;

	Message(("NV Sanity Check..."));
		
	if( h->checksum != NVChecksum((ulong*)kNVStorageBase) ) 
	{
		Message(("NV Checksum failed.  Zorching NV storage."));
		
		NVInit(kNVPhaseSave);				/* alloc buf, zero */		
		NVCommit();							/* update checksum, write, free buf */
				
		return false;						/* say we had a problem */
	}
	else
	{
		Message(("NV Checksum OK!"));
		return true;						/* say everything was OK */
	}
}
#endif




/* Sets up the globals: gNVHeader
						gFirstNode
*/

void NVInit(NVPhase phase)
{

#ifdef SIMULATOR
	NVInit_Simulator();
#endif /* SIMULATOR */

	switch(phase)
	{
		case kNVPhaseSaveFB:
#ifdef HARDWARE
			usingFBRAM = true;
			
			gNVBuffer = (ulong*)GetDisplayPageBase(1);				/* use the FB to hold the data while we bang on it */
			gNVHeader = (NVHeader*)( (ulong)gNVBuffer + kNVBufSize - kNVSize );		/* point to start of NVStorage */
			
			memcpy((void*)gNVBuffer,(void*)kNVFlashBase,kNVBufSize);				/* copy flash into buffer */
			memset((void*)gNVHeader,0,kNVSize);										/* clear out NVStorage area */
			break;
#endif /* HARDWARE */

		case kNVPhaseSave:
			usingFBRAM = false;
			
			gNVBuffer = (ulong*)AllocateMemory(kNVBufSize);			/* allocate buffer for flash we need to save */
			Assert(gNVBuffer != nil);
			
			memcpy((void*)gNVBuffer,(void*)kNVFlashBase,kNVBufSize);				/* copy flash into buffer */
			
			gNVHeader = (NVHeader*)( (ulong)gNVBuffer + kNVBufSize - kNVSize );		/* point to start of NVStorage */
			
			memset((void*)gNVHeader,0,kNVSize);										/* clear out NVStorage area */
			break;

		case kNVPhaseRestore:
			gNVHeader = (NVHeader*)kNVStorageBase;									/* point to start of NVStorage */
			break;
	}
	
	gFirstNode = gNext = (NVNode*)( (ulong)gNVHeader + sizeof(NVHeader)) ;
}




void NVCommit(void)
{
	gNVHeader->checksum = NVChecksum((ulong *)gNVHeader);		/* calc cksum right before writing */

	Message(("NVCommit: new checksum = %#x\n",gNVHeader->checksum));
		
#ifdef HARDWARE	
	EraseAndFlashBlock(gNVBuffer,(ulong*)kNVFlashBase);
#else
	if (!IsError((kNVFlashBase==nil) || (gNVBuffer==nil))) 
	{
		memcpy(kNVFlashBase, gNVBuffer, kNVBufSize); 			/* copy all 128K to buffer */
		if (gSimulator->GetUseNVStore()) 
			gSimulator->SetPreference(kPrefsNVContents, kNVStorageBase, kNVSize); /* only save 8K to prefs file */
	}
#endif
	
	if( !usingFBRAM )	
		FreeMemory((void*)gNVBuffer);
}




/* Checksum all of an NV block *EXCEPT* the checksum field (first long) */

ulong NVChecksum(ulong *base)
{
	ulong sum = 0;
	ulong *ptr = base;
	
	ptr++;										/* don't include checksum field in sum */
	
	while( (ulong)ptr < ((ulong)base + kNVSize) )
	{
		sum += *ptr;
		ptr++;
	}
			
	return sum;
}




/* 	****************************
	Non-Volatile Reader & Writer 
	**************************** */

/* checksum is updated when NV block is committed */

void
NVWrite(uchar *data,long len,ulong tag)
{
/*
	Message(("NVWrite : data = %#x, len = %#x, tag = '%c%c%c%c'",data,len, 
		(tag>>24)&0xff,(tag>>16)&0xff,(tag>>8)&0xff,(tag>>0)&0xff));
*/

	printf("start: %x, len: %x, end: %x\n",(ulong)(&gNext->data), len, ((ulong)(&gNext->data) + len));
	
	gNext->len = len;									/* set up node header */
	gNext->tag = tag;
	
	memcpy( (void*)&gNext->data,(void*)data,len );		/* copy data into buffer */
	
	gNext = NEXT(gNext);								/* update next ptr */
}




/* search NV block for requested tag, return 0 if not found */

uchar*
NVRead(ulong tag,long *len)
{
	NVNode *n = gFirstNode;
		
	while( (n != nil) && ((ulong)n < (ulong)kNVStorageBase + kNVSize) )
	{
		if(n->tag == tag) 
		{
			*len = n->len;
			return (uchar*)&n->data;
		}
		
		n = NEXT(n);
	}
	
	*len = 0;
	return 0;
}




/* 	*****************************
	Non-Volatile Flag Maintenance 
	***************************** */

ulong NVGetFlags(void)
{
	NVHeader* h;
	
	h = (NVHeader*)kNVStorageBase;
	return h->flags;
}


#ifdef HARDWARE

void NVSetFlags(ulong flags)
{	
	Message(("NVSetFlags : flags = %08x",flags));
	
	usingFBRAM = true;
			
	gNVBuffer = (ulong*)GetDisplayPageBase(1);				/* use the FB to hold the data while we bang on it */
	gNVHeader = (NVHeader*)( (ulong)gNVBuffer + kNVBufSize - kNVSize );		/* point to start of NVStorage */
			
	memcpy((void*)gNVBuffer,(void*)kNVFlashBase,kNVBufSize);				/* copy flash into buffer */
	
	gNVHeader->flags = flags;												/* set flags */
		
	NVCommit();																/* and write it back */
}

#else

void NVSetFlags(ulong )
{
}

#endif



#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void NVJustSetFlags(ulong flags)
{
	gNVHeader->flags = flags;
}
#endif




