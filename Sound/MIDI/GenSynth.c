/*
********************************************************************************************
**
** ÒGenSynth.cÓ
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**	Confidential-- Internal use only
**
**	© Copyright 1983-1996 by Steve Hales, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
**	No portion of this program (whether in source, object, or other form)
**	may be copied, duplicated, distributed, altered, or otherwise utilized without
**	direct consent of the copyright holder.
**
** Overview
**	General purpose Music Synthesis software, C-only implementation 
**	(no assembly language optimizations made)
**
** Modification History:
**
**  6/7/93 		begin real serious work
**  6/8/93 		make a stab at getting MIDI performance to work
** 	8/17/93		improved note prioritization
**	8/18/93 		Implement complete API; do interpolated versions of loops and amp. scaling
**	8/21/93		Fixed bug with NoteDur processing samples after note has released
**	8/21/93		Fixed bug with NoteNextSize not being setup correctly inside of ServeMidiNote
**	8/22/93		Incorporated Windows changes for Microsoft C++ Compiler
**	8/23/93		Added new parameters to InitGeneralSound
**	8/24/93		Added even more new parameters to InitGeneralSound
**	8/24/93		Verified Cross platform compiled code
**	10/12/94	fixed pitch tables
**	11/7/95		Major changes, revised just about everything.
**	11/14/95	Fixed volume scale problem. Forgot to scale volume based upon song level
**	11/20/95	Removed the BF_ flags, now you must walk through the union structure
**				Forgot to set songVolume
**				Remove bit fields. BIT FIELDS DON'T WORK WITH MPW!!!!
**	11/24/95	Touched up GM_EndSample & GM_IsSoundDone & GM_BeginDoubleBuffer a bit
**   12/95 (jln)	upgraded mixing bus to 32 bit; improved scaleback resolution; added reverb unit; first pass at volume ADSR
**	12/6/95 (sh)	Moved function GM_GetAudioSampleFrame into GenSynth.c
**				Added GM_SetReverbType
**	12/7/95		Added reverb enable via channel controlers in ServeMIDINote
**				Removed some INTEL stuff
**				Moved DEFAULT_REVERB_TYPE to GENSND.H
**				Added GM_GetReverbType
**				Added some more behavior for note removal for sustaining notes
**	1/4/96		Added GM_ChangeSampleReverb for sound effects
**				Changed behavior for setting sample volumes
**	1/7/96		Changed GM_BeginDoubleBuffer to use a 32 bit value for volume
**				Changed PV_ScaleVolumeFromChannelAndSong to correct bug with sound effects
**				and song volume
**				Changed GM_BeginDoubleBuffer & GM_BeginSample to support effectsVolume
**	1/10/96		Split bulk of init code into GenSetup.c
**	1/18/96		Spruced up for C++ extra error checking
**				Added MIN_LOOP_SIZE
**	1/21/96		Changed references to 'true' & 'false' to TRUE and FALSE
**	1/28/96		Fixed 32 bit value bug in function ServeMIDINote
**	1/29/96		Added useSampleRate factor for playback of instruments
**	1/30/96		Fixed useSampleRate in ServeMIDINote
**	2/5/96		Removed unused variables. Working towards multiple songs
**	2/11/96		Removed FixedDivide & FixedMultiply. Use XFixedDivide & XFixedMultiply
**	2/13/96		Moved MusicIRQ into its own function, PV_ProcessSequencerEvents, and now called it
**				from ProcessSampleFrame.
**				Renamed MusicIRQ to PV_MusicIRQ;
**	2/18/96		Added inst_struct->panPlacement use in ServeMidiNote
**	3/1/96		Changed static variables to static const
**	3/6/96		Fixed a divide by zero error in PV_ADSRModule
**	4/6/96		Added default velocity translation curve
**	5/15/96		Removed reference to SwapWord
**
**
********************************************************************************************
*/
#include "GenSnd.h"
#include "GenPriv.h"

#define CLIP(LIMIT_VAR, LIMIT_LOWER, LIMIT_UPPER) if (LIMIT_VAR < LIMIT_LOWER) LIMIT_VAR = LIMIT_LOWER; if (LIMIT_VAR > LIMIT_UPPER) LIMIT_VAR = LIMIT_UPPER;

MusicVars * MusicGlobals = NULL;

// Variables - pitch tables

#define ys	97271
#define PTmake(x)		\
			(539*ys)/(x),	\
			(571*ys)/(x),	\
			(605*ys)/(x),	\
			(641*ys)/(x),	\
			(679*ys)/(x),	\
			(719*ys)/(x),	\
			(762*ys)/(x),	\
			(807*ys)/(x),	\
			(855*ys)/(x),	\
			(906*ys)/(x),	\
			(960*ys)/(x),	\
			(1017*ys)/(x)

static const UINT32 majorPitchTable[192] = 
{
	PTmake	(102400),
	PTmake	(51200),
	PTmake	(25600),		// 0..11
	PTmake	(12800),		// 12..23
	PTmake	(6400),		// 24..35
	PTmake	(3200),		// 36..47
	PTmake	(1600),		// 48..59
	PTmake	(800),		// 60-71: first entry of this table should = $1,0000.
	PTmake	(400),		// 72-83
	PTmake	(200),		// 84-95
	PTmake	(100),		// 96-107
	PTmake	(50),		// 108-119
	PTmake	(25),		// 120-127 ($80-up unused)
	PTmake	(25),		// MPW probably won't handle this the same.  This means
	PTmake	(25),		// divide by 25 then multiply by 2.  Same as divide by 12.5
	PTmake	(25)
};

// Needs to be recomputed if the synthesis time per chunk is ever different than 11 ms.
static const INT32 logLookupTable[] =
{ 0, 1000000, 5000000, 400000, 3000000, 2500000, 2000000, 1500000, 1000000, 
        700000,500000,400000,250000,200000,150000,150000,100000,100000,100000,100000};

static const UINT32 expDecayLookup[] =
{ 65536, 55110, 60678, 62370, 63060, 63592, 63876, 64132, 64286, 64438, 64534, 64634, 64716, 64716, 64830, 64830, 64918,64918, 64986, 64986,	// 0, 50-950 ms
  65040, 65040, 65084, 65084, 65120, 65120, 65152, 65152, 65180, 65180, 65204, 65204, 65224, 65224, 65244, 65244, 65260,65260, 65274, 65274,	// 1000-1950 ms
  65288, 65288, 65296, 65296, 65306, 65306, 65316, 65316, 65326, 65326, 65336, 65336, 65342, 65342, 65350, 65350, 65356,65356, 65364, 65364,	// 2000-2950ms
  65372, 65372, 65376, 65376, 65380, 65380, 65384, 65384, 65388, 65388, 65394, 65394, 65396, 65396, 65400, 65400, 65404,65404, 65408, 65408,	// 3000-3950ms
  65412, 65412, 65414, 65414, 65416, 65416, 65418, 65418, 65422, 65422, 65424, 65424, 65426, 65426, 65430, 65430, 65432,65432, 65434, 65434, 	// 4000ms
  65438, 65438, 65438, 65438, 65440, 65440, 65442, 65442, 65444, 65444, 65446, 65446, 65446, 65446, 65448, 65448, 65450,65450, 65452, 65452, 	// 5000ms
  65454, 65454, 65454, 65454, 65456, 65456, 65456, 65456, 65458, 65458, 65460, 65460, 65460, 65460, 65462, 65462, 65462,65462, 65464, 65464, 	// 6000ms
  65466, 65466, 65466, 65466, 65466, 65466, 65468, 65468, 65468, 65468, 65470, 65470, 65470, 65470, 65470, 65470, 65472,65472, 65472, 65472, 	// 7000ms
  65474, 65474, 65474, 65474, 65474, 65474, 65476, 65476, 65476, 65476, 65478, 65478, 65478, 65478, 65478, 65478, 65480,65480, 65480, 65480, 	// 8000ms
  65482, 65482, 65482, 65482, 65482, 65482, 65482, 65482, 65482, 65482, 65484, 65484, 65484, 65484, 65484, 65484, 65484,65484, 65484, 65484, 	// 9000ms
  65486, 65486, 65486, 65486, 65486, 65486, 65486, 65486, 65486, 65486, 65486, 65486, 65488, 65488, 65488, 65488, 65488,65488, 65488, 65488, 	// 10 seconds
  65490, 65490, 65490, 65490, 65490, 65490, 65490, 65490, 65490, 65490, 65490, 65490, 65492, 65492, 65492, 65492, 65492,65492, 65492, 65492, 	// 11 seconds
  65494, 65494, 65494, 65494, 65494, 65494, 65494, 65494, 65494, 65494, 65496, 65496, 65496, 65496, 65496, 65496, 65496,65496, 65496, 65496, 	// 12 seconds
  65498, 65498, 65498, 65498, 65498, 65498, 65498, 65498, 65498, 65498, 65498, 65498, 65500, 65500, 65500, 65500, 65500,65500, 65500, 65500, 	// 13 seconds
  65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502, 65502,65502, 65502, 65502,	// 14 seconds
  65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504, 65504,65504, 65504, 65504 	// 15 seconds  
 };

