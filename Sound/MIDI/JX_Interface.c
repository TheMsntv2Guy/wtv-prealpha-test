/*****************************************************/
/*
**	JX_Interface.c
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
** 
**	© Copyright 1983-1996 by Steve Hales, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
**	No portion of this program (whether in source, object, or other form)
**	may be copied, duplicated, distributed, altered, or otherwise utilized without
**	direct consent of the copyright holder.
**
** Modification History:
**
**	8/24/94		Created
**	11/7/95		Major changes, revised just about everything
**	11/11/95	Made changes to the live mode code
**	11/15/95	Added songMasterVolume
**	11/16/95	Changed default percussion for live link to off
**	12/6/95		Removed references to E_DROP_SAMPLE
**				Added reverbType setup to PV_LoadSong
**	12/11/95	Changed jxPrepSystem to not load instruments
**	12/15/95	Changed memory allocation to X_API
**	1/12/96		Returned errors from GM_BeginSong
**	1/18/96		Spruced up for C++ extra error checking
**	1/24/96		Moved song related private functions into GenSong.c
**	1/28/96		Removed all uses of Resource Manager
**
*/
/*****************************************************/

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

#include "GenSnd.h"
#include "GenPriv.h"

#define USE_PROFILER	0

#if USE_PROFILER
	#include <Profiler.h>
#endif

#if STAND_ALONE == 1
	#include <SetupA4.h>
	#include <A4Stuff.h>
#endif

#include "PrivateSoundMusicSystem.h"
#include "SoundMusicSystem.h"
#include "SoundMusicMidiSys.h"


static long			soundStarted = FALSE;
static long			loopSongFlag = FALSE;
static GM_Song *	songPlaying = NULL;
static short int		songMasterVolume = MAX_NOTE_VOLUME;

// Private Functions

static BOOL_FLAG PV_ProcessContinueLoop(INT32 ref)
{
	register BOOL_FLAG		flag;
	register VoiceChannel	*pVoice;

	flag = FALSE;
	pVoice = (VoiceChannel *)ref;
	if (pVoice)
	{
		if (pVoice->loopEnabled)
		{
			if (pVoice->loopDoneProcPtr)
			{
				pVoice->calledLoopCallBack = TRUE;
				flag = ! (*pVoice->loopDoneProcPtr)(pVoice->samplePlayData.soundID);
			}
			else
			{
				if (pVoice->sampleLoopFlag != -1)
				{
					pVoice->sampleLoopFlag--;
					if (pVoice->sampleLoopFlag > 0)
					{
						flag = TRUE;
					}
				}
				else
				{
					flag = TRUE;
				}
			}
		}
	}
	return flag;
}

void PV_AudioTask(void)
{
}


