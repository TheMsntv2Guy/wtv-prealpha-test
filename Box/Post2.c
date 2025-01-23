
/**************************************/
/**** Includes                    *****/
/**************************************/
#include "HWRegs.h"
#include "BoxHWEquates.h"
#include "BoxAbsoluteGlobals.h"

#include "BoxUtils.h"
#include "Checksum.h"
#include "WTVTypes.h"
#include "Interrupts.h"
#include "HWKeyboard.h"

#include "ErrorNumbers.h"
#include "Debug.h"
#include "SystemGlobals.h"
#include "Interrupts.h"
#include "Utilities.h"


#include "HWModem.h"
#include "HWAudio.h"
#include "HWDisplay.h"

#include "Serial.h"

#include "Post.h"
#include "Debug.h"

#define kStepPerSample                0x08000000

Boolean ModemTest()
{

	Message(("ModemTest: Begin"));

	char *tt33CallingTone = "AT %TT33";
	char *tt34CallingTone = "AT %TT33";
	char *temp = "AT DT 2 \r";    /* Just for now until Simulator is working */

	short count;
	ulong sent = 0;
	
	Message(("ModemTest: Begin"));

	SerialSetBaud(kMODEM_PORT, 38400);
	
	SerialSetDtr(kMODEM_PORT, true);
	DelayFor(60);
	count = (short) strlen(temp);
	while(sent < count)
	{
		Message(("ModemTest: Sending %c to Modem", temp[sent]));
		SerialWriteCharSync(kMODEM_PORT, temp[sent]);
		sent++;
	}
		

/*	
	count = (short) strlen(tt33CallingTone);
	while(sent < count)
	{
		if(!putchar_modem(tt33CallingTone[sent]))
			sent++;
	}
	
	sent = 0;
	count = (short) strlen(tt34CallingTone);
	while(sent < count)
	{
		if(!putchar_modem(tt34CallingTone[sent]))
			sent++;
	}
*/

	return (true);  /* Is there a way to tell when this fails  */
}

/***************************************************************************/

void AudioTest()
{
	long *temp0;
	long *temp1;
	
	short i;
	ulong x;
	ulong ints;
	
	static long	height = 0;
	static long direction = kStepPerSample;

#ifdef NEED_INIT

	static long	height = 0;
	static long direction = kStepPerSample;
	
	ulong datatop;
	ulong fieldBufferSize;

	Message(("Post:Initializing audio..."));

	/* By this point, the Display driver will have allocated it's
	   field buffers right above the data/bss sections, and pointed
	   agSysHeapBase to the location just above those field buffers.
	   We'll allocate the 2 audio buffers right above the field
	   buffers & update the sys heap base ptr accordingly.
	 */

	datatop = (ulong) READ_AG(agSysHeapBase);

	datatop = ((datatop + kAudioBufAlign) & ~kAudioBufAlign);	

	gAudioBuf0 = (datatop & 0x0fffffff) | kAudioBufferSegment;
	gAudioBuf1 = (gAudioBuf0 + kAudioBufferSize) | kAudioBufferSegment;
				
	datatop += (kAudioBufferSize << 1);			/* sysheap starts after 2 bufs */
	
	WRITE_AG(agSysHeapBase, (char*)datatop);		

	gCurrentAudioBuf = gAudioBuf0;

	Message(("Audio Buffer 0 @ %x", gAudioBuf0));
	Message(("Audio Buffer 1 @ %x", gAudioBuf1));
	Message(("New Sys Heap Base = %x", READ_AG(agSysHeapBase)));

	*(volatile ulong *)kAudNConfig = 0;					// Set to 16 bit, stereo
	*(volatile ulong *)kAudNSize = kAudioBufferSize;

#endif NEED_INIT
		
	*(volatile ulong *)kAudDMACntl = 0; 			// Make sure DMA is off!

	temp0 = (long *)gAudioBuf0;
	temp1 = (long *)gAudioBuf1;

	Message(("TempAudio Buffer 0 @ %x", temp0));
	Message(("TempAudio Buffer 1 @ %x", temp1));
	
	for (i = 0; i < 512; i++) {

//		if (direction > 0 && (height + direction < height)) 
//			direction = -direction;
		
//		else if (direction < 0 && (height + direction > height)) 
//			direction = -direction;
		
//		height += direction;

		height = 0x0123abcd;
		
		*temp0++ = height;
		*temp1++ = height;
	}		
	
	*(volatile ulong *)kAudNConfig = 0;					// Set to 16 bit, stereo
	*(volatile ulong *)kAudNSize = kAudioBufferSize;

	Message(("Start Audio DMA"));
	
	Message(("Jumping to debugger before initiating DMA"));
	CallDebugger();
	
	*(volatile ulong *)kAudNStart = gAudioBuf0;
	*(volatile ulong *)kAudDMACntl = (kDMA_NVF | kDMA_Enable);
	*(volatile ulong *)kAudNStart = gAudioBuf1;
	*(volatile ulong *)kAudDMACntl = (kDMA_NVF| kDMA_Enable);
	
	DelayFor(20);
	
	Message(("Jumping to debugger after initiating DMA"));
	CallDebugger();
	
	for (x = 0; x < 500; x++) {

		temp0 = (long *)gAudioBuf0;
		temp1 = (long *)gAudioBuf1;
			
		for (i = 0; i < 512; i++) {
	
//			if (direction > 0 && (height + direction < height)) 
//				direction = -direction;
			
//			else if (direction < 0 && (height + direction > height)) 
//				direction = -direction;
			
//			height += direction;

			height = 0x0123abcd;
			
			*temp0++ = height;
			*temp1++ = height;				
		}
	}
	
	Message(("End Audio DMA"));
	
//Message(("Stop AudioDMA"));

//		*(volatile ulong *)kAudDMACntl = 0;
}


/***************************************************************************/




void VideoTest()
{
	ulong x;
	ulong y;
	ulong wordsize;
	ulong page;
	ulong *paddr;
	ulong pattern = 0x33333333;
	
	gVideoShift = 0;
	EnableDisplay();
	
	wordsize = ( (gScreenWidth * kBytesPerPixel) * (gScreenHeight >> 1) );
	wordsize >>= 2;		/* bytes -> words */
		
	EraseDisplay(0);
	EraseDisplay(1);
		
	for (y=0; y<100; y++)
	{		
			paddr = (ulong *)GetDisplayPageBase(0);
			for (x=0; x<wordsize; x++)
				*paddr++ = pattern;

			paddr = (ulong *)GetDisplayPageBase(2);
			for (x=0; x<wordsize; x++)
				*paddr++ = pattern;
				
			DisplayPage(0);
			
			paddr = (ulong *)GetDisplayPageBase(1);
			for (x=0; x<wordsize; x++)
				*paddr++ = pattern;

			paddr = (ulong *)GetDisplayPageBase(3);
			for (x=0; x<wordsize; x++)
				*paddr++ = pattern;
				
			DisplayPage(1);				
	}

	EraseDisplay(0);
	EraseDisplay(1);
}
