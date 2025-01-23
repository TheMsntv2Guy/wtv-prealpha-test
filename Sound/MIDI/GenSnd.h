#ifndef G_SOUND
#define G_SOUND
/*
***********************************************************************
**
** “GenSnd.h”
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**
**	© Copyright 1983-1996 by Steve Hales, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
**	No portion of this program (whether in source, object, or other form)
**	may be copied, duplicated, distributed, altered, or otherwise utilized
**	without direct consent of the copyright holder.
**
** Overview
**	The purpose of this layer of code is to remove Macintosh Specific code.
**	No file, or memory access. All functions are passed pointers to data
**	that needs to be passed into the mixer, and MIDI sequencer
**
** Modification History
**
**	4/6/93		Created
**	4/12/93		First draft ready
**	4/14/93		Added Waveform structure
**	7/7/95		Added Instrument API
**	11/7/95		Major changes, revised just about everything.
**	11/11/95	Added external queued midi links
**	11/20/95	Removed the BF_ flags, now you must walk through the union structure
**				Remove bit fields. BIT FIELDS DON'T WORK WITH MPW!!!!
**	12/5/95 (jn)	Added avoidReverb into instrument field; removed drop sample case
**	12/6/95		Move REVERB_TYPE from GENPRIV.H
**				Added GM_SetReverbType; removed extern references
**				Added ReverbType to GM_Song structure
**				Removed defaultPlaybackRate & defaultInterpolationMode from the GM_Song
**				structure
**	12/7/95		Moved DEFAULT_REVERB_TYPE from GenSnd.c
**				Added GM_GetReverbType
**	1/4/96		Added GM_ChangeSampleReverb for sound effects
**	1/7/96		Changed GM_BeginDoubleBuffer to use a 32 bit value for volume
**				Added GM_SetEffectsVolume & GM_GetEffectsVolume
**	1/13/96		Added extendedFormat bit to internal instrument format
**	1/18/96		Spruced up for C++ extra error checking
**	1/19/96		Changed GM_BeginSample to support bitsize and channels
**	1/29/96		Changed WaveformInfo to support FIXED_VALUE for sample rate
**				Added useSampleRate factor for playback of instruments & sampleAndHold bits
**	2/4/96		Added songMidiTickLength to GM_Song
**	2/5/96		Moved lots of variables from the MusicVars structure into
**				the GM_Song structure.
**				Changed GM_EndSong & GM_SongTicks & GM_IsSongDone & 
**				GM_SetMasterSongTempo to pass in a GM_Song pointer
**				Added GM_GetSongTickLength
**	2/12/96		Added GM_SetSongTickPosition
**				Added songMicrosecondLength to GM_Song structure
**	2/14/96		Added GM_StartLiveSong
**	2/18/96		Added panPlacement to the GM_Instrument structure
**	2/29/96		Added trackMuted to GM_Song structure
**	3/5/96		Added MAX_SONG_VOLUME
**	4/15/96		Added support to interpret SONG resource via GM_MergeExternalSong
**
********************************************************************
*/
#include "Machine.h"

/* System defines */

/* Used in InitGeneralSound */

// Quality types
enum
{
	Q_11K = 0,
	Q_22K,
	Q_44K
};
typedef long Quality;

// Modifier types
#define M_NONE		0L
#define M_USE_16		(1<<0L)
#define M_USE_STEREO	(1<<1L)
typedef long AudioModifiers;

// Interpolation types
enum
{
	E_AMP_SCALED_DROP_SAMPLE = 0,
	E_2_POINT_INTERPOLATION,
	E_LINEAR_INTERPOLATION
};
typedef long TerpMode;

enum 
{
	REVERB_TYPE_1 = 1,		// None
	REVERB_TYPE_2,			// Igor's Closet
	REVERB_TYPE_3,			// Igor's Garage
	REVERB_TYPE_4,			// Igor’s Acoustic Lab
	REVERB_TYPE_5,			// Igor's Cavern
	REVERB_TYPE_6			// Igor's Dungeon
};
typedef long ReverbMode;
#define DEFAULT_REVERB_TYPE	REVERB_TYPE_4