static long JX_MusicDriverInterface(long message, void *pData)
{
	SetupVoices			*pSetup;
	PlaySampleData		*pSampleData;
	long					returnValue;
	NoteInfo				*pNote;
	long					count;
	PlayMemorySongData	*pInfo;
	Quality				theQuality;
	TerpMode				theTerp;
	AudioModifiers			theMods;
	short int				theErr;
	XPTR				xSongData;

	returnValue = 0;
	switch (message)
	{
		case jxVerifyMDRVCode:
			returnValue = MDRV_VERSION;
			break;

		case jxReplaySongFlag:		//
			loopSongFlag = (long)pData;
			if (songPlaying)
			{
				GM_SetSongLoopFlag(songPlaying, loopSongFlag);
			}
			break;
		case jxPlayMemorySong:		// play song from memory
			GM_FreeSong(songPlaying);

			pInfo = (PlayMemorySongData *)pData;
			if (pInfo->theSong && pInfo->theMidi)
			{
				HLock(pInfo->theSong);
				HLock(pInfo->theMidi);
				songPlaying = GM_LoadSong(pInfo->theSongID, (SongResource *)*pInfo->theSong, *pInfo->theMidi, 
																		GetHandleSize(pInfo->theMidi), 
																		NULL, TRUE, &theErr);
				HUnlock(pInfo->theSong);
				HUnlock(pInfo->theMidi);
				if (songPlaying)
				{
					songPlaying->songVolume = songMasterVolume;
					songPlaying->songID = pInfo->theSongID;
					songPlaying->loopSong = loopSongFlag;
					returnValue = GM_BeginSong(songPlaying, NULL);
				}
				else
				{
					returnValue = theErr;
				}
			}
			else
			{
				returnValue = SMS_INVALID_REFERENCE;
			}
			break;
		case jxLoadPlaySong:		// load and start playing song
#if USE_PROFILER
			ProfilerInit(collectDetailed, microsecondsTimeBase, 500, 100);
#endif
			GM_FreeSong(songPlaying);
			count = (long)pData;

			xSongData = XGetAndDetachResource(ID_SONG, count);
			if (xSongData)
			{
				songPlaying = GM_LoadSong(count, (SongResource *)xSongData, NULL, 0L,  
										NULL, TRUE, &theErr);
				XDisposePtr(xSongData);
				// Ok, finally let's play the tune
				if (songPlaying)
				{
					songPlaying->songVolume = songMasterVolume;
					songPlaying->loopSong = loopSongFlag;
					GM_BeginSong(songPlaying, NULL);
				}
				else
				{
					returnValue = theErr;
				}
			}
			else
			{
				returnValue = SMS_NO_SONG_DATA;
			}
#if USE_PROFILER
			ProfilerDump("\pP_LoadSong.p");
			ProfilerTerm();
#endif
			break;
		case jxLoadNoPlaySong:		// load and don't play song
			GM_FreeSong(songPlaying);
			count = (long)pData;
			xSongData = XGetAndDetachResource(ID_SONG, count);
			if (xSongData)
			{
				songPlaying = GM_LoadSong(count, (SongResource *)xSongData, NULL, 0L,
										NULL, TRUE, &theErr);
				XDisposePtr(xSongData);
				if (songPlaying)
				{
					songPlaying->songVolume = songMasterVolume;
					songPlaying->loopSong = loopSongFlag;
				}
				else
				{
					returnValue = theErr;
				}
			}
			else
			{
				returnValue = SMS_NO_SONG_DATA;
			}
			break;
		case jxPlayMusic:			// play song loaded by jxLoadNoPlaySong
			if (songPlaying)
			{
				songPlaying->loopSong = loopSongFlag;
				GM_BeginSong(songPlaying, NULL);
			}
			else
			{
				returnValue = SMS_MEMORY_ERR;
			}
			break;

		case jxLoadAllInstruments:	// Load all instruments, call jxFreeSong to clean up
//			DebugStr("\pjxLoadAllInstruments");
			for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
			{
				GM_SetUsedInstrument(count, -1, TRUE);		// force a load of all keys
				GM_LoadInstrument(count);
			}
			break;
		case jxLoadThisInstrument:	//	load particular instrument into memory
			count = (long)pData;
			GM_SetUsedInstrument(count, -1, TRUE);		// force a load of all keys
			returnValue = GM_LoadInstrument(count);
			break;
		case jxUnloadThisInstrument:	//	unload particular instrument from memory
			returnValue = GM_UnloadInstrument((long)pData);
			break;
		case jxCacheInstrumentFlag:	//	set cache instrument flag
			GM_FlushInstrumentCache((BOOL_FLAG) (((long)pData) & 1L));
			break;
		case jxStatusMusic:			//
			returnValue = 0;
			if (songPlaying)
			{
				if (GM_IsSongDone())
				{
					returnValue = 0L;	// song is done
				}
				else
				{
					returnValue = -1L;	// song is still playing
				}
			}
			break;
		case jxStopMusic:			//	stop music
			GM_EndSong();
			break;
		case jxFreeSong:			// deallocate all, but not kill system
			GM_FreeSong(songPlaying);
			songPlaying = NULL;
			break;
		case jxFreeMusic:			// deallocate all and kill system
			GM_FinisGeneralSound();
			break;

		case jxSetSongCallbackProc:
			if (songPlaying)
			{
				GM_SetSongCallback(songPlaying, (GM_SongCallbackProcPtr)pData);
			}
			break;

		case jxPlayNote:			//	play a note from external midi source
			pNote = (NoteInfo *)pData;
			ServeMIDINote(pNote->program, pNote->program, pNote->pitch, pNote->velocity);
			break;
		case jxStopNote:			//	stop a note
			pNote = (NoteInfo *)pData;
			StopMIDINote(pNote->program, pNote->program, pNote->pitch);
			break;
		case jxStopNotes:			// call for live notes to stop
			GM_EndAllNotes();
			break;

		case jxPause:				// pause system or sequencer
			count = (long)pData;
			if (count)		// pause all
			{
				GM_PauseGeneralSound();
			}
			else
			{
				GM_PauseSequencer();
			}
			break;
		case jxResume:				// resume system or sequencer
			count = (long)pData;
			if (count)		// resume all
			{
				GM_ResumeGeneralSound();
			}
			else
			{
				GM_ResumeSequencer();
			}
			break;
		case jxGetErrorResults:		//
			break;
		case jxGetSoundBufferSync:	//
			if (MusicGlobals)
			{
				returnValue = MusicGlobals->syncCount;
			}
			break;
		case jxSetSoundBufferSync:	//
			if (MusicGlobals)
			{
				MusicGlobals->syncCount = (long)pData;
				MusicGlobals->syncCountFrac = 0;
			}
			break;
		case jxPlaySample:			//
			pSampleData = (PlaySampleData *)pData;
			returnValue = GM_BeginSample((unsigned char *)pSampleData->sampleData, 
									pSampleData->sampleLength, 
									pSampleData->sampleRate, 
									pSampleData->startLoop, pSampleData->endLoop, 
									pSampleData->sampleVolume, pSampleData->stereoPosition,
									pSampleData->callbackRefNum,
									pSampleData->bitSize,
									pSampleData->channels,
									PV_ProcessContinueLoop,
//									(pSampleData->loopDoneProcPtr) ? PV_ProcessContinueLoop : NULL,
									pSampleData->callbackProcPtr);
			pSampleData->callbackRefNum = returnValue;
			break;
		case jxStopSample:			//
			pSampleData = (PlaySampleData *)pData;
			GM_EndSample(pSampleData->callbackRefNum);
			break;

		case jxIsSoundDone:			//
			pSampleData = (PlaySampleData *)pData;
			returnValue = GM_IsSoundDone(pSampleData->callbackRefNum);
			break;

		case jxChangeSamplePitch:	//
			pSampleData = (PlaySampleData *)pData;
			GM_ChangeSamplePitch(pSampleData->callbackRefNum, pSampleData->sampleRate);
			break;
		case jxChangeSampleVolume:	//
			pSampleData = (PlaySampleData *)pData;
			GM_ChangeSampleVolume(pSampleData->callbackRefNum, pSampleData->sampleVolume);
			break;
		case jxChangeSampleStereoPos:
			pSampleData = (PlaySampleData *)pData;
			GM_ChangeSampleStereoPosition(pSampleData->callbackRefNum, pSampleData->stereoPosition);
			break;
		
		case jxSelectPlayRate:		//			set quality and playback rates
			returnValue = (long)pData;
			if (returnValue == useBestDriver)
			{
				theQuality = Q_22K;
				theMods = M_NONE;
#if CPU_TYPE == kRISC
				theTerp = E_LINEAR_INTERPOLATION;
				// If stereo is supported, do it
				if (PV_IsStereoSupported())
				{
					theMods |= M_USE_STEREO;
				}
#else
				theTerp = E_2_POINT_INTERPOLATION;
#endif
				// If 16 bit output is support, do it. It will be faster overall
				if (PV_Is16BitSupported())
				{
					theMods |= M_USE_16;
				}
			}
			else
			{
				if ((returnValue & 0xFFL) == use44khzDriver)
				{
					theQuality = Q_44K;
				}
				else
				{
					theQuality = ( (returnValue & 0xFFL) == use11khzDriver ) ? Q_11K : Q_22K;
				}
				theMods = M_NONE;
				if ( (returnValue & useStereo) == useStereo)
				{
					theMods |= M_USE_STEREO;
				}
				if ( (returnValue & use16Bit) == use16Bit)
				{
					theMods |= M_USE_16;
				}
#if CPU_TYPE == kRISC
				theTerp = E_AMP_SCALED_DROP_SAMPLE;
				if ( (returnValue & use2ptIntrep) == use2ptIntrep)
				{
					theTerp = E_2_POINT_INTERPOLATION;
				}
				if ( (returnValue & useLargeIntrep) == useLargeIntrep)
				{
					theTerp = E_LINEAR_INTERPOLATION;
				}
#else
				theTerp = E_AMP_SCALED_DROP_SAMPLE;
				if ( (returnValue & use2ptIntrep) == use2ptIntrep)
				{
					theTerp = E_AMP_SCALED_DROP_SAMPLE;
				}
				if ( (returnValue & useLargeIntrep) == useLargeIntrep)
				{
//					theTerp = E_2_POINT_INTERPOLATION;
					theTerp = E_AMP_SCALED_DROP_SAMPLE;
				}
#endif
			}
//			theQuality = Q_44K;
//			theTerp = E_AMP_SCALED_DROP_SAMPLE;
			returnValue = GM_ChangeAudioModes(theQuality, theTerp, theMods);
			break;
		case jxSetupVoices:			//
			pSetup = (SetupVoices *)pData;
			// If we have already setup the system, then this call will only change the voices
			if (MusicGlobals)
			{
				returnValue = GM_ChangeSystemVoices(pSetup->maxSongVoices, 
										pSetup->maxNormalizedVoices, 
										pSetup->maxEffectVoices);
			}
			else
			{
				// E_AMP_SCALED_DROP_SAMPLE
				// E_2_POINT_INTERPOLATION
				// E_LINEAR_INTERPOLATION

				theQuality = Q_22K;
//				theQuality = Q_44K;
//				theMods = M_USE_STEREO | M_USE_16;
				theMods = M_NONE;
				returnValue = GM_InitGeneralSound(theQuality, 
										E_LINEAR_INTERPOLATION, theMods,
										pSetup->maxSongVoices, 
										pSetup->maxNormalizedVoices, 
										pSetup->maxEffectVoices);
				if (returnValue)
				{
					GM_FinisGeneralSound();
				}
			}
			break;
		case jxStopAllSamples:		//
			GM_EndAllSoundEffects();
			break;

		case jxChangeSongVolume:	// change song's volume if one is loaded.
			songMasterVolume = (INT16)( ((long)pData) & 0xFFFFL);
			// convert into range 0 to 127 from 0 to 256
//			songMasterVolume = (count * MAX_NOTE_VOLUME) / MAX_MASTER_VOLUME;
			if (songPlaying)
			{
				GM_SetSongVolume(songPlaying, songMasterVolume);
			}
			break;

		case jxGetSongVolume:		// get song volume if loaded otherwise return FULL
//			returnValue = GM_GetSongVolume(songPlaying);
			// convert into range 0 to 256 from 0 to 127
//			count = (songMasterVolume * MAX_MASTER_VOLUME) / MAX_NOTE_VOLUME;
			*((INT32 *)pData) = songMasterVolume;
			break;

		case jxFade:				//
			GM_SetMasterVolume((INT32)pData);
			break;

		case jxPrepSystem:			//
		case jxEnableMIDIInput:		//
			GM_FreeSong(songPlaying);
			songPlaying = GM_CreateLiveSong(-32768);
			if (songPlaying)
			{
				if (message != jxPrepSystem)
				{
					for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
					{
						GM_SetUsedInstrument(count, -1, TRUE);		// force a load of all keys
						GM_LoadInstrument(count);
					}
				}
				songPlaying->instrumentData = MusicGlobals->InstrumentData;
				PV_ConfigureInstruments(songPlaying);
			}
			returnValue = 0L;
			break;

		case jxGetAudioFrameSlice:
			returnValue = GM_GetAudioSampleFrame((short int *)pData, &(((short *)pData)[512]));
			break;

		case jxEnableMIDIOutput:		//
		case jxSetMidiControllerProc:
		case jxSetCallbackVars:		//
		case jxGetMidiClocks:		//
		case jxGetTempo:			//
		case jxGetBeat:			//
		case jxGetSongLength:		//
		case jxGetNextBeatClock:		//
		case jxGetSongLengthBeats:	//
		case jxLoadPartialInstrument:	//
		case jxUnloadPartialInstrument: //
		case jxSetChannelPrograms:	//
		case jxSetDefaultPercProgram: //
		case jxSetAudioSyncProc:	// set audio sync proc
		case jxGetUsedPatchList:
			break;
	}
	return returnValue;
}

