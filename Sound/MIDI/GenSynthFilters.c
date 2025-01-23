/*
********************************************************************************************
**
** ╥GenSynthFilters.c╙
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


#define GET_FILTER_PARAMS \
	CLIP (this_voice->LPF_frequency, 0x200, MAXRESONANCE*256);	\
	if (this_voice->previous_zFrequency == 0)\
		this_voice->previous_zFrequency = this_voice->LPF_frequency;\
	CLIP (this_voice->LPF_resonance, 0, 0x100);\
	CLIP (this_voice->LPF_lowpassAmount, -0xFF, 0xFF);\
	Z1 = this_voice->LPF_lowpassAmount << 8;\
	if (Z1 < 0)\
		Xn = 65536 + Z1;\
	else\
		Xn = 65536 - Z1;\
	if (Z1 >= 0)\
	{\
		Zn = ((0x10000 - Z1) * this_voice->LPF_resonance) >> 8;\
		Zn = -Zn;\
	}\
	else\
		Zn = 0;




void PV_ServeStereoInterp2FilterPartialBuffer (NoteRecord *this_voice, int looping)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register UBYTE FAR		*source;
	register UBYTE			b, c;
	register FIXED_VALUE 	cur_wave;
	register FIXED_VALUE 	wave_increment;
	register FIXED_VALUE 	end_wave, wave_adjust;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 inner;

	INT32 amplitudeLincrement, amplitudeRincrement;
	INT32 ampValueL, ampValueR;
	INT32 a;

	register INT16 *z;
	register INT32 Z1value, zIndex1, zIndex2, Xn, Z1, Zn, sample;

	if (this_voice->channels > 1) { PV_ServeStereoInterp2PartialBuffer (this_voice, looping); return; }

	z = this_voice->z;
	Z1value = this_voice->Z1value;
	zIndex2 = this_voice->zIndex;

	GET_FILTER_PARAMS

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);

	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = ((ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop)) >> 2;
	amplitudeRincrement = ((ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop)) >> 2;

	amplitudeL = amplitudeL >> 2;
	amplitudeR = amplitudeR >> 2;

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
//	if (*((long *) 0x17a))
//		DebugStr("\pIn filter case");
	if (this_voice->LPF_resonance == 0)
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = ((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) << 2;
				sample = (sample * Xn + Z1value * Z1) >> 16;
				Z1value = sample - (sample >> 9);
				*destL += sample * amplitudeL;
				*destR += sample * amplitudeR;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	else
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			zIndex1 = zIndex2 - (this_voice->previous_zFrequency >> 8);
			this_voice->previous_zFrequency += (this_voice->LPF_frequency - this_voice->previous_zFrequency) >> 3;
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = ((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) << 2;
				sample = (sample * Xn + Z1value * Z1 + z[zIndex1 & MAXRESONANCE] * Zn) >> 16;
				zIndex1++;
				z[zIndex2 & MAXRESONANCE] = sample;
				zIndex2++;
				Z1value = sample - (sample >> 9);
				*destL += sample * amplitudeL;
				*destR += sample * amplitudeR;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}

	this_voice->Z1value = Z1value;
	this_voice->zIndex = zIndex2;
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
FINISH:
	return;
}




void PV_ServeStereoInterp2FilterFullBuffer (NoteRecord *this_voice)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register UBYTE FAR		*source;
	register UBYTE			b, c;
	register FIXED_VALUE 	cur_wave;
	register FIXED_VALUE 	wave_increment;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 inner;

	INT32 amplitudeLincrement, amplitudeRincrement;
	INT32 ampValueL, ampValueR;
	INT32 a;

	register INT16 *z;
	register INT32 Z1value, zIndex1, zIndex2, Xn, Z1, Zn, sample;

	if (this_voice->channels > 1) { PV_ServeStereoInterp2PartialBuffer (this_voice, FALSE); return; }

	z = this_voice->z;
	Z1value = this_voice->Z1value;
	zIndex2 = this_voice->zIndex;

	GET_FILTER_PARAMS

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);

	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = ((ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop)) >> 2;
	amplitudeRincrement = ((ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop)) >> 2;

	amplitudeL = amplitudeL >> 2;
	amplitudeR = amplitudeR >> 2;

	destL = &MusicGlobals->songBufferLeftMono[0];
	destR = &MusicGlobals->songBufferRightStereo[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (this_voice->LPF_resonance == 0)
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = ((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) << 2;
				sample = (sample * Xn + Z1value * Z1) >> 16;
				Z1value = sample - (sample >> 9);
				*destL += sample * amplitudeL;
				*destR += sample * amplitudeR;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	else
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			zIndex1 = zIndex2 - (this_voice->previous_zFrequency >> 8);
			this_voice->previous_zFrequency += (this_voice->LPF_frequency - this_voice->previous_zFrequency) >> 3;
			for (inner = 0; inner < 16; inner++)
			{
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = ((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) << 2;
				sample = (sample * Xn + Z1value * Z1 + z[zIndex1 & MAXRESONANCE] * Zn) >> 16;
				zIndex1++;
				z[zIndex2 & MAXRESONANCE] = sample;
				zIndex2++;
				Z1value = sample - (sample >> 9);
				*destL += sample * amplitudeL;
				*destR += sample * amplitudeR;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}

	this_voice->Z1value = Z1value;
	this_voice->zIndex = zIndex2;
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
FINISH:
	return;
}

// еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее ееееееееее
// еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее ееееееееее
// еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее ееееееееее
// еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее ееееееееее
// еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее еееееееееееее ееееееееее
// 16 bit cases


void PV_ServeStereoInterp2FilterFullBuffer16 (NoteRecord *this_voice)
{
	PV_ServeStereoInterp2FilterPartialBuffer16 (this_voice, FALSE);
}

void PV_ServeStereoInterp2FilterPartialBuffer16 (NoteRecord *this_voice, int looping)
{
	register INT32 FAR		*destL;
	register INT32 FAR		*destR;
	register INT16 FAR		*source;
	register INT16			b, c;
	register FIXED_VALUE 	cur_wave;
	register FIXED_VALUE 	wave_increment;
	register FIXED_VALUE 	end_wave, wave_adjust;
	register INT32 amplitudeL;
	register INT32 amplitudeR;
	register INT32 inner;

	INT32 amplitudeLincrement, amplitudeRincrement;
	INT32 ampValueL, ampValueR;
	INT32 a;

	register INT16 *z;
	register INT32 Z1value, zIndex1, zIndex2, Xn, Z1, Zn, sample;

	if (this_voice->channels > 1) { PV_ServeStereoInterp2PartialBuffer16 (this_voice, looping); return; }

	z = this_voice->z;
	Z1value = this_voice->Z1value;
	zIndex2 = this_voice->zIndex;

	GET_FILTER_PARAMS

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);

	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = ((ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop));
	amplitudeRincrement = ((ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop));

	destL = &MusicGlobals->songBufferLeftMono[0];
	destR = &MusicGlobals->songBufferRightStereo[0];
	source = (short *) this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (looping)
	{
		end_wave = (FIXED_VALUE) (this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;
	}
	else
		end_wave = (FIXED_VALUE) (this_voice->NotePtrEnd - this_voice->NotePtr - 1) << STEP_BIT_RANGE;
	if (this_voice->LPF_resonance == 0)
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(INT16 *);		// is in the mail
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = ((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) >> 4;
				sample = (sample * Xn + Z1value * Z1) >> 16;
				Z1value = sample - (sample >> 9);
				*destL += (sample * amplitudeL) >> 4;
				*destR += (sample * amplitudeR) >> 4;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	else
		for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
		{
			zIndex1 = zIndex2 - (this_voice->previous_zFrequency >> 8);
			this_voice->previous_zFrequency += (this_voice->LPF_frequency - this_voice->previous_zFrequency) >> 3;
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(INT16 *);		// is in the mail
				b = source[cur_wave>>STEP_BIT_RANGE];
				c = source[(cur_wave>>STEP_BIT_RANGE)+1];
				sample = ((((INT32) (cur_wave & STEP_FULL_RANGE) * (c-b))>>STEP_BIT_RANGE) + b - 0x80) >> 4;
				sample = (sample * Xn + Z1value * Z1 + z[zIndex1 & MAXRESONANCE] * Zn) >> 16;
				zIndex1++;
				z[zIndex2 & MAXRESONANCE] = sample;
				zIndex2++;
				Z1value = sample - (sample >> 9);
				*destL += (sample * amplitudeL) >> 4;
				*destR += (sample * amplitudeR) >> 4;
				destL++;
				destR++;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}

	this_voice->Z1value = Z1value;
	this_voice->zIndex = zIndex2;
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
FINISH:
	return;
}

