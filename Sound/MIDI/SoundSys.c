/*****************************************************/
/*
**	SoundSys.c
**		Revision 3
**
**		Sound System.  Plays sounds staticly from pre-loaded 'snd ' resources.
**		This is low-level, just start & stop one sample, and play a list of
**		samples.
**
**		Tries to use the Sound Manager, if the system has it, otherwise
**		it goes low-level to the Sound Driver.
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**  
**
**	History	-
** 	7/20/89		Created
** 	7/24/89		Created Asyncronous Sample List Player.
** 	12/3/89		Modified all tools to work with the Sound Driver instead of the
**				Sound Manager (He got fired.)
**	12/21/89	Added PlayTheSample.
**	2/1/90		Added Sound Manager (he got hired as a consultant) and use it
**				when system 6.0 or greater is around. (Better sound.   !!?!??)
**	4/10/90		Fixed pause/resume sound to allocate/deallocate sound channel.
**	7/3/90		Added a REST mechanisim to the SampleList player
**	7/28/90		Added IsSoundListFinished() to the SampleList player
**	7/31/90		Added IsSoundFXFinished()
**	8/13/90		Moved example and StandAlone code to a seperate project.
**  12/12/90		Added GetSoundWaveform() & GetSoundLength() & GetSoundDefaultRate().
**	1/9/91		Added sound done call back function.
**	3/4/91		Completed work on Sound Driver's call back.
**	4/10/91		Fixed a bug in CalcPlaybackLength that calculated the wrong length.
**	4/10/91		Added purgability of the samples.
**	4/23/91		Added better compatibility to Think C version 3.0
**	4/30/91		Put in support for snd resource type 1 and type 2.
**	5/18/91		Changed to zone calls instead of globals.
**	5/20/91		Mucking around for bug that Mick is having.
**	5/20/91		Added csnd compressed resource based upon the LZSS algo.
**	5/25/91		Added a FinisSoundChannel/InitSoundChannel inside of EndSound() for Mick
**	5/27/91		Fixed potential bug with BeginSoundList() in which the sound may be
**				loaded during a VBL!
**	5/28/91		Modified to new Think C MPW like header names.
**	6/18/91		Added delay to the starting of a new sound with BeginSound (4 Ticks).
**	7/24/91		Added delay to the starting of a new sound with PlayTheSample (4 Ticks).
**	8/5/91		Removed duplicate code in BeginSound & PlayTheSample.
**	8/5/91		Added 2 tick delay to soundLength. Used in IsSoundFXFinished.
**	8/5/91		Added BeginSoundReverse & BeginSoundSection.
**	8/6/91		Fixed some memory management problems with SoundMemorySize.
**	8/6/91		Added a semaphore to the vbl task.
**	9/9/91		Cleaned up for Think C 5.0
**	9/9/91		Added PLAY_ALL_SAMPLE
**	9/9/91		Changed all 'int's to 'short int'. Sorry guys. This is because I want
**				16 bit integers.
**	10/8/91		Added sound looping features. BeginSoundLoop(), BeginSoundEnvelope(),
**				and BeginSoundEnvelopeProc().
**	11/11/91	Added SoundLock().
**	1/22/92		Put in buffer SndDoCommands for looped & enveloped sounds
**	3/9/92		Modifed for MPW and removed all THINK C 3 & 4 compiler stuff
**	3/23/92		Added access to"Private-Sound/Music.h"
**	5/1/92		Added the SoundChannelRestart() function
**	7/10/92		Combined SoundSys.h & MusicSys.h into new and improved version
**	7/10/92		Added new calls to initilize & clean up the system
**	7/10/92		Heavily modified for Rev 3 of the sound system.
**	7/10/92		Now calling jim's sound mixer
**	7/13/92		Added BeginMasterFadeOut & FadeLevel
**	7/18/92		Fully intergrated the SoundManager out and replaced with "SoundGod"
**	7/26/92		Added ChangeOuputQuality() and fixed bug with BeginSongLooped
**	7/31/92		Fixed weird bug with MPW passing of 32-bit value "shorts"!!
**	7/31/92		Fixed BeginMasterFadeOut code.
**	8/4/92		Change playback rate command to 16.16 fixed value. Sorry guys.
**	8/6/92		Built a mult-voice tracker, to work with sound effects
**	12/4/92		Added LoadSound
**	12/9/92		Added FreeSound
**	12/29/92	ResumeSoundMusicSystem and PauseSoundMusicSystem now return
**				errors
**	3/6/93		Fixed the SoundDone callback system.
**	3/6/93		Elimated the playback decay govenour.
**	3/16/93		Added BeginMasterFadeIn
**	3/19/93		Added SetSongDoneCallBack
**	5/31/93		Made LoadSong/FreeSong work
**	6/14/93		Changed VBL task to be out of the system heap
**	6/24/93		Added StartFilePlayback, EndFilePlayback, and ChangeFilePlaybackRate
**	6/24/93		Added IsThisSoundFXFinished
**	7/2/93		Added EndSoundList
**	7/13/93		Fixed File streaming stop bug
**	7/14/93		Worked on stopping callback bug
**	7/15/93		Think I fixed the callback bug
**	7/21/93		Forced callbacks to use a long for MPW
**	7/25/93		Fixed bug with resume while file playback is in progress
**	7/26/93		Changed all callbacks to be pascal based
**	9/2/93		Fixed macro bug with FreeSong
**	9/15/93		Fixed a looping bug with BeginSongLoop
**	9/19/93		Exposed DeltaDecompressHandle for Larry, for those who want to decompress a 'csnd' resource
**	10/27/93	Fixed bug with ReleaseRegisteredSounds that caused a crash
**	11/18/93	Added ServiceFilePlayback to the API for StartFilePlayback
**	11/18/93	Added GetSoundLoopStart & GetSoundLoopEnd
**	11/18/93	Added SetMasterVolume & GetMasterVolume
**	12/4/93		Added code/data cache flush to callback
**	12/4/93		Added GetCurrentMidiClock() & SetCurrentMidiClock()
**	1/11/94		Cleaned header up a bit.
**	2/28/94		Added PlayTheSampleWithID
**	3/15/94		Found a nasty callback problem and fixed it.
**	4/5/94		Fixed memory leak with instrument loading
**	4/29/94		Removed external div and mul function references for A4 land.
**	6/23/94		Updated to CodeWarrior
**	8/30/94		Added a nil to vbl task during clean up
**	8/30/94		Added TickCount delay during creation/delete of vbl task
**	9/7/94		Fixed looping problem in BeginSoundEvenlope
**	9/7/94		Rolled in changed from Broderbund
**	9/13/94		Moved custom vbl task to after voice processing
**	9/19/94		Fixed ReverseSound bug. Overwrite by 1
**	9/19/94		Fixed bug with RegisterSound that load duplicated sounds
**	9/20/94		Added VM support for memory allocation
**	9/20/94		Added the ability to change the number of voices for sound effects
**	10/26/94	 Added MACE compatibility for 3:1 and 6:1
**	11/30/94	Added error checking during LoadSamples
**	11/30/94	Changed the way samples are registered, should fix problems with
**				 duplicated ID.
**	12/1/94		Added SetSoundVolume
**	12/2/94		Renamed SetSoundVolume to ChangeSoundVolume
**	12/6/94		Fixed value mapping of value in ChangeSoundVolume
**	1/19/95		Removed final reference to SoundDriver
**	1/24/95		Removed SetSoundVol/GetSoundVol for PowerPC
**	3/26/95		Changed copyright notice
**	4/4/95		Added callback support for streaming audio files
**	5/2/95		Moved debug macros from here to PrivateSoundMusicSys.h
**	5/3/95		Wedged in audio streaming support
**	5/20/95		Change Exp1to3 to use StateBlockPtr
**	6/19/95		Changed SizeResource to GetResourceSizeOnDisk. Support for Uni
**				headers
**	8/14/95		Fixed bug in BeginSoundLoop(). Would only play once then stop
**	8/21/95		Changed BeginSoundList & BeginSound & LoadSound & FreeSound & BeginSoundReverse & 
**				BeginSoundSection & BeginSoundLoop & BeginSoundEnvelope & BeginSoundEnvelopeProc &
**				EndSound to return errors
**	8/21/95		Changed SetMasterVolume & GetMasterVolume to return real values between 0 & 256
**				reguardless of hardware resrictions
**	9/6/95		Changed sound done callback & loop done callback to use typedefs
**	9/15/95		Added stereo information in private sound data structure
**	10/27/95	Removed the -6/+6 nonsense from LowLevelPlaySample
**	10/30/95	Removed DecayRate stuff
**	11/13/95	Removed old file streaming API
**	11/21/95	Added AudioStreamStop to EndAllSound
**	11/27/95	Added EndSong to EndAllSound
**				Created EndSoundEffects to just end sound effects
**	11/29/95	Changed the way LowLevelPlaySample works. Now passes a structure with most of the parameters
**				Removed reference to ALL_STREAMS now calling AudioStreamStopAll()
**	11/29/95	Added PlayTheSampleWithExtras.
**	12/6/95		Added PlayTheSampleLoopedWithExtras
**	12/14/95	Added DeltaDecompressPtr
**	12/15/95	Changed memory allocation to X_API
**				Pulled out most Mac specific code
**	1/3/96		Changed OSErr to SMSErr. Happy New Year!
**				Added more errors
**	1/4/96		Added error codes to PlayTheSample, PlayTheSampleWithID, PlayTheSampleWithExtras,
**				and PlayTheSampleLoopedWithExtras
**				Added ChangeSoundReverb
**	1/18/96		Spruced up for C++ extra error checking
**	1/24/96		Moved DeltaDecompressPtr & DeltaDecompressHandle to X_API.c
**	2/7/96		Renamed DeltaDecompressPtr to DecompressSampleFormatPtr
**				Renamed DeltaDecompressHandle to DecompressSampleFormatHandle
**	2/12/96		Changed Boolean to SMSBoolean
**				Changed Ptr to void *
**	3/6/96		Changed BeginSound system to support 16 bit samples
**	4/8/96		Fixed a bug with length calculations and 16 bit samples
**				Added GetSoundChannels & GetSoundBitSize
**
*/
/*****************************************************/

