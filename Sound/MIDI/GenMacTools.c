/*
************************************************************************
**
** “GenMacTools.c”
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**	Confidential-- Internal use only
**
**	© Copyright 1983-1996 by Steve Hales, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
**	No portion of this program (whether in source, object, or other form)
**	may be copied, duplicated, distributed, altered, or otherwise utilized without
**	direct consent of the copyright holder.
**
** Overview
**	General purpose Music Synthesis software, C-only implementation (no assembly
**    language optimizations made)
**
**	The file should be converted to the platform of choice. All the GM API uses functions from
**	here and the X_API code.
**
** Modification History:
**
**	8/19/93		Split from main file
**	10/12/94	Cleaned up, and created set the slice to 256.
**	11/7/95		Major changes, revised just about everything.
**	11/11/95	Added microSyncCount code for live link
**	11/16/95	Removed microSyncCount
**	1/18/96		Spruced up for C++ extra error checking
**	1/28/96		Changed memory API to use X_API memory API
**	2/3/96		Removed more Macintosh specific stuff outside
**	2/8/96		Added automatic detection of A4/A5 stuff for 68k
**	2/11/96		Removed FixedDivide & FixedMultiply. Use XFixedDivide & XFixedMultiply
**				Removed MoveData. Use XBlockMove
**
**
************************************************************************
*/


#include "GenSnd.h"
#include "GenPriv.h"
#include "X_API.h"

#if X_HARDWARE_PLATFORM == X_MACINTOSH
/*
************************************************************************
**
** “GenMacTools.c”
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**	Confidential-- Internal use only
**
**	© Copyright 1983-1996 by Steve Hales, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
**	No portion of this program (whether in source, object, or other form)
**	may be copied, duplicated, distributed, altered, or otherwise utilized without
**	direct consent of the copyright holder.
**
** Overview
**	General purpose Music Synthesis software, C-only implementation (no assembly
**    language optimizations made)
**
**	The file should be converted to the platform of choice. All the GM API uses functions from
**	here and the X_API code.
**
** Modification History:
**
**	8/19/93		Split from main file
**	10/12/94	Cleaned up, and created set the slice to 256.
**	11/7/95		Major changes, revised just about everything.
**	11/11/95	Added microSyncCount code for live link
**	11/16/95	Removed microSyncCount
**	1/18/96		Spruced up for C++ extra error checking
**	1/28/96		Changed memory API to use X_API memory API
**	2/3/96		Removed more Macintosh specific stuff outside
**	2/8/96		Added automatic detection of A4/A5 stuff for 68k
**	2/11/96		Removed FixedDivide & FixedMultiply. Use XFixedDivide & XFixedMultiply
**				Removed MoveData. Use XBlockMove
**
**
************************************************************************
*/

#include <Types.h>
#include <Events.h>
#include <OSUtils.h>
//#include <ToolUtils.h>
//#include <Files.h>
//#include <Resources.h>
//#include <Retrace.h>
//#include <Memory.h>
#include <Sound.h>
//#include <Errors.h>
//#include <Timer.h>
//#include <GestaltEqu.h>

#include "GenSnd.h"
#include "GenPriv.h"
#include "X_API.h"

#ifdef __MWERKS__
	#undef STAND_ALONE
	#ifndef powerc
		#if __A5__
			#define STAND_ALONE 0
		#else
			#define STAND_ALONE 1
		#endif
	#endif
#endif

#if STAND_ALONE
	#include <SetupA4.h>
	#include <A4Stuff.h>
#endif

// Variables
static long		audioFramesToGenerate;			// number of samples per audio frame to generate
static long		shutDownDoubleBuffer;
static long		activeDoubleBuffer;

// Code Functions

BOOL_FLAG IsOptionOn(void)
{
#if 0
	long	KMap[4];
	
	GetKeys((void *)&KMap);	/* get map of keyboard to check for option key */
#else
	register long *KMap;

	KMap = (long *)0x174;
#endif
	return(( ( (KMap[1] & 0x04) == 0x04) ? TRUE : FALSE));
}

long VX_GetMouse(void)
{
	register long *pMouse;

	pMouse = (long *)0x830;
	return *pMouse;
}

