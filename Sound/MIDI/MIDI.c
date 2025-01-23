// ===========================================================================
//	MIDI.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#include "SongData.h"
#include "MemoryManager.h"
#include "ObjectStore.h"
#include "System.h"


#ifdef FOR_MAC
#include "MacintoshUndefines.h"
#include <Types.h>
#include <Files.h>
#include <Quickdraw.h>
#include <Folders.h>
#include <Movies.h>
#include <QuickTimeComponents.h>
#include <TextUtils.h>

#include <Resources.h>
#endif

#if X_HARDWARE_PLATFORM == X_WEBTV
#include "HWAudio.h"
#include "HWRegs.h"	
#include "BoxUtils.h"
	#ifdef HARDWARE
	#include "Interrupts.h"
	#endif
#endif

#include "MacintoshRedefines.h"

#include "X_API.h"
#include "GenSnd.h"
#include "GenPriv.h"

#define IGOR_MIX_LEVEL				650				// mix level is 3.50
#define IGOR_MAX_AUDIO_STREAMS		3				// 3 audio streams
#define IGOR_MAX_MIDI_VOICES		28				// 28 midi voices
#define IGOR_MAX_VOICES				(IGOR_MAX_MIDI_VOICES + IGOR_MAX_AUDIO_STREAMS)
#define IGOR_REVERB_TYPE			REVERB_TYPE_4	// Default use of Acoustic Lab Reverb										

#if IGOR_MAX_VOICES > 31
	#error "More than 31 audio voices allocated"
#endif

// ===========================================================================
static short int	audioEnableCount = 0; //	When 0, init audio system, if non zero do not.

static ObjectList*	gPlayList = nil;

// ===========================================================================
MIDI::MIDI()
{
	if (gPlayList == nil)
		gPlayList = new(ObjectList);
	
#ifdef VERBOSE
	Message(("ZEROING Song Reference"));
#endif
	fSongReference = nil;
	fVolume = (ulong)-1;
	fNoFade = false;
}

MIDI::~MIDI()
{
	if (!IsDone())
		Stop();
	else
		EndPlay();
	gPlayList->Remove(this);
}

void MIDI::EndPlay()
{

#ifdef VERBOSE
	ImportantMessage(("freeing song ref %x...", (int)fSongReference));
#endif
	if (fSongReference != nil)
	{
		GM_FreeSong((GM_Song *)fSongReference);
		fSongReference = nil;
		fResource.UnlockData();
	}
}

void MIDI::Idle()
{
	Assert(gPlayList != nil);
	ulong	count = gPlayList->GetCount();
	
	for (int i = 0; i < count; i++)
	{
		MIDI*	current = (MIDI*)gPlayList->At(i);
		
		if (current->IsDone())
		{
			gPlayList->RemoveAt(i);
			current->EndPlay();
			i--; count--;
		}
	}
}

Boolean MIDI::IsDone()
{
	return (Boolean)GM_IsSongDone((GM_Song *)fSongReference);
}

void MIDI::Play(ulong loopCount)
{
	{
		SongResource	*theSong;
		OPErr			theErr;
		short int		theSongID;
		GM_Song			*songPlaying;
		XPTR			pMidi = nil;
		FSNode*			node;
		long			midiSize;
	
		Stop();
		Assert(gPlayList != nil && gPlayList->Find(this) == -1);

#ifdef VERBOSE
		Message(("Starting to play MIDI..."));
#endif
		if (fResource.GetStatus() != kComplete)
			return;

		if (audioEnableCount)
		{
		static const kFilePrefixSize = sizeof("file://") - 1;
#define kRelativeOffset			(-1)	// because current file usage is relative

			// Lock the data in cache until it is done playing.
			midiSize = fResource.GetDataLength();
			if (fResource.IsLocal()) {
				node = Resolve(fResource.GetURL() + kFilePrefixSize + kRelativeOffset, false);
				if (node != nil)
					pMidi = node->data;
				}
			else
				pMidi = fResource.LockData();
				
			if (IsWarning(pMidi == nil))
				;
			if (pMidi != nil)
			{
				// Create a song performance record. This is only used to tell the snyth
				// engine how to play the midi file. Once GM_LoadSong is called, it can be
				// thrown away.
				theSongID = 1000;			// the theSongID is used as a tracking device
											// during song callbacks. Can be anything.
				theSong = NewSongPtr(theSongID,
										IGOR_MAX_MIDI_VOICES,
										IGOR_MIX_LEVEL,
										IGOR_MAX_AUDIO_STREAMS,
										IGOR_REVERB_TYPE);
				if (theSong)
				{
					// load the instruments associated with this midi file.
					//DebugStr("\p Loading song instruments");
				songPlaying = GM_LoadSong(theSongID, 
											(void *)theSong, 
											pMidi,
											midiSize, 
											nil, TRUE,
											&theErr);
	
					// dispose of the performance record.
					DisposeSongPtr(theSong);
	
					if (songPlaying)
						songPlaying->disposeMidiDataWhenDone = TRUE;	// free our midi pointer when the song is freed

					if (songPlaying && theErr == NO_ERR)
					{
#ifdef VERBOSE
						Message(("Playing song and didnt crash"));
#endif
						// Set the loop song flag
						GM_SetSongLoopFlag(songPlaying, (loopCount) ? TRUE : FALSE);
						
						// set volume
						if (fVolume != (ulong)-1)
							GM_SetSongVolume(songPlaying, fVolume);
	
						// Start the song playing
						theErr = GM_BeginSong(songPlaying, nil);
					}
					fSongReference = (void *)songPlaying;
					if (theErr)
					{
						ImportantMessage(("BeginSong error"));
						// Stop();
						EndPlay();	// DRA - Stop does not delete fSongReference if we're not in the play list.
					}
					else
					{
						Assert(gPlayList != nil);
						gPlayList->Add(this);
					}
				}
			}
		}
		
#ifdef VERBOSE	
		TrivialMessage(("MIDI started playing."));
#endif
	}
}