#include <Memory.h>
#include <Resources.h>
#include <Sound.h>

#include "X_API.h"
#include "MacSpecificSMS.h"


/* SPECIAL FLAGS for SoundSys */

#define USE_DEBUG					0	// this is defined before the inclusion of PrivateSoundMusicSys.h
#define USE_AUDIO_STREAM			1

#include "WhoLicensed.h"
#include "PrivateSoundMusicSystem.h"

#include "SoundMusicSystem.h"
#include "PrivateAudioStream.h"

#if CODE_BASE == USE_NATIVE
	#include "GenSnd.h"
#endif



/*****************************************************/
/*
** Structure definations that are internal to the SoundSys.
*/
/*****************************************************/
#if GENERATINGPOWERPC
	#pragma options align=power
#endif
/* Structure containing a particular Sound */
struct SoundInfo
{
// These are setup during initilize time
	unsigned char		reversed;				// waveform reversed from BeginSoundReverse?

	XPTR 			theSound;
	void	*			sampleDataPtr;			// begining of sample waveform data
	long				waveLength;			// length of waveform data
	long				loopStart;				// loop start
	long				loopEnd;				// loop end
	long				theDefaultRate;			// default sample rate
	short int			theID;				// resource ID
	short int			currentVolume;			// current volume of this sample. 0 to 256
	short int			currentStereoPosition;	// current stereo position of this sample -255 to 255
	short int			bitSize;				// bit size 8 or 16
	short int			channels;				// mono or stereo (1 or 2)
	short int			reverbType;			// 0 for off 1 for on
};
typedef struct SoundInfo SoundInfo;

static short int		masterHardwareVolume;		// Master volume level
static short int		totalSounds;				// total loaded sounds
static SoundInfo 	*theSounds;				// indexed array into loaded sounds

static short int		soundSystemPaused;
static short int		soundsRegistered;
static long			soundHeartBeat;
static short int		systemVersion;

static VBLCallbackProc	vblankProcPtr;
static AudioDoneProc	soundDoneProcPtr;

/* For asyncrounous sample playing */

static SampleList	*asyncSampleList;
static SampleList	*savedSampleList;
static short int		asyncSampleIndex;
static short int		asyncSampleLength;
static short int		decayLength;
static short int		vbTaskLock;


#if CODE_BASE == USE_MIXED
// PPC using 68k driver
enum {
	upp68KCallbackProcInfo = kCStackBased
		 | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long)))
};

static void ProcessCallback(long reference);

static RoutineDescriptor	mdrvCallback = BUILD_ROUTINE_DESCRIPTOR(upp68KCallbackProcInfo, ProcessCallback);
#endif

static short int		maxVoices;
VoiceChannel 	*	theVoices = NULL;
static VoiceChannel *customCallBackVoice;

#if GENERATINGPOWERPC
	#pragma options align=reset
#endif

/* Internal routines */

static unsigned char LoadSampleIntoSlot(short int theID, SoundInfo *theSlot, unsigned char prePurge);
static void FreeSampleFromSlot(SoundInfo *theSlot);

// Functions

static SoundInfo * FindSound(short int theID)
{
	register short int theCount;
	register SoundInfo *sampleData;

	sampleData = NULL;
	if (theSounds)
	{
		for (theCount = 0; theCount < totalSounds; theCount++)
		{
			if (theSounds[theCount].theID == theID)
			{
				if (theSounds[theCount].theSound)		/* sound loaded? */
				{
					sampleData = &theSounds[theCount];
					break;
				}
				else
				{
					if (LoadSampleIntoSlot(theID, &theSounds[theCount], FALSE))	/* load now */
					{
						sampleData = &theSounds[theCount];
						break;
					}
				}
			}
		}
	}
	return(sampleData);
}



static VoiceChannel * FindFreeVoice(void)
{
	short int		theCount;
	VoiceChannel 	*pVoice, *pVoiceArray;
	char		forceNewVoice;

	forceNewVoice = FALSE;
	pVoice = NULL;
	pVoiceArray = theVoices;
	for (theCount = 0; theCount < maxVoices; theCount++)
	{
		if (pVoiceArray[theCount].playing == FALSE)
		{
			pVoice = &pVoiceArray[theCount];
			DEBUG_STR("\pFound Free voice");
			break;
		}
	}
	if (pVoice == NULL)
	{
		DEBUG_STR("\pFailed to find a free voice");
	}
	else
	{
		pVoice->playing = FALSE;
		pVoice->soundStart = 0;
	}
	return pVoice;
}

static VoiceChannel * FindVoice(short int theID)
{
	register short int		theCount;
	register VoiceChannel *	pVoice;

	pVoice = NULL;
	for (theCount = 0; theCount < maxVoices; theCount++)
	{
		if (theVoices[theCount].samplePlayData.soundID == theID)
		{
			return &theVoices[theCount];
		}
	}
	return NULL;
}

static void ClearVoice(VoiceChannel *pVoice)
{
	register char *pData;
	register long len;

	pData = (char *)pVoice;
	len = sizeof(VoiceChannel) - 1;
	while (len--)
	{
		*pData++ = 0;
	}
	pVoice->calledDoneCallBack = TRUE;
}

