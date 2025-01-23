// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#ifndef __SONGDATA_H__
#define __SONGDATA_H__

#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

#ifndef __STREAM_H__
#include "Stream.h"
#endif

class SongData {
public:
	virtual					~SongData();
	
	Error					GetStatus() const;
	
	virtual void			Play(ulong loopCount = 0);
	virtual void			Stop();

	void					SetLoopCount(ulong loopCount);
	virtual void			SetVolume(ulong volume);
	
	static SongData*		NewSongData(const Resource*);

protected:
	void					SetResource(const Resource*);
	
public:
	Resource				fResource;
};

class MIDI : public SongData, public Listable {
public:
							MIDI();
	virtual					~MIDI();
							
	virtual void			Play(ulong loopCount = 0);
	virtual void			Stop();

	Boolean					GetNoFade() const;	
	virtual void			SetNoFade(Boolean);
	virtual void			SetVolume(ulong volume);
	Boolean					IsDone();
	void					EndPlay();
	
	static void				Idle();

#ifdef DEBUG_TOURIST
	static Boolean			IsPlayListEmpty(void);
	static void				NoLoopingInPlayList(void);
#endif

protected:
#ifdef FOR_MAC
	Movie					fMovie;
	Boolean					fMovieGood;
	Boolean					fMovieLoaded;
	short					fResRefNum;
#endif
	void*					fSongReference;	// song pointer
	ulong					fVolume;
	Boolean					fNoFade;
};

class MIDISong : public MIDI {
public:
	virtual void			Play(ulong loopCount = 0);
};

class SoundEffect : public MIDI {
public:
	virtual void			Play(ulong loopCount = 0);
	
	static SoundEffect*		NewSoundEffect(const char* url);
	static SoundEffect*		NewSoundEffect(const char* url, ulong volume, Boolean noFade);
};

// called once, for the Macintosh at startup
void AudioSetup(void);

// called once, for the Macintosh at shutdown
void AudioCleanup(void);

// called to play a midi file. Will not return until done.
void AudioPlayMidiFile(void *pFileName);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include SongData.h multiple times"
	#endif
#endif /* __SONGDATA_H__ */
