// ===========================================================================
//	AudioStream.c
//
//	Portions copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

/*****************************************************/
/*
**	AudioStreams.c
**
**		This implements multi source audio streaming code.
**
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**  
**
**	History	-
** 	5/1/95		Created
**	11/20/95	Removed pragma unused
**	11/24/95	Changed PV_GetEmptyAudioStream to not stop sounds
**	11/27/95	Add an oversampling system to compensate for interopolation
**				modes that go beyond the sample length. See MAX_SAMPLE_OVERSAMPLE
**				Created AudioStreamStopAll to just end all streams
**				Fixed bug with volume on low level play. Scaled twice.
**	11/28/95	Fixed bug with AudioStream not playing a smaller buffer.
**				Fixed another (!) bug with small buffers and the second buffer.
**	11/29/95	Added IsAudioStreamValid
**				Removed ALL_STREAMS reference
**	11/30/95	Changed the audio callback to use a pointer based reference
**				instead of a user reference.
**	12/15/95	Changed memory allocation to X_API
**				Pulled out most Mac specific code
**	1/3/96		Changed OSErr to SMSErr and error codes.
**	1/7/96		Removed rescritction on 16 bit samples
**	1/18/96		Spruced up for C++ extra error checking
**	2/12/96		Changed Boolean to SMSBoolean
*/
/*****************************************************/


#include "X_API.h"
#include "MacSpecificSMS.h"
#include "MemoryManager.h"

#define USE_DEBUG			0			// this is defined before the inclusion of PrivateSoundMusicSys.h

#include "PrivateSoundMusicSystem.h"
#include "SoundMusicSystem.h"
#include "PrivateAudioStream.h"

#undef	CODE_BASE
#define	CODE_BASE	USE_NATIVE			// the other way does not build

#if CODE_BASE == USE_NATIVE
	#include "GenSnd.h"
#endif

#define AUDIO_STREAM_ID	((short int)0x7000)		// These values plus the reference value is the ID for
											// sound started with the audio stream functions

#define MAX_SAMPLE_OVERSAMPLE		4			// number of samples extra per buffer


static short int		maxStreams = 0;
static AudioStream	*theStreams = NULL;
static SMSErr		lastAudioStreamErr = noErr;


// NO AUDIO ON NON_DEDBUG BUILDS -- TALK TO ANDY RUBIN
#ifndef DEBUG
	SMSErr AudioStreamOpen(short int)
	{
		return 0;
	}
	
	SMSErr AudioStreamClose(void)
	{
		return 0;
	}
	
	SMSBoolean IsAudioStreamPlaying(long)
	{
		return false;
	}

	long AudioStreamStart(long, AudioStreamObjectProc, long , long , short int , short int )
	{
		return 0;
	}
	//
	SMSErr AudioStreamStop(long )
	{
		return 0;
	}

	void AudioStreamService(void)
	{
	}
#else

//static void *		systemTaskTrap;

#if CODE_BASE == USE_NATIVE
	static void ASMP_Callback(INT32 reference, G_PTR pWhichBufferFinished, INT32 *pBufferSize_IN_OUT);
#else
	static void ASMP_Callback(long reference);
#endif

#if CODE_BASE != USE_NATIVE
// Given an AudioStream reference, this will return a valid sound resource ID that can be used with
// all other SoundMusicSys sound functions.
static short int PV_AudioStreamConvertToID(long reference)
{
	register short int	count;

	for (count = 0; count < maxStreams; count++)
	{
		if (theStreams[count].reference == reference)
		{
			return AUDIO_STREAM_ID + count;
		}
	}
	return -1;
}
#endif

// Convert an reference to an AudioStream structure pointer
static AudioStream * PV_AudioStreamGetFromReference(register long reference)
{
	register short int	count;

	if (reference != -1)
	{
		for (count = 0; count < maxStreams; count++)
		{
			if (theStreams[count].streamActive)
			{
				if (theStreams[count].reference == reference)
				{
					return &theStreams[count];
				}
			}
		}
	}
	return NULL;
}