enum 
{
	SCAN_NORMAL = 0,			// normal Midi scan
	SCAN_SAVE_PATCHES,		// save patches during Midi scan. Fast
	SCAN_DETERMINE_LENGTH	// calculate tick length. Slow
};
typedef long ScanMode;




#define SYMPHONY_SIZE			32		// max voices at once
#define MAX_INSTRUMENTS		128		// MIDI number of programs per patch bank
#define MAX_BANKS				4		// two GM banks; two user banks
#define MAX_TRACKS			64		// max MIDI file tracks to process
#define MAX_CHANNELS			20		// max MIDI channels + one extra for sound effects + 3 for alignment (!!)
#define MAX_CONTROLLERS		128		// max MIDI controllers
#define MAX_SONG_VOLUME		127

/* Common errors returned from the system */
enum
{
	NO_ERR = 0,
	PARAM_ERR,
	MEMORY_ERR,
	BAD_INSTRUMENT,
	BAD_SAMPLE,
	BAD_RATE,
	BAD_MIDI_DATA,
	ALREADY_PAUSED,
	ALREADY_RESUMED,
	NO_SONG_PLAYING,
	TOO_MANY_SONGS_PLAYING
};
typedef short int OPErr;

typedef BOOL_FLAG	(*GM_LoopDoneCallbackPtr)(INT32 refnum);
typedef void		(*GM_DoubleBufferCallbackPtr)(INT32 refnum, G_PTR pWhichBufferFinished, INT32 *pBufferSize);
typedef void		(*GM_SoundDoneCallbackPtr)(INT32 refnum);
typedef void		(*GM_ControlerCallbackPtr)(short int channel, short int controler, short int value);
typedef pascal void	(*GM_SongCallbackProcPtr)(short int theID);



#if CPU_TYPE == kRISC
	#pragma options align=power
#endif

struct KeymapSplit
{
	BYTE						lowMidi;
	BYTE						highMidi;
	INT16					smodParameter1;
	INT16					smodParameter2;
	struct GM_Instrument FAR *	pSplitInstrument;
};
typedef struct KeymapSplit KeymapSplit;

// Flags for ADSR module
#define ADSR_LINEAR_RAMP 		1	//'LINE'
#define ADSR_EXPONENTIAL_CURVE 2	//'CURV'
#define ADSR_SUSTAIN 			3	//'SUST'
#define ADSR_TERMINATE		4	//'LAST'
#define ADSR_GOTO				5	//'GOTO'
#define ADSR_GOTO_CONDITIONAL	6	//'GOST'
#define ADSR_RELEASE			7	//'RELS'
#define ADSR_STAGES			8

struct ADSRRecord
{
	INT32			currentTime;
	INT32			currentPosition;
	INT32			currentLevel;
	INT32			mode;
	INT32			previousTarget;
	INT32			sustainingDecayLevel;
	INT32			ADSRLevel[ADSR_STAGES];
	INT16			ADSRTime[ADSR_STAGES];
	BYTE				ADSRFlags[ADSR_STAGES];
};

typedef struct ADSRRecord ADSRRecord;

// kinds of LFO modules
#define VOLUME_LFO				QuadChar('V','O','L','U')
#define PITCH_LFO				QuadChar('P','I','T','C')
#define STEREO_PAN_LFO			QuadChar('S','P','A','N')
#define STEREO_PAN_NAME2		QuadChar('P','A','N',' ')
#define LPF_FREQUENCY			QuadChar('L','P','F','R')

// kinds of LFO wave shapes
#define SINE_WAVE 				QuadChar('S','I','N','E')
#define TRIANGLE_WAVE 			QuadChar('T','R','I','A')
#define SQUARE_WAVE 			QuadChar('S','Q','U','A')
#define SQUARE_WAVE2 			QuadChar('S','Q','U','2')
#define SAWTOOTH_WAVE 		QuadChar('S','A','W','T')
#define SAWTOOTH_WAVE2 		QuadChar('S','A','W','2')

