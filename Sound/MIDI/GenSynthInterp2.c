/*
********************************************************************************************
**
** ╥GenSynthFInter2.c╙
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


void PV_ServeStereoInterp2FullBuffer(NoteRecord *this_voice)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register LOOPCOUNT		a, inner;
	register UBYTE FAR		*source, *calculated_source;
	register INT32			b, c;
	register FIXED_VALUE 	cur_wave;
	register INT32			sample;
	register FIXED_VALUE 	wave_increment;
	INT32				ampValueL, ampValueR;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 amplitudeLincrement;
	register INT32 amplitudeRincrement;

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
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80;
				*destL += sample * amplitudeL;
				*destR += sample * amplitudeR;
				cur_wave += wave_increment;
	
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80;
				destL[1] += sample * amplitudeL;
				destR[1] += sample * amplitudeR;
				cur_wave += wave_increment;
	
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80;
				destL[2] += sample * amplitudeL;
				destR[2] += sample * amplitudeR;
				cur_wave += wave_increment;
	
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80;
				destL[3] += sample * amplitudeL;
				destR[3] += sample * amplitudeR;
				destL += 4;
				destR += 4;
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
				calculated_source = source + ((cur_wave>> STEP_BIT_RANGE) * 2);
				b = calculated_source[0];
				c = calculated_source[2];
				*destL += (((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) * amplitudeL);
				b = calculated_source[1];
				c = calculated_source[3];
				*destR += (((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) * amplitudeR);
				destL++;
				destR++;
		
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
	this_voice->NoteWave = cur_wave;
}

void PV_ServeStereoInterp2PartialBuffer (NoteRecord *this_voice, int looping)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register LOOPCOUNT		a, inner;
	register UBYTE FAR		*source, *calculated_source;
	register INT32			b, c, sample;
	register FIXED_VALUE 	cur_wave;
	register FIXED_VALUE 	wave_increment;
	register FIXED_VALUE 	end_wave, wave_adjust;
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
		end_wave = (FIXED_VALUE) (this_voice->NotePtrEnd - this_voice->NotePtr - 1) << STEP_BIT_RANGE;

	if (this_voice->channels == 1)
	{	// mono instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80;
				*destL += sample * amplitudeL;
				*destR += sample * amplitudeR;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	else
	{	// Stereo 8 bit instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				calculated_source = source + ((cur_wave>> STEP_BIT_RANGE) * 2);
				b = calculated_source[0];
				c = calculated_source[2];
				*destL += (((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) * amplitudeL);
				b = calculated_source[1];
				c = calculated_source[3];
				*destR += (((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) * amplitudeR);
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

// ееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
// ееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
// ееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
// ееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
// ееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее

// 16 bit cases

void PV_ServeStereoInterp2FullBuffer16 (NoteRecord *this_voice)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register LOOPCOUNT		a, inner;
	register INT16 FAR		*source, *calculated_source;
	register INT32			b, c;
	register FIXED_VALUE 	cur_wave;
	register INT32			sample;
	register FIXED_VALUE 	wave_increment;
	INT32				ampValueL, ampValueR;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 amplitudeLincrement;
	register INT32 amplitudeRincrement;

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop);

	amplitudeL = amplitudeL >> 4;
	amplitudeR = amplitudeR >> 4;
	amplitudeLincrement = amplitudeLincrement >> 4;
	amplitudeRincrement = amplitudeRincrement >> 4;

	destL = &MusicGlobals->songBufferLeftMono[0];
	destR = &MusicGlobals->songBufferRightStereo[0];
	cur_wave = this_voice->NoteWave;

	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);
	if (this_voice->channels == 1)
	{	// mono instrument

		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 4; inner++)
			{
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				*destL += (sample * amplitudeL) >> 4;
				*destR += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;
	
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				destL[1] += (sample * amplitudeL) >> 4;
				destR[1] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;
	
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				destL[2] += (sample * amplitudeL) >> 4;
				destR[2] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;
	
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				destL[3] += (sample * amplitudeL) >> 4;
				destR[3] += (sample * amplitudeR) >> 4;
				destL += 4;
				destR += 4;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	else
	{	// stereo 16 bit instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				calculated_source = source + ((cur_wave>> STEP_BIT_RANGE) * 2);
				b = calculated_source[0];
				c = calculated_source[2];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				*destL += (sample * amplitudeL) >> 4;
				b = calculated_source[1];
				c = calculated_source[3];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				*destR += (sample * amplitudeR) >> 4;
				destL++;
				destR++;
		
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
	this_voice->NoteWave = cur_wave;
}

void PV_ServeStereoInterp2PartialBuffer16 (NoteRecord *this_voice, int looping)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register LOOPCOUNT		a, inner;
	register INT16 FAR		*source, *calculated_source;
	register INT32			b, c, sample;
	register FIXED_VALUE 	cur_wave;
	register FIXED_VALUE 	wave_increment;
	register FIXED_VALUE 	end_wave, wave_adjust;
	INT32				ampValueL, ampValueR;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 amplitudeLincrement, amplitudeRincrement;

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop);

	amplitudeL = amplitudeL >> 4;
	amplitudeR = amplitudeR >> 4;
	amplitudeLincrement = amplitudeLincrement >> 4;
	amplitudeRincrement = amplitudeRincrement >> 4;

	destL = &MusicGlobals->songBufferLeftMono[0];
	destR = &MusicGlobals->songBufferRightStereo[0];
	cur_wave = this_voice->NoteWave;
	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (looping)
	{
		end_wave = (FIXED_VALUE) (this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;
	}
	else
	{
		end_wave = (FIXED_VALUE) (this_voice->NotePtrEnd - this_voice->NotePtr - 1) << STEP_BIT_RANGE;
	}

	if (this_voice->channels == 1)
	{	// mono instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(INT16 *);
				calculated_source = source + (cur_wave>> STEP_BIT_RANGE);
				b = *calculated_source;
				c = calculated_source[1];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				*destL += (sample * amplitudeL) >> 4;
				*destR += (sample * amplitudeR) >> 4;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	else
	{	// Stereo 16 bit instrument
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(INT16 *);
				calculated_source = source + ((cur_wave>> STEP_BIT_RANGE) * 2);
				b = calculated_source[0];
				c = calculated_source[2];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				*destL += (sample * amplitudeL) >> 4;
				b = calculated_source[1];
				c = calculated_source[3];
				sample = (((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b;
				*destR += (sample * amplitudeR) >> 4;
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