static const UINT32 fractionalPitchTable[] = 
{
	65536,  65566,  65595,  65625,  65654,  65684,  65714,  65743, 
	65773,  65803,  65832,  65862,  65892,  65922,  65951,  65981, 
	66011,  66041,  66071,  66100,  66130,  66160,  66190,  66220, 
	66250,  66280,  66309,  66339,  66369,  66399,  66429,  66459, 
	66489,  66519,  66549,  66579,  66609,  66639,  66670,  66700, 
	66730,  66760,  66790,  66820,  66850,  66880,  66911,  66941, 
	66971,  67001,  67032,  67062,  67092,  67122,  67153,  67183, 
	67213,  67244,  67274,  67304,  67335,  67365,  67395,  67426, 
	67456,  67487,  67517,  67548,  67578,  67609,  67639,  67670, 
	67700,  67731,  67761,  67792,  67823,  67853,  67884,  67915, 
	67945,  67976,  68007,  68037,  68068,  68099,  68129,  68160, 
	68191,  68222,  68252,  68283,  68314,  68345,  68376,  68407, 
	68438,  68468,  68499,  68530,  68561,  68592,  68623,  68654, 
	68685,  68716,  68747,  68778,  68809,  68840,  68871,  68902, 
	68933,  68965,  68996,  69027,  69058,  69089,  69120,  69152, 
	69183,  69214,  69245,  69276,  69308,  69339,  69370,  69402	
};

static const UBYTE defaultVolumeScale[] = {
// Subtle curve that is above zero
							254, 248, 243, 238, 232, 227, 222, 217, 213, 208, 203, 199, 194, 190, 186, 182, 178, 174,
							170, 166, 163, 159, 156, 152, 149, 146, 142, 139, 136, 133, 130, 127, 125, 122, 119, 117,
							114, 112, 109, 107, 104, 102, 100, 98, 96, 93, 91, 89, 87, 85, 84, 82, 80, 78, 76, 75,
							73, 72, 70, 68, 67, 65, 64, 63, 61, 60, 59, 57, 56, 55, 54, 52, 51, 50, 49, 48, 47, 46,
							45, 44, 43, 42, 41, 40, 39, 38, 38, 37, 36, 35, 34, 34, 33, 32, 31, 31, 30, 29, 29, 28,
							28, 27, 26, 26, 25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 18, 17,
							17, 17, 16, 16, 15, 0
};

/*
static const UBYTE defaultVolumeScale[] = {
// Subtle curve that is above zero
							127, 125, 123, 121, 119, 117, 115, 113, 111, 109, 108, 106, 104, 102, 101, 99, 97,
							96, 94, 93, 91, 90, 88, 87, 85, 84, 82, 81, 80, 78, 77, 76, 75, 73, 72, 71, 70, 69,
							67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 53, 52, 51, 50, 49, 48,
							48, 47, 46, 45, 44, 44, 43, 42, 42, 41, 40, 40, 39, 38, 38, 37, 36, 36, 35, 35, 34,
							33, 33, 32, 32, 31, 31, 30, 30, 29, 29, 28, 28, 27, 27, 27, 26, 26, 25, 25, 24, 24,
							24, 23, 23, 22, 22, 22, 21, 21, 21, 20, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17, 17,
							17, 16, 16, 16, 16, 0
};

static const UBYTE defaultVolumeScale[] = {
// harsh curve that ends into zero
								127, 124, 121, 118, 115, 112, 109, 106, 104, 101, 98, 96, 93, 91, 89, 87, 84, 82, 80,
								78, 76, 74, 72, 71, 69, 67, 65, 64, 62, 60, 59, 57, 56, 55, 53, 52, 51, 49, 48, 47, 46,
								44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 34, 33, 32, 31, 30, 29, 29, 28, 27, 27, 26,
								25, 25, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16, 16, 15, 15,
								14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 10, 10, 10, 10, 9, 9, 9, 9, 8, 8, 8,
								8, 8, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 5, 5, 3, 1, 0
};
*/

void PV_DoCallBack(NoteRecord *this_one)
{
	if (this_one->NoteEndCallback)
	{
		(*this_one->NoteEndCallback)((INT32)this_one->NoteRefNum);
	}
}

void PV_CleanNoteEntry(NoteRecord * the_entry)
{
	register char	*p;
	register long	size;

	p = (char *)the_entry;
	p += sizeof(INT16);
	size = sizeof(NoteRecord) - sizeof(INT16);
	while (size--)
	{
		*p++ = 0;
	}
}



// Compute scale back amplification factors. Used to amplify and scale the processed audio frame.
//
//	Relies upon:
//	MusicGlobals->MaxNotes
//	MusicGlobals->MaxEffects
//	MusicGlobals->MaxNormNotes
//	MusicGlobals->MasterVolume
//
void PV_CalcScaleBack(void)
{
	register INT32			scaleSize;

	scaleSize = (MusicGlobals->MaxNotes*UPSCALAR + MusicGlobals->MaxEffects*UPSCALAR) << 8;
	if (MusicGlobals->MaxNormNotes <= 32)
		scaleSize = scaleSize / (MusicGlobals->MaxNormNotes*UPSCALAR) * MusicGlobals->MasterVolume;
	else
		scaleSize = scaleSize * 100 / (MusicGlobals->MaxNormNotes*UPSCALAR) * MusicGlobals->MasterVolume;

	scaleSize = scaleSize >> 8;

	MusicGlobals->scaleBackAmount = (scaleSize << 8) / (MusicGlobals->MaxNotes*UPSCALAR+MusicGlobals->MaxEffects*UPSCALAR);

}



long PV_DoubleBufferCallbackAndSwap(GM_DoubleBufferCallbackPtr doubleBufferCallback, 
										NoteRecord *this_voice)
{
	INT32	bufferSize;

	bufferSize = this_voice->NotePtrEnd - this_voice->NotePtr;
	// we hit the end of the loop call double buffer to notify swap
	(*doubleBufferCallback)(this_voice->NoteRefNum, this_voice->NotePtr, &bufferSize);
	// now we swap pointers
	if (bufferSize)
	{
		if (this_voice->NotePtr == this_voice->doubleBufferPtr1)
		{
			this_voice->NotePtr = this_voice->doubleBufferPtr2;
			this_voice->NotePtrEnd = this_voice->doubleBufferPtr2 + bufferSize;
		}
		else
		{
			this_voice->NotePtr = this_voice->doubleBufferPtr1;
			this_voice->NotePtrEnd = this_voice->doubleBufferPtr1 + bufferSize;
		}

		this_voice->NoteLoopPtr = this_voice->NotePtr;
		this_voice->NoteLoopEnd = this_voice->NotePtrEnd;
		this_voice->NoteDur = 0x7FFF;			// reset durations otherwise the voice will shut down
		this_voice->NoteDecay = 0x7FFF;
	}
	else
	{
		this_voice->NoteDur = -1;
		PV_DoCallBack(this_voice);
	}
	return bufferSize;
}

FIXED_VALUE PV_GetWavePitch(FIXED_VALUE notePitch)
{
	switch (MusicGlobals->outputQuality)
	{
		case Q_44K:
// If performing at 44 khz then the pitch rates must be 1/8
			return notePitch >>  (17 - STEP_BIT_RANGE);
		case Q_22K:
// if performing at 22 khz then pitches must be 1/4 for the 24.8 fixed values
			return notePitch >> (16 - STEP_BIT_RANGE);
		case Q_11K:
// If performing at 11 khz then the pitch rates must be 1/2
			return notePitch >>  (15 - STEP_BIT_RANGE);
	}
// error,  return same pitch
	return notePitch;
}

