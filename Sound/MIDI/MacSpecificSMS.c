/*****************************************************/
/*
**	MacSpecificSMS.c
**
**		Macintosh specific functions used for SoundMusicSys
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**  
**
**	History	-
** 	12/15/95	Created
**	12/19/95	Added XSetHardwareSampleRate and XGetHardwareSampleRate
**	1/3/96		Changed OSErr to SMSErr. Happy New Year!
**	1/18/96		Spruced up for C++ extra error checking
**	1/24/96		Moved DeltaDecompressPtr & DeltaDecompressHandle from SoundSys.c
**	2/7/96		Renamed DeltaDecompressPtr to DecompressSampleFormatPtr
**				Renamed DeltaDecompressHandle to DecompressSampleFormatHandle
*/
/*****************************************************/
#include "X_API.h"

#if X_HARDWARE_PLATFORM == X_MACINTOSH
	#include <Types.h>
	#include <OSUtils.h>
	#include <Files.h>
	#include <Devices.h>
	#include <Resources.h>
	#include <Retrace.h>
	#include <Memory.h>
	#ifndef __SOUND__
		#include <Sound.h>
		#include <SoundComponents.h>
		#include <SoundInput.h>
	#endif
	#include <Errors.h>
	#include <GestaltEqu.h>
	#include <Traps.h>
#endif

#include "X_API.h"
#include "MacSpecificSMS.h"

#ifndef __MIXEDMODE__
	#include <SysEqu.h>
#else
	#include <LowMem.h>
	#define LMGetSoundActive() (* (unsigned char *) 0x27E)
#endif

#ifndef gestalt16BitSoundIO
	#define	gestalt16BitSoundIO			7
#endif
#ifndef gestalt16BitAudioSupport
	#define	gestalt16BitAudioSupport		12
#endif

#ifndef __MIXEDMODE__
/* Sound Manager 3.0 routines
*/
pascal OSErr GetDefaultOutputVolume(long *level) = {0x203C,0x022C,0x0018,0xA800};
pascal OSErr SetDefaultOutputVolume(long level) = {0x203C,0x0230,0x0018,0xA800};
#endif



#if THINK_C
	#include <Think.h>
#else
	#ifndef TRUE
		#define TRUE	true
		#define FALSE	false
	#endif
#endif

#ifndef NULL
	#define NULL	0L
#endif

#include "PrivateSoundMusicSystem.h"


#if X_HARDWARE_PLATFORM == X_MACINTOSH

// For the vertical blank task
static long 	*	taskPtr;
static VBLTask	*	theSoundVBPtr;
static THz			theTaskZonePtr;


#if GENERATING68K	
// 68k based VBL task function
#pragma parameter __D0 GetA0toVariable
pascal long GetA0toVariable(void) = {0x2028, 0xFFFC}; 

static pascal void DoSoundVB(void)
{
	long			saveA5, newA5;

	newA5 = GetA0toVariable();	/* Globals register points the vbl task structure */

	saveA5 = SetGlobalsRegister(newA5);			/* set current a5 */
	theSoundVBPtr->vblCount = 1;		/* tell it to contiue */

	ProcessVBLTasks();

	/* Restore A5 for the rest of the interupt process
	*/
	SetGlobalsRegister(saveA5);         /* restore previous a5 */
}
#else
// PowerPC based VBL task function
static pascal void DoSoundVB(VBLTaskPtr theVBLTask) 
{
	if (theVBLTask == theSoundVBPtr)
	{
		theSoundVBPtr->vblCount = 1;		/* tell it to contiue */
		ProcessVBLTasks();
	}
}

#endif


