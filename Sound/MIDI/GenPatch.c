/*
************************************************************************
**
** “GenPatch.c”
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
**	This contains code to load an maintain patches (instruments). This code library is specific
**	to the platform, but the API is independent in nature.
**
**
** Modification History:
**
**	7/7/95		Created
**	11/7/95		Major changes, revised just about everything.
**	11/20/95	Removed the BF_ flags, now you must walk through the union structure
**				Remove bit fields. BIT FIELDS DON'T WORK WITH MPW!!!!
**   12/6/95		Added reverb on/off bit
**	12/11/95	Enlarged PV_GetEnvelopeData to include the parsing for the external instrument
**				format
**	12/14/95	Modified PV_GetSampleData to accept an external snd handle and cache/convert
**				that into a sample
**	1/13/96		Modified PV_GetInstrument to only run SMOD on 8 bit mono data
**				Modified PV_GetEnvelopeData to support the extended format bit in the internal instrument
**	1/18/96		Spruced up for C++ extra error checking
**				Added MIN_LOOP_SIZE
**				Added GM_SetUsedInstrument & GM_IsInstrumentUsed & GM_IsInstrumentRangeUsed
**				and changed the way InstUseList is built. Now its only built with GM_LoadSongInstruments
**				and only loads the samples needed in the splits
**	1/28/96		Removed all uses of Resource Manager
**	1/29/96		Added useSampleRate factor for playback of instruments & sampleAndHold bits
**				Changed PV_CreateInstrumentFromResource to propigate features to splits
**	2/3/96		Removed extra includes
**	2/5/96		Removed unused variables. Working towards multiple songs
**	2/18/96		Added panPlacement to the GM_Instrument structure
**	2/21/96		Changed to support the change in XGetResourceAndDetach
**	3/25/96		Removed private GetLong to XGetLong
**	3/28/96		Changed PV_GetInstrument to support external instruments
**				Added PV_SetSampleIntoCache
**	4/10/96		Reworked the sample cache system to not clone the sample data
**	5/12/96		Removed CPU edian/alignment issues by use XGetShort & XGetLong
**				CSND SUPPORT TOTALLY BROKEN!!!
**
**
************************************************************************
*/

#define DISPLAY_INSTRUMENTS		0
#define DISPLAY_INSTRUMENTS_FILE	0

#include "GenSnd.h"
#include "GenPriv.h"
#include "SMOD.h"
#include "X_API.h"

#if STAND_ALONE == 1
	#include <SetupA4.h>
	#include <A4Stuff.h>
#endif

#include "PrivateSoundMusicSystem.h"
#include "Headers.h"

// SMOD jump table
static void (*smod_functions[])(unsigned char *pSample, long length, long param1, long param2) =
{
	VolumeAmpScaler,		//	Amplifier/Volume Scaler
	NULL,
	NULL,
	NULL
};

#define NOTE_DEFAULT_DECAY		8

// Private Functions

static BYTE	ConvertADSRFlags(INT32 f);
static BYTE	ConvertADSRFlags(INT32 f)
{
	switch (f & 0x5F5F5F5F)
	{
		case QuadChar('C','U','R','V'):
			return ADSR_EXPONENTIAL_CURVE;
		case QuadChar('S','U','S','T'):
			return ADSR_SUSTAIN;
		case QuadChar('L','A','S','T'):
			return ADSR_TERMINATE;
		case QuadChar('G','O','T','O'):
			return ADSR_GOTO;
		case QuadChar('G','O','S','T'):
			return ADSR_GOTO_CONDITIONAL;
		case QuadChar('R','E','L','S'):
			return ADSR_RELEASE;
		case QuadChar('L','I','N','E'):
		default:
			return ADSR_LINEAR_RAMP;
	}
}
		

#if X_HARDWARE_PLATFORM != X_MACINTOSH
// Decompress a 'csnd' format sound.
// First byte is a type.
// Next three bytes are a length.
// Type 0 is Delta LZSS compression
void * DecompressSampleFormatPtr(void * pData, long dataSize)
{
	long		theTotalSize;
	char		theType;
	XPTR	theNewData;

	theTotalSize = XGetLong(pData);
	theType = theTotalSize >> 24L;		// get type

	theNewData = XNewPtr(theTotalSize);
	if (theNewData)
	{
		switch (theType)
		{
			case 0:
				LZSSDeltaUncompress((unsigned char *)(char *)(pData) + sizeof(long), dataSize - sizeof(long), 
								(unsigned char *)theNewData, &theTotalSize);
				break;
			default:
				XDisposePtr(theNewData);
				theNewData = NULL;
				break;
		}
	}

	return theNewData;
}
#endif

// Get sound resource and detach from resource manager or decompress
// This function can be replaced for a custom sound retriver
static XPTR PV_GetSoundResource(long theID, long *pSize)
{
	XPTR	thePreSound, theData;

	theData = XGetAndDetachResource(ID_SND, theID, pSize);
	if (theData == NULL)
	{
		theData = XGetAndDetachResource(ID_CSND, theID, pSize);
		if (theData)
		{
			thePreSound = theData;
			theData = DecompressSampleFormatPtr(thePreSound, *pSize);
			XDisposePtr(thePreSound);
		}
	}
	return theData;
}

