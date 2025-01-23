
/*****************************************************/
/*
**	X_API.h
**
**		This provides for platform specfic functions that need to be rewitten,
**		but will provide a API that is stable across all platforms
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**
**	History	-
**	9/25/95		Created
**	10/15/95	Added XFiles
**	10/20/95	Added XMemory support
**	2/11/96		Added X_WEBTV system
**				Added XSound Manager stuff
**	2/17/96		Added more platform compile arounds
**	2/21/96		Changed XGetAndDetachResource to return a size
**	3/29/96		Added XPutLong & XPutShort
*/
/*****************************************************/
#define X_MACINTOSH	0		// MacOS
#define X_WIN95		1		// Windows 95 OS
#define X_WEBTV		2		// WebTV OS

// ¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥
// Make sure you set the X_HARDWARE_PLATFORM define correctly. Everything depends upon this flag being setup correctly.
//
//
#ifdef SIMULATOR
#define X_HARDWARE_PLATFORM	X_MACINTOSH
#else
#define X_HARDWARE_PLATFORM	X_WEBTV
#endif


#if X_HARDWARE_PLATFORM == X_WEBTV
#ifndef Ptr
typedef char *		Ptr;
#endif
typedef short		OSErr;
#define __attribute__ (x)
#define pascal
#endif
typedef unsigned char Byte;

#ifndef noErr
#define noErr 0
#endif




/*****************************************************/
/*
**	SoundMusicSys.h
**
**		Structures for handling the sounds used by system
**
**  		(c) 1989-1996 by Steve Hales, All Rights Reserved
**  
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
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
**	11/11/91	 Added SoundLock().
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
**	8/6/92		Built a mult-voice tracker, to work with sound effects.
**	8/23/92		Added 2 pt intrepolation	
**	8/24/92		Added ChangeSystemVoices
**	12/4/92		Added LoadSound
**	12/9/92		Added FreeSound
**	12/29/92	 ResumeSoundMusicSystem and PauseSoundMusicSystem now return
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
**	9/2/94		Added mixedmode manager support
**	12/2/94		Added ChangeSoundVolume and ChangeFilePlaybackVolume
**	4/4/95		Added callback support for streaming audio files, and SetFilePlaybackLoop
**	5/2/95		Added audio streaming interface for multi-source audio mixing
**	5/3/95		Removed the MusicDriverHandler interface
**	5/4/95		Changed AudioStream API to work better with OOP practices (Thanks Bob H.)
**	8/12/95		Changed ChangeOuputQuality to ChangeOutputQuality
**	8/14/95		Fixed bug in BeginSoundLoop(). Would only play once then stop
**	8/21/95		Added CacheInstrumentsNow & CacheInstrumentsLater
**				Changed BeginSoundList & BeginSound & LoadSound & FreeSound & BeginSoundReverse & 
**				BeginSoundSection & BeginSoundLoop & BeginSoundEnvelope & BeginSoundEnvelopeProc &
**				EndSound to return errors
**				Changed SetMasterVolume & GetMasterVolume to return real values between 0 & 256
**				reguardless of hardware resrictions
**				Removed PurgeSongs & LockSongs. Use CacheInstrumentsNow & CacheInstrumentsLater &
**				LoadSong to achieve the desired effect.
**	9/15/95		Added API for ChangeSoundStereoPosition.
**	9/18/95		Added GetSongPatchList
**	9/19/95		Added AudioStreamSetVolume & AudioStreamSetStereoPosition
**	9/21/95		Added IsNewSoundManagerInstalled & IsSoundManagerStereo & 
**				IsSoundManager16Bit functions.
**	9/22/95		Typedefs callback functions
**				Added SetMidiControllerCallback (PowerPC only)
**	9/23/95		Added new Quality modes for support for 44k, stereo, and 16 bit
**	10/1/95		Added GetAudioFrameSlice
**	10/2/95		Removed all reference to old quality modes. No more ifdef
**				Changed private reference of _POWERPC_ to GENERATINGPOWERPC.
**				Changed SoundQuality to an unsigned long. Thanks Mike H.
**	10/30/95	Removed the function AudioStreamConvertToID. Needed to for codebase
**				reasons. There should be an equivalent API for anything releated to streams.
**	10/31/95	Added to AudioStreamStop. Pass ALL_STREAMS to stop all streams.
**				Created a safehouse for Init & finis. Cannot call InitSoundMusicSystem and 
**				FinisSoundMusicSystem. An error will be returned.
**	11/7/95		Fixed bug with looping songs. Wouldn't stop after one loop
**	11/12/95 	Added SMS Error codes. Not used yet, but preparing for crossplatform
**	11/13/95 	Removed StartFilePlayback (old file streaming API)
**	11/21/95 	Added AudioStreamStop to EndAllSound
**	11/27/95 	Added EndSong to EndAllSound
**			 	Created EndSoundEffects to just end sound effects
**			 	Created AudioStreamStopAll to just end all streams
**				Created IsSoundInList
**	11/28/95	Changed AudioStream messages to enums
**	11/29/95	Added PlayTheSampleWithExtras. Added IsAudioStreamValid
**	12/1/95		Force a bail if not running Sound Manager 3.0 or better
**	12/6/95		Added PlayTheSampleLoopedWithExtras
**	12/14/95	Added DeltaDecompressPtr
**	1/3/96		Changed OSErr to SMSErr. Happy New Year!
**				Added more error codes
**				Changed AudioStream errors codes to match new SMS error codes
**				Added AudioStreamError to get last error
**				Exposed external midi control API
**	1/4/96		Added error codes to PlayTheSample, PlayTheSampleWithID, PlayTheSampleWithExtras,
**				and PlayTheSampleLoopedWithExtras
**				Added ChangeSoundReverb
**	1/7/96		Added AudioStreamReverb
**				Added SetEffectsVolume & GetEffectsVolume
**	1/18/96		Fixed a bug with 16 bit samples and PlayTheSampleWithExtras
**	1/25/96		Added error code to GetSongPatchList
**	1/28/96		Added DecompressPtr
**	2/4/96		Support for PRAGMA_ALIGN_SUPPORTED. 68k alignment always
**	2/7/96		Renamed DeltaDecompressPtr to DecompressSampleFormatPtr
**	2/12/96		Renamed GetSongLength to GetSongMidiTickLength. Now works for PPC
**				Added SetSongMidiTickPosition
**				Renamed GetCurrentMidiClock to GetSongMidiTickPosition
**				Added GetSongSecondsLength & GetSongSecondsPosition
**				Changed Boolean to SMSBoolean
**				Changed Ptr to void *
**	2/17/96		Added StopAllInstruments
**	3/6/96		Changed BeginSound system to support 16 bit samples
**	4/8/96		Fixed a bug with length calculations and 16 bit samples
**				Added GetSoundChannels & GetSoundBitSize
*/	
/*****************************************************/

#ifndef SOUNDMUSICSYS_DRIVER
#define SOUNDMUSICSYS_DRIVER