void EndVoice(VoiceChannel *pVoice, SMSBoolean shutSound, SMSBoolean callCallback)
{
	register SoundDoneCallbackPtr	localVoiceDoneProcPtr;
	VoiceChannel				localVoice;

	DEBUG_STR("\pEndVoice");
	if (pVoice)
	{
		if (pVoice->calledDoneCallBack == FALSE)
		{
			pVoice->calledDoneCallBack = TRUE;

			pVoice->soundLength = 0;
			pVoice->loopEnabled = FALSE;
			pVoice->playing = FALSE;
			localVoiceDoneProcPtr = pVoice->voiceDoneProcPtr;
			pVoice->voiceDoneProcPtr = NULL;
			if (shutSound)
			{
				pVoice->samplePlayData.callbackProcPtr = NULL;
				StopSoundData(&pVoice->samplePlayData);
			}

/* Must clear the voice before calling the callback, otherwise, if a new voices is added
** into this slot, we could wipe it out.
*/
			localVoice = *pVoice;
			ClearVoice(pVoice);
			if (callCallback)
			{
				DEBUG_STR("\pEndVoice:Callback");
				if (localVoiceDoneProcPtr)
				{
					localVoice.voiceDoneProcPtr = NULL;
					(*localVoiceDoneProcPtr)((long)&localVoice);
				}
			}
		}
	}
}

// Called after ProcessCallback. Used to call the user with an ID of the sample
// about to be killed. Called by EndVoice
static void CallPlaySampleCallback(long pVoice)
{
	if (soundDoneProcPtr)
	{
		(*soundDoneProcPtr)(((VoiceChannel *)pVoice)->samplePlayData.soundID & 0xFFFFL);
	}
}

// Called by all voices started with LowLevelPlaySample. Called from the Mixing engine.
static void ProcessCallback(long pV)
{
	register long		saveA5;
	register VoiceChannel *pVoice;

	pVoice = (VoiceChannel *)pV;
	saveA5 = SetGlobalsRegister(pVoice->samplePlayData.appA5);			/* set current a5 */
	EndVoice(pVoice, FALSE, TRUE);
	saveA5 = SetGlobalsRegister(saveA5);
}

void * LowLevelPlaySample(void * pSamp, short int volume, short int stereoPos,
						SampleDataInfo * pSampInfo,
						short int loopCount, SMSBoolean loop, 
						AudioLoopDoneProc loopProc,
						SoundDoneCallbackPtr soundDoneProc)
{
	register VoiceChannel	*pVoice;
	register PlaySampleData 	*pSampleData;
	SoundDoneCallbackPtr	callbackProcPtr;
	long					reference;

	pVoice = FindFreeVoice();
	if (pVoice)
	{
		if ( (pSamp) && (soundSystemPaused == FALSE) && (volume) )
		{
			if (	(pSampInfo->loopStart > pSampInfo->loopEnd) || 
				(pSampInfo->loopStart > pSampInfo->frames) || 
				(pSampInfo->loopEnd > pSampInfo->frames) )
			{
				pSampInfo->loopStart = 0;
				pSampInfo->loopEnd = 0;
			}
			if (loopCount != -1)
			{
				pVoice->soundLength = CalcPlaybackLength(pSampInfo->rate, pSampInfo->frames);
			}
			else
			{
				pVoice->soundLength = -1;
			}
			pVoice->soundStart = soundHeartBeat;

			pVoice->loopStartTick = CalcPlaybackLength(pSampInfo->rate, pSampInfo->loopStart);
			pVoice->loopEndTick = CalcPlaybackLength(pSampInfo->rate, pSampInfo->loopEnd) - pSampInfo->loopStart;

			pVoice->sampleLoopFlag = loopCount;
			pVoice->loopDoneProcPtr = loopProc;
			pVoice->voiceDoneProcPtr = soundDoneProc;
			pVoice->loopEnabled = loop;
			pVoice->calledLoopCallBack = FALSE;
			pVoice->calledDoneCallBack = FALSE;

			pSampleData = &pVoice->samplePlayData;
			pSampleData->sampleData = pSamp;
			pSampleData->sampleLength = pSampInfo->frames;
			pSampleData->sampleRate = pSampInfo->rate;
			pSampleData->startLoop = pSampInfo->loopStart;
			pSampleData->endLoop = pSampInfo->loopEnd;
#if CODE_BASE == USE_NATIVE
			pSampleData->loopDoneProcPtr = loopProc;
#else
			pSampleData->loopFlag = &pVoice->sampleLoopFlag;
#endif
			pSampleData->soundID = pSampInfo->theID;
			pSampleData->appA5 = GetGlobalsRegister();

			pSampleData->channels = pSampInfo->channels;
			pSampleData->bitSize = pSampInfo->bitSize;

			pSampleData->stereoPosition = stereoPos;
			pSampleData->sampleVolume =  (volume * 127) / FULL_VOLUME;
			if (pSampleData->sampleVolume < 1)
			{
				pSampleData->sampleVolume = 1;
			}
#if CODE_BASE == USE_MIXED
			callbackProcPtr = (SoundDoneCallbackPtr)&mdrvCallback;
#else
			callbackProcPtr = ProcessCallback;
#endif
			pSampleData->callbackProcPtr = callbackProcPtr;

			pSampleData->callbackRefNum = (long)pVoice;
#if USE_DEBUG == 3
			DPrint(drawDebug, "%lX\r", pVoice);
#endif
			reference = PlaySoundData(&pVoice->samplePlayData);
#if CODE_BASE == USE_NATIVE
			if (reference != -1)
			{
				pVoice->playing = TRUE;
			}
#else
			pVoice->playing = TRUE;
#endif
			return pVoice;
		}
	}
	return NULL;
}


/* This function will determine the length of a sample in 1/60th of a second.
**
** formula for determining the absolute playback length of the sample is:
** (length of data * 60) / sample rate
** length = (rate * ticks) / 60
** ex:
**		(15204 * 60) / 5512 = 165 1/60th of a second in length
*/
short int CalcPlaybackLength(long theRate, long theLength)
{
	short int	shortRate;

	shortRate = theRate >> 16L;
	if (shortRate)
	{
#if USE_A5 == 0
		return DivideUnsigned(MultiplyUnsigned(theLength, 60), shortRate);
#else
		return ( (short int) ((theLength * 60) / shortRate) );
#endif
	}
	else
	{
		return 0;
	}
}

// Process VBL tasks. This is the generic task handler.
void ProcessVBLTasks(void)
{
	SampleList *	theList, *saveList;
	short int		count;
	VoiceChannel *	pVoice;

	if (vbTaskLock == FALSE)
	{
		vbTaskLock = TRUE;

		if (theVoices)
		{
			pVoice = &theVoices[0];
			count = 0;
			while (count < maxVoices)
			{
				if (pVoice->playing)
				{
					if (pVoice->soundLength != -1)
					{
						if (pVoice->soundLength > 0)			/* don't go below zero */
						{
							pVoice->soundLength--;
						}
					}
#if CODE_BASE != USE_NATIVE
					if (pVoice->loopEnabled)
					{
						if (pVoice->loopDoneProcPtr)
						{
							pVoice->calledLoopCallBack = TRUE;
							pVoice->sampleLoopFlag = 
										((*pVoice->loopDoneProcPtr)(pVoice->samplePlayData.soundID)) ? 0 : -1;
						}
					}
#endif
				}
				count++;
				pVoice++;
			}
		}


		if (asyncSampleList)		/* got some Samples to play? */
		{
			if (decayLength < 0)
			{
				if (asyncSampleIndex < asyncSampleLength)
				{
					theList = &asyncSampleList[asyncSampleIndex];
					/* need to save the list, because BeginSound NILs it out to shut things down,
					** but we don't want that here, so we will restore it
					*/
					saveList = asyncSampleList;
					if (theList->theSampleID == REST_SAMPLE)
					{
						vbTaskLock = FALSE;
						EndSoundQuick(theList->theSampleID);	/* Time to rest for the duration (pepsi break!) */
						vbTaskLock = TRUE;
					}
					else
					{
						BeginSound(theList->theSampleID, theList->theRate);
					}
					if (theList->theLength == PLAY_ALL_SAMPLE)
					{
						pVoice = FindVoice(theList->theSampleID);
						if (pVoice)
						{
							decayLength = pVoice->soundLength;
						}
						else
						{
							decayLength = 0;
						}
					}
					else
					{
						decayLength = theList->theLength;
					}
					asyncSampleIndex++;
					asyncSampleList = saveList;
				}
				else
				{
					asyncSampleList = NULL;
				}
			}
			else
			{
				decayLength--;
			}
		}

		if (vblankProcPtr)
		{
			(*vblankProcPtr)();
		}
		HandleMusicVBLTask();
		soundHeartBeat++;			/* sync to the internal clock */
		vbTaskLock = FALSE;
	}
}



