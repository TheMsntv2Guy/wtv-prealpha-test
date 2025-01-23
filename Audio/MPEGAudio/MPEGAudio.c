// ===========================================================================
//	MPEGAudio.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MPEGAUDIO_H__
#include "MPEGAudio.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif



#ifdef	USE_MAC_SOUNDMGR
#include 	<Sound.h>
#else
#if defined	FOR_MAC || defined __MWERKS__
#include	"X_API.h"
#include	"PrivateAudioStream.h"
#else
#include	"../../Sound/MIDI/X_API.h"
#include	"../../Sound/MIDI/PrivateAudioStream.h"
#endif
short
AudioStreamCallback(short message, AudioStreamData *pAS);

#endif


#define	kBadFormat			-5555
#define	kNoSync				-5556
#define	kRanDry				-5557



//=================================================================
//	Table 3-B.2a
//	48  kHz		56,64,80,96,112,128,160,192 kbits/s and free
//	44.1kHz		56,64,80 kbits/s
//	32	kHz		56,64,80 kbits/s
//	27 active subbands

//	 0	a a a a a a a a a a a a a a a a
//	 1	a a a a a a a a a a a a a a a a
//	 2	a a a a a a a a a a a a a a a a
//	 3	b b b b b b b b b b b b b b b b
//	 4	b b b b b b b b b b b b b b b b
//	 5	b b b b b b b b b b b b b b b b
//	 6	b b b b b b b b b b b b b b b b
//	 7	b b b b b b b b b b b b b b b b
//	 8	b b b b b b b b b b b b b b b b
//	 9	b b b b b b b b b b b b b b b b
//	10	b b b b b b b b b b b b b b b b
//	11	c c c c c c c c
//	12	c c c c c c c c
//	13	c c c c c c c c
//	14	c c c c c c c c
//	15	c c c c c c c c
//	16	c c c c c c c c
//	17	c c c c c c c c
//	18	c c c c c c c c
//	19	c c c c c c c c
//	20	c c c c c c c c
//	21	c c c c c c c c
//	22	c c c c c c c c
//	23	d d d d
//	24	d d d d
//	25	d d d d
//	26	d d d d

//	Table 3-B.2b
//	48  kHz		--Not Relevant--
//	44.1kHz		96,112,128,160,192 kbits/s and free
//	32	kHz		96,112,128,160,192 kbits/s and free
//	30 active subbands

//	 0	a a a a a a a a a a a a a a a a
//	 1	a a a a a a a a a a a a a a a a
//	 2	a a a a a a a a a a a a a a a a
//	 3	b b b b b b b b b b b b b b b b
//	 4	b b b b b b b b b b b b b b b b
//	 5	b b b b b b b b b b b b b b b b
//	 6	b b b b b b b b b b b b b b b b
//	 7	b b b b b b b b b b b b b b b b
//	 8	b b b b b b b b b b b b b b b b
//	 9	b b b b b b b b b b b b b b b b
//	10	b b b b b b b b b b b b b b b b
//	11	c c c c c c c c
//	12	c c c c c c c c
//	13	c c c c c c c c
//	14	c c c c c c c c
//	15	c c c c c c c c
//	16	c c c c c c c c
//	17	c c c c c c c c
//	18	c c c c c c c c
//	19	c c c c c c c c
//	20	c c c c c c c c
//	21	c c c c c c c c
//	22	c c c c c c c c
//	23	d d d d
//	24	d d d d
//	25	d d d d
//	26	d d d d
//	27	d d d d
//	28	d d d d
//	29	d d d d

//	Note that e is a superset of f

//	Table 3-B.2c
//	48  kHz		32,48 kbits/s
//	44.1kHz		32,48 kbits/s
//	32	kHz		--Not Relevant--
//	8 active subbands

//	 0	e e e e e e e e e e e e e e e e
//	 1	e e e e e e e e e e e e e e e e
//	 2	e e e e e e e e
//	 3	e e e e e e e e
//	 4	e e e e e e e e
//	 5	e e e e e e e e
//	 6	e e e e e e e e
//	 7	e e e e e e e e

//	Table 3-B.2d
//	48  kHz		--Not Relevant--
//	44.1kHz		--Not Relevant--
//	32	kHz		32,48 kbits/s
//	12 active subbands

//	 0	e e e e e e e e e e e e e e e e
//	 1	e e e e e e e e e e e e e e e e
//	 2	e e e e e e e e
//	 3	e e e e e e e e
//	 4	e e e e e e e e
//	 5	e e e e e e e e
//	 6	e e e e e e e e
//	 7	e e e e e e e e
//	 8	e e e e e e e e
//	 9	e e e e e e e e
//	10	e e e e e e e e
//	11	e e e e e e e e

