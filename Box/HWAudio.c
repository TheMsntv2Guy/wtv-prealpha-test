#include "WTVTypes.h"
#include "BoxBoot.h"
#include "BoxUtils.h"
#include "BoxHWEquates.h"
#include "BoxAbsoluteGlobals.h"
#include "Debug.h"
#include "Interrupts.h"
#include "HWRegs.h"

#include "HWAudio.h"
#include "HWRegs.h"


#ifdef __MWERKS__
#include "boxansi.h"
#endif

// We should #include this from X_API.h, which should specify it.
void musicBufferHandler (short *bufferToFill);


ulong gAudioBuf0;
ulong gAudioBuf1;
ulong gCurrentAudioBuf;

static int inInterrupt;

/* This should be called RIGHT AFTER InitDisplay() */

void InitAudio(void)
{
ulong datatop;

	Message(("Initializing audio..."));

	/* By this point, the Display driver will have allocated it's
	   field buffers right above the data/bss sections, and pointed
	   agSysHeapBase to the location just above those field buffers.
	   We'll allocate the 2 audio buffers right above the field
	   buffers & update the sys heap base ptr accordingly.
	 */

	inInterrupt = 0;
	datatop = (ulong) READ_AG(agSysHeapBase);

	datatop = ((datatop + kAudioBufAlign) & ~kAudioBufAlign);	

	gAudioBuf0 = (datatop & 0x0fffffff) | kAudioBufferSegment;
	gAudioBuf1 = (gAudioBuf0 + kAudioBufferSize) | kAudioBufferSegment;
				
	datatop += (kAudioBufferSize << 1);			/* sysheap starts after 2 bufs */
	
	WRITE_AG(agSysHeapBase, (char*)datatop);		

	gCurrentAudioBuf = gAudioBuf0;

	Message(("Audio Buffer 0 @ %lx", gAudioBuf0));
	Message(("Audio Buffer 1 @ %lx", gAudioBuf1));
	Message(("New Sys Heap Base = %lx", READ_AG(agSysHeapBase)));

	if(SPOTVersion() != kSPOT1)
	{
		*(volatile ulong *)kAudNConfig = 0;					// Set to 16 bit, stereo
		*(volatile ulong *)kAudNSize = kAudioBufferSize;
	}

	*(volatile ulong *)(kSPOTBase + kBusChipCntl) |= kAudClkDiv4;
}



#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong GetAudioOptions(void)
{
	if(SPOTVersion() != kSPOT1)
	{
		return *(volatile ulong *)kAudNConfig;
	}
	else
		return 0;
}
#endif


/* 	Set up the desired options, Size, and Start address, then call this to
   	enable the DMA engine.
	
	Does NOT enable interrupts.  If you want interrupts, enable them before
	calling this.
*/

void StartAudioDMA(void)
{
	if(SPOTVersion() != kSPOT1)
	{
		// note: set it up twice to start, so it know to go to next buffer
		*(volatile ulong *)kAudNStart = (gAudioBuf0 & 0x0fffffff);
		*(volatile ulong *)kAudDMACntl = (kDMA_NV | kDMA_Enable);
		*(volatile ulong *)kAudNStart = (gAudioBuf1 & 0x0fffffff);
		*(volatile ulong *)kAudDMACntl = (kDMA_NV | kDMA_Enable);
	}
}


void StopAudioDMA(void)
{
	if(SPOTVersion() != kSPOT1)
	{
		*(volatile ulong *)kAudDMACntl = 0;
	}
}



void audioInterruptHandler(void)
{
	inInterrupt = 1;


	if( *(vulong *)kAudCCnt != kAudioBufferSize )			/* other buf DMA still running? */
	{
		musicBufferHandler ( (short *) gCurrentAudioBuf );	
	
		*(vulong *)kAudNStart = (gCurrentAudioBuf & 0x0fffffff);
		*(vulong *)kAudNSize = kAudioBufferSize;
		*(vulong *)kAudNConfig = 0;
	
		if (gCurrentAudioBuf == gAudioBuf0)
			gCurrentAudioBuf = gAudioBuf1;
		else
			gCurrentAudioBuf = gAudioBuf0;
	}
	else													/* we lost one or more interrupts, dammit */
	{
		memset(gAudioBuf0, 0, kAudioBufferSize);			/* 11.6ms silence */

		musicBufferHandler ( (short *) gAudioBuf1 );		/* takes up to ~6ms */	

		StartAudioDMA();									/* start playing buf 0, will take 11.6ms */
				
		gCurrentAudioBuf = gAudioBuf0;
	}


	*(vulong *)(kBusIntStatusClearReg) = kAudioIntMask;		/* clear interrupt source */
	*(vulong *)kAudDMACntl = (kDMA_NV | kDMA_Enable);		/* enable next DMA transaction */
	
	
	inInterrupt = 0;
}






/* 	==============
	Test-only code
	============== */

#if 0					// use this to test on scope -- generates triangle waves.
#define kStepPerSample		0x08000000

	long i;
	long *audPtr;
	audPtr = (long*)gCurrentAudioBuf;
	static long	height = 0;
	static long direction = kStepPerSample;
	static long	directionChanges;
	static long	maxHeight;
	static long	minHeight;
	
	directionChanges = 0;
	for (i = 0; i < 512; i++) {
		if (direction > 0 && (height + direction < height)) {
			direction = -direction;
			directionChanges++;
			maxHeight = height;
		}
		else if (direction < 0 && (height + direction > height)) {
			direction = -direction;
			directionChanges++;
			minHeight = height;
		}
		height += direction;
		
		*audPtr++ = height;
	}
#endif

