// ===========================================================================
//	Audio.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef	__AUDIO_H__
#define	__AUDIO_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"			/* for Priority */
#endif
#ifndef __DISPLAYABLE_H__
#include "Displayable.h"		/* for Displayable */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"			/* for Resource */
#endif

enum AudioStreamState {
	kCreateAudioData = 0,
	kBeginPlayingAudio,
	kAudioDone,
	kAudioPlaying,
	kAudioError
};

class AudioStream : public Displayable 
{
public:
							AudioStream();
	virtual					~AudioStream();
	static AudioStream*		NewAudioStream(const DataType dataType);
							
	Priority				GetPriority() const;
	const Resource*			GetResource() const;
	Error					GetStatus() const;

	void					SetLoopCount(const ulong loopCount);
	void					SetPriority(const Priority);
	void					SetResource(const Resource* resource);

	virtual	void			Start();
	virtual void			Stop();
	virtual Boolean			Idle();
	
protected:
	void					Reset();
	ulong					fLoopCount;
	Resource				fResource;
	DataStream*				fStream;
	AudioStreamState		fState;
	Boolean					fIsPlaying;
	Priority				fPriority;
};

// ===========================================================================

inline const Resource* AudioStream::GetResource() const
{
	return &fResource;
}

// ===========================================================================
#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Audio.h multiple times"
	#endif
#endif
