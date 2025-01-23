/*
********************************************************************************************
**
** “GenSetup.c”
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
**	1/10/96		Split from GenSynth.c
**	1/12/96		Changed GM_ChangeSystemVoices to support 32 voices instead of 31
**	1/18/96		Spruced up for C++ extra error checking
**				Fixed bug with GM_FinisGeneralSound referencing MusicVars before its allocated
**	1/19/96		Changed GM_BeginSample to support bitsize and channels
**	2/5/96		Working towards multiple songs
**	2/12/96		Moved cleaning of external midi queue to Init code
**	3/6/96		Eliminated the global songVolume
**	4/10/96		Changed min loop size from 100 to use define 'MIN_LOOP_SIZE'
**				Eliminated the 'can't use 0 as loop start' in GM_BeginSample
**
**
********************************************************************************************
*/
#include "GenSnd.h"
#include "GenPriv.h"
#include "Headers.h"

void SwapWord(UINT16 FAR *w)
{
#if 0
	UINT16	a,b;

	a = *w & 0xFF;
	b = *w & 0xFF00;
	*w = (a << 8) + (b >> 8);
#else
	*w = *w;
#endif
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void SwapLong(UINT32 FAR *ll)
{
#if 0
	UINT32				newValue;
	UINT32				oldValue;
	register unsigned char *	pOld;
	register unsigned char *	pNew;
	register unsigned char 	temp;

	oldValue = *ll;
	pOld = (unsigned char *)&oldValue;
	pNew = (unsigned char *)&newValue;

	temp = pOld[0];
	pNew[3] = temp;
	temp = pOld[1];
	pNew[2] = temp;
	temp = pOld[2];
	pNew[1] = temp;
	temp = pOld[3];
	pNew[0] = temp;

	*ll = newValue;
#else
	*ll = *ll;
#endif
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
OPErr GM_PauseGeneralSound(void)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (MusicGlobals)
	{
		if (MusicGlobals->systemPaused == FALSE)
		{
#if USE_SEQUENCER
			GM_PauseSequencer();
#endif
			GM_EndAllSoundEffects();
			MusicGlobals->systemPaused = TRUE;
			FinisSoundManager();		// disengage from hardware
		}
		else
		{
			theErr = ALREADY_PAUSED;
		}
	}
	return theErr;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
OPErr GM_ResumeGeneralSound(void)
{                
	OPErr	theErr;

	theErr = NO_ERR;
	if (MusicGlobals)
	{
		if (MusicGlobals->systemPaused)
		{
			InitSoundManager();			// reconnect to hardware
			MusicGlobals->systemPaused = FALSE;
#if USE_SEQUENCER
			GM_ResumeSequencer();
#endif			       
		}
		else
		{
			theErr = ALREADY_RESUMED;
		}
	}
	return theErr;
}
#endif

// Stop just sound effects
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_EndAllSoundEffects(void)
{           
	register LOOPCOUNT count;
	register NoteRecord *this_one;

	if (MusicGlobals)
	{
		for (count = MusicGlobals->MaxNotes; count < MusicGlobals->MaxNotes + MusicGlobals->MaxEffects; count++)
		{
			this_one = &MusicGlobals->NoteEntry[count];
			if (this_one->NoteDur >= 0)
			{
				PV_DoCallBack(this_one);
				this_one->NoteDur = -1;
			}
		}
	}
}
#endif


OPErr GM_ChangeSystemVoices(INT16 maxSongVoices, INT16 maxNormalizedVoices, INT16 maxEffectVoices)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if ((maxEffectVoices+maxSongVoices) >= SYMPHONY_SIZE)
		maxSongVoices = SYMPHONY_SIZE - maxEffectVoices - 1;
	if ( (maxSongVoices >= 0) && 
		(maxNormalizedVoices > 0) && 
		(maxEffectVoices >= 0) && 
		((maxEffectVoices+maxSongVoices) > 0) &&
		((maxEffectVoices+maxSongVoices) <= SYMPHONY_SIZE) )
	{
		MusicGlobals->MaxNotes = maxSongVoices;
		MusicGlobals->MaxNormNotes = maxNormalizedVoices;
		MusicGlobals->MaxEffects = maxEffectVoices;

		PV_CalcScaleBack();
	}
	else
	{
		theErr = PARAM_ERR;
	}
	return theErr;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_SetMasterVolume(INT32 theVolume)
{
	if (MusicGlobals)
	{
		MusicGlobals->MasterVolume = (INT16)theVolume;
		PV_CalcScaleBack();
	}
}
#endif

INT32 GM_GetMasterVolume(void)
{
	if (MusicGlobals)
	{
		return MusicGlobals->MasterVolume;
	}
	else
	{
		return MAX_MASTER_VOLUME;
	}
}


OPErr GM_InitGeneralSound(Quality theQuality, TerpMode theTerp, AudioModifiers theMods,
				INT16 maxVoices, INT16 normVoices, INT16 maxEffects)
{
	register MusicVars *	myMusicGlobals = nil;
	register INT32			count;
	OPErr				theErr;

	theErr = theMods;
	theErr = NO_ERR;
	if (normVoices > 32)
	{
		 if ((normVoices/100) > (maxVoices + maxEffects))
		{
			theErr = PARAM_ERR;
		}
	}
	else if (normVoices > (maxVoices + maxEffects))
	{
		theErr = PARAM_ERR;
	}

// Check terp mode
	switch (theTerp)
	{
		case E_AMP_SCALED_DROP_SAMPLE:
		case E_2_POINT_INTERPOLATION:
		case E_LINEAR_INTERPOLATION:
			break;
		default:
			theErr = PARAM_ERR;
			break;
	}

	if (theErr == NO_ERR)
	{
// Allocate MusicGlobals
//		DebugStr("\p Allocating a dummy note record for sizing us out");
//		MusicGlobals = (MusicVars *)XNewPtr( (long)sizeof(NoteRecord) );
//		DebugStr("\p Allocating MusicGlobals");
		MusicGlobals = (MusicVars *)XNewPtr( (long)sizeof(MusicVars) );
		if (MusicGlobals)
		{
			myMusicGlobals = MusicGlobals;
			for (count = 0; count < SYMPHONY_SIZE; count++)
			{
				myMusicGlobals->NoteEntry[count].NoteDur = -1;
			}
			myMusicGlobals->interpolationMode = theTerp;
		
			myMusicGlobals->MasterVolume = MAX_MASTER_VOLUME;
			myMusicGlobals->effectsVolume = MAX_MASTER_VOLUME * 3;	// samples 3 times normal volume
			myMusicGlobals->maxChunkSize = MAX_CHUNK_SIZE;
			myMusicGlobals->outputQuality = theQuality;

			myMusicGlobals->One_Slice = MAX_CHUNK_SIZE;
			myMusicGlobals->One_Loop = MAX_CHUNK_SIZE;
			myMusicGlobals->Two_Loop = MAX_CHUNK_SIZE/2;
			myMusicGlobals->Four_Loop = MAX_CHUNK_SIZE/4;
			myMusicGlobals->Sixteen_Loop = MAX_CHUNK_SIZE/16;
		
			myMusicGlobals->generate16output = TRUE;
			myMusicGlobals->generateStereoOutput = TRUE;

			myMusicGlobals->MaxNotes = maxVoices;
			myMusicGlobals->MaxNormNotes = normVoices;
			myMusicGlobals->MaxEffects = maxEffects;
			myMusicGlobals->reverbPtr = 0;
			myMusicGlobals->reverbBuffer = (long *)XNewPtr(REVERB_BUFFER_SIZE * sizeof(long));
			Message(("Reverb buffer is at address %x",&(MusicGlobals->reverbBuffer[0])));
			if (myMusicGlobals->reverbBuffer)
			{
				for (count = 0; count < REVERB_BUFFER_SIZE; count++)
				{
					myMusicGlobals->reverbBuffer[count] = 0;
				}
				myMusicGlobals->LPfilterL = 0;
				myMusicGlobals->LPfilterR = 0;
				myMusicGlobals->LPfilterLz = 0;
				myMusicGlobals->LPfilterRz = 0;
				myMusicGlobals->reverbUnitType = DEFAULT_REVERB_TYPE;
//				PV_ResetControlers(-1);		// reset controler values
#if USE_SEQUENCER
				GM_EndAllNotes();
#endif

// Compute volume multiplier for mix-level
				PV_CalcScaleBack();
// Turn off all notes!
				for (count = 0; count < myMusicGlobals->MaxNotes+myMusicGlobals->MaxEffects; count++)
				{
					myMusicGlobals->NoteEntry[count].NoteDur = -1;
				}
	
#ifdef SUPPORT_CONTROLLER_CALLBACKS
				for (count = 0; count < MAX_CONTROLLERS; count++)
				{
					myMusicGlobals->channelCallbackProc[count] = NULL;
				}
#endif

				for (count = 0; count < MAX_SAMPLES; count++)
				{
//					myMusicGlobals->samplePtrs[count] = NULL;
//					myMusicGlobals->sampleID[count] = -1;
					myMusicGlobals->sampleCaches[count] = NULL;
				}
				myMusicGlobals->anoymousCacheID = -30000;
				myMusicGlobals->cacheSamples = FALSE;				
	
				myMusicGlobals->cacheInstruments = FALSE;
				for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
				{
					myMusicGlobals->InstrumentData[count] = NULL;
				}
			}
			else
			{
				theErr = MEMORY_ERR;
			}
		}
		else
		{
			theErr = MEMORY_ERR;
		}
	}
	if (theErr == NO_ERR)
	{
		myMusicGlobals->insideAudioInterrupt = 0;
		myMusicGlobals->syncCount = 0;
		myMusicGlobals->syncCountFrac = 0;
		myMusicGlobals->sequencerPaused = FALSE;
		myMusicGlobals->systemPaused = FALSE;
		if (InitSoundManager() == FALSE)
		{
			theErr = MEMORY_ERR;
		}
	}
	return theErr;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
OPErr GM_ChangeAudioModes(Quality theQuality, TerpMode theTerp, AudioModifiers theMods)
{
	OPErr				theErr;
	register MusicVars *	myMusicGlobals;
	INT16				maxChunkSize;

	theErr = NO_ERR;
// Check terp mode
	switch (theTerp)
	{
		case E_AMP_SCALED_DROP_SAMPLE:
		case E_2_POINT_INTERPOLATION:
		case E_LINEAR_INTERPOLATION:
			break;
		default:
			theErr = PARAM_ERR;
			break;
	}
	switch (theQuality)
	{
		case Q_11K:
		case Q_22K:
		case Q_44K:
			break;
		default:
			theErr = PARAM_ERR;
			break;
	}
	if (theErr == NO_ERR)
	{
		FinisSoundManager();
		myMusicGlobals = MusicGlobals;

		if ( (theMods & M_USE_16) == M_USE_16)
		{
			myMusicGlobals->generate16output = XIs16BitSupported();
		}
		else
		{
			myMusicGlobals->generate16output = FALSE;
		}
		// double check users request for Stereo output. Make sure the hardware can play it
		if ( (theMods & M_USE_STEREO) == M_USE_STEREO)
		{
			myMusicGlobals->generateStereoOutput = XIsStereoSupported();
		}
		else
		{
			myMusicGlobals->generateStereoOutput = FALSE;
		}
		maxChunkSize = myMusicGlobals->maxChunkSize;
		myMusicGlobals->outputQuality = theQuality;
		myMusicGlobals->One_Slice = maxChunkSize;
		myMusicGlobals->One_Loop = maxChunkSize;
		myMusicGlobals->Two_Loop = maxChunkSize/2;
		myMusicGlobals->Four_Loop = maxChunkSize/4;
		myMusicGlobals->Sixteen_Loop = MAX_CHUNK_SIZE/16;
		myMusicGlobals->interpolationMode = theTerp;
// Recompute mix level
		PV_CalcScaleBack();
		if (InitSoundManager() == FALSE)
		{
			theErr = MEMORY_ERR;
		}
	}
	return theErr;
}
#endif

void GM_FinisGeneralSound(void)
{
	if (MusicGlobals)
	{
		MusicGlobals->systemPaused = TRUE;
#if USE_SEQUENCER
		GM_FreeSong(NULL);		// free all songs
#endif
		// Close up sound manager BEFORE releasing memory!
		FinisSoundManager();
  
  		GM_FlushInstrumentCache(FALSE);

		if (MusicGlobals->reverbBuffer)
		{
			XDisposePtr((XPTR)MusicGlobals->reverbBuffer);
		}

		XDisposePtr( (XPTR) MusicGlobals );
		MusicGlobals = NULL;
	}
}

INT32 PV_ScaleVolumeFromChannelAndSong(INT16 channel, INT32 volume)
{
	register INT32		newVolume;
	register GM_Song	*pSong;

	// scale song volume based upon master song volume, only if a song channel
	if (channel != SOUND_EFFECT_CHANNEL)
	{
		pSong = MusicGlobals->theSongPlaying;
		if (pSong)
		{
			newVolume = (((unsigned long)volume) * 
						(unsigned long)pSong->channelVolume[channel]) / MAX_NOTE_VOLUME;

			// scale note velocity via current song volume
			newVolume = (((unsigned long)newVolume) * 
						(unsigned long)pSong->songVolume) / MAX_NOTE_VOLUME;
		}
		else
		{
			newVolume = volume;
		}
	}
	else
	{
		// scale note velocity via current master effects volume
		newVolume = (((unsigned long)volume) * 
					(unsigned long)MusicGlobals->effectsVolume) / MAX_MASTER_VOLUME;
	}
	return newVolume;
}


// ------------------------------------------------------------------------------------------------------//

// Given a stereo position from -63 to 63, return a volume level from 0 to 127
void PV_CalculateStereoVolume(NoteRecord *this_voice, INT32 *pLeft, INT32 *pRight)
{
	register INT32 stereoPosition;
	register INT32 left, right, noteVolume;

#if ALLOW_DEBUG_STEREO
	if (IsOptionOn() == FALSE)
	{
		stereoPosition = this_voice->stereoPosition + this_voice->stereoPanBend;
	}
	else
	{
		stereoPosition = ((VX_GetMouse() & 0xFFFFL) / 6) - 63;
	}
#else
		stereoPosition = this_voice->stereoPosition + this_voice->stereoPanBend;
		if (stereoPosition > 63)
		{
			stereoPosition = 63;
		}
		if (stereoPosition < -63)
		{
			stereoPosition = -63;
		}
#endif
	if (stereoPosition)
	{
		if (stereoPosition < 0)	// left changes
		{
			right = MAX_NOTE_VOLUME + (stereoPosition * 2);
			left = MAX_NOTE_VOLUME;
		}
		else
		{					// right changes
			right = MAX_NOTE_VOLUME;
			left = MAX_NOTE_VOLUME - (stereoPosition * 2);
		} 
	}
	else
	{
		left = MAX_NOTE_VOLUME;
		right = MAX_NOTE_VOLUME;
	}
	// scale new volume based up channel volume, song volume, and current note volume
	noteVolume = PV_ScaleVolumeFromChannelAndSong(this_voice->NoteChannel, this_voice->NoteVolume);
	noteVolume = (noteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;

	*pLeft = (((unsigned long)left) * 
					(unsigned long)noteVolume) / MAX_NOTE_VOLUME;
	*pRight = (((unsigned long)right) * 
					(unsigned long)noteVolume) / MAX_NOTE_VOLUME;
}




void SetChannelVolume(INT16 the_channel, INT16 newVolume)
{
	register MusicVars		*myMusicGlobals;
	register LOOPCOUNT		count;
	register NoteRecord *	theNote;

	myMusicGlobals = MusicGlobals;
	// update the current notes playing to the new volume
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		theNote = &myMusicGlobals->NoteEntry[count];
		if (theNote->NoteDur >= 0)
		{
			if (theNote->NoteChannel == the_channel)
			{
				if (newVolume == 0)
				{
					theNote->NoteDur = 0;
					theNote->NoteDecay = 0;
					theNote->volumeADSRRecord.ADSRTime[0] = 1;
					theNote->volumeADSRRecord.ADSRFlags[0] = ADSR_TERMINATE;
					theNote->volumeADSRRecord.ADSRLevel[0] = 0;	// just in case
				}
				// now calculate the new volume based upon the current channel volume and
				// the unscaled note volume
				newVolume = PV_ScaleVolumeFromChannelAndSong(the_channel, theNote->NoteMIDIVolume);
				newVolume = (newVolume * MusicGlobals->scaleBackAmount) >> 8;
				theNote->NoteVolume = newVolume;
			}
		}
	}
}


// Put all notes that have been in 'SUS_HOLD' mode into their normal decay release mode
void PV_ChangeSustainedNotes(INT16 the_channel, INT16 data)
{
	register MusicVars		*myMusicGlobals;
	register LOOPCOUNT		count;
	register NoteRecord *	theNote;

	myMusicGlobals = MusicGlobals;
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		theNote = &myMusicGlobals->NoteEntry[count];
		if (theNote->NoteDur >= 0)
		{
			if (theNote->NoteChannel == the_channel)
			{
				if (data == 0)	// release
				{
					// the note has been released by the fingers, so release note
					if (theNote->sustainMode == SUS_ON_NOTE_OFF)
					{
						theNote->NoteDur = 0;	// decay note out to prevent clicks
					}
					theNote->sustainMode = SUS_NORMAL;
				}
				else
				{	// change status
					theNote->sustainMode = SUS_ON_NOTE_ON;
				}
			}
		}
	}
}

// Set stereo position from control values of 0-127. This will translate into values of 63 to -63
INT16 SetChannelStereoPosition(INT16 the_channel, UINT16 newPosition)
{
	register MusicVars		*myMusicGlobals;
	register LOOPCOUNT		count;
	register NoteRecord *	theNote;
	register INT16			newLogPosition;
	static char stereoScale[] = 
	{
		63, 58, 55, 52, 50, 47, 45, 43,		 41, 39, 37, 35, 33, 32, 30, 29,
		27, 26, 25, 23, 22, 21, 20, 19, 		18, 17, 17, 16, 15, 14, 14, 13, 
		12, 12, 11, 11, 10, 10,   9,  9,   		 8,   8,   7,   7,   7,   6,   6,   6, 
		  6,   5,   5,   5,   5,   4,   4,  4,   		 4,   4,   3,   3,    3,   2,   1,   0,
		  0,
		-1, -2,  -3, -3,  -3,   -4, -4, -4, 	 	-4, -4,  -5,  -5,  -5, -5,  -6,  -6, 
		-6, -6,  -7,  -7,  -7,  -8, -8, -9, 		-9,-10,-10,-11,-11,-12,-12,-13, 
		-14,-14,-15,-16,-17,-17,-18,-19,	-20,-21,-22,-23,-25,-26,-27,-29, 
		-30,-32,-33,-35,-37,-39,-41,-43, 	-45, -47, -50, -52, -55, -58, -63
	};

	myMusicGlobals = MusicGlobals;
	// make sure and set the channel stereo position
	newLogPosition = stereoScale[newPosition];
	// update the current notes playing to the new stereo position. It will get incorporated into the mix at the
	// next audio frame
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		theNote = &myMusicGlobals->NoteEntry[count];
		if (theNote->NoteDur >= 0)
		{
			if (theNote->NoteChannel == the_channel)
			{
				theNote->stereoPosition = newLogPosition;
			}
		}
	}
	return newLogPosition;
}

// Set mod wheel position from control values of 0-127.
void SetChannelModWheel(INT16 the_channel, UINT16 value)
{
	register MusicVars		*myMusicGlobals;
	register LOOPCOUNT		count;
	register NoteRecord *	theNote;

	myMusicGlobals = MusicGlobals;

	// update the current notes playing to the new MOD wheel setting
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		theNote = &myMusicGlobals->NoteEntry[count];
		if (theNote->NoteDur >= 0)
		{
			if (theNote->NoteChannel == the_channel)
				theNote->ModWheelValue = value;
		}
	}
}