// Additional elements used by Curve functions
#define LOWPASS_AMOUNT		QuadChar('L','P','A','M')
#define VOLUME_LFO_FREQUENCY 	QuadChar('V','O','L','F')
#define PITCH_LFO_FREQUENCY 	QuadChar('P','I','T','F')
#define NOTE_VOLUME			QuadChar('N','V','O','L')
#define VOLUME_ATTACK_TIME 	QuadChar('A','T','I','M')
#define VOLUME_ATTACK_LEVEL 	QuadChar('A','L','E','V')
#define SUSTAIN_RELEASE_TIME 	QuadChar('S','U','S','T')
#define SUSTAIN_LEVEL			QuadChar('S','L','E','V')
#define RELEASE_TIME			QuadChar('R','E','L','S')
#define WAVEFORM_OFFSET		QuadChar('W','A','V','E')


struct LFORecordType
{
	INT32	period;
	INT32	level;
	INT32	mode;
	INT32	currentTime, LFOcurrentTime;
	INT32	currentWaveValue;
	INT32	where_to_feed;
	INT32	DC_feed;
	INT32	waveShape;
	ADSRRecord a;
};
typedef struct LFORecordType LFORecordType;

struct CurveEntry
{
	INT16	from_Value;
	INT16	to_Scalar;
};
typedef struct CurveEntry CurveEntry;
struct CurveRecord
{
	INT32		tieFrom;
	INT32		tieTo;
	INT16		curveCount;
	CurveEntry	curveData[6];	// set larger if need be.
};
typedef struct CurveRecord CurveRecord;

struct KeymapSplitInfo
{
	UINT16 		defaultInstrumentID;
	UINT16		KeymapSplitCount;
	KeymapSplit	keySplits[1];
};
typedef struct KeymapSplitInfo KeymapSplitInfo;

struct WaveformInfo
{
	INT16		waveformID;		// extra specific data
	INT16		bitSize;			// number of bits per sample
	INT16		channels;			// number of channels (1 or 2)
	INT16		baseMidiPitch;		// base Midi pitch of recorded sample ie. 60 is middle 'C'
	INT32		waveSize;			// total waveform size in bytes
	INT32		waveFrames;		// number of frames
	INT32		startLoop;			// start loop point offset
	INT32		endLoop;			// end loop point offset
	FIXED_VALUE	sampledRate;		// FIXED_VALUE 16.16 value for recording
	INT16		noteDecayPref;		// Note Decay in 1/60th of release
	BYTE FAR *	theWaveform;		// array data that morphs into what ever you need
};
typedef struct WaveformInfo WaveformInfo;


// Internal Instrument structure
struct GM_Instrument
{
	INT16				masterRootKey;
	INT16				smodResourceID;
	INT16				smodParameter1;
	INT16				smodParameter2;
	INT16				panPlacement;			// inital stereo pan placement of this instrument
	INT16				usageReferenceCount;	// number of references this instrument is associated to
	BOOL_FLAG	/*0*/	enableInterpolate;		// enable interpolation of instrument
	BOOL_FLAG	/*1*/	disableSndLooping;		// Disable waveform looping
	BOOL_FLAG	/*2*/	neverInterpolate;		// seems redundant: Never interpolate
	BOOL_FLAG	/*3*/	playAtSampledFreq;		// Play instrument at sampledRate only
	BOOL_FLAG	/*4*/	doKeymapSplit;			// If TRUE, then this instrument is a keysplit defination
	BOOL_FLAG	/*5*/	notPolyphonic;			// if FALSE, then instrument is a mono instrument
	BOOL_FLAG	/*6*/	enablePitchRandomness;	// if TRUE, then enable pitch variance feature
	BOOL_FLAG	/*7*/	avoidReverb;			// if TRUE, this instrument is not mixed into reverb unit
	BOOL_FLAG	/*0*/	enableSoundModifier;
	BOOL_FLAG	/*1*/	extendedFormat;		// extended format instrument
	BOOL_FLAG	/*2*/	sampleAndHold;
	BOOL_FLAG	/*3*/	useSampleRate;			// factor in sample rate into pitch calculation
	BOOL_FLAG	/*4*/	reserved_1;
	BOOL_FLAG	/*5*/	reserved_2;
	BOOL_FLAG	/*6*/	reserved_3;
	BOOL_FLAG	/*7*/	reserved_4;
	ADSRRecord			volumeADSRRecord;
	INT32				LPF_frequency;
	INT32				LPF_resonance;
	INT32				LPF_lowpassAmount;
	INT32				LFORecordCount;
	LFORecordType				LFORecord[8];
	INT32				curveRecordCount;
	CurveRecord			curve[4];
	union
	{
		KeymapSplitInfo	k;
		WaveformInfo		w;
	} u;
};
typedef struct GM_Instrument GM_Instrument;