void SetSoundVBCallBack(VBLCallbackProc theProcPtr)
{
	vblankProcPtr = theProcPtr;
}

void SetSoundDoneCallBack(AudioDoneProc theProcPtr)
{
	register long	address;

	address = (long)theProcPtr;
	soundDoneProcPtr = 0L;
	if (address > 0x1000L)
	{
		soundDoneProcPtr = theProcPtr;
	}
}


// Load a sample into a particular slot in the database.
static unsigned char LoadSampleIntoSlot(short int theID, SoundInfo *theSlot, unsigned char prePurge)
{
	long					resourceSize;
	XPTR				xData, yData;
	char					worked;
	SampleDataInfo			sampleData;

	worked = FALSE;
	if (prePurge == FALSE)
	{
		theSlot->reversed = FALSE;

		xData = XGetAndDetachResource(ID_CSND, theID, &resourceSize);
		if (xData)
		{
			yData = DecompressSampleFormatPtr(xData, resourceSize);
			XDisposePtr(xData);
			xData = yData; 
		}
		else
		{
			xData = XGetAndDetachResource(ID_SND, theID, &resourceSize);
		}
		if (xData)
		{
			yData = XGetSamplePtrFromSnd(xData, &sampleData);
			if (yData)
			{
				xData = sampleData.pMasterPtr;
				theSlot->sampleDataPtr = yData;
				theSlot->waveLength = sampleData.frames;
				theSlot->loopStart = sampleData.loopStart;
				theSlot->loopEnd = sampleData.loopEnd;
				theSlot->theDefaultRate = sampleData.rate;
				theSlot->currentVolume = FULL_VOLUME;
				theSlot->currentStereoPosition = MIDDLE_POS;
				theSlot->bitSize = sampleData.bitSize;
				theSlot->channels = sampleData.channels;
				worked = TRUE;
			}
			else
			{
badSound:
				XDisposePtr(xData);
				xData = NULL;
			}
		}
	}
	else
	{
		// don't load sample now
		xData = NULL;
		worked = TRUE;
	}
	theSlot->theID = theID;
	theSlot->theSound = xData;
	return worked;
}


static void FreeSampleFromSlot(SoundInfo *theSlot)
{
	register XPTR xData;

	if ( (IsSoundFXFinished()) && (asyncSampleList == NULL) )	/* sound in progress? */
	{	/* no */
		xData = theSlot->theSound;
		if (xData)
		{
			XDisposePtr(xData);
		}
		theSlot->theSound = NULL;
		theSlot->sampleDataPtr = NULL;
		theSlot->reversed = FALSE;
	}
}

static SMSErr LoadSamples(short int *pSoundID, SMSBoolean prePurge)
{
	register short int *sndIndex, sndCount, count, scount;
	SMSErr theErr;

	theErr = SMS_NO_ERR;

	sndIndex = pSoundID;
	sndCount = 0;
	while (*sndIndex++ != SOUND_END)
	{
		sndCount++;
	}

	if (sndCount > 0)
	{
		theSounds = (SoundInfo *)XNewPtr((long)sndCount * sizeof(SoundInfo));
		if (theSounds != NULL)
		{
			totalSounds = 0;
			scount = 0;
			// Now walk through all sounds that wants to be registered and load them, but
			// load them if they already have been loaded.
			for (count = 0; count < sndCount; count++)
			{
				if (FindSound(pSoundID[count]) == NULL)
				{
					if (LoadSampleIntoSlot(pSoundID[count], &theSounds[scount], prePurge))
					{
						scount++;
						totalSounds = scount;
					}
					else
					{
						theErr = SMS_MEMORY_ERR;
						break;
					}
				}
				else
				{
					theErr = SMS_ID_ALREADY_REGISTERED;
					break;
				}
			}
		}
		else
		{
			theErr = SMS_MEMORY_ERR;
		}
	}
	return(theErr);
}

static void FreeSamples(void)
{
	register short int theCount;

	if (theSounds)
	{
		for (theCount = 0; theCount < totalSounds; theCount++)
		{
			FreeSampleFromSlot(&theSounds[theCount]);
		}
		XDisposePtr((void *)theSounds);
		theSounds = NULL;
	}
	soundsRegistered = FALSE;
}

static void ReverseSample(char *pSample, long length)
{
	register char *pSampleEnd;
	register char temp1, temp2;

	pSampleEnd = pSample + length - 1;
	while (pSample < pSampleEnd)
	{
		temp1 = *pSample;
		temp2 = *pSampleEnd;
		*pSampleEnd = temp1;
		*pSample = temp2;
		pSample++;
		pSampleEnd--;
	}
}

static void InitSoundChannel(VoiceChannel *pVoice)
{
	pVoice->playing = FALSE;
	pVoice->loopDoneProcPtr = NULL;
	pVoice->voiceDoneProcPtr = NULL;
}

static void FinisSoundChannel(VoiceChannel *pVoice)
{
	pVoice->playing = FALSE;
}


/* External routines
*/

/*****************************************************/
/*	RegisterSounds( array of integers );
**		This will load the 'snd ' resources that
**		have been identified by the array into memory.
**
**		static short int sounds[] = {128, 129, 130, SOUND_END};
**			RegisterSounds(sounds, FALSE);
**
**		if prePurge is FALSE, then all of the sounds are loaded, if TRUE then
**		the each individual sound will be loaded when BeginSound is called.
**
**		The SOUND_END denotes end of list.
*/
/*****************************************************/
SMSErr RegisterSounds(short int *pSoundID, SMSBoolean prePurge)
{
	if (maxVoices)
	{
		if (soundsRegistered)
		{
			ReleaseRegisteredSounds();
		}
		soundsRegistered = TRUE;
		return LoadSamples(pSoundID, prePurge);
	}
	else
	{
		return SMS_NO_ERR;
	}
}

// End sound effects only
void EndSoundEffects(void)
{
	register VoiceChannel	*pVoice;
	register long			count;
	
	for (count = 0; count < maxVoices; count++)
	{
		pVoice = &theVoices[count];
		if (IsSoundInList(pVoice->samplePlayData.soundID) == FALSE)
		{
			EndSound(pVoice->samplePlayData.soundID);
		}
	}
}

void EndAllSound(void)
{
	EndSoundList();
	EndSong();
	EndSoundEffects();
#if USE_AUDIO_STREAM
	AudioStreamStopAll();
#endif
	StopAllEffects();
}

void ReleaseRegisteredSounds(void)
{
	EndAllSound();
	FreeSamples();
}


SMSBoolean InitVoices(short int voices)
{
	register short int	theCount;
	register SMSBoolean	safe;

	safe = FALSE;
	vbTaskLock = TRUE;
	if (theVoices)
	{
		XDisposePtr((void *)theVoices);
	}
	maxVoices = voices;
	theVoices = (VoiceChannel *)XNewPtr(voices * (long)sizeof(VoiceChannel));
	if (theVoices)
	{
		for (theCount = 0; theCount < maxVoices; theCount++)
		{
			InitSoundChannel(&theVoices[theCount]);
		}
		safe = TRUE;
	}
	else
	{
		safe = FALSE;
	}
	vbTaskLock = FALSE;
	return safe;
}