static const Allocation xa[16]={
    { 0,  0 },
    { 3,  5 },
    { 0,  3 },
    { 0,  4 },
    { 0,  5 },
    { 0,  6 },
    { 0,  7 },
    { 0,  8 },
    { 0,  9 },
    { 0, 10 },
    { 0, 11 },
    { 0, 12 },
    { 0, 13 },
    { 0, 14 },
    { 0, 15 },
    { 0, 16 }
};

static const Allocation xb[16]={
    { 0,  0 },
    { 3,  5 },
    { 5,  7 },
    { 0,  3 },
    { 9, 10 },
    { 0,  4 },
    { 0,  5 },
    { 0,  6 },
    { 0,  7 },
    { 0,  8 },
    { 0,  9 },
    { 0, 10 },
    { 0, 11 },
    { 0, 12 },
    { 0, 13 },
    { 0, 16 }
};

static const Allocation xc[8]={
    { 0,  0 },
    { 3,  5 },
    { 5,  7 },
    { 0,  3 },
    { 9, 10 },
    { 0,  4 },
    { 0,  5 },
    { 0, 16 }
};

static const Allocation xd[4]={
    { 0,  0 },
    { 3,  5 },
    { 5,  7 },
    { 0, 16 }
};

static const Allocation xe[16]={
    { 0,  0 },
    { 3,  5 },
    { 5,  7 },
    { 9, 10 },
    { 0,  4 },
    { 0,  5 },
    { 0,  6 },
    { 0,  7 },
    { 0,  8 },
    { 0,  9 },
    { 0, 10 },
    { 0, 11 },
    { 0, 12 },
    { 0, 13 },
    { 0, 14 },
    { 0, 15 }
};

//	Subband limit table

static const Byte SBLimTable[4] = { 27,30, 8,12 };

//	Number of bits in allocation index

static const Byte BitsTable[4*32] = {
	4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,0,0,0,0,0,	// 3-B.2a
	4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,0,0,	// 3-B.2b
	4,4,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 3-B.2c
	4,4,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	// 3-B.2d
};

// (TableID << 5) | allocIndex = Allocation pointer

static const Allocation *AllocationTable[4*32] = {
	xa,xa,xa,xb,xb,xb,xb,xb,xb,xb,xb,xc,xc,xc,xc,xc,xc,xc,xc,xc,xc,xc,xc,xd,xd,xd,xd, 0, 0, 0, 0, 0,	// 3-B.2a
	xa,xa,xa,xb,xb,xb,xb,xb,xb,xb,xb,xc,xc,xc,xc,xc,xc,xc,xc,xc,xc,xc,xc,xd,xd,xd,xd,xd,xd,xd, 0, 0,	// 3-B.2b
	xe,xe,xe,xe,xe,xe,xe,xe, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 3-B.2c
	xe,xe,xe,xe,xe,xe,xe,xe,xe,xe,xe,xe, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 	// 3-B.2d
};





static const char jsb_table[3][4] =  {	{ 4, 8, 12, 16 },
										{ 4, 8, 12, 16 },
                            			{ 0, 4,  8, 16 } };  /* lay+m_e -> jsbound */



static const unsigned short	freq[4] = {44100, 48000, 32000, 0};

//	indexed by layer and bitrate index

static const short 	bitrate[3][15] = {
	{	0,	32,	64,	96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
	{	0,	32,	48,	56,	64,	80,	96, 112, 128, 160, 192, 224, 256, 320, 384 },
	{	0,	32,	40,	48,	56,	64,	80,	96, 112, 128, 160, 192, 224, 256, 320 }
};



//	Multiple*(1 << (16 + 14))

const unsigned long  mmultiple[64]={
2147483648, 1704458901,1352829926,1073741824,
 852229450, 676414963, 536870912, 426114725,
 338207482, 268435456, 213057363, 169103741,
 134217728, 106528681,  84551870,  67108864,
  53264341,  42275935,  33554432,  26632170,
  21137968,  16777216,  13316085,  10568984,
   8388608,   6658043,   5284492,   4194304,
   3329021,   2642246,   2097152,   1664511,
   1321123,   1048576,    832255,    660561,
    524288,    416128,    330281,    262144,
    208064,    165140,    131072,    104032,
     82570,     65536,     52016,     41285,
     32768,     26008,     20643,     16384,
     13004,     10321,      8192,      6502,
      5161,      4096,      3251,      2580,
      2048,      1625,      1290,         0,
};


#define SCALE(_bits, _steps) (mmultiple[_bits] + (_steps >> 1))/_steps;	// Usually _steps = 2^n - 1




MPEGAudioStream::MPEGAudioStream()
{
#ifndef	USE_MAC_SOUNDMGR
	int e = AudioStreamOpen(1);
	if ( e ) {
		Message(("AudioStreamOpen failed %d",e));
	}
#endif
}


MPEGAudioStream::~MPEGAudioStream()
{
		
	Stop();
	if (fSoundBufferA)
		FreeTaggedMemory(fSoundBufferA, "Sound Buffer");
	if (fSoundBufferB)
		FreeTaggedMemory(fSoundBufferB, "Sound Buffer");	
	if (fSynthesisFilter0)
		delete(fSynthesisFilter0);
	if (fSynthesisFilter1)
		delete(fSynthesisFilter1);
#ifndef	USE_MAC_SOUNDMGR
	AudioStreamClose();
#endif	
}


void
MPEGAudioStream::Start()
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
MPEGAudioStream::Idle()
{
	PerfDump perfdump("MPEGAudioStream::Idle");
	AudioStreamService();
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
	case kAudioDone:
		break;
 	}
 	return (fIsPlaying);
}


