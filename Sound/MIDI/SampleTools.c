/*****************************************************/
/*
**	SampleTools.c
**
**		Tools for manipulating 'snd' resources.
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**
**
**	History	-
**	1/31/93		Created
**	6/23/95		Integrated into SoundMusicSys Library
**	11/22/95	Added MACE support for CreateSampleFromSnd
**	12/15/95	Changed memory allocation to X_API
**				Pulled out most Mac specific code
**	1/18/96		Spruced up for C++ extra error checking
**	2/3/96		Removed extra includes
**	2/11/96		No longer relies on MacOS Sound.h
**	2/12/96		Changed Boolean to SMSBoolean
**	2/17/96		Added more platform compile arounds
**	3/28/96		Added XPutShort & XPutLong
**	3/29/96		Tried to figure out ima 4 to 1 and failed
**	4/2/96		Added more snd tools to set loop points, rates, and basekey
**	4/9/96		Changed AIFFSampleRate from extended80 to char [10]
**	5/12/96		Fixed long alignment problem with XGetSamplePtrFromSnd and missing
**				XGetLong's
**
*/
/*****************************************************/
#include "X_API.h"
#include "limits.h"

#if X_HARDWARE_PLATFORM == X_MACINTOSH
	#include <Types.h>
	#include <Resources.h>
	#include <Memory.h>
//	#include <Sound.h>

	#include "MacSpecificSMS.h"

	#include "SoundMusicSystem.h"
#endif

#include "PrivateSoundMusicSystem.h"

#ifdef powerc
	#pragma options align=mac68k
#endif

// These are included here, because we want to be independent of MacOS, but use this standard format
#ifndef __SOUND__
enum 
{
	notCompressed				= 0,							/*compression ID's*/
	fixedCompression			= -1,						/*compression ID for fixed-sized compression*/
	variableCompression			= -2,						/*compression ID for variable-sized compression*/
	twoToOne					= 1,
	eightToThree				= 2,
	threeToOne				= 3,
	sixToOne					= 4,

	stdSH					= 0x00,							/*Standard sound header encode value*/
	extSH					= 0xFF,							/*Extended sound header encode value*/
	cmpSH					= 0xFE							/*Compressed sound header encode value*/
};


enum
{
	soundListRsrc				= QuadChar('s','n','d',' '),				/*Resource type used by Sound Manager*/
	rate44khz					= 0xAC440000L,					/*44100.00000 in fixed-point*/
	rate22050hz				= 0x56220000L,					/*22050.00000 in fixed-point*/
	rate22khz					= 0x56EE8BA3L,					/*22254.54545 in fixed-point*/
	rate11khz					= 0x2B7745D1L,					/*11127.27273 in fixed-point*/
	rate11025hz				= 0x2B110000,					/*11025.00000 in fixed-point*/

	kMiddleC					= 60,							/*MIDI note value for middle C*/

	soundCmd					= 80,
	bufferCmd					= 81,
	firstSoundFormat			= 0x0001,						/*general sound format*/
	secondSoundFormat			= 0x0002						/*special sampled sound format (HyperCard)*/
};
#endif

struct XSoundHeader 
{
	char *				samplePtr;					/*if NIL then samples are in sampleArea*/
	unsigned long			length;						/*length of sound in bytes*/
	unsigned long			sampleRate;					/*sample rate for this sound*/
	unsigned long			loopStart;					/*start of looping portion*/
	unsigned long			loopEnd;					/*end of looping portion*/
	unsigned char			encode;						/*header encoding*/
	unsigned char			baseFrequency;				/*baseFrequency value*/
	unsigned char			sampleArea[1];				/*space for when samples follow directly*/
};
typedef struct XSoundHeader XSoundHeader;

typedef XSoundHeader *XSoundHeaderPtr;

