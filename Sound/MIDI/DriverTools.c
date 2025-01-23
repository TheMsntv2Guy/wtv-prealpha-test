/*****************************************************/
/*
**	DriverTools.c
**
**		Instrument and Song resource tools for the SoundMusicSys driver.
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**
**	History	-
**	1/31/93		Created
**	6/23/95		Integrated into SoundMusicSys Library
**	11/20/95	Removed pragma unused
**	12/11/95	Removed GetInstrumentEnvelopeData
**	12/15/95	Changed memory allocation to X_API
**				Pulled out most Mac specific code
**	1/1/96		Forced more defaults for NewSong
**	1/11/96		Forced more defaults for NewInstrument
**	1/18/96		Spruced up for C++ extra error checking
**	1/28/96		Added GetKeySplitFromPtr
**				Added NewSongPtr & DisposeSongPtr
**	2/3/96		Removed extra includes
**	2/12/96		Changed Boolean to SMSBoolean
**	2/17/96		Added more platform compile arounds
**	2/19/96		Changed NewSongPtr
**	5/15/96		Fixed odd address problem with GetKeySplitFromPtr
**
*/
/*****************************************************/
#include "X_API.h"

#if X_HARDWARE_PLATFORM == X_MACINTOSH
	#include <Types.h>
	#include <Resources.h>
	#include <Memory.h>
//	#include <Sound.h>

	#include "MacSpecificSMS.h"
#endif



#include "PrivateSoundMusicSystem.h"
//#include "SoundMusicSystem.h"




SongResource * NewSongPtr(	short int midiID,
						short int maxSongVoices, 
						short int maxNormalizedVoices, 
						short int maxEffectVoices,
						short int reverbType)
{
	register SongResource *song;

	song = (SongResource *)XNewPtr((long)sizeof(SongResource));
	if (song)
	{
		song->midiResourceID = midiID;
		song->noteDecay = 5;
		song->maxNotes = maxSongVoices;
		song->maxNormNotes = maxNormalizedVoices;
		song->maxEffects = maxEffectVoices;
		song->reverbType = reverbType;
		song->flags1 = XBF_enableMIDIProgram | XBF_interpolateSong | XBF_interpolateLead;
		song->flags2 = XBF_masterEnableAmpScale | XBF_forceAmpScale;
		song->defaultPercusionProgram = -1;		// 126
		song->remapCount = 0;
		song->copyright = 0;
		song->author = 0;
	}
	return song;
}

void DisposeSongPtr(SongResource *theSong)
{
	XDisposePtr((XPTR)theSong);
}


// Get a keysplit entry. The result will be ordered for the native CPU
void GetKeySplitFromPtr(InstrumentResource *theX, short int entry, KeySplit *keysplit)
{
	register KeySplit *pSplits;
	short count;

	count = XGetShort(&theX->keySplitCount);
	if ( (count) && (entry < count) )
	{
		pSplits = (KeySplit *) ( ((Byte *)&theX->keySplitCount) + sizeof(short int));
		XBlockMove(&pSplits[entry], keysplit, sizeof(KeySplit));
		keysplit->sndResourceID = XGetShort(&keysplit->sndResourceID);
		keysplit->smodParameter1 = XGetShort(&keysplit->smodParameter1);
		keysplit->smodParameter2 = XGetShort(&keysplit->smodParameter2);
	}
	else
	{
		XSetMemory((void *)keysplit, (long)sizeof(KeySplit), 0);
	}
}



/* EOF of DriverTools.c
*/