void MPEGAudioStream::Stop()
{

	if (fFrames && fFrequency)
	{
		long leftInBuffer = fStreamBytesRead-(fLongs<<2);
		long bytesPlayed = fStreamBytesConsumed + leftInBuffer;
		
		if ( fFrames ) 
			Message(("MPEG Audio played %ld samples at %ldHz %ld bytes %ld bytes/sec",
			fFrames * kFrameSize, fFrequency, bytesPlayed, bytesPlayed/((fFrames * kFrameSize)/fFrequency)));

		Message(("Starved %ld times, lost sync %ld times", fStarves, fSyncLoss));
	}
	
	
#ifdef	USE_MAC_SOUNDMGR
	if (fSndChannel)
	{
		short e;
		e = SndDisposeChannel(fSndChannel, true);
		fSndChannel = nil;
		if (e)
			 Message(("SndDisposeChannel: %d", e));
	}
#else 
	AudioStreamStop(fAudioStreamID);
#endif
	if (fSoundBufferA)
		FreeTaggedMemory(fSoundBufferA, "Sound Buffer");
	fSoundBufferA = 0;
	if (fSoundBufferB)
		FreeTaggedMemory(fSoundBufferB, "Sound Buffer");
	fSoundBufferB = 0;
	fFrames = 0;
	fStarves = 0;
	fSyncLoss = 0;
	fState = kCreateAudioData;
	fIsPlaying = false;
	CloseStream();
}





short MPEGAudioStream::OpenStream()
{

	if (fStream)
		delete(fStream);
	fStream = nil;
		
	fStream = fResource.NewStream();

	if (fStream)
	{
		fStream->Rewind();
//#define	USE_BUFFER
#ifdef	USE_BUFFER	
		fStreamBufferLength = kBufferSize;
		if (fStreamBuffer)
			FreeTaggedMemory(fStreamBuffer, "MPEG Stream Buffer");
		if (!(fStreamBuffer = (long *)AllocateTaggedMemory(fStreamBufferLength, "MPEG Stream Buffer", 0)))
		{
			delete(fStream);
			fStream = nil;
			fState = kAudioError;
			return -1;
		}
#endif
		fStreamPtr = fStreamBuffer;
		fStreamBytesConsumed = 0;
		fStreamBytesRead = 0;
		fLongs = 0;
	}
	return 0;
}


void MPEGAudioStream::CloseStream()
{
	
	if (fStream)
		delete fStream;
	fStream = nil;
	if (fStreamBuffer) 
		FreeTaggedMemory(fStreamBuffer, "MPEG Stream Buffer");
	fStreamBuffer = nil;
	fState = kCreateAudioData;
}



const kStartupReadAhead = 6*1024;		// 2 seconds at 28.8

const kDecodeFrameMin = 4096;





//	Read more data into the stream buffer, gets called rarely

long MPEGAudioStream::ReadStream()
{

	long	count = 0;
	
	
	while (fStream->GetStatus() == kPending && fStream->GetPending() < 8)
		;
		
		
	fStreamBytesConsumed += fStreamBytesRead;

#ifdef	USE_BUFFER	
	if (fStream->GetPending() >= fStreamBufferLength) {
		count = fStreamBufferLength;
	} else 
#endif	
	{
		count = fStream->GetPending();
		if ((count & 3) && (count > 3))
		{
			// make sure its on a long boundarie
			count &= ~3; 
		}
	}
	
//if (count > 4096) count = 4096;			//••• testing hack


	fStreamBytesRead = count;
#ifdef	USE_BUFFER	
	CopyMemory(fStream->ReadNext(fStreamBytesRead), fStreamBuffer, count);
	fStreamPtr = fStreamBuffer;
#else
	fStreamPtr = (long *)fStream->ReadNext(fStreamBytesRead);
#endif
	fLongs = (count + 3) >> 2;


	if (fLongs == 0)
	{
		fError = kRanDry;
	}

	return count;
}






