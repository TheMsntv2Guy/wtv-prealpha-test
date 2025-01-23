/*****************************************************/
/*
**	MusicSystem.c
**		Revision 3
**
**		This is the object interface to Jim's music driver
** 
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**
**	History	-
**	3/15/91		Created
**	5/1/91		Added SongTicks() and fixed IsSongDone()
**	2/25/92		Added live midi input device
**	3/23/92		Added access to"Private-Sound/Music.h"
**	7/4/92		Added PlaySampleData functions
**	7/13/92		Removed all references to the Sound Manager
**	8/1/93		Split functions away from MusicDriverHandler
**	9/8/93		Better error checking
**	8/29/94		Added QLock to load/unload instruments.
**	8/30/94		Added lock/unlock to Pause/Resume
**	9/7/94		Rolled in Broderbund changes
**	9/13/94		Fixed VM tests
**	9/13/94		Added EndSound/EndSong in ChangeSystemVoices
**	9/13/94		Made ChangeSystemVoices more robust
**	9/14/94		Added some Gestalt calls for PowerPC and AV Macs
**	9/14/94		Changed ChangeSystemVoices to accept 0 for voices
**	9/19/94		Better error checking in ChangeSystemVoices and InitSoundMusicSystem
**	9/20/94		Fixed bug with ChangeSystemVoices and JX_SetupVoices
**	10/20/94	Added wrapper for 'C' code	
**	10/25/94	Added forced quality test for PPC
**	11/16/94	Reordered the JX_SetupVoices call in ChangeSystemVoices
**	11/30/94	Added an error condition if trying to play a song inside of
**				of a callback
**	12/6/94		Added volume commands
**	3/26/95		Changed copyright notice
**	3/26/95		Added PauseMusicOnly, ResumeMusicOnly, IsSoundMusicSystemPaused, 
**				IsMusicSystemPaused and  IsSoundSystemPaused for more complete pausing control.
**	3/28/95		Fixed bug with ResumeMusicOnly and Sound Manager 3.0 not being installed
**	5/3/95		Oh boy. Removed the k_ interface with MusicDriverHandler, or at least
**				hide it from most users.
**	5/4/95		Changed reference to MIDI_1 & MIDI_2 to MIDI and MIDI_OLD
**	5/15/95		Added to P_IsNewSoundManagerInstalled to verify SM 3.0 being installed
**	6/7/95		Added test to prevent code from running slow on a PowerPC
**	6/7/95		Fixed MaxVoiceLoad to return the right value on a PowerPC
**	7/24/95		Fixed MDH_GetSongVolume to return the right value
**	8/21/95		Added CacheInstrumentsNow & CacheInstrumentsLater
**	8/30/95		Changed IsSongDone to call direct
**	9/1/95		Changed GetSongVolume & SetSongVolume to direct
**	9/3/95		Changed BeginSong & BeginSongLooped & LoadSong & FreeSong &
**				SetLoopFlag to call direct
**	9/15/95		Added stereo postion calls, and sample volume controls
**	10/10/95	Made FreeSong the same as EndSong
**	11/7/95		Fixed bug with looping songs. Wouldn't stop after one loop
**	11/20/95	Removed pragma unused
**				Made StopAllEffects a direct call
**	11/28/95	Allowing overdrive of volume of SetSongVolume for PowerPC
**	12/1/95		Force a bail if not running Sound Manager 3.0 or better
**	12/7/95		Now including GenSnd.h
**				Added SetReverbType API; implemented SetMidiControllerCallback
**				Added GetReverbType
**	12/11/95	Changed the way EnableLiveLink works. Now works with connection
**				types.
**				Removed MDH_PrepForInstrumentPlayback. Now use EnableLiveLink
**				with a USE_EXTERNAL connection type
**				Moved MDH_PlayNote & MDH_StopNote into MidiQueue68k.c since they
**				are only 68k code.
**				Added DisableLiveLink
**	12/15/95	Changed memory allocation to X_API
**				Pulled out most Mac specific code
**	12/17/95	Disable reverb as default
**	1/3/96		Changed OSErr to SMSErr. Happy New Year!
**				Exposed external midi control API
**	1/7/96		Added SetEffectsVolume
**	1/12/96		Added SMS_PARAM_ERR to PV_BeginSong
**
*/
/*****************************************************/

#include <Types.h>
#include <OSUtils.h>
#include <Files.h>
#include <Resources.h>
#include <Memory.h>
#include <Events.h>
#include <Sound.h>

#include "X_API.h"
#include "MacSpecificSMS.h"


#if 0
#if THINK_C
	#include <Think.h>
#else
	#define TRUE	true
	#define FALSE	false
#endif
#endif

#include "PrivateSoundMusicSystem.h"
#include "SoundMusicSystem.h"
#include "SoundMusicMidiSys.h"
#include "WhoLicensed.h"

#if CODE_BASE == USE_NATIVE
	#include "GenSnd.h"
#endif

/*
#define USE_NEW_QUALITY_MODE		1
// SoundQuality
#define jxAnalyzeQuality			1				// best driver for speed of mac
#define jxLowQuality			2				// lowest quality (11khz) no interpolation
#define jxHighQuality			3				// high quality (22khz) no interpolation
#define jxIntrepLowQuality		4				// lowest quality with 2 pt interpolation
#define jxIntrepHighQuality		5				// high quality with 2 pt interpolation
#define jxIntrepBestLowQuality	6				// low quality with 128K memory interpolation
#define jxIntrepBestHighQuality	7				// high quality with 128K memory interpolation
*/

MDRVCodeBasePtr theMusicDriver;
//long (*theMusicDriver)(long msg, ...);

static Boolean			soundMusicSysSetup = FALSE;



static long				gDriverType = noDriverInstalled;		// private driver type
static long				gLiveLinkSaveDriverType;
static Handle 			hMusicDriverCode;
#if CODE_BASE == USE_MIXED
static UniversalProcPtr	uppMusicDriverCode;
#endif
static short int 			theCurrentSong;
static Boolean			musicPaused;
static Boolean			beingServed;
static long				startingTick;
static long				pauseTick;
static Boolean			autoFadeEnabled;
static Boolean			insideSongCallback;
static Boolean			fadeDone;
static Byte			fadeFlag;		/* FADE_OUT - Fade out, FADE_IN - Fade in */
static short int			theFadeLevel;
static short int			theFadeCrement, theFadeStart, theFadeCount;
static Boolean			preLoadSong;
static Boolean			songLoopFlag;
static AudioDoneProc	songDoneCallBackProcPtr;

static short int			lastMaxNotes, lastNormNotes, lastEffects;

static pascal void PV_HandleSongCallback(short int theSongID);


#define FADE_IN	0
#define FADE_OUT	1


#define INIT_R	(unsigned short int)56549
#define C1 (unsigned short int)52845
#define C2 (unsigned short int)22719

static unsigned short int R;

/* Extern functions:
*/
Handle DecompressHandle(Handle theHandle);
Handle CompressHandle(Handle theHandle);


long GetSyncTime(void)
{
	return JX_GetSoundBufferSync();
}

void SetSyncTime(long newTime)
{
	JX_SetSoundBufferSync(newTime);
}


#if 0
/*
static unsigned char Encrypt(register unsigned char plain)
{
	register unsigned char cipher;

	cipher = (plain ^ (R >> 8));
	R = (cipher + R) * C1 + C2;
	return(cipher);
}

static void EncryptHandle(Handle theHandle)
{
	register long theSize;
	register unsigned char *pByte, *pEnd;

	if (theHandle)
	{
		R = INIT_R;
		HLock(theHandle);
		pByte = (unsigned char *)*theHandle;
		pEnd = pByte + (long)GetHandleSize(theHandle);
		pByte += 7;
		while (pByte < pEnd)
		{
			*pByte = Encrypt(*pByte);
			pByte++;
		}
		HUnlock(theHandle);
	}
}
*/
#endif