// ------------------------------------------------------------------------------------------------------//



// Generic ADSR Unit
void PV_ADSRModule (ADSRRecord *a, INT32 sustaining);
void PV_ADSRModule (ADSRRecord *a, INT32 sustaining)
{
	INT32 currentTime = a->currentTime;
	INT32 index =  a->currentPosition;
	INT32 scalar, i;

// Find the release or LAST marker when the note is being turned off.

	if ((!sustaining) && (a->mode != ADSR_RELEASE) && (a->mode != ADSR_TERMINATE))
	{
		for (i = 0; i < ADSR_STAGES; i++)
		{
			scalar = a->ADSRFlags[i];
			if ((scalar == ADSR_RELEASE) || (scalar == ADSR_TERMINATE))
			{
				index = i;
				a->previousTarget = a->currentLevel;
				goto foundRelease;
			}
			if (scalar == ADSR_SUSTAIN)
			{
				index = i+1;
				a->previousTarget = a->currentLevel;
				goto foundRelease;
			}
			
		}
foundRelease:
		currentTime = 0;
		a->mode = ADSR_RELEASE;
	}
			
	switch (a->ADSRFlags[index])
	{
		case ADSR_SUSTAIN:
			a->mode = ADSR_SUSTAIN;
			if (a->ADSRLevel[index] < 0)
			{
				if (a->ADSRLevel[index] < -50)
					a->sustainingDecayLevel = ( a->sustainingDecayLevel * (expDecayLookup[-a->ADSRLevel[index]/50000] >> 1)  ) >> 15;
				else
					a->sustainingDecayLevel = ( a->sustainingDecayLevel * (expDecayLookup[logLookupTable[-a->ADSRLevel[index]]/50000] >> 1)  ) >> 15;
			}
			else
			{
				if (currentTime)
				{
					currentTime += 11;		// milliseconds;
					if (currentTime >= a->ADSRTime[index])
						currentTime = a->ADSRTime[index];
					if (a->ADSRTime[index])
					{
						scalar = (currentTime << 12) / a->ADSRTime[index];	// scalar is 0..4095
					}
					else
					{
						scalar = 0;
					}
					a->currentLevel = a->previousTarget + (((a->ADSRLevel[index] - a->previousTarget) * scalar) >> 12);
				}
			}
			break;
		default:
			currentTime += 11;		// milliseconds;
			if (currentTime >= a->ADSRTime[index])
			{
				a->previousTarget = a->ADSRLevel[index];
				a->currentLevel = a->ADSRLevel[index];
				currentTime -=  a->ADSRTime[index];
				if (a->ADSRFlags[index] != ADSR_TERMINATE)
					index++;
				else
				{
					a->mode = ADSR_TERMINATE;
					currentTime -= 11;		// prevent long note times from overflowing if they stay on for more than 32.767 seconds
				}
			}
			else
			{
				if (currentTime)
				{
					if (a->ADSRTime[index])
						scalar = (currentTime << 12) / a->ADSRTime[index];	// scalar is 0..4095
					else
						scalar = 4096;
						a->currentLevel = a->previousTarget + (((a->ADSRLevel[index] - a->previousTarget) * scalar) >> 12);	// ADSRLevels max out at 64k
				}
			}
			break;
	}

	// the index may have changed, so check for new cases.
#if 0
	switch (a->ADSRFlags[index])
	{
		case ADSR_GOTO:
			index = a->ADSRLevel[index] - 1;
			break;
	}
#endif
			
	a->currentTime = currentTime;
	a->currentPosition = index & 7;	// protect against runaway indexes
}

INT32 GetWaveShape (INT32 where, INT32 what_kind);
INT32 GetWaveShape (INT32 where, INT32 what_kind)
{
	switch (what_kind)
	{
		case SINE_WAVE:
			if (where > 32768)
				return ((65536- where)* 4) - 65536;
			else
				return (where * 4) - 65536;
		case SAWTOOTH_WAVE:
				return (32768 - where) * 2;
		case SAWTOOTH_WAVE2:
				return (where - 32768) * 2;
		case TRIANGLE_WAVE:
			if (where > 32768)
				return ((65536- where)* 4) - 65536;
			else
				return (where * 4) - 65536;
		case SQUARE_WAVE:
			if (where > 32768)
				return 65536;
			else
				return -65536;
		case SQUARE_WAVE2:
			if (where > 32768)
				return 65536;
			else
				return 0;
		default:
			if (where > 32768)
				return ((65536- where)* 4) - 65536;
			else
				return (where * 4) - 65536;
	};
}

static const short resonantFilterLookup[] = {
	                   42, 40, 37, 35, 33, 63, 59, 56, 53, 
	50, 47, 45, 42, 40, 37, 35, 33, 63, 59, 56, 53, 
	50, 47, 45, 42, 40, 37, 35, 33, 32, 30, 28, 27, 
	50, 47, 45, 42, 40, 37, 35, 33, 32, 30, 28, 27, 
	50, 47, 45, 42, 40, 37, 35, 33, 32, 30, 28, 27, 
	25, 24, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 
	13, 12, 11, 11, 10,  9,    9,  8,   8,    7,   7,   7, 
	  6,   6,   6,   6,   6,  5,    5,  5,   5,    5,   5,   5, 
	  5,   5,   4,   4,   4,  4,    4,   4,  4,    4,   4,   3,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	   };

static void PV_ServeInstrumentCurves(NoteRecord *this_voice)
{
	INT32 i, j, count, tieFromValue, scalar;
	GM_Instrument *inst_struct;
	INT32 curveCount;
//	this_voice->ModWheelValue = 127;

	if (this_voice->ModWheelValue == this_voice->LastModWheelValue)
		return;
	this_voice->LastModWheelValue = this_voice->ModWheelValue;


	inst_struct = this_voice->inst_struct;
	
	if (inst_struct->curveRecordCount == 0)
		return;
	for (i = 0; i < inst_struct->curveRecordCount; i++)
	{
		if (inst_struct->curve[i].tieFrom == QuadChar('M','O','D','W'))
		{
			tieFromValue = this_voice->ModWheelValue;
			curveCount = inst_struct->curve[i].curveCount;		// in case no hits occur
			scalar = tieFromValue;
			for (count = 0; count < curveCount; count++)
			{
				if (inst_struct->curve[i].curveData[count].from_Value <= tieFromValue)
				{
					if (inst_struct->curve[i].curveData[count+1].from_Value >= tieFromValue)
					{
						scalar = inst_struct->curve[i].curveData[count].to_Scalar;
						if (inst_struct->curve[i].curveData[count].from_Value != inst_struct->curve[i].curveData[count+1].from_Value)
						{
							INT32 from_difference = inst_struct->curve[i].curveData[count+1].from_Value - inst_struct->curve[i].curveData[count].from_Value;
							INT32 to_difference = inst_struct->curve[i].curveData[count+1].to_Scalar - inst_struct->curve[i].curveData[count].to_Scalar;
							scalar += ((((tieFromValue - inst_struct->curve[i].curveData[count].from_Value) << 8) / from_difference) * to_difference) >> 8;
						}
					}
				}
			}
			switch (inst_struct->curve[i].tieTo)
			{
				case LOWPASS_AMOUNT:
					this_voice->LPF_base_lowpassAmount = (this_voice->LPF_base_lowpassAmount * scalar) >> 8;
					break;
				case PITCH_LFO:
					for (j = this_voice->LFORecordCount - 1; j >= 0; --j)
						if (this_voice->LFORecords[j].where_to_feed == PITCH_LFO)
						{
							this_voice->LFORecords[j].level = (this_voice->inst_struct->LFORecord[j].level * scalar) >> 8;
							goto done;
						}
					break;
				case VOLUME_LFO:
					for (j = this_voice->LFORecordCount - 1; j >= 0; --j)
						if (this_voice->LFORecords[j].where_to_feed == VOLUME_LFO)
						{
							this_voice->LFORecords[j].level = (this_voice->inst_struct->LFORecord[j].level * scalar) >> 8;
							goto done;
						}
					break;
				case PITCH_LFO_FREQUENCY:
					for (j = this_voice->LFORecordCount - 1; j >= 0; --j)
						if (this_voice->LFORecords[j].where_to_feed == PITCH_LFO)
						{
							this_voice->LFORecords[j].period = (this_voice->inst_struct->LFORecord[j].period * scalar) >> 8;
							goto done;
						}
					break;
				case VOLUME_LFO_FREQUENCY:
					for (j = this_voice->LFORecordCount - 1; j >= 0; --j)
						if (this_voice->LFORecords[j].where_to_feed == VOLUME_LFO)
						{
							this_voice->LFORecords[j].period = (this_voice->inst_struct->LFORecord[j].period * scalar) >> 8;
							goto done;
						}
					break;
			}
			done:;
		}
	}
}