#ifdef __cplusplus
extern "C" {
#endif

// SOUND DRIVER EQUATES

/* Definitions for Sound resources */
#define SOUND_END				-1					/* End Sound for list */

/* Defines for sound functions */
#define SOUND_RATE_DEFAULT		0L					/* Default rate */
#define SOUND_RATE_FAST		0x56EE8BA3L			/* Fast */
#define SOUND_RATE_MEDIUM		0x2B7745D1L			/* Medium */
#define SOUND_RATE_MOSEY		SOUND_RATE_FAST/3	/* Mosey */
#define SOUND_RATE_SLOW		SOUND_RATE_MEDIUM/2	/* Slow */

#define SOUND_RATE_22k		SOUND_RATE_FAST
#define SOUND_RATE_11k		SOUND_RATE_MEDIUM
#define SOUND_RATE_7k			SOUND_RATE_MOSEY
#define SOUND_RATE_5k			SOUND_RATE_SLOW

#define REST_SAMPLE			-1L				/* When using the SampleList mechanisim, this will
											** rest (no sound) for the duration of the length
											*/
#define PLAY_ALL_SAMPLE		-2L				/* When using the SampleList mechanisim, this will
											** play the entire sound if this is put into the SampleList
											** theLength parameter.
											*/

// Reserved callback Sample ID's. These ID's are used for specialized features. Your callback function may
// be called with one of these values. If you do not want to handle it, do nothing.

#define CUSTOM_PLAY_ID	((short int)0x8000)		// ID value of samples played with PlayTheSample

// Ranges for SetMasterFade() and FadeLevel()
#define MAX_VOLUME		0x100
#define MIN_VOLUME		0

#define FULL_VOLUME		MAX_VOLUME
#define NO_VOLUME			MIN_VOLUME

// Ranges for stereo positiing
#define LEFT_POS			-255
#define MIDDLE_POS			0
#define RIGHT_POS			255

// Structure for a list of samples to play asynchronously
#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=mac68k
#endif
struct SampleList
{
	short int	theSampleID;		/* Sample resource ID or REST_SAMPLE for no sound */
	long		theRate;			/* Sample playback rate */
	short int	theLength;			/* Length of note in 1/60ths of a second */
};
typedef struct SampleList SampleList;
#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=reset
#endif

// SoundMusicSys error codes. These are going to follow Macintosh errors because user may have hard
// coded them.
enum
{
	SMS_NO_ERR					=	0,			// no errors
	SMS_PARAM_ERR				= 	-50,			// some parameter out of whack
	SMS_MEMORY_ERR				=	-108,		// out of memory
	SMS_NO_SONG_DATA			=	-192,		// Can't find Song resource
	SMS_BUFFER_TOO_SMALL		=	-210,		// Data buffer too small
	SMS_BAD_SAMPLE_DATA		=	-206,		// Sample data bad (corrupt?)
	SMS_SOUND_HARDWARE_BUSY		=	-209,		// Sound hardware busy, can't get a strong arm around it
	SMS_NOT_REENTERANT			=	-603,		// can't start a song at a song callback, or initilize twice
	SMS_NO_MIDI_DATA			=	-1300,		// Can't find Midi resource from Song resource
	SMS_ID_ALREADY_REGISTERED		=	-1855,		// ID passed is a duplicate
	SMS_ID_NEVER_REGISTERED		=	-2504,		// ID passed was never registered
	SMS_OS_TOO_OLD				=	-4000,		// SMS can't work with this old OS
	SMS_BAD_INSTRUMENT			=	-5000,
	SMS_BAD_RATE_RATE			=	-5001,
	SMS_BAD_MIDI_DATA			=	-5002,
	SMS_ALREADY_PAUSED			=	-5003,
	SMS_ALREADY_RESUMED			=	-5004,
	SMS_NO_SONG_PLAYING			=	-5005,
	SMS_STREAM_STOP_PLAY		=	-5006,		// stop stream at next buffer
	SMS_INVALID_REFERENCE			=	-5007,		// bad reference number passed
	SMS_NO_FREE_VOICES			=	-5008,		// no more free voices
	SMS_MDRV_GLUE_OUT_OF_SYNC	=	-5009,		// for 68k only: MDRV resource and glue mismatched version
	SMS_CANT_LOAD_MDRV			=	-5010,		// for 68k only; can't load MDRV resource
	SMS_EVAL_EXPIRED				=	-5011,		// if an evaluation copy; its expired
	SMS_ITASK_FAILED				=	-5012,		// failed to setup interrupts
	SMS_OMS_NOT_THERE			=	-5013,		// OMS not in library
	SMS_OMS_FAILED				=	-5014,		// OMS failed to sign in
	SMS_MIDI_MANAGER_FAILED		=	-5015,		// Midi Manager failed to sign in
	SMS_MIDI_MANGER_NOT_THERE	=	-5016,		// Midi Manager installed
	SMS_GENERAL_BAD				=	-5017,		// General bad news error
	SMS_OUT_OF_RANGE			=	-5018		// out of range
};
typedef short int SMSErr;

typedef short int SMSBoolean;

// Old error codes. Left over for support.
enum
{
	AS_RETURN_OK			=	SMS_NO_ERR,
	AS_RETURN_STOP_PLAY		=	SMS_STREAM_STOP_PLAY,
	AS_RETURN_NO_MEM		=	SMS_MEMORY_ERR,
	AS_RETURN_BAD_FORMAT	=	SMS_PARAM_ERR,
	AS_RETURN_BAD_DATA		=	SMS_BAD_SAMPLE_DATA,
	AS_RETURN_JUST_BAD		=	SMS_GENERAL_BAD
};



// Audio Sample Data Format. (ASDF)
// Support for 8, 16 bit data, mono and stereo. Can be extended for multi channel beyond 2 channels, but
// not required at the moment.
//
//	DATA BLOCK
//		8 bit mono data
//			ZZZZZZZÉ
//				Where Z is signed 8 bit data
//
//		16 bit mono data
//			WWWWWÉ
//				Where W is signed 16 bit data
//
//		8 bit stereo data
//			ZXZXZXZXÉ
//				Where Z is signed 8 bit data for left channel, and X is signed 8 bit data for right channel.
//
//		16 bit stereo data
//			WQWQWQÉ
//				Where W is signed 16 bit data for left channel, and Q is signed 16 bit data for right channel.
//



enum
{
	AS_CREATE				=	1,
	AS_DESTROY,
	AS_GET_DATA
};

// The AudioStreamObjectProc callback is called to allocate buffer memory, get the next block of data to stream and
// mix into the final audio output, and finally dispose of the memory block. All messages will happen at 
// non-interrupt time.
//
// INPUT:
// Message
//	AS_CREATE
//		Use this message to create a block a data with a length of pAS->dataLength. Keep in mind that dataLength
//		is always total number of samples,  not bytes allocated. Allocate the block of data into the Audio Sample 
//		Data Format based upon pAS->dataBitSize and pAS->channelSize. Store the pointer into pAS->pData.
//
//	AS_DESTROY
//		Use this message to dispose of the memory allocated. pAS->pData will contain the pointer allocated.
//		pAS->dataLength will be the sample size not the buffer size. ie. for 8 bit data use pAS->dataLength, 
//		for 16 bit mono data double pAS->dataLength.
//
//	AS_GET_DATA
//		This message is called whenever the streaming object needs a new block of data. Right after AS_CREATE
//		is called, AS_GET_DATA will be called twice. Fill pAS->pData with the new data to be streamed.
//		Set pAS->dataLength to the amount of data put into pAS->pData.
//
// OUTPUT:
// returns
//	SMS_NO_ERR
//		Everythings ok
//
//	SMS_STREAM_STOP_PLAY
//		Everything is fine, but stop playing stream
//
//	SMS_MEMORY_ERR
//		Couldn't allocate memory for buffers.
//
//	SMS_PARAM_ERR
//		General purpose error. Something wrong with parameters passed.
//
//

#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=mac68k
#endif
struct AudioStreamData
{
	long		asReference;		//	IN for all messages. Reference for which streaming audio object
	void		*pData;			// 	OUT for AS_CREATE, IN for AS_DESTROY and AS_GET_DATA
	long		dataLength;		//	OUT for AS_CREATE, IN for AS_DESTROY. IN and OUT for AS_GET_DATA
	long		sampleRate;		//	IN for all messages. Fixed 16.16 value
	short int	dataBitSize;		//	IN for AS_CREATE. Not used elsewhere
	short int	channelSize;		//	IN for AS_CREATE. Not used elsewhere
};
typedef struct AudioStreamData	AudioStreamData;
#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=reset
#endif


// CALLBACK FUNCTIONS TYPES

// AudioStream object callback
typedef short int (*AudioStreamObjectProc)(short int message, AudioStreamData *pAS);

// Song done, sound done callbacks
typedef pascal void (*AudioDoneProc)(short int theID);

// Sound looping. Return 1 to stop loop, 0 to continue loop
typedef pascal SMSBoolean (*AudioLoopDoneProc)(short int sampleID);

// Midi Controller callbacks
typedef void (*MidiControllerProc)(short int channel, short int controller, short int value);

// VBL Task callback
typedef pascal void (*VBLCallbackProc)(void);

// SoundQuality bit map
// bit		description
// 15		if 1 then output is 44k
// 14		if 1 then output is 22k
// 13		if 1 then output is 11k
// 12		if 1 then 16 bit output if hardware is able
// 11		if 1 then 8 bit output. NOTE: 8 bit output is slower on 16 bit hardware
// 10		if 1 then stereo output if hardware is able
// 9			if 1 then mono
// 8-4		reserved. set to 0.
			// Sample mixing synthesis method
// 3			// reserved
// 2			// linear interpolation
// 1			// two point linear interpolation
// 0			// drop sample

#define Q_DETERMINE			0x0000L
#define Q_RATE_44				(1L<<15)			// PPC only
#define Q_RATE_22				(1L<<14)
#define Q_RATE_11				(1L<<13)
#define Q_USE_16				(1L<<12)			// PPC only
#define Q_USE_8				(1L<<11)
#define Q_USE_STEREO			(1L<<10)			// PPC only
#define Q_USE_MONO			(1L<<9)
#define Q_USE_TYPE_4			(1L<<3)			// reserved
#define Q_USE_TYPE_3			(1L<<2)
#define Q_USE_TYPE_2			(1L<<1)
#define Q_USE_TYPE_1			(1L<<0)

// Masks
#define Q_RATE_MASK			(Q_RATE_44 | Q_RATE_22 | Q_RATE_11)
#define Q_BIT_MASK			(Q_USE_16 | Q_USE_8)
#define Q_MODE_MASK			(Q_USE_STEREO | Q_USE_MONO)
#define Q_TYPE_MASK			(Q_USE_TYPE_1 | Q_USE_TYPE_2 | Q_USE_TYPE_3 | Q_USE_TYPE_4)

// low quality and performance hits. Note: Unless you have 8 bit hardware, generating 8 bit output on
// a 16 bit device will be slower than generating 16 bit output
#define Q_LOW			(Q_RATE_11 | Q_USE_TYPE_1 | Q_USE_8 | Q_USE_MONO)
// This the typical quality setting. 22k output, best interpolation, 16 bit, stereo
#define Q_NORMAL		(Q_RATE_22 | Q_USE_TYPE_3 | Q_USE_16 | Q_USE_STEREO)
// Output at 44khz. Silky smooth, but costly in CPU tiime. Almost double.
#define Q_HIGH			(Q_RATE_44 | Q_USE_TYPE_3 | Q_USE_16 | Q_USE_STEREO)

// Support for old defines.
#define jxAnalyzeQuality			Q_DETERMINE
#define jxLowQuality			(Q_RATE_11 | Q_USE_TYPE_1 | Q_USE_8 | Q_USE_MONO)
#define jxHighQuality			(Q_RATE_22 | Q_USE_TYPE_1 | Q_USE_8 | Q_USE_MONO)
#define jxIntrepLowQuality		(Q_RATE_11 | Q_USE_TYPE_2 | Q_USE_8 | Q_USE_MONO)
#define jxIntrepHighQuality		(Q_RATE_22 | Q_USE_TYPE_2 | Q_USE_8 | Q_USE_MONO)
#define jxIntrepBestLowQuality	(Q_RATE_11 | Q_USE_TYPE_3 | Q_USE_8 | Q_USE_MONO)
#define jxIntrepBestHighQuality	(Q_RATE_22 | Q_USE_TYPE_3 | Q_USE_8 | Q_USE_MONO)

// Support for hystrical or historical reasons. Bad spellings
#define jxAnalyizeQuality			jxAnalyzeQuality
#define jxInterpLowQuality		jxIntrepLowQuality
#define jxInterpHighQuality		jxIntrepHighQuality
#define jxInterpBestLowQuality	jxIntrepBestLowQuality
#define jxInterpBestHighQuality	jxIntrepBestHighQuality

/* NOTE:
**	For future revisions, the numbers will change into bit fields for features that are added: ie.
**	support for 16 bit, stereo, etc. Use the defines to pass the values you want. If you 
**	don't now, you may get in trouble in the future.
*/
typedef unsigned long SoundQuality;

// API FUNCTIONS

SMSErr	InitSoundMusicSystem(	short int maxSongVoices, 
							short int maxNormalizedVoices, 
							short int maxEffectVoices,
							SoundQuality quality);
void		FinisSoundMusicSystem(void);

long		MaxVoiceLoad(void);

SMSErr	ChangeSystemVoices(	short int maxSongVoices, 
							short int maxNormalizedVoices, 
							short int maxEffectVoices);

SMSBoolean IsSoundSystemPaused(void);		// just sound sub system
SMSBoolean IsMusicSystemPaused(void);		// just music sub system
SMSBoolean IsSoundMusicSystemPaused(void);	// everything

SMSErr	PauseSoundMusicSystem(void);
SMSErr	ResumeSoundMusicSystem(void);
SMSErr	PauseMusicOnly(void);
SMSErr	ResumeMusicOnly(void);

// First byte is a compression type.
// Next 3 bytes is uncompressed length.
//
// Type 0 - Delta encoded LZSS
#ifdef __TYPES__
Handle	DecompressSampleFormatHandle(Handle theData);
#endif

//define	DeltaDecompressPtr(x)		DecompressSampleFormatPtr(x)
//define	DeltaDecompressHandle(x)	DecompressSampleFormatHandle(x)


SMSErr	RegisterSounds(short int *pSoundID, SMSBoolean prePurge);
void		ReleaseRegisteredSounds(void);

long		SoundMemorySize(short int *pSoundID);

SMSErr	BeginSoundList(SampleList * sampleList, short int totalSamples);
void		EndSoundList(void);
SMSBoolean IsSoundInList(short int theID);

SMSErr	BeginSound(short int theID, long theRate);
SMSErr	BeginSoundReverse(short int theID, long theRate);
SMSErr	BeginSoundSection(short int theID, long theRate, long sectionStart, long sectionEnd);
SMSErr	BeginSoundLoop(short int theID, long theRate, long loopStart, long loopEnd);
SMSErr	BeginSoundEnvelope(short int theID, long theRate, short int loopTime);
SMSErr	BeginSoundEnvelopeProc(short int theID, long theRate, AudioLoopDoneProc theLoopProc);

void		ChangeSoundStereoPosition(short int theID, short int stereoPosition);
void		ChangeSoundPitch(short int theID, long theRate);
void		ChangeSoundVolume(short int theID, short int theVolume);
void		ChangeSoundReverb(short int theID, SMSBoolean enableReverb);

SMSErr	LoadSound(short int theID);
SMSErr	FreeSound(short int theID);

SMSBoolean IsSoundFXFinished(void);
SMSBoolean IsThisSoundFXFinished(short theID);
SMSBoolean IsSoundListFinished(void);

SMSErr	PlayTheSample(void * pSamp, long sampSize, long sampRate);
SMSErr	PlayTheSampleWithID(void * pSamp, long sampSize, long sampRate, short int theID);
SMSErr	PlayTheSampleWithExtras(void * pSamp, long frames, long sampRate, short int theID, short int channels, 
								short int bitSize, short int volume, short int stereoPos);
SMSErr	PlayTheSampleLoopedWithExtras(void * pSamp, long frames, long sampRate, short int theID, short int channels, 
								short int bitSize, short int volume, short int stereoPos,
								long loopStart, long loopEnd, AudioLoopDoneProc theLoopProc);

void		SetSoundDoneCallBack(AudioDoneProc theProcPtr);
void		SetSongDoneCallBack(AudioDoneProc theProcPtr);

void		SetMasterVolume(short int theVolume);
short int	GetMasterVolume(void);

void		SetSoundVBCallBack(VBLCallbackProc theProcPtr);

void *	GetSoundWaveform(short int theID);
long		GetSoundLength(short int theID);
long		GetSoundDefaultRate(short int theID);
long		GetSoundLoopStart(short int theID);
long		GetSoundLoopEnd(short int theID);
short int	GetSoundChannels(short int theID);
short int	GetSoundBitSize(short int theID);

short int	GetSoundTime(short int theID, long sampRate);
short int	CalcPlaybackLength(long theRate, long theLength);
void		EndAllSound(void);
void		EndSoundEffects(void);

SMSErr	EndSound(short int theID);

void		PurgeAllSounds(unsigned long minMemory);


// Multi source user config based streaming
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
long		AudioStreamStart(long userReference, AudioStreamObjectProc pProc, 						//
							long dataLength, 
							long sampleRate,					// Fixed 16.16
							short int dataBitSize,				// 8 or 16 bit data
							short int channelSize);				// 1 or 2 channels of date

// This will stop a streaming audio object.
//
// INPUT:
//	asReference	This is the reference number returned from AudioStreamStart.
//
SMSErr	AudioStreamStop(long reference);

// This will return the last AudioStream Error
SMSErr	AudioStreamError(void);

// This will stop all streams that are current playing.
void		AudioStreamStopAll(void);

// This is the streaming audio service routine. Call this as much as possible, but not during an
// interrupt. This is a very quick routine. A good place to call this is in your main event loop.
void		AudioStreamService(void);

// Returns TRUE or FALSE if a given AudioStream is still active
SMSBoolean IsAudioStreamPlaying(long reference);

// Returns TRUE if a given AudioStream is valid
SMSBoolean IsAudioStreamValid(long reference);

// Set the volume level of a audio stream
void		AudioStreamSetVolume(long reference, short int newVolume);

// Set the sample rate of a audio stream
void		AudioStreamSetRate(long reference, unsigned long newRate);

// Set the stereo position of a audio stream
void		AudioStreamSetStereoPosition(long reference, short int stereoPosition);

// Enable/Disable reverb on this particular audio stream
void		AudioStreamReverb(long reference, SMSBoolean enableReverb);

// Song
SMSErr	BeginSong(short int theSong);
SMSErr	BeginSongLooped(short int theSong);
#ifdef __TYPES__
SMSErr	BeginSongFromMemory(short int theID, Handle theSongResource, Handle theMidiResource, SMSBoolean loopSong);
#endif
SMSErr	LoadSong(short int theSong);
void		FreeSong(void);
void		EndSong(void);

// Given a controller and a callback function; this function will be called everytime the midi sequencer encounters
// this particular controller
void		SetMidiControllerCallback(short int theController, MidiControllerProc theProc);			// PPC only

SMSErr	GetSongPatchList(short int theSong, short int *pInstArray512, short int *pSndArray768);

void		SetSongVolume(short int volume);
short int	GetSongVolume(void);

void		SetEffectsVolume(short int volume);
short int	GetEffectsVolume(void);

void		SetReverbType(short int reverbType);
short int	GetReverbType(void);

void		SetLoopSongFlag(SMSBoolean theFlag);
SMSBoolean	IsSongDone(void);
long		SongTicks(void);

// Will cache instruments and load all the instruments from the current resource file.
SMSErr	CacheInstrumentsNow(SMSBoolean cacheFlag);			// PPC only

// Will cache instruments and not load any instruments. Instruments will be cached as music is loaded.
SMSErr	CacheInstrumentsLater(SMSBoolean cacheFlag);		// PPC only

// PurgeSongs & LockSongs are no longer supported. Use CacheInstrumentsNow, CacheInstrumentsLater, and 
// LoadSong to achieve the desired effect.
#define	PurgeSongs(x)
#define	LockSongs(x)

void		BeginMasterFadeOut(long time);
void		BeginMasterFadeIn(long time);
void		SetMasterFade(long time);
long		FadeLevel(void);

SMSErr	ChangeOutputQuality(SoundQuality qual);
#define	ChangeOuputQuality ChangeOutputQuality

// These functions aren't really working yet. So don't rely on them
SMSErr	GetErrors(void *buffer);
long		GetCurrentMidiBeat(void);
long		GetNextBeatMidiClock(void);
long		GetCurrentTempo(void);
//

// Works only on PPC
long		GetSongMidiTickLength(void);				// return midi tick length
SMSErr	SetSongMidiTickPosition(long clock);			// set position with midi tick
long		GetSongMidiTickPosition(void);				// current midi tick
long		GetSongSecondsLength(void);				// return seconds length
long		GetSongSecondsPosition(void);				// current second

short int	GetAudioFrameSlice(short int *audioFrame);

// Useful tools for determining the hardware enviroments
SMSBoolean	IsNewSoundManagerInstalled(void);
SMSBoolean	IsSoundManagerStereo(void);
SMSBoolean	IsSoundManager16Bit(void);

// Enable live link connection to OMS, Midi Manager, or process yourself
enum
{
	USE_OMS = 1,			// to connect to OMS as an external device. This will load all instruments.
	USE_MIDI_MANAGER,	// to connect to Apple's MIDI Manager as an external device. This will load all instruments
	USE_EXTERNAL			// to use an API to drive the synth engine. This will NOT load any instruments
};

SMSErr	EnableLiveLink(short maxVoices, short normVoices, short effectVoices, short connectionType);
void		DisableLiveLink(void);

// These are standard encoded midi command codes with channel and parameters
void		ProcessExternalMidiEvent(short int commandByte, short int data1Byte, short int data2Byte);

// Load and free instruments
SMSErr	LoadInstrument(short int theInstrument);
SMSErr	UnloadInstrument(short int theInstrument);

// Stop playing all instruments currently on
void	StopAllInstruments(void);

#ifdef __cplusplus
}
#endif


