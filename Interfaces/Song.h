// ===========================================================================
//	Song.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SONG_H__
#define __SONG_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"	/* for Priority and CacheEntry functions (for inlines) */
#endif
#ifndef __DISPLAYABLE_H__
#include "Displayable.h"			/* Displayable */
#endif
#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"			/* Error */
#endif
#ifndef __PARSER_H__
#include "Parser.h"					/* Attribute */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"				/* Resource */
#endif
#ifndef __SONGDATA_H__
#include "SongData.h"				/* SongData */
#endif




// =============================================================================

enum SongIdleState {
	kCreateSongData = 0,
	kBeginPlayingSong,
	kSongDone,
	kSongPlaying,
	kSongError
};

class Song : public Displayable 
{
public:
							Song();
	virtual					~Song();
							
	Priority				GetPriority() const;
	const Resource*			GetResource() const;
	Error					GetStatus() const;

	void					SetAttributeStr(Attribute attributeID, const char* value);
	void					SetLoopCount(ulong loopCount);
	void					SetPriority(Priority);

	void					Idle();
	void					Load(const Resource* parent);
	void					Play();
	void					PlaySoon();
	void					Stop();
	void					AttachPreviousSong(Song* previous);
	
protected:
	void					Reset();
	
	ulong					fLoopCount;
	Resource				fResource;
	SongData*				fSongData;
	char*					fSRC;
	Song*					fPreviousSong;

	SongIdleState			fState;

	Boolean					fAllowedToPlay;
};


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline Priority Song::GetPriority() const
{
	return fResource.GetPriority();
}
#endif

inline const Resource* Song::GetResource() const
{
	return &fResource;
}

inline void Song::Play()
{
	if (fSongData != nil)
		fSongData->Play(fLoopCount);
}

inline void Song::SetLoopCount(ulong loopCount)
{
	fLoopCount = loopCount;
}

inline void Song::SetPriority(Priority priority)
{
	fResource.SetPriority(priority);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline void Song::Stop()
{
	if (fSongData != nil)
		fSongData->Stop();
}
#endif

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Song.h multiple times"
	#endif
#endif /* __SONG_H__ */