struct XCmpSoundHeader 
{
	char *				samplePtr;					/*if nil then samples are in sample area*/
	unsigned long			numChannels;				/*number of channels i.e. mono = 1*/
	unsigned long			sampleRate;					/*sample rate in Apples Fixed point representation*/
	unsigned long			loopStart;					/*loopStart of sound before compression*/
	unsigned long			loopEnd;					/*loopEnd of sound before compression*/
	unsigned char			encode;						/*data structure used , stdSH, extSH, or cmpSH*/
	unsigned char			baseFrequency;				/*same meaning as regular SoundHeader*/
	unsigned long			numFrames;					/*length in frames ( packetFrames or sampleFrames )*/
	char					AIFFSampleRate[10];				/*IEEE sample rate*/
	char *				markerChunk;				/*sync track*/
	long					format;						/*data format type, was futureUse1*/
	unsigned long			futureUse2;					/*reserved by Apple*/
	void *				stateVars;					/*pointer to State Block*/
	void *				leftOverSamples;			/*used to save truncated samples between compression calls*/
	short				compressionID;				/*0 means no compression, non zero means compressionID*/
	unsigned short			packetSize;					/*number of bits in compressed sample packet*/
	unsigned short			snthID;						/*resource ID of Sound Manager snth that contains NRT C/E*/
	unsigned short			sampleSize;					/*number of bits in non-compressed sample*/
	unsigned char			sampleArea[1];				/*space for when samples follow directly*/
};
typedef struct XCmpSoundHeader XCmpSoundHeader;

typedef XCmpSoundHeader * XCmpSoundHeaderPtr;

struct XExtSoundHeader 
{
	char *				samplePtr;					/*if nil then samples are in sample area*/
	unsigned long			numChannels;				/*number of channels,  ie mono = 1*/
	unsigned long			sampleRate;					/*sample rate in Apples Fixed point representation*/
	unsigned long			loopStart;					/*same meaning as regular SoundHeader*/
	unsigned long			loopEnd;					/*same meaning as regular SoundHeader*/
	unsigned char			encode;						/*data structure used , stdSH, extSH, or cmpSH*/
	unsigned char			baseFrequency;				/*same meaning as regular SoundHeader*/
	unsigned long			numFrames;					/*length in total number of frames*/
	char					AIFFSampleRate[10];				/*IEEE sample rate*/
	char *				markerChunk;				/*sync track*/
	char *				instrumentChunks;			/*AIFF instrument chunks*/
	char *				AESRecording;
	unsigned short			sampleSize;					/*number of bits in sample*/
	unsigned short			futureUse1;					/*reserved by Apple*/
	unsigned long			futureUse2;					/*reserved by Apple*/
	unsigned long			futureUse3;					/*reserved by Apple*/
	unsigned long			futureUse4;					/*reserved by Apple*/
	unsigned char			sampleArea[1];				/*space for when samples follow directly*/
};
typedef struct XExtSoundHeader XExtSoundHeader;

typedef XExtSoundHeader *XExtSoundHeaderPtr;

struct SoundFormat1
{
	short		type;
	short		numModifiers;
	unsigned short	modNumber;
	long			modInit;
	short		numCommands;
// first command
	unsigned short	cmd;
	short		param1;
	long			param2;
};
typedef struct SoundFormat1 SoundFormat1;

struct SndHeader1
{
	SoundFormat1	sndHeader;
	XSoundHeader	sndBuffer;
};
typedef struct SndHeader1 SndHeader1;


struct SoundFormat2
{
	short int		type;
	short int		refCount;
	short int		numCmds;
// first command
	unsigned short	cmd;
	short		param1;
	long			param2;
};
typedef struct SoundFormat2 SoundFormat2;

struct SndHeader2
{
	SoundFormat2	sndHeader;
	XSoundHeader	sndBuffer;
};
typedef struct SndHeader2 SndHeader2;


#ifdef powerc
	#pragma options align=reset
#endif



