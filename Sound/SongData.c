// ===========================================================================
//	SongData.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __SONGDATA_H__
#include "SongData.h"
#endif




// ===========================================================================
//	implementation
// ===========================================================================

SongData::~SongData()
{
}

SongData* SongData::NewSongData(const Resource* resource)
{
	Assert(resource != nil);

	SongData* songData;
	
	switch (resource->GetDataType())
	{
		case kDataTypeMIDI:		songData = new(MIDISong); break;
		default:				return nil;
	}
	
	songData->SetResource(resource);
	return songData;
}

void SongData::Play(ulong UNUSED(loopCount))
{
	Trespass();
}

void SongData::SetResource(const Resource* resource)
{
	Assert(resource != nil);
	fResource = *resource;
}

void SongData::SetVolume(ulong UNUSED(volume))
{
}

void SongData::Stop()
{
	Trespass();
}