SMSErr InitSoundSystem(short int voices)
{
	SysEnvRec theWorld;
	SMSErr theErr;

	theErr = SMS_NO_ERR;
	systemVersion = 0x0100;
	asyncSampleList = NULL;
	soundSystemPaused = TRUE;
	vblankProcPtr = NULL;
	soundDoneProcPtr = NULL;
	theSounds = NULL;
	soundsRegistered = FALSE;

	// Set master hardware volume
	masterHardwareVolume = XGetHardwareVolume();

	if (SysEnvirons(1, &theWorld) == SMS_NO_ERR)
	{
		systemVersion = theWorld.systemVersion;
	}

	if (systemVersion >= 0x0600)	/* system 6.0 */
	{
		if (InitVoices(voices))
		{
			vbTaskLock = TRUE;
			theErr = SetupVBLTask();
			vbTaskLock = FALSE;
			if (theErr)
			{
				XDisposePtr((void *)theVoices);
				theVoices = NULL;
			}
			else
			{
#if USE_AUDIO_STREAM
				if (AudioStreamOpen(voices) == SMS_NO_ERR)
				{
					soundSystemPaused = FALSE;
				}
				else
				{
					theErr = SMS_MEMORY_ERR;
				}
#else
				soundSystemPaused = FALSE;
#endif
			}
		}
		else
		{
			theErr = SMS_MEMORY_ERR;
		}
	}
	return theErr;
}


/*****************************************************/
/*	FinisSoundSystem( );
**		This will release all the sounds that were
**		loaded into memory from InitSoundSystem().
**
*/
/*****************************************************/
void FinisSoundSystem()
{
	short int theCount;

	soundSystemPaused = FALSE;
	EndAllSound();
	vbTaskLock = TRUE;
	CleanupVBLTask();

	for (theCount = 0; theCount < maxVoices; theCount++)
	{
		FinisSoundChannel(&theVoices[theCount]);
	}
#if USE_AUDIO_STREAM
	AudioStreamClose();
#endif
	if (theVoices)
	{
		XDisposePtr((void *)theVoices);
		theVoices = NULL;
	}
	FreeSamples();
}

/*****************************************************/
/*	PauseSoundSystem( void );
**		This will pause, thus dis-connect all interupts related to the sound system.
**		You can resume easily, by calling ResumeSoundSystem().
**
**	NOTE:	This does not release any memory.
*/
/*****************************************************/
void PauseSoundSystem()
{
	short int theCount;

	if (soundSystemPaused == FALSE)
	{
		savedSampleList = asyncSampleList;

#if USE_AUDIO_STREAM
		AudioStreamPause();
#endif

		EndSoundEffects();
		vbTaskLock = TRUE;
		CleanupVBLTask();
		soundSystemPaused = TRUE;
		for (theCount = 0; theCount < maxVoices; theCount++)
		{
			FinisSoundChannel(&theVoices[theCount]);
		}
	}
}

/*****************************************************/
/*	ResumeSoundSystem( void );
**		This resume, thus re-connecting all interupts relating to the sound
**		system.
**
*/
/*****************************************************/
void ResumeSoundSystem()
{
	short int theCount;

	if (soundSystemPaused)
	{
		soundSystemPaused = FALSE;
		asyncSampleList = savedSampleList;
		for (theCount = 0; theCount < maxVoices; theCount++)
		{
			InitSoundChannel(&theVoices[theCount]);
		}
		vbTaskLock = FALSE;
		SetupVBLTask();
#if USE_AUDIO_STREAM
		AudioStreamResume();
#endif
	}
}


/*****************************************************/
/*	SoundMemorySize( array of integers );
**		This will scan the 'snd ' resources that will be loaded into memory,
**		and count the total amount of memory that will be used by this
**		sound list.
**
**	example:
**		static short int sounds[] = {128, 129, 130, SOUND_END};
**			totalSize = SoundMemorySize(sounds);
**
**		The SOUND_END denotes end of list.
*/
/*****************************************************/
long SoundMemorySize(short int *pSoundID)
{
	register short int *sndIndex, sndCount, theCount;
	long totalSize, decompressedSize;
	register Handle hData;

	sndIndex = pSoundID;
	sndCount = 0;
	while (*sndIndex++ != SOUND_END)
	{
		sndCount++;
	}
	totalSize = (long)sndCount * sizeof(SoundInfo);

	if (sndCount > 0)
	{
		for (theCount = 0; theCount < sndCount; theCount++)
		{
			SetResLoad(TRUE);
			hData = GetResource('csnd', pSoundID[theCount]);
			if (hData == NULL)
			{
				SetResLoad(FALSE);		/* don't really load the resource, just give me their size */
				hData = GetResource('snd ', pSoundID[theCount]);
				if (hData)
				{
#ifndef __MIXEDMODE__
					decompressedSize = SizeResource(hData);
#else
					decompressedSize = GetResourceSizeOnDisk(hData);
#endif
				}
			}
			else
			{
				HLock(hData);
				BlockMoveData(*hData, &decompressedSize, (long)sizeof(long));
				HUnlock(hData);
				ReleaseResource(hData);
			}

			if (hData != NULL)
			{
				totalSize += decompressedSize;
			}
		}
		SetResLoad(TRUE);		/* ok, now really load resources */
	}
	return(totalSize);
}

void * GetSoundWaveform(short int theID)
{
	register SoundInfo * pSnd;
	register void *wavePtr;

	wavePtr = NULL;
	pSnd = FindSound(theID);
	if (pSnd)
	{
		/* Play sample */
		wavePtr = (void *)pSnd->sampleDataPtr;
	}
	return(wavePtr);
}

long GetSoundLength(short int theID)
{
	register SoundInfo * pSnd;
	register long theLength;

	theLength = 0;
	pSnd = FindSound(theID);
	if (pSnd != NULL)
	{
		theLength = pSnd->waveLength;
	}
	return theLength;
}

short int GetSoundTime(short int theID, long theRate)
{
	return (CalcPlaybackLength(theRate, GetSoundLength(theID)));
}

long GetSoundDefaultRate(short int theID)
{
	register SoundInfo * pSnd;
	register long theRate;

	theRate = 0;
	pSnd = FindSound(theID);
	if (pSnd != NULL)
	{
		theRate = pSnd->theDefaultRate;
	}
	return theRate;
}

long GetSoundLoopStart(short int theID)
{
	register SoundInfo * pSnd;
	register long loopPoint;

	loopPoint = 0;
	pSnd = FindSound(theID);
	if (pSnd != NULL)
	{
		loopPoint = pSnd->loopStart;
	}
	return loopPoint;
}

long GetSoundLoopEnd(short int theID)
{
	register SoundInfo * pSnd;
	register long loopPoint;

	loopPoint = 0;
	pSnd = FindSound(theID);
	if (pSnd != NULL)
	{
		loopPoint = pSnd->loopEnd;
	}
	return loopPoint;
}

short int GetSoundBitSize(short int theID)
{
	register SoundInfo * pSnd;
	register short int bitSize;

	bitSize = 8;
	pSnd = FindSound(theID);
	if (pSnd)
	{
		bitSize = pSnd->bitSize;
	}
	return bitSize;
}

short int GetSoundChannels(short int theID)
{
	register SoundInfo * pSnd;
	register short int channels;

	channels = 1;
	pSnd = FindSound(theID);
	if (pSnd)
	{
		channels = pSnd->channels;
	}
	return channels;
}

/*****************************************************/
/*
** FUNCTION SetMasterVolume;
**
** Overvue --
**	Sets the master hardware volume.
**
**	INPUT	--	short;	new volume level from 0 (quiet) to 256 (full)
**
**	OUTPUT	--	
**
** NOTE:	
**	This scales the volume level to match the Macintosh hardware volume
**	levels, which are from 0-7, and for the New Sound Manager the values
**	are the same.
*/
/*****************************************************/
void SetMasterVolume(short int theVolume)
{
	masterHardwareVolume = theVolume;
	XSetHardwareVolume(theVolume);
}

/*****************************************************/
/*
** FUNCTION GetMasterVolume;
**
** Overvue --
**	Returns the volume level of the Hardware from 0 to 256.
**
**	INPUT	--	
**	OUTPUT	--	short;	volume level from 0 to 256
**
** NOTE:	
**	This matches the Macintosh hardware, so the values are scaled from
**	0-7.
*/
/*****************************************************/
short int GetMasterVolume(void)
{
	if (ABS(masterHardwareVolume - XGetHardwareVolume()) > 64)
	{
		masterHardwareVolume = XGetHardwareVolume();
	}
	return masterHardwareVolume;
}