//	Decode and play the stream live

long MPEGAudioStream::StartPlaying()
{
	int error;
	Boolean onlyOneBufferLeft = false;
	
	const kMaxChannels = 2;
#ifdef	USE_MAC_SOUNDMGR
	// has sound manager header at start for double buffering callback
	const kSoundBufferHeaderOffset = 12;
#else
	const kSoundBufferHeaderOffset = 0;

	fAudioStreamID = (long)this;

#endif
		
	if ( IsError(fStream == nil ) )
		return kGenericError;
	if (fSynthesisFilter0 == nil)
	{
		fSynthesisFilter0 = new(SynthesisFilter);
		if (fSynthesisFilter0 == nil)
			return kLowMemory;
	}
	if (fSynthesisFilter1 == nil)
	{
		fSynthesisFilter1 = new(SynthesisFilter);
		if (fSynthesisFilter1 == nil)
		{
			delete(fSynthesisFilter0);
			fSynthesisFilter0 = nil;
			return  kLowMemory;
		}
	}
	
	
	if (fSoundBufferA == nil)
		fSoundBufferA = AllocateTaggedMemory(kSoundBufferHeaderOffset + kFrameSize*kMaxChannels*sizeof(ushort), "Sound Buffer");
	if (fSoundBufferB == nil)
		fSoundBufferB = AllocateTaggedMemory(kSoundBufferHeaderOffset + kFrameSize*kMaxChannels*sizeof(ushort), "Sound Buffer");

	if (fSoundBufferA == nil || fSoundBufferB == nil )
	{
		error = kLowMemory;
		goto done;
	}
	fError = 0;
	fSyncLoss = 0;
	fStarves = 0;
	fFrames = 0;
	fLongs = 0;
	fStreamBytesRead = 0;
	fStreamBytesConsumed = 0;
	fStreamBufferLength = kBufferSize;


	// make sure we have enough data to start 
	
	if (fStream->GetStatus() == kPending && fStream->GetPending() < kStartupReadAhead)
		return 1;

	ReadStream();
	while (1) 
	{
		error = DecodeFrame((ushort *)((char *)fSoundBufferA + kSoundBufferHeaderOffset));
		if (error <= 0)
			break;
	}
	if (error < 0) {
		if (error == kBadFormat)
			Message(("Unsupported MPEG Stream Format"));
		if (error == kNoSync)
			Message(("Out of sync in MPEG"));
		goto done;
	}
	if (error == 1 )
		onlyOneBufferLeft = true;
	fFrames++;

#ifdef	USE_MAC_SOUNDMGR

	if (error == 0) {
		do {
			error = DecodeFrame((ushort *)((char *)fSoundBufferB + kSoundBufferHeaderOffset)) != 0;
		} while (error == 2);
		
		if (error < 0) {
			if (error == kBadFormat)
				Message(("Unsupported MPEG Stream Format"));
			if (error == kNoSync)
				Message(("Out of sync in MPEG"));
			goto done;
		}
		if (error == 1 )
			onlyOneBufferLeft = true;
		fFrames++;
	}
#endif


#if	0
	Message(("testing MPEG Decode"));
	while (1) 
	{
		error = DecodeFrame((ushort *)((char *)fSoundBufferA + kSoundBufferHeaderOffset));
		if (error > 0) {
			error = 0;
			goto done;
		}
		if (error < 0) 
		{	
			Message(("Decode error %d", error));
			goto done;
		}
	}
#endif


#ifdef	USE_MAC_SOUNDMGR

//	Create a stupid channel


	if (fSndChannel)
		SndDisposeChannel(fSndChannel, true);
	fSndChannel = nil;
	
	error = SndNewChannel(&fSndChannel, sampledSynth, initStereo, 0);
	if (error) 
	{
		Message(("SndNewChannel: %d\n", error));
		goto done;
	}
	
//	Setup double buffer header


	Assert(fNumChannels >0 && fNumChannels < 3);
	Assert(fFrequency != 0);
	
	if (!fDoubleBackUPP)
		fDoubleBackUPP = NewSndDoubleBackProc(DoubleBack);
	fDBHeader.dbhNumChannels = fNumChannels;
	fDBHeader.dbhSampleSize = 16;
	fDBHeader.dbhCompressionID = 0;
	fDBHeader.dbhPacketSize = 0;
	fDBHeader.dbhSampleRate = fFrequency << 16;
	fDBHeader.dbhDoubleBack = fDoubleBackUPP;
	fDBHeader.dbhBufferPtr[0] = (SndDoubleBuffer *)fSoundBufferA;
	fDBHeader.dbhBufferPtr[1] = (SndDoubleBuffer *)fSoundBufferB;
	
//	Create buffers
	
	fBuffer = (SndDoubleBuffer *)fSoundBufferA;
	fBuffer->dbUserInfo[0] = (long)this;
	fBuffer->dbUserInfo[1] = 0;
	fBuffer->dbNumFrames = kFrameSize;
	if (onlyOneBufferLeft)
		fBuffer->dbFlags = dbLastBuffer+dbBufferReady;
	else
		fBuffer->dbFlags = dbBufferReady;
	
	fBuffer = (SndDoubleBuffer *)fSoundBufferB;
	fBuffer->dbUserInfo[0] = (long)this;
	fBuffer->dbUserInfo[1] = 0;
	if (onlyOneBufferLeft)
	{
		fBuffer->dbNumFrames = 0;
		fBuffer->dbFlags = dbLastBuffer+dbBufferReady;
	}
	else
	{
		fBuffer->dbNumFrames = kFrameSize;
		fBuffer->dbFlags = dbBufferReady;
	}
	
//	Play sound

//	Message(("Playing %d channel at %dHz…\n", channels, frequency));

	error = SndPlayDoubleBuffer(fSndChannel, &fDBHeader);
	if (error)
		Message(("SndPlayDoubleBuffer error: %d", error));

#else

	error = AudioStreamStart(fAudioStreamID, AudioStreamCallback, kBufferSize,fFrequency << 16,16,fNumChannels);		
	if (error == -1)
		Message(("AudioStreamStart error"));
	else
		error = 0;
#endif



done:
	if (error) {
		if (fSoundBufferA)
			FreeTaggedMemory(fSoundBufferA, "Sound Buffer");
		if (fSoundBufferB)
			FreeTaggedMemory(fSoundBufferB, "Sound Buffer");	
		fSoundBufferA = nil;
		fSoundBufferB= nil;
		return error;
	}
	fIsPlaying = true;
	fLastFrame = 0;
	fLastBits = 0;
	return 0;
}