// Process this active voice
static void PV_ServeThisInstrument (NoteRecord *this_voice)
{
	register unsigned long	start, end, loopend, size;
	register long			n, i, value;
	register FIXED_VALUE	wave_increment;
	LFORecordType				*rec;

	PV_ServeInstrumentCurves(this_voice);

	/* Calculate pitch bend before calculating the note's maximum
	** # of samples that it can play this frame.
	*/

// Get the latest pitchbend controller value, which should be munged into an
// 8.8 Fixed value for semitones to bend.
	n = this_voice->PitchBend;

//	n += 12*256;	// if we need to bump up sound pitches for testing of advanced filters
// Process LFO's
	this_voice->volumeLFOValue = 4096;	// default value. Will change below if there's a volume LFO unit present.
	this_voice->stereoPanBend = 0;	// default to no pan modifications.
	this_voice->LPF_resonance = this_voice->LPF_base_resonance;
	this_voice->LPF_lowpassAmount = this_voice->LPF_base_lowpassAmount;
	if (this_voice->LPF_base_frequency <= 0)			// if resonant frequency tied to note pitch, zero out frequency
		this_voice->LPF_frequency = 0;
	if (this_voice->LFORecordCount)
		for (i = 0; i < this_voice->LFORecordCount; i++)
		{
			rec = &(this_voice->LFORecords[i]);
			PV_ADSRModule (&(rec->a), (this_voice->NoteDur > 100) || this_voice->sustainMode == SUS_ON_NOTE_ON);
			if ( (rec->level) || (rec->DC_feed) )
			{
				rec->LFOcurrentTime += 11000;
				if (rec->period)
				{
					if (rec->LFOcurrentTime > rec->period)
						rec->LFOcurrentTime -= rec->period;
					// Produce a percentage index into the current LFO period, scaled to 0..65536.
					// this calculation maxes out at about 15 seconds 
					rec->currentWaveValue = (rec->LFOcurrentTime << 7) / (rec->period >> 9);				
					value = (rec->a.currentLevel * rec->level) >> 16;
	//					value = (value * rec->a.sustainingDecayLevel) >> 16;
					value = (value * GetWaveShape(rec->currentWaveValue, rec->waveShape)) >> 16;
				}
				else
					value = 0;
				value += (rec->a.currentLevel * rec->DC_feed) >> 16;
				switch (rec->where_to_feed)
				{
					case PITCH_LFO:
						n += value;
						break;
					case VOLUME_LFO:
						this_voice->volumeLFOValue += value;
						break;
					case STEREO_PAN_LFO:
					case STEREO_PAN_NAME2:
						this_voice->stereoPanBend += value;
						break;
					case LPF_FREQUENCY:
						if (this_voice->LPF_base_frequency <= 0)
							this_voice->LPF_frequency = value;
						else
							this_voice->LPF_frequency = this_voice->LPF_base_frequency - value;
						break;
					case QuadChar('L','P','A','M'):
						this_voice->LPF_lowpassAmount += value;
						break;
					case QuadChar('L','P','R','E'):
						this_voice->LPF_resonance += value;
						break;
					default:
						//DebugStr("\p Invalid LFO Unit Feed-To");
						break;
				}
			} // if rec->level
		} // for ()

	// to test notes with only pitch bend or LFO applied:  
	//if (n == 0) return;
// If pitchbend value has changed, recalculate the pitch data for this instrument
	if (this_voice->LPF_base_frequency <= 0)
		this_voice->LPF_frequency = 
			resonantFilterLookup[(n + ((this_voice->NoteMIDIPitch - this_voice->LPF_base_frequency) << 8) + 0x80) >> 8]
			 * 256 + this_voice->LPF_frequency;

	if (n != this_voice->LastPitchBend)
	{
		this_voice->LastPitchBend = n;
		n += this_voice->ProcessedPitch << 8;		// ProcessedPitch is based on the sample data and MIDI pitch.
	
	// Clip value to within reasonable MIDI pitch range
		if (n < -0x1800)
			n += 0x0c00;
		if (n < -0x1800)
			n += 0x0c00;
		if (n < -0x1800)
			n = -0x1800;
		if (n > 0x08C00)
			n -= 0x0C00;		// 12 (one octave) in 8.8 Fixed form
		if (n > 0x08C00)
			n -= 0x0C00;		// 12 (one octave) in 8.8 Fixed form
		if (n > 0x08C00)
			n -= 0x0C00;		// 12 (one octave) in 8.8 Fixed form
		if (n > 0x08C00)
			n = 0x0C00;		// 12 (one octave) in 8.8 Fixed form
	// Process whole pitch value in semitones
		this_voice->NotePitch = majorPitchTable[(n >> 8) + 24];
	// Process fractional semitone values
		this_voice->NotePitch = XFixedMultiply(this_voice->NotePitch, fractionalPitchTable[(n & 0xFF) >> 1]);
//		DebugStr("\p Here we go again.");
	// factor in sample rate of sample, if enabled
		this_voice->NotePitch = XFixedMultiply(this_voice->noteSamplePitchAdjust, this_voice->NotePitch);

	// Recalculate number of samples in a slice
		this_voice->NoteNextSize = 0;
	}

	if (this_voice->NoteNextSize == 0)
	{
		wave_increment = PV_GetWavePitch(this_voice->NotePitch);
		this_voice->NoteNextSize = 
			(INT16)((wave_increment * (FIXED_VALUE)MusicGlobals->One_Slice) >> STEP_BIT_RANGE) + 3;
	}
	size = this_voice->NoteNextSize;
	start = this_voice->NoteWave >> STEP_BIT_RANGE;
	end = this_voice->NotePtrEnd - this_voice->NotePtr;
	this_voice->NoteNextSize = size;

#if 0
	switch (this_voice->NotePitch >> (STEP_BIT_RANGE+3))
	{
		case 11:		// 4.0 - 4.5
		case 10:		// 3.0 - 3.5
		case 9:		// 4.0 - 4.5
		case 8:		// 3.0 - 3.5
			this_voice->LPF_lowpassAmount = (this_voice->LPF_lowpassAmount >> 2) + 182;
			break;
		case 7:		// 4.0 - 4.5
		case 6:		// 3.0 - 3.5
			this_voice->LPF_lowpassAmount = (this_voice->LPF_lowpassAmount >> 1) + 128;
			break;
		case 5:		// 2.5 - 3.0
		case 4:		// 2.0 - 2.5
			this_voice->LPF_lowpassAmount = ((this_voice->LPF_lowpassAmount * 3) >> 2) + 64;
			break;

		case 3:		// 1.5 - 2.0
		case 2:		// 1.0 - 1.5
		case 1:
		case 0:
			break;
		default:	// greater than 3.5 times original rate
			this_voice->LPF_lowpassAmount = (this_voice->LPF_lowpassAmount >> 2) + 182;
			break;
	}
#endif

// This is sure easier than the LFO modules!
	PV_ADSRModule (&(this_voice->volumeADSRRecord), (this_voice->NoteDur > 100) || this_voice->sustainMode == SUS_ON_NOTE_ON);
	this_voice->NoteVolumeEnvelope = ((this_voice->volumeADSRRecord.currentLevel >> 2) * this_voice->volumeADSRRecord.sustainingDecayLevel) >> 14;
	this_voice->NoteVolumeEnvelopeBeforeLFO = this_voice->NoteVolumeEnvelope;
	if (this_voice->volumeLFOValue >= 0)		// don't handle volume LFO values less than zero.
		this_voice->NoteVolumeEnvelope = (this_voice->NoteVolumeEnvelope * this_voice->volumeLFOValue) >> 12;

	if (this_voice->NoteLoopEnd)
	{
		loopend = (unsigned long)this_voice->NoteLoopEnd - 
				(unsigned long)this_voice->NotePtr;
	}
	else
	{
		loopend = 0;
	}

	// At end of loop, continue loop?
	if ( start > (loopend - size) )
	{
		if (this_voice->NoteLoopCount == 0)
		{
			this_voice->NoteLoopCount++;
			if (this_voice->NoteLoopProc)
			{
				if ((*this_voice->NoteLoopProc)(this_voice->NoteRefNum) == FALSE)
				{
					this_voice->NoteLoopProc = NULL;
					this_voice->NoteLoopPtr = NULL;
					this_voice->NoteLoopEnd = NULL;
					this_voice->NoteDur = 0;
					loopend = 0;
					this_voice->NoteLoopCount = 0;
				}
			}
		}
	}
	else
	{
		this_voice->NoteLoopCount = 0;
	}

	if ((this_voice->volumeADSRRecord.ADSRTime[0] != 0) || (this_voice->volumeADSRRecord.ADSRFlags[0] != ADSR_TERMINATE) || (this_voice->sampleAndHold != 0) )
	{
// New style volume ADSR instruments
		if (this_voice->NoteDur < 100)
		{
			if (loopend == 0)
				goto ENDING;		// this case handles sample-and-release or one-shot instruments
			if ((this_voice->volumeADSRRecord.mode == ADSR_TERMINATE) || (this_voice->volumeADSRRecord.mode == ADSR_RELEASE))
				if (this_voice->sampleAndHold == 0)
					goto ENDING;
			goto LOOPING;		// this case handles sample-and-hold instruments
		}
		else
		{
			if (loopend == 0)
				goto ENDING;
			else
				goto LOOPING;
		}
	}
	else
	{
// Old style instrument decay
		if ((this_voice->NoteDur == 0) || (loopend == 0))
		{
			if ((this_voice->NoteDur == 0) && (this_voice->NoteDecay == 0))
			{
				// Interpolation of volume ADSR levels will smooth out the transition to note termination.
				this_voice->NoteVolumeEnvelope = 0;
				this_voice->NoteVolumeEnvelopeBeforeLFO = 0;
				goto PARTIAL;
			}
ENDING:
			if ( (end - start) <= size)
			{
PARTIAL:
				if (((this_voice->LPF_lowpassAmount != 0) || (this_voice->LPF_resonance != 0)) && (this_voice->channels == 1))
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->filterPartialBufferProc16(this_voice, FALSE);
					else
						MusicGlobals->filterPartialBufferProc(this_voice, FALSE);
				}
				else
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->partialBufferProc16(this_voice, FALSE);
					else
						MusicGlobals->partialBufferProc(this_voice, FALSE);
				}
			}
			else
			{
				if (((this_voice->LPF_lowpassAmount != 0) || (this_voice->LPF_resonance != 0)) && (this_voice->channels == 1))
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->filterFullBufferProc16(this_voice);
					else
						MusicGlobals->filterFullBufferProc(this_voice);
				}
				else
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->fullBufferProc16(this_voice);
					else
						MusicGlobals->fullBufferProc(this_voice);
				}
			}
		}
		else
		{
LOOPING:
			if (loopend > (start + size) )
			{
				if (((this_voice->LPF_lowpassAmount != 0) || (this_voice->LPF_resonance != 0)) && (this_voice->channels == 1))
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->filterFullBufferProc16(this_voice);
					else
						MusicGlobals->filterFullBufferProc(this_voice);
				}
				else
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->fullBufferProc16(this_voice);
					else
						MusicGlobals->fullBufferProc(this_voice);
				}
			}
			else
			{
				if (((this_voice->LPF_lowpassAmount != 0) || (this_voice->LPF_resonance != 0)) && (this_voice->channels == 1))
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->filterPartialBufferProc16(this_voice, TRUE);
					else
						MusicGlobals->filterPartialBufferProc(this_voice, TRUE);
				}
				else
				{
					if (this_voice->bitSize == 16)
						MusicGlobals->partialBufferProc16(this_voice, TRUE);
					else
						MusicGlobals->partialBufferProc(this_voice, TRUE);
				}
			}
		}
	}