void MIDI::SetVolume(ulong volume)
{
	fVolume = volume;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean MIDI::GetNoFade() const
{
	return fNoFade;
}
#endif

void MIDI::SetNoFade(Boolean value)
{
	fNoFade = value;
}

void MIDI::Stop()
{
	short int	songVolume;

	if (fSongReference && gPlayList->Find(this) != -1)
	{
		// We're going to fade the song out before we stop it.
		
		if (!fNoFade)
		{
			songVolume = GM_GetSongVolume((GM_Song *)fSongReference);
	
			while (songVolume)
			{
				DelayFor(1);
				GM_SetSongVolume((GM_Song *)fSongReference, songVolume);
				songVolume--;
			}
		}
#ifdef VERBOSE	
		Message(("Ending song and killing its reference."));
#endif
		GM_EndAllNotes();	// this will kill the notes. All notes from all songs!!
		// Stop song from playing, free song and related instruments. If there's another
		// song playing using the same instruments, they will stop playing. This also
		// frees the GM_Song structure.
		((GM_Song *)fSongReference)->disposeMidiDataWhenDone = TRUE;	// free our midi pointer
		EndPlay();
		fSongReference = nil;
		gPlayList->Remove(this);
	}
#ifdef VERBOSE
	else
		Message(("Tried to stop but found no song to kill."));
#endif
}

#ifdef DEBUG_TOURIST
Boolean
MIDI::IsPlayListEmpty()
{
	return ((gPlayList == nil) && (gPlayList->GetCount() == 0));
}
#endif /* DEBUG_TOURIST */

#ifdef DEBUG_TOURIST
void
MIDI::NoLoopingInPlayList()
{
	if (gPlayList == nil)
		return;
	
	ulong count = gPlayList->GetCount();
	
	for (int i = 0; i < count; i++)
	{
		MIDI* current = (MIDI*)gPlayList->At(i);
		if (current == nil)
			continue;

		// Set the loop song flag
		GM_Song* songPlaying = (GM_Song*)current->fSongReference;
		if (songPlaying == nil)
			continue;
		
		GM_SetSongLoopFlag(songPlaying, FALSE);
	}
}
#endif /* DEBUG_TOURIST */

void MIDISong::Play(ulong loopCount)
{
	if (!gSystem->GetSongsMuted())
		MIDI::Play(loopCount);
}
void SoundEffect::Play(ulong loopCount)
{
	if (!gSystem->GetSoundEffectsMuted())
		MIDI::Play(loopCount);
}

SoundEffect* SoundEffect::NewSoundEffect(const char* url)
{
	SoundEffect* effect = new(SoundEffect);
	if (effect != nil) {
		effect->fResource.SetURL(url);
		effect->fResource.SetDataType(kDataTypeMIDI);
	}
	return effect;
}

SoundEffect* SoundEffect::NewSoundEffect(const char* url, ulong volume, Boolean noFade)
{
	SoundEffect* effect = NewSoundEffect(url);
	if (effect != nil) {
		effect->SetVolume(volume);
		effect->SetNoFade(noFade);
	}
	return effect;
}

// ¥¥¥¥¥¥
//
//	AudioSetup()
//
//		This function is called ONCE at the startup of the application, or system.
//		It uses the native OS memory manager via the X_API functions. So make sure
//		your memory manager is active before calling.
//
// ¥¥¥¥¥¥
void AudioSetup(void)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (audioEnableCount == 0)
	{
		theErr = GM_InitGeneralSound(Q_22K, 
			E_LINEAR_INTERPOLATION,
			gSystem->GetInStereo() ? (M_USE_STEREO | M_USE_16) : M_USE_16,
			IGOR_MAX_MIDI_VOICES, 
			IGOR_MIX_LEVEL, 
			IGOR_MAX_AUDIO_STREAMS);

		if (theErr)
		{
			GM_FinisGeneralSound();
		}
		else
		{
#if X_HARDWARE_PLATFORM == X_WEBTV
			if(SPOTVersion() != kSPOT1)
			{
				EnableInts(kAudioIntMask);
				StartAudioDMA();
			}
#endif
			audioEnableCount++;
		}
	}

	if (theErr)
	{
		Complain(("pSMS Err"));
	}
}