// Return a pointer to a 'snd' resource from its ID.
// If a handle is passed, then its assumed to be a snd resource and is used instead. It is not disposed
// of after use, so you must dispose of it
static void * PV_GetSampleData(long theID, XPTR useThisSnd, CacheSampleInfo *pInfo)
{
	XPTR			theData, thePreSound;
	char				*theSound;
	CacheSampleInfo	theSoundData;
	SampleDataInfo		newSoundInfo;
	long				size;

	theSound = NULL;
	if (useThisSnd)
	{
		theData = useThisSnd;
	}
	else
	{
		theData = PV_GetSoundResource(theID, &size);
	}
	if (theData)
	{
		// convert snd resource into a simple pointer of data with information

		thePreSound = XGetSamplePtrFromSnd(theData, &newSoundInfo);

		if (newSoundInfo.pMasterPtr != theData)
		{	// this means that XGetSamplePtrFromSnd created a new sample
			XDisposePtr(theData);
		}

		if (thePreSound)
		{
			// validate loop points
			if (	(newSoundInfo.loopStart > newSoundInfo.loopEnd) ||
				(newSoundInfo.loopEnd > newSoundInfo.frames) ||
				((newSoundInfo.loopEnd - newSoundInfo.loopStart) < MIN_LOOP_SIZE) )
			{
				// disable loops
				newSoundInfo.loopStart = 0;
				newSoundInfo.loopEnd = 0;
			}
			theSoundData.theID = theID;
			theSoundData.waveSize = newSoundInfo.size;
			theSoundData.waveFrames = newSoundInfo.frames;
			theSoundData.loopStart = newSoundInfo.loopStart;
			theSoundData.loopEnd = newSoundInfo.loopEnd;
			theSoundData.baseKey = newSoundInfo.baseKey;
			theSoundData.bitSize = newSoundInfo.bitSize;
			theSoundData.channels = newSoundInfo.channels;
			theSoundData.rate = newSoundInfo.rate;

#if DISPLAY_INSTRUMENTS
			DPrint(drawDebug, "---->Getting 'snd' ID %ld rate %lX loopstart %ld loopend %ld basekey %ld\r", (long)theID,
													theSoundData.rate, theSoundData.loopStart,
													theSoundData.loopEnd, (long)theSoundData.baseKey);
#endif
			theSoundData.pSampleData = thePreSound;
			theSoundData.pMasterPtr = newSoundInfo.pMasterPtr;
			theSoundData.cacheBlockID = ID_INST;
			*pInfo = theSoundData;
			theSound = (char *) thePreSound;
		}		
	}
	return theSound;
}

// Free cache entry
static void PV_FreeCacheEntry(CacheSampleInfo *pCache)
{
	if (pCache)
	{
		if (pCache->pSampleData)
		{
			XDisposePtr(pCache->pMasterPtr);
		}
		XDisposePtr(pCache);
	}
}

// Given an ID this will return a pointer to the sample that matches that ID if it is loaded into the sample
// cache, otherwise NULL is returned.
static void * PV_FindSoundFromID(short int theID)
{
	register short int	count;
	register void		*pSample;
	CacheSampleInfo	*pCache;

	pSample = NULL;
	for (count = 0; count < MAX_SAMPLES; count++)
	{
		pCache = MusicGlobals->sampleCaches[count];
		if (pCache)
		{
			if (pCache->theID == theID)
			{
				pSample = pCache->pSampleData;
				break;
			}
		}
	}
	return pSample;
}

// Given an ID, this will return the cache entry
static CacheSampleInfo * PV_FindCacheFromID(short int theID)
{
	register short int	count;
	register void		*pSample;
	CacheSampleInfo	*pCache;

	pSample = NULL;
	for (count = 0; count < MAX_SAMPLES; count++)
	{
		pCache = MusicGlobals->sampleCaches[count];
		if (pCache)
		{
			if (pCache->theID == theID)
			{
				return pCache;
			}
		}
	}
	return NULL;
}