DONE:
	if (this_voice->NoteDur == 0)
	{
		if ((this_voice->volumeADSRRecord.ADSRTime[0] != 0) || (this_voice->volumeADSRRecord.ADSRFlags[0] != ADSR_TERMINATE))
		{
// Handle new style volume ADSR's
			if (this_voice->volumeADSRRecord.mode == ADSR_TERMINATE)
			{
				if ((this_voice->volumeADSRRecord.currentLevel < 0x100) || (this_voice->volumeADSRRecord.sustainingDecayLevel < 0x100))
				{
					PV_DoCallBack(this_voice);
					this_voice->NoteDur = -1;
				}
			}
			else
			{
				if (this_voice->volumeADSRRecord.sustainingDecayLevel == 0)
				{
					PV_DoCallBack(this_voice);
					this_voice->NoteDur = -1;
				}
				// If low in volume, fade it out gracefully next cycle
				if (this_voice->volumeADSRRecord.sustainingDecayLevel < 0x800)
					this_voice->volumeADSRRecord.sustainingDecayLevel = 0;
			}
		}
		else
		{
// old style scheme with NoteDecay values and so forth
			if (this_voice->NoteDecay == 0)
			{
				PV_DoCallBack(this_voice);
				this_voice->NoteDur = -1;
			}
			else
				this_voice->NoteDecay--;
		}
	}
	else
	{
		this_voice->NoteDur--;
	}
}

// Process active sample voices in stereo
INLINE static void PV_ServeStereoInstruments(void)
{
	register MusicVars	*myMusicGlobals;
	register INT32 FAR	*destL;
	register INT32 FAR	*destR;
	register LOOPCOUNT	count;
	register NoteRecord *this_voice;
	BOOL_FLAG		someSoundActive;

	myMusicGlobals = MusicGlobals;

	if (myMusicGlobals->MaxNotes + myMusicGlobals->MaxEffects > SYMPHONY_SIZE - 1)
		myMusicGlobals->MaxNotes = SYMPHONY_SIZE - 1 - myMusicGlobals->MaxEffects;
	destL = &myMusicGlobals->songBufferLeftMono[0];
	destR = &myMusicGlobals->songBufferRightStereo[0];

	for (count = 0; count < myMusicGlobals->Four_Loop; count++)
	{
		destL[0] = 0;
		destL[1] = 0;
		destL[2] = 0;
		destL[3] = 0;
		destL += 4;
		
		destR[0] = 0;
		destR[1] = 0;
		destR[2] = 0;
		destR[3] = 0;
		destR += 4;
	}

#if 1
	// Process active voices
	someSoundActive = FALSE;
	for (count = 0; count < SYMPHONY_SIZE; count++)
	{
		this_voice = &myMusicGlobals->NoteEntry[count];
		if (this_voice->NoteDur >= 0)
		{
			if (this_voice->avoidReverb == FALSE)
			{
				PV_ServeThisInstrument(this_voice);
				someSoundActive = TRUE;
			}
		}
	}
	PV_ProcessReverbStereo();


	for (count = 0; count < SYMPHONY_SIZE; count++)
	{
		this_voice = &myMusicGlobals->NoteEntry[count];
		if (this_voice->NoteDur >= 0)
		{
			if (this_voice->avoidReverb != FALSE)
			{
				PV_ServeThisInstrument(this_voice);
				someSoundActive = TRUE;
			}
		}
	}
#endif
}


void FAR ProcessSampleFrame(void FAR *destinationSamples)
{
// Set up the various procs:
	MusicGlobals->fullBufferProc      = PV_ServeStereoInterp2FullBuffer;
	MusicGlobals->partialBufferProc = PV_ServeStereoInterp2PartialBuffer;
	MusicGlobals->fullBufferProc16      = PV_ServeStereoInterp2FullBuffer16;
	MusicGlobals->partialBufferProc16 = PV_ServeStereoInterp2PartialBuffer16;

#if 1
	MusicGlobals->filterPartialBufferProc = PV_ServeStereoInterp2FilterPartialBuffer;
	MusicGlobals->filterPartialBufferProc16 = PV_ServeStereoInterp2FilterPartialBuffer16;
	MusicGlobals->filterFullBufferProc = PV_ServeStereoInterp2FilterFullBuffer;
	MusicGlobals->filterFullBufferProc16 = PV_ServeStereoInterp2FilterFullBuffer16;
#else
	MusicGlobals->filterFullBufferProc      = PV_ServeStereoInterp2FullBuffer;
	MusicGlobals->filterPartialBufferProc = PV_ServeStereoInterp2PartialBuffer;
	MusicGlobals->filterFullBufferProc16      = PV_ServeStereoInterp2FullBuffer16;
	MusicGlobals->filterPartialBufferProc16 = PV_ServeStereoInterp2PartialBuffer16;
#endif

#if 1
	if (MusicGlobals->systemPaused == FALSE)

	{
		PV_ServeStereoInstruments();
		PV_ProcessSequencerEvents();		// process all songs and external events
		PV_Generate16outputStereo((OUTSAMPLE16 FAR *)destinationSamples);
	}
#endif
}