#endif	// SOUNDMUSICSYS_DRIVER


#ifndef __X_API__
#define __X_API__

// some common types


typedef void *			XPTR;
typedef short			XERR;
typedef unsigned char	XBYTE;
typedef unsigned short	XWORD;
typedef unsigned long	XDWORD;
typedef char			XBOOL;

#ifndef TRUE
	#define TRUE	1
#endif

#ifndef FALSE
	#define FALSE	0
#endif

#ifndef NULL
	#define NULL	0L
#endif

#ifndef QuadChar
	#define QuadChar(ch1,ch2,ch3,ch4) \
		((((ch1)&0x0ff)<<24) + (((ch2)&0x0ff)<<16) + (((ch3)&0x0ff)<<8) + ((ch4)&0x0ff))
#endif

#if X_HARDWARE_PLATFORM == X_MACINTOSH
	// Macintosh versions
	#include <Files.h>

	#if THINK_C
		#include <Think.h>
	#endif
#endif


#if X_HARDWARE_PLATFORM == X_MACINTOSH
	#define X_WORD_ORDER	FALSE
#endif
#if X_HARDWARE_PLATFORM == X_WEBTV
	#define X_WORD_ORDER	FALSE
#endif

// Memory Manager
extern XPTR	XNewPtr(long size);
extern void		XDisposePtr(XPTR data);
extern long		XGetPtrSize(XPTR data);
extern void		XBlockMove(XPTR source, XPTR dest, long size);
extern XBOOL	XIsVirtualMemoryAvailable(void);
extern void		XLockMemory(XPTR data, long size);
extern void		XUnlockMemory(XPTR data, long size);
extern void		XSetMemory(void *pAdr, long len, char value);
extern void		XBubbleSortArray(short int *theArray, short int theCount);
extern void		XSetBit(void *pBitArray, long whichbit);
extern void		XClearBit(void *pBitArray, long whichbit);
extern XBOOL	XTestBit(void *pBitArray, long whichbit);