// Given a pointer this will return an index to the sample that matches that pointer if it is loaded into the sample
// cache, otherwise NULL is returned.
static short int PV_FindCacheIndexFromPtr(void *pInSample)
{
	register short int	count;
	register short int	sampleIndex;
	CacheSampleInfo	*pCache;

	sampleIndex = -1;
	for (count = 0; count < MAX_SAMPLES; count++)
	{
		pCache = MusicGlobals->sampleCaches[count];
		if (pCache)
		{
			if (pCache->pSampleData == pInSample)
			{
				sampleIndex = count;
				break;
			}
		}
	}
	return sampleIndex;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
// This will free a reference to the sound id that is passed.
static void PV_FreeCacheEntryFromID(short int theID)
{
	register short int 	count;
	CacheSampleInfo	*pCache;

	if (MusicGlobals->cacheSamples == FALSE)
	{
		for (count = 0; count < MAX_SAMPLES; count++)
		{
			pCache = MusicGlobals->sampleCaches[count];
			if (pCache)
			{
				if (pCache->theID == theID)
				{
					PV_FreeCacheEntry(pCache);
					MusicGlobals->sampleCaches[count] = NULL;
					break;
				}
			}
		}
	}
}
#endif

// This will free a cache reference to the sound pointer that is passed.
static void PV_FreeCacheEntryFromPtr(void *pSample)
{
	register short int 	count;
	CacheSampleInfo	*pCache;

	if (MusicGlobals->cacheSamples == FALSE)
	{
		for (count = 0; count < MAX_SAMPLES; count++)
		{
			pCache = MusicGlobals->sampleCaches[count];
			if (pCache)
			{
				if (pCache->pSampleData == pSample)
				{
					PV_FreeCacheEntry(pCache);
					MusicGlobals->sampleCaches[count] = NULL;
					break;
				}
			}
		}
	}
}


// This will place a sample into the sample cache. Use for eMidi files.
void PV_SetSampleIntoCache(long theID, XPTR pSndFormatData)
{
	register short int		count;
	register void			*pSample;
	CacheSampleInfo		info;

// first, find out if there is a sample already for this ID
	pSample = PV_FindSoundFromID(theID);
	if (pSample)
	{	// yes, there is so free it
		PV_FreeCacheEntryFromPtr(pSample);
	}
	pSample = PV_GetSampleData(theID, pSndFormatData, &info);
	if (pSample)
	{
		for (count = 0; count < MAX_SAMPLES; count++)
		{
			if (MusicGlobals->sampleCaches[count] == NULL)
			{
				MusicGlobals->sampleCaches[count] = (CacheSampleInfo *) XNewPtr((long)sizeof(CacheSampleInfo));
				if (MusicGlobals->sampleCaches[count])
				{
					*MusicGlobals->sampleCaches[count] = info;
					MusicGlobals->sampleCaches[count]->theID = theID;
				}
				break;
			}
		}
	}
}

// This will return a pointer to a sound. If the sound is already loaded, then just the pointer is returned.
static void * PV_GetSampleFromID(short int theID, CacheSampleInfo *pInfo)
{
	register short int		count;
	register void			*pSample;
	CacheSampleInfo		*pCache;

	pSample = NULL;
	pCache = PV_FindCacheFromID(theID);
	if (pCache == NULL)
	{	// not loaded, so load it
		pSample = PV_GetSampleData(theID, NULL, pInfo);
		if (pSample)
		{
			for (count = 0; count < MAX_SAMPLES; count++)
			{
				if (MusicGlobals->sampleCaches[count] == NULL)
				{
					MusicGlobals->sampleCaches[count] = (CacheSampleInfo *) XNewPtr((long)sizeof(CacheSampleInfo));
					if (MusicGlobals->sampleCaches[count])
					{
						*MusicGlobals->sampleCaches[count] = *pInfo;
						MusicGlobals->sampleCaches[count]->theID = theID;
					}
					break;
				}
			}
		}
	}
	else
	{
		*pInfo = *pCache;
		pSample = pInfo->pSampleData;
	}
	return pSample;
}



// Process a sample pointer with a SMOD
static void PV_ProcessSampleWithSMOD(void *pSample, long length, short int masterID, short int smodID, long param1, long param2)
{
	short int	sampleIndex;

	if ( (smodID < SMOD_COUNT) && smod_functions[smodID])
	{
		sampleIndex = PV_FindCacheIndexFromPtr(pSample);
		if (sampleIndex != -1)
		{
			// Find sample, and remove its ID number so that other instrument that try to share this one
			// will force a new copy to be loaded.
			MusicGlobals->sampleCaches[sampleIndex]->theID = -masterID;		// SMOD's sounds are negitive of their masters

			(*smod_functions[smodID])((unsigned char *)pSample, length, param1, param2);
		}
		else
		{
			DEBUG_STR("\pSomething is wrong with sample cache. Can't find sample.");
		}
	}
}




// Given an external instrument resource and an internal resource type fill the envelope data
// and if not there, place a default envelope
static void PV_GetEnvelopeData(InstrumentResource	*theX, GM_Instrument *theI, long theXSize)
{
	long				count, count2, lfoCount;
	long				size, unitCount, unitType, unitSubCount;
	unsigned short  int	data;
	register char 		*pData, *pData2;
	register char 		*pUnit;
	register KeySplit 	*pSplits;
	register LFORecordType	*pLFO;
	register ADSRRecord *pADSR;
	register CurveRecord *pCurve;
	int	disableModWheel = 0;

#if 1
	theI->volumeADSRRecord.currentTime = 0;
	theI->volumeADSRRecord.currentPosition = 0;
	theI->volumeADSRRecord.currentLevel = 0;
	theI->volumeADSRRecord.previousTarget = 0;
	theI->volumeADSRRecord.mode = 0;
	theI->volumeADSRRecord.sustainingDecayLevel = 65536;

	theI->LPF_frequency = 0;
	theI->LPF_resonance = 0;
	theI->LPF_lowpassAmount = 0;

	// fill default
	theI->volumeADSRRecord.ADSRLevel[0] = VOLUME_RANGE;
	theI->volumeADSRRecord.ADSRFlags[0] = ADSR_TERMINATE;
	theI->volumeADSRRecord.ADSRTime[0] = 0;
	theI->LFORecordCount = 0;
	theI->curveRecordCount = 0;
	theI->extendedFormat = FALSE;
	theI->reserved_1 = FALSE;
#endif
	pUnit = NULL;
	size = theXSize;
	if (theX && size)
	{
		if (theX->flags1 & ZBF_extendedFormat)
		{
			// search for end of tremlo data $8000. If not there, don't walk past end of instrument
			pSplits = (KeySplit *) ( ((char *)&theX->keySplitCount) + sizeof(short));
			count = XGetShort(&theX->keySplitCount);
			pData = (char *)&pSplits[count];
			pData2 = (char *)theX;
			size -= (pData - pData2);
			for (count = 0; count < size; count++)
			{
				data = (pData[count] << 8) + pData[count+1];
				if (data == 0x8000)
				{
					count += 4;						// skip past end token and extra word
					data = pData[count] + 1;				// get first string length;
					count2 = pData[count+data] + 1;		// get second string length
					pUnit = (char *) (&pData[count + data + count2]);
					// NOTE: src will be non aligned, possibly on a byte boundry.
					break;
				}
			}
			if (pUnit)
			{
				theI->extendedFormat = TRUE;
				pUnit += 12;		// reserved global space

				unitCount = *pUnit;		// how many unit records?
				pUnit++;					// byte
				if (unitCount)
				{
					lfoCount = 0;
#ifdef VERBOSE
					Message(("Parsing %d units", unitCount));
#endif
					for (count = 0; count < unitCount; count++)
					{
						unitType = XGetLong(pUnit) & 0x5F5F5F5F;
						pUnit += 4;	// long
						switch (unitType)
						{
							case QuadChar('C','U','R','V'):
#ifdef VERBOSE
								Message(("Curve data found in record!"));
#endif
								if (theI->curveRecordCount >= 4)	// can only handle 4
									goto bailoninstrument;
								pCurve = &theI->curve[theI->curveRecordCount];
								theI->curveRecordCount++;
								pCurve->tieFrom = XGetLong(pUnit) & 0x5F5F5F5F; pUnit += 4;
								pCurve->tieTo = XGetLong(pUnit) & 0x5F5F5F5F; pUnit += 4;
								unitSubCount = *pUnit++;
								if (unitSubCount > 8)
									goto bailoninstrument;
								pCurve->curveCount = unitSubCount;
								for (count2 = 0; count2 < unitSubCount; count2++)
								{
									pCurve->curveData[count2].from_Value = *pUnit++;
									pCurve->curveData[count2].to_Scalar = (*pUnit << 8) + (pUnit[1] & 0xFF); pUnit += 2;
								}
								// there's one extra slot in the definition to allow for this PAST the end of the 8 possible entries
								pCurve->curveData[count2].from_Value = 127;
								pCurve->curveData[count2].to_Scalar = pCurve->curveData[count2].to_Scalar;
								break;
							case QuadChar('A','D','S','R'):
								unitSubCount = *pUnit;		// how many unit records?
								pUnit++;					// byte
								if (unitSubCount > ADSR_STAGES)
								{	// can't have more than ADSR_STAGES stages
									goto bailoninstrument;
								}
								pADSR = &(theI->volumeADSRRecord);
								for (count2 = 0; count2 < unitSubCount; count2++)
								{
									pADSR->ADSRLevel[count2] = XGetLong(pUnit);
									pUnit += 4;

									pADSR->ADSRTime[count2] = XGetLong(pUnit) / 1000;	// new: convert to milliseconds
									pUnit += 4;

									pADSR->ADSRFlags[count2] = ConvertADSRFlags(XGetLong(pUnit));
									pUnit += 4;
								}
								break;

							case QuadChar('L','P','G','F'):		// low pass global filter parameters
								theI->LPF_frequency = XGetLong(pUnit);
								pUnit += 4;
								theI->LPF_resonance = XGetLong(pUnit);
								pUnit += 4;
								theI->LPF_lowpassAmount = XGetLong(pUnit);
								pUnit += 4;
								break;
								
							case QuadChar('D','M','O','D'):		// default mod wheel hookup
								disableModWheel = 1;
								break;									

							// LFO types
							case PITCH_LFO:
							case VOLUME_LFO:
							case STEREO_PAN_LFO:
							case STEREO_PAN_NAME2:
							case QuadChar('L','P','A','M'):
							case QuadChar('L','P','R','E'):
							case LPF_FREQUENCY:
								if (lfoCount > 4)
									goto bailoninstrument;
								//DebugStr("\p parsing an LFO unit");
								unitSubCount = *pUnit;		// how many unit records?
								pUnit++;					// byte
								if (unitSubCount > ADSR_STAGES)
								{	// can't have more than ADSR_STAGES stages
									//DebugStr("\p too many stages");
									goto bailoninstrument;
								}
								pLFO = &(theI->LFORecord[lfoCount]);
								for (count2 = 0; count2 < unitSubCount; count2++)
								{
									pLFO->a.ADSRLevel[count2] = XGetLong(pUnit);
									pUnit += 4;
									pLFO->a.ADSRTime[count2] = XGetLong(pUnit) / 1000; // New: convert to milliseconds
									pUnit += 4;
									pLFO->a.ADSRFlags[count2] = ConvertADSRFlags(XGetLong(pUnit));
									pUnit += 4;
								}
								pLFO->where_to_feed = unitType & 0x5F5F5F5F;

								pLFO->period = XGetLong(pUnit);
								pUnit += 4;
								pLFO->waveShape = XGetLong(pUnit);
								pUnit += 4;
								pLFO->DC_feed = XGetLong(pUnit);
								pUnit += 4;
								pLFO->level = XGetLong(pUnit);
								pUnit += 4;

#if 1
								pLFO->currentWaveValue = 0;
								pLFO->currentTime = 0;
								pLFO->LFOcurrentTime = 0;
								pLFO->a.currentTime = 0;
								pLFO->a.currentPosition = 0;
								pLFO->a.currentLevel = 0;
								pLFO->a.previousTarget = 0;
								pLFO->a.mode = 0;
								pLFO->a.sustainingDecayLevel = 65536;
#endif

								lfoCount++;
								break;
						}
					}
#ifdef VERBOSE
					Message(("%d LFO units", lfoCount));
#endif
					if ((lfoCount < 4) && (theI->curveRecordCount < 4) && (disableModWheel == 0))
					{
#if 1
						pCurve = &theI->curve[theI->curveRecordCount];
						theI->curveRecordCount++;
						
						// Create a straight-line curve function to tie mod wheel to pitch LFO
						pCurve->tieFrom = QuadChar('M','O','D','W');
						pCurve->tieTo = PITCH_LFO;
						pCurve->curveCount = 2;
						pCurve->curveData[0].from_Value = 0;
						pCurve->curveData[0].to_Scalar = 0;
						pCurve->curveData[1].from_Value = 127;
						pCurve->curveData[1].to_Scalar = 256;
						pCurve->curveData[2].from_Value = 127;
						pCurve->curveData[2].to_Scalar = 256;

						// Create a default pitch LFO unit to tie the MOD wheel to.
						pLFO = &theI->LFORecord[lfoCount];
						pLFO->where_to_feed = PITCH_LFO;
						pLFO->period = 180000;
						pLFO->waveShape = SINE_WAVE;
						pLFO->DC_feed = 0;
						pLFO->level = 64;
						pLFO->currentWaveValue = 0;
						pLFO->currentTime = 0;
						pLFO->LFOcurrentTime = 0;
						pLFO->a.ADSRLevel[0] = 65536;
						pLFO->a.ADSRTime[0] = 0;
						pLFO->a.ADSRFlags[0] = ADSR_TERMINATE;
						pLFO->a.currentTime = 0;
						pLFO->a.currentPosition = 0;
						pLFO->a.currentLevel = 0;
						pLFO->a.previousTarget = 0;
						pLFO->a.mode = 0;
						pLFO->a.sustainingDecayLevel = 65536;
						lfoCount++;
#endif
					}

					theI->LFORecordCount = lfoCount;
bailoninstrument:
					;
				}
			}
		}
	}
}

// Create instrument from 'snd' resource ID
static GM_Instrument * PV_CreateInstrumentFromResource(GM_Instrument *theMaster, short theID)
{
	GM_Instrument		*theI;
	UBYTE			*theSound;
	CacheSampleInfo	sndInfo;

	theI = NULL;
	theSound = (UBYTE *)PV_GetSampleFromID(theID, &sndInfo);
	if (theSound)
	{
		theI = (GM_Instrument *)XNewPtr((long)sizeof(GM_Instrument));
		if (theI)
		{
			theI->u.w.theWaveform = (UBYTE *)theSound;

			if (theMaster)
			{
				theI->enableInterpolate = theMaster->enableInterpolate;
				theI->disableSndLooping = theMaster->disableSndLooping;
				theI->neverInterpolate = theMaster->neverInterpolate;
				theI->playAtSampledFreq = theMaster->playAtSampledFreq;
				theI->doKeymapSplit = FALSE;
				theI->notPolyphonic = theMaster->notPolyphonic;
				theI->enablePitchRandomness = theMaster->enablePitchRandomness;
				theI->avoidReverb = theMaster->avoidReverb;
				theI->useSampleRate = theMaster->useSampleRate;
				theI->sampleAndHold = theMaster->sampleAndHold;
				theI->reserved_1 = theMaster->reserved_1;
			}
			else
			{
				theI->enableInterpolate = FALSE;
				theI->disableSndLooping = FALSE;
				theI->neverInterpolate = FALSE;
				theI->playAtSampledFreq = FALSE;
				theI->doKeymapSplit = FALSE;
				theI->notPolyphonic = FALSE;
				theI->enablePitchRandomness = FALSE;
				theI->avoidReverb = FALSE;
				theI->useSampleRate = FALSE;
				theI->sampleAndHold = FALSE;
			}
			theI->u.w.bitSize = sndInfo.bitSize;
			theI->u.w.channels = sndInfo.channels;
			theI->u.w.waveformID = sndInfo.theID;
			theI->u.w.waveSize = sndInfo.waveSize;
			theI->u.w.waveFrames = sndInfo.waveFrames;
			theI->u.w.startLoop = sndInfo.loopStart;
			theI->u.w.endLoop = sndInfo.loopEnd;
			theI->u.w.baseMidiPitch = sndInfo.baseKey;
			theI->u.w.sampledRate = sndInfo.rate;
			theI->u.w.noteDecayPref = NOTE_DEFAULT_DECAY;
		}
	}
	return theI;
}

// Instruments 0 to MAX_INSTRUMENTS*MAX_BANKS are the standard MIDI instrument placements.
// This will create an internal instrument from and external instrument. If theX is non-null then it
// will use that data to create the GM_Instrument
GM_Instrument * PV_GetInstrument(short theID, void *theExternalX, long patchSize)
{
	GM_Instrument		*theI, *theS;
	InstrumentResource	*theX;
	long				size;
	short int 			count;
	UBYTE			*theSound;
	KeySplit			theXSplit;
	CacheSampleInfo	sndInfo;
	LOOPCOUNT		i;

	theI = NULL;
	theX = (InstrumentResource *) theExternalX;
	if (theExternalX == NULL)
	{
		theX = (InstrumentResource *)XGetAndDetachResource(ID_INST, theID, &patchSize);
	}
	if (theX)
	{
		if (XGetShort(&theX->keySplitCount) < 2)	// if its 1, then it has no splits
		{
			theSound = (UBYTE *)PV_GetSampleFromID(XGetShort(&theX->sndResourceID), &sndInfo);
			if (theSound)
			{
				theI = (GM_Instrument *)XNewPtr((long)sizeof(GM_Instrument));
				if (theI)
				{
#ifdef VERBOSE
					Message(("Loading instrument %d", theID));
#endif
					theI->u.w.theWaveform = (UBYTE *)theSound;

					theI->enableInterpolate = TEST_FLAG_VALUE(theX->flags1, ZBF_enableInterpolate);
					theI->disableSndLooping = TEST_FLAG_VALUE(theX->flags1, ZBF_disableSndLooping);
					theI->neverInterpolate = TEST_FLAG_VALUE(theX->flags2, ZBF_neverInterpolate);
					theI->playAtSampledFreq = TEST_FLAG_VALUE(theX->flags2, ZBF_playAtSampledFreq);
					theI->doKeymapSplit = FALSE;
					theI->notPolyphonic = TEST_FLAG_VALUE(theX->flags2, ZBF_notPolyphonic);
					theI->enablePitchRandomness = TEST_FLAG_VALUE(theX->flags2, ZBF_enablePitchRandomness);
					theI->avoidReverb = TEST_FLAG_VALUE(theX->flags1, ZBF_avoidReverb);
					theI->useSampleRate = TEST_FLAG_VALUE(theX->flags1, ZBF_useSampleRate);
					theI->sampleAndHold = TEST_FLAG_VALUE(theX->flags1, ZBF_sampleAndHold);
					theI->reserved_1 = TEST_FLAG_VALUE(theX->flags1, ZBF_reserved_1);

					PV_GetEnvelopeData(theX, theI, patchSize);		// get envelope

					theI->u.w.bitSize = sndInfo.bitSize;
					theI->u.w.channels = sndInfo.channels;
					theI->u.w.waveformID = XGetShort(&theX->sndResourceID);
					theI->u.w.waveSize = sndInfo.waveSize;
					theI->u.w.waveFrames = sndInfo.waveFrames;
					theI->u.w.startLoop = sndInfo.loopStart;
					theI->u.w.endLoop = sndInfo.loopEnd;

					theI->masterRootKey = XGetShort(&theX->midiRootKey);
					theI->panPlacement = theX->panPlacement;
					theI->u.w.baseMidiPitch = sndInfo.baseKey;
					theI->u.w.noteDecayPref = NOTE_DEFAULT_DECAY;		// NOTE: This needs to be defined
					theI->u.w.sampledRate = sndInfo.rate;

					theI->enableSoundModifier = TEST_FLAG_VALUE(theX->flags2, ZBF_enableSoundModifier);
					theI->smodResourceID = theX->smodResourceID;
					theI->smodParameter1 = XGetShort(&theX->smodParameter1);
					theI->smodParameter2 = XGetShort(&theX->smodParameter2);
					// Process sample in place
					if ( (theI->enableSoundModifier) && (theI->u.w.bitSize == 8) && (theI->u.w.channels == 1) )
					{
#if DISPLAY_INSTRUMENTS
						DPrint(drawDebug, "---->Processing instrument %ld with SMOD %ld\r", (long)theID, (long)theI->smodResourceID);
#endif
						PV_ProcessSampleWithSMOD(theI->u.w.theWaveform,
												theI->u.w.waveSize,
												theI->u.w.waveformID,
												theI->smodResourceID,
												theI->smodParameter1,
												theI->smodParameter2);
					}
				}
			}
		}
		else
		{	// Keysplits
			TrivialMessage(("----->Processing %d keysplits", (long)XGetShort(&theX->keySplitCount)));
			size = XGetShort(&theX->keySplitCount) * sizeof(KeymapSplit);
			size += sizeof(KeymapSplitInfo);

			theI = (GM_Instrument *)XNewPtr(size + sizeof(GM_Instrument));
			if (theI)
			{
				theI->enableInterpolate = TEST_FLAG_VALUE(theX->flags1, ZBF_enableInterpolate);
				theI->disableSndLooping = TEST_FLAG_VALUE(theX->flags1, ZBF_disableSndLooping);
				theI->neverInterpolate = TEST_FLAG_VALUE(theX->flags2, ZBF_neverInterpolate);
				theI->doKeymapSplit = TRUE;
				theI->notPolyphonic = TEST_FLAG_VALUE(theX->flags2, ZBF_notPolyphonic);
				theI->enablePitchRandomness = TEST_FLAG_VALUE(theX->flags2, ZBF_enablePitchRandomness);
				theI->avoidReverb = TEST_FLAG_VALUE(theX->flags1, ZBF_avoidReverb);
				theI->useSampleRate = TEST_FLAG_VALUE(theX->flags1, ZBF_useSampleRate);
				theI->sampleAndHold = TEST_FLAG_VALUE(theX->flags1, ZBF_sampleAndHold);
				theI->playAtSampledFreq = TEST_FLAG_VALUE(theX->flags2, ZBF_playAtSampledFreq);
				theI->reserved_1 = TEST_FLAG_VALUE(theX->flags1, ZBF_reserved_1);

				PV_GetEnvelopeData(theX, theI, patchSize);		// get envelope

				theI->u.k.KeymapSplitCount = XGetShort(&theX->keySplitCount);
				theI->u.k.defaultInstrumentID = XGetShort(&theX->sndResourceID);

				theI->masterRootKey = XGetShort(&theX->midiRootKey);
				theI->panPlacement = theX->panPlacement;
				theI->enableSoundModifier = TEST_FLAG_VALUE(theX->flags2, ZBF_enableSoundModifier);
				theI->smodResourceID = theX->smodResourceID;
				theI->smodParameter1 = XGetShort(&theX->smodParameter1);
				theI->smodParameter2 = XGetShort(&theX->smodParameter2);

				for (count = 0; count < theI->u.k.KeymapSplitCount; count++)
				{
					GetKeySplitFromPtr(theX, count, &theXSplit);
					theI->u.k.keySplits[count].lowMidi = theXSplit.lowMidi;
					theI->u.k.keySplits[count].highMidi = theXSplit.highMidi;
					theI->u.k.keySplits[count].smodParameter1 = theXSplit.smodParameter1;
					theI->u.k.keySplits[count].smodParameter2 = theXSplit.smodParameter2;

					if (GM_IsInstrumentRangeUsed(theID, theXSplit.lowMidi, theXSplit.highMidi))
					{
#if DISPLAY_INSTRUMENTS
						DPrint(drawDebug, "------->Keysplit %ld low %ld high %ld\r", (long)count, 
																	(long)theXSplit.lowMidi, 
																	(long)theXSplit.highMidi);
#endif
						theS =  PV_CreateInstrumentFromResource(theI, theXSplit.sndResourceID);
						theI->u.k.keySplits[count].pSplitInstrument = theS;
						if (theS)
						{
							theS->masterRootKey = theI->masterRootKey;
							theS->avoidReverb = theI->avoidReverb;
							theS->volumeADSRRecord = theI->volumeADSRRecord;
							for (i = 0; i < theI->LFORecordCount; i++)
							{
								theS->LFORecord[i] = theI->LFORecord[i];
							}
							theS->LFORecordCount = theI->LFORecordCount;
							for (i = 0; i < theI->curveRecordCount; i++)
							{
								theS->curve[i] = theI->curve[i];
							}
							theS->curveRecordCount = theI->curveRecordCount;
							theS->LPF_frequency = theI->LPF_frequency;
							theS->LPF_resonance = theI->LPF_resonance;
							theS->LPF_lowpassAmount = theI->LPF_lowpassAmount;
							theS->u.w.noteDecayPref = NOTE_DEFAULT_DECAY;		// NOTE: This needs to be defined
							// Process sample in place
							if ( (theS->enableSoundModifier) && (theS->u.w.bitSize == 8) && (theS->u.w.channels == 1) )
							{
#if DISPLAY_INSTRUMENTS
								DPrint(drawDebug, "----->Processing instrument %ld with SMOD %ld\r", (long)theID, (long)theI->smodResourceID);
#endif
								PV_ProcessSampleWithSMOD(	theS->u.w.theWaveform,
														theS->u.w.waveSize,
														theXSplit.sndResourceID,
														theS->smodResourceID,
														theS->smodParameter1,
														theS->smodParameter2);
							}
						}
					}
				}
			}
		}
		if (theExternalX == NULL)
		{
			XDisposePtr((XPTR)theX);
		}
	}
#if DISPLAY_INSTRUMENTS
	if (theI)
	{
		DPrint(drawDebug, "-------->INST info: masterRootKey %ld\r", (long)theI->masterRootKey);
	}
#endif
	return theI;
}

// This will remap the 'from' instrument into the 'to' instrument.
OPErr GM_RemapInstrument(INT32 from, INT32 to)
{
	OPErr			theErr;

	theErr = BAD_INSTRUMENT;
	if ( (from >= 0) && (from < MAX_INSTRUMENTS*MAX_BANKS) )
	{
		if ( (to >= 0) && (to < MAX_INSTRUMENTS*MAX_BANKS) )
		{
			if (to != from)
			{
				if (MusicGlobals->InstrumentData[from])
				{
					MusicGlobals->remapArray[to] = from;
					theErr = NO_ERR;
				}
			}
			else
			{
				theErr = NO_ERR;
			}
		}
	}
	return theErr;
}

BOOL_FLAG PV_AnyStereoInstrumentsLoaded(void)
{
	register GM_Instrument	*theI;
	register short int		instrument;
	BOOL_FLAG					stereoLoaded;

	stereoLoaded = FALSE;
	if (MusicGlobals->InstrumentData)
	{
		for (instrument = 0; instrument < (MAX_INSTRUMENTS*MAX_BANKS); instrument++)
		{
			theI = MusicGlobals->InstrumentData[instrument];
			if (theI)
			{
				if (theI->doKeymapSplit == FALSE)	// only look at wave data
				{
					if (theI->u.w.channels > 1)
					{
						stereoLoaded = TRUE;
						break;
					}
				}
			}
		}
	}
	return stereoLoaded;
}

// Given an instrument number from 0 to MAX_INSTRUMENTS*MAX_BANKS, this will load that instrument into the musicvars globals, including
// splits
OPErr GM_LoadInstrument(INT32 instrument)
{
	register GM_Instrument	*theI;
	register OPErr			theErr;

	theErr = MEMORY_ERR;
	if ( (instrument >= 0) && (instrument < (MAX_INSTRUMENTS*MAX_BANKS)) )
	{
		if (MusicGlobals->InstrumentData)
		{
			theErr = NO_ERR;

			theI = MusicGlobals->InstrumentData[instrument];
			if (MusicGlobals->cacheInstruments == FALSE)
			{
				if (theI == NULL)
				{	// an instrument is not there, so load it
					theI = PV_GetInstrument(instrument, NULL, 0);
				}
			}
			else
			{	// use cached instrument, if its not there, then load it
				if (theI == NULL)
				{
					theI = PV_GetInstrument(instrument, NULL, 0);
				}
			}

			if (theI)
			{
				theI->usageReferenceCount++;		// increment reference count
				MusicGlobals->InstrumentData[instrument] = theI;
				MusicGlobals->remapArray[instrument] = instrument;
			}
			else
			{
				theErr = BAD_INSTRUMENT;
			}
		}
	}
	else
	{
		theErr = PARAM_ERR;
	}
	return theErr;
}

OPErr GM_UnloadInstrument(INT32 instrument)
{
	register GM_Instrument	*	theI;
	register KeymapSplit FAR *	k;
	register OPErr				theErr;
	register short int			splitCount;

	if ( (instrument >= 0) && (instrument < (MAX_INSTRUMENTS*MAX_BANKS)) )
	{
		if (MusicGlobals->InstrumentData)
		{
			theErr = NO_ERR;
			if (MusicGlobals->cacheInstruments == FALSE)
			{
				theI = MusicGlobals->InstrumentData[instrument];
				if (theI)
				{
					theI->usageReferenceCount--;		// decrement reference count
					if (theI->usageReferenceCount == 0)
					{
						if (theI->doKeymapSplit)
						{
							k = theI->u.k.keySplits;
							for (splitCount = 0; splitCount < theI->u.k.KeymapSplitCount; splitCount++)
							{
								if (k->pSplitInstrument)
								{
									if (k->pSplitInstrument->u.w.theWaveform)
									{
										PV_FreeCacheEntryFromPtr(k->pSplitInstrument->u.w.theWaveform);
									}
									XDisposePtr(k->pSplitInstrument);
								}
								k++;
							}
						}
						else
						{
							if (theI->u.w.theWaveform)
							{
								PV_FreeCacheEntryFromPtr(theI->u.w.theWaveform);
							}
						}
						XDisposePtr((void FAR *)theI);
						MusicGlobals->InstrumentData[instrument] = NULL;
					}
				}
			}
			else
			{
				theErr = BAD_INSTRUMENT;
			}
		}
	}		
	else
	{
		theErr = PARAM_ERR;
	}
	return theErr;
}

void GM_FlushInstrumentCache(BOOL_FLAG startStopCache)
{
	register short int count;

	if (MusicGlobals)
	{
		MusicGlobals->cacheSamples = startStopCache;
		MusicGlobals->cacheInstruments = startStopCache;
		if (startStopCache == FALSE)		// clear cache
		{
			GM_EndSong(NULL);			// stop all songs
			for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
			{
				GM_UnloadInstrument(count);
			}
			for (count = 0; count < MAX_SAMPLES; count++)
			{
				if (MusicGlobals->sampleCaches[count])
				{
					PV_FreeCacheEntry(MusicGlobals->sampleCaches[count]);
					MusicGlobals->sampleCaches[count] = NULL;
				}
			}
		}
	}
}


// Scan the midi file and determine which instrument that need to be loaded and load them.
OPErr GM_LoadSongInstruments(GM_Song *theSong, short int *pArray, short int loadInstruments)
{
	register long		count, loadCount, instCount, newCount;
	BOOL_FLAG		loopSongSave;
	OPErr			theErr;
	BYTE				remapUsedSaved[MAX_INSTRUMENTS];
	BYTE				remapUsed[MAX_INSTRUMENTS];

	int	howManyLoaded = 0;
	// Set the sequencer to mark instruments only
	theErr = NO_ERR;

// Up to HERE, we were OK.

	MusicGlobals->pUsedPatchList = (BYTE *)XNewPtr((MAX_INSTRUMENTS*MAX_BANKS*128L) / 8);
	if (MusicGlobals->pUsedPatchList)
	{
		for (count = 0; count < MAX_INSTRUMENTS*MAX_BANKS; count++)
		{
			MusicGlobals->remapArray[count] = count;
			if (pArray)
			{
				pArray[count] = -1;
			}
		}
		for (count = 0; count < MAX_CHANNELS; count++)
		{
			theSong->firstChannelBank[count] = 0;
			theSong->firstChannelProgram[count] = -1;
		}


		if (PV_ConfigureMusic(theSong) == NO_ERR)
		{
			if (theSong->defaultPercusionProgram == -1)
			{
				theSong->channelBank[PERCUSSION_CHANNEL] = 0;
				theSong->firstChannelBank[PERCUSSION_CHANNEL] = 0;
			}
			else
			{
				if (theSong->defaultPercusionProgram)
				{
					theSong->firstChannelProgram[PERCUSSION_CHANNEL] = theSong->defaultPercusionProgram;
					GM_SetUsedInstrument(theSong->defaultPercusionProgram, 60, TRUE);
				}
			}

			theSong->AnalyzeMode = SCAN_SAVE_PATCHES;
			theSong->SomeTrackIsAlive = TRUE;

			loopSongSave = theSong->loopSong;
			theSong->loopSong = FALSE;

#ifdef VERBOSE
			Message(("Parsing song for insts"));
#endif
			while (theSong->SomeTrackIsAlive)
			{
				PV_MusicIRQ(theSong);
			}
#ifdef VERBOSE
			Message(("Done parsing song for insts"));
#endif
			theSong->AnalyzeMode = SCAN_NORMAL;
			theSong->loopSong = loopSongSave;
		
#if DISPLAY_INSTRUMENTS
			DPrint(drawDebug, "Loading instruments:\r");
#endif
			instCount = 0;
			for (count = 0; count < MAX_INSTRUMENTS*MAX_BANKS; count++)
			{
				// instrument needs to be loaded
				if (GM_IsInstrumentUsed(count, -1))
				{
#if DISPLAY_INSTRUMENTS
					DPrint(drawDebug, "Instrument %ld: ", (long)count);
#endif
					loadCount = theSong->instrumentRemap[count];
					if (loadCount == -1)
					{
						loadCount = count;
					}
#if DISPLAY_INSTRUMENTS
					else
					{
						DPrint(drawDebug, "remapped to %ld ", (long)loadCount);
					}
#endif
#if DISPLAY_INSTRUMENTS
					DPrint(drawDebug, "loading instrument %ld\r", loadCount);
#endif
					if (pArray)
					{
						pArray[instCount++] = loadCount;
					}

					if (loadInstruments)
					{
						if (loadCount != count)
						{
							GM_GetInstrumentUsedRange(loadCount, remapUsedSaved);		// save
							GM_GetInstrumentUsedRange(count, remapUsed);
							GM_SetInstrumentUsedRange(loadCount, remapUsed);
						}
						theErr = GM_LoadInstrument(loadCount);
						theSong->instrumentData[loadCount] = MusicGlobals->InstrumentData[loadCount];
						if (theErr != NO_ERR)
						{	// if the instrument is some other bank, then go back to the standard GM bank before failing
							if (loadCount > MAX_INSTRUMENTS)
							{
#if DISPLAY_INSTRUMENTS
								DPrint(drawDebug, "Failed loading extra bank instrument %ld, falling back to GM.\r", loadCount);
#endif
								newCount = (loadCount % MAX_INSTRUMENTS);
								newCount += ((loadCount / MAX_INSTRUMENTS) & 1) * MAX_INSTRUMENTS;
								loadCount = newCount;
								
#if DISPLAY_INSTRUMENTS
								DPrint(drawDebug, "Loading instrument %ld\r", loadCount);
#endif
								theErr = GM_LoadInstrument(loadCount);
								theSong->instrumentData[loadCount] = MusicGlobals->InstrumentData[loadCount];
							}
						}
						if (loadCount != count)
						{
							GM_SetInstrumentUsedRange(loadCount, remapUsedSaved);		// save
						}
						if (theErr)
						{
#if DISPLAY_INSTRUMENTS
							DPrint(drawDebug, "Failed to load instrument %ld (%ld)\r", (long)loadCount, (long)theErr);
#endif
							break;
						}
						theErr = GM_RemapInstrument(loadCount, count);	// remap from: to
					}
				}
			}
		}	
	
		if (theErr != NO_ERR)
		{
			GM_UnloadSongInstruments(theSong);
		}
		XDisposePtr(MusicGlobals->pUsedPatchList);
		MusicGlobals->pUsedPatchList = NULL;
	}
	else
	{
		theErr = MEMORY_ERR;
	}
#if DISPLAY_INSTRUMENTS
	DPrint(drawDebug, "\rClick to exit");
	while (Button() == FALSE) {};
	while (Button()) {};
	FlushEvents(mDownMask, 0);
	DCopy(drawDebug);
	DDispose(drawDebug);
#endif

	return theErr;
}

OPErr GM_UnloadSongInstruments(GM_Song *theSong)
{
	short int			count;

	if (theSong)
	{
		for (count = 0; count < (MAX_INSTRUMENTS*MAX_BANKS); count++)
		{
			if (theSong->instrumentData[count])
			{
				GM_UnloadInstrument(count);
				theSong->instrumentData[count] = NULL;		// redundant, but clear
			}
		}
	}
	return NO_ERR;
}

// Set the patch & key used bit. Pass -1 in theKey to set all the keys in that patch
void GM_SetUsedInstrument(INT16 thePatch, INT16 theKey, BOOL_FLAG used)
{
	long	bit, count;

	if (MusicGlobals && MusicGlobals->pUsedPatchList)
	{
		if (theKey != -1)
		{
			bit = ((long)thePatch * 128L) + (long)theKey;
//			if (bit < (MAX_INSTRUMENTS*MAX_BANKS*128L))
			{
				if (used)
				{
					XSetBit((void *)MusicGlobals->pUsedPatchList, bit);
				}
				else
				{
					XClearBit((void *)MusicGlobals->pUsedPatchList, bit);
				}
			}
		}
		else
		{
			for (count = 0; count < MAX_INSTRUMENTS; count++)
			{
				bit = ((long)thePatch * 128L) + count;
//				if (bit < (MAX_INSTRUMENTS*MAX_BANKS*128L))
				{
					if (used)
					{
						XSetBit((void *)MusicGlobals->pUsedPatchList, bit);
					}
					else
					{
						XClearBit((void *)MusicGlobals->pUsedPatchList, bit);
					}
				}
			}
		}
	}
}

BOOL_FLAG GM_IsInstrumentUsed(INT16 thePatch, INT16 theKey)
{
	register long		bit, count;
	register BOOL_FLAG	used;

	used = FALSE;
	if (MusicGlobals && MusicGlobals->pUsedPatchList)
	{
		if (theKey != -1)
		{
			bit = ((long)thePatch * 128L) + (long)theKey;
//			if (bit < (MAX_INSTRUMENTS*MAX_BANKS*128L))
			{
				used = XTestBit((void *)MusicGlobals->pUsedPatchList, bit);
			}
		}
		else
		{
			for (count = 0; count < MAX_INSTRUMENTS; count++)
			{
				bit = ((long)thePatch * 128L) + count;
//				if (bit < (MAX_INSTRUMENTS*MAX_BANKS*128L))
				{
					used = XTestBit((void *)MusicGlobals->pUsedPatchList, bit);
					if (used)
					{
						break;
					}
				}
			}
		}
	}
	else
	{
		used = TRUE;
	}
	return used;
}

void GM_GetInstrumentUsedRange(INT16 thePatch, BYTE *pUsedArray)
{
	register long		bit, count;

	for (count = 0; count < MAX_INSTRUMENTS; count++)
	{
		bit = ((long)thePatch * 128L) + count;
		pUsedArray[count] = XTestBit((void *)MusicGlobals->pUsedPatchList, bit);
	}
}

// #ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GM_SetInstrumentUsedRange(INT16 thePatch, BYTE *pUsedArray)
{
	register long	count;

	for (count = 0; count < MAX_INSTRUMENTS; count++)
	{
		GM_SetUsedInstrument(thePatch, count, pUsedArray[count]);
	}
}
// #endif

BOOL_FLAG GM_IsInstrumentRangeUsed(INT16 thePatch, INT16 theLowKey, INT16 theHighKey)
{
	register BOOL_FLAG	used;
	register long		count;

	used = FALSE;
	for (count = theLowKey; count <= theHighKey; count++)
	{
		used = GM_IsInstrumentUsed(thePatch, count);
		if (used)
		{
			break;
		}
	}
	return used;
}


// EOF of GenPatch.c