static unsigned char PV_Decrypt(register unsigned char cipher)
{
	register unsigned char plain;

	plain = (cipher ^ (R >> 8));
	R = (cipher + R) * C1 + C2;
	return(plain);
}

static void PV_DecryptHandle(Handle theHandle)
{
	register unsigned char *pByte, *pEnd;

	if (theHandle)
	{
		R = INIT_R;
		HLock(theHandle);
		pByte = (unsigned char *)*theHandle;
		pEnd = pByte + (long)GetHandleSize(theHandle);
		pByte += 7;
		while (pByte < pEnd)
		{
			*pByte = PV_Decrypt(*pByte);
			pByte++;
		}
		HUnlock(theHandle);
	}
}


Boolean IsMusicSystemPaused(void)
{
	return musicPaused;
}


Boolean IsSoundMusicSystemPaused(void)
{
	return IsMusicSystemPaused() && IsSoundSystemPaused();
}


// Filter out any quality modes that we can't support

static SoundQuality PV_VerifyQuality(SoundQuality quality)
{
//#if USE_NEW_QUALITY_MODE == 0
#if 1 == 0
	#if CODE_BASE != USE_NATIVE
		#if _POWERPC_
// If we are compiled for PowerPC, don't allow any quality modes other than jxHighQuality, and jxLowQuality
// only do this for 68k emulation of driver on PowerPC glue code
	switch(quality)
	{
		case jxAnalyizeQuality:
		case jxIntrepLowQuality:
		case jxIntrepHighQuality:
		case jxIntrepBestLowQuality:
		case jxIntrepBestHighQuality:
			quality = jxHighQuality;
			break;
	}
		#else
// we're not compiled for a PowerPC, but we're running on one, we still need to change the modes
	if (IsCPUPowerPC())
	{
		switch(quality)
		{
			case jxAnalyizeQuality:
			case jxIntrepLowQuality:
			case jxIntrepHighQuality:
			case jxIntrepBestLowQuality:
			case jxIntrepBestHighQuality:
				quality = jxHighQuality;
				break;
		}
	}
		#endif
	#endif
	return quality;
#else
	#if CODE_BASE != USE_NATIVE
	if ( (quality & Q_RATE_44) == Q_RATE_44)
	{
		// don't allow 44k on non-PowerPC
		quality &= ~Q_RATE_44;
		quality |= Q_RATE_22;
	}
	if ( (quality & Q_USE_16) == Q_USE_16)
	{
		// no support for 16 bit
		quality &= ~Q_USE_16;
	}
	if ( (quality & Q_USE_STEREO) == Q_USE_STEREO)
	{
		// no support for stereo
		quality &= ~Q_USE_STEREO;
	}
// We're a 68k app, and if we're running on a PowerPC, kill the high end modes beause
// we'll run out of CPU time!
	if (IsCPUPowerPC())
	{
		if ( (quality & Q_USE_TYPE_1) == Q_USE_TYPE_1)
		{
			; // do nothing extra, drop sample
		}
		if ( (quality & Q_USE_TYPE_2) == Q_USE_TYPE_2)
		{
			quality &= ~Q_USE_TYPE_2;
			quality |= Q_USE_TYPE_1;
		}
		if ( (quality & Q_USE_TYPE_3) == Q_USE_TYPE_3)
		{
			quality &= ~Q_USE_TYPE_3;
			quality |= Q_USE_TYPE_1;
		}
	}
	#endif
	return quality;
#endif
}

#if CODE_BASE == USE_MIXED

enum {
	uppMusicDriverProcInfo = kCStackBased
		 | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long)))
		 | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long)))
		 | RESULT_SIZE(kFourByteCode) | REGISTER_RESULT_LOCATION(kRegisterD0)
};

#define NewMusicDriverProc(userRoutine)	(UniversalProcPtr) NewRoutineDescriptor((ProcPtr)(userRoutine), uppMusicDriverProcInfo, kM68kISA)

/*

			   		PPC app and 68K code resource 

The trick here is that you have to call CallUniversalProc() instead of merely
jumping to the CR like you do with an all 68K app/CR.  
The steps to do this are: 
	handle = Get1Resource()
	HLock(handle)	
	CallUniversalProc(*handle, ...)		
and you've jumped to main.  (Actually you've jumped to the resource's header
which branches to main.)  This example uses macros that expand to this calling 
method.  The macros are defined at the top of this file.  There is another way.
You can make a UPP from the function's address of ISAType kM68kISA first if you
want but it is unnecessary.  In that case you code would look like:
	handle = Get1Resource()
	HLock(handle)	
	myUPP = NewRoutineDescriptor(*handle, kM68kISA)
	CallUniversalProc(myUPP, ...)		

*/
static long PPC_68K_CrossJump(long msg, void * data)
{
	return CallUniversalProc(uppMusicDriverCode, uppMusicDriverProcInfo, msg, data);
}
#endif

#if (CODE_BASE == USE_MIXED) || (CODE_BASE == USE_68K)
static SMSErr PV_GetDriverCodeResource(short int theResourceID)
{
	Handle	theCode;
	SMSErr	theErr;

	theErr = SMS_NO_ERR;
	theMusicDriver = NULL;
	theCode = NULL;
	/* get driver code and lock it down. 
	*/
	theCode = (Handle)GetResource('Jnth', theResourceID);
	if (theCode)
	{
		DetachResource(theCode);		/* disconnect it from the resource manager */
	}
	else
	{
		theCode = (Handle)GetResource('MDRV', theResourceID);
		if (theCode)
		{
			DetachResource(theCode);				/* disconnect it from the resource manager */
			PV_DecryptHandle(theCode);				/* decrypt code */
			theCode = DecompressHandle(theCode);	/* DECOMPRESS IT */
			if (theCode == NULL)
			{
				theErr = SMS_MEMORY_ERR;
			}
		}
		else
		{
			theErr = SMS_CANT_LOAD_MDRV;
		}
	}

	if ( (theCode) && (theErr == SMS_NO_ERR) )
	{
		hMusicDriverCode = theCode;
		MoveHHi(theCode);
		HNoPurge(theCode);
		HLock(theCode);
#if CODE_BASE == USE_MIXED
		uppMusicDriverCode = NewMusicDriverProc((void *)*theCode);
		theMusicDriver = (MDRVCodeBasePtr)PPC_68K_CrossJump;
#else
		theMusicDriver = (MDRVCodeBasePtr)*theCode;
		if (IsVirtualMemoryAvailable())
		{
			LockMemory(*theCode, GetHandleSize(theCode));
		}
#endif
	}
	return theErr;
}

static void PV_FreeDriverCodeResource(void)
{
	if (hMusicDriverCode)
	{
#if CODE_BASE == USE_MIXED
		DisposeRoutineDescriptor(uppMusicDriverCode);
#endif
		HUnlock(hMusicDriverCode);
		if (IsVirtualMemoryAvailable())
		{
			UnlockMemory(*hMusicDriverCode, GetHandleSize(hMusicDriverCode));
		}
		DisposHandle(hMusicDriverCode);
		hMusicDriverCode = NULL;
		theMusicDriver = NULL;
	}
}
#else

static SMSErr PV_GetDriverCodeResource(short int UNUSED(theResourceID))
{
//#pragma unused (theResourceID)
#if CODE_BASE == USE_NATIVE
	theMusicDriver = GetInterfaceAddress();
#endif
	return SMS_NO_ERR;
}