MDRVCodeBasePtr GetInterfaceAddress(void)
{
	return &JX_MusicDriverInterface;
}

#if STAND_ALONE == 1

Boolean IsSoundManager16Bit(void)
{
	long	feature;

	feature = 0;
	if (Gestalt(gestaltSoundAttr, &feature) == noErr)
	{
		if (feature & (1<<gestalt16BitSoundIO))
		{
			return TRUE;
		}
	}
	return FALSE;
}

extern void LZSSDeltaUncompress(void * pSource, long size, void * pDest, long* pSize);

Handle DeltaDecompressHandle(register Handle theData)
{
	long theTotalSize;
	Handle theNewData;

	HLock(theData);
	BlockMoveData(*theData, &theTotalSize, (long)sizeof(long));

	theNewData = NewHandle(theTotalSize);
	if (theNewData)
	{
		HLock(theNewData);
		LZSSDeltaUncompress((*theData) + sizeof(long), GetHandleSize(theData) - sizeof(long), 
						*theNewData, &theTotalSize);
		HUnlock(theNewData);
	}
	HUnlock(theData);

	return(theNewData);
}


long main(long message, void *pData)
{
#if !_POWERPC_
	long	saveA4;

	saveA4 = SetCurrentA4();
#endif
	message = JX_MusicDriverInterface(message, pData);
#if !_POWERPC_
	SetA4(saveA4);
#endif
	return message;
}


enum {
	uppMusicDriverProcInfo = kCStackBased
		 | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long)))
		 | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long)))
		 | RESULT_SIZE(kFourByteCode) | REGISTER_RESULT_LOCATION(kRegisterD0)
};

ProcInfoType __procinfo = uppMusicDriverProcInfo;


#endif