// Get an empty AudioStream
static long PV_GetEmptyAudioStream(void)
{
	register short int	count;
	register long		ref;

	ref = -1;
	for (count = 0; count < maxStreams; count++)
	{
		if (theStreams[count].streamActive == FALSE)
		{
			ref = count;
			theStreams[count].reference = -1;
#if CODE_BASE == USE_NATIVE
// don't do this. theStreams[count].playbackReference is most likely invalid
//			GM_EndSample(theStreams[count].playbackReference);
#else
			EndSoundQuick(PV_AudioStreamConvertToID(ref));
#endif
			break;
		}
	}
	return ref;
}

static short int PV_ScaleVolumeFrom256to127(register short largeVolume)
{
	return (largeVolume * 127) / 256;
}

static long PV_GetSampleSizeInBytes(register AudioStreamData * pAS)
{
	return pAS->channelSize * (pAS->dataBitSize / 8);
}


static void PV_FillBufferEndWithSilence(char *pDest, register AudioStreamData * pAS)
{
	register long	bufferSize, blockSize;
	register long	count;
	short int		*pWData;

	if (pDest)
	{
		blockSize = MAX_SAMPLE_OVERSAMPLE * PV_GetSampleSizeInBytes(pAS);
		bufferSize = (pAS->dataLength * PV_GetSampleSizeInBytes(pAS));

		pDest += bufferSize;
		if (pAS->dataBitSize == 8)
		{
			for (count = 0; count < blockSize; count++)
			{
				*pDest++ = 0x80;
			}
		}
		else
		{
			pWData = (short int *)pDest;
			blockSize /= 2;
			for (count = 0; count < blockSize; count++)
			{
				*pWData++ = 0;
			}
		}
	}
}

static void PV_CopyLastSamplesToFirst(char *pSource, char *pDest, register AudioStreamData * pAS)
{
	register long	bufferSize, blockSize;

	if (pAS->dataLength && pSource && pDest)
	{
		blockSize = MAX_SAMPLE_OVERSAMPLE * PV_GetSampleSizeInBytes(pAS);
		bufferSize = (pAS->dataLength * PV_GetSampleSizeInBytes(pAS));
		CopyMemory(pSource + bufferSize, pDest, blockSize);
	}
}