// Process one frame of audio via the Sound Manager callback system
static pascal void PV_SoundOutputDoubleProc(register SndChannelPtr processThisChannel, register SndDoubleBufferPtr pFillBuffer)
{
	register MusicVars *myMusicGlobals;
	register UINT32	frac;
#ifndef powerc
	register long		tempA5;
#endif

	if (processThisChannel && pFillBuffer)
	{
#ifndef powerc
#if STAND_ALONE == 1
		tempA5 = SetA4(pFillBuffer->dbUserInfo[0]);
#else
		tempA5 = SetA5(pFillBuffer->dbUserInfo[0]);
#endif
#endif
// We now have a valid A5 world and are ready to go.

		if ( (activeDoubleBuffer) && (shutDownDoubleBuffer == FALSE) )
		{
			myMusicGlobals = MusicGlobals;
			myMusicGlobals->insideAudioInterrupt++;
			frac = myMusicGlobals->syncCountFrac + 45100L;		// fractional of 256/372 into 16.16 fixed point
			myMusicGlobals->syncCountFrac = frac & 0xFFFFL;
			if (frac > 0xFFFFL)
			{
				myMusicGlobals->syncCount++;
			}
			// Generate one frame audio
			ProcessSampleFrame(&pFillBuffer->dbSoundData[0]);		// process first chunk
			PV_AudioTask();
			
			pFillBuffer->dbFlags |= dbBufferReady;

			myMusicGlobals->insideAudioInterrupt--;
		}
		else
		{
//			DebugStr("\pLast buffer!");
			pFillBuffer->dbFlags |= dbLastBuffer;
			activeDoubleBuffer = FALSE;
		}
#ifndef powerc
#if STAND_ALONE == 1
		SetA4(tempA5);
#else
		SetA5(tempA5);
#endif
#endif
	}
}


// -----------------------------------------------------------	//
//  Clear Mac Sound Manager double-buffer samples
// -----------------------------------------------------------	//
static void PV_ClearOutputBuffer(register SndDoubleBuffer *bufptr)
{
	register LOOPCOUNT		count;
	register OUTSAMPLE8	*dest8;
	register OUTSAMPLE16	*dest16;

	bufptr->dbFlags = 0;
	if (MusicGlobals->generate16output)
	{
		dest16 = (OUTSAMPLE16 *) &bufptr->dbSoundData[0];
	}
	else
	{
		dest8 = (OUTSAMPLE8 *) &bufptr->dbSoundData[0];	
	}
	if (MusicGlobals->generate16output)
	{	// use 16 bit output
		for (count = 0; count <  audioFramesToGenerate / 4; count++)
		{
			*dest16++ = 0;
			*dest16++ = 0;
			*dest16++ = 0;
			*dest16++ = 0;
			if (MusicGlobals->generateStereoOutput)
			{	// this is a slow way to do this!
				*dest16++ = 0;
				*dest16++ = 0;
				*dest16++ = 0;
				*dest16++ = 0;
			}
		}
	}
	else
	{	// use 8 bit output
		for (count = 0; count <  audioFramesToGenerate / 4; count++)
		{
			*dest8++ = 0x80;
			*dest8++ = 0x80;
			*dest8++ = 0x80;
			*dest8++ = 0x80;
			if (MusicGlobals->generateStereoOutput)
			{	// this is a slow way to do this!
				*dest8++ = 0x80;
				*dest8++ = 0x80;
				*dest8++ = 0x80;
				*dest8++ = 0x80;
			}
		}
	}
	bufptr->dbNumFrames = audioFramesToGenerate;
	bufptr->dbFlags = dbBufferReady;

}