// Internal Song structure
struct GM_Song
{
	INT16				songID;
	INT16				maxSongVoices;
	INT16				maxNormalizedVoices;
	INT16				maxEffectVoices;
	FIXED_VALUE			MasterTempo;				// master midi tempo (fixed point)
	UINT16				songTempo;				// tempo (16667 = 1.0)
	INT16				songPitchShift;				// master pitch shift

	GM_SongCallbackProcPtr	songEndCallbackPtr;

	ReverbMode			defaultReverbType;

	ScanMode				AnalyzeMode;				// analyze mode

	BOOL_FLAG	/* 0 */	allowProgramChanges;
	BOOL_FLAG	/* 1 */	loopSong;					// loop song when done
	BOOL_FLAG	/* 2 */	disableClickRemoval;
	BOOL_FLAG	/* 3 */	enablePitchRandomness;
	BOOL_FLAG	/* 4 */	disposeMidiDataWhenDone;	// if TRUE, then free midi data
	BOOL_FLAG	/* 5 */	SomeTrackIsAlive;			// song still alive
	BOOL_FLAG	/* 6 */	songFinished;				// TRUE at start of song, FALSE and end
	BOOL_FLAG	/* 7 */	terminateDecay;

	INT16				songVolume;
	INT16				defaultPercusionProgram;		// default percussion program for percussion channel. -1 means GM style bank select

	INT16				songLoopCount;				// current loop counter. Starts at 0
	INT16				songMaxLoopCount;			// when songLoopCount reaches songMaxLoopCount it will be set to 0

	UINT32				songMidiTickLength;			// song midi tick length. -1 not calculated yet.
	UINT32				songMicrosecondLength;		// song microsecond length. -1 not calculated yet.

	void					*midiData;				// pointer to midi data for this song

	//	instrument array. These are the instruments that are used by just this song
	GM_Instrument			*instrumentData[MAX_INSTRUMENTS*MAX_BANKS];

	INT16				instrumentRemap[(MAX_INSTRUMENTS*MAX_BANKS)];

	INT16				firstChannelBank[MAX_CHANNELS];		// set during preprocess. this is the program
	INT16				firstChannelProgram[MAX_CHANNELS];	// to be set at the start of a song

// channel based controler values
	BYTE					channelRegisteredParameterLSB[MAX_CHANNELS];	// Registered Parameter least signifcant byte
	BYTE					channelRegisteredParameterMSB[MAX_CHANNELS];	// Registered Parameter most signifcant byte
	UBYTE				channelSustain[MAX_CHANNELS];				// sustain pedal on/off
	UBYTE				channelVolume[MAX_CHANNELS];				// current channel volume
	UBYTE				channelExpression[MAX_CHANNELS];			// current channel expression
	UBYTE				channelPitchBendRange[MAX_CHANNELS];			// current bend range in half steps
	UBYTE				channelReverb[MAX_CHANNELS];				// current channel reverb
	UBYTE				channelModWheel[MAX_CHANNELS];				// Mod wheel (primarily affects pitch bend)
	INT16				channelBend[MAX_CHANNELS];					// MUST BE AN INT16!! current amount to bend new notes
	INT16				channelProgram[MAX_CHANNELS];				// current channel program
	INT16				channelBank[MAX_CHANNELS];					// current bank
	INT16				channelStereoPosition[MAX_CHANNELS];			// current channel stereo position

// internal timing variables for sequencer
	INT32				MicroJif;
	INT32				UnscaledMIDITempo;
	INT32				MIDITempo;
	INT32				MIDIDivision;
	INT32				UnscaledMIDIDivision;
	UINT32				CurrentMidiClock;