/*****************************************************/
/*	PlayTheSample(Ptr pSamp, long sampSize, short int sampRate);
**		This will play a raw sample at sampRate samples/second.
**
**		pSamp is a pointer to a buffer.
**		sampSize is the buffer size.
**		sampRate is the number of samples per second to play.
**
**		NOTE:	Reserve 6 byte from the begining of pSamp for this
**				fuctions use.  The real sample will start at pSamp+6.
**
*/
/*****************************************************/
SMSErr PlayTheSample(void * pSamp, long sampSize, long sampRate)
{
	SampleDataInfo		theSampInfo;

	theSampInfo.rate = sampRate;
	theSampInfo.frames = sampSize;
	theSampInfo.size = sampSize;
	theSampInfo.loopStart = 0;
	theSampInfo.loopEnd = 0;
	theSampInfo.bitSize = 8;
	theSampInfo.channels = 1;
	theSampInfo.theID = CUSTOM_PLAY_ID;
	theSampInfo.baseKey = 60;

	DEBUG_STR("\pPlayTheSample!");

	customCallBackVoice = (VoiceChannel *)LowLevelPlaySample((char *)pSamp+6, FULL_VOLUME, MIDDLE_POS,
									&theSampInfo,
									0, FALSE, 
									NULL, CallPlaySampleCallback);
	return (customCallBackVoice) ? SMS_NO_ERR : SMS_NO_FREE_VOICES;
}

/*****************************************************/
/*	PlayTheSampleWithID(void * pSamp, long sampSize, short int sampRate);
**		This will play a raw sample at sampRate samples/second.
**
**		pSamp is a pointer to a buffer.
**		sampSize is the buffer size.
**		sampRate is the number of samples per second to play.
**
**		NOTE:	Reserve 6 byte from the begining of pSamp for this
**				fuctions use.  The real sample will start at pSamp+6.
**
*/
/*****************************************************/
SMSErr PlayTheSampleWithID(void * pSamp, long sampSize, long sampRate, short int theID)
{
	SampleDataInfo		theSampInfo;

	theSampInfo.rate = sampRate;
	theSampInfo.frames = sampSize;
	theSampInfo.size = sampSize;
	theSampInfo.loopStart = 0;
	theSampInfo.loopEnd = 0;
	theSampInfo.bitSize = 8;
	theSampInfo.channels = 1;
	theSampInfo.theID = theID;
	theSampInfo.baseKey = 60;

	DEBUG_STR("\pPlayTheSample!");

	customCallBackVoice = (VoiceChannel *)LowLevelPlaySample((char *)pSamp+6, FULL_VOLUME, MIDDLE_POS,
									&theSampInfo,
									0, FALSE, 
									NULL, CallPlaySampleCallback);
	return (customCallBackVoice) ? SMS_NO_ERR : SMS_NO_FREE_VOICES;
}

SMSErr PlayTheSampleWithExtras(void * pSamp, long frames, long sampRate, short int theID, short int channels, short int bitSize,
								short int volume, short int stereoPos)
{
	SampleDataInfo		theSampInfo;

	theSampInfo.rate = sampRate;
	theSampInfo.frames = frames;
	theSampInfo.size = (frames * (bitSize / 8) * channels);
	theSampInfo.loopStart = 0;
	theSampInfo.loopEnd = 0;
	theSampInfo.bitSize = bitSize;
	theSampInfo.channels = channels;
	theSampInfo.theID = theID;
	theSampInfo.baseKey = 60;

	customCallBackVoice = (VoiceChannel *)LowLevelPlaySample(pSamp, volume, stereoPos,
									&theSampInfo,
									0, FALSE, 
									NULL, CallPlaySampleCallback);
	return (customCallBackVoice) ? SMS_NO_ERR : SMS_NO_FREE_VOICES;
}

SMSErr PlayTheSampleLoopedWithExtras(void * pSamp, long frames, long sampRate, short int theID, short int channels, 
								short int bitSize, short int volume, short int stereoPos,
								long loopStart, long loopEnd, AudioLoopDoneProc theLoopProc)
{
	SampleDataInfo		theSampInfo;

	theSampInfo.rate = sampRate;
	theSampInfo.frames = frames;
	theSampInfo.size = (frames * (bitSize / 8) * channels);
	theSampInfo.loopStart = loopStart;
	theSampInfo.loopEnd = loopEnd;
	theSampInfo.bitSize = bitSize;
	theSampInfo.channels = channels;
	theSampInfo.theID = theID;
	theSampInfo.baseKey = 60;

	customCallBackVoice = (VoiceChannel *)LowLevelPlaySample(pSamp, volume, stereoPos,
									&theSampInfo,
									-1, TRUE, 
									theLoopProc, CallPlaySampleCallback);
	return (customCallBackVoice) ? SMS_NO_ERR : SMS_NO_FREE_VOICES;
}



// range from -255 to 255
void ChangeSoundStereoPosition(short int theID, short int stereoPosition)
{
	register SoundInfo		*pSnd;
	register VoiceChannel	*pVoice;

	if (soundSystemPaused == FALSE)
	{
		pVoice = FindVoice(theID);
		if (pVoice)
		{
			pVoice->samplePlayData.soundID = theID;
			pVoice->samplePlayData.stereoPosition = stereoPosition / 4;
			// range from -63 to 63
			ChangeStereoPositionData(&pVoice->samplePlayData);
		}
		pSnd = FindSound(theID);
		if (pSnd)
		{
			pSnd->currentStereoPosition = stereoPosition;
		}
	}
}

void ChangeSoundPitch(short int theID, long theRate)
{
	register VoiceChannel	*pVoice;

	if (soundSystemPaused == FALSE)
	{
		pVoice = FindVoice(theID);
		if (pVoice != NULL)
		{
			pVoice->samplePlayData.soundID = theID;
			pVoice->samplePlayData.sampleRate = theRate;
			ChangePitchData(&pVoice->samplePlayData);
		}
	}
}

/*****************************************************/
/*	ChangeSoundVolume(short int theID, short theVolume)
*/
/*****************************************************/
void ChangeSoundVolume(short int theID, short int theVolume)
{
	register SoundInfo * pSnd;
	register VoiceChannel * pVoice;

	if (soundSystemPaused == FALSE)
	{
		theVolume =  (theVolume * 127) / FULL_VOLUME;
		if (theVolume < 1)
		{
			theVolume = 1;
		}
		pVoice = FindVoice(theID);
		if (pVoice)
		{
			pVoice->samplePlayData.soundID = theID;
			pVoice->samplePlayData.sampleVolume = theVolume;
			JX_ChangeSampleVolume(&pVoice->samplePlayData);
		}

		pSnd = FindSound(theID);
		if (pSnd)
		{
			pSnd->currentVolume = theVolume;
		}
	}
}

void ChangeSoundReverb(short int theID, SMSBoolean enableReverb)
{
#if CODE_BASE == USE_NATIVE
	register SoundInfo * pSnd;
	register VoiceChannel * pVoice;

	if (soundSystemPaused == FALSE)
	{
		pVoice = FindVoice(theID);
		if (pVoice)
		{
			GM_ChangeSampleReverb(pVoice->samplePlayData.callbackRefNum, enableReverb);
		}

		pSnd = FindSound(theID);
		if (pSnd)
		{
			pSnd->reverbType = enableReverb;
		}
	}
#endif
}