// Resource Manager
struct XFILENAME
{
	long		fileID;
#if X_HARDWARE_PLATFORM == X_MACINTOSH
	FSSpec	theFile;
#endif
#if X_HARDWARE_PLATFORM == X_WIN95
	char		theFile[1024];
#endif
};
typedef struct XFILENAME		XFILENAME;
typedef long				XFILE;
typedef long				XRFILE;

extern XRFILE	XFileOpenResource(XFILENAME *file);
extern void		XFileClose(XRFILE fileRef);
extern XPTR	XGetAndDetachResource(long resourceType, long resourceID, long *pReturnedResourceSize);

// File Manager
extern XFILE	XFileOpenForRead(XFILENAME *file);
extern XFILE	XFileOpenForWrite(XFILENAME *file);
extern void		XFileClose(XFILE fileRef);
extern long		XFileRead(XFILE fileRef, void * buffer, long bufferLength);
extern long		XFileWrite(XFILE fileRef, void *buffer, long bufferLength);
extern void		XFileSetPosition(XFILE fileRef, long filePosition);
extern long		XFileGetPosition(XFILE fileRef);
extern long		XFileGetLength(XFILE fileRef);

extern XDWORD	XFixedDivide(XDWORD divisor, XDWORD dividend);
extern XDWORD	XFixedMultiply(XDWORD prodA, XDWORD prodB);