//	Interpret Frame parameters

long MPEGAudioStream::DecodeFrameHeader()
{
    LayerHeader *hdr = &fFrameParams.header;
	ulong	 header_bits = getbits(20);
	
	hdr->version = (header_bits>>19) & 1;
	hdr->lay = 4 - ((header_bits>>17) & 3);
	hdr->error_protection = !((header_bits>>16) & 1) ; /* error protect. TRUE/FALSE */
	hdr->bitrate_index = ((header_bits>>12) & 0xf);
	hdr->sampling_frequency = ((header_bits>>10) & 0x3);
	hdr->padding = ((header_bits>>9) & 0x1);
	hdr->extension = ((header_bits>>8) & 0x1);
	hdr->mode = ((header_bits>>6) & 0x3);
	hdr->mode_ext = ((header_bits>>4) & 0x3);
	hdr->copyright = ((header_bits>>3) & 0x1);
	hdr->original = ((header_bits>>2) & 0x1);
	hdr->emphasis = (header_bits & 0x3);
	
	fFrameParams.actual_mode = hdr->mode;
	fFrameParams.stereo = (hdr->mode == MPG_MD_MONO) ? 1 : 2;
	
//	Decide which allocation table to use

	fFrameParams.sblimit = 32;
	
	if (hdr->lay == 2)
	{
		int tabnum, bitsPerChannel, sfreq;
		
		bitsPerChannel = bitrate[hdr->lay - 1][hdr->bitrate_index] / fFrameParams.stereo;
		sfreq = hdr->sampling_frequency;
		
		// decision rules refer to per-channel bitrates (kbits/sec/chan)
		
		if ((sfreq == 1 && bitsPerChannel >= 56) || (bitsPerChannel >= 56 && bitsPerChannel <= 80)) 
			tabnum = 0;
		else if (sfreq != 1 && bitsPerChannel >= 96)
			tabnum = 1;
		else if (sfreq != 2 && bitsPerChannel <= 48) 
			tabnum = 2;
		else 
			tabnum = 3;
		fFrameParams.tab_num = tabnum;
		fFrameParams.sblimit =  SBLimTable[tabnum];
	}
	else
	{
		fError = kBadFormat;
		return -1;
	}
	
	if (hdr->mode == MPG_MD_JOINT_STEREO)
	{
		if (hdr->lay<1 || hdr->lay >3 ||  hdr->mode_ext>3) 
		{	
			fError = kBadFormat;
	//		Complain(("js_bound bad layer/modext (%d/%d)", hdr->lay, hdr->mode_ext));
			return -1;
		}
  	 	fFrameParams.jsbound = jsb_table[hdr->lay-1][hdr->mode_ext];
	}
	else
		fFrameParams.jsbound = fFrameParams.sblimit;
	return 0;
}