void ServeMIDINote(INT16 the_instrument, INT16 the_channel, INT16 notePitch, INT32 Volume)
{
	register MusicVars *	myMusicGlobals;
	register UBYTE FAR *	pSample;
	register NoteRecord *	the_entry;
	register GM_Instrument FAR *	theI;
	register GM_Instrument FAR *	inst_struct;
	register KeymapSplit FAR *k;
	register INT32			count;
	INT16				newPitch, playPitch;
//	INT16				highest;
	UINT16				splitCount;
//	UINT16				activeNotes;
	INT16				decay;
	UINT32				loopstart, loopend;
	BOOL_FLAG			preKill, enableReverb;
	INT32				bestSlot, bestLevel;
	register INT32			i, j;
	register GM_Song		*pSong;
	INT32				volume32;
	INT32				sampleNumber;

	myMusicGlobals = MusicGlobals;
	pSong = myMusicGlobals->theSongPlaying;

// scale with default velocity curve
	Volume = defaultVolumeScale[127 - (Volume & 0x7F)];
	volume32 = Volume;

	/* Part of our allocation strategy is to determine the "normal" voice load,
	** and behave differently (replace existing notes preferentially) if we're
	** over the "normal" load.  Here, we count up the number of active voices.
	*/
#if 0
	activeNotes = 0;
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		if (myMusicGlobals->NoteEntry[count].NoteDur >= 0)
			activeNotes++;
	}
#endif

	inst_struct = NULL;
	theI = myMusicGlobals->InstrumentData[myMusicGlobals->remapArray[the_instrument]];
	if (theI)
	{
		playPitch = notePitch;
		if (theI->masterRootKey)
		{
			playPitch = notePitch - theI->masterRootKey + 60;
		}
		sampleNumber = 0;
		// keysplit?
		if (theI->doKeymapSplit)
		{	// yes, find an instrument
			splitCount = theI->u.k.KeymapSplitCount;
			k = theI->u.k.keySplits;
			for (count = 0; count < splitCount; count++)
			{
				if ( (playPitch >= k->lowMidi) && (playPitch <= k->highMidi) )
				{
					theI = k->pSplitInstrument;
					if (theI)
					{
						inst_struct = theI;
						if (theI->masterRootKey)
						{
							playPitch = notePitch - theI->masterRootKey + 60;
						}
						break;
					}
				}
				k++;
				sampleNumber++;
			}
		}
		else
		{
			inst_struct = theI;
		}
	}

	if (inst_struct == NULL)
	{
		return;
	}
	// ok, got a valid instrument, start it
	decay = inst_struct->u.w.noteDecayPref;
	if ( (decay > 250) || (decay < 0) )
	{
		DEBUG_STR("\pServeMIDINote:decay out of range!");
		decay = 1;
	}
	loopstart = inst_struct->u.w.startLoop;
	loopend = inst_struct->u.w.endLoop;
	if ( (inst_struct->disableSndLooping) || 
		(loopstart == loopend) || 
		(loopstart > loopend) || 
		(loopend > inst_struct->u.w.waveFrames) ||
		(loopend - loopstart < MIN_LOOP_SIZE) )
	{
		loopstart = 0;
		loopend = 0;
	}

// If the count of active notes exceeds the normal limits
// for voices, then replace ACTIVE slots in their decay cycle first (to
// reduce the voice load on the CPU.) If count's within normal limits, then
// use EMPTY slots first to improve sound quality of the other notes by allowing
// them to decay more completely before being killed.

	// Find a place for the new note
	the_entry = NULL;
	preKill = FALSE;		// this is for the sustain pedal


// These calculations are needed anyway, and also are used in the following prioritization code.
	volume32 = (Volume * MusicGlobals->scaleBackAmount) >> 8;
	volume32 = PV_ScaleVolumeFromChannelAndSong(the_channel, volume32);


// Find an active slot (best), or
// Terminate notes in one-shot decay (less desirable)
// or notes naturally fading out (preferable)
	bestSlot = -1; bestLevel = 0x10000;
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		if (myMusicGlobals->NoteEntry[count].NoteDur < 0)
		{
			the_entry = &myMusicGlobals->NoteEntry[count];
			goto EnterNote;
		}

#if 1
		if (myMusicGlobals->NoteEntry[count].NoteDur <= 0)
		{
			if (bestLevel > 0x2000)
			{
				bestLevel = 0x2000;
				the_entry = &myMusicGlobals->NoteEntry[count];
			}
		}
		else
		{
			if (myMusicGlobals->NoteEntry[count].NoteProgram == the_instrument)
			{
				if (myMusicGlobals->NoteEntry[count].NoteChannel == the_channel)
				{
					if (myMusicGlobals->NoteEntry[count].volumeADSRRecord.sustainingDecayLevel < bestLevel)
					{
						bestLevel = myMusicGlobals->NoteEntry[count].volumeADSRRecord.sustainingDecayLevel;
						the_entry = &myMusicGlobals->NoteEntry[count];
					}
				}
			}
		}
#endif
	}

	if (bestLevel <= 0x2000)
		goto EnterNote;

#if 1
// Now kill notes that are much lower in volume than the current note (less than 25% of the volume)
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		if (myMusicGlobals->NoteEntry[count].NoteVolume < bestLevel)
		{
			bestLevel = myMusicGlobals->NoteEntry[count].NoteVolume;
			the_entry = &myMusicGlobals->NoteEntry[count];
		}
	}
	if ((bestLevel * 8) <  volume32)
		goto EnterNote;

	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		if ( bestLevel > ((myMusicGlobals->NoteEntry[count].NoteVolume * 
			myMusicGlobals->NoteEntry[count].NoteVolumeEnvelopeBeforeLFO) >> VOLUME_PRECISION_SCALAR))
		{
			bestLevel = (myMusicGlobals->NoteEntry[count].NoteVolume * 
				myMusicGlobals->NoteEntry[count].NoteVolumeEnvelopeBeforeLFO) >> VOLUME_PRECISION_SCALAR;
			the_entry = &myMusicGlobals->NoteEntry[count];
		}
	}

	if ((bestLevel * 8) <  volume32)
		goto EnterNote;

#endif

#if 1
// Now kill notes that are in sustain pedal mode, are in same channel or instrument
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		if (myMusicGlobals->NoteEntry[count].sustainMode != SUS_NORMAL)
		{
			if (myMusicGlobals->NoteEntry[count].NoteProgram == the_instrument)
			{
				if (myMusicGlobals->NoteEntry[count].NoteChannel == the_channel)
				{
					the_entry = &myMusicGlobals->NoteEntry[count];
					goto EnterNote;
				}
			}
		}
	}
#endif

	return;
EnterNote:
// gently terminate (euthanize) any notes of same pitch, instrument number, and channel:
#if 1
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
		if (myMusicGlobals->NoteEntry[count].NoteDur >= 0)
			if (myMusicGlobals->NoteEntry[count].NoteProgram == the_instrument)
				if (myMusicGlobals->NoteEntry[count].NoteMIDIPitch == notePitch)
					if (myMusicGlobals->NoteEntry[count].NoteChannel == the_channel)
					{
						myMusicGlobals->NoteEntry[count].volumeADSRRecord.mode = ADSR_TERMINATE;
						myMusicGlobals->NoteEntry[count].volumeADSRRecord.currentPosition = 0;
						myMusicGlobals->NoteEntry[count].volumeADSRRecord.ADSRLevel[0] = 0;
						myMusicGlobals->NoteEntry[count].volumeADSRRecord.ADSRTime[0] = 1;
						myMusicGlobals->NoteEntry[count].volumeADSRRecord.ADSRFlags[0] = ADSR_TERMINATE;
						myMusicGlobals->NoteEntry[count].NoteVolumeEnvelopeBeforeLFO = 0;		// so these notes can be reused
					}