// Change pitch all notes playing on this channel, and for new notes on this channel
INT16 SetChannelPitchBend(INT16 the_channel, UBYTE bendRange, BYTE bendMSB, BYTE bendLSB)
{
	register LOOPCOUNT		count;
	register MusicVars		*myMusicGlobals;
	register long			bendAmount, the_pitch_bend;

	myMusicGlobals = MusicGlobals;
	// Convert LSB & MSB into values from -8192 to 8191
	the_pitch_bend = (((bendMSB * 128) + bendLSB) - 8192);

	// Scale values from -8192 to 8192 to -bend to bend in 8.8 fixed point
	bendAmount = bendRange * 256;
	the_pitch_bend = (the_pitch_bend * bendAmount) / 8192;

	// update the current note playing to the new bend value
	for (count = 0; count < myMusicGlobals->MaxNotes; count++)
	{
		if (myMusicGlobals->NoteEntry[count].NoteDur >= 0)
		{
			if (myMusicGlobals->NoteEntry[count].NoteChannel == the_channel)
			{
				myMusicGlobals->NoteEntry[count].PitchBend = the_pitch_bend;
			}
		}
	}
	return the_pitch_bend;
}


// Begin a double buffer sound effect
INT32 GM_BeginDoubleBuffer(G_PTR pBuffer1, G_PTR pBuffer2, INT32 theSize, INT32 theRate,
							INT16 bitSize, INT16 channels,
							INT32 sampleVolume, INT16 stereoPosition,
							INT32 refData,
							GM_DoubleBufferCallbackPtr bufferCallback)
{
	register MusicVars	*myMusicGlobals;
	register NoteRecord	*the_entry;
	register INT16		 count, max, min;

	myMusicGlobals = MusicGlobals;
	if (myMusicGlobals->MaxEffects > 0)
	{
		min = myMusicGlobals->MaxNotes;
		max =  min + (myMusicGlobals->MaxEffects-1);
		for (count = max; count >= min; count--)
		{
			the_entry = &myMusicGlobals->NoteEntry[count];
			if (the_entry->NoteDur < 0)
			{
				PV_CleanNoteEntry(the_entry);	// fill with all zero's except NoteDur field.
				the_entry->noteSamplePitchAdjust = 0x10000;
				the_entry->doubleBufferProc = bufferCallback;
				the_entry->NotePtr = (UBYTE FAR *) pBuffer1;
				the_entry->NotePtrEnd = (UBYTE FAR *) pBuffer1 + theSize;

				the_entry->doubleBufferPtr1 = (UBYTE FAR *) pBuffer1;
				the_entry->doubleBufferPtr2 = (UBYTE FAR *) pBuffer2;

				the_entry->NoteLoopPtr = the_entry->NotePtr;
				the_entry->NoteLoopEnd = the_entry->NotePtrEnd;

				the_entry->NotePitch = (UINT32)theRate / 22050;	// was 22254;
				the_entry->NoteLoopProc = NULL;

				the_entry->NoteEndCallback = NULL;
				the_entry->NoteProgram = -1;      
				the_entry->stereoPosition = (stereoPosition * 63) / 255;
				the_entry->bitSize = bitSize;
				the_entry->channels = channels;
				the_entry->avoidReverb = TRUE;

				sampleVolume = (sampleVolume * myMusicGlobals->effectsVolume) / MAX_MASTER_VOLUME;
				if (sampleVolume)
				{
					the_entry->NoteVolume = sampleVolume;
					the_entry->NoteMIDIVolume = sampleVolume;
					the_entry->NoteVolumeEnvelope = VOLUME_RANGE;
					the_entry->volumeADSRRecord.ADSRLevel[0] = VOLUME_RANGE;
					the_entry->volumeADSRRecord.ADSRFlags[0] = ADSR_TERMINATE;
					the_entry->volumeADSRRecord.currentPosition = VOLUME_RANGE;
					the_entry->volumeADSRRecord.sustainingDecayLevel = 0x10000;
	
					the_entry->NoteChannel = SOUND_EFFECT_CHANNEL;
					the_entry->NoteDecay = 0x7FFF;		// never release
					the_entry->NoteRefNum = refData;
					the_entry->sustainMode = SUS_NORMAL;
					the_entry->NoteDur = 0x7FFF;
					return count;
				}
			}
		}
	}
	return -1L;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
INT32 GM_BeginSample(G_PTR theData, INT32 theSize, INT32 theRate, 
				INT32 theStartLoop, INT32 theEndLoop, 
				INT32 sampleVolume, INT32 stereoPosition,
				INT32 refData, INT16 bitSize, INT16 channels, 
				GM_LoopDoneCallbackPtr theLoopContinueProc,
				GM_SoundDoneCallbackPtr theCallbackProc)
{
	register MusicVars	*myMusicGlobals;
	register NoteRecord	*the_entry;
	register INT16		 count, max, min;

	myMusicGlobals = MusicGlobals;
	if (myMusicGlobals->MaxEffects > 0)
	{
		min = myMusicGlobals->MaxNotes;
		max =  min + (myMusicGlobals->MaxEffects-1);
		for (count = max; count >= min; count--)
		{
			the_entry = &myMusicGlobals->NoteEntry[count];
			if (the_entry->NoteDur < 0)
			{
				PV_CleanNoteEntry(the_entry);		// zeroes ALL entries
				the_entry->noteSamplePitchAdjust = 0x10000;

				the_entry->NotePtr = (UBYTE FAR *) theData;
				the_entry->NotePtrEnd = (UBYTE FAR *) theData + FP_OFF(theSize);

				the_entry->NotePitch = (UINT32)theRate / 22050;	// was 22254;

				the_entry->NoteLoopProc = theLoopContinueProc;
				if ( (theStartLoop < theEndLoop) && (theEndLoop - theStartLoop > MIN_LOOP_SIZE) )
				{
					the_entry->NoteLoopPtr = theData + FP_OFF(theStartLoop);
					the_entry->NoteLoopEnd = theData + FP_OFF(theEndLoop);
				}
				the_entry->NoteEndCallback = theCallbackProc;
				the_entry->NoteProgram = -1;      
				the_entry->stereoPosition = (stereoPosition * 63) / 255;
				the_entry->bitSize = bitSize;
				the_entry->channels = channels;
				the_entry->avoidReverb = TRUE;

				sampleVolume = (sampleVolume * myMusicGlobals->effectsVolume) / MAX_MASTER_VOLUME;
				if (sampleVolume)
				{
					the_entry->NoteVolume = sampleVolume;
					the_entry->NoteMIDIVolume = sampleVolume;
					the_entry->NoteVolumeEnvelope = VOLUME_RANGE;
					the_entry->volumeADSRRecord.ADSRLevel[0] = VOLUME_RANGE;
					the_entry->volumeADSRRecord.currentLevel = VOLUME_RANGE;
					the_entry->volumeADSRRecord.ADSRFlags[0] = ADSR_TERMINATE;
					the_entry->volumeADSRRecord.sustainingDecayLevel = 0x10000;
					the_entry->NoteChannel = SOUND_EFFECT_CHANNEL;
					the_entry->NoteDecay = 0x7FFF;		// never release
					the_entry->NoteRefNum = refData;
					the_entry->sustainMode = SUS_NORMAL;
					the_entry->sampleAndHold = 0;
					the_entry->NoteDur = 0x7FFF;
					return count;
				}
			}
		}
	}
	return -1L;
}
#endif

void GM_EndSample(INT32 theRef)
{
	if (MusicGlobals)
	{
		if ( (theRef >= MusicGlobals->MaxNotes) && (theRef < (MusicGlobals->MaxNotes+MusicGlobals->MaxEffects)) )
		{
			PV_DoCallBack(&MusicGlobals->NoteEntry[theRef]);
			MusicGlobals->NoteEntry[theRef].NoteDur = -1;
		}
	}
}

BOOL_FLAG GM_IsSoundDone(INT32 theRef)
{
	register INT32 count;

	if (MusicGlobals)
	{
		if (theRef > 0)
		{
			if (theRef < (MusicGlobals->MaxNotes+MusicGlobals->MaxEffects))
			{
				return (MusicGlobals->NoteEntry[theRef].NoteDur < 0);
			}
		}
	
		if (theRef < 0)
		{
			return TRUE;
		}
		for (count = MusicGlobals->MaxNotes; count < MusicGlobals->MaxNotes+MusicGlobals->MaxEffects; count++)
		{
			if (MusicGlobals->NoteEntry[count].NoteDur >= 0)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}                


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_ChangeSamplePitch(INT32 theRef, INT32 theNewRate)
{
	if (MusicGlobals)
	{
		if ( (theRef > 0) && (theRef < (MusicGlobals->MaxNotes+MusicGlobals->MaxEffects)) )
		{
			if ((theNewRate >> 16) == 0x56ee)
			{
				MusicGlobals->NoteEntry[theRef].NotePitch  = (UINT32)theNewRate / 22254;
			}
			else
			{
				MusicGlobals->NoteEntry[theRef].NotePitch  = ( (UINT32)theNewRate / 22050);
			}
		}
	}
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_ChangeSampleReverb(INT32 theRef, INT16 enable)
{
	if (MusicGlobals)
	{
		if ( (theRef > 0) && (theRef < (MusicGlobals->MaxNotes+MusicGlobals->MaxEffects)) )
		{
			MusicGlobals->NoteEntry[theRef].avoidReverb = ! enable;
		}
	}
}
#endif

// Volume range is from 0 to MAX_NOTE_VOLUME
#if CODE_BASE == USE_NATIVE
void GM_ChangeSampleVolume(INT32 theRef, INT16 sampleVolume)
{
	register NoteRecord *	theNote;

	if (MusicGlobals)
	{
		if ( (theRef > 0) && (theRef < (MusicGlobals->MaxNotes+MusicGlobals->MaxEffects)) )
		{
			theNote = &MusicGlobals->NoteEntry[theRef];
			sampleVolume *= 4;
			theNote->NoteVolume = sampleVolume;
			theNote->NoteMIDIVolume = sampleVolume;
		}
	}
}
#endif

// range from -63 to 63
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_ChangeSampleStereoPosition(INT32 theRef, INT16 newStereoPosition)
{
	register NoteRecord *	theNote;

	if (MusicGlobals)
	{
		if ( (theRef > 0) && (theRef < (MusicGlobals->MaxNotes+MusicGlobals->MaxEffects)) )
		{
			theNote = &MusicGlobals->NoteEntry[theRef];
			theNote->stereoPosition = newStereoPosition;
		}
	}
}
#endif


// Set the global reverb type
void GM_SetReverbType(ReverbMode reverbMode)
{
	switch (reverbMode)
	{
		case REVERB_TYPE_1:
		case REVERB_TYPE_2:
		case REVERB_TYPE_3:
		case REVERB_TYPE_4:
		case REVERB_TYPE_5:
		case REVERB_TYPE_6:
			break;
		case 0:	// no change
		default:
			reverbMode = -1;
			break;
	}
	if ( (MusicGlobals) && (reverbMode != -1) )
	{
		MusicGlobals->reverbUnitType = reverbMode;
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ReverbMode GM_GetReverbType(void)
{
	ReverbMode reverbMode;

	reverbMode = REVERB_TYPE_1;
	if (MusicGlobals)
	{
		reverbMode = MusicGlobals->reverbUnitType;
	}
	return reverbMode;
}
#endif

/* EOF of GenSetup.c
*/