#if GENERATING68K	
// 68k based VBL task setup and cleanup
SMSErr SetupVBLTask(void)
{
	SMSErr	theErr;
	struct Jump
	{
		short 	bsr_inderect;		// 0
		void *	address;			// 4
		long		movea;			// 8
		short	jmp_a1;			// 12
	};
	struct Jump *heapJumpPtr;

/* This elimnates the need to flush the data/code caches on the '040

		0x6104,			 bsr.s 6(pc)
		0x0000, 0x0000,	 dc.l	0
		0x225F			 movea.l (a7)+, a1
		0x2251			 move (a1), a1
		0x4Ed1			 jmp (a1)
*/

/* Set up Vertical Blank Interrupt
*/
	theErr = SMS_NO_ERR;
	taskPtr = (void *)NewPtrClear((long)sizeof(VBLTask) + sizeof(long));
	heapJumpPtr = (void *)NewPtrSys((long)sizeof(struct Jump));
	if ( (taskPtr) && (heapJumpPtr) )
	{
		if (IsVirtualMemoryAvailable())
		{
			LockMemory(taskPtr, (long)sizeof(VBLTask) + sizeof(long));
			LockMemory(heapJumpPtr, (long)sizeof(struct Jump));
		}

		theSoundVBPtr = (void *)( (Byte *)taskPtr + sizeof(long) );
		taskPtr[0] = GetGlobalsRegister();
		theTaskZonePtr = GetZone();
		heapJumpPtr->bsr_inderect = 0x6104;
		heapJumpPtr->address = DoSoundVB;
		heapJumpPtr->movea = 0x225F2251L;
		heapJumpPtr->jmp_a1 = 0x4Ed1;

		theSoundVBPtr->vblAddr = (void *)heapJumpPtr;
		theSoundVBPtr->vblCount = 1;		/* Every 1/60th of a second */
		theSoundVBPtr->qType = vType;
		theSoundVBPtr->qLink = NULL;
		theSoundVBPtr->vblPhase = 0;

/* Ok, flush the code/data cache for the '040 Macs. */
#ifndef __MIXEDMODE__
		if (*((char *)CPUFlag) >= 2)
#else
		if (LMGetCPUFlag() >= 2)
#endif
		{
			FlushInstructionCache();
			FlushDataCache();
		}

		if (VInstall((QElemPtr)theSoundVBPtr))
		{
			theErr = SMS_ITASK_FAILED;
		}
	}
	else
	{
		if (heapJumpPtr)
		{
			if (IsVirtualMemoryAvailable())
			{
				UnlockMemory((void *)heapJumpPtr, 12L);
			}
			DisposePtr((void *)heapJumpPtr);
		}
		if (taskPtr)
		{
			if (IsVirtualMemoryAvailable())
			{
				UnlockMemory(taskPtr, (long)sizeof(VBLTask) + sizeof(long));
			}
			DisposePtr((void *)taskPtr);
			taskPtr = NULL;
			theSoundVBPtr = NULL;
		}
		theErr = SMS_MEMORY_ERR;
	}
	return theErr;
}

SMSErr CleanupVBLTask(void)
{
	if (theSoundVBPtr)
	{
		VRemove((QElemPtr)theSoundVBPtr);
		if (theSoundVBPtr->vblAddr)
		{
			if (IsVirtualMemoryAvailable())
			{
				UnlockMemory((void *)theSoundVBPtr->vblAddr, 12L);
			}
			DisposePtr((void *)theSoundVBPtr->vblAddr);
		}
		theSoundVBPtr = NULL;
	}
	if (taskPtr)
	{
		if (IsVirtualMemoryAvailable())
		{
			UnlockMemory(taskPtr, (long)sizeof(VBLTask) + sizeof(long));
		}
		DisposePtr((void *)taskPtr);
	}
	return SMS_NO_ERR;
}
#else
// PowerPC based VBL task setup and cleanup
SMSErr SetupVBLTask(void)
{
	SMSErr	theErr;
/* Set up Vertical Blank Interrupt
*/
	theErr = SMS_NO_ERR;
	theSoundVBPtr = (VBLTask *)NewPtrSysClear((long)sizeof(VBLTask));
	if (theSoundVBPtr)
	{
		if (IsVirtualMemoryAvailable())
		{
			LockMemory(theSoundVBPtr, (long)sizeof(VBLTask));
		}
		theTaskZonePtr = GetZone();
		SetZone(SystemZone());

		theSoundVBPtr->vblAddr = NewVBLProc(DoSoundVB);
		theSoundVBPtr->vblCount = 1;		/* Every 1/60th of a second */
		theSoundVBPtr->qType = vType;
		theSoundVBPtr->qLink = NULL;
		theSoundVBPtr->vblPhase = 0;
		SetZone(theTaskZonePtr);

		if (VInstall((QElemPtr)theSoundVBPtr))
		{
			theErr = SMS_ITASK_FAILED;
		}
	}
	else
	{
		theErr = SMS_MEMORY_ERR;
	}
	return theErr;
}