// Given a Mac snd pointer, this will return the encoding type, and a pointer to a SoundHeader structure
static void * PV_GetSoundHeaderPtr(XPTR pRes, short int *pEncode)
{
	XSoundHeader	*pSndBuffer;
	short int		soundFormat;
	short int		numSynths, numCmds;
	long			offset;
	char			*pSndFormat;

	*pEncode = -1;
	pSndBuffer = NULL;
	if (pRes)
	{
		pSndFormat = (char *)pRes;
		soundFormat = (short int)XGetShort(pSndFormat);
		switch (soundFormat)
		{
			case firstSoundFormat:
				// look inside the format 1 resource and decode offsets
				numSynths = (short int)XGetShort((short int *)pSndFormat + 1);		// get number of synths
				numCmds = (short int )XGetShort(pSndFormat + 4 + numSynths * 6);	// get number of commands
				break;
			case secondSoundFormat:
				numSynths = 0;		// format 2 has none
				numCmds = (short int)XGetShort((short int *)pSndFormat + 2);
				break;
			default:
				soundFormat = -1;
				break;
		}

		if (soundFormat != -1)	/* did we get the right format? */
		{
			/* compute address of sound header. 
			*/
			offset = 6 + 6 * numSynths + 8 * numCmds;
			pSndBuffer = (XSoundHeader *) ((char *)pRes + offset);
			*pEncode = pSndBuffer->encode & 0xFF;
		}
	}
	return pSndBuffer;
}



