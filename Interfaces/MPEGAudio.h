

#ifndef	__MPEGAUDIO_H__
#define	__MPEGAUDIO_H__


#ifndef	__AUDIO_H__
#include "Audio.h"
#endif

/* MPEG Header Definitions - Mode Values */

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/* Mode Extention */

#define         MPG_MD_LR_LR             0
#define         MPG_MD_LR_I              1
#define         MPG_MD_MS_LR             2
#define         MPG_MD_MS_I              3

//#define         MIN(A, B)       ((A) < (B) ? (A) : (B))
//#define         MAX(A, B)       ((A) > (B) ? (A) : (B))

/***********************************************************************
*
*  Global Type Definitions
*
***********************************************************************/

//	Header Information Structure */

typedef struct {
    uchar version;
    uchar lay;
    uchar error_protection;
    uchar bitrate_index;
    uchar sampling_frequency;
    uchar padding;
    uchar extension;
    uchar mode;
    uchar mode_ext;
    uchar copyright;
    uchar original;
    uchar emphasis;
} LayerHeader;

//	Parent Structure Interpreting some Frame Parameters in Header

typedef struct {
    LayerHeader		header;		// raw header information
    uchar         	actual_mode;	// when writing IS, may forget if 0 chs
    uchar         	tab_num;		// number of table that is used
    uchar         	stereo;			// 1 for mono, 2 for stereo
    uchar        	jsbound;		// first band of joint stereo coding
    uchar        	sblimit;		// total number of sub bands
} FrameParams;

/***********************************************************************
*
*  Global Variable External Declarations
*
***********************************************************************/


// The Allocation structure contains all useful info to decode a subband

typedef struct {
	Byte steps;
	Byte bits;
} Allocation;



#define getbits(_x) get_bits((_x))
#define get1bit() get_bits(1)


int SubBandSynthesis(long *bandPtr,int channel,short *samples);


#define	bmask(_bc)	((1<<(_bc))-1)


long 	get_byte();
void 	next_start_code();
long 	find_next_start_code();
//#define get_bit() (gBits) ? (fLast32 & (1 << gBits)) : GetBitsRead(1))
#define get_bit() get_bits(1)
#define get_bits(_x) ((_x <= fBits) ? (fLast32 >> (fBits -= _x)) & bmask(_x) : GetBitsRead(_x))
#define next_bits(_x) ((_x <= fBits) ? (fLast32 >> (fBits - _x)) & bmask(_x) : NextBitsRead(_x))
#define skipBits(_x) 	if ((fBits -= _x) < 0) {			\
							if (!fLongs--) ReadStream();	\
							fBits += 32;					\
							fLast32 = *gStream++; }
							
#define BYTEALIGNED (!(fBits & 0x7))


/*
 *  @(#) synthesis_filter.h 1.8, last edit: 6/15/94 16:52:00
 *  @(#) Copyright (C) 1993, 1994 Tobias Bading (bading@cs.tu-berlin.de)
 *  @(#) Berlin University of Technology
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */



#define INTSYNTH

#define INTDCT




// A class for the synthesis filter bank:
// This class does a fast downsampling from 32, 44.1 or 48 kHz to 8 kHz, if ULAW is defined.
// Frequencies above 4 kHz are removed by ignoring higher subbands.

class SynthesisFilter {
public:
					SynthesisFilter();

	// calculate 32 PCM samples and put the into the Obuffer-object

  	void				CalculatePCMSamples (long *samples,ushort *buffer);


protected:

	void 				ComputeNewV(long *samples);
	void 				ComputeSamples(ushort *buffer);

#ifndef INTSYNTH
	  // the scalefactor scales the calculated float pcm samples to short values
	  // (raw pcm samples are in [-1.0, 1.0], if no violations occur)
	ulong 				recommended_scalefactor (void) { return (ulong)(32768.0 / max_violation); }
	ulong 				violations (void) { return range_violations; }
	double				hardest_violation (void) { return max_violation; }
#endif



protected:

	ulong 				fWrittenSamples;
	ulong 				fWritePos;		// 0-15

#ifdef INTSYNTH
	static const long 	fD[512];
	short 				fV1[512], fV2[512];
	short* 				fActualV;				// v1 or v2
#else


	static const double d[512];
	double 				v1[512], v2[512];
	double*				fActualV;				// v1 or v2
	double 				max_violation;
	ulong 				range_violations;
	  // the scalefactor scales the calculated float pcm samples to short values
	  // (raw pcm samples are in [-1.0, 1.0], if no violations occur)
	ulong 				recommended_scalefactor (void) { return (ulong)(32768.0 / max_violation); }
	ulong 				violations (void) { return range_violations; }
	double				hardest_violation (void) { return max_violation; }

#endif
	
	
#ifdef ULAW
	ulong 				offset1, offset2;		// number of samples to skip
	ulong 				remaining_offset;		// number of samples still to skip when entering
												// compute_pcm_samples() next time
	ulong 				highest_subband;		// highest subband to use, e.g. 7 for 32 kHz to 8 kHz conversion
#endif
	

};


#ifdef	FOR_MAC

//#define	USE_MAC_SOUNDMGR

#endif

#ifdef	USE_MAC_SOUNDMGR
pascal void			DoubleBack(SndChannel *chan,SndDoubleBuffer*buffer);
#endif


class	MPEGAudioStream : public AudioStream {
public:
						MPEGAudioStream();
	virtual				~MPEGAudioStream();

	virtual	void		Start();
	virtual void		Stop();
	virtual Boolean		Idle();
	long   				DecodeFrame(ushort *buffer);		// dont call - only for double buffering
	
protected:
	void				CloseStream();
	Boolean				StillPlaying();
	short				OpenStream();
	long				DecodeFrameHeader();
	int 				SubBandSynthesis(long *bandPtr,int channel,ushort *samples);
	
	//	Info at head of each frame
	
	void  				DecodeAllocation2( const Allocation *Alloc[2][32]);
	void   				DecodeScale2(long *scale, const Allocation *Alloc[2][32]);
	
	//	Frame contents
	
	void 				DecodeSamples2(long samples[2][3+1][32],const Allocation *Alloc[2][32], long *scale);
	
	
	long				StartPlaying();
	
	// bitstream
	long				SeekSync();
	long				BytesLeft();
	long				GetByte();
	void				SetStreamPos(long pos);
	long				GetStreamPos();
	long				BitsUsed();
	long				NextBitsRead(long count);
	long				GetBitsRead(long count);
	long				ReadStream();
public:


#ifdef	USE_MAC_SOUNDMGR
	SndDoubleBufferHeader	fDBHeader;
	SndDoubleBuffer 		*fBuffer;
	SndDoubleBackUPP 		fDoubleBackUPP;
	SndChannelPtr			fSndChannel;
#else
	long				fAudioStreamID;
#endif
	
	void				*fSoundBufferA;
	void				*fSoundBufferB;
	void				PickTable();
	
    FrameParams			fFrameParams;
	SynthesisFilter	 	*fSynthesisFilter0;
	SynthesisFilter 	*fSynthesisFilter1;
	Boolean				fHead;
	long				fFrames;
	short				fNumChannels;
	long				fFrequency;
	long				fLastFrame;
	
	long				fStreamBufferLength;
	long				fStreamBytesConsumed;
	long				fStreamBytesRead;
	long				*fStreamBuffer;
	long				*fStreamPtr;
	ulong				fLastBits;
	short				fBits;
	long				fLongs;
	long				fLast32;	// most recent 32 bits in the stream
	
	long				fStarves;
	long				fSyncLoss;
	long				fError;
	
};

const 	kFrameSize=1152;				// fixed size of MPEG Frame 

const	kBufferSize = 40000*4;			// stream read-ahead buffer -variable size must be multiple of 4




#endif	/* __MPEGAUDIO_H__ */