SMSErr CleanupVBLTask(void)
{
	if (theSoundVBPtr)
	{
		VRemove((QElemPtr)theSoundVBPtr);
		if (theSoundVBPtr->vblAddr)
		{
			SetZone(SystemZone());
			DisposeRoutineDescriptor((RoutineDescriptor *)theSoundVBPtr->vblAddr);
			SetZone(theTaskZonePtr);
		}
		theSoundVBPtr = NULL;
	}
	if (theSoundVBPtr)
	{
		if (IsVirtualMemoryAvailable())
		{
			UnlockMemory(theSoundVBPtr, (long)sizeof(VBLTask));
		}
		SetZone(SystemZone());
		DisposPtr((Ptr)theSoundVBPtr);
		SetZone(theTaskZonePtr);
	}
	return SMS_NO_ERR;
}
#endif


SMSBoolean IsSoundManagerActive(void)
{
#ifdef __MIXEDMODE__
	return (LMGetSoundActive()) ? TRUE : FALSE;
#else
	return ( *( (Byte *)SoundActive )) ? TRUE : FALSE;
#endif
}

/*
** This will return TRUE if the virtual memory manager is installed, otherwise FALSE.
*/
SMSBoolean IsVirtualMemoryAvailable(void)
{
	return XIsVirtualMemoryAvailable();
}

SMSBoolean IsSoundManager16Bit(void)
{
	return XIs16BitSupported();
}

// Are we executing on a PowerPC 601 or better?
SMSBoolean IsCPUPowerPC(void)
{
	long	feature;

	feature = 0;
	if (Gestalt(gestaltNativeCPUtype, &feature) != SMS_NO_ERR)
	{
		feature = 0;
	}
	return (feature >= gestaltCPU601) ? TRUE : FALSE;
}

SMSBoolean IsSoundManagerStereo(void)
{
#if CODE_BASE != USE_68K
	return XIsStereoSupported();
#endif
	return FALSE;
}

/*
** This will return TRUE if the new sound manager is installed, otherwise FALSE.
*/
SMSBoolean IsNewSoundManagerInstalled(void)
{
	NumVersion	theVersion;
	void *		trap1;
	void *		trap2;
	SMSBoolean	installed;
	long			feature;

	installed = FALSE;
	trap1 = (void *)GetToolTrapAddress(_SoundDispatch);	/* SndSoundManagerVersion Trap */
	trap2 = (void *)GetToolTrapAddress(_Unimplemented);	/* Unimplemented Trap */
	if (trap1 != trap2)
	{
		feature = (long)SndSoundManagerVersion();
		theVersion = *( (NumVersion *)&feature);
		if (theVersion.majorRev >= 3)
		{
			installed = TRUE;
		}
		else
		{
			// Now must double verify results, because if QuickTime is not installed and SM 3.0 is installed on certion Macs,
			// then SndSoundManagerVersion will return the wrong version number!! (Thanks Apple)

			feature = 0;
			if (Gestalt(gestaltSoundAttr, &feature) == SMS_NO_ERR)
			{
				if (feature & (1<<gestaltSndPlayDoubleBuffer))
				{
					// well the double buffer proc is there, so that means that SM 3.0 is installed even though SndSoundManagerVersion
					// told us otherwise!
					installed = TRUE;
				}
			}
		}
	}
	return installed;
}
#endif	// X_HARDWARE_PLATFORM == X_MACINTOSH


// Decompress a 'csnd' format sound.
// First byte is a type.
// Next three bytes are a length.
// Type 0 is Delta LZSS compression
void * DecompressSampleFormatPtr(void * pData, long dataSize)
{
	long		theTotalSize;
	char		theType;
	XPTR	theNewData;

	theTotalSize = XGetLong(pData);
	theType = theTotalSize >> 24L;		// get type

	theNewData = XNewPtr(theTotalSize);
	if (theNewData)
	{
		switch (theType)
		{
			case 0:
				LZSSDeltaUncompress((unsigned char *)(char *)(pData) + sizeof(long), dataSize - sizeof(long), 
								(unsigned char *)theNewData, &theTotalSize);
				break;
			default:
				XDisposePtr(theNewData);
				theNewData = NULL;
				break;
		}
	}

	return theNewData;
}