//	Decode Bit allocation

void MPEGAudioStream::DecodeAllocation2(const Allocation *Alloc[2][32])
{
    int i, j, b;
    int stereo = fFrameParams.stereo;
    int sblimit = fFrameParams.sblimit;
    int jsbound = fFrameParams.jsbound;		// Joint Stereo Bound

	const Allocation **table = AllocationTable + (fFrameParams.tab_num << 5);	// Point to the right table
	const Byte *bits = BitsTable + (fFrameParams.tab_num << 5);
	
	for (i=0;i<jsbound;i++)
		for (j=0;j<stereo;j++) {
			b = (char)getbits(bits[i]);
			Alloc[j][i] = b ? table[i] + b : 0;
		}
	
	for (i=jsbound;i<sblimit;i++) {		// expand to 2 channels
		b = (char) getbits(bits[i]);
		Alloc[0][i] = Alloc[1][i] = b ? table[i] + b : 0;
	}
	
	for (i=sblimit;i<32;i++) {
		for (j=0;j<stereo;j++)
			Alloc[j][i] = 0;
	}
}



void MPEGAudioStream::DecodeScale2(long *scale, const Allocation *Alloc[2][32])
{
	int i, j, highBand = 0;
	int stereo = fFrameParams.stereo;
	int sblimit = fFrameParams.sblimit;
	int scfsi[2][32];
   
	for (i=0;i<sblimit;i++)
	for (j=0;j<stereo;j++)
		if (Alloc[j][i]) {
			scfsi[j][i] = (char) getbits(2);	// Scale factor coding
			highBand = i;
		}
	if (fFrameParams.actual_mode != MPG_MD_JOINT_STEREO)	// Small optimization that could work with js but does not
		fFrameParams.sblimit = sblimit = highBand + 1;
	
	for (i=0;i<sblimit;i++)
	for (j=0;j<stereo;j++) {
		if (Alloc[j][i])  {
			long steps;
			if (!(steps = Alloc[j][i]->steps))
				steps = (1 << Alloc[j][i]->bits) - 1;
			
			switch (scfsi[j][i]) {
				case 0:
					scale[0]  = SCALE(getbits(6), steps);	// all three scale factors transmitted
					scale[64] = SCALE(getbits(6), steps);
					scale[128] = SCALE(getbits(6), steps);	break;
				case 1:
					scale[0]  =								// scale factor 1 & 3 transmitted
					scale[64] = SCALE(getbits(6), steps);
					scale[128] = SCALE(getbits(6), steps);	break;
				case 3:
					scale[0]  = SCALE(getbits(6), steps);	// scale factor 1 & 2 transmitted
					scale[64] =
					scale[128] = SCALE(getbits(6), steps);	break;
				case 2:
					scale[0] =								// one scale factor transmitted
					scale[64] =
					scale[128] = SCALE(getbits(6), steps);	break;
			}
			scale++;
		}
	}
}







#define kScaleBits 14



//	Decode, dequantize and scale samples

void MPEGAudioStream::DecodeSamples2(long samples[2][3+1][32], const Allocation *Alloc[2][32], long *scale)
{
	int i, j, k, m;
	int stereo = fFrameParams.stereo;
	int sblimit = fFrameParams.sblimit;
	int jsbound = fFrameParams.jsbound;
	unsigned int nlevels;
	long f;
	const Allocation *a;
	
    for (i=0;i<sblimit;i++)
    	for (j=0;j<((i<jsbound)?stereo:1);j++) {				// Joint Stereo
			
			if ( (a= Alloc[j][i]) != 0 ) {
				k = a->bits;
				if ((nlevels = a->steps) > 0) {
					unsigned int c = getbits(k);				// Group of three samples
					for (m=0;m<3;m++) {
						f = (c % nlevels) - (nlevels >> 1);		// Sign
						c /= nlevels;
						samples[j][m][i] = (f*scale[0] >> (kScaleBits + 0));	// Dequantize and scale, OFTEN ZERO
//						TRACKSAMPLE(samples[j][m][i], f, nlevels, scale[0]);
					}
				} else {
					for (m=0;m<3;m++) {
						f = ((getbits(k) + 1) - (1 << (k-1)));	// Sign
						samples[j][m][i] = (f*scale[0] >>  (kScaleBits + 0));	// Dequantize and scale, OFTEN ZERO
//						TRACKSAMPLE(samples[j][m][i], f, (1 << k)-1, scale[0]);
					}
				}
				scale++;
			} else {
				samples[j][0][i] = 0;			// No sample transmitted
				samples[j][1][i] = 0;
				samples[j][2][i] = 0;
			}

			if (stereo == 2 && i>= jsbound)	{	// joint stereo : copy L to R
				samples[1][0][i] = samples[0][0][i];
				samples[1][1][i] = samples[0][1][i];
				samples[1][2][i] = samples[0][2][i];
			}
    }
    
    for (i = sblimit; i < 32; i++)			// Fill in space with no data
    	for (j = 0; j < stereo; j++) {
			samples[j][0][i] = 0;
			samples[j][1][i] = 0;
			samples[j][2][i] = 0;				// No sample here
		}
}      