extern unsigned short int 	XGetShort(void *pData);
extern unsigned long		XGetLong(void *pData);
extern void				XPutShort(void *pData, unsigned long data);
extern void				XPutLong(void *pData, unsigned long data);

// Sound Support
extern XBOOL	XIs16BitSupported(void);
extern XBOOL	XIsStereoSupported(void);

extern void		XExpandMace1to6(void *inBuffer, void *outBuffer, unsigned long cnt, 
					void * inState, void * outState, 
					unsigned long numChannels, unsigned long whichChannel);
extern void		XExpandMace1to3(void *inBuffer, void *outBuffer, unsigned long cnt, 
					void * inState, void * outState, 
					unsigned long numChannels, unsigned long whichChannel);


#endif	// __X_API__

/*****************************************************/
/*
**	MacSpecificSMS.h
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
*/
/*****************************************************/

#ifndef _MAC_SPEC_
#define _MAC_SPEC_

#undef _POWERPC_
#define _POWERPC_ ((powerc) || (__powerc))


// Set global variables
	#define GetGlobalsRegister()	0
	#define SetGlobalsRegister(x)	(x)
	#define USE_A5			1

extern OSErr		SetupVBLTask(void);
extern OSErr		CleanupVBLTask(void);

extern short int		XGetHardwareVolume(void);
extern void			XSetHardwareVolume(short theVolume);
extern void			XSetHardwareSampleRate(unsigned long sampleRate);
extern unsigned long	XGetHardwareSampleRate(void);



#endif	// _MAC_SPEC_

/*****************************************************/
/*
**	PrivateSoundMusicSystem.h
**
**		This a private header for communication between the music driver
**		and the sound effects driver.
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**
**	History	-
**	3/23/92		Created
**	7/10/92		Changed file name
**	12/21/93	Commented out [] arrays for C++
**	10/14/94	Added default perc in Song resource
**	10/24/94	Added functions from private to public
**	12/6/94		Added volume functions
**	3/26/95		Changed copyirght notice
**	3/26/95		Changed JX_Pause & JX_Resume parameters
**	5/2/95		Moved debug macros in here from SoundSys.c
**	6/23/95		Added DriverTools.c & SampleTools to library
**	9/6/95		Added CODE_BASE macro for multi processer support
**	9/6/95		Changed sound done callback & loop done callback to use typedefs
**	9/15/95		Added stereo postion calls, and sample volume controls
**	9/21/95		Moved IsNewSoundManagerInstalled & IsSoundManagerStereo & 
**				IsSoundManager16Bit to main header file
**	9/22/95		Typedefs callback functions
**				Added SetMidiControllerCallback (PowerPC only), and JX_SetMidiControllerProc
**				Removed kChangeOutputQuality
**	11/20/95	Made StopAllEffects a direct call
**	11/29/95	Extended the PlaySampleData structure to include stereo and 8/16 bits
**				Extended the SampleDataInfo structure to include volume and stereo placement
**				Changed the way LowLevelPlaySample works. Now passes a structure with most of the parameters
**   12/95 (jln)	upgraded mixing bus to 32 bit; improved scaleback resolution; added reverb unit; first pass at volume ADSR
**	12/6/95		Added reverbType to external song resource structure
**				Added GetInstrumentEnvelopeData API
**	12/11/95	Removed GetInstrumentEnvelopeData
**	12/15/95	Added XGetSamplePtrFromSnd
**	1/18/96		Removed bitfields in Song resource. They don't work with C++!!!!
**	1/28/96		Added GetKeySplitFromPtr
**				Added NewSongPtr & DisposeSongPtr
**	1/29/96		Added useSampleRate factor for playback of instruments & sampleAndHold bits
**	2/11/96		Removed GetDefaultMIDISetup
**	2/12/96		Changed Boolean to SMSBoolean
**	2/17/96		Added more platform compile arounds
**	2/19/96		Changed NewSongPtr
**	4/2/96		Added XSetSoundLoopPoints & XSetSoundSampleRate & XSetSoundBaseKey
*/
/*****************************************************/

#ifndef _PRIVATE_SOUNDMUSICSYS_
#define _PRIVATE_SOUNDMUSICSYS_

#ifndef __X_API__
	#include "X_API.h"
#endif

#if X_HARDWARE_PLATFORM == X_MACINTOSH
	#ifndef SOUNDMUSICSYS_DRIVER
		#include "SoundMusicSystem.h"
	#endif
#endif


// Total number of voices that can be mixed at once.
#define MAX_VOICES	32

// Set which code base we're using
#define DETERMINE_CODE		0					// determine which for mixed compilations and 
											// redefined CODE_BASE
#define USE_NATIVE			1					// all 'C' version
#define USE_MIXED			2					// Compiled 'C' for PPC and 68k MDRV
#define USE_68K			3					// Compiled 'C' for 68k, and 68k MDRV

#if 01
	#define CODE_BASE			DETERMINE_CODE
#else
	#define CODE_BASE			USE_NATIVE
#endif

#if CODE_BASE == DETERMINE_CODE
	#undef CODE_BASE
	#if GENERATINGPOWERPC
		#define CODE_BASE 		USE_NATIVE		// USE_MIXED
	#else
		#define CODE_BASE 		USE_68K
	#endif
#endif



#ifndef DEBUG_STR
	#if USE_DEBUG == 0
		#define DEBUG_STR(x)
	#endif

	#if USE_DEBUG == 1
		#define DEBUG_STR(x)	DebugStr((void *)(x))
	#endif

	#if USE_DEBUG == 2
		extern short int drawDebug;
		void DPrint(short int d, ...);
		#define DEBUG_STR(x)	DPrint(drawDebug, "%p\r", x)
	#endif

	#if USE_DEBUG == 3
		#define DEBUG_STR(x)
		extern short int drawDebug;
		void DPrint(short int d, ...);
		#define DEBUG_STR2(x)	DPrint(drawDebug, "%p\r", x)
	#else
		#define DEBUG_STR2(x)
	#endif
#endif

#ifndef __MACLIB__
	typedef unsigned short Word;
#endif

#undef ABS
#define ABS(x)			(((x) < 0) ? -(x) : (x))


#if X_HARDWARE_PLATFORM == X_MACINTOSH

#define SOUNDMGR_ID			(('s'<<8)+'m')				/* ID used for the call back procedure */

#define ARG_SIZE				long


#define envMacClassic			15
#define envMacIIsi				16
#define envMacLC				17

// Driver Resource IDs
#define jxDriverMIDI			11		// 11 & 22 khz version

// Driver parameters
#define noDriverInstalled			(ARG_SIZE)(-1)
#define useBestDriver			(ARG_SIZE)0
#define use11khzDriver			(ARG_SIZE)11
#define use22khzDriver			(ARG_SIZE)22
#define use44khzDriver			(ARG_SIZE)44
#define use2ptIntrep				(ARG_SIZE)0x100
#define useLargeIntrep			(ARG_SIZE)0x200
#define useStereo				(ARG_SIZE)0x1000
#define use16Bit				(ARG_SIZE)0x2000