/*****************************************************/
/*	BeginSound(short int theID, long theRate)
**		This will play sound theID.  theID is the same
**		number as the resource ID.  theRate is a Fixed
**		type.
**
**		If a sound is currently playing, this will
**		abort it, in favor of the new sound.
*/
/*****************************************************/
SMSErr BeginSound(short int theID, long theRate)
{
	register SoundInfo	*pSnd;
	register long		theLength;
	register char		*theSample;
	SMSErr			theErr;
	SampleDataInfo		theSampInfo;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pSnd = FindSound(theID);
		if (pSnd)
		{
			theSample = (char *)pSnd->sampleDataPtr;
			theLength = pSnd->waveLength;
			if (pSnd->reversed)
			{
				ReverseSample(theSample, theLength);
				pSnd->reversed = FALSE;
			}

			if (theRate == SOUND_RATE_DEFAULT)	/* use default */
			{
				theRate = pSnd->theDefaultRate;
			}

			theSampInfo.rate = theRate;
			theSampInfo.frames = theLength;
			theSampInfo.size = (theLength * (pSnd->bitSize / 8) * pSnd->channels);
			theSampInfo.loopStart = 0;
			theSampInfo.loopEnd = 0;
			theSampInfo.bitSize = pSnd->bitSize;
			theSampInfo.channels = pSnd->channels;
			theSampInfo.theID = theID;

			if (LowLevelPlaySample(theSample, pSnd->currentVolume, pSnd->currentStereoPosition,
								&theSampInfo, 0, FALSE, NULL, CallPlaySampleCallback) == NULL)
			{
				theErr = SMS_NO_FREE_VOICES;
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}

/*****************************************************/
/*	LoadSound(short int theID)
**		This will load the sound theID. Useful for pre loading sound effects.
*/
/*****************************************************/
SMSErr LoadSound(short int theID)
{
	register SoundInfo	*pSnd;
	SMSErr			theErr;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pSnd = FindSound(theID);
		if (pSnd)
		{
			if (pSnd->reversed)
			{
				ReverseSample((char *)pSnd->sampleDataPtr, pSnd->waveLength);
				pSnd->reversed = FALSE;
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}

/*****************************************************/
/*	FreeSound(short int theID)
**		This will load the sound theID. Useful for pre loading sound effects.
*/
/*****************************************************/
SMSErr FreeSound(short int theID)
{
	register SoundInfo	*pSnd;
	SMSErr			theErr;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		EndSound(theID);		/* Stop sound before releasing! Good thing! */
		pSnd = FindSound(theID);
		if (pSnd != NULL)
		{
			FreeSampleFromSlot(pSnd);
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}


SMSErr BeginSoundReverse(short int theID, long theRate)
{
	register long		theLength;
	register char		*theSample;
	register SoundInfo	*pSnd;
	SMSErr			theErr;
	SampleDataInfo		theSampInfo;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pSnd = FindSound(theID);
		if (pSnd != NULL)
		{
			theLength = pSnd->waveLength;
			theSample = (char *)pSnd->sampleDataPtr;
			if (pSnd->reversed == FALSE)	/* reversed already? */
			{	/* no */
				ReverseSample((char *)theSample, theLength);
				pSnd->reversed = TRUE;
			}
			if (theRate == SOUND_RATE_DEFAULT)	/* use default */
			{
				theRate = pSnd->theDefaultRate;
			}

			theSampInfo.rate = theRate;
			theSampInfo.frames = theLength;
			theSampInfo.size = (theLength * (pSnd->bitSize / 8) * pSnd->channels);
			theSampInfo.loopStart = 0;
			theSampInfo.loopEnd = 0;
			theSampInfo.bitSize = pSnd->bitSize;
			theSampInfo.channels = pSnd->channels;
			theSampInfo.theID = theID;

			if (LowLevelPlaySample(theSample, pSnd->currentVolume, pSnd->currentStereoPosition,
								&theSampInfo, 0, FALSE, NULL, CallPlaySampleCallback) == NULL)
			{
				theErr = SMS_NO_FREE_VOICES;
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}

SMSErr BeginSoundSection(short int theID, long theRate, long sectionStart, long sectionEnd)
{
	register long		theLength;
	register char		*theSample;
	register SoundInfo	*pSnd;
	SMSErr			theErr;
	SampleDataInfo		theSampInfo;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pSnd = FindSound(theID);
		if (pSnd != NULL)
		{
			if (pSnd->reversed)
			{
				ReverseSample((char *)pSnd->sampleDataPtr, pSnd->waveLength);
				pSnd->reversed = FALSE;
			}
			if (theRate == SOUND_RATE_DEFAULT)	/* use default */
			{
				theRate = pSnd->theDefaultRate;
			}

			if ((sectionEnd == -1L) || (sectionEnd > pSnd->waveLength))
			{
				sectionEnd = pSnd->waveLength;
			}

			theLength = sectionEnd - sectionStart;
			theSampInfo.rate = theRate;
			theSampInfo.frames = theLength;
			theSampInfo.size = (theLength * (pSnd->bitSize / 8) * pSnd->channels);
			theSampInfo.loopStart = sectionStart;
			theSampInfo.loopEnd = sectionEnd;
			theSampInfo.bitSize = pSnd->bitSize;
			theSampInfo.channels = pSnd->channels;
			theSampInfo.theID = theID;

			theSample = (char *)pSnd->sampleDataPtr + sectionStart;
			if (LowLevelPlaySample(theSample, pSnd->currentVolume, pSnd->currentStereoPosition,
								&theSampInfo, 0, FALSE, NULL, CallPlaySampleCallback) == NULL)
			{
				theErr = SMS_NO_FREE_VOICES;
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}


SMSErr BeginSoundLoop(short int theID, long theRate, long sectionStart, long sectionEnd)
{
	register long		theLength;
	register char		*theSample;
	register SoundInfo	*pSnd;
	SMSErr			theErr;
	SampleDataInfo		theSampInfo;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pSnd = FindSound(theID);
		if (pSnd != NULL)
		{
			if (pSnd->reversed)
			{
				ReverseSample((char *)pSnd->sampleDataPtr, pSnd->waveLength);
				pSnd->reversed = FALSE;
			}
			if (theRate == SOUND_RATE_DEFAULT)	/* use default */
			{
				theRate = pSnd->theDefaultRate;
			}

			if (sectionStart == -1L)
			{
				sectionStart = 1;
			}
			if (sectionEnd == -1L)
			{
				sectionEnd = pSnd->waveLength;
			}
			if (sectionEnd > pSnd->waveLength)
			{
				sectionEnd = pSnd->waveLength;
			}

			theLength = pSnd->waveLength;
			theSample = (char *)pSnd->sampleDataPtr;
			theSampInfo.rate = theRate;
			theSampInfo.frames = theLength;
			theSampInfo.size = (theLength * (pSnd->bitSize / 8) * pSnd->channels);
			theSampInfo.loopStart = sectionStart;
			theSampInfo.loopEnd = sectionEnd;
			theSampInfo.bitSize = pSnd->bitSize;
			theSampInfo.channels = pSnd->channels;
			theSampInfo.theID = theID;
			if (LowLevelPlaySample(theSample, pSnd->currentVolume, pSnd->currentStereoPosition,
								&theSampInfo, -1, TRUE, NULL, CallPlaySampleCallback) == NULL)
			{
				theErr = SMS_NO_FREE_VOICES;
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}


SMSErr BeginSoundEnvelope(short int theID, long theRate, short int theLoopTime)
{
	register long		theLength;
	register char		*theSample;
	register SoundInfo	*pSnd;
	SMSErr			theErr;
	SampleDataInfo		theSampInfo;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pSnd = FindSound(theID);
		if (pSnd != NULL)
		{
			if (pSnd->reversed)
			{
				ReverseSample((char *)pSnd->sampleDataPtr, pSnd->waveLength);
				pSnd->reversed = FALSE;
			}
			if (theRate == SOUND_RATE_DEFAULT)	/* use default */
			{
				theRate = pSnd->theDefaultRate;
			}

			theLength = pSnd->waveLength;
			theSample = (char *)pSnd->sampleDataPtr;
			theSampInfo.rate = theRate;
			theSampInfo.frames = theLength;
			theSampInfo.size = (theLength * (pSnd->bitSize / 8) * pSnd->channels);
			theSampInfo.loopStart = pSnd->loopStart;
			theSampInfo.loopEnd = pSnd->loopEnd;
			theSampInfo.bitSize = pSnd->bitSize;
			theSampInfo.channels = pSnd->channels;
			theSampInfo.theID = theID;

			if (LowLevelPlaySample(theSample, pSnd->currentVolume, pSnd->currentStereoPosition,
								&theSampInfo, theLoopTime, TRUE, NULL, CallPlaySampleCallback) == NULL)
			{
				theErr = SMS_NO_FREE_VOICES;
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}


SMSErr BeginSoundEnvelopeProc(short int theID, long theRate, AudioLoopDoneProc theLoopProc)
{
	register long		theLength;
	register char		*theSample;
	register SoundInfo	*pSnd;
	SMSErr			theErr;
	SampleDataInfo		theSampInfo;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pSnd = FindSound(theID);
		if (pSnd != NULL)
		{
			if (pSnd->reversed)
			{
				ReverseSample((char *)pSnd->sampleDataPtr, pSnd->waveLength);
				pSnd->reversed = FALSE;
			}
			if (theRate == SOUND_RATE_DEFAULT)	/* use default */
			{
				theRate = pSnd->theDefaultRate;
			}

			theLength = pSnd->waveLength;
			theSample = (char *)pSnd->sampleDataPtr;
			theSampInfo.rate = theRate;
			theSampInfo.frames = theLength;
			theSampInfo.size = (theLength * (pSnd->bitSize / 8) * pSnd->channels);
			theSampInfo.loopStart = pSnd->loopStart;
			theSampInfo.loopEnd = pSnd->loopEnd;
			theSampInfo.bitSize = pSnd->bitSize;
			theSampInfo.channels = pSnd->channels;
			theSampInfo.theID = theID;

			if (LowLevelPlaySample(theSample, pSnd->currentVolume, pSnd->currentStereoPosition,
								&theSampInfo, -1, TRUE, theLoopProc, CallPlaySampleCallback) == NULL)
			{
				theErr = SMS_NO_FREE_VOICES;
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}


/*****************************************************/
/*	EndSound()
**		This will stop the current sound from playing.
**		Use this function to stop all sound, including sample lists.
*/
/*****************************************************/
SMSErr EndSound(short int theID)
{
	register VoiceChannel	*pVoice;
	SMSErr				theErr;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		pVoice = FindVoice(theID);
		if (pVoice)
		{
			asyncSampleList = NULL;
			if (pVoice->playing)
			{
				pVoice->samplePlayData.soundID = theID;
				pVoice->samplePlayData.appA5 = GetGlobalsRegister();
				EndVoice(pVoice, TRUE, TRUE);
			}
		}
		else
		{
			theErr = SMS_ID_NEVER_REGISTERED;
		}
	}
	return theErr;
}

/* End a sample, but don't do a call back */
void EndSoundQuick(short int theID)
{
	register VoiceChannel * pVoice;

	if (soundSystemPaused == FALSE)
	{
		pVoice = FindVoice(theID);
		if (pVoice)
		{
			pVoice->samplePlayData.soundID = theID;
			pVoice->samplePlayData.appA5 = GetGlobalsRegister();
			EndVoice(pVoice, TRUE, FALSE);
		}
	}
}

/*****************************************************/
/*	IsSoundFXFinished()
**		This will return TRUE if the sample started with BeginSound is no
**		longer playing, otherwise FALSE is returned.
*/
/*****************************************************/
SMSBoolean IsSoundFXFinished(void)
{
	register VoiceChannel * pVoice;
	SMSBoolean done;
	short int count;

	done = TRUE;
	if (soundSystemPaused == FALSE)
	{
		for (count = 0; count < maxVoices; count++)
		{
			pVoice = &theVoices[count];
			if (pVoice->playing)
			{
				done = FALSE;
				break;
			}
/*
			if (pVoice->playing)
			{
				if (IsSoundDataDone(&pVoice->samplePlayData))
				{
					done = TRUE;
				}
			}
*/		}
	}
	return done;
}

/*****************************************************/
/*
** FUNCTION IsThisSoundFXFinished(short theID);
**
** Overvue --
**	Will return TRUE if the sound identified by theID is not playing.
**
**	INPUT	--	theID,	A resource ID that has been passed into
**						RegisterSounds
**	OUTPUT	--	SMSBoolean,	TRUE if the sound is not playing;
**						FALSE if the sound is playing.
**
** NOTE:	
*/
/*****************************************************/
SMSBoolean IsThisSoundFXFinished(short theID)
{
	register VoiceChannel * pVoice;
	SMSBoolean done;

	done = TRUE;
	if (soundSystemPaused == FALSE)
	{
		pVoice = FindVoice(theID);
		if (pVoice)
		{
			if (pVoice->playing)
			{
				done = FALSE;
			}
/*
			if (IsSoundDataDone(&pVoice->samplePlayData) == FALSE)
			{
				done = FALSE;
			}
*/
		}
	}
	return done;
}

/*****************************************************/
/*	BeginSoundList(SampleList * sampleList, short int totalSamples)
**		Start asyncronsously playing the Samples contained in the SampleList
**		array.  It will stop after playing totalSamples.
**
**		If a sound is currently playing, this will
**		abort it, in favor of the new sound.
*/
/*****************************************************/
SMSErr BeginSoundList(SampleList * sampleList, short int totalSamples)
{
	register SoundInfo	*theSound;
	register short int	theCount;
	SMSErr			theErr;

	theErr = SMS_NO_ERR;
	if (soundSystemPaused == FALSE)
	{
		/* Scan through the list and make sure all the sounds are loaded into memory.
		*/
		for (theCount = 0; theCount < totalSamples; theCount++)
		{
			theSound = FindSound(sampleList[theCount].theSampleID);
			if (theSound == NULL)
			{
				theErr = SMS_ID_NEVER_REGISTERED;
				break;
			}
		}
		if (theErr == SMS_NO_ERR)
		{
			decayLength = asyncSampleIndex = 0;
			asyncSampleLength = totalSamples;
			asyncSampleList = sampleList;
		}
	}
	return theErr;
}

void EndSoundList(void)
{
	SampleList * theList, *theRealList;

	if (asyncSampleList)
	{
		theRealList = asyncSampleList;
		asyncSampleList = NULL;
		if (asyncSampleIndex < asyncSampleLength)
		{
			theList = &theRealList[asyncSampleIndex];
			if (theList->theSampleID != REST_SAMPLE)
			{
				EndSoundQuick(theList->theSampleID);
			}
			asyncSampleIndex++;
		}
	}
}

SMSBoolean IsSoundInList(short int theID)
{
	SMSBoolean	inList;
	SampleList * theList, *theRealList;
	register	long	count;

	inList = FALSE;
	theRealList = asyncSampleList;
	if (theRealList)
	{
		for (count = 0; count < asyncSampleLength; count++)
		{
			theList = &theRealList[count];
			if (theList->theSampleID == theID)
			{
				inList = TRUE;
				break;
			}
		}
	}
	return inList;
}

/*****************************************************/
/*	IsSoundListFinished()
**		This will return TRUE if the sample list started with BeginSoundList
**		is no longer playing, otherwise FALSE is returned.
*/
/*****************************************************/
SMSBoolean IsSoundListFinished(void)
{
	SMSBoolean done;

	done = FALSE;
	if (soundSystemPaused == FALSE)
	{
		if (asyncSampleList == NULL)
		{
			done = TRUE;
		}
	}
	return done;
}

/*****************************************************/
/*	PurgeAllSounds(unsigned long minMemory)
**		This will purge the sound effects currently loaded if free memory is
**		less than minMemory, otherwise nothing happens.
*/
/*****************************************************/
void PurgeAllSounds(unsigned long minMemory)
{
	register short int theCount;

	if (theSounds)
	{
		for (theCount = 0; theCount < totalSounds; theCount++)
		{
			if (FreeMem() < minMemory)
			{
				FreeSampleFromSlot(&theSounds[theCount]);
			}
		}
	}
}

SMSBoolean IsSoundSystemPaused(void)
{
	return soundSystemPaused;
}

/* EOF of SoundSys.c
*/