#if X_HARDWARE_PLATFORM == X_MACINTOSH
// Decompress a 'csnd' format sound.
// First byte is a type.
// Next three bytes are a length.
// Type 0 is Delta LZSS compression
Handle DecompressSampleFormatHandle(register Handle theData)
{
	long		theTotalSize;
	char		theType;
	Handle	theNewData;

	HLock(theData);

	theTotalSize = XGetLong(*theData);
	theType = theTotalSize >> 24L;		// get type
	theNewData = NewHandle(theTotalSize);
	if (theNewData)
	{
		HLock(theNewData);
		switch (theType)
		{
			case 0:
				LZSSDeltaUncompress((unsigned char *)(*theData) + sizeof(long), GetHandleSize(theData) - sizeof(long), 
								(unsigned char *)*theNewData, &theTotalSize);
				break;
			default:
				DisposeHandle(theNewData);
				theNewData = NULL;
				break;
		}
		if (theNewData)
		{
			HUnlock(theNewData);
		}
	}
	HUnlock(theData);

	return(theNewData);
}
#endif	// X_HARDWARE_PLATFORM == X_MACINTOSH


void * DecompressPtr(register void * pData, long dataSize)
{
	long theTotalSize;
	XPTR theNewData;

	XBlockMove(pData, &theTotalSize, (long)sizeof(long));

	theNewData = XNewPtr(theTotalSize);
	if (theNewData)
	{
		LZSSUncompress((unsigned char *)(char *)(pData) + sizeof(long), dataSize - sizeof(long), 
						(unsigned char *)theNewData, &theTotalSize);
	}

	return theNewData;
}

#if X_HARDWARE_PLATFORM == X_MACINTOSH
void XSetHardwareVolume(short int theVolume)
{
	long		newVolume;

	if (theVolume < 0)
	{
		theVolume = 0;
	}
	if (theVolume > FULL_VOLUME)
	{
		theVolume = FULL_VOLUME;
	}

/* Scale volume to match the Macintosh hardware
*/
	if (IsNewSoundManagerInstalled())
	{
		newVolume = theVolume;
		newVolume |= (newVolume << 16L);
		SetDefaultOutputVolume(newVolume);
	}
#if GENERATING68K	
	else
	{
		newVolume = (theVolume * 7) / FULL_VOLUME;
		SetSoundVol((short int)newVolume);
	}
#endif
}

short int XGetHardwareVolume(void)
{
	short	theRealVolume;
	long		theLongRealVolume;

	if (IsNewSoundManagerInstalled())
	{
		GetDefaultOutputVolume(&theLongRealVolume);
		theRealVolume = theLongRealVolume & 0xFFFFL;
	}
#if GENERATING68K	
	else
	{
		GetSoundVol(&theRealVolume);
		theRealVolume++;
#if USE_A5 == 0
		theRealVolume = DivideUnsigned((theRealVolume * (long)FULL_VOLUME), 7);
#else
		theRealVolume = (theRealVolume * (long)FULL_VOLUME) / 7;
#endif
	}
#endif
	return theRealVolume;
}

void XSetHardwareSampleRate(unsigned long sampleRate)
{
#if 0
	NumVersion	theVersion;
	long			feature;

	feature = (long)SndSoundManagerVersion();
	theVersion = *( (NumVersion *)&feature);
	if (theVersion.majorRev >= 3)
	{
		if (theVersion.minorAndBugRev >= 1)
		{
			SetSoundOutputInfo(NULL, siSampleRate, &sampleRate);
		}
	}
#else
	sampleRate = sampleRate;
#endif
}

unsigned long XGetHardwareSampleRate(void)
{
#if 0
	long			sampleRate;
	NumVersion	theVersion;
	long			feature;

	sampleRate = rate22khz;
	feature = (long)SndSoundManagerVersion();
	theVersion = *( (NumVersion *)&feature);
	if (theVersion.majorRev >= 3)
	{
		if (theVersion.minorAndBugRev >= 1)
		{
			if (GetSoundOutputInfo(NULL, siSampleRate, &sampleRate))
			{
				sampleRate = rate22khz;
			}
		}
	}
	return sampleRate;
#else
	return rate22khz;
#endif
}
#endif	// X_HARDWARE_PLATFORM == X_MACINTOSH




// EOF of MacSpecificSMS.c