// Connect to hardware.
//
// Global variables must be set before this can be called.
//
//	MusicGlobals->generate16output
//	MusicGlobals->generateStereoOutput
//	MusicGlobals->outputQuality
//
// This will allocate a sound channel, allocate two sound frame buffers
//
BOOL_FLAG InitSoundManager(void)
{
	register MusicVars *myMusicGlobals;
	register OSErr		theErr;
	register long		size;
	long				globalsRegister, sndInit;

	myMusicGlobals = MusicGlobals;

	switch (myMusicGlobals->outputQuality)
	{
		case Q_44K:
		case Q_22K:
			myMusicGlobals->theDoubleBufferHeader.dbhSampleRate = rate44khz;
			audioFramesToGenerate = 512;
			break;
		case Q_11K:
		// if 16 bit hardware is supported, then we have to change the primary output rate to
		// match the hardware platform. This is very Macintosh specific
			if (XIs16BitSupported())
			{
				myMusicGlobals->theDoubleBufferHeader.dbhSampleRate = rate22050hz;	// output 22.050 khz
			}
			else
			{
				myMusicGlobals->theDoubleBufferHeader.dbhSampleRate = rate22khz;	// output 22.254 khz
			}
			audioFramesToGenerate = 256;
			break;
	}

	if (myMusicGlobals->generateStereoOutput)
	{
//		sndInit = initStereo+initNoDrop+initNoInterp;
		sndInit = initStereo+initNoInterp;
	}
	else
	{
//		sndInit = initMono+initNoDrop+initNoInterp;
		sndInit = initMono+initNoInterp;
	}
	if (SndNewChannel(&myMusicGlobals->theOutputChannel, sampledSynth, sndInit, NULL))
	{
		myMusicGlobals->theOutputChannel = NULL;
		return FALSE;
	}
	size = sizeof(SndDoubleBuffer);

	if (myMusicGlobals->generate16output == FALSE)
	{
		size += (sizeof(OUTSAMPLE8) * audioFramesToGenerate);
	}
	else
	{
		size += (sizeof(OUTSAMPLE16) * audioFramesToGenerate);
	}
	if (myMusicGlobals->generateStereoOutput)
	{
		size *= 2;
	}

	myMusicGlobals->theBuffer1 = (SndDoubleBufferPtr)XNewPtr(size);
	myMusicGlobals->theBuffer2 = (SndDoubleBufferPtr)XNewPtr(size);
	if ( (myMusicGlobals->theBuffer1 == NULL) || (myMusicGlobals->theBuffer2 == NULL) )
	{
		return FALSE;
	}

#ifndef __powerc
#if STAND_ALONE == 1
	globalsRegister = SetCurrentA4();
#else
	globalsRegister = SetCurrentA5();
#endif
#else
	globalsRegister = 0;
#endif

	myMusicGlobals->theBuffer1->dbUserInfo[0] = globalsRegister;
	myMusicGlobals->theBuffer1->dbUserInfo[1] = 0;
	myMusicGlobals->theBuffer2->dbUserInfo[0] = globalsRegister;
	myMusicGlobals->theBuffer2->dbUserInfo[1] = 0;

	PV_ClearOutputBuffer(myMusicGlobals->theBuffer1);
	PV_ClearOutputBuffer(myMusicGlobals->theBuffer2);

	myMusicGlobals->theDoubleBufferHeader.dbhNumChannels = (myMusicGlobals->generateStereoOutput) ? 2 : 1;

	myMusicGlobals->theDoubleBufferHeader.dbhSampleSize = (myMusicGlobals->generate16output) ? 16 : 8;
	myMusicGlobals->theDoubleBufferHeader.dbhCompressionID = 0;
	myMusicGlobals->theDoubleBufferHeader.dbhPacketSize = 0;
	myMusicGlobals->theDoubleBufferHeader.dbhBufferPtr[0] = myMusicGlobals->theBuffer1;
	myMusicGlobals->theDoubleBufferHeader.dbhBufferPtr[1] = myMusicGlobals->theBuffer2;

#ifdef __MIXEDMODE__
	myMusicGlobals->theDoubleBufferHeader.dbhDoubleBack = NewSndDoubleBackProc(PV_SoundOutputDoubleProc);
#else
	myMusicGlobals->theDoubleBufferHeader.dbhDoubleBack = (SndDoubleBackProcPtr) PV_SoundOutputDoubleProc;
#endif

	shutDownDoubleBuffer = FALSE;
	activeDoubleBuffer = TRUE;
	theErr = SndPlayDoubleBuffer(myMusicGlobals->theOutputChannel, &myMusicGlobals->theDoubleBufferHeader);
	if (theErr != noErr)
	{
		return FALSE;
	}
	return TRUE;
}

// Disconnect from hardware only
void FinisSoundManager(void)
{
	SndCommand	theCmd;
	long			ticks;

	if (MusicGlobals->theOutputChannel)
	{
//  Kill sounds currently playing in this channel
		theCmd.cmd = flushCmd;
		SndDoImmediate(MusicGlobals->theOutputChannel, &theCmd);
		theCmd.cmd = quietCmd;
		SndDoImmediate(MusicGlobals->theOutputChannel, &theCmd);

		shutDownDoubleBuffer = TRUE;
		SndDisposeChannel(MusicGlobals->theOutputChannel, FALSE);
		MusicGlobals->theOutputChannel = NULL;
		activeDoubleBuffer = FALSE;
		ticks = TickCount() + 3;
		while (TickCount() < ticks) {};		// wait until released, just to make sure
	}
#ifdef __MIXEDMODE__
	DisposeRoutineDescriptor(MusicGlobals->theDoubleBufferHeader.dbhDoubleBack);
#endif
	if (MusicGlobals->theBuffer1)
	{
		XDisposePtr((XPTR)MusicGlobals->theBuffer1);
		MusicGlobals->theBuffer1 = NULL;
	}
	if (MusicGlobals->theBuffer2)
	{
		XDisposePtr((XPTR)MusicGlobals->theBuffer2);
		MusicGlobals->theBuffer2 = NULL;
	}
	MusicGlobals->theOutputChannel = NULL;
}

// EOF of GenMacTools.c

#else

void FinisSoundManager(void)
{
}

BOOL_FLAG InitSoundManager(void)
{
	return TRUE;
}

void musicBufferHandler (short *bufferToFill);

void musicBufferHandler (short *bufferToFill)
{
	register MusicVars *myMusicGlobals;
	register UINT32	frac;

#if 1
		myMusicGlobals = MusicGlobals;
		frac = myMusicGlobals->syncCountFrac + 45100L;		// fractional of 256/372 into 16.16 fixed point
		myMusicGlobals->syncCountFrac = frac & 0xFFFFL;
		if (frac > 0xFFFFL)
		{
			myMusicGlobals->syncCount++;
		}
		// Generate one frame audio
		ProcessSampleFrame(bufferToFill);
#endif
}

#endif

