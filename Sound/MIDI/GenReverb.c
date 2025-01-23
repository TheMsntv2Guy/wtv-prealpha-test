#include "GenSnd.h"
#include "GenPriv.h"
#include "Headers.h"
void PV_ProcessReverbStereo(void)
{
	register INT32 b, bz;
	register INT32 *sourceL, *sourceR;
	register INT32 *reverbBuf;
	register int a;
	register long reverbPtr1, reverbPtr2, reverbPtr3, reverbPtr4;

	if (MusicGlobals->reverbUnitType == REVERB_TYPE_1) return;

#if 0
// Look for intruders in our reverb unit
	for (a = 0; a < REVERB_BUFFER_SIZE; a++)
	{
		if (MusicGlobals->reverbBuffer[a] != 0)
		{
			Message(("Stomped at address %x", &(MusicGlobals->reverbBuffer[a])));
			Message(("Buffer value = %x", MusicGlobals->reverbBuffer[a]));
			Message(("reverbptr = %x", MusicGlobals->reverbPtr & REVERB_BUFFER_MASK));
			MusicGlobals->reverbBuffer[a] = 0;
		}
	}
#endif

	reverbBuf = &MusicGlobals->reverbBuffer[0];
	sourceL = &MusicGlobals->songBufferLeftMono[0];
	sourceR = &MusicGlobals->songBufferRightStereo[0];

	b = MusicGlobals->LPfilterL;
	bz = MusicGlobals->LPfilterR;
	reverbPtr1 = MusicGlobals->reverbPtr & REVERB_BUFFER_MASK;
	reverbPtr2 = (MusicGlobals->reverbPtr - 1100*2) & REVERB_BUFFER_MASK; 
	reverbPtr3 = (MusicGlobals->reverbPtr - 1473*2) & REVERB_BUFFER_MASK; 
	reverbPtr4 = (MusicGlobals->reverbPtr - 1711*2) & REVERB_BUFFER_MASK;

	for (a = MusicGlobals->One_Loop; a > 0; --a)
	{
		b -= (bz + b) >> 2;
		bz = b;
		b += reverbBuf[reverbPtr2] >> 3;
		b += reverbBuf[reverbPtr3] >> 3;
		b += reverbBuf[reverbPtr4] >> 3;
		reverbBuf[reverbPtr1] = *sourceL + *sourceR+ b - (b >> 2);
		*sourceL = *sourceL +  (b >> 1) - (b >> 3);
		sourceL++;
		*sourceR = *sourceR +  (b >> 1) - (b >> 3);
		sourceR++;
				
		reverbPtr1 = (reverbPtr1 + 1) & REVERB_BUFFER_MASK;
		reverbPtr2 = (reverbPtr2 + 1) & REVERB_BUFFER_MASK;
		reverbPtr3 = (reverbPtr3 + 1) & REVERB_BUFFER_MASK;
		reverbPtr4 = (reverbPtr4 + 1) & REVERB_BUFFER_MASK;
	}
	MusicGlobals->LPfilterL = b;
	MusicGlobals->LPfilterR = bz;
	MusicGlobals->reverbPtr = reverbPtr1;
}