static void PV_FreeDriverCodeResource(void)
{
	theMusicDriver = NULL;
}

#endif


//
// Function code for MusicDriverHandler
//

static void PV_SetLastVoices(short int maxSongVoices, short int maxNormalizedVoices, short int maxEffectVoices)
{
	lastMaxNotes = maxSongVoices;
	lastNormNotes = maxNormalizedVoices;
	lastEffects = maxEffectVoices;
}

static void PV_GetSongVoices(SongResource ** theSong, short int *maxNotes, short int *normNotes, short int *maxEffects)
{
	if (theSong)
	{
		*maxNotes = (**theSong).maxNotes;
		*normNotes = (**theSong).maxNormNotes;
		*maxEffects = (**theSong).maxEffects;
	}
}

static void PV_VerifyVoices(short int maxSongVoices, short int maxNormalizedVoices, short int maxEffectVoices)
{
	Boolean	stopVoices;

	stopVoices = TRUE;
	if (lastMaxNotes == maxSongVoices)
	{
		if (lastNormNotes == maxNormalizedVoices)
		{
			if (lastEffects == maxEffectVoices)
			{
				stopVoices = FALSE;
			}
		}
	}
	if (stopVoices)
	{
		PV_SetLastVoices(maxSongVoices, maxNormalizedVoices, maxEffectVoices);
		EndAllSound();
	}
}

SMSErr LoadSong(short int theSongID)
{
	SMSErr	theErr;
	Handle	theRes;
	short	theID;
	short int	maxSongVoices, maxNormalizedVoices, maxEffectVoices;
	
	if (gDriverType == noDriverInstalled)
	{
		return SMS_PARAM_ERR;
	}
	if (insideSongCallback)
	{
		return SMS_NOT_REENTERANT;
	}
	theErr = SMS_NO_ERR;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		theRes = GetResource(ID_SONG, (short int)theSongID);
		if (theRes)
		{
			// if the voices are different, stop all sound effects (!) don't like this but it needs to happen
			PV_GetSongVoices((SongResource **)theRes, &maxSongVoices, &maxNormalizedVoices, &maxEffectVoices);
			PV_VerifyVoices(maxSongVoices, maxNormalizedVoices, maxEffectVoices);

			theID = *((short int *)(*theRes));

			theRes = GetResource(ID_CMID, theID);
			if (theRes == NULL)
			{
				theRes = GetResource(ID_MIDI, theID);
			}
			if (theRes == NULL)
			{
				theRes = GetResource(ID_MIDI_OLD, theID);
			}
			if (theRes)
			{
				if (theCurrentSong != -1)
				{
					EndSong();		/* stop the music! */
				}
				theErr = JX_LoadNoPlaySong(theSongID);
				switch (theErr)
				{
					case 0:
						theCurrentSong = (short int)theSongID;
						preLoadSong = TRUE;
						theErr = SMS_NO_ERR;
						break;
#if CODE_BASE == USE_NATIVE
					case PARAM_ERR:
						theErr = SMS_PARAM_ERR;
						break;
					case MEMORY_ERR:
						theErr = SMS_MEMORY_ERR;
						break;
					case BAD_INSTRUMENT:
						theErr = SMS_BAD_INSTRUMENT;
						break;
					case BAD_MIDI_DATA:
						theErr = SMS_BAD_MIDI_DATA;
						break;
#else
					case 1:
						theErr = SMS_MEMORY_ERR;
						break;
#endif
					default:
						theErr = SMS_PARAM_ERR;
						break;
				}
			}
			else
			{
				theErr = SMS_NO_MIDI_DATA;
			}
		}
		else
		{
			theErr = SMS_NO_SONG_DATA;
		}
	}
	return theErr;
}


static SMSErr PV_BeginSong(short int theSongID, Boolean loopSong)
{
	SMSErr	theErr;
	short	theID;
	Handle	theRes;
	short int	maxSongVoices, maxNormalizedVoices, maxEffectVoices;

	if (gDriverType == noDriverInstalled)
	{
		return SMS_PARAM_ERR;
	}
	if (insideSongCallback)
	{
		return SMS_NOT_REENTERANT;
	}
	theErr = SMS_NO_ERR;

	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		theRes = GetResource(ID_SONG, (short int)theSongID);
		if (theRes)
		{
			// if the voices are different, stop all sound effects (!) don't like this but it needs to happen
			PV_GetSongVoices((SongResource **)theRes, &maxSongVoices, &maxNormalizedVoices, &maxEffectVoices);
			PV_VerifyVoices(maxSongVoices, maxNormalizedVoices, maxEffectVoices);

			songLoopFlag = loopSong;

			theID = *((short int *)(*theRes));
			theRes = GetResource(ID_CMID, theID);
			if (theRes == NULL)
			{
				theRes = GetResource(ID_MIDI, theID);
			}
			if (theRes == NULL)
			{
				theRes = GetResource(ID_MIDI_OLD, theID);
			}
			if (theRes)
			{
				if ( (preLoadSong) && (theCurrentSong == ((short int)theSongID)) )
				{
					JX_ReplaySongFlag(loopSong);
					JX_PlayMusic();		/* play preloaded song */
				}
				else
				{
					if (theCurrentSong != -1)
					{
						EndSong();		/* stop the music! */
					}
					JX_ReplaySongFlag(loopSong);
					theErr = JX_LoadPlaySong(theSongID);
					switch (theErr)
					{
						case 0:
							theErr = SMS_NO_ERR;
							if (autoFadeEnabled)
							{
								JX_FadeLevel(theFadeLevel);
							}
							theCurrentSong = (short int)theSongID;
#if CODE_BASE == USE_NATIVE
							JX_SetSongCallbackProc(&PV_HandleSongCallback);
#endif
							startingTick = GetSyncTime();
							preLoadSong = FALSE;
							break;
#if CODE_BASE == USE_NATIVE
						case PARAM_ERR:
							theErr = SMS_PARAM_ERR;
							break;
						case MEMORY_ERR:
							theErr = SMS_MEMORY_ERR;
							break;
						case BAD_INSTRUMENT:
							theErr = SMS_BAD_INSTRUMENT;
							break;
						case BAD_MIDI_DATA:
							theErr = SMS_BAD_MIDI_DATA;
							break;
#else
						case 1:
							theErr = SMS_MEMORY_ERR;
							break;
#endif
						default:
							theErr = SMS_PARAM_ERR;
							break;
					}
				}
			}
			else
			{
				theErr = SMS_NO_MIDI_DATA;
			}
		}
		else
		{
			theErr = SMS_NO_SONG_DATA;
		}
		if (theErr)
		{
			JX_FreeSong();			/* Dispose of song memory only */
		}
	}
	return theErr;
}

SMSErr BeginSong(short int theSong)
{
	return PV_BeginSong(theSong, FALSE);
}

SMSErr BeginSongLooped(short int theSong)
{
	return PV_BeginSong(theSong, TRUE);
}