// Start playing buffer. Account for MAX_SAMPLE_OVERSAMPLE in length
static void PV_StartThisBufferPlaying(register AudioStream *	theStream, short int bufferNumber)
{
#if CODE_BASE == USE_NATIVE
	switch (bufferNumber)
	{
		case 1:
			if (theStream->streamLength2)
			{
				theStream->playbackReference = 
					GM_BeginDoubleBuffer(	(G_PTR)theStream->pStreamData1,
										(G_PTR)theStream->pStreamData2,
										theStream->streamLength1,
										theStream->streamData.sampleRate,
										theStream->streamData.dataBitSize, theStream->streamData.channelSize,
										PV_ScaleVolumeFrom256to127(theStream->streamVolume),
										theStream->streamStereoPosition,
										(long)theStream,
//										theStream->reference,
										ASMP_Callback);
			}
			else
			{
				theStream->streamShuttingDown = TRUE;
			}
			break;
		case 2:
			if (theStream->streamLength1)
			{
				theStream->playbackReference = 
					GM_BeginDoubleBuffer(	(G_PTR)theStream->pStreamData2,
										(G_PTR)theStream->pStreamData1,
										theStream->streamLength2,
										theStream->streamData.sampleRate,
										theStream->streamData.dataBitSize, theStream->streamData.channelSize,
										PV_ScaleVolumeFrom256to127(theStream->streamVolume),
										theStream->streamStereoPosition,
										(long)theStream,
//										theStream->reference,
										ASMP_Callback);
			}
			else
			{
				theStream->streamShuttingDown = TRUE;
			}
			break;
	}
#else
	register VoiceChannel *	pVoice;
	SampleDataInfo			theSampInfo;

	switch (bufferNumber)
	{
		case 2:
			if (theStream->streamLength2)
			{
				theSampInfo.rate = theStream->streamData.sampleRate;
				theSampInfo.frames = theStream->streamLength2;
				theSampInfo.size = PV_GetSampleSizeInBytes(&theStream->streamData) * theSampInfo.frames;
				theSampInfo.bitSize = theStream->streamData.dataBitSize;
				theSampInfo.channels = theStream->streamData.channelSize;
				theSampInfo.loopStart = 0;
				theSampInfo.loopEnd = 0;
				theSampInfo.theID = PV_AudioStreamConvertToID(theStream->reference);

				pVoice = LowLevelPlaySample((Ptr)theStream->pStreamData2, 
										theStream->streamVolume,
										theStream->streamStereoPosition,
										&theSampInfo,
										0, FALSE, 
										NULL, ASMP_Callback);
				if (pVoice)
				{
//					pVoice->user1Data = theStream->reference;
					pVoice->user1Data = (long)theStream;
				}
				theStream->playbackReference = (long)pVoice;
			}
			else
			{
				theStream->streamShuttingDown = TRUE;
			}
			break;
		case 1:
			if (theStream->streamLength1)
			{
				theSampInfo.rate = theStream->streamData.sampleRate;
				theSampInfo.frames = theStream->streamLength1;
				theSampInfo.size = PV_GetSampleSizeInBytes(&theStream->streamData) * theSampInfo.frames;
				theSampInfo.bitSize = theStream->streamData.dataBitSize;
				theSampInfo.channels = theStream->streamData.channelSize;
				theSampInfo.loopStart = 0;
				theSampInfo.loopEnd = 0;
				theSampInfo.theID = PV_AudioStreamConvertToID(theStream->reference);

				pVoice = LowLevelPlaySample((Ptr)theStream->pStreamData1, 
										theStream->streamVolume,
										theStream->streamStereoPosition,
										&theSampInfo,
										0, FALSE, 
										NULL, ASMP_Callback);
				if (pVoice)
				{
//					pVoice->user1Data = theStream->reference;
					pVoice->user1Data = (long)theStream;
				}
				theStream->playbackReference = (long)pVoice;
			}
			else
			{
				theStream->streamShuttingDown = TRUE;
			}
			break;
	}
#endif
}

/*
static pascal void ASMP_SystemTask(void)
{
	AudioStreamService();
}
*/

