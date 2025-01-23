/*
************************************************************************
**
** ÒGenSong.cÓ
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**	Confidential-- Internal use only
**
**	© Copyright 1983-1996 by Steve Hales, All Rights Reserved.
**	Written by Steve Hales and Jim Nitchals
**	No portion of this program (whether in source, object, or other form)
**	may be copied, duplicated, distributed, altered, or otherwise utilized without
**	direct consent of the copyright holder.
**
** Overview
**	This contains code to load an maintain songs and midi data. This code library is specific
**	to the platform, but the API is independent in nature.
**
**	NOTE: relies on structures from SoundMusicSys API
**
**
** Modification History:
**
**	1/24/96		Created
**	1/25/96		Moved GM_GetUsedPatchlist from GenSeq.c
**	1/28/96		Moved GM_FreeSong from GenSeq.c
**	2/3/96		Removed extra includes
**	2/5/96		Removed unused variables. Working towards multiple songs
**				Added GM_GetSongTickLength
**	2/12/96		Added GM_SetSongTickPosition
**	3/1/96		Fixed bug with GM_SetSongTickPosition that would blow the songLoop flag away
**	3/5/96		Eliminated the global songVolume 
**	4/15/96		Added support to interpret SONG resource via GM_MergeExternalSong
**	5/12/96		Removed CPU edian/alignment issues by use XGetShort & XGetLong
**
**
************************************************************************
*/

#include "GenSnd.h"
#include "GenPriv.h"
#include "X_API.h"

#include "PrivateSoundMusicSystem.h"
#include "Headers.h"

// Functions


static XPTR PV_GetMidiData(long theID, long *pSize)
{
	XPTR	theData, pData;
	long		midiSize;

	theData = XGetAndDetachResource(ID_MIDI, theID, &midiSize);
	if (theData == NULL)
	{
		theData = XGetAndDetachResource(ID_MIDI_OLD, theID, &midiSize);
	}
	if (theData == NULL)
	{
		theData = XGetAndDetachResource(ID_CMID, theID, &midiSize);
		if (theData)
		{
//		pData = DecompressPtr(theData, midiSize);
			pData = NULL;
			if (pData)
			{
				XDisposePtr(theData);
				theData = pData;
			}
		}
	}
	if (theData)
	{
		*pSize = midiSize;
	}
	return theData;
}

static GM_Song * PV_CreateSongFromMidi(short theID, XPTR useThisMidiData, long midiSize)
{
	XPTR		theMidiData;
	GM_Song *	theSong = nil;
	long			count;

	if (useThisMidiData)
	{
		theMidiData = useThisMidiData;
	}
	else
	{
		midiSize = 0;
		theMidiData = PV_GetMidiData(theID, &midiSize);
	}
	if (theMidiData)
	{
		theSong = (GM_Song *)XNewPtr((long)sizeof(GM_Song));
		if (theSong)
		{
			theSong->midiData = theMidiData;
			theSong->disposeMidiDataWhenDone = (useThisMidiData == NULL) ? TRUE : FALSE;
			// Fill in remap first
			for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
			{
				theSong->instrumentRemap[count] = -1;		// no remap
			}
		}
	}
	return theSong;
}

static void PV_SetTempo(GM_Song *pSong, long masterTempo)
{
	if (pSong)
	{
		if (masterTempo == 0L)
		{
			masterTempo = 16667;
		}
		masterTempo = (100L * masterTempo) / 16667;
		if (masterTempo < 25) masterTempo = 25;
		if (masterTempo > 300) masterTempo = 300;
		GM_SetMasterSongTempo(pSong, (masterTempo << 16L) / 100L);
	}
}

void GM_MergeExternalSong(void *theExternalSong, long theSongID, GM_Song *theSong)
{
	short int		maps;
	short int		count;
	short int		number;
	SongResource *theSX;

	theSX = (SongResource *)theExternalSong;

	if (theSX && theSong)
	{
		theSong->songID = theSongID;
		theSong->songPitchShift = theSX->songPitchShift;
		theSong->allowProgramChanges = (theSX->flags1 & XBF_enableMIDIProgram) ? TRUE : FALSE;
		theSong->terminateDecay = (theSX->flags1 & XBF_terminateDecay) ? TRUE : FALSE;
		theSong->defaultPercusionProgram = theSX->defaultPercusionProgram;
		theSong->defaultReverbType = theSX->reverbType;
		theSong->maxSongVoices = theSX->maxNotes;
		theSong->maxNormalizedVoices = XGetShort(&theSX->maxNormNotes);
		theSong->maxEffectVoices = theSX->maxEffects;
		maps = XGetShort(&theSX->remapCount);
		PV_SetTempo(theSong, XGetShort(&theSX->songTempo));
		theSong->songVolume = MAX_NOTE_VOLUME;

// Load instruments
		if ((theSX->flags1 & XBF_enableMIDIProgram) == FALSE)
		{
			number = (theSX->flags1 & XBF_fileTrackFlag) ? MAX_TRACKS : MAX_CHANNELS;
			for (count = 0; count < number; count++)
			{
				theSong->instrumentRemap[count] = count;
			}
		}

		// Fill in remap first
		if (maps)
		{
			for (count = 0; count < maps; count++)
			{
				number = XGetShort(&theSX->remaps[count].instrumentNumber) & ((MAX_INSTRUMENTS*MAX_BANKS)-1);
				theSong->instrumentRemap[number] = XGetShort(&theSX->remaps[count].ResourceINSTID);
			}
		}
	}
}


