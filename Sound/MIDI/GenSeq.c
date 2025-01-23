/*
************************************************************************
**
** “GenSeq.c”
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
**	Contains only MIDI Sequencer
**
** Modification History:
**
**	8/19/93		Split from Synthesier.c
**	11/7/95		Major changes, revised just about everything.
**	11/11/95	Started implementing exteral midi controls
**				Created Q_MIDIEvent queue system.
**				Created external GM_ interface to midi commands that can be added to queue
**				Created midi queue processor
**				Added semaphore locking of queue processing
**	11/15/95	Fixed bug with GM_GetSongVolume. Wrong values
**	12/4/95		Added DISABLE_QUEUE as a debugging feature
**	12/5/95 (jn)	Modified to support reverb; 32 bit in GM_GetAudioSampleFrame
**	12/6/95 (sh)	Moved function GM_GetAudioSampleFrame into GenSynth.c
**				Put GM_SetReverbType into PV_ConfigureMusic
**	12/7/95		Added channelReverb to PV_ResetControlers
**				Added PV_SetChannelReverb
**	1/7/96		Added GM_SetEffectsVolume & GM_GetEffectsVolume
**	1/12/96		Fixed bug with GM_BeginSong that would not set up the voices at song start up
**	1/18/96		Implemented GM patch bank loading for percussion and other banks
**	1/23/96		Added GM_FreeSong
**	1/25/96		Moved GM_GetUsedPatchlist to GenSong.c
**				Fixed bug with default percussion forcing a load of instrument 0
**	1/28/96		Moved GM_FreeSong to GenSong.c
**	2/5/96		Removed unused variables. Working towards multiple songs
**	2/12/96		Moved cleaning of external midi queue to Init code
**				Removed extra system setup for processing songs
**				GM_BeginSong now does even more, since its suppose to start a song
**	2/13/96		GM_BeginSong will now try to play more than one song. It puts it into the queue
**				Lots of multi-song support.
**				Moved sequencer slice control code from GenSynth.c
**	2/18/96		Fixed bug that was ignoring controller events and stuff during a position scan
**	2/29/96		Added support for looping controls
**	3/1/96		Changed static variables to static const
**	3/2/96		Fixed bug with reading sysex data wrong
**	3/5/96		Changed GM_GetSongVolume & GM_SetSongVolume to only use songVolume
**				Eliminated the global songVolume 
**	3/21/96		Started implementing Igor meta events
**	3/25/96		Removed private PV_Get4Type to XGetLong
**	3/28/96		Fixed lock up bug that I created when reading meta events!
**	4/8/96		Fixed key split problem during meta instrument load
**	4/11/96		Fixed a memory leak caused by instruments not being set into the cache correctly
**	4/15/96		Added resource name into eMidi data structure
**				Added support to interpret SONG resource via eMidi
**	4/21/96		Added GM_GetRealtimeAudioInformation
**	5/12/96		Added support for csnd in meta events
**				Added some failsafe support in PV_ProcessIgorMeta
**
**
************************************************************************
*/

#include "GenSnd.h"
#include "GenPriv.h"
#include "limits.h"

#define IGOR_SYSEX_ID	0x0000010D

/*
March 20, 1996

Igor's Software Laboratories
Steve Hales
882 Hagemann Drive
Livermore CA 94550      
Tel: (510) 449-1947
Fax: (510) 449-1342


Dear Member:

This letter confirms our receipt of your application for a System Exclusive ID 
number and your payment of the processing fee. You have been assigned the MIDI 
System Exclusive ID number: 

        ID # :  00H 01H 0DH
*/

INT16 GM_ConvertPatchBank(INT16 thePatch, INT16 theBank, INT16 theChannel)
{
// perc: 1 3 5 7, etc
// inst: 0 2 4 6, etc
	if (theChannel == PERCUSSION_CHANNEL)
	{
		theBank = (theBank * 2) + 1;		// odd banks are percussion
	}
	else
	{
		theBank = theBank * 2 + 0;		// even banks are for instruments
	}

	if (theBank < MAX_BANKS)
	{
		thePatch = (theBank * 128) + thePatch;
	}
	return thePatch;
}

// Scale the division amount by the current tempo:
static void PV_ScaleDivision(GM_Song *pSong, long div)
{
	register INT32	midiDivsion;

	midiDivsion = (div * 64) / (pSong->MIDITempo & 0xFFFFL);
	midiDivsion = (midiDivsion * pSong->MasterTempo) / 0x10000L;

	if (pSong->AnalyzeMode == SCAN_SAVE_PATCHES)
	{
		midiDivsion = 0x7FFF;
	}
	pSong->MIDIDivision = midiDivsion;
}


// Resets song controlers. Pass -1 to reset all channels, otherwise just the channel passed
// will be reset.
void PV_ResetControlers(GM_Song *pSong, INT16 channel2Reset)
{
	register LOOPCOUNT		count, max, start;
	register BOOL_FLAG		generateStereoOutput;
//		-60, -53, -46, -39, -32, -25, -18, -11, -4,
	static const INT16 stereoDefaultPositions[MAX_CHANNELS] =
	{
		40, -40, 30, -30, 20, -20, 10, -10, 5, 
		0, // percussion
		15, -15, 25, -25, 35, -35,
		0 // sound effects
	};

	generateStereoOutput = MusicGlobals->generateStereoOutput;	// cache this
	if (channel2Reset == -1)
	{
		// do all channels
		max = MAX_CHANNELS;
		start = 0;
	}
	else
	{
		// do just this one
		max = channel2Reset + 1;
		start = channel2Reset;
	}
	for (count = start; count < max; count++)
	{
		if (channel2Reset == -1)
		{	// if all channels, then reset default programs
			pSong->channelProgram[count] = count;
		}
		pSong->channelRegisteredParameterLSB[count] = UCHAR_MAX;
		pSong->channelRegisteredParameterMSB[count] = UCHAR_MAX;

		pSong->channelVolume[count] = MAX_NOTE_VOLUME;				// controler 7
		pSong->channelPitchBendRange[count] = DEFAULT_PITCH_RANGE;	// pitch bend controler
		pSong->channelExpression[count] = 0;	// no extra expression
		pSong->channelModWheel[count] = 0;
		pSong->channelBend[count] = 0;
		pSong->channelReverb[count] = 0;
		if (generateStereoOutput)
		{
			pSong->channelStereoPosition[count] = stereoDefaultPositions[count];
		}
		else
		{
			pSong->channelStereoPosition[count] = 0;
		}
		pSong->channelSustain[count] = FALSE;
	}
}