/* Messages to Jim's eXcelent music driver
*/
#define jxLoadPlaySong			(ARG_SIZE)0	// load and start playing song
#define jxLoadNoPlaySong			(ARG_SIZE)1	// load and don't play song
#define jxPlayMusic				(ARG_SIZE)2	// play song loaded by jxLoadNoPlaySong
#define jxLoadAllInstruments		(ARG_SIZE)3	// Load all instruments, call jxFreeSong to clean up
#define jxStatusMusic			(ARG_SIZE)4
#define jxStopMusic				(ARG_SIZE)5
#define jxStopNotes				(ARG_SIZE)6	// call for live notes to stop
#define jxFreeSong				(ARG_SIZE)7	// deallocate all, but not kill system
#define jxFreeMusic				(ARG_SIZE)8	// deallocate all and kill system
#define jxPlayNote				(ARG_SIZE)9
#define jxPause				(ARG_SIZE)10
#define jxResume				(ARG_SIZE)11
#define jxStopNote				(ARG_SIZE)12
#define jxReplaySongFlag			(ARG_SIZE)13
#define jxGetErrorResults		(ARG_SIZE)14
#define jxGetSoundBufferSync		(ARG_SIZE)15
#define jxSetSoundBufferSync	 	(ARG_SIZE)16
#define jxPlaySample			(ARG_SIZE)17
#define jxStopSample			(ARG_SIZE)18
#define jxFade					(ARG_SIZE)19
#define jxIsSoundDone			(ARG_SIZE)20
#define jxSetupVoices			(ARG_SIZE)21
#define jxStopAllSamples			(ARG_SIZE)22
#define jxChangeSamplePitch		(ARG_SIZE)23
#define jxSelectPlayRate			(ARG_SIZE)24
#define jxEnableMIDIInput			(ARG_SIZE)25
#define jxEnableMIDIOutput		(ARG_SIZE)26
#define jxSetCallbackVars		(ARG_SIZE)27
#define jxGetMidiClocks			(ARG_SIZE)28
#define jxGetTempo				(ARG_SIZE)29
#define jxGetBeat				(ARG_SIZE)30
#define jxGetSongLength			(ARG_SIZE)31
#define jxGetNextBeatClock		(ARG_SIZE)32
#define jxGetSongLengthBeats		(ARG_SIZE)33
#define jxLoadThisInstrument		(ARG_SIZE)34
#define jxPrepSystem			(ARG_SIZE)35
#define jxUnloadThisInstrument	(ARG_SIZE)36
#define jxPlayMemorySong		(ARG_SIZE)37
#define jxLoadPartialInstrument	(ARG_SIZE)38
#define jxUnloadPartialInstrument	(ARG_SIZE)39
#define jxSetChannelPrograms		(ARG_SIZE)40
#define jxSetDefaultPercProgram	(ARG_SIZE)41
#define jxChangeSampleVolume		(ARG_SIZE)42
#define jxChangeSongVolume		(ARG_SIZE)43
#define jxGetSongVolume			(ARG_SIZE)44
#define jxSetAudioSyncProc		(ARG_SIZE)45	// set audio sync proc
#define jxCacheInstrumentFlag		(ARG_SIZE)46	// set cache instrument flag
#define jxVerifyMDRVCode		(ARG_SIZE)47	// return a verifcation that a valid MDRV exisits
#define jxSetSongCallbackProc		(ARG_SIZE)48
#define jxChangeSampleStereoPos	(ARG_SIZE)49
#define jxGetUsedPatchList		(ARG_SIZE)50
#define jxSetMidiControllerProc	(ARG_SIZE)51
#define jxGetAudioFrameSlice		(ARG_SIZE)52
#define jxSetSongPosition			(ARG_SIZE)53
#define jxGetSongSeconds			(ARG_SIZE)54
#define jxGetSongLengthSeconds	(ARG_SIZE)55

/* Function interface to Jim's eXcelent music driver
** Interface to Jim's fabulous MIDI & SMUS driver; private to me only
*/
#define JX_LoadPlaySong(theSong)		(*theMusicDriver)(jxLoadPlaySong, (void *)(theSong))
#define JX_LoadNoPlaySong(theSong)	(*theMusicDriver)(jxLoadNoPlaySong, (void *)(theSong))
#define JX_PlayMusic()				(*theMusicDriver)(jxPlayMusic, 0L)
#define JX_StatusMusic()				(*theMusicDriver)(jxStatusMusic, 0L)
#define JX_StopMusic()				(*theMusicDriver)(jxStopMusic, 0L)
#define JX_PlaySongFromMem(theData)	(*theMusicDriver)(jxPlayMemorySong, (void *)(theData))
#define JX_StopNotes()				(*theMusicDriver)(jxStopNotes, 0L)
#define JX_FreeSong()				(*theMusicDriver)(jxFreeSong, 0L)
#define JX_SetSongCallbackProc(code)	(*theMusicDriver)(jxSetSongCallbackProc, (code))
#define JX_FreeMusic()				(*theMusicDriver)(jxFreeMusic, 0L)
#define JX_Pause(flag)				(*theMusicDriver)(jxPause, (void *)(flag))
#define JX_Resume(flag)				(*theMusicDriver)(jxResume, (void *)(flag))
#define JX_ReplaySongFlag(flag)		(*theMusicDriver)(jxReplaySongFlag, (void *)flag)
#define JX_PlaySoundData(pData)		(*theMusicDriver)(jxPlaySample, (void *)(pData))
#define JX_StopSoundData(pData)		(*theMusicDriver)(jxStopSample, (void *)(pData))
#define JX_FadeLevel(level)			(*theMusicDriver)(jxFade, (void *)(level))
#define JX_IsSoundDone(pData)			(*theMusicDriver)(jxIsSoundDone, (void *)(pData))
#define JX_SetupVoices(pData)			(*theMusicDriver)(jxSetupVoices, (void *)(pData))
#define JX_SetupCallbackVars(pData)	(*theMusicDriver)(jxSetCallbackVars, (void *)(pData))
#define JX_StopAllSamples()			(*theMusicDriver)(jxStopAllSamples, 0L)
#define JX_ChangePitch(pData)			(*theMusicDriver)(jxChangeSamplePitch, (void *)(pData))

#define JX_ChangeSampleVolume(pData)	(*theMusicDriver)(jxChangeSampleVolume, (void *)(pData))
#define JX_ChangeSongVolume(volume)	(*theMusicDriver)(jxChangeSongVolume, (void *)(volume))
#define JX_GetSongVolume(pVolume)	(*theMusicDriver)(jxGetSongVolume, (void * *)(pVolume))
#define JX_ChangeSampleStereoPos(pos)	(*theMusicDriver)(jxChangeSampleStereoPos, (void *)(pos))

#define JX_SelectPlayRate(rate)		(*theMusicDriver)(jxSelectPlayRate, (void *)(rate))
#define JX_EnableMIDIInput()			(*theMusicDriver)(jxEnableMIDIInput, 0L)
#define JX_EnableMIDIOutput()			(*theMusicDriver)(jxEnableMIDIOutput, 0L)
#define JX_GetErrors()				(*theMusicDriver)(jxGetErrorResults, 0L)
#define JX_GetSoundBufferSync()		(*theMusicDriver)(jxGetSoundBufferSync, 0L)
#define JX_SetSoundBufferSync(x)		(*theMusicDriver)(jxSetSoundBufferSync, (void *)(x))

#define JX_GetMidiClocks()			(*theMusicDriver)(jxGetMidiClocks, 0L)
#define JX_GetTempo()				(*theMusicDriver)(jxGetTempo, 0L)
#define JX_GetBeat()				(*theMusicDriver)(jxGetBeat, 0L)
#define JX_GetNextBeatClock()			(*theMusicDriver)(jxGetNextBeatClock, 0L)


#define JX_SetSongPosition(pos)		(*theMusicDriver)(jxSetSongPosition, (void *)(pos))
#define JX_GetSongLength()			(*theMusicDriver)(jxGetSongLength, 0L)
#define JX_GetSongSeconds()			(*theMusicDriver)(jxGetSongSeconds, 0L)
#define JX_GetSongLengthSeconds()		(*theMusicDriver)(jxGetSongLengthSeconds, 0L)

#define JX_GetSongLengthBeats()		(*theMusicDriver)(jxGetSongLengthBeats, 0L)

#define JX_LoadAllInstruments()		(*theMusicDriver(jxLoadAllInstruments, 0L)
#define JX_PlayNote(pNote)			(*theMusicDriver)(jxPlayNote, (NoteInfo *)(pNote))
#define JX_StopNote(pNote)			(*theMusicDriver)(jxStopNote, (NoteInfo *)(pNote))
#define JX_PrepSystem()				(*theMusicDriver)(jxPrepSystem, 0L)
#define JX_LoadThisInstrument(x)		(*theMusicDriver)(jxLoadThisInstrument, (void *)(x))
#define JX_UnloadThisInstrument(x)		(*theMusicDriver)(jxUnloadThisInstrument, (void *)(x))

#define JX_SetCacheInstrumentFlag(x)	(*theMusicDriver)(jxCacheInstrumentFlag, (void *)(x))

#define JX_GetUsedPatchList(x)		(*theMusicDriver)(jxGetUsedPatchList, (void *)(x))
#define JX_SetChannelPrograms(x)		(*theMusicDriver)(jxSetChannelPrograms, (void *)(x))
#define JX_SetDefaultPercussion(x)		(*theMusicDriver)(jxSetDefaultPercProgram, (void *)(x))