	UINT32				songMicrosecondIncrement;
	UINT32				songMicroseconds;

// internal position variables for sequencer
	BOOL_FLAG			trackMuted[MAX_TRACKS];		// track mute control
	UINT32				tracklen[MAX_TRACKS];
	BOOL_FLAG			trackon[MAX_TRACKS];
	UBYTE				*ptrack[MAX_TRACKS];
	UBYTE				*trackstart[MAX_TRACKS];
	INT32				trackticks[MAX_TRACKS];
	INT32				runningStatus[MAX_TRACKS];
};
typedef struct GM_Song GM_Song;

#if CPU_TYPE == kRISC
	#pragma options align=reset
#endif

// Functions


/**************************************************/
/*
** FUNCTION InitGeneralSound(Quality theQuality, 
**								TerpMode theTerp, INT16 maxVoices,
**								INT16 normVoices, INT16 maxEffects,
**								INT16 maxChunkSize)
**
** Overvue --
**	This will setup the sound system, allocate memory, and such,
**  so that any calls to play effects or songs can happen right
**  away.
**
** Private notes:
**  This code will preinitialize the MIDI sequencer, allocate
**  amplitude scaling buffer & init it, and init the
**  General Magic sound system.
**
**	INPUT	--	theQuality
**					Q_11K	Sound ouput is FIXED_VALUE at 11127
**					Q_22K	Sound output is FIXED_VALUE at 22254
**			--	theTerp
**					Interpolation type
**			--	use16bit
**					If true, then hardware will be setup for 16 bit output
**			--	maxVoices
**					maximum voices
**			--	normVoices
**					number of voices normally. ie a gain
**			--	maxEffects
**					number of voices to be used as effects
**
**	OUTPUT	--	
**
** NOTE:	
**	Only call this once.
*/
/**************************************************/
OPErr GM_InitGeneralSound(Quality theQuality, TerpMode theTerp, AudioModifiers theMods,
				INT16 maxVoices, INT16 normVoices, INT16 maxEffects);

/**************************************************/
/*
** FUNCTION FinisGeneralSound;
**
** Overvue --
**	This will release any memory allocated by InitGeneralSound, clean up.
**
**	INPUT	--	
**	OUTPUT	--	
**
** NOTE:	
**	Only call this once.
*/
/**************************************************/
void GM_FinisGeneralSound(void);

OPErr GM_ChangeSystemVoices(INT16 maxVoices, INT16 normVoices, INT16 maxEffects);

OPErr GM_ChangeAudioModes(Quality theQuality, TerpMode theTerp, AudioModifiers theMods);

/**************************************************/
/*
** FUNCTION PauseGeneralSound;
**
** Overvue --
**	This is used to pause the system, and release hardware for other tasks to
**	play around. Call ResumeGeneralSound when ready to resume working with
**	the system.
**
**	INPUT	--	
**	OUTPUT	--	OPErr	Errors in pausing system.
**
** NOTE:	
*/
/**************************************************/
OPErr GM_PauseGeneralSound(void);

OPErr GM_PauseSequencer(void);
OPErr GM_ResumeSequencer(void);

