// ===========================================================================
//	Audio.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __AUDIO_H__
#include "Audio.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MPEGAUDIO_H__
#include "MPEGAudio.h"
#endif
#ifndef __REALAUDIO_H__
#include "RealAudio.h"
#endif

AudioStream::AudioStream()
{
}


AudioStream::~AudioStream()
{
	fResource.SetPriority(kBackground);
}

AudioStream* AudioStream::NewAudioStream(const DataType dataType)
{
	AudioStream*	audioStream = nil;
	
	IsError((dataType != kDataTypeMPEGAudio) && 
	        (dataType != kDataTypeRealAudioMetafile) &&
	        (dataType != kDataTypeRealAudioProtocol));
	
	switch (dataType)
	{
		case kDataTypeMPEGAudio:
			audioStream = new(MPEGAudioStream);
			break;
		case kDataTypeRealAudioMetafile:
		case kDataTypeRealAudioProtocol:
#if defined (REALAUDIO) && defined (DEBUG)
			audioStream = new(RealAudioStream);
#endif
			break;
		default:
			Message(("Unhandled audio stream data type, %X",dataType));
			break;
	}
	
	return audioStream;			
}


void AudioStream::Start()
{
	Trespass();
}	


void AudioStream::Stop()
{
	Trespass();	
}


Boolean AudioStream::Idle()
{
	Trespass();	
	return false;		
}


void AudioStream::SetResource(const Resource* parent)
{	
	fResource = *parent;
}


Priority AudioStream::GetPriority() const
{
	return fPriority;
}


Error AudioStream::GetStatus() const
{
	return fState == kAudioError ? kGenericError : kNoError;
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void AudioStream::SetLoopCount(const ulong loopCount)
{
	fLoopCount = loopCount;
}
#endif


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void AudioStream::SetPriority(const Priority priority)
{
	fPriority = priority;
}
#endif