#define JX_SetMidiControllerProc(x)	(*theMusicDriver)(jxSetMidiControllerProc, (void *)(x))

#define JX_VerifyMDRVCode()			(*theMusicDriver)(jxVerifyMDRVCode, 0L)

#define JX_GetAudioFrameSlice(x)		(*theMusicDriver)(jxGetAudioFrameSlice, (void *)(x))

#define MDRV_VERSION				3953		// current version of MDRV code resource
typedef long (*MDRVCodeBasePtr)(long msg, void * p1);
extern MDRVCodeBasePtr theMusicDriver;




#define eInvalidRate			-1	// Invalid rate!
#define eVBLMemory		-2	// Unable to allocate 36 bytes for VBL task!
#define eVBLOverload		-3	// VBL task overloaded!  Reduce voices!
#define eCantLoadSong		-4	// Error encountered while doing LoadSong.
#define eCantLoadFullSet		-5	// Error loading full instrument set.
#define eCantLoadInstrument	-6	// Could not load an INST resource requested by song.
#define eCantLoadSnd		-7	// Problem loading sound resource.
#define eCantLoadSMOD		-8	// SMOD missing.
#define e128kTableMemory	-9	// Unable to allocate 128k for interpolation table.
#define e32kScaleMemory	-10	// Unable to allocate 32K for amplitude scale buffer.
#define eMaxNotesRange		-11	// Range error: Max Notes requested <= 0.
#define eOutputBuffer		-12	// Unable to allocate sample output conversion buffer.
#define eTempoTooBig		-13	// Tempo way too large to scale down
#define eSongDivisionTooBig	-14	// Song division is way too large (or tempo is too small)
#define eCantCleanVBL		-15	// Could not locate our vbl!
#define eBadSoundTask		-16	// An incompatible sound task is already running.
#define eSndPlayDoubleFail	-17	// SndPlayDoubleBuffer failed.
#define eCorruptSnd			-18	// Corrupted sound resource.
#define eSampleRate			-19	// Error in sample rate conversion.
#define eCantLoadMidi		-20	// Can't load MIDI resource.

//#define kBeginSong					(short int)0	// kBeginSong, theSongID
//#define kBeginSongLooped				(short int)1	// kBeginSongLooped, theSongID
//#define kEndSong					(short int)2	// kEndSong, 0
//#define kIsSongDone					(short int)3	// kIsSongDone, 0
//#define kTicksSinceStart				(short int)4	// kTicksSinceStart, 0
//#define kPurgeFlag					(short int)5	// kPurgeFlag, SMSBoolean
//#define kLockFlag					(short int)6	// kLockFlag, SMSBoolean
//#define kLoopSongFlag				(short int)7	// kLoopSongFlag, SMSBoolean
//#define kFadeOutput					(short int)8	// kFadeOutput, level (0 to 256)
//#define kAutoFadeSoundOut			(short int)10	// kAutoFadeSoundOut, ticks for how fast
//#define kAutoFadeSoundIn				(short int)11	// kAutoFadeSoundIn, ticks for how fast
//#define kFadeLevel					(short int)12	// kFadeLevel, short int
//#define kSetMasterFade				(short int)13	// kSetMasterFade, short int
//#define kChangeOutputQuality			(short int)14	// kChangeOutputQuality, SoundQuality
//#define kLoadSong					(short int)15	// kLoadSong, theSongID
//#define kFreeSong					(short int)16	// kFreeSong
#define kGetErrors					(short int)17	// kGetErrors, char *
//#define kGetMidiClock				(short int)18	// kGetMidiClock
#define kGetMidiBeat				(short int)19	// kGetMidiBeat
#define kGetNextMidiBeat				(short int)20	// kGetNextMidiBeat
//#define kSetMidiClock				(short int)21	// kSetMidiClock, pos
#define kGetTempo					(short int)22	// kGetTempo
//#define kGetSongLength				(short int)23	// kGetSongLength
//#define kGetSongVolume				(short int)24	// kGetSongVolume
//#define kSetSongVolume				(short int)25	// kSetSongVolume, volume

// Function call interface:
/*
#define	BeginSong(theSong)			MusicDriverHandler(kBeginSong, (long)(theSong))
#define	BeginSongLooped(theSong)		MusicDriverHandler(kBeginSongLooped, (long)(theSong))
#define	LoadSong(theSong)			MusicDriverHandler(kLoadSong, (long)(theSong))
#define	FreeSong()				MusicDriverHandler(kFreeSong, 0L)
#define	SetLoopSongFlag(theFlag)		MusicDriverHandler(kLoopSongFlag, (long)(theFlag))
#define	EndSong()					MusicDriverHandler(kEndSong, 0L)
#define	IsSongDone()				MusicDriverHandler(kIsSongDone, 0L)
#define	SongTicks()				MusicDriverHandler(kTicksSinceStart, 0L)
#define	PurgeSongs(purge)			MusicDriverHandler(kPurgeFlag, (long)(purge))
#define	LockSongs(lock)			MusicDriverHandler(kLockFlag, (long)(lock))
#define	BeginMasterFadeOut(time)		MusicDriverHandler(kAutoFadeSoundOut, (long)(time))
#define	BeginMasterFadeIn(time)		MusicDriverHandler(kAutoFadeSoundIn, (long)(time))
#define	SetMasterFade(level)		MusicDriverHandler(kSetMasterFade, (long)(level))
#define	FadeLevel()				MusicDriverHandler(kFadeLevel, 0L)
#define	ChangeOuputQuality(qual)		MusicDriverHandler(kChangeOutputQuality, (long)(qual))
#define	GetErrors(buffer)			MusicDriverHandler(kGetErrors, (long)(buffer))
#define	GetCurrentMidiClock()		MusicDriverHandler(kGetMidiClock, 0L)
#define	GetCurrentMidiBeat()		MusicDriverHandler(kGetMidiBeat, 0L)
#define	GetNextBeatMidiClock()		MusicDriverHandler(kGetNextMidiBeat, 0L);
#define	SetCurrentMidiClock(clock)	MusicDriverHandler(kSetMidiClock, (long)(clock))
#define	GetCurrentTempo()			MusicDriverHandler(kGetTempo, 0L)
#define	GetSongLength()			MusicDriverHandler(kGetSongLength, 0L)
#define	SetSongVolume(volume)		MusicDriverHandler(kSetSongVolume, (long)(volume))
#define	GetSongVolume()			MusicDriverHandler(kGetSongVolume, 0L)
*/

// Private messages to the music driver
//#define kPlaySoundData				(short int)100	// kPlaySoundData, PlaySampleData *
//#define kStopSoundData				(short int)101	// kStopSoundData, PlaySampleData *
#define kIsSoundDone				(short int)102	// kIsSoundDone, PlaySampleData *
//#define kStopAllSoundEffects			(short int)103	// kStopAllSoundEffects
#define kChangeSamplePitch			(short int)104	// kChangeSamplePitch, PlaySampleData *
#define kSetupCallbackProc			(short int)105	// kSetupCallbackProc, CallbackData *
#define kChangeSampleVolume			(short int)106	// kChangeSampleVolume, PlaySampleData *
#define kChangeStereoPosition			(short int)107	// kChangeStereoPosition, PlaySampleData *




#if !THINK_C
	#define MUSICDRIVERHANDLER MusicDriverHandler
#endif
pascal unsigned long MusicDriverHandler(short int theMessage, long theData);
#endif


#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=mac68k
#endif
struct SetupVoices
{
	short int		maxSongVoices;			// max voices for music
	short int		maxNormalizedVoices;		// max voices for normalize song + effect
	short int		maxEffectVoices;			// max voices for effects
};
typedef struct SetupVoices SetupVoices;

/* Instrument and Song structures
*/
struct Remap
{
	short int		instrumentNumber;
	short int		ResourceINSTID;
};
typedef struct Remap Remap;
	
#define ID_SONG				QuadChar('S','O','N','G')
#define ID_INST				QuadChar('I','N','S','T')
#define ID_MIDI				QuadChar('M','i','d','i')
#define ID_MIDI_OLD				QuadChar('M','I','D','I')
#define ID_CMID				QuadChar('c','m','i','d')
#define ID_CMIDI				QuadChar('c','m','i','d')
#define ID_SND					QuadChar('s','n','d',' ')
#define ID_CSND				QuadChar('c','s','n','d')
#define ID_SETUP				QuadChar('M','D','F','T')