/**************************************************/
/*
** FUNCTION ResumeGeneralSound;
**
** Overvue --
**	This is used to resume the system, and take over the sound hardware. Call this
**	after calling PauseGeneralSound.
**
**	INPUT	--	
**	OUTPUT	--	OPErr	Errors in resuming. Continue to call until errors have
**						stops, or alert user that something is still holding
**						the sound hardware.
**
** NOTE:	
*/
/**************************************************/
OPErr GM_ResumeGeneralSound(void);

/**************************************************/
/*
** FUNCTION BeginSong(GM_Song *theSong, GM_SongCallbackProcPtr theCallbackProc);
**
** Overvue --
**	This will start a song, given the data structure that contains the midi data,
**	instrument data, and specifics about playback of this song.
**
**	INPUT	--	theSong,			contains pointers to the song data, midi data,
**								all instruments used in this song.
**				theCallbackProc,	when the song is done, even in the looped
**								case, this procedure will be called.
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
OPErr GM_BeginSong(GM_Song *theSong, GM_SongCallbackProcPtr theCallbackProc);

GM_Song * GM_LoadSong(short int songID, void *theExternalSong, void *theExternalMidiData, long midiSize, 
					short int *pInstrumentArray, short int loadInstruments, OPErr *pErr);
GM_Song * GM_CreateLiveSong(short int songID);
OPErr GM_StartLiveSong(GM_Song *pSong, BOOL_FLAG loadPatches);

/**************************************************/
/*
** FUNCTION EndSong;
**
** Overvue --
**	This will end the current song playing.
**  All song resources not be will be disposed or released.
**
**	INPUT	--	
**	OUTPUT	--	
**
** NOTE:	
*/
/*************************************************/
void GM_EndSong(GM_Song *pSong);

void GM_FreeSong(GM_Song *theSong);

void GM_MergeExternalSong(void *theExternalSong, long theSongID, GM_Song *theSong);

/**************************************************/
/*
** FUNCTION SongTicks;
**
** Overvue --
**	This will return in 1/60th of a second, the count since the start of the song
**	currently playing.
**
**	INPUT	--	
**	OUTPUT	--	INT32,		returns ticks since BeginSong.
**						returns 0 if no song is playing.
**
** NOTE:	
*/
/**************************************************/
INT32 GM_SongTicks(GM_Song *pSong);

INT32 GM_SongMicroseconds(GM_Song *pSong);

// Return the length in MIDI ticks of the song passed
//	pSong	GM_Song structure. Data will be cloned for this function.
//	pErr		OPErr error type
INT32 GM_GetSongTickLength(GM_Song *pSong, OPErr *pErr);

OPErr GM_SetSongTickPosition(GM_Song *pSong, INT32 songTickPosition);

// Return the used patch array of instruments used in the song passed.
//	theExternalSong	standard SONG resource structure
//	theExternalMidiData	if not NULL, then will use this midi data rather than what is found in external SONG resource
//	midiSize			size of midi data if theExternalMidiData is not NULL
//	pInstrumentArray	array, if not NULL will be filled with the instruments that need to be loaded.
//	pErr				pointer to an OPErr
INT32 GM_GetUsedPatchlist(void *theExternalSong, void *theExternalMidiData, long midiSize, 
					short int *pInstrumentArray, OPErr *pErr);

/**************************************************/
/*
** FUNCTION IsSongDone;
**
** Overvue --
**	This will return a BOOL_FLAG if a song is done playing or not.
**
**	INPUT	--	
**	OUTPUT	--	BOOL_FLAG,	returns TRUE if song is done playing,
**							or FALSE if not playing.
**
** NOTE:	
*/
/**************************************************/
BOOL_FLAG GM_IsSongDone(GM_Song *pSong);