#endif

	if (the_entry)
	{
		the_entry->NoteDur = -1;
		PV_CleanNoteEntry(the_entry);
		the_entry->inst_struct = inst_struct;
		the_entry->NoteVolumeEnvelopeBeforeLFO = VOLUME_PRECISION_SCALAR;
		pSample = inst_struct->u.w.theWaveform;
		the_entry->NotePtr = pSample;
		the_entry->NotePtrEnd = pSample + inst_struct->u.w.waveFrames;
		the_entry->NoteChannel = the_channel;

		// copy the volume ADSR record into the NoteRecord
		the_entry->volumeADSRRecord = inst_struct->volumeADSRRecord;
		
		// copy the sample-and-hold flag
		the_entry->sampleAndHold = inst_struct->sampleAndHold;

		// Copy the LFO record count
		the_entry->LFORecordCount = inst_struct->LFORecordCount;

		// If there are any LFO records, copy them into the NoteRecord.
		if (the_entry->LFORecordCount)
		{
			for (i = 0; i < the_entry->LFORecordCount; i++)
				the_entry->LFORecords[i] = inst_struct->LFORecord[i];
		}

		// if the instrument defines reverb on or the channel has reverb on, then enable it.
		// if the channel is off, but the instrument defines reverb then enable it
		if (pSong->channelReverb[the_channel] > REVERB_CONTROLER_THRESHOLD)
		{
			enableReverb = TRUE;
		}
		else
		{
			if (inst_struct->avoidReverb)	// instrument defines reverb?
			{
				enableReverb = FALSE;
			}
			else
			{
				enableReverb = TRUE;
			}
		}
		the_entry->avoidReverb = (enableReverb) ? FALSE : TRUE;

// Setup playback pitch
		if (inst_struct->playAtSampledFreq == FALSE)
		{
			newPitch = playPitch + 60 - inst_struct->u.w.baseMidiPitch;
			while (newPitch < -24) { newPitch += 12; }
			while (newPitch > 144) { newPitch -= 12; }
			the_entry->ProcessedPitch = newPitch;
			the_entry->NotePitch = majorPitchTable[newPitch+24];

		}
		else
		{
			the_entry->ProcessedPitch = 0;
			the_entry->NotePitch = 0x10000;		// 1.0 step rate
		}

		// factor in sample rate of sample, if enabled
		if (inst_struct->useSampleRate)
		{
			//DebugStr("\pHow good was my version?");
			the_entry->noteSamplePitchAdjust = XFixedDivide(inst_struct->u.w.sampledRate >> 2, 22050L << 14);
			the_entry->NotePitch = XFixedMultiply (the_entry->NotePitch, the_entry->noteSamplePitchAdjust);
		}
		else
			the_entry->noteSamplePitchAdjust = 0x10000;

		if (loopstart)
//		if (loopstart + loopend)
		{
			the_entry->NoteLoopPtr = the_entry->NotePtr + FP_OFF(loopstart);
			the_entry->NoteLoopEnd = the_entry->NotePtr + FP_OFF(loopend);
		}
		else
		{
			the_entry->NoteLoopPtr = 0;
			the_entry->NoteLoopEnd = 0;
		}
		the_entry->NoteDecay = decay;		// release rate of note
		the_entry->NoteNextSize = 0;
		the_entry->NoteWave = 0;

		the_entry->NoteProgram = the_instrument;
		the_entry->NoteMIDIPitch = notePitch;	// save note pitch unprocessed
		the_entry->NoteMIDIVolume = Volume;	// save note volume unscaled

		the_entry->NoteVolume = volume32;

// Resonant low-pass filter stuff
		the_entry->LPF_base_frequency = inst_struct->LPF_frequency;
		the_entry->LPF_base_resonance = inst_struct->LPF_resonance;
		the_entry->LPF_base_lowpassAmount = inst_struct->LPF_lowpassAmount;

		the_entry->LPF_frequency = inst_struct->LPF_frequency;
		the_entry->LPF_resonance = inst_struct->LPF_resonance;
		the_entry->LPF_lowpassAmount = inst_struct->LPF_lowpassAmount;

		the_entry->NoteEndCallback = NULL;
		the_entry->NoteLoopProc = NULL;
		the_entry->NoteRefNum = 0L;
		the_entry->PitchBend = pSong->channelBend[the_channel];
		the_entry->LastPitchBend = 0;
		the_entry->ModWheelValue = pSong->channelModWheel[the_channel];
		the_entry->LastModWheelValue = 0;
		the_entry->NoteLoopCount = 0;

		// set the inital pan placement. If zero, then just use the current channel placement
		if (inst_struct->panPlacement)
		{
			the_entry->stereoPosition = inst_struct->panPlacement;
		}
		else
		{
			the_entry->stereoPosition = pSong->channelStereoPosition[the_channel];
		}
		the_entry->bitSize = inst_struct->u.w.bitSize;
		the_entry->channels = inst_struct->u.w.channels;

		if ( (preKill == FALSE) && (pSong->channelSustain[the_channel]) )
		{
			the_entry->sustainMode = SUS_ON_NOTE_ON;
		}
		else
		{
			the_entry->sustainMode = SUS_NORMAL;
		}

		if (inst_struct->curveRecordCount)
		{
			for (i = 0; i < inst_struct->curveRecordCount; i++)
			{
				INT32 scalar;
				INT32 tieFromValue = 0;
				INT32 curveCount = inst_struct->curve[i].curveCount;
				switch (inst_struct->curve[i].tieFrom)
				{
					case QuadChar('P','I','T','C'):
						//DebugStr("\p tying to pitch");
						tieFromValue = notePitch;
						break;
					case QuadChar('V','O','L','U'):
						tieFromValue = Volume;
						break;
					case QuadChar('S','A','M','P'):
						tieFromValue = sampleNumber;
						break;
					case QuadChar('M','O','D','W'):
						tieFromValue = pSong->channelModWheel[the_channel];
						break;
					default:
						//DebugStr("\p invalid tie-from value");
						break;
				}
				scalar = tieFromValue;
				for (count = 0; count < curveCount; count++)
				{
					if (inst_struct->curve[i].curveData[count].from_Value <= tieFromValue)
					{
						if (inst_struct->curve[i].curveData[count+1].from_Value >= tieFromValue)
						{
							scalar = inst_struct->curve[i].curveData[count].to_Scalar;
							if (inst_struct->curve[i].curveData[count].from_Value != inst_struct->curve[i].curveData[count+1].from_Value)
							{
								INT32 from_difference = inst_struct->curve[i].curveData[count+1].from_Value - inst_struct->curve[i].curveData[count].from_Value;
								INT32 to_difference = inst_struct->curve[i].curveData[count+1].to_Scalar - inst_struct->curve[i].curveData[count].to_Scalar;
								scalar += ((((tieFromValue - inst_struct->curve[i].curveData[count].from_Value) << 8) / from_difference) * to_difference) >> 8;
							}
						}
					}
				}
				switch (inst_struct->curve[i].tieTo)
				{
					case NOTE_VOLUME:
//						DebugStr("\p scaling note velocity");
						the_entry->NoteVolume = (the_entry->NoteVolume * scalar) >> 8;
						break;
					case SUSTAIN_RELEASE_TIME:
						for (j = 0; j < 8; j++)
						{
							if (the_entry->volumeADSRRecord.ADSRFlags[j] == ADSR_SUSTAIN)
							{
								if (the_entry->volumeADSRRecord.ADSRLevel[j] < 0)
								{
									scalar = scalar >> 2;
									if (the_entry->volumeADSRRecord.ADSRLevel[j]  < -50)
										the_entry->volumeADSRRecord.ADSRLevel[j] = 
											-(-(the_entry->volumeADSRRecord.ADSRLevel[j] * scalar) >> 6);
									else
										the_entry->volumeADSRRecord.ADSRLevel[j] = 
											-((logLookupTable[-the_entry->volumeADSRRecord.ADSRLevel[j]] * scalar) >> 6);
									j = 8;
								}
							}
						}
						break;
					case SUSTAIN_LEVEL:
						for (j = 1; j < 8; j++)
						{
							if (the_entry->volumeADSRRecord.ADSRFlags[j] == ADSR_SUSTAIN)
							{
								if (the_entry->volumeADSRRecord.ADSRLevel[j] > 0)
										the_entry->volumeADSRRecord.ADSRLevel[j] = 
											(the_entry->volumeADSRRecord.ADSRLevel[j] * scalar) >> 8;
								else
										the_entry->volumeADSRRecord.ADSRLevel[j-1] = 
											(the_entry->volumeADSRRecord.ADSRLevel[j-1] * scalar) >> 8;
								j = 8;
							}
						}
						break;
					case RELEASE_TIME:
						for (j = 1; j < 8; j++)
						{
							if (the_entry->volumeADSRRecord.ADSRFlags[j] == ADSR_TERMINATE)
							{
								if (the_entry->volumeADSRRecord.ADSRTime[j] > 0)
										the_entry->volumeADSRRecord.ADSRTime[j] = 
											(the_entry->volumeADSRRecord.ADSRTime[j] * scalar) >> 8;
								else
										the_entry->volumeADSRRecord.ADSRTime[j-1] = 
											(the_entry->volumeADSRRecord.ADSRTime[j-1] * scalar) >> 8;
								j = 8;
							}
						}
						break;
					case VOLUME_ATTACK_TIME:
						if (the_entry->volumeADSRRecord.ADSRTime[0] != 0)
							the_entry->volumeADSRRecord.ADSRTime[0] = (the_entry->volumeADSRRecord.ADSRTime[0] * scalar) >> 8;
						else
							the_entry->volumeADSRRecord.ADSRTime[1] = (the_entry->volumeADSRRecord.ADSRTime[1] * scalar) >> 8;
						break;
					case VOLUME_ATTACK_LEVEL:
						if (1)	//(*((long *) 0x17a))
						{
							if (the_entry->volumeADSRRecord.ADSRLevel[0] >  the_entry->volumeADSRRecord.ADSRLevel[1])
								the_entry->volumeADSRRecord.ADSRLevel[0] = (the_entry->volumeADSRRecord.ADSRLevel[0] * scalar) >> 8;
							else
								the_entry->volumeADSRRecord.ADSRLevel[1] = (the_entry->volumeADSRRecord.ADSRLevel[1] * scalar) >> 8;
						}
						break;
					case WAVEFORM_OFFSET:
						the_entry->NoteWave = scalar << STEP_BIT_RANGE;
						break;
					case LOWPASS_AMOUNT:
						the_entry->LPF_base_lowpassAmount = (the_entry->LPF_base_lowpassAmount * scalar) >> 8;
						break;
					case PITCH_LFO:
						for (j = the_entry->LFORecordCount - 1; j >= 0; --j)
							if (the_entry->LFORecords[j].where_to_feed == PITCH_LFO)
							{
								the_entry->LFORecords[j].level = (the_entry->LFORecords[j].level * scalar) >> 8;
								goto exit;
							}
						break;
					case VOLUME_LFO:
						for (j = the_entry->LFORecordCount - 1; j >= 0; --j)
							if (the_entry->LFORecords[j].where_to_feed == VOLUME_LFO)
							{
								the_entry->LFORecords[j].level = (the_entry->LFORecords[j].level * scalar) >> 8;
								goto exit;
							}
						break;
					case PITCH_LFO_FREQUENCY:
						for (j = 0; j < the_entry->LFORecordCount; j++)
							if (the_entry->LFORecords[j].where_to_feed == PITCH_LFO)
							{
								the_entry->LFORecords[j].period = (the_entry->LFORecords[j].period * scalar) >> 8;
								goto exit;
							}
						break;
					case VOLUME_LFO_FREQUENCY:
						for (j = 0; j < the_entry->LFORecordCount; j++)
							if (the_entry->LFORecords[j].where_to_feed == VOLUME_LFO)
							{
								the_entry->LFORecords[j].period = (the_entry->LFORecords[j].period * scalar) >> 8;
								goto exit;
							}
						break;
				}
			exit:;
			}
		}


// is there an initial volume level in the ADSR record that starts at time=0?  If so, don't interpolate the
// note's volume up from 0 to the first target level.  Otherwise, it's a traditional ramp-up from 0.
		if (the_entry->volumeADSRRecord.ADSRTime[0] == 0)
		{
			the_entry->volumeADSRRecord.currentLevel = the_entry->volumeADSRRecord.ADSRLevel[0];
			if (MusicGlobals->generateStereoOutput)
			{
				INT32 aLeft, aRight;
				PV_CalculateStereoVolume(the_entry, &aLeft, &aRight);
				the_entry->lastAmplitudeL = aLeft;
				the_entry->lastAmplitudeR = aRight;
			}
			else
				the_entry->lastAmplitudeL = (the_entry->NoteVolume * the_entry->volumeADSRRecord.ADSRLevel[0]) >> VOLUME_PRECISION_SCALAR;
		}
		else
		{
				the_entry->lastAmplitudeL = 0;
				the_entry->lastAmplitudeR = 0;
		}

// This step is performed last.
		the_entry->NoteDur = 32000;
	}
}

