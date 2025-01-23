// ===========================================================================
//	Song.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __SONG_H__
#include "Song.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif



// ===========================================================================

Song::Song()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberSong;
#endif /* DEBUG_CLASSNUMBER */
}

Song::~Song()
{
	if (fPreviousSong != nil)
		delete(fPreviousSong);
	if (fSongData != nil) {
		fSongData->Stop();
		delete(fSongData);
	}
	
	if (fSRC != nil)
		FreeTaggedMemory(fSRC, "Song::fSRC");
		
	fResource.SetPriority(kBackground);
}

void Song::AttachPreviousSong(Song* previous)
{
	IsError(fState == kSongPlaying);
	fPreviousSong = previous;
}

Error Song::GetStatus() const
{
	if (fState <= kBeginPlayingSong)
		return kPending;
	if (fState >= kSongDone)
		return kComplete;
	return kGenericError;
}

void Song::Idle()
{
	PerfDump perfdump("Song::Idle");
	
	Postulate(kCreateSongData == 0);
	
	switch (fState) {
	case kCreateSongData:
		if ((fSongData = SongData::NewSongData(&fResource)) != nil)
			fState = kBeginPlayingSong;
		break;
			
	case kBeginPlayingSong:
	{
		Error	resourceStatus = fResource.GetStatus();
		if (resourceStatus == kComplete)
			fState = kSongDone;		// and fall thru
		else
		{
			if (resourceStatus != kPending)
				fState = kSongError;
			break;
		}
	}
		
	case kSongDone:
		if (fAllowedToPlay) {
			if (fPreviousSong != nil)
			{	delete(fPreviousSong); fPreviousSong = nil; }
			Play();
			fState = kSongPlaying;
		}
		break;

	case kSongError:
		if (fPreviousSong != nil)
			{	delete(fPreviousSong); fPreviousSong = nil; }
		break;
	}
	
}

void Song::Load(const Resource* parent)
{
	if (IsError(parent == nil))
		return;

	if (fSRC == nil)
		return;
	
	// Initialize resource.
	fResource.SetURL(fSRC, parent);
	fResource.SetPriority(kSelectable);
	FreeTaggedMemory(fSRC, "Song::fSRC");
	fSRC = nil;
}

void Song::PlaySoon()
{
	Assert(!fAllowedToPlay && fState != kSongPlaying);
	fAllowedToPlay = true;
	
	if (fState == kSongDone) {
		if (fPreviousSong != nil)
		{	delete(fPreviousSong); fPreviousSong = nil; }
		Play();
		fState = kSongPlaying;
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void Song::Reset()
{
	if (fSongData != nil)
	{
		fSongData->Stop();
		delete(fSongData);
		fSongData = nil;
	}
	
	fState = kCreateSongData;
}
#endif

void Song::SetAttributeStr(Attribute attributeID, const char* value)
{	
	switch (attributeID) 
	{		
		case A_SRC:	
			if (fSRC != nil)
			{
				ImportantMessage(("Double source attribute \"%s\" (\"%s\" already set)\n",
					value, fSRC));
				break;	// already have one
			}
			TrivialMessage(("Setting %x->SRC = '%s'", (ulong)this, value));
			fSRC = CopyStringTo(fSRC, value, "Song::fSRC");
			break;
		default:
			Displayable::SetAttributeStr(attributeID, value);
	}
}