//	Seek to the next 0xFFF in the stream

long  MPEGAudioStream::SeekSync()
{
	unsigned long aligning;
	long sync;
	long sc = 4;
	
	aligning = (32-fBits) & 7;
	if (aligning)
	{
		getbits((int)(8-aligning));
	}
	while ((sync = next_bits(12)) != 0xFFF) {
		if (BytesLeft() == 0) 
			return 0;
		getbits(8);
		if (--sc < 0)
		{
			fSyncLoss++;
	//		Complain(("sync error"));
		}
	}
	sync = getbits(12);

	return(1);
}




//	Decode the next frame of audio into a buffer, returns 1 if at end of stream, 0 if decoded okay or < 0 for errors

long 
MPEGAudioStream::DecodeFrame(ushort *buffer) 
{

	long		scale[2*32*3];
	long		samples[2][3+1][32];

	const Allocation	*Alloc[2][32];		// Allocations info for each subband
	
#if	1
	if (fStream->GetStatus() == kPending && BytesLeft() < kDecodeFrameMin)
		return 2;
#endif

	if (!SeekSync()) {
		fError = kNoSync;
		return fError;
	}

//	Start decoding the file

    fFrameParams.tab_num = 0;
    
	if (DecodeFrameHeader() != 0) 
		return -1;
			
	if (fFrameParams.header.error_protection)		// Ignore CRC
		getbits(16);
	
	fNumChannels = fFrameParams.stereo;
	fFrequency = freq[fFrameParams.header.sampling_frequency];
			
//	Level II Decode
				
	DecodeAllocation2(Alloc);		// Bit allocation
	
	if (fError)
		return fError;
		
	DecodeScale2(scale, Alloc);		// Scale Factors
				
	if (fError)
		return fError;
		
//	Decode 12 groups of samples

	for (short i=0; i<12; i++) {
		DecodeSamples2(samples, Alloc, scale + ((i>>2) << 6));	
		if (fError)
			return fError;
		for (short j=0; j<3; j++) {
			ushort left[32];
			ushort right[32];
			if (fNumChannels == 2) {
				fSynthesisFilter0->CalculatePCMSamples(&(samples[0][j][0]), left);
				fSynthesisFilter1->CalculatePCMSamples(&(samples[1][j][0]), right);
				for (short k = 0; k < 32; k++) {	
					*buffer++ = left[k];
					*buffer++ = right[k];	// Interleave stereo samples
				}
			} else {
				fSynthesisFilter0->CalculatePCMSamples(&(samples[0][j][0]), buffer);
				buffer += 32;
			}
		}
	}
	if (fStream->GetStatus() == kComplete && BytesLeft() < 6)			// not enough for new frame header and sync, this must be the last one
		return 1;
	return 0;
}


//==========================================================================
//==========================================================================


Boolean MPEGAudioStream::StillPlaying()
{
#ifdef	USE_MAC_SOUNDMGR
	SCStatus status;

	SndChannelStatus(fSndChannel, sizeof(status), &status);
	fIsPlaying = status.scChannelBusy != 0;
#else
	IsAudioStreamPlaying(fAudioStreamID);
#endif
	return fIsPlaying;
}



//	If count > fBits we need to get another long

long MPEGAudioStream::GetBitsRead(long count)
{
	ulong	a = (fLast32 & bmask(fBits)) << (count -= fBits);

	if (fLongs == 0)
	{
		ReadStream();
		if (fLongs == 0 )
		{
			return 0;
		}
	}
	fLongs--;
	fLast32 = *fStreamPtr++;
	fBits = 32 - count;
	if (fBits == 32)
		return a;
	else
		return a | (fLast32 >> fBits) & bmask(count);
}