// This will return a pointer into a snd2 pointer block or create a new block if MACE compressed
XPTR XGetSamplePtrFromSnd(XPTR pRes, SampleDataInfo *pInfo)
{
	register XPTR				hSnd, newSound, monoSound;
	register XSoundHeader		*pSndBuffer;
	register XCmpSoundHeader 	*pCmpBuffer;
	register XExtSoundHeader		*pExtBuffer;
	long						offset, count;
	char						*pSampleData;
	char						*pLeft, *pRight;
	short int					encode;

	hSnd = NULL;
	pInfo->size = ULONG_MAX;		// if left alone, then wrong type of resource
	pInfo->frames = ULONG_MAX;
	pInfo->rate = rate22khz;
	pInfo->loopStart = 0;
	pInfo->loopEnd = 0;
	pInfo->baseKey = kMiddleC;
	pInfo->bitSize = 8;
	pInfo->channels = 1;

	pSndBuffer = (XSoundHeader *) PV_GetSoundHeaderPtr(pRes, &encode);
	if (pSndBuffer)	/* did we get the right format? */
	{
		/* compute address of sound header. 
		*/
		switch (encode)
		{
			case stdSH:	// standard header
				pSampleData = (char *)&pSndBuffer->sampleArea[0];
				pInfo->size = XGetLong(&pSndBuffer->length);
				pInfo->frames = pInfo->size;
				pInfo->loopStart = XGetLong(&pSndBuffer->loopStart);
				pInfo->loopEnd = XGetLong(&pSndBuffer->loopEnd);
				pInfo->baseKey = pSndBuffer->baseFrequency;
				pInfo->rate = XGetLong(&pSndBuffer->sampleRate);
				pInfo->channels = 1;
				pInfo->bitSize = 8;			// defaults for standard headers
				pInfo->pMasterPtr = pRes;
				break;

			case extSH:	// extended header
				pExtBuffer = (XExtSoundHeader *)pSndBuffer;
				pSampleData = (char *)&pExtBuffer->sampleArea[0];
				pInfo->channels = XGetLong(&pExtBuffer->numChannels);
				pInfo->bitSize = XGetShort(&pExtBuffer->sampleSize);
				pInfo->frames = XGetLong(&pExtBuffer->numFrames);
				pInfo->size = pInfo->frames * (pInfo->channels) * (pInfo->bitSize / 8);
				pInfo->loopStart = XGetLong(&pExtBuffer->loopStart);
				pInfo->loopEnd = XGetLong(&pExtBuffer->loopEnd);
				pInfo->baseKey = pExtBuffer->baseFrequency;
				pInfo->rate = XGetLong(&pExtBuffer->sampleRate);
				pInfo->pMasterPtr = pRes;
				break;
				
			case cmpSH:	// compressed header
				pCmpBuffer = (XCmpSoundHeader *)pSndBuffer;
				if ((pSampleData = pCmpBuffer->samplePtr) == NULL)	/* get ptr to sample data */
				{
					pSampleData = (char *) pCmpBuffer->sampleArea;
				}
				pInfo->channels = XGetLong(&pCmpBuffer->numChannels);
				pInfo->bitSize = XGetShort(&pCmpBuffer->sampleSize);
				pInfo->frames = XGetLong(&pCmpBuffer->numFrames);
				pInfo->loopStart = XGetLong(&pCmpBuffer->loopStart);
				pInfo->loopEnd = XGetLong(&pCmpBuffer->loopEnd);
				pInfo->baseKey = pCmpBuffer->baseFrequency;
				pInfo->rate = XGetLong(&pCmpBuffer->sampleRate);

				switch(pCmpBuffer->compressionID)
				{
					default:
						DEBUG_STR("\pInvalid compression ID");
						break;
	
					case fixedCompression:
						switch (pCmpBuffer->format)
						{
							case QuadChar('i','m','a','4'):	// IMA 4 : 1
	/*
							pInfo->size = pCmpBuffer->numFrames * (pInfo->channels) * (pInfo->bitSize / 8);
							newSound = XNewPtr(pInfo->size);
							pInfo->pMasterPtr = newSound;
							if (newSound)
							{
							}
							pSampleData = (char *)newSound;
	*/
								break;
						}
						break;
					case threeToOne:
						pInfo->size = pCmpBuffer->numFrames * (pInfo->channels) * (pInfo->bitSize / 8);
						pInfo->size *= 6;	// 2 bytes at 3:1 is 6 bytes for a packet, 1 byte at 6:1 is 6 bytes too
						newSound = XNewPtr(pInfo->size);
						pInfo->pMasterPtr = newSound;
						if (newSound)
						{
							if (pInfo->channels == 1)
							{
								XExpandMace1to3(pSampleData, newSound, pInfo->frames, NULL, NULL, 1, 1);
								pInfo->frames *= 6;			// adjust the frame count to equal the real frames
							}
							else
							{
								monoSound = XNewPtr(pInfo->size / 2);
								if (monoSound)
								{
									XExpandMace1to3(pSampleData, newSound, pInfo->frames, NULL, NULL, pInfo->channels, 1);
									XExpandMace1to3(pSampleData, monoSound, pInfo->frames, NULL, NULL, pInfo->channels, 2);
									pLeft = (char *)newSound;
									pRight = (char *)monoSound;
									pInfo->frames *= 6;			// adjust the frame count to equal the real frames
									offset = pInfo->frames - 1;
									// copy the data into a stereo sample block, copy backwards so that we don't have to create
									// two blocks of data
									for (count = offset; count >= 0; count--)
									{
										pLeft[count*2+0] = pLeft[count];
										pLeft[count*2+1] = pRight[count];
									}
									XDisposePtr(monoSound);
								}
							}
						}
						pSampleData = (char *)newSound;
						break;
					case sixToOne:
						pInfo->size = pCmpBuffer->numFrames * (pInfo->channels) * (pInfo->bitSize / 8);
						pInfo->size *= 6;	// 2 bytes at 3:1 is 6 bytes for a packet, 1 byte at 6:1 is 6 bytes too
						newSound = XNewPtr(pInfo->size);
						pInfo->pMasterPtr = newSound;
						if (newSound)
						{
							if (pInfo->channels == 1)
							{
								XExpandMace1to6(pSampleData, newSound, pInfo->frames, NULL, NULL, 1, 1);
								pInfo->frames *= 6;			// adjust the frame count to equal the real frames
							}
							else
							{
								monoSound = XNewPtr(pInfo->size / 2);
								if (monoSound)
								{
									XExpandMace1to6(pSampleData, newSound, pInfo->frames, NULL, NULL, pInfo->channels, 1);
									XExpandMace1to6(pSampleData, monoSound, pInfo->frames, NULL, NULL, pInfo->channels, 2);
									pLeft = (char *)newSound;
									pRight = (char *)monoSound;
									pInfo->frames *= 6;			// adjust the frame count to equal the real frames
									offset = pInfo->frames - 1;
									// copy the data into a stereo sample block, copy backwards so that we don't have to create
									// two blocks of data
									for (count = offset; count >= 0; count--)
									{
										pLeft[count*2+0] = pLeft[count];
										pLeft[count*2+1] = pRight[count];
									}
									XDisposePtr(monoSound);
								}
							}
						}
						pSampleData = (char *)newSound;
						break;
				}
				break;
		}
	}
	return pSampleData;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void Phase8BitWaveform(unsigned char * pByte, long size);
void Phase8BitWaveform(unsigned char * pByte, long size)
{
	while (size--)
	{
		*pByte++ -= 128;
	}
}
#endif


/* EOF of SampleTools.c
*/