SMSErr BeginSongFromMemory(short int theSongID, Handle theSongResource, Handle theMidiResource, Boolean loopSong)
{
	SMSErr				theErr;
	PlayMemorySongData	theInfo;
	short int				maxSongVoices, maxNormalizedVoices, maxEffectVoices;

	theErr = SMS_NO_ERR;
	if (insideSongCallback == FALSE)
	{
		if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
		{
			startingTick = GetSyncTime();
			preLoadSong = FALSE;
			songLoopFlag = loopSong;

			// if the voices are different, stop all sound effects (!) don't like this but it needs to happen
			PV_GetSongVoices((SongResource **)theSongResource, &maxSongVoices, &maxNormalizedVoices, &maxEffectVoices);
			PV_VerifyVoices(maxSongVoices, maxNormalizedVoices, maxEffectVoices);

			if (theCurrentSong != -1)
			{
				EndSong();		/* stop the music! */
			}
			JX_ReplaySongFlag(loopSong);
			if (autoFadeEnabled)
			{
				JX_FadeLevel(theFadeLevel);
			}
			theCurrentSong = theSongID;
			theInfo.theSong = theSongResource;
			theInfo.theMidi = theMidiResource;
			theInfo.theSongID = theSongID;
			theErr = JX_PlaySongFromMem(&theInfo);
			if (theErr == SMS_NO_ERR)
			{
#if CODE_BASE == USE_NATIVE
				JX_SetSongCallbackProc(&PV_HandleSongCallback);
#endif
				theCurrentSong = theSongID;
			}
			else
			{
				theErr = SMS_MEMORY_ERR;
			}
		}
	}
	else
	{
		theErr = SMS_NOT_REENTERANT;
	}
	return theErr;
}


void SetLoopSongFlag(Boolean theFlag)
{
	if (gDriverType != noDriverInstalled)
	{
		JX_ReplaySongFlag((Boolean)theFlag);
		songLoopFlag = theFlag;
	}
}


void FreeSong(void)
{
	EndSong();
}


void EndSong(void)
{
	if ( (gDriverType != noDriverInstalled) && (insideSongCallback == FALSE) )
	{
		if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
		{
			if (musicPaused)
			{
				ResumeMusicOnly();
			}
//			if ((theCurrentSong != -1) || (theCurrentSong == theSongID))
			if (theCurrentSong != -1)
			{
				JX_ReplaySongFlag(FALSE);
				JX_StopMusic();				/* die, like a virgin! */
	
				/* Notes require a separate "kill", because sound effects may still be active.
				*/
				JX_FreeSong();			/* Dispose of song memory only */
	
				theCurrentSong = -1;
				preLoadSong = FALSE;
				startingTick = 0;
			}
		}
	}
}

static Boolean PV_IsSongDone(long theSongID)
{
	register Boolean returnValue;

	theSongID = theSongID;
	returnValue = FALSE;
	if ( (gDriverType != noDriverInstalled) && (gDriverType != jxLiveInput) )
	{
		if (theCurrentSong != -1)
		{
			if (IsSoundMusicSystemPaused() == FALSE)
			{
				/* ok, now test the music in progress
				*/
				returnValue = (JX_StatusMusic()) ? FALSE : TRUE;
			}
		}
		else
		{
			returnValue = TRUE;
		}
	}
	return returnValue;
}

Boolean IsSongDone(void)
{
	return PV_IsSongDone(0L);
}


#if (! _POWERPC_) && (CODE_BASE != USE_NATIVE)
#pragma parameter __D0 GetD0toVariable
pascal long GetD0toVariable(void) = {0x2000}; 

#pragma parameter __D0 GetD1toVariable
pascal long GetD1toVariable(void) = {0x2001}; 

static long MDH_GetErrors(long theData)
{
	register char *src, *dst;
	short int theID;
	long returnValue;

	returnValue = SMS_NO_ERR;
	if (IsSoundMusicSystemPaused() == FALSE)
	{
		theCurrentSong = -1;

/*
**
These are funny lines, but this is how I get the returned values to compile under both
Think C & MPW.

The order is important. You must call GetD0toVariable first to D0 into a variable,
since this process destroys D0.

	asm
	{
		move.w	D0, theID
		move.l	D1,  src
	}
*/
		JX_GetErrors();
		theID = (short)GetD0toVariable();
		src = (void *)GetD1toVariable();
		returnValue = theID;
		if (theData)
		{
			dst = (char *)theData;
			src++;	/* skip past len byte (pascal) */
			while (*src)
			{
				*dst++ = *src++;
			}
			*dst = 0;
		}
	}
	return returnValue;
}
#else
static long MDH_GetErrors(long theData)
{
	theData = 0;

	return 0;
}
#endif


long MaxVoiceLoad(void)
{
	long			returnValue;
	SysEnvRec	theWorld;

	returnValue = 0;
	if (IsCPUPowerPC())
	{
#if CODE_BASE == USE_MIXED
		returnValue = 8;				// 68k code running on PowerPC
#endif

#if CODE_BASE == USE_NATIVE
	#if ! _POWERPC_
		returnValue = 8;				// 68k code native 'C'
	#else
		returnValue = MAX_VOICES;		// PowerPC code native 'C'
	#endif
#endif

	}
	else
	{
		if ( SysEnvirons(curSysEnvVers, &theWorld) == SMS_NO_ERR)
		{
			switch (theWorld.processor)
			{
				case env68000:
				case env68010:
				case envCPUUnknown:
					returnValue = 3;
					break;
				case env68020:
					returnValue = 6;
					break;
				case env68030:
					returnValue = 8;
					break;
				case env68040:
					returnValue = MAX_VOICES;
					break;
			}
		}
	}
	return returnValue;
}


// Enable the use of external midi data to drive synth engine
// Pass
//		USE_OMS to connect to OMS as an external device. This will load all instruments.
//		USE_MIDI_MANAGER to connect to Apple's MIDI Manager as an external device. This will load all instruments.
//		USE_EXTERNAL to use an API to drive the engine. This will NOT load any instruments.
SMSErr EnableLiveLink(short maxVoices, short normVoices, short effectVoices, short connectionType)
{
	SMSErr theErr;

	theErr = SMS_NO_ERR;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		if ( (maxVoices < normVoices) || (maxVoices > MAX_VOICES) )
		{
			theErr = SMS_PARAM_ERR;
		}
		else
		{
			theErr = ChangeSystemVoices(maxVoices, 	// max voices
									normVoices, 	// max normalize
									effectVoices	// max effects
									);
			if (theErr == SMS_NO_ERR)
			{
				theErr = SetupExternalMidi(128, connectionType);	// create midi connection
				if (theErr == SMS_NO_ERR)
				{
					if (connectionType == USE_EXTERNAL)
					{
						// this will not load instruments
						if (JX_PrepSystem())
						{
							theErr = SMS_MEMORY_ERR;
						}
					}
					else
					{
						// This will load all instruments
						if (JX_EnableMIDIInput())
						{
							theErr = SMS_MEMORY_ERR;
						}
					}
					gLiveLinkSaveDriverType = gDriverType;
					gDriverType = jxLiveInput;
					SetSongVolume(FULL_VOLUME);
				}
			}
		}
	}
	return theErr;
}

void DisableLiveLink(void)
{
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType == jxLiveInput) )
	{
		CleanupExternalMidi();
		JX_FreeSong();
		gDriverType = gLiveLinkSaveDriverType;
	}
}


SMSErr LoadInstrument(short int theInstrument)
{
	SMSErr	theErr;

	theErr = SMS_NO_ERR;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != noDriverInstalled) )
	{
		QLockNotes();
		theErr = JX_LoadThisInstrument(theInstrument);
		QUnlockNotes();
	}
	return theErr;
}

SMSErr UnloadInstrument(short instrument)
{
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != noDriverInstalled) )
	{
		QLockNotes();
		JX_UnloadThisInstrument(instrument);
		QUnlockNotes();
	}
	return SMS_NO_ERR;
}

void MDH_StopNotes(void)
{
	JX_StopNotes();
}