//	If count > fBits we need to get another long, don't change fLast32, fBits or fLongs

long MPEGAudioStream::NextBitsRead(long count)
{
	ulong	last32;
	ulong	a = (fLast32 & bmask(fBits)) << (count -= fBits);
	if (fLongs == 0)
	{
		ReadStream();
		if (fLongs == 0) {
			return 0;
		}
	}
	last32 = *fStreamPtr;

	if (count == 0)
		return a;
	else
		return a | (last32 >> (32 - count)) & bmask(count);
}

long MPEGAudioStream::BytesLeft()
{
	return (fLongs<<2) + fStream->GetPending();
}






#ifdef	USE_MAC_SOUNDMGR


// double buffer call


pascal void	
DoubleBack(SndChannel *chan, SndDoubleBuffer* header)
{
	if (chan && header )
	{
		ushort *buf = (ushort *)header->dbSoundData;
		MPEGAudioStream *mpa = (MPEGAudioStream*)header->dbUserInfo[0];

		if (mpa)
		{
			long stat = mpa->DecodeFrame(buf);
			
			switch (stat)	{
			case 0:					// decoded
				mpa->fFrames++;
				header->dbNumFrames = kFrameSize;
				header->dbFlags |= dbBufferReady;
				break;
			case 1:					// decoded and completed
				header->dbNumFrames = kFrameSize;
				header->dbFlags |= dbLastBuffer+dbBufferReady;
				break;
			case 2:					// ran out of data
				mpa->fStarves++;
				header->dbNumFrames = 0;
				header->dbFlags |= dbBufferReady;
				break;
			default:				// error
				mpa->fError = stat;
				header->dbNumFrames = 0;
				header->dbFlags |= dbLastBuffer + dbBufferReady;
				break;
			}
		}
	}
}

#else

short
AudioStreamCallback(short message, AudioStreamData *pAS)
{
	short result = SMS_NO_ERR;

	
	switch ( message ) {
	case AS_CREATE:
		TrivialMessage(("MPEG Audio start callback len %ld bits %hd chans %hd rate %lx ref %lx",
			pAS->dataLength,pAS->dataBitSize,pAS->channelSize,pAS->sampleRate,(ulong)pAS->asReference));
			
		pAS->pData = AllocateTaggedMemory( pAS->dataLength * (pAS->dataBitSize * pAS->channelSize) >>3,"MPEG Audio Data");
		if ( pAS->pData == nil )
			result = SMS_MEMORY_ERR;
		break;

	case AS_DESTROY:
		
		FreeTaggedMemory(pAS->pData,"MPEG Audio Data");
		break;
		
	case AS_GET_DATA:


		MPEGAudioStream *mpa = (MPEGAudioStream*)pAS->asReference;
		long stat = mpa->DecodeFrame((ushort *)pAS->pData);
		switch (stat)	{
		case 0:					// decoded
			mpa->fFrames++;
			pAS->dataLength = kFrameSize;
			break;
		case 1:					// decoded and completed
			result = SMS_STREAM_STOP_PLAY;
			pAS->dataLength = kFrameSize;
			break;
		case 2:					// ran out of data
			mpa->fStarves++;
			pAS->dataLength = 0;
			break;
		default:				// error
			mpa->fError = stat;
			pAS->dataLength = 0;
			result = SMS_PARAM_ERR;
			break;
		}
		break;
	}
	return result;
}


#endif









#if	0


//	Returns the number of bits used, wraps now and then

long	MPEGAudioStream::BitsUsed() {
	long leftInBuffer = fStreamBytesRead-(fLongs<<2);
	return ((fStreamBytesConsumed-leftInBuffer)<<5) - fBits;
}


// Return the byte aligned stream position

long	MPEGAudioStream::GetStreamPos()				
{
	return ((fStreamPtr - fStreamBuffer) << 2) - (fBits >> 3);	// Bits used
}

// Set the byte aligned stream position

void	MPEGAudioStream::SetStreamPos(long pos) 
{
	fStreamPtr = fStreamBuffer + (pos >> 2);
	fBits = 32 - ((pos & 0x3) << 3);
	if (fBits) 
		fLast32 = *fStreamPtr++;
}

//=================================================================================
//=================================================================================
//	The following should all be byte aligned. They probably need special treatment

//	Pull an aligned byte from the stream, could be sped up

long MPEGAudioStream::GetByte()
{
	if (!BYTEALIGNED)
		Complain(("Non byte aligned in get_byte.  MPEG file is corrupted"));
	return get_bits(8);
}

#endif







