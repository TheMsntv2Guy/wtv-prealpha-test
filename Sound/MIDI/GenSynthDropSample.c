// ===========================================================================
//	GenSynthDropSample.c
//
//	Portions copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

/*
********************************************************************************************
**
** “GenSynthDropSample.c”
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**	Confidential-- Internal use only
**
** Modification History:
**
**	1/18/96		Spruced up for C++ extra error checking
**				Changed the macro 'THE_CHECK' to accept a type for typecasting the source pointer
**	3/1/96		Removed extra PV_DoCallBack, and PV_GetWavePitch
********************************************************************************************
*/

#include "GenSnd.h"
#include "GenPriv.h"

#define CLIP(LIMIT_VAR, LIMIT_LOWER, LIMIT_UPPER) if (LIMIT_VAR < LIMIT_LOWER) LIMIT_VAR = LIMIT_LOWER; if (LIMIT_VAR > LIMIT_UPPER) LIMIT_VAR = LIMIT_UPPER;



/*------------------------------------------------------------------------	*/
/*  Cases for Fully amplitude scaled synthesis 					                	*/
/*------------------------------------------------------------------------	*/

void PV_ServeDropSampleFullBuffer (NoteRecord *this_voice)
{
	register INT32 FAR		*dest;
	register LOOPCOUNT		a, inner;
	register UBYTE FAR		*source;
	register FIXED_VALUE	cur_wave, wave_increment;
	register INT32			amplitude, amplitudeAdjust;

	if (this_voice->channels > 1)
	{
		PV_ServeInterp2FullBuffer(this_voice);
		return;
	}

	dest = &MusicGlobals->songBufferLeftMono[0];
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Sixteen_Loop;
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
	{
		for (inner = 0; inner < 4; inner++)
		{
			dest[0] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
			cur_wave += wave_increment;
			dest[1] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
			cur_wave += wave_increment;
			dest[2] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
			cur_wave += wave_increment;
			dest[3] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
			cur_wave += wave_increment;
			dest += 4;
		}
		amplitude += amplitudeAdjust;
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = amplitude;
}

void PV_ServeDropSamplePartialBuffer (NoteRecord *this_voice, int looping)
{
	register INT32 FAR		*dest;
	register LOOPCOUNT		a, inner;
	register UBYTE FAR		*source;
	register FIXED_VALUE	cur_wave, wave_increment, end_wave, wave_adjust;
	register INT32			amplitude, amplitudeAdjust;

	if (this_voice->channels > 1)
	{
		PV_ServeInterp2PartialBuffer(this_voice, looping);
		return;
	}

	dest = &MusicGlobals->songBufferLeftMono[0];
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Sixteen_Loop;
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (looping)
	{
		end_wave = (FIXED_VALUE) (this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;
	}
	else
// Interpolated cases have to stop sooner, hence the - 1 everywhere else.
		end_wave = (FIXED_VALUE) (this_voice->NotePtrEnd - this_voice->NotePtr) << STEP_BIT_RANGE;

	for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
	{
		for (inner = 0; inner < 16; inner++)
		{
			THE_CHECK(UBYTE *);
			*dest += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
			dest++;
			cur_wave += wave_increment;
		}
		amplitude += amplitudeAdjust;
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = amplitude;
FINISH:
	return;
}

// STEREO OUTPUT CODE

/*------------------------------------------------------------------------	*/
/*  Cases for Fully amplitude scaled synthesis 					                	*/
/*------------------------------------------------------------------------	*/

void PV_ServeStereoAmpFullBuffer (NoteRecord *this_voice)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register LOOPCOUNT		a, inner;
	register UBYTE FAR		*source;
	register FIXED_VALUE	cur_wave_shift;
	register FIXED_VALUE	cur_wave, wave_increment;
	INT32				ampValueL, ampValueR;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 amplitudeLincrement, amplitudeRincrement;

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop);

	destL = &MusicGlobals->songBufferLeftMono[0];
	destR = &MusicGlobals->songBufferRightStereo[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (this_voice->channels == 1)
	{	// mono instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 4; inner++)
			{
				*destL += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				*destR += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				destL++;
				destR++;
				cur_wave += wave_increment;
				*destL += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				*destR += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				destL++;
				destR++;
				cur_wave += wave_increment;
				*destL += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				*destR += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				destL++;
				destR++;
				cur_wave += wave_increment;
				*destL += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				*destR += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	else
	{	// stereo 8 bit instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 4; inner++)
			{
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destR++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destR++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destR++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destR++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
}

void PV_ServeStereoAmpPartialBuffer (NoteRecord *this_voice, int looping)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register LOOPCOUNT		a, inner;
	register UBYTE FAR		*source;
	register FIXED_VALUE	cur_wave_shift;
	register FIXED_VALUE	cur_wave, wave_increment, end_wave, wave_adjust;
	INT32				ampValueL, ampValueR;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 amplitudeLincrement, amplitudeRincrement;

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop);


	destL = &MusicGlobals->songBufferLeftMono[0];
	destR = &MusicGlobals->songBufferRightStereo[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);
	if (looping)
	{
		end_wave = (FIXED_VALUE) (this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;
	}
	else
		end_wave = (FIXED_VALUE) (this_voice->NotePtrEnd - this_voice->NotePtr) << STEP_BIT_RANGE;

	if (this_voice->channels == 1)
	{	// mono instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				*destL += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				*destR += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	else
	{	// stereo 8 bit instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL += ((source[cur_wave_shift+0] - 0x80) * amplitudeL) >> 1;
				*destR += ((source[cur_wave_shift+1] - 0x80) * amplitudeR) >> 1;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
FINISH:
	return;
}