void SetMasterFade(long theData)
{
	register long theTicks;

	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		autoFadeEnabled = FALSE;
		if ((long)theFadeLevel != theData)	/* Don't bother if the same value */
		{
			if (theData > 256)
			{
				theData = 256;
			}
			if (theData < 0)
			{
				theData = 0;
			}
			JX_FadeLevel((short int)theData);
			theFadeLevel = theData;
			theTicks = GetSyncTime() + 5;
			while (GetSyncTime() < theTicks) {};
		}
	}
}

void BeginMasterFadeIn(long theData)
{
	long theTicks;

	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		JX_FadeLevel(0);
		theTicks = GetSyncTime() + 5;
		while (GetSyncTime() < theTicks) {};

		fadeFlag = FADE_IN;
		autoFadeEnabled = TRUE;
		fadeDone = FALSE;
		theFadeLevel = 0;
		theFadeCrement = (256 * 4) / ((short int)theData);
		theFadeStart = ((short int)theData) / 4;

		theFadeCount = theFadeStart;
	}
}

void BeginMasterFadeOut(long theData)
{
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		fadeFlag = FADE_OUT;
		autoFadeEnabled = TRUE;
		fadeDone = FALSE;
		theFadeLevel = 256;
		theFadeCrement = (256 * 4) / ((short int)theData);
		theFadeStart = ((short int)theData) / 4;

		theFadeCount = theFadeStart;
	}
}


long FadeLevel(void)
{
	return theFadeLevel;
}


Boolean IsSoundDataDone(PlaySampleData *theData)
{
	Boolean	returnValue;

	returnValue = FALSE;
	if (theData)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			returnValue = JX_IsSoundDone(theData);
		}
	}
	return returnValue;
}

long PlaySoundData(PlaySampleData * theData)
{
	long	reference;

	reference = 0;
	if (theData)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			reference = JX_PlaySoundData(theData);
		}
	}
	return reference;
}

void StopSoundData(PlaySampleData * theData)
{
	if (theData)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			JX_StopSoundData(theData);
		}
	}
}

void ChangePitchData(PlaySampleData * theData)
{
	if (theData)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			JX_ChangePitch(theData);
		}
	}
}

void ChangeVolumeData(PlaySampleData * theData)
{
	if (theData)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			JX_ChangeSampleVolume(theData);
		}
	}
}

void ChangeStereoPositionData(PlaySampleData * theData)
{
	if (theData)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			JX_ChangeSampleStereoPos(theData);
		}
	}
}

static void MDH_SetupCallbackProc(CallbackData * theData)
{
	if (theData)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			JX_SetupCallbackVars(theData);
		}
	}
}




static long MDH_GetMIDIClock(void)
{
	register long returnValue;
	short int saveSong;

	returnValue = 0L;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		/* ok, now service the music in progress
		*/
		if (theCurrentSong != -1)
		{
			saveSong = theCurrentSong;
			theCurrentSong = -1;
			returnValue = JX_GetMidiClocks();
			theCurrentSong = saveSong;
		}
	}
	return returnValue;
}

static long MDH_GetMIDIBeat(void)
{
	register long returnValue;
	short int saveSong;

	returnValue = 0L;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		/* ok, now service the music in progress
		*/
		if (theCurrentSong != -1)
		{
			saveSong = theCurrentSong;
			theCurrentSong = -1;
			returnValue = JX_GetBeat();
			theCurrentSong = saveSong;
		}
	}
	return returnValue;
}

static long MDH_GetNextMidiBeat(void)
{
	register long returnValue;
	short int saveSong;

	returnValue = 0L;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		/* ok, now service the music in progress
		*/
		if (theCurrentSong != -1)
		{
			saveSong = theCurrentSong;
			theCurrentSong = -1;
			returnValue = JX_GetNextBeatClock();
			theCurrentSong = saveSong;
		}
	}
	return returnValue;
}



static long MDH_GetCurrentTempo(void)
{
	register long returnValue;
	short int saveSong;

	returnValue = 0L;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		/* ok, now service the music in progress
		*/
		if (theCurrentSong != -1)
		{
			saveSong = theCurrentSong;
			theCurrentSong = -1;
			returnValue = JX_GetTempo();
			theCurrentSong = saveSong;
		}
	}
	return returnValue;
}

static long MDH_GetSongLength(void)
{
	register long returnValue;
	short int saveSong;

	returnValue = 0L;
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		/* ok, now service the music in progress
		*/
		if (theCurrentSong != -1)
		{
			saveSong = theCurrentSong;
			theCurrentSong = -1;
			returnValue = JX_GetSongLength();
			theCurrentSong = saveSong;
		}
	}
	return returnValue;
}

static long MDH_SetMIDIClock(long newClockPosition)
{
	return newClockPosition;
}

long SongTicks(void)
{
	register long	songTickCount;
	static long		lastSongTickCount = 0;

	songTickCount = 0L;
	if ( (gDriverType != noDriverInstalled) && (gDriverType != jxLiveInput) )
	{
		if (IsSoundMusicSystemPaused() == FALSE)
		{
			if (theCurrentSong != -1)
			{
				if (musicPaused == FALSE)
				{
					songTickCount = GetSyncTime() - startingTick;
					if (songTickCount < 0L)
					{
						songTickCount = 0L;
					}
					lastSongTickCount = songTickCount;
				}
				else
				{
					songTickCount = lastSongTickCount;
				}
			}
		}
	}
	return songTickCount;
}


short int GetSongVolume(void)
{
	long	value;

	value = 0;
	if (gDriverType != noDriverInstalled)
	{
		JX_GetSongVolume(&value);
		value = (value * FULL_VOLUME) / 127;
	}
	return value;
}

void SetEffectsVolume(short int newVolume)
{
#if CODE_BASE == USE_NATIVE
	GM_SetEffectsVolume(newVolume);
#endif
}

short int GetEffectsVolume(void)
{
#if CODE_BASE == USE_NATIVE
	return GM_GetEffectsVolume();
#else
	return FULL_VOLUME;
#endif
}

void SetSongVolume(short int newVolume)
{
	register short int theVolume;

	if (gDriverType != noDriverInstalled)
	{
// If PowerPC code, then allow overdrive of volume
#if CODE_BASE != USE_NATIVE
		if (newVolume > FULL_VOLUME)
		{
			newVolume = FULL_VOLUME;
		}
#endif
		if (newVolume < 0)
		{
			newVolume = 0;
		}
		theVolume = newVolume;
		theVolume =  (theVolume * 127) / FULL_VOLUME;
		if (theVolume < 1)
		{
			theVolume = 1;
		}
		JX_ChangeSongVolume(theVolume);
	}
}


/* Main interface into the Music Driver.
**
** NOTE:	The passed parameters are NOT registers because THINK C fucks them up using
**		macros and recursive function calls!
*/
pascal unsigned long MusicDriverHandler(short int theMessage, long theData)
{
	register long returnValue;

	if (gDriverType == noDriverInstalled)
	{
		return (long)SMS_PARAM_ERR;
	}
	returnValue = SMS_NO_ERR;
	switch (theMessage)
	{
		case kGetMidiClock:
			returnValue = MDH_GetMIDIClock();
			break;

		case kGetMidiBeat:
			returnValue = MDH_GetMIDIBeat();
			break;

		case kGetNextMidiBeat:
			returnValue = MDH_GetNextMidiBeat();
			break;

		case kGetSongLength:
			returnValue = MDH_GetSongLength();
			break;

		case kGetTempo:
			returnValue = MDH_GetCurrentTempo();
			break;

		case kSetMidiClock:
			returnValue = MDH_SetMIDIClock(theData);
			break;

		case kSetupCallbackProc:
			MDH_SetupCallbackProc((CallbackData *)theData);
			break;

		case kGetErrors:
			returnValue = MDH_GetErrors(theData);
			break;
	}
	return returnValue;
}