/**************************************************/
/*
** FUNCTION BeginSample(G_PTR theData, INT32 theSize, INT32 theRate, 
**						INT32 playTime,
**						INT32 theStartLoop, INT32 theEndLoop, 
**						BOOL_FLAG (*theLoopContinueProc)(void), 
**						void *theCallbackProc);
**
** Overvue --
**	This will play a sampled sound effect mixed into the current song being played.
**
**	INPUT	--	theData,		8 bit unsigned data
**				theSize,		length of sampled sound
**				theRate,		FIXED_VALUE value for sample playback, in 16.16 format
**				theStartLoop,	the starting sample of the loop point
**				theEndLoop,	the ending sample of the loop point
**				theLoopContinueProc,
**							if not NULL, then the sample will continualy play
**							the sample until this function returns FALSE, then
**							proceed to play the rest of the sample
**				theCallBackProc,
**							when the sample is finished being played, this
**							procedure will be called
**
**	OUTPUT	--	ref,		a sound reference number that is unique to 
**							every sample that is currently being played
**
** NOTE:	
*/
/**************************************************/
INT32 GM_BeginSample(G_PTR theData, INT32 theSize, INT32 theRate, 
						INT32 theStartLoop, INT32 theEndLoop, 
						INT32 sampleVolume, INT32 stereoPosition,
						INT32 refData, INT16 bitSize, INT16 channels, 
						GM_LoopDoneCallbackPtr theLoopContinueProc,
						GM_SoundDoneCallbackPtr theCallbackProc);

INT32 GM_BeginDoubleBuffer(G_PTR pBuffer1, G_PTR pBuffer2, INT32 size, INT32 rate,
							INT16 bitSize, INT16 channels,
							INT32 sampleVolume, INT16 stereoPosition,
							INT32 refData,
							GM_DoubleBufferCallbackPtr bufferCallback);


/**************************************************/
/*
** FUNCTION EndSample;
**
** Overvue --
**	This will end the sample by the reference number that is passed.
**
**	INPUT	--	theRef,	a reference number that was returned
**						by BeginSound
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
void GM_EndSample(INT32 theRef);

/**************************************************/
/*
** FUNCTION IsSoundDone;
**
** Overvue --
**	This will return status of a sound that is being played.
**
**	INPUT	--	theRef,		a reference number that was returned
**							by BeginSound
**	OUTPUT	--	BOOL_FLAG	TRUE if sound is done playing
**							FALSE if sound is still playing
**
** NOTE:	
*/
/***********************************************/
BOOL_FLAG GM_IsSoundDone(INT32 theRef);

/*************************************************/
/*
** FUNCTION SetMasterVolume;
**
** Overvue --
**	This will set the master output volume of the mixer.
**
**	INPUT	--	theVolume,	0-256 which is the master volume.
**	OUTPUT	--	
**
** NOTE:	
**	This is different that the hardware volume. This will scale the output by
**	theVolume factor.
**	There is somewhat of a CPU hit, while calulating the new scale buffer.
*/
/*************************************************/
void GM_SetMasterVolume(INT32 theVolume);

INT32 GM_GetMasterVolume(void);

/**************************************************/
/*
** FUNCTION ChangeSamplePitch(INT16 theRef, INT32 theNewRate);
**
** Overvue --
**	This will change the current sample referenced by theRef, to the new sample
**	rate of theNewRate.
**
**	INPUT	--		theRef,		a reference number returned from BeginSound
**					theNewRate,	FIXED_VALUE value for sample playback, in 16.16 format
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
void GM_ChangeSamplePitch(INT32 theRef, INT32 theNewRate);

void GM_ChangeSampleVolume(INT32 theRef, INT16 newVolume);

void GM_ChangeSampleStereoPosition(INT32 theRef, INT16 newStereoPosition);

void GM_ChangeSampleReverb(INT32 theRef, INT16 enable);

/**************************************************/
/*
** FUNCTION GM_EndAllSoundEffects;
**
** Overvue --
**	This will end all sound effects from the system.  It
**	does not shut down sound hardware or deallocate
** 	memory used by the music system.
**
**	INPUT	--	
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
void GM_EndAllSoundEffects(void);
void GM_EndAllNotes(void);

/**************************************************/
/*
** FUNCTION SetMasterSongTempo(INT32 newTempo);
**
** Overvue --
**	This will set the master tempo for the currently playing song.
**
**	INPUT	--	newTempo is in microseconds per MIDI quater-note.
**				Another way of looking at it is, 24ths of a microsecond per
**				MIDI clock.
**	OUTPUT	--	NO_SONG_PLAYING is returned if there is no song playing
**
** NOTE:	
*/
/**************************************************/
OPErr GM_SetMasterSongTempo(GM_Song *pSong, INT32 newTempo);