#if CODE_BASE == USE_NATIVE
void ASMP_Callback(INT32 reference, G_PTR pWhichBufferFinished, register INT32 *pBufferSize_IN_OUT)
{
	register AudioStream *	theStream;

//#pragma unused (pWhichBufferFinished)
	pWhichBufferFinished = pWhichBufferFinished;
//	theStream = PV_AudioStreamGetFromReference(reference);
	theStream = (AudioStream *)reference;
	if (theStream)
	{
		switch (theStream->streamMode & 0x7F)
		{
			default:
				DEBUG_STR("\pBad case in SSMP_Callback");
				break;
			case 3:
				DEBUG_STR("\pStop audio");
				theStream->streamMode = 0x80 | 4;		// end
				*pBufferSize_IN_OUT = 0;
				break;
			case 1:	// start buffer 2 playing
				DEBUG_STR("\pAS_PLAY_BUFFER_2");

				if (theStream->streamFirstTime)
				{
					// copy end of buffer 2 into the start of buffer 1
					// This only needs to happen once at the start because the buffers are different in the begining
					PV_CopyLastSamplesToFirst((char *)theStream->pStreamData2, (char *)theStream->pStreamData1, &theStream->streamData);
					theStream->streamFirstTime = FALSE;
				}

				*pBufferSize_IN_OUT = theStream->streamLength2;
				if (theStream->streamShuttingDown || (theStream->streamLength2 == 0))
				{
					theStream->streamShuttingDown = TRUE;
					DEBUG_STR("\pEnd of BUFFER_1");
					if (theStream->streamLength2)
					{
						theStream->streamMode = 0x80 | 3;		// end
					}
					else
					{
						theStream->streamMode = 0x80 | 4;		// end
					}
				}
				else
				{
					theStream->streamMode = 0x80 | 2;		// buffer1 read and playing2
				}
				break;
			case 2:
				DEBUG_STR("\pAS_PLAY_BUFFER_1");
				*pBufferSize_IN_OUT = theStream->streamLength1;
				if (theStream->streamShuttingDown || (theStream->streamLength1 == 0))
				{
					theStream->streamShuttingDown = TRUE;
					DEBUG_STR("\pEnd of BUFFER_2");
					if (theStream->streamLength1)
					{
						theStream->streamMode = 0x80 | 3;		// end
					}
					else
					{
						theStream->streamMode = 0x80 | 4;		// end
					}
				}
				else
				{
					theStream->streamMode = 0x80 | 1;		// buffer2 read and playing1
				}
				break;
		}
	}
}
#else
static void ASMP_Callback(long reference)
{
	register AudioStream *	theStream;
	register VoiceChannel *	pVoice;

	pVoice = (VoiceChannel *)reference;
//	theStream = PV_AudioStreamGetFromReference(pVoice->user1Data);
	theStream = (AudioStream *)pVoice->user1Data;
	if (theStream)
	{
		switch (theStream->streamMode & 0x7F)
		{
			default:
				DEBUG_STR("\pBad case in SSMP_Callback");
				break;
			case 3:
				DEBUG_STR("\pStop audio");
				theStream->streamMode = 0x80 | 4;		// end
				break;
			case 1:	// start buffer 2 playing
				DEBUG_STR("\pAS_PLAY_BUFFER_2");

				if (theStream->streamFirstTime)
				{
					// copy end of buffer 2 into the start of buffer 1
					// This only needs to happen once at the start because the buffers are different in the begining
					PV_CopyLastSamplesToFirst(theStream->pStreamData2, theStream->pStreamData1, &theStream->streamData);
					theStream->streamFirstTime = FALSE;
				}

				PV_StartThisBufferPlaying(theStream, 2);
				if (theStream->streamShuttingDown || (theStream->streamLength2 == 0))
				{
					theStream->streamShuttingDown = TRUE;
					DEBUG_STR("\pEnd of BUFFER_1");
					if (theStream->streamLength2)
					{
						theStream->streamMode = 0x80 | 3;		// end
					}
					else
					{
						theStream->streamMode = 0x80 | 4;		// end
					}
				}
				else
				{
					theStream->streamMode = 0x80 | 2;	// buffer1 read and playing2
				}
				break;
			case 2:
				DEBUG_STR("\pAS_PLAY_BUFFER_1");
				PV_StartThisBufferPlaying(theStream, 1);
				if (theStream->streamShuttingDown || (theStream->streamLength1 == 0))
				{
					theStream->streamShuttingDown = TRUE;
					DEBUG_STR("\pEnd of BUFFER_2");
					if (theStream->streamLength1)
					{
						theStream->streamMode = 0x80 | 3;		// end
					}
					else
					{
						theStream->streamMode = 0x80 | 4;		// end
					}
				}
				else
				{
					theStream->streamMode = 0x80 | 1;	// buffer1 read and playing2
				}
				break;
		}
	}
}
#endif

SMSErr AudioStreamOpen(short int startMaxStreams)
{
	SMSErr	theErr;

	theErr = noErr;
	maxStreams = startMaxStreams;
	theStreams = (AudioStream *)XNewPtr((long)sizeof(AudioStream) * maxStreams);
	if (theStreams == NULL)
	{
		theErr = SMS_MEMORY_ERR;
	}
//	systemTaskTrap = NGetTrapAddress(_SystemTask, ToolTrap);
//	NSetTrapAddress((UniversalProcPtr)ASMP_SystemTask, _SystemTask, ToolTrap);
	return theErr;
}