void StopMIDINote(INT16 the_instrument, INT16 the_channel, INT16 notePitch)
{
	register LOOPCOUNT		count;
	register MusicVars		*myMusicGlobals;
	register INT16			decay;
	register GM_Song		*pSong;

	myMusicGlobals = MusicGlobals;
	pSong = myMusicGlobals->theSongPlaying;

	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		if (myMusicGlobals->NoteEntry[count].NoteDur >= 0)
		{
			if (myMusicGlobals->NoteEntry[count].NoteChannel == the_channel)
			{
				if (myMusicGlobals->NoteEntry[count].NoteMIDIPitch == notePitch)
				{
					// if pedal is on, put note into SUS_ON_NOTE_OFF, and don't kill note
					if (pSong->channelSustain[the_channel])
					{
						myMusicGlobals->NoteEntry[count].sustainMode = SUS_ON_NOTE_OFF;
					}
					else
					{
						myMusicGlobals->NoteEntry[count].NoteDur = 0;	// put into release mode, decay
// debugging
						if (the_instrument != myMusicGlobals->NoteEntry[count].NoteProgram)
						{
							DEBUG_STR("\pProgram number changed before note finished playing!  Bad!");
						}
						myMusicGlobals->NoteEntry[count].NoteDur = 0;	// put into release mode, decay
						decay = myMusicGlobals->NoteEntry[count].NoteDecay;
						if ( (decay > 500) || (decay < 0) )
						{
							DEBUG_STR("\pStopMIDINote:decay out of range!");
							myMusicGlobals->NoteEntry[count].NoteDecay = 1;
						}
					}
				}
			}
		}
	}
}


// Stop just midi notes
void GM_EndAllNotes(void)
{
	register LOOPCOUNT count;

// SYMPHONY_SIZE
	for (count = 0; count < MusicGlobals->MaxNotes; count++)
	{
		MusicGlobals->NoteEntry[count].NoteDur = -1;
	}
}

// Used to get the current frame of audio data that has been built. Useful for fun displays. Returns 16 bit information
// only. If generating 8 bit, then data output is converted. If mono data then right channel will be dead.
// This code is deliberately less efficient than the real output scaling code, for space conservation purposes.

INT16 GM_GetAudioSampleFrame(INT16 *pLeft, INT16 *pRight)
{
	register LOOPCOUNT		size, count;
	register INT32			*sourceL, *sourceR;
	register INT32 i, k8000;

	if (MusicGlobals)
	{
		k8000 = 0x8000;
		sourceL = &MusicGlobals->songBufferLeftMono[0];
		sourceR = &MusicGlobals->songBufferRightStereo[0];
		size = MusicGlobals->One_Loop;

		for (count = 0; count < MusicGlobals->Four_Loop; count++)
		{
			i = (*sourceL++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pLeft++ = i - k8000;
			i = (*sourceR++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pRight++ = i - k8000;

			i = (*sourceL++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pLeft++ = i - k8000;
			i = (*sourceR++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pRight++ = i - k8000;

			i = (*sourceL++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pLeft++ = i - k8000;
			i = (*sourceR++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pRight++ = i - k8000;

			i = (*sourceL++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pLeft++ = i - k8000;
			i = (*sourceR++ >> OUTPUT_SCALAR);
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFE; else i = 0;}
			*pRight++ = i - k8000;
		}
	}
	return size;
}

/* EOF of GenSynth.c
*/