// Returns private driver type flags for the JX_ interface
static long PV_GetPrivateDriverType(SoundQuality quality)
{
	register long	privateDriverType;

	quality = PV_VerifyQuality(quality);
//#if USE_NEW_QUALITY_MODE == 1
#if 1 == 1
	privateDriverType = noDriverInstalled;
	if (quality == Q_DETERMINE)
	{
		privateDriverType = useBestDriver;
	}
	if ( (quality & Q_RATE_44) == Q_RATE_44)
	{
		privateDriverType = use44khzDriver;
	}
	if ( (quality & Q_RATE_22) == Q_RATE_22)
	{
		privateDriverType = use22khzDriver;
	}
	if ( (quality & Q_RATE_11) == Q_RATE_11)
	{
		privateDriverType = use11khzDriver;
	}
	if ( (quality & Q_USE_TYPE_1) == Q_USE_TYPE_1)
	{
		; // do nothing extra, drop sample
	}
	if ( (quality & Q_USE_TYPE_2) == Q_USE_TYPE_2)
	{
		privateDriverType |= use2ptIntrep;
	}
	if ( (quality & Q_USE_TYPE_3) == Q_USE_TYPE_3)
	{
		privateDriverType |= useLargeIntrep;
	}
	if ( (quality & Q_USE_16) == Q_USE_16)
	{
		privateDriverType |= use16Bit;
	}
	if ( (quality & Q_USE_STEREO) == Q_USE_STEREO)
	{
		privateDriverType |= useStereo;
	}
#else
	switch(quality)
	{
		default:
			privateDriverType = noDriverInstalled;
			break;
		case jxAnalyizeQuality:
			privateDriverType = useBestDriver;
			break;
		case jxHighQuality:
			privateDriverType = use22khzDriver;
			break;
		case jxLowQuality:
			privateDriverType = use11khzDriver;
			break;
		case jxIntrepLowQuality:
			privateDriverType = use11khzDriver | use2ptIntrep;
			break;
		case jxIntrepHighQuality:
			privateDriverType = use22khzDriver | use2ptIntrep;
			break;
		case jxIntrepBestLowQuality:
			privateDriverType = use11khzDriver | useLargeIntrep;
			break;
		case jxIntrepBestHighQuality:
			privateDriverType = use22khzDriver | useLargeIntrep;
			break;
	}
#endif
	return privateDriverType;
}

SMSErr ChangeOutputQuality(SoundQuality quality)
{
	SMSErr	returnValue;
	long		type;

	returnValue = SMS_NO_ERR;

	if ( (IsSoundMusicSystemPaused() == FALSE) )
	{
//#if USE_NEW_QUALITY_MODE == 1
#if 1 == 1
		type = PV_GetPrivateDriverType(quality);
		if (type != noDriverInstalled)
		{
			JX_SelectPlayRate(type);
		}
		else
		{
			returnValue = SMS_PARAM_ERR;
		}	
#else
		switch(PV_VerifyQuality(quality))
		{
			case noDriverInstalled:
			default:
				returnValue = SMS_PARAM_ERR;
				break;
			case jxAnalyizeQuality:
				JX_SelectPlayRate(useBestDriver);
				break;
			case jxIntrepLowQuality:
				JX_SelectPlayRate(use11khzDriver | use2ptIntrep);
				break;
			case jxIntrepHighQuality:
				JX_SelectPlayRate(use22khzDriver | use2ptIntrep);
				break;
			case jxHighQuality:
				JX_SelectPlayRate(use22khzDriver);
				break;
			case jxLowQuality:
				JX_SelectPlayRate(use11khzDriver);
				break;
			case jxIntrepBestLowQuality:
				JX_SelectPlayRate(use11khzDriver | useLargeIntrep);
				break;
			case jxIntrepBestHighQuality:
				JX_SelectPlayRate(use22khzDriver | useLargeIntrep);
				break;
		}
#endif
	}
	return returnValue;
}

SMSErr ChangeSystemVoices(short int maxSongVoices, short int maxNormalizedVoices, register short int maxEffectVoices)
{
	SetupVoices	theSetup;
	SMSErr		theErr;
	long			ticks;

	theErr = SMS_NO_ERR;
	if (gDriverType != noDriverInstalled)
	{
		if ( (maxSongVoices >= 0) && 
			(maxNormalizedVoices > 0) && 
			(maxEffectVoices >= 0) && 
			((maxEffectVoices+maxSongVoices) > 0) &&
			((maxEffectVoices+maxSongVoices) < MAX_VOICES) &&
			((maxEffectVoices+maxSongVoices) >= maxNormalizedVoices) )
		{

			EndAllSound();
			EndSong();
//			PauseSoundMusicSystem();
			ticks = TickCount() + 2;
			while (TickCount() < ticks) {};
			if (InitVoices(maxEffectVoices))
			{
//				ResumeSoundMusicSystem();

				theSetup.maxSongVoices = maxSongVoices;
				theSetup.maxNormalizedVoices = maxNormalizedVoices;
				theSetup.maxEffectVoices = maxEffectVoices;
				PV_SetLastVoices(maxSongVoices, maxNormalizedVoices, maxEffectVoices);
				JX_SetupVoices(&theSetup);
			}
			else
			{
				theErr = SMS_MEMORY_ERR;
			}
		}
		else
		{
			theErr = SMS_PARAM_ERR;
		}
	}
	else
	{
		theErr = SMS_PARAM_ERR;
	}
	return theErr;
}

#if CODE_BASE != USE_NATIVE
static void PV_68_SetAudioSyncCallback(void)
{
	long		audioSyncCallback[2];

	audioSyncCallback[0] = (long)PV_68K_ProcessExternalMIDIQueue;
	audioSyncCallback[1] = GetGlobalsRegister();
	(*theMusicDriver)(jxSetAudioSyncProc, audioSyncCallback);
}
#endif