static GM_Song * PV_CreateSongFromExternalSong(short int songID, void *theExternalSong, void *theExternalMidiData, long midiSize)
{
	GM_Song		*theSong;
	SongResource *theSX;

	theSX = (SongResource *)theExternalSong;

	theSong = NULL;
	if (theSX)
	{
		theSong = PV_CreateSongFromMidi(XGetShort(&theSX->midiResourceID), theExternalMidiData, midiSize);
		if (theSong)
		{
			GM_MergeExternalSong(theSX, songID, theSong);
		}
	}
	return theSong;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
static void PV_ClearSongInstruments(GM_Song *pSong)
{
	long	count;

	if (pSong)
	{
		for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
		{
			pSong->instrumentData[count] = NULL;
		}
	}
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
GM_Song * GM_CreateLiveSong(short int songID)
{
	GM_Song		*pSong;
	short int		count;

	pSong = NULL;

	pSong = (GM_Song *)XNewPtr((long)sizeof(GM_Song));
	if (pSong)
	{
		// Fill in remap first
		for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
		{
			pSong->instrumentRemap[count] = -1;		// no remap
		}

		for (count = 0; count < MAX_CHANNELS; count++)
		{
			pSong->firstChannelBank[count] = 0;
			pSong->firstChannelProgram[count] = -1;
		}
		PV_ConfigureInstruments(pSong);

		pSong->defaultReverbType = GM_GetReverbType();
		pSong->songID = songID;
		pSong->songPitchShift = 0;
		pSong->allowProgramChanges = TRUE;
		pSong->terminateDecay = FALSE;
		pSong->defaultPercusionProgram = -1;

		pSong->maxSongVoices = 8;
		pSong->maxNormalizedVoices = 4;
		pSong->maxEffectVoices = 0;

		PV_SetTempo(pSong, 0L);
		pSong->songVolume = MAX_NOTE_VOLUME;
	}
	return pSong;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
OPErr GM_StartLiveSong(GM_Song *pSong, BOOL_FLAG loadPatches)
{
	OPErr		theErr;
	short int		songSlot, count;

	theErr = NO_ERR;
	if (pSong)
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
			if (loadPatches)
			{
				for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
				{
					GM_LoadInstrument(count);
					MusicGlobals->remapArray[count] = count;
					pSong->instrumentData[count] = MusicGlobals->InstrumentData[count];
					pSong->instrumentRemap[count] = -1;
				}
			}

			pSong->SomeTrackIsAlive = FALSE;
			pSong->songFinished = FALSE;
			pSong->AnalyzeMode = SCAN_NORMAL;
/*
			theErr = GM_ChangeSystemVoices(pSong->maxSongVoices,
										pSong->maxNormalizedVoices,
										pSong->maxEffectVoices);
*/
			// Set reverb type now.
			GM_SetReverbType(pSong->defaultReverbType);

			// first time looping, and set mute tracks to off
			pSong->songLoopCount = 0;
			for (count = 0; count < MAX_TRACKS; count++)
			{
				pSong->trackMuted[count] = FALSE;
			}

			// Start song playing now.
			MusicGlobals->pSongsToPlay[songSlot] = pSong;
		}
	}
	return theErr;
}
#endif


// Load the SongID from an external SONG resource and or a extneral midi resource.
//
//	songID			will be the ID used during playback
//	theExternalSong	standard SONG resource structure
//	theExternalMidiData	if not NULL, then will use this midi data rather than what is found in external SONG resource
//	midiSize			size of midi data if theExternalMidiData is not NULL
//	pInstrumentArray	array, if not NULL will be filled with the instruments that need to be loaded.
//	loadInstruments	if not zero, then instruments and samples will be loaded
//	pErr				pointer to an OPErr

GM_Song * GM_LoadSong(short int songID, void *theExternalSong, void *theExternalMidiData, long midiSize, 
					short int *pInstrumentArray, short int loadInstruments, OPErr *pErr)
{
	GM_Song		*theSong;

	*pErr = NO_ERR;
	theSong = PV_CreateSongFromExternalSong(songID, theExternalSong, theExternalMidiData, midiSize);
#ifdef VERBOSE
	Message(("Created SongFromExternalSong"));
#endif

#if 1
	if (theSong)
	{
		if (GM_LoadSongInstruments(theSong, pInstrumentArray, loadInstruments))
		{
			Message(("Failed to load song instruments"));
			GM_FreeSong(theSong);
			theSong = NULL;
			*pErr = BAD_INSTRUMENT;
		}
		else
		{
			// song length not calculated
#ifdef VERBOSE
			Message(("Loaded song insts OK"));
#endif
			theSong->songMidiTickLength = ULONG_MAX;
			theSong->songMicrosecondLength = ULONG_MAX;
			*pErr = NO_ERR;
		}
	}
	else
	{
		Message(("Couldnt create song from external song"));
		*pErr = MEMORY_ERR;
	}
#endif
	return theSong;
}

void GM_FreeSong(GM_Song *theSong)
{
#ifdef VERBOSE
	Message(("Freeing Song"));
#endif
	GM_EndSong(theSong);
	if (theSong)
	{
		GM_UnloadSongInstruments(theSong);
		if (theSong->disposeMidiDataWhenDone)
		{
			XDisposePtr((XPTR)theSong->midiData);
		}
		XDisposePtr((XPTR)theSong);
	}
}

// Return the length in MIDI ticks of the song passed

//	pSong	GM_Song structure. Data will be cloned for this function.
//	pErr		OPErr error type
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
INT32 GM_GetSongTickLength(GM_Song *pSong, OPErr *pErr)
{
	GM_Song		*theSong;
	INT32		tickLength;

	*pErr = NO_ERR;
	tickLength = 0;
	if (pSong->songMidiTickLength == -1)
	{
		theSong = (GM_Song *)XNewPtr(sizeof(GM_Song));
		if (theSong)
		{
			*theSong = *pSong;
			PV_ClearSongInstruments(theSong);		// don't free the instruments

			if (PV_ConfigureMusic(theSong) == NO_ERR)
			{
				theSong->AnalyzeMode = SCAN_DETERMINE_LENGTH;
				theSong->SomeTrackIsAlive = TRUE;
	
				theSong->loopSong = FALSE;
				while (theSong->SomeTrackIsAlive)
				{
					PV_MusicIRQ(theSong);
				}
				theSong->AnalyzeMode = SCAN_NORMAL;
				tickLength = pSong->songMidiTickLength = theSong->CurrentMidiClock;
				pSong->songMicrosecondLength = theSong->songMicroseconds;
				theSong->midiData = NULL;
				theSong->songEndCallbackPtr = NULL;
				theSong->disposeMidiDataWhenDone = FALSE;

				GM_FreeSong(theSong);
			}
		}
	}
	else
	{
		tickLength = pSong->songMidiTickLength;
	}
	return tickLength;
}
#endif

// Set the song position
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
OPErr GM_SetSongTickPosition(GM_Song *pSong, INT32 songTickPosition)
{
	GM_Song		*theSong;
	OPErr		theErr;
	BOOL_FLAG	foundPosition;
	long			count;

	theErr = NO_ERR;
	theSong = (GM_Song *)XNewPtr(sizeof(GM_Song));
	if (theSong)
	{
		*theSong = *pSong;
		PV_ClearSongInstruments(theSong);		// don't free the instruments

		if (PV_ConfigureMusic(theSong) == NO_ERR)
		{
			theSong->AnalyzeMode = SCAN_DETERMINE_LENGTH;
			theSong->SomeTrackIsAlive = TRUE;

			theSong->loopSong = FALSE;
			foundPosition = FALSE;
			while (theSong->SomeTrackIsAlive)
			{
				PV_MusicIRQ(theSong);
				if (theSong->CurrentMidiClock > songTickPosition)
				{
					foundPosition = TRUE;
					break;
				}
			}
			theSong->AnalyzeMode = SCAN_NORMAL;
			theSong->loopSong = pSong->loopSong;
			if (foundPosition)
			{
				for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
				{
					theSong->instrumentData[count] = pSong->instrumentData[count];
				}

				MusicGlobals->theSongPlaying = NULL;
				GM_EndAllNotes();
				*pSong = *theSong;		// copy over all song information at the new position
				PV_ClearSongInstruments(theSong);		// don't free the instruments

				MusicGlobals->theSongPlaying = pSong;
			}
			// free duplicate song
			theSong->midiData = NULL;
			theSong->songEndCallbackPtr = NULL;
			theSong->disposeMidiDataWhenDone = FALSE;
		}
		GM_FreeSong(theSong);
	}
	return theErr;
}
#endif

// Return the used patch array of instruments used in the song passed.

//	theExternalSong	standard SONG resource structure
//	theExternalMidiData	if not NULL, then will use this midi data rather than what is found in external SONG resource
//	midiSize			size of midi data if theExternalMidiData is not NULL
//	pInstrumentArray	array, if not NULL will be filled with the instruments that need to be loaded.
//	pErr				pointer to an OPErr
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
INT32 GM_GetUsedPatchlist(void *theExternalSong, void *theExternalMidiData, long midiSize, 
					short int *pInstrumentArray, OPErr *pErr)
{
	GM_Song		*theSong;
	long			count;

	*pErr = NO_ERR;

	theSong = GM_LoadSong(0, theExternalSong, theExternalMidiData, midiSize,
						pInstrumentArray, FALSE, pErr);
	if (theSong)
	{
		GM_FreeSong(theSong);
	}

	count = 0;
	if (*pErr == NO_ERR)
	{
		for (; count < MAX_INSTRUMENTS*MAX_BANKS; count++)
		{
			if (pInstrumentArray[count] == -1)
			{
				break;
			}
		}
	}
	return count;
}
#endif


// EOF of GenSong.c