void PV_ConfigureInstruments(GM_Song *theSong)
{
	register short int	count;

	PV_ResetControlers(theSong, -1);

	for (count = 0; count < MAX_CHANNELS; count++)
	{
		if (theSong->firstChannelProgram[count] != -1)
		{
			theSong->channelProgram[count] = theSong->firstChannelProgram[count];
			theSong->channelBank[count] = theSong->firstChannelBank[count];
		}
	}

	// Any stereo instruments loaded, if so then don't use default pan positions just center
	if (PV_AnyStereoInstrumentsLoaded())
	{
		for (count = 0; count < MAX_CHANNELS; count++)
		{
			theSong->channelStereoPosition[count] = 0;
		}
	}
	if (theSong->defaultPercusionProgram == -1)
	{
		theSong->channelProgram[PERCUSSION_CHANNEL] = 0;
		theSong->channelBank[PERCUSSION_CHANNEL] = 0;
		theSong->firstChannelProgram[PERCUSSION_CHANNEL] = 0;
		theSong->firstChannelBank[PERCUSSION_CHANNEL] = 0;
	}
	else
	{
		if (theSong->defaultPercusionProgram)
		{
			theSong->channelProgram[PERCUSSION_CHANNEL] = theSong->defaultPercusionProgram;
		}
	}

// Set up the default division for the MIDI file:
	if (theSong->MasterTempo == 0L)
	{
		theSong->MasterTempo = (FIXED_VALUE) 0x10000;	// 1.0
	}

// Sample output at 22.254 khz
//	theSong->MicroJif = 16667;
//	theSong->UnscaledMIDITempo = 500000;
// Sample output at 22.050 khz

	theSong->MicroJif = 11610;	
//	theSong->MicroJif = 10565;	// 9% slower
	theSong->UnscaledMIDITempo = 495417;	// (22050/22254) * 500000

	theSong->songMicroseconds = 0;
	theSong->CurrentMidiClock = 0;
	theSong->songMicrosecondIncrement = theSong->MicroJif;

	theSong->MIDITempo = (theSong->UnscaledMIDITempo / theSong->MicroJif) & 0xFFFF;
	theSong->UnscaledMIDIDivision = 60;
	PV_ScaleDivision(theSong, theSong->MIDITempo);
}

// Configure the global synth variables from the passed song
OPErr PV_ConfigureMusic(GM_Song *theSong)
{
	register LOOPCOUNT		count;
	register UBYTE		*	scanptr;
	register INT16			numtracks;
	register UINT32		trackLength;

	PV_ConfigureInstruments(theSong);
	scanptr = (UBYTE *)theSong->midiData;
	if (scanptr == 0)
	{
		return BAD_MIDI_DATA;
	}
	for (count = 0; count < 5000; count++)
	{
		if (XGetLong(scanptr) == QuadChar('M','T','h','d'))
		{
			goto FoundMThd;
		}
		scanptr++;
	}
	return BAD_MIDI_DATA;
FoundMThd:
	count = (scanptr[12] << 8) + scanptr[13];
	theSong->UnscaledMIDIDivision = count;
	PV_ScaleDivision(theSong, count);

	for (count = 0; count < 3200; count++)
	{
		if (XGetLong(scanptr) == QuadChar('M','T','r','k'))
		{
			goto FoundMTrk;
		}
		scanptr++;
	}
	return BAD_MIDI_DATA;
FoundMTrk:
	numtracks = 0;
	while ( (numtracks < MAX_TRACKS) && (XGetLong(scanptr) == QuadChar('M','T','r','k')) )
	{
		// calculate length of this track.
		scanptr += 4;
		trackLength = *scanptr++;
		trackLength = (trackLength << 8) + *scanptr++;
		trackLength = (trackLength << 8) + *scanptr++;
		trackLength = (trackLength << 8) + *scanptr++;

		theSong->ptrack[numtracks] = scanptr;
		theSong->trackstart[numtracks] = scanptr;
		theSong->trackticks[numtracks] = 0;
		theSong->trackon[numtracks] = 'F';
		theSong->tracklen[numtracks] = trackLength;
		scanptr += trackLength;		
		numtracks++;
	}
	return NO_ERR;	
}

// Set up the system to start playing a song
OPErr GM_BeginSong(GM_Song *theSong, GM_SongCallbackProcPtr theCallbackProc)
{
	OPErr		theErr;
	short int		songSlot, count;

	theErr = NO_ERR;
	if (theSong)
	{
// first find a slot in the song queue
		songSlot = -1;
		for (count = 0; count < MAX_SONGS; count++)
		{
			if (MusicGlobals->pSongsToPlay[count] == NULL)
			{
				songSlot = count;
				break;
			}
		}
		if (songSlot != -1)
		{
			theSong->AnalyzeMode = SCAN_NORMAL;
			theSong->songEndCallbackPtr = theCallbackProc;
	
			theErr = PV_ConfigureMusic(theSong);
			if (theErr == NO_ERR)
			{	
				theSong->SomeTrackIsAlive = TRUE;
				theSong->songFinished = FALSE;
	
				theErr = GM_ChangeSystemVoices(theSong->maxSongVoices,
											theSong->maxNormalizedVoices,
											theSong->maxEffectVoices);
				if (theErr == NO_ERR)
				{
					// first time looping, and set mute tracks to off
					theSong->songLoopCount = 0;
					theSong->songMaxLoopCount = 0;
					for (count = 0; count < MAX_TRACKS; count++)
					{
						theSong->trackMuted[count] = FALSE;
					}

					// Set reverb type now.
					GM_SetReverbType(theSong->defaultReverbType);
	
					// Start song playing now.
					MusicGlobals->pSongsToPlay[songSlot] = theSong;
				}
			}
		}
		else
		{
			theErr = TOO_MANY_SONGS_PLAYING;
		}
	}
	return theErr;
}