SMSErr InitSoundMusicSystem(short int maxSongVoices, short int maxNormalizedVoices, short int maxEffectVoices,
						SoundQuality quality)
{
	SMSErr		theErr;
	SysEnvRec	theWorld;
	SetupVoices	theSetup;
	CallbackData	theCallbackData;

	if (soundMusicSysSetup)
	{
		return SMS_NOT_REENTERANT;
	}
	soundMusicSysSetup = TRUE;
	if (CodeSignature() == NULL)		/* do this just so our copyright gets linked in */
	{
		return SMS_EVAL_EXPIRED;
	}

	theErr = SysEnvirons(1, &theWorld);
	if ( (theErr != SMS_NO_ERR) && (theWorld.systemVersion < 0x0600) )
	{
		return SMS_OS_TOO_OLD;
	}

	// check to see if we're running Sound Manager 3.0 or better
	if (IsNewSoundManagerInstalled() == FALSE)
	{
		return dsOldSystem;
	}

	if ( (maxSongVoices >= 0) && 
		(maxNormalizedVoices > 0) && 
		(maxEffectVoices >= 0) && 
		((maxEffectVoices+maxSongVoices) > 0) &&
		((maxEffectVoices+maxSongVoices) < MAX_VOICES) &&
		((maxEffectVoices+maxSongVoices) >= maxNormalizedVoices) )
	{
		theErr = SMS_NO_ERR;
	}
	else
	{
		theErr = SMS_PARAM_ERR;
	}

// Check to see if the we're not using Sound Manager 3.0, then check to see if the old Sound Manager
// is busy, and fail if it is because we're either installed or its really busy and Sound Manager 2.0
// can't mix sounds.
	if (IsNewSoundManagerInstalled() == FALSE)
	{
		if (IsSoundManagerActive())
		{
			theErr = SMS_SOUND_HARDWARE_BUSY;
		}
	}

	gDriverType = noDriverInstalled;
	autoFadeEnabled = FALSE;
	if (theErr == SMS_NO_ERR)
	{
		gDriverType = PV_GetPrivateDriverType(quality);
		if (gDriverType == noDriverInstalled)
		{
			theErr = SMS_PARAM_ERR;
		}
		else
		{
			/* get driver code and lock it down. 
			*/
			theErr = PV_GetDriverCodeResource(jxDriverMIDI);
			if (theErr == SMS_NO_ERR)
			{
				// verify that this glue code matches the MDRV code resource version number
				if (JX_VerifyMDRVCode() >= MDRV_VERSION)
				{
					theCurrentSong = -1;				// no song playing
					startingTick = 0;
					songLoopFlag = FALSE;
					theFadeLevel = 256;
					musicPaused = FALSE;
					beingServed = FALSE;
					songDoneCallBackProcPtr = NULL;
					insideSongCallback = FALSE;
					theErr = InitSoundSystem(maxEffectVoices);
					if (theErr == SMS_NO_ERR)
					{
						theSetup.maxSongVoices = maxSongVoices;
						theSetup.maxNormalizedVoices = maxNormalizedVoices;
						theSetup.maxEffectVoices = maxEffectVoices;
						
						PV_SetLastVoices(maxSongVoices, maxNormalizedVoices, maxEffectVoices);

						theErr = JX_SetupVoices(&theSetup);
						if (theErr == SMS_NO_ERR)
						{
							theCallbackData.callbackProcPtr = (void *)NULL;
							theCallbackData.callbackRefNum = 0L;
							SetupCallbackVars(&theCallbackData);
		
							theErr = JX_SelectPlayRate(gDriverType);
							if (theErr != SMS_NO_ERR)
							{
								FinisSoundSystem();
								theErr = SMS_PARAM_ERR;
							}
							else
							{	// all is well
								SetSongVolume(FULL_VOLUME);
								SetReverbType(0);		// no reverb

#if CODE_BASE != USE_NATIVE
								PV_68_SetAudioSyncCallback();
#endif
							}
						}
						else
						{
							FinisSoundSystem();
							theErr = SMS_PARAM_ERR;
						}
					}
				}
				else
				{
					theErr = SMS_MDRV_GLUE_OUT_OF_SYNC;	// MDRV doesn't match this code
				}
			}
		}
	}
	return theErr;
}

void FinisSoundMusicSystem(void)
{
	if (soundMusicSysSetup)
	{
		soundMusicSysSetup = FALSE;
		CleanupExternalMidi();
		if (theMusicDriver)
		{
			FinisSoundSystem();
			musicPaused = FALSE;
			EndSong();		/* stop the music! */
			JX_FreeMusic();

			PV_FreeDriverCodeResource();
			gDriverType = noDriverInstalled;
/* Found strange bug with PurgeMem. Calling PurgeMem will corrupt the heap later...
*/
#if 0
			PurgeMem(maxSize);
			CompactMem(maxSize);
#endif
		}
	}
}

SMSErr PauseMusicOnly(void)
{
	SMSErr theErr;

	theErr = SMS_PARAM_ERR;
	if (gDriverType != noDriverInstalled)
	{
		theErr = SMS_NO_ERR;
		if (IsMusicSystemPaused() == FALSE)
		{
			QLockNotes();
			JX_Pause(FALSE);		// pause just sequencer
			musicPaused = TRUE;
			pauseTick = GetSyncTime();
		}
	}
	return theErr;
}

SMSErr ResumeMusicOnly(void)
{
	SMSErr theErr;

	theErr = SMS_PARAM_ERR;
	if (gDriverType != noDriverInstalled)
	{
		theErr = SMS_NO_ERR;
		if (IsMusicSystemPaused() != FALSE)
		{
			JX_Resume(FALSE);		// resume just sequencer
			musicPaused = FALSE;
			RestartMidiSyncTimeBase();
			startingTick += GetSyncTime() - pauseTick;
			QUnlockNotes();
		}
	}
	return theErr;
}

SMSErr PauseSoundMusicSystem(void)
{
	SMSErr theErr;

	theErr = SMS_PARAM_ERR;
	if (gDriverType != noDriverInstalled)
	{
		theErr = SMS_NO_ERR;
		if (IsSoundMusicSystemPaused() == FALSE)
		{
			PauseSoundSystem();
			QLockNotes();
			JX_Pause(TRUE);
			musicPaused = TRUE;
			pauseTick = GetSyncTime();
		}
	}
	return theErr;
}

SMSErr ResumeSoundMusicSystem(void)
{
	SMSErr theErr;

	theErr = SMS_PARAM_ERR;
	if (gDriverType != noDriverInstalled)
	{
		theErr = SMS_NO_ERR;
		if (IsNewSoundManagerInstalled() == FALSE)
		{
			if (IsSoundManagerActive())
			{
				theErr = SMS_SOUND_HARDWARE_BUSY;
			}
		}
	
		if (theErr == SMS_NO_ERR)
		{
			if (IsSoundMusicSystemPaused() != FALSE)
			{
				JX_Resume(TRUE);
				musicPaused = FALSE;
				RestartMidiSyncTimeBase();
				startingTick += GetSyncTime() - pauseTick;
				QUnlockNotes();
				ResumeSoundSystem();
			}
		}
	}
	return theErr;
}


static void AutoFadeHandler(void)
{
	theFadeCount--;
	if (fadeFlag == FADE_OUT)
	{ /* Fade out */
		if (fadeDone == FALSE)
		{
			if (theFadeCount < 0)
			{
				theFadeCount = theFadeStart;
				if (theFadeLevel < 0)
				{
					fadeDone = TRUE;
					EndSong();
					JX_StopNotes();
					JX_FadeLevel(0);
					theFadeLevel = 0;
				}
				else
				{
					theFadeLevel -= theFadeCrement;
					if (theFadeLevel >= 0)
					{
						JX_FadeLevel(theFadeLevel);
					}
				}
			}
		}
		else
		{
			autoFadeEnabled = FALSE;
			JX_FadeLevel(256);
			theFadeLevel = 256;
			EndAllSound();
		}
	}
	else
	{
		if (fadeDone == FALSE)
		{
			if (theFadeCount < 0)
			{
				theFadeCount = theFadeStart;
				if (theFadeLevel > 256)
				{
					fadeDone = TRUE;
				}
				else
				{
					theFadeLevel += theFadeCrement;
					if (theFadeLevel < 256)
					{
						JX_FadeLevel(theFadeLevel);
					}
				}
			}
		}
		else
		{
			autoFadeEnabled = FALSE;
			JX_FadeLevel(256);
			theFadeLevel = 256;
		}
	}
}

void HandleMusicVBLTask(void)
{
	if (autoFadeEnabled)
	{
		AutoFadeHandler();
	}

#if CODE_BASE != USE_NATIVE
	if ( (IsSoundMusicSystemPaused() == FALSE) && (gDriverType != jxLiveInput) )
	{
		/* ok, now test the music in progress
		*/
		if (theCurrentSong != -1)
		{
			if (JX_StatusMusic() == 0L)
			{
				if (songDoneCallBackProcPtr)
				{
					insideSongCallback = TRUE;
					(*songDoneCallBackProcPtr)(theCurrentSong);
					insideSongCallback = FALSE;
				}
				if (songLoopFlag == FALSE)
				{
					theCurrentSong = -1;
					startingTick = 0;
				}
			}
		}
	}
#endif
}

