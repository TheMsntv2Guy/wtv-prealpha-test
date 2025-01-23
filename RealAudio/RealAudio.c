// ===========================================================================
//	RealAudio.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __REALAUDIO_H__
#include "RealAudio.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif
#ifndef __DECODER_H__
#include "DECODER.H"
#endif

#include "CRaSession.h"

#define	kBadFormat			-5555
#define	kNoSync				-5556
#define	kRanDry				-5557


RealAudioStream::RealAudioStream()
{
	fSession = new(CRaSession);
	
	/* djb - this could only be done once, but it's not that expensive. */
	/* the where is this majik number from? - it's probably the current bandwidth of the */
	/* connection, if we know it */
	CDecoder::set_bandwidth(28800);
	
	fURL = nil;
}


RealAudioStream::~RealAudioStream()
{
	Stop();
	
	if (fSession)
		delete fSession;
	if (fURL)
		FreeTaggedMemory(fURL, "RA: URL");
}


void
RealAudioStream::Start()
{
	if (fStream == nil)
		OpenStream();
		
	if (fStream)
	{
		fStream->Rewind();
		fState = kBeginPlayingAudio;
	}
}


Boolean
RealAudioStream::Idle()
{
	switch (fState) {
	case kCreateAudioData:
		OpenStream();
		if (fStream)
			fState = kBeginPlayingAudio;
		break;
	case kBeginPlayingAudio:
	
		if (fStream) 
		{
			if (StartPlaying() == 0)
				fState = kAudioPlaying;
		}
		break;
		
	case kAudioPlaying:
		
		if (fError)
		{
			Stop();
			fState = kAudioError;
		}
		else 
		{
			StillPlaying();
			if (!fIsPlaying)
			{
				Stop();
				fState = kAudioDone;
			}
		}
		break;
	case kAudioError:
		fError = 0;
		break;
 	}

 	if (fSession)
		fSession->ProcessIdle();

 	return (fIsPlaying);
}


void RealAudioStream::Stop()
{

	fState = kCreateAudioData;
	fIsPlaying = false;
	CloseStream();
}

short RealAudioStream::OpenStream()
{
	if (fResource.GetStatus() != kComplete)
		return 0;
		
	if (fStream)
		delete(fStream);
		
	if (fURL) {
		FreeTaggedMemory(fURL, "RA: URL");
		fURL = nil;
	}
		
	fStream = nil;
		
	fStream = fResource.NewStream();

	if (fStream)
	{
		char* tURL;
		long length = fStream->GetDataLength();
		fStream->Rewind();
		
		tURL = (char*)AllocateTaggedMemory(length + 1, "RA: temp url");
		if (tURL) {
			long i = 0;
			fStream->Read(tURL, length);
			
			/* strip out carriage returns from url */
			tURL[length] = 0;
			while ( tURL[i] != 0)
			{
				if (tURL[i] == '\n')
					tURL[i] = 0;
				i++;
			}
			
			SetURL( tURL );
			
			FreeTaggedMemory( tURL, "RA: temp url");
		}
	}
	
	return 0;
}


void RealAudioStream::CloseStream()
{
	
	if (fStream)
		delete fStream;
	fStream = nil;
	
	if (fSession)
		fSession->Close();
		
	fState = kCreateAudioData;
}

long RealAudioStream::StartPlaying()
{
		
	Assert(fStream != nil);

	if (fSession)
		fSession->Begin();

	fIsPlaying = true;
	return 0;
}

Boolean RealAudioStream::StillPlaying()
{
	return fIsPlaying;
}

void RealAudioStream::SetURL( const char* url ) 
{
	if (url) {
		fURL = (char*)AllocateTaggedMemory(strlen(url) + 1, "RA: URL");
		if (fURL)
		{
			strcpy( fURL, url );	
			fSession->OpenURL( fURL );
		}		
	}
}