#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
OPErr GM_PauseSequencer(void)
{
	if ( (MusicGlobals->systemPaused == FALSE) && (MusicGlobals->sequencerPaused == FALSE) )
	{
		MusicGlobals->sequencerPaused = TRUE;
		// stop all MIDI notes
		GM_EndAllNotes();
	}
	return NO_ERR;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
OPErr GM_ResumeSequencer(void)
{
	if ( (MusicGlobals->systemPaused == FALSE) && (MusicGlobals->sequencerPaused) )
	{
		MusicGlobals->sequencerPaused = FALSE;
	}
	return NO_ERR;
}
#endif

BOOL_FLAG GM_IsSongDone(GM_Song *pSong)
{
	register long	count;
	BOOL_FLAG	songDone;

	songDone = FALSE;
//	if ( (MusicGlobals->systemPaused == FALSE) && (MusicGlobals->sequencerPaused == FALSE) && (pSong) )
	if (pSong)
	{
		songDone = TRUE;
		for (count = 0; count < MAX_TRACKS; count++)
		{
			if (pSong->trackon[count])
			{
				songDone = FALSE;
				break;
			}
		}
	}
	return songDone;
}


static void PV_CallSongCallback(GM_Song *theSong, BOOL_FLAG clearCallback)
{
	pascal void (*theCallback)(short int theID);

	if (theSong)
	{
		theCallback = theSong->songEndCallbackPtr;
		if (theCallback)
		{
			if (clearCallback)
			{
				theSong->songEndCallbackPtr = NULL;
			}
			(*theCallback)(theSong->songID);
		}
	}
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_SetSongCallback(GM_Song *theSong, 	GM_SongCallbackProcPtr theCallback)
{
	if (MusicGlobals && theSong)
	{
		theSong->songEndCallbackPtr = theCallback;
	}
}
#endif


// Stop this song playing, or if NULL stop all songs playing
void GM_EndSong(GM_Song *pSong)
{
	register LOOPCOUNT count;

	if (pSong)
	{
		for (count = 0; count < MAX_SONGS; count++)
		{
			if (MusicGlobals->pSongsToPlay[count] == pSong)
			{
				MusicGlobals->pSongsToPlay[count] = NULL;
				if (pSong == MusicGlobals->theSongPlaying)
				{
					MusicGlobals->theSongPlaying = NULL;
				}
				break;
			}
		}
		for (count = 0; count < MAX_TRACKS; count++)
		{
			pSong->trackon[count] = 0;
		}
		PV_CallSongCallback(pSong, TRUE);
	}
	else
	{
		GM_EndAllNotes();
		for (count = 0; count < MAX_SONGS; count++)
		{
			MusicGlobals->pSongsToPlay[count] = NULL;
		}
		MusicGlobals->theSongPlaying = NULL;
		MusicGlobals->systemPaused = FALSE;
		MusicGlobals->sequencerPaused = FALSE;
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
// This will return realtime information about the current set of notes being playing right now.
void GM_GetRealtimeAudioInformation(GM_AudioInfo *pInfo)
{
	register MusicVars	*myMusicGlobals;
	register LOOPCOUNT	count, active;
	register NoteRecord *this_voice;

	myMusicGlobals = MusicGlobals;
	active = 0;
	for (count = 0; count < (myMusicGlobals->MaxNotes+myMusicGlobals->MaxEffects); count++)
	{
		this_voice = &myMusicGlobals->NoteEntry[count];
		if (this_voice->NoteDur >= 0)
		{
			pInfo->patch[active] = this_voice->NoteProgram;
			pInfo->scaledVolume[active] = this_voice->NoteVolume;
			pInfo->volume[active] = this_voice->NoteMIDIVolume;
			pInfo->channel[active] = this_voice->NoteChannel;
			pInfo->voice[active] = count;
			active++;
		}
	}
	pInfo->voicesActive = active;
	pInfo->maxNotesAllocated = myMusicGlobals->MaxNotes;
	pInfo->maxEffectsAllocated = myMusicGlobals->MaxEffects;
	pInfo->normalAllocated = myMusicGlobals->MaxNormNotes;
}
#endif

#ifdef SUPPORT_CONTROLLER_CALLBACKS
void GM_SetControllerCallback(short int controller, GM_ControlerCallbackPtr controllerCallback)
{
	if ( (MusicGlobals) && (controller < MAX_CONTROLLERS) )
	{
		MusicGlobals->channelCallbackProc[controller] = controllerCallback;
	}
}
#endif

void GM_SetSongLoopFlag(GM_Song *theSong, BOOL_FLAG loopSong)
{
	if (MusicGlobals && theSong)
	{
		theSong->loopSong = loopSong;
	}
}

// Set song volume. Range is 0 to 127. You can overdrive
void GM_SetSongVolume(GM_Song *theSong, INT16 newVolume)
{
	if (theSong)
	{
		if (newVolume < 0)
		{
			newVolume = 0;
		}
		if (newVolume > MAX_NOTE_VOLUME * 10)
		{
			newVolume = MAX_NOTE_VOLUME * 10;
		}
		theSong->songVolume = newVolume;
	}
}

// Get song volume
INT16 GM_GetSongVolume(GM_Song *theSong)
{
	INT16	volume;

	volume = MAX_NOTE_VOLUME;
	if (theSong)
	{
		volume = theSong->songVolume;
	}
	return volume;
}

// range is 0 to MAX_MASTER_VOLUME (256). Note volume is from 0 to MAX_NOTE_VOLUME (127)
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_SetEffectsVolume(INT16 newVolume)
{
	register MusicVars		*myMusicGlobals;
	register LOOPCOUNT		count;
	register NoteRecord *	theNote;

	myMusicGlobals = MusicGlobals;
	if (myMusicGlobals)
	{
		myMusicGlobals->effectsVolume = newVolume;
		newVolume = (newVolume * MAX_NOTE_VOLUME) / MAX_MASTER_VOLUME; // scale

		// update the current notes playing to the new volume
		for (count = 0; count < myMusicGlobals->MaxNotes; count++)
		{
			theNote = &myMusicGlobals->NoteEntry[count];
			if (theNote->NoteDur >= 0)
			{
				if (theNote->NoteChannel == SOUND_EFFECT_CHANNEL)
				{
					// make sure and set the channel volume not scaled, because its scaled later
					if (newVolume == 0)
					{
						theNote->NoteDur = 0;
						theNote->NoteDecay = 0;
						theNote->volumeADSRRecord.ADSRTime[0] = 1;
						theNote->volumeADSRRecord.ADSRFlags[0] = ADSR_TERMINATE;
						theNote->volumeADSRRecord.ADSRLevel[0] = 0;	// just in case
					}
					// now calculate the new volume based upon the current channel volume and
					// the unscaled note volume
					newVolume = PV_ScaleVolumeFromChannelAndSong(SOUND_EFFECT_CHANNEL, theNote->NoteMIDIVolume);
					newVolume = (newVolume * MusicGlobals->scaleBackAmount) >> 8;
					theNote->NoteVolume = newVolume;
				}
			}
		}
	}
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
INT16 GM_GetEffectsVolume(void)
{
	INT16	volume;

	volume = MAX_MASTER_VOLUME;
	if (MusicGlobals)
	{
		volume = MusicGlobals->effectsVolume;
	}
	return volume;
}
#endif


OPErr GM_SetMasterSongTempo(GM_Song *pSong, INT32 newTempo)
{
	if (pSong)
	{
		pSong->MasterTempo = newTempo;
	}
	return NO_ERR;
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
INT32 GM_SongTicks(GM_Song *pSong)
{
	if (pSong)
	{
		return pSong->CurrentMidiClock;
	}
	return 0L;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
INT32 GM_SongMicroseconds(GM_Song *pSong)
{
	if (pSong)
	{
		return pSong->songMicroseconds;
	}
	return 0L;
}
#endif

// Set channel reverb amount
static void PV_SetChannelReverb(INT16 the_channel, INT16 reverbAmount)
{
	register MusicVars		*myMusicGlobals;
	register LOOPCOUNT		count;
	register NoteRecord *	theNote;

	myMusicGlobals = MusicGlobals;
	// update the current notes playing to the new reverb
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		theNote = &myMusicGlobals->NoteEntry[count];
		if (theNote->NoteDur >= 0)
		{
			if (theNote->NoteChannel == the_channel)
			{
				// Since we don't have a true analog reverb, we'll just enable after about 10% or so...
				if (reverbAmount > REVERB_CONTROLER_THRESHOLD)
				{
					theNote->avoidReverb = FALSE;		// enable full on
				}
				else
				{
					theNote->avoidReverb = TRUE;
				}
			}
		}
	}
}

// Process midi program change
static void PV_ProcessProgramChange(GM_Song *pSong, INT16 MIDIChannel, INT16 currentTrack, INT16 program)
{
	if ( (currentTrack == -1) || (pSong->trackMuted[currentTrack] == FALSE) )	// track muted?
	{
		if (pSong->allowProgramChanges)
		{
			if (MIDIChannel == PERCUSSION_CHANNEL)
			{
				// only change the percussion program if we're not in perc mode
				if (pSong->defaultPercusionProgram > 0)
				{
					program = pSong->defaultPercusionProgram;
				}
				else
				{
					if (pSong->defaultPercusionProgram == -1)
					{
						program = 0;
					}
				}
			}
			pSong->channelProgram[MIDIChannel] = program;
		}
	
		if (pSong->AnalyzeMode)
		{
			// if analyzing, note the program or the channel
			if (pSong->allowProgramChanges == FALSE)
			{
				program = MIDIChannel;
			}
	
			if (pSong->firstChannelProgram[MIDIChannel] == -1)
			{	// first time only
				pSong->firstChannelProgram[MIDIChannel] = program;
				pSong->firstChannelBank[MIDIChannel] = pSong->channelBank[MIDIChannel];
			}
	
			if (MIDIChannel == PERCUSSION_CHANNEL)
			{
				// only change the percussion program if we're not in perc mode
				if (pSong->defaultPercusionProgram > 0)
				{
					program = pSong->defaultPercusionProgram;
				}
				else
				{
					if (pSong->defaultPercusionProgram == -1)
					{
						program = 0;
					}
				}
			}
			pSong->channelProgram[MIDIChannel] = program;
		}
	}
}

// Process note off
static void PV_ProcessNoteOff(GM_Song *pSong, INT16 MIDIChannel, INT16 currentTrack, INT16 note, INT16 volume)
{
	register INT16		thePatch;

	volume = volume;	// not used

	if ( (currentTrack == -1) || (pSong->trackMuted[currentTrack] == FALSE) )	// track muted?
	{
		if (pSong->AnalyzeMode == SCAN_NORMAL)
		{
			note += pSong->songPitchShift;
			if (pSong->defaultPercusionProgram == -1)		// in GM mode?
			{
				if (MIDIChannel == PERCUSSION_CHANNEL)
				{
					thePatch = GM_ConvertPatchBank(note, pSong->channelBank[MIDIChannel], MIDIChannel);
				}
				else
				{
					thePatch = GM_ConvertPatchBank(pSong->channelProgram[MIDIChannel],
											pSong->channelBank[MIDIChannel], MIDIChannel);
				}
			}
			else
			{
				thePatch = pSong->channelProgram[MIDIChannel];
			}
			StopMIDINote(thePatch, MIDIChannel, note);
		}
		else
		{
			if (pSong->firstChannelProgram[MIDIChannel] != -1)
			{
				if (pSong->defaultPercusionProgram == -1)		// in GM mode?
				{
					if (MIDIChannel == PERCUSSION_CHANNEL)
					{
						thePatch = GM_ConvertPatchBank(note, pSong->channelBank[MIDIChannel], MIDIChannel);
						note = -1;
					}
					else
					{
						thePatch = GM_ConvertPatchBank(pSong->channelProgram[MIDIChannel],
												pSong->channelBank[MIDIChannel], MIDIChannel);
					}
				}
				else
				{
					thePatch = pSong->channelProgram[MIDIChannel];
				}
				GM_SetUsedInstrument(thePatch, note, TRUE);		// mark note in instrument
			}
		}
	}
}

// Process note on
static void PV_ProcessNoteOn(GM_Song *pSong, INT16 MIDIChannel, INT16 currentTrack, INT16 note, INT16 volume)
{
	register INT16		thePatch;

	if ( (currentTrack == -1) || (pSong->trackMuted[currentTrack] == FALSE) )	// track muted?
	{
		if (volume)
		{	
			if (pSong->AnalyzeMode == SCAN_NORMAL)
			{
				note += pSong->songPitchShift;
				if (pSong->defaultPercusionProgram == -1)
				{
					if (MIDIChannel == PERCUSSION_CHANNEL)
					{
						thePatch = GM_ConvertPatchBank(note, pSong->channelBank[MIDIChannel], MIDIChannel);
					}
					else
					{
						thePatch = GM_ConvertPatchBank(pSong->channelProgram[MIDIChannel],
												pSong->channelBank[MIDIChannel], MIDIChannel);
					}
				}
				else
				{
					thePatch = pSong->channelProgram[MIDIChannel];
				}
				ServeMIDINote(thePatch, MIDIChannel, note, volume);
			}
			else
			{
				if (pSong->allowProgramChanges == FALSE)
				{
					// if analyzing, note the channel. This is required in case program changes have been turned off
					// and there are no program changes before the first note.
					if (pSong->firstChannelProgram[MIDIChannel] == -1)
					{	// first time only
						pSong->firstChannelProgram[MIDIChannel] = MIDIChannel;
					}
					GM_SetUsedInstrument(MIDIChannel, note, TRUE);		// mark instrument
				}
				else
				{
					if (pSong->firstChannelProgram[MIDIChannel] != -1)
					{
						if (pSong->defaultPercusionProgram == -1)
						{
							if (MIDIChannel == PERCUSSION_CHANNEL)
							{
								thePatch = GM_ConvertPatchBank(note, pSong->channelBank[MIDIChannel], MIDIChannel);
								note = -1;
							}
							else
							{
								thePatch = GM_ConvertPatchBank(pSong->channelProgram[MIDIChannel],
														pSong->channelBank[MIDIChannel], MIDIChannel);
							}
						}
						else
						{
							thePatch = pSong->channelProgram[MIDIChannel];
						}
						GM_SetUsedInstrument(thePatch, note, TRUE);		// mark note in instrument
					}
				}
			}
		}
		else
		{
			PV_ProcessNoteOff(pSong, MIDIChannel, currentTrack, note, volume);
		}
	}
}

// Process pitch bend
static void PV_ProcessPitchBend(GM_Song *pSong, INT16 MIDIChannel, INT16 currentTrack, BYTE valueMSB, BYTE valueLSB)
{
	if ( (currentTrack == -1) || (pSong->trackMuted[currentTrack] == FALSE) )	// track muted?
	{
		if ( (pSong->AnalyzeMode == SCAN_NORMAL) || (pSong->AnalyzeMode == SCAN_DETERMINE_LENGTH) )
		{
// This solves (!) a bug with pitch bend. I don't know what it is right now. You can't pitch percussion at all
// with the GM percussion set
			if (pSong->defaultPercusionProgram == -1)
			{
				if (MIDIChannel != PERCUSSION_CHANNEL)
				{
					// change the current channel bends for new notes
					pSong->channelBend[MIDIChannel] = SetChannelPitchBend(MIDIChannel, pSong->channelPitchBendRange[MIDIChannel], valueMSB, valueLSB);
				}
			}
			else
			{
				// change the current channel bends for new notes
				pSong->channelBend[MIDIChannel] = SetChannelPitchBend(MIDIChannel, pSong->channelPitchBendRange[MIDIChannel], valueMSB, valueLSB);
			}
		}
	}
}

// Process midi controlers
static void PV_ProcessControler(GM_Song *pSong, INT16 MIDIChannel, INT16 currentTrack, INT16 controler, UINT16 value)
{
	BYTE		valueLSB, valueMSB;

	if ( (currentTrack == -1) || (pSong->trackMuted[currentTrack] == FALSE) )	// track muted?
	{
		switch (controler)
		{
			case 0:		// bank select LSB. We ignore the MSB of bank select
				pSong->channelBank[MIDIChannel] = value;
				break;			
			case 100:		// registered parameter numbers LSB
				pSong->channelRegisteredParameterLSB[MIDIChannel] = value;
				break;
			case 101:		// registered parameter numbers MSB
				pSong->channelRegisteredParameterMSB[MIDIChannel] = value;
				break;
			case 6:		// data entry for RPN controlers
				valueLSB = pSong->channelRegisteredParameterLSB[MIDIChannel];
				valueMSB = pSong->channelRegisteredParameterMSB[MIDIChannel];
				switch ((valueMSB * 128) + valueLSB)
				{
					case 0:		// set pitch bend range in half steps
						pSong->channelPitchBendRange[MIDIChannel] = value;
						break;
					case 1:		// Fine Tuning
					case 2:		// Coarse Tuning
						break;
				}
				// reset
				pSong->channelRegisteredParameterLSB[MIDIChannel] = UCHAR_MAX;
				pSong->channelRegisteredParameterMSB[MIDIChannel] = UCHAR_MAX;
				break;
	
			case 1:		// Modulation LSB
				pSong->channelModWheel[MIDIChannel] = value;
				if (pSong->AnalyzeMode == SCAN_NORMAL)
				{
					SetChannelModWheel(MIDIChannel, value);
				}
				break;
			case 33:		// Modulation MSB
				break;
			case 7:		// Volume change LSB
				// make sure and set the channel volume not scaled, because its scaled later
				pSong->channelVolume[MIDIChannel] = value;
				if (pSong->AnalyzeMode == SCAN_NORMAL)
				{
					SetChannelVolume(MIDIChannel, value);
				}
				break;
			case 39:		// Volume change MSB
				break;
			case 8:		// balance
			case 40:
				break;
			case 10:		// stereo pan LSB
				pSong->channelStereoPosition[MIDIChannel] = SetChannelStereoPosition(MIDIChannel, value);
				break;
			case 42:		// stereo pan MSB
				break;
			case 11:		// expression LSB
				pSong->channelExpression[MIDIChannel] = value;
				// for now, let's just scale up the volume level of the channel
				valueLSB = pSong->channelVolume[MIDIChannel];
				// Say 127 is 10% higher
				// make sure and set the channel volume not scaled, because its scaled later
				value += (valueLSB / 10);
				pSong->channelVolume[MIDIChannel] = value;
				if (pSong->AnalyzeMode == SCAN_NORMAL)
				{
					SetChannelVolume(MIDIChannel, value);
				}
				break;
			case 43:		// expression MSB
				break;
			case 64:		// sustain
				pSong->channelSustain[MIDIChannel] = (value) ? TRUE : FALSE;
				// stop sustaining note, set current notes to sustain
				if (pSong->AnalyzeMode == SCAN_NORMAL)
				{
					PV_ChangeSustainedNotes(MIDIChannel, value);
				}
				break;
			case 67:		// soft
				break;
			case 91:		// amount of reverb
				// set the channel reverb
				pSong->channelReverb[MIDIChannel] = value;
				if (pSong->AnalyzeMode == SCAN_NORMAL)
				{
					PV_SetChannelReverb(MIDIChannel, value);
				}
				break;
			case 121:		// Reset
				// Don't support the in sequence feature to reset channel controlers.
				// It seems that most sequences set the channels, then reset them! Ouch!
				//PV_ResetControlers(pSong, MIDIChannel);
				break;
			case 123:		// all notes off
				if (pSong->AnalyzeMode == SCAN_NORMAL)
				{
					GM_EndAllNotes();
				}
				break;
		}
	}
	
	
	if (pSong->AnalyzeMode == SCAN_NORMAL)
	{
		// process special mute track controlers
		switch (controler)
		{
			case 85:		// looping off (value = 0) otherwise its a max loop count
				GM_SetSongLoopFlag(pSong, (value) ? TRUE : FALSE);
				pSong->songMaxLoopCount = value;
				break;
			case 86:		// mute on loop count x
				if (currentTrack != -1)
				{
					if (pSong->songLoopCount == value)
					{
						pSong->trackMuted[currentTrack] = TRUE;
					}
				}
				break;
			case 87:		// unmute on loop count x
				if (currentTrack != -1)
				{
					if (pSong->songLoopCount == value)
					{
						pSong->trackMuted[currentTrack] = FALSE;
					}
				}
				break;
		}
	}
}



/*
Igor Meta events
FF 7F len data - this is the standard sequencer specific data block. We can use it this way. Sequencers 
			ignore this data, but will not let you edit it in any way. I've tried other meta id's and 
			the results are the same.  Most don’t even display that its present. But it does allow 
			us to work with blocks of 8 bit data. The only way we can set it up for a sequencer 
			to copy and paste our block is to make it a sysex. Which means 7 bit bytes! Ug!

FF 7F len (this is a variable len)
Byte		Data Type
0-2		<Igor ID> This will be our MMA approved ID
3-6		4 byte command count
7-10		4 byte command ID for type of event. Message type:

FLUS - command
{
11		flush current instruments (default, you don’t need to send this)
}

CACH - command
{
11		cache instruments. Issuing this will cache instruments and they will not be flushed.
}

DATA - resource
{		
11-14	Number of resources included in this meta event. You can have one event per data 
		block, or include all the resources in 	one event.


14-		The data following will be in a block format. First there will  be a 4 byte data type ID, 
		followed by a 4 byte ID, a pascal string (first byte is length, then data), and then a 4 byte 
		resource length, then the resource data. The length is only the length of the resource data.

Types:
snd( )	Standard Macintosh snd resource type. When building ALWAYS put snd's first
ID		ID reference number that is referenced in the INST resources
NAME	pascal string containing resource name
LENGTH
DATA

csnd		Standard Macintosh snd resource type that is compressed When building ALWAYS put snd's first
ID		ID reference number that is referenced in the INST resources
NAME	pascal string containing resource name
LENGTH
DATA

SONG	Igor Standard song performance resource
ID		Doesn’t matter. Should only be one song per midi file
NAME	pascal string containing resource name
LENGTH
DATA

INST		Igor Standard external instrument resource
ID		Patch number to replace
NAME	pascal string containing resource name
LENGTH
DATA
}
*/

static void PV_ProcessIgorResource(GM_Song *pSong, long command, unsigned char *pMidiStream, long theID, long length)
{
	GM_Instrument	*theI;
	XPTR		theNewData;

	switch (command)
	{
		case QuadChar('c','s','n','d'):
			theNewData = DecompressSampleFormatPtr((XPTR)pMidiStream, length);
			PV_SetSampleIntoCache(theID, theNewData);
			break;
		case QuadChar('s','n','d',' '):
			PV_SetSampleIntoCache(theID, (XPTR)pMidiStream);
			break;
		case QuadChar('I','N','S','T'):
			// this will load the instrument into the cache. When the sequence is later scanned it will flag this
			// instrument for load, but since we've already loaded it into the cache it will not be loaded again.
			// We don't need to set the reference count, because that will happen when we try to load the instrument
			// again. It will get set to one hit. This works because we have the instrument already setup in the master
			// instrument array.
			if ( (theID >= 0) && (theID < (MAX_INSTRUMENTS*MAX_BANKS)) )
			{
				GM_SetUsedInstrument(theID, -1, TRUE);	// load this instrument for key splits and such
				theI = PV_GetInstrument(theID, (void *)pMidiStream, length);
				if (theI)
				{
//					theI->usageReferenceCount = 0;		// no change here, it will happen later
					MusicGlobals->InstrumentData[theID] = theI;
					MusicGlobals->remapArray[theID] = theID;
				}
				GM_SetUsedInstrument(theID, -1, FALSE);	// don't load this instrument, already loaded	
			}
			break;
		case QuadChar('S','O','N','G'):
			// merge in changes from external song
			GM_MergeExternalSong((void *)pMidiStream, theID, pSong);
			break;
	}
}


// Validate command types. This is used to protect us from bad memory pointers, etc
static BOOL_FLAG PV_ValidateType(long command)
{
	BOOL_FLAG	valid;

	valid = FALSE;
	switch (command)
	{
		case QuadChar('c','s','n','d'):
		case QuadChar('s','n','d',' '):
		case QuadChar('I','N','S','T'):
		case QuadChar('S','O','N','G'):
			valid = TRUE;
			break;
	}
	return valid;
}


// Process Igor Meta events given a midi stream right after the sysex ID
static void PV_ProcessIgorMeta(GM_Song *pSong, unsigned char *pMidiStream)
{
	long	command, unitCount, cmdCount, count, length, theID;
	char	resourceName[256];

	// only process instrument loading during the first midi scan. We can't allocate memory otherwise
	if (pSong->AnalyzeMode == SCAN_SAVE_PATCHES)
	{
		cmdCount = XGetLong(pMidiStream);
		if (cmdCount <  (MAX_SAMPLES * 3))
		{
			pMidiStream += 4;
			for (count = 0; count < cmdCount; count++)
			{
				command = XGetLong(pMidiStream);
				pMidiStream += 4;
				switch (command)
				{
					case QuadChar('F','L','U','S'):	// immediate command
						GM_FlushInstrumentCache(FALSE);
						break;
					
					case QuadChar('C','A','C','H'):	// immediate command
						GM_FlushInstrumentCache(TRUE);
						break;
					
					case QuadChar('D','A','T','A'):	// block resources
						unitCount = XGetLong(pMidiStream);
						if (unitCount < (MAX_SAMPLES * 3))
						{
							pMidiStream += 4;
							for (count = 0; count < unitCount; count++)
							{
								command = XGetLong(pMidiStream);		// type
								if (PV_ValidateType(command))
								{
								pMidiStream += 4;
								theID = XGetLong(pMidiStream);		// ID
								pMidiStream += 4;
								length = *pMidiStream;				// NAME
								XBlockMove(pMidiStream, resourceName, length + 1);	// copy pascal string name
								pMidiStream += length + 1;
								length = XGetLong(pMidiStream);		// LENGTH of resource
								pMidiStream += 4;
								PV_ProcessIgorResource(pSong, command, pMidiStream, theID, length);
								pMidiStream += length;
							}
								else
								{
									break;
								}
							}
						}
						
						break;
				}
			}
		}
	}
}

// Given a pointer to a midi stream, this will read the variable length value from and update the
// midi stream pointer
static long PV_ReadVariableLengthMidi(UBYTE **ppMidiStream)
{
	register	INT32 value;
	register	UBYTE *midi_stream;

	midi_stream = *ppMidiStream;
	value = 0;					// read length and…
	while (*midi_stream & 0x80)
	{
		value = value << 7;  
		value |= *midi_stream++ - 0x80; 
	}
	value = value << 7;  
	value |= *midi_stream++;
	*ppMidiStream = midi_stream;
	return value;
}


// Walk through the midi stream and process midi events for one slice of time.
void PV_MusicIRQ(GM_Song *pSong)
{
	register MusicVars	*myMusicGlobals;
	register LOOPCOUNT	currentTrack;
	register UBYTE		midi_byte, volume, controler;
	register INT32		value;
	register UBYTE 	*midi_stream;
	register INT16		MIDIChannel;
	register BYTE		valueLSB, valueMSB;
	UBYTE 			*temp_midi_stream;

	myMusicGlobals = MusicGlobals;

	// Song not loaded
	if (pSong == NULL)
	{
		return;
	}
	pSong->SomeTrackIsAlive = FALSE;

	pSong->CurrentMidiClock += pSong->MIDIDivision;
	pSong->songMicroseconds += pSong->songMicrosecondIncrement;

	for (currentTrack = 0; currentTrack < MAX_TRACKS; currentTrack++)
	{
		midi_stream = pSong->ptrack[currentTrack];
		if (midi_stream == NULL)
		{
			goto ServeNextTrack;
		}
		if (pSong->trackon[currentTrack])
		{
			pSong->SomeTrackIsAlive = TRUE;
			if (pSong->trackon[currentTrack] == 'F')		// first time
			{
				pSong->trackon[currentTrack] = 'R';		// running
				goto UpdateDeltaTime;
			}
			pSong->trackticks[currentTrack] -= pSong->MIDIDivision;
Do_GetEvent:
			if (pSong->trackticks[currentTrack] < 0)
			{
				if ((midi_stream - pSong->trackstart[currentTrack]) > pSong->tracklen[currentTrack])
				{
					/* the track has ended unexpectedly.
					*/
					pSong->trackon[currentTrack] = 0;
					goto ServeNextTrack;
				}
				goto GetMIDIevent;
			}
			if (pSong->trackticks[currentTrack] >= 0)
			{
				goto ServeNextTrack;
			}
UpdateDeltaTime:
			temp_midi_stream = midi_stream;
			value = PV_ReadVariableLengthMidi(&temp_midi_stream);
			midi_stream = temp_midi_stream;
			pSong->trackticks[currentTrack] += (value << 6);
			goto Do_GetEvent;
		}
		goto ServeNextTrack;
GetMIDIevent:
		midi_byte = *midi_stream++;
		if (midi_byte <= 0x7F)
		{
			--midi_stream;
			midi_byte = pSong->runningStatus[currentTrack];
		}
		if (midi_byte == 0xFF)
		{
			/* Meta Event
			*/
			midi_byte = *midi_stream++;
			switch (midi_byte)
			{
				/* Set MIDI Tempo:
				*/
				case 0x51:
					value = midi_stream[1] << 8;
					value |= midi_stream[2];
					value = value << 8;
					value |= midi_stream[3];
					pSong->UnscaledMIDITempo = value;
					pSong->MIDITempo = (pSong->UnscaledMIDITempo / pSong->MicroJif) & 0xFFFFL;
					PV_ScaleDivision(pSong, pSong->UnscaledMIDIDivision);
					goto SkipMeta;

				/* Set Time Signature:
				*/
				case 0x58:
					break;

				case 0x7F:
					temp_midi_stream = midi_stream;
					value = PV_ReadVariableLengthMidi(&temp_midi_stream);	// get length
					midi_stream = temp_midi_stream;
					if (midi_stream[0] == 0x00)		// IGOR sysex ID
					{
						if (midi_stream[1] == 0x01)
						{
							if (midi_stream[2] == 0x0D)
							{
								PV_ProcessIgorMeta(pSong, midi_stream + 3);
							}
						}
					}
					midi_stream += value;			// skip meta
					goto UpdateDeltaTime;

				/* End-of-track:
				*/
				case 0x2F:
					pSong->trackon[currentTrack] = 0;
					goto ServeNextTrack;
			}
			goto SkipMeta;
		}
		else
		{
			pSong->runningStatus[currentTrack] = midi_byte;
			if (midi_byte == 0)		/* 0 = null event. */
			{
				goto GetMIDIevent;
			}
			MIDIChannel = midi_byte & 0xF;
			switch (midi_byte & 0xF0)			// process commands
			{
				case 0x90:					// •• Note On
					value = *midi_stream++;		// MIDI note
					volume = *midi_stream++;	// note on velocity
					PV_ProcessNoteOn(pSong, MIDIChannel, currentTrack, value, volume);
					break;
				case 0x80:					// •• Note Off
					value = *midi_stream++;		// MIDI note
					volume = *midi_stream++;			// note off velocity, ignore for now
					PV_ProcessNoteOff(pSong, MIDIChannel, currentTrack, value, volume);
					break;
				case 0xB0:					// •• Control Change
					controler = *midi_stream++; /* control # */
					midi_byte = *midi_stream++; /* new value */
					PV_ProcessControler(pSong, MIDIChannel, currentTrack, controler, midi_byte);
					
#ifdef SUPPORT_CONTROLLER_CALLBACKS
					if (pSong->AnalyzeMode == SCAN_NORMAL)
					{
						if (myMusicGlobals->channelCallbackProc[controler])
						{
							if (myMusicGlobals->channelCallbackProc[controler])
							{	// call user function
								(*myMusicGlobals->channelCallbackProc[controler])(MIDIChannel, controler, midi_byte);
							}
						}
					}
#endif
					break;
				case 0xC0:					// •• ProgramChange
					value = *midi_stream++;
					PV_ProcessProgramChange(pSong, MIDIChannel, currentTrack, value);
					break;
				case 0xE0:					// •• SetPitchBend
					valueLSB = *midi_stream++;
					valueMSB = *midi_stream++;
					PV_ProcessPitchBend(pSong, MIDIChannel, currentTrack, valueMSB, valueLSB);
					break;
				case 0xA0:					// ••  Key Pressure
					midi_stream += 2;			// note, key pressure
					break;
				case 0xD0:					// •• ChannelPressure
					midi_stream++;
					break;
				case 0xF7:					// •• System Exclusive
				case 0xF0:					// •• System Exclusive
					temp_midi_stream = midi_stream;
					value = PV_ReadVariableLengthMidi(&temp_midi_stream);
					midi_stream = temp_midi_stream;
					midi_stream += value;	// skip sysex
					break;
			} /* end switch */
			/* Not Meta Event. */
			goto UpdateDeltaTime;
		}
		;
		goto ServeNextTrack;
		;
SkipMeta:
		temp_midi_stream = midi_stream;
		value = PV_ReadVariableLengthMidi(&temp_midi_stream);
		midi_stream = temp_midi_stream;
		midi_stream += value;	// skip sysex
		goto UpdateDeltaTime;
ServeNextTrack:
		pSong->ptrack[currentTrack] = midi_stream;
	}

	// song finished
	if (pSong->AnalyzeMode == SCAN_NORMAL)
	{
		if (GM_IsSongDone(pSong))
		{
			if (pSong->songFinished == FALSE)
			{
				if (pSong->loopSong)
				{
					// reconfigure song and loop
//					PV_CallSongCallback(pSong, FALSE);
					PV_ConfigureMusic(pSong);
					pSong->songLoopCount++;
					if (pSong->songLoopCount > pSong->songMaxLoopCount)
					{
						pSong->songLoopCount = 0;
					}
				}
				else
				{
					// call callback and finish
					PV_CallSongCallback(pSong, TRUE);
					pSong->songFinished = TRUE;
				}
			}
		}
	}
}

// process all songs and external events
void PV_ProcessSequencerEvents(void)
{
	GM_Song		*pSong;
	short int		count;

	for (count = 0; count < MAX_SONGS; count++)
	{
		pSong = MusicGlobals->pSongsToPlay[count];
		if (pSong)
		{
			if (pSong->AnalyzeMode == SCAN_NORMAL)
			{
				MusicGlobals->theSongPlaying = pSong;
				PV_MusicIRQ(pSong);					// process current song playing
			}
		}
	}
	MusicGlobals->theSongPlaying = NULL;
}

// EOF of GenSeq.c