// bits for Song flags1
#define XBF_reserved2				0x80
#define XBF_terminateDecay			0x40
#define XBF_interpolateSong			0x20
#define XBF_interpolateLead			0x10
#define XBF_fileTrackFlag				0x08
#define XBF_enableMIDIProgram		0x04
#define XBF_disableClickRemoval		0x02
#define XBF_useLeadForAllVoices		0x01

// bits for Song flags2
#define XBF_reserved3				0x80
#define XBF_reserved4				0x40
#define XBF_reserved5				0x20
#define XBF_masterEnablePitchRandomness 0x10
#define XBF_ampScaleLead			0x08
#define XBF_forceAmpScale			0x04
#define XBF_masterEnableAmpScale		0x02
#define XBF_reserved6				0x01

// Song resource
struct SongResource
{
	short int		midiResourceID;
	char			leadINSTID;
	char			reverbType;
	unsigned short	songTempo;
	char			reserved_0;
	char			songPitchShift;
	char			maxEffects;
	char			maxNotes;
	short int		maxNormNotes;
	unsigned char	flags1;			// see XBF for flags1
	char			noteDecay;
	char			defaultPercusionProgram;		// yes, I wanted signed!
	unsigned char	flags2;			// see XBF for flags2
	short int		remapCount;
	Remap		remaps[1];			// variable
	unsigned char	copyright;			// variable
	unsigned char	author;			// variable
};
typedef struct SongResource SongResource;

struct KeySplit
{
	char			lowMidi;
	char			highMidi;
	short int		sndResourceID;
	short int		smodParameter1;
	short int		smodParameter2;
};
typedef struct KeySplit KeySplit;

// bits for Instrument flags1
#define ZBF_enableInterpolate		0x80
#define ZBF_enableAmpScale		0x40
#define ZBF_disableSndLooping		0x20
#define ZBF_reserved_1			0x10
#define ZBF_useSampleRate		0x08
#define ZBF_sampleAndHold		0x04
#define ZBF_extendedFormat		0x02
#define ZBF_avoidReverb			0x01
// bits for Instrument flags2
#define ZBF_neverInterpolate		0x80
#define ZBF_playAtSampledFreq	0x40
#define ZBF_fitKeySplits			0x20
#define ZBF_enableSoundModifier	0x10
#define ZBF_useINSTforSplits		0x08
#define ZBF_notPolyphonic		0x04
#define ZBF_enablePitchRandomness	0x02
#define ZBF_playFromSplit		0x01

#define SET_FLAG_VALUE(oldflag, newflag, value)		(value) ? ((oldflag) | (newflag)) : ((oldflag) & ~(newflag))
#define TEST_FLAG_VALUE(flags, flagbit)				((flags) & (flagbit)) ? TRUE : FALSE

// Special Instrument resource. This can only be used when there is no tremolo data, or key splits
#if 0 //X_HARDWARE_PLATFORM == X_WEBTV
struct InstrumentResource __attribute__ ((packed))
{
	short		sndResourceID;
	short		midiRootKey ;
	char			panPlacement ;
	unsigned char	flags1 ;				// see ZBF bits for values
	unsigned char	flags2 ;				// see ZBF bits for values
	char			smodResourceID ;
	short		smodParameter1 ;
	short		smodParameter2 ;
	short		keySplitCount ;			// if this is non-zero, then KeySplit structure is inserted
	// to go beyond this point, if keySplitCount is non-zero, you must use function calls.
	short		tremoloCount ;			// if this is non-zero, then a Word is inserted.
	short		tremoloEnd ;			// Always 0x8000
	short		reserved_3 ;
	short		descriptorName ;		// Always 0
	short		descriptorFlags ;		// Always 0
};
#else
struct InstrumentResource
{
	short		sndResourceID ;
	short		midiRootKey ;
	char			panPlacement ;
	unsigned char	flags1 ;				// see ZBF bits for values
	unsigned char	flags2 ;				// see ZBF bits for values
	char			smodResourceID ;
	short		smodParameter1 ;
	short		smodParameter2 ;
	short		keySplitCount ;			// if this is non-zero, then KeySplit structure is inserted
	// to go beyond this point, if keySplitCount is non-zero, you must use function calls.
	short		tremoloCount ;			// if this is non-zero, then a Word is inserted.
	short		tremoloEnd ;			// Always 0x8000
	short		reserved_3 ;
	short		descriptorName ;		// Always 0
	short		descriptorFlags ;		// Always 0
};
#endif

typedef struct InstrumentResource InstrumentResource;
#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=reset
#endif

struct SampleDataInfo
{
	unsigned long	rate;				// sample rate
	unsigned long	frames;			// number of audio frames
	unsigned long	size;				// size in bytes
	unsigned long	loopStart;			// loop start frame
	unsigned long	loopEnd;			// loop end frame
	short int		bitSize;			// sample bit size; 8 or 16
	short int		channels;			// mono or stereo; 1 or 2
	short int		baseKey;			// base sample key
	short int		theID;			// sample ID if required
	void		*	pMasterPtr;		// master pointer if required
};
typedef struct SampleDataInfo SampleDataInfo;


#if X_HARDWARE_PLATFORM == X_MACINTOSH

#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=mac68k
#endif
// Used by kSetupCallbackProc
struct CallbackData
{
	void *	callbackProcPtr;
	long		callbackRefNum;
};
typedef struct CallbackData CallbackData;


struct PlayMemorySongData
{
	Handle	theSong;
	Handle	theMidi;
	short int	theSongID;
};
typedef struct PlayMemorySongData PlayMemorySongData;

#if PRAGMA_ALIGN_SUPPORTED
	#pragma options align=reset
#endif

// Internal private functions
OSErr InitSoundSystem(short int voices);
SMSBoolean InitVoices(short int voices);
void FinisSoundSystem(void);
void PauseSoundSystem(void);
void ResumeSoundSystem(void);

void SoundLock(short int theID, SMSBoolean flag);


void StopAllEffects(void);

#define SetupCallbackVars(theD)		MusicDriverHandler(kSetupCallbackProc, (long)(theD))


long GetSyncTime(void);
void SetSyncTime(long newTime);

SMSBoolean IsSoundManagerActive(void);
SMSBoolean IsCPUPowerPC(void);
SMSBoolean IsVirtualMemoryAvailable(void);

MDRVCodeBasePtr GetInterfaceAddress(void);

Handle DecompressHandle(Handle theData);

OSErr GetSoundResourceInformation(Handle theSnd, long *pLoopStart, long *pLoopEnd, 
									long *pSampleOffsetStart, long *pTotalSize, short *pBaseKey,
									short int *pNumChannels, short int *pBitSize,
									unsigned long *pRate,
									short int *pCompressionType);
void ProcessVBLTasks(void);


#endif

extern void EndSoundQuick(short int theID);


extern void GetKeySplitFromPtr(InstrumentResource *theX, short int entry, KeySplit *keysplit);
SongResource * NewSongPtr(	short int midiID,
						short int maxSongVoices, 
						short int maxNormalizedVoices, 
						short int maxEffectVoices,
						short int reverbType);

extern void DisposeSongPtr(SongResource *theSong);

extern void LZSSDeltaUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);
extern void LZSSUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);

// First byte is a compression type.
// Next 3 bytes is uncompressed length.
//
// Type 0 - Delta encoded LZSS
extern void *	DecompressSampleFormatPtr(void * pData, long dataSize);

// First 4 bytes is uncompressed length, followed by LZSS compressed data
extern void *	DecompressPtr(void * pData, long dataSize);

extern XPTR XGetSamplePtrFromSnd(XPTR pRes, SampleDataInfo *pInfo);

extern void XSetSoundLoopPoints(XPTR pRes, long loopStart, long loopEnd);
extern void XSetSoundSampleRate(XPTR pRes, unsigned long sampleRate);
extern void XSetSoundBaseKey(XPTR pRes, short int baseKey);


#endif	// _PRIVATE_SOUNDMUSICSYS_