#if CODE_BASE == USE_NATIVE
static pascal void PV_HandleSongCallback(short int theSongID)
{
	if (songDoneCallBackProcPtr)
	{
		insideSongCallback = TRUE;
		(*songDoneCallBackProcPtr)(theSongID); // theCurrentSong
		insideSongCallback = FALSE;
	}
	if (songLoopFlag == FALSE)
	{
		theCurrentSong = -1;
		startingTick = 0;
	}
}
#endif

void SetSongDoneCallBack(AudioDoneProc theProcPtr)
{
	songDoneCallBackProcPtr = theProcPtr;
}


// Set cache flag and load all instruments
SMSErr CacheInstrumentsNow(Boolean cacheFlag)
{
	register long	count;
	SMSErr		theErr;
	short int		validInstruments[128], insts;

	theErr = SMS_NO_ERR;
	if (gDriverType != noDriverInstalled)
	{
		JX_SetCacheInstrumentFlag(cacheFlag);

		if (cacheFlag)
		{
			insts = GetInstrumentArray(validInstruments, 128);
			if (insts)
			{
				for (count = 0; count < insts; count++)
				{
					if (LoadInstrument(validInstruments[count]))
					{
						theErr = SMS_MEMORY_ERR;
						JX_SetCacheInstrumentFlag(FALSE);		// clear and reset cache
						break;
					}
				}
			}
		}
	}
	return theErr;
}

// Sets the cache flag and lets instruments cache on demand
SMSErr CacheInstrumentsLater(Boolean cacheFlag)
{
	if (gDriverType != noDriverInstalled)
	{
		JX_SetCacheInstrumentFlag(cacheFlag);
	}
	return SMS_NO_ERR;
}


SMSErr GetErrors(void *buffer)
{
	return (SMSErr)MusicDriverHandler(kGetErrors, (long)(buffer));
}

long GetCurrentMidiClock(void)
{
	return MusicDriverHandler(kGetMidiClock, 0L);
}

long GetCurrentMidiBeat(void)
{
	return MusicDriverHandler(kGetMidiBeat, 0L);
}

long GetNextBeatMidiClock(void)
{
	return MusicDriverHandler(kGetNextMidiBeat, 0L);
}

long GetCurrentTempo(void)
{
	return MusicDriverHandler(kGetTempo, 0L);
}

long GetSongLength(void)
{
	return MusicDriverHandler(kGetSongLength, 0L);
}

void SetCurrentMidiClock(long clock)
{
	MusicDriverHandler(kSetMidiClock, (long)(clock));
}

// Get one slice of audio data. Left and right channels. Data will be in 16 bit always
short int GetAudioFrameSlice(short int *audioFrame)
{
	register long	length;

	if (gDriverType != noDriverInstalled)
	{
		length = JX_GetAudioFrameSlice(audioFrame);
	}
	else
	{
		length = 0;
	}
	return (short)length;
}

// Given a song ID and two arrays, this will return the INST resources ID and the 'snd ' resource ID
// that are needed to load the song
SMSErr GetSongPatchList(short int theSongID, short int *pInstArray512, short int *pSndArray768)
{
	register long	count, instCount, sndCount, newCount, completeSndCount, soundID;
	SongResource **theSong;
#if CODE_BASE != USE_NATIVE
	Remap		theMap;
	long			mapCount;
#endif
	short int		completeSndArray[768];
	short int		completeInstArray[512];
	Boolean		goodSound;
	SMSErr		theErr;

	theErr = SMS_NO_ERR;
	if ( (pInstArray512) && (pSndArray768) )
	{
		for (count = 0; count < 512; count++)
		{
			pInstArray512[count] = -1;
			completeInstArray[count] = -1;
		}
		for (count = 0; count < 768; count++)
		{
			pSndArray768[count] = -1;
			completeSndArray[count] = -1;
		}
#if CODE_BASE != USE_NATIVE
		LoadSong(theSongID);	// must load before GetResource
		EndSong();
#endif
		theSong = (SongResource **)GetResource(ID_SONG, theSongID);
		if (theSong)
		{
#if CODE_BASE != USE_NATIVE
			instCount = JX_GetUsedPatchList(completeInstArray);
			if (instCount)
			{
				mapCount = (long)(**theSong).remapCount;
				for (newCount = 0; newCount < instCount; newCount++)
				{
					for (count = 0; count < mapCount; count++)
					{
						GetSongRemap(theSong, newCount, &theMap);
						if (completeInstArray[newCount] == theMap.instrumentNumber)
						{
							completeInstArray[newCount] = theMap.ResourceINSTID;
						}
					}
				}
#else

			HLock((Handle)theSong);
			instCount = GM_GetUsedPatchlist(*theSong, NULL, 0L, completeInstArray, &theErr);
			HUnlock((Handle)theSong);
			if (instCount)
			{
#endif
#if 0
// this is used to create an instrument bank
				instCount = 0;
				completeInstArray[instCount++] = 0;
				completeInstArray[instCount++] = 4;
				completeInstArray[instCount++] = 6;
				completeInstArray[instCount++] = 11;
				completeInstArray[instCount++] = 12;
				completeInstArray[instCount++] = 16;
				completeInstArray[instCount++] = 40;
				completeInstArray[instCount++] = 56;
				completeInstArray[instCount++] = 65;
				completeInstArray[instCount++] = 71;
				completeInstArray[instCount++] = 73;
				completeInstArray[instCount++] = 105;
#endif
				// remove duplicates in inst
				sndCount = 0;
				for (newCount = 0; newCount < instCount; newCount++)
				{
					goodSound = TRUE;
					soundID = completeInstArray[newCount];
					for (count = 0; count < sndCount; count++)
					{
						if (soundID == pInstArray512[count])
						{
							goodSound = FALSE;
							break;
						}
					}
					if (goodSound)
					{
						pInstArray512[sndCount++] = soundID;
					}
				}
				instCount = sndCount;
				XBubbleSortArray(pInstArray512, instCount);

				completeSndCount = 0;
				for (count = 0; count < instCount; count++)
				{
					newCount = CollectSoundsFromInstrument(pInstArray512[count], 
												&completeSndArray[completeSndCount], 128);
					completeSndCount += newCount;
				}
				
				// remove duplicates in snds
				sndCount = 0;
				for (newCount = 0; newCount < completeSndCount; newCount++)
				{
					goodSound = TRUE;
					soundID = completeSndArray[newCount];
					for (count = 0; count < sndCount; count++)
					{
						if (soundID == pSndArray768[count])
						{
							goodSound = FALSE;
							break;
						}
					}
					if (goodSound)
					{
						pSndArray768[sndCount++] = soundID;
					}
				}
				XBubbleSortArray(pSndArray768, sndCount);
			}
		}
		ReleaseResource((Handle)theSong);
	}
	return theErr;
}

void StopAllEffects(void)
{
	if (gDriverType != noDriverInstalled)
	{
		if ( (IsSoundSystemPaused() == FALSE) )
		{
			autoFadeEnabled = FALSE;
			JX_StopAllSamples();
		}
	}
}

// Given a controller and a callback function; this function will be called everytime the midi sequencer encounters
// this particular controller
void SetMidiControllerCallback(short int theController, MidiControllerProc theProc)
{
#if CODE_BASE == USE_NATIVE
	GM_SetControllerCallback(theController, theProc);
#endif
}

void SetReverbType(short int reverbType)
{
#if CODE_BASE == USE_NATIVE
	GM_SetReverbType(reverbType+1);
#endif
}

short int GetReverbType(void)
{
#if CODE_BASE == USE_NATIVE
	return GM_GetReverbType() - 1;
#endif
	return 0;
}


/* EOF of MusicSystem.c
*/