// ¥¥¥¥¥¥
//
//	AudioCleanup()
//
//		This function is called ONCE at the shutdown of the application, or system.
//		It uses the native OS memory manager via the X_API functions. So make sure
//		your memory manager is active before calling.
//
// ¥¥¥¥¥¥
#ifdef INCLUDE_FINALIZE_CODE
void AudioCleanup(void)
{
	audioEnableCount--;
	if (audioEnableCount == 0)
	{
		GM_FinisGeneralSound();
	}
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void AudioPlayMidiFile(void *pFileName)
{
	SongResource *	theSong;
	OPErr			theErr;
	short int		theSongID;
	GM_Song			*songPlaying;
	FSNode			*sysFile;
	unsigned long	now;

#ifdef VERBOSE
	TrivialMessage(("Starting to play MIDI..."));
#endif
	if (audioEnableCount)
	{
		sysFile = Resolve((char *)pFileName, false);
		if (sysFile)
		{
			// Create a song performance record. This is only used to tell the snyth
			// engine how to play the midi file. Once GM_LoadSong is called, it can be
			// thrown away.
			theSongID = 2000;			// the theSongID is used as a tracking device
										// during song callbacks. Can be anything.
			theSong = NewSongPtr(theSongID,
									IGOR_MAX_MIDI_VOICES,
									IGOR_MIX_LEVEL,
									IGOR_MAX_AUDIO_STREAMS,
									IGOR_REVERB_TYPE);
			if (theSong)
			{
				// load the instruments associated with this midi file.
				songPlaying = GM_LoadSong(theSongID, 
										(void *)theSong, 
										sysFile->data, 
										sysFile->dataLength, 
										nil, TRUE,
										&theErr);
	
				// dispose of the performance record.
				DisposeSongPtr(theSong);
	
				if (songPlaying && theErr == NO_ERR)
				{
					// Start the song playing
#ifdef VERBOSE
					Message(("About to hang in loop 1"));
#endif
					theErr = GM_BeginSong(songPlaying, nil);
				}

#ifdef VERBOSE
				Message(("About to hang in loop 2"));
#endif
				GM_SetSongVolume(songPlaying, MAX_SONG_VOLUME * 4);		// overdrive volume

				// bad news, but useful now. We wait in a loop until song is done.
				// We could set up a callback and then create a queue to free the memory
				// but its too hard right now.
				if (theErr == 0)
				{
					now = Now()+120;
					while (GM_IsSongDone(songPlaying) == FALSE)
					{
						if (now < Now())
						{
							now = Now()+120;
#ifdef VERBOSE
							Message(("Waiting for song to finish..."));
#endif
						}
					}
					GM_FreeSong(songPlaying);
#ifdef VERBOSE
					Message(("Finished playing song."));
#endif
				}
#ifdef VERBOSE
				else
					Message(("Error in BeginSong"));
#endif
			}
		}
	}
}
#endif

// ¥¥¥¥¥¥
//
//	PV_AudioTask()
//
//		Generic useful connection. Called once every audio frame build. It is called
//		from an interrupt.
//
// ¥¥¥¥¥¥
#ifdef SIMULATOR
void
PV_AudioTask(void)
{
}
#endif

// ¥¥¥¥¥¥
//
//	PV_GetExternalTimeSync()
//
//		This functions is used by the external MIDI codebase. If you are going to pass
//		MIDI controllers and other note commands, you need to return a value that changes
//		at least 1 every 1/60th of a second.
// ¥¥¥¥¥¥
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long
PV_GetExternalTimeSync(void)
{
	return Now();
}
#endif