// Instrument API

OPErr	GM_LoadInstrument(INT32 instrument);
OPErr	GM_UnloadInstrument(INT32 instrument);
void		GM_FlushInstrumentCache(BOOL_FLAG startStopCache);
OPErr	GM_RemapInstrument(INT32 from, INT32 to);

/**************************************************/
/*
** FUNCTION GM_LoadSongInstruments(GM_Song *theSong)
**
** Overvue --
**	This will load the instruments required for this song to play
**
**	INPUT	--	theSong,	a pointer to the GM_Song data containing the MIDI Data
**			--	pArray,	an array that will be filled with the instruments that are
**						loaded, if not NULL.
**			--	loadInstruments, if TRUE will load instruments and samples
**
**	OUTPUT	--	OPErr,		an error will be returned if the song
**							cannot be loaded
**
** NOTE:	
*/
/**************************************************/
OPErr	GM_LoadSongInstruments(GM_Song *theSong, short int *pArray, short int loadInstruments);

OPErr	GM_UnloadSongInstruments(GM_Song *theSong);


void		GM_SetSongLoopFlag(GM_Song *theSong, BOOL_FLAG loopSong);

void		GM_SetSongVolume(GM_Song *theSong, INT16 newVolume);
INT16	GM_GetSongVolume(GM_Song *theSong);


// range is 0 to MAX_MASTER_VOLUME (256)
void		GM_SetEffectsVolume(INT16 newVolume);
INT16	GM_GetEffectsVolume(void);

INT16	GM_ConvertPatchBank(INT16 thePatch, INT16 theBank, INT16 theChannel);

BOOL_FLAG GM_IsInstrumentRangeUsed(INT16 thePatch, INT16 theLowKey, INT16 theHighKey);
BOOL_FLAG GM_IsInstrumentUsed(INT16 thePatch, INT16 theKey);
void GM_SetUsedInstrument(INT16 thePatch, INT16 theKey, BOOL_FLAG used);
void GM_SetInstrumentUsedRange(INT16 thePatch, BYTE *pUsedArray);
void GM_GetInstrumentUsedRange(INT16 thePatch, BYTE *pUsedArray);

void GM_SetSongCallback(GM_Song *theSong, GM_SongCallbackProcPtr songEndCallbackPtr);

void GM_SetControllerCallback(short int controller, GM_ControlerCallbackPtr controllerCallback);

INT16 GM_GetAudioSampleFrame(INT16 *pLeft, INT16 *pRight);

// Set the global reverb type
void GM_SetReverbType(ReverbMode theReverbMode);

// Get the global reverb type
ReverbMode GM_GetReverbType(void);

// External MIDI links
void QGM_NoteOn(INT32 timeStamp, INT16 channel, INT16 note, INT16 velocity);
void QGM_NoteOff(INT32 timeStamp, INT16 channel, INT16 note, INT16 velocity);
void QGM_ProgramChange(INT32 timeStamp, INT16 channel, INT16 program);
void QGM_PitchBend(INT32 timeStamp, INT16 channel, BYTE valueMSB, BYTE valueLSB);
void QGM_Controller(INT32 timeStamp, INT16 channel, INT16 controller, INT16 value);
void QGM_LockExternalMidiQueue(void);
void QGM_UnlockExternalMidiQueue(void);
void QGM_AllNotesOff(void);

#endif /* G_SOUND */