SMSErr AudioStreamClose(void)
{
	AudioStreamStopAll();
	maxStreams = 0;
	if (theStreams)
	{
		XDisposePtr((Ptr)theStreams);
		theStreams = NULL;
	}
//	NSetTrapAddress(systemTaskTrap, _SystemTask, ToolTrap);
	return noErr;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void AudioStreamPause(void)
{
	register short int		voice;
	register AudioStream *	theStream;

	if (theStreams)
	{
		for (voice = 0; voice < maxStreams; voice++)
		{
			theStream = &theStreams[voice];
			if (theStream->streamActive)
			{
				theStream->streamPaused = TRUE;
#if CODE_BASE == USE_NATIVE
				GM_EndSample(theStream->playbackReference);
#else
				EndSoundQuick(PV_AudioStreamConvertToID(theStream->reference));
#endif
			}
		}
	}
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void AudioStreamResume(void)
{
	register short int		voice;
	register AudioStream *	theStream;

	if (theStreams)
	{
		for (voice = 0; voice < maxStreams; voice++)
		{
			theStream = &theStreams[voice];
			if (theStream->streamActive)
			{
				theStream->streamPaused = FALSE;
				PV_StartThisBufferPlaying(theStream, theStream->streamMode & 0x7F);
			}
		}
	}
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
SMSErr AudioStreamError(void)
{
	return lastAudioStreamErr;
}
#endif

// This will start a streaming audio object.
//
// INPUT:
//	userReference	This is a reference value that will be returned and should be passed along to all AudioStream
//				functions.
//
//	pProc		is a AudioStreamObjectProc proc pointer. At startup of the streaming the proc will be called
//				with AS_CREATE, then followed by two AS_GET_DATA calls to get two buffers of data,
//				and finally AS_DESTROY when finished.
//
// OUTPUT:
//	long			This is an audio reference number. Will be -1 if there is an error.

long AudioStreamStart(long userReference, AudioStreamObjectProc pProc, 						
							long dataLength, 
							long sampleRate,					// Fixed 16.16
							short int dataBitSize,				// 8 or 16 bit data
							short int channelSize)				// 1 or 2 channels of date
{
	long					reference;
	register AudioStream *	theStream;
	AudioStreamData		ssData;
	short int				theErr = 0;
	long					byteLength;
	char					bufferCount;

	reference = -1;
	if ( (pProc) && ( (channelSize >= 1) || (channelSize <= 2) ) && ( (dataBitSize == 8) || (dataBitSize == 16) ) )
	{
#if CODE_BASE != USE_NATIVE
		if ( (channelSize == 2) || (dataBitSize == 16) )		// don't support this yet
		{
			lastAudioStreamErr = SMS_BAD_SAMPLE_DATA;
			return -1;
		}
#endif
		// Can't use -1 as an reference
		if (userReference == -1)
		{
			lastAudioStreamErr = SMS_INVALID_REFERENCE;
			return -1;
		}
		reference = PV_GetEmptyAudioStream();
		if (reference != -1)
		{
			theStream = &theStreams[reference];
			theStream->streamCallback = pProc;
			theStream->reference = userReference;
			theStream->streamShuttingDown = FALSE;
			theStream->streamVolume = FULL_VOLUME;
			theStream->streamStereoPosition = MIDDLE_POS;
			ssData.pData = NULL;
			ssData.asReference = theStream->reference;
			ssData.sampleRate = sampleRate;
			ssData.dataLength = dataLength;
			ssData.dataBitSize = dataBitSize;
			ssData.channelSize = channelSize;
			theErr = (*pProc)(AS_CREATE, &ssData);
			if (theErr == SMS_NO_ERR)
			{
				theStream->streamData = ssData;
				theStream->pStreamBuffer = ssData.pData;
				theStream->streamBufferLength = ssData.dataLength;
				theStream->pStreamData1 = theStream->streamData.pData;

				byteLength = ssData.dataLength * PV_GetSampleSizeInBytes(&ssData);
				theStream->pStreamData2 = (char *)theStream->streamData.pData + (byteLength / 2);
				theStream->streamLength1 = ssData.dataLength / 2;
				theStream->streamLength2 = ssData.dataLength / 2;

				theStream->streamOrgLength1 = theStream->streamLength1;
				theStream->streamOrgLength2 = theStream->streamLength2;
				theStream->streamMode = 0;

				// ok, fill first buffer
				ssData.asReference = theStream->reference;
				ssData.pData = theStream->pStreamData1;
				// get the full amount this buffer only
				ssData.dataLength = theStream->streamLength1;
				ssData.dataBitSize = dataBitSize;
				ssData.channelSize = channelSize;
				ssData.sampleRate = sampleRate;
				bufferCount = 1;
				theErr = (*pProc)(AS_GET_DATA, &ssData);
				if ( (theErr == SMS_NO_ERR) || (theErr == SMS_STREAM_STOP_PLAY) )
				{
					theStream->streamLength1 = ssData.dataLength;			// just in case it changes
					if (theErr == SMS_STREAM_STOP_PLAY)
					{
						theStream->streamShuttingDown = TRUE;
					}
					else
					{
						ssData.dataLength -= MAX_SAMPLE_OVERSAMPLE;
						if (ssData.dataLength < 0)
						{
							ssData.dataLength += MAX_SAMPLE_OVERSAMPLE;		// going to click for sure
						}
						// copy end of buffer 1 into the start of buffer 2
						PV_CopyLastSamplesToFirst((char *)theStream->pStreamData1, (char *)theStream->pStreamData2, &ssData);
	
						// ok, now fill second buffer
						ssData.asReference = theStream->reference;
	
						// now, push second pointer out for oversampling, and get fewer bytes for this buffer
						ssData.pData = (char *)theStream->pStreamData2 + (PV_GetSampleSizeInBytes(&ssData) * MAX_SAMPLE_OVERSAMPLE);
						ssData.dataLength = theStream->streamLength2 - MAX_SAMPLE_OVERSAMPLE;
	
						ssData.dataBitSize = dataBitSize;
						ssData.channelSize = channelSize;
						ssData.sampleRate = sampleRate;
						theErr = (*pProc)(AS_GET_DATA, &ssData);
						theStream->streamLength2 = ssData.dataLength;			// just in case it changes
						bufferCount = 2;
					}

					// Ok, start the sample playback
					theStream->streamMode = 1;
					theStream->streamData = ssData;
					theStream->streamFirstTime = TRUE;
					PV_StartThisBufferPlaying(theStream, 1);
					theStream->streamActive = TRUE;
					theStream->streamPaused = FALSE;
					reference = theStream->reference;
					if (theErr == SMS_STREAM_STOP_PLAY)
					{
						theStream->streamShuttingDown = TRUE;
						if (bufferCount == 1)
						{
							theStream->streamLength2 = 0;
						}
					}
				}
				if ( (theErr != SMS_NO_ERR) && (theErr != SMS_STREAM_STOP_PLAY) )
				{	// we've got to dispose of the data now
					ssData.asReference = theStream->reference;
					ssData.pData = theStream->pStreamBuffer;
					ssData.dataLength = theStream->streamBufferLength;
					ssData.sampleRate = sampleRate;
					ssData.dataBitSize = dataBitSize;
					ssData.channelSize = channelSize;
					theStream->streamCallback = NULL;
					(*pProc)(AS_DESTROY, &ssData);
				}
			}
		}
	}
	else
	{
		theErr = SMS_NO_FREE_VOICES;
	}
	lastAudioStreamErr = theErr;
	return reference;
}

void AudioStreamStopAll(void)
{
	register short int		voice;
	register AudioStream *	theStream;

	if (theStreams)
	{
		for (voice = 0; voice < maxStreams; voice++)
		{
			theStream = &theStreams[voice];
			if (theStream->streamActive)
			{
				AudioStreamStop(theStream->reference);
			}
		}
	}
}

// This will stop a streaming audio object.
//
// INPUT:
//	asReference	This is the reference number returned from AudioStreamStart.
//
SMSErr AudioStreamStop(long asReference)
{
	register AudioStream *	theStream;
	AudioStreamData		ssData;
	short int				theErr;

	theStream = PV_AudioStreamGetFromReference(asReference);
	if (theStream)
	{
		if (theStream->streamActive)
		{
			theStream->streamLength1 = 0;
			theStream->streamLength2 = 0;		// don't play next buffer.
#if CODE_BASE == USE_NATIVE
			GM_EndSample(theStream->playbackReference);
#else
			EndSoundQuick(PV_AudioStreamConvertToID(theStream->reference));
#endif
			theStream->streamActive = FALSE;
			theStream->streamShuttingDown = FALSE;
		}
		if (theStream->streamCallback)
		{
			ssData = theStream->streamData;
	
			ssData.asReference = theStream->reference;
			ssData.pData = theStream->pStreamBuffer;
			ssData.dataLength = theStream->streamBufferLength;
			theErr = (*theStream->streamCallback)(AS_DESTROY, &ssData);
			theStream->streamCallback = NULL;
			theStream->reference = -1;
		}
	}
	return noErr;
}

// Set the stereo position of a audio stream
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void AudioStreamSetStereoPosition(long reference, short int stereoPosition)
{
	register AudioStream *	theStream;

	theStream = PV_AudioStreamGetFromReference(reference);
	if (theStream)
	{
		theStream->streamStereoPosition = stereoPosition;
#if CODE_BASE == USE_NATIVE
		GM_ChangeSampleStereoPosition(theStream->playbackReference, stereoPosition / 4);
#else
		ChangeSoundStereoPosition(PV_AudioStreamConvertToID(reference), stereoPosition);
#endif
	}
}
#endif

// Set the volume level of a audio stream
#ifdef REALAUDIO
void	AudioStreamSetVolume(long reference, short int newVolume)
{
	register AudioStream *	theStream;

	theStream = PV_AudioStreamGetFromReference(reference);
	if (theStream)
	{
		theStream->streamVolume = newVolume;
#if CODE_BASE == USE_NATIVE
		GM_ChangeSampleVolume(theStream->playbackReference, PV_ScaleVolumeFrom256to127(newVolume));
#else
		ChangeSoundVolume(PV_AudioStreamConvertToID(reference), newVolume);
#endif
	}
}
#endif

// Enable/Disable reverb on this particular audio stream
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void	AudioStreamReverb(long reference, SMSBoolean enableReverb)
{
#if CODE_BASE == USE_NATIVE
	register AudioStream *	theStream;

	theStream = PV_AudioStreamGetFromReference(reference);
	if (theStream)
	{
		GM_ChangeSampleReverb(theStream->playbackReference, enableReverb);
	}
#endif
}
#endif

// Set the sample rate of a audio stream
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void AudioStreamSetRate(long reference, unsigned long newRate)
{
	register AudioStream *	theStream;

	theStream = PV_AudioStreamGetFromReference(reference);
	if (theStream)
	{
		theStream->streamData.sampleRate = newRate;
#if CODE_BASE == USE_NATIVE
		GM_ChangeSamplePitch(theStream->playbackReference, newRate);
#else
		ChangeSoundPitch(PV_AudioStreamConvertToID(reference), newRate);
#endif
	}
}
#endif

// Returns TRUE or FALSE if a given AudioStream is still active
SMSBoolean IsAudioStreamPlaying(long reference)
{
	register AudioStream *	theStream;
	register SMSBoolean		active;

	active = FALSE;
	theStream = PV_AudioStreamGetFromReference(reference);
	if (theStream)
	{
		if (theStream->streamActive)
		{
			active = TRUE;
		}
	}
	return active;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
SMSBoolean IsAudioStreamValid(long reference)
{
	return (PV_AudioStreamGetFromReference(reference)) ? TRUE : FALSE;
}
#endif

// This is the streaming audio service routine. Call this as much as possible, but not during an
// interrupt. This is a very quick routine. A good place to call this is in your main event loop.
void AudioStreamService(void)
{
	register short int		voice;
	register AudioStream *	theStream;
	AudioStreamData		ssData;
	AudioStreamObjectProc	theProc;
	SMSBoolean				done;

	if (theStreams)
	{
		for (voice = 0; voice < maxStreams; voice++)
		{
			theStream = &theStreams[voice];
			if ((theStream->streamActive) && (theStream->streamPaused == FALSE))
			{
#if CODE_BASE == USE_NATIVE
				done = GM_IsSoundDone(theStream->playbackReference);
#else
				done = IsThisSoundFXFinished(PV_AudioStreamConvertToID(theStream->reference));
#endif
				// voice has shutdown, either from a voice setup change or some other problem, restart
				// the stream from the last place that we're aware of.
				if ( (done) && (theStream->streamShuttingDown == FALSE) )
				{
					PV_StartThisBufferPlaying(theStream, theStream->streamMode & 0x7F);
				}
			}

			if ( (theStream->streamActive) && (theStream->streamMode & 0x80) && (theStream->streamPaused == FALSE) )
			{
				theStream->streamMode &= 0x7F;
				theProc = theStream->streamCallback;
				if (theProc)
				{
					ssData = theStream->streamData;
					switch (theStream->streamMode)
					{
						default:
							DEBUG_STR("\pBad case in AudioStreamService");
							break;
						case 4:
							DEBUG_STR("\pAudioStreamStop");
							AudioStreamStop(theStream->reference);
							break;
						case 3:
							break;
						case 2:		// read buffer 1 into memory
							DEBUG_STR("\pAS_READ_BUFFER_1");
							if (theStream->streamShuttingDown == FALSE)
							{
								ssData.dataLength = theStream->streamOrgLength1 - MAX_SAMPLE_OVERSAMPLE;
								ssData.pData = (char *)theStream->pStreamData1 + (PV_GetSampleSizeInBytes(&ssData) * MAX_SAMPLE_OVERSAMPLE);
								ssData.asReference = theStream->reference;
								if ((*theProc)(AS_GET_DATA, &ssData) != SMS_NO_ERR)
								{
									DEBUG_STR("\pSTOP!");
									theStream->streamShuttingDown = TRUE;
									theStream->streamLength2 = 0;
									PV_FillBufferEndWithSilence((char *)ssData.pData, &ssData);
								}
								theStream->streamLength1 = ssData.dataLength;			// just in case it changes
								// copy end of buffer 1 into the start of buffer 2
								PV_CopyLastSamplesToFirst((char *)theStream->pStreamData1, (char *)theStream->pStreamData2, &ssData);
							}
							break;
						case 1:		// read buffer 2 into memory
							DEBUG_STR("\pAS_READ_BUFFER_2");
							if (theStream->streamShuttingDown == FALSE)
							{
								ssData.dataLength = theStream->streamOrgLength2 - MAX_SAMPLE_OVERSAMPLE;
								ssData.pData = (char *)theStream->pStreamData2 + (PV_GetSampleSizeInBytes(&ssData) * MAX_SAMPLE_OVERSAMPLE);
								ssData.asReference = theStream->reference;
								if ((*theProc)(AS_GET_DATA, &ssData) != SMS_NO_ERR)
								{
									DEBUG_STR("\pSTOP!");
									theStream->streamShuttingDown = TRUE;
									theStream->streamLength1 = 0;
									PV_FillBufferEndWithSilence((char *)ssData.pData, &ssData);
								}
								theStream->streamLength2 = ssData.dataLength;
								// copy end of buffer 2 into the start of buffer 1
								PV_CopyLastSamplesToFirst((char *)theStream->pStreamData2, (char *)theStream->pStreamData1, &ssData);
							}
							break;
					}
				}
			}
		}
	}
}

#endif // DEBUG


// EOF of AudioStream.c

