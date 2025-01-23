/*
**
** Sound Modifier:  Amplifier/Volume Scaler.
**
** Parameter 1 is the multiplication factor (as a whole number.)  Set this
** number higher for more amplification.
**
** Parameter 2 is the division factor (also a whole number.)  Set this
** number higher for lower volume.
**
** The intermediate results are calculated to 32 bit precision.  Volume
** scaling of, for example, 100 / 99, will work as expected (a 1% rise in
** volume.)
**
**
** Written by James L. Nitchals.
**
** 'C' version by Steve Hales.
**
** © Copyright 1991-1996 Steve Hales.  All rights reserved.
** No portion of this program (whether in source, object, or other form)
** may be copied, duplicated, distributed, or otherwise utilized without
** direct consent of the author.
**
** Modification History:
**
**	10/5/95		Created
*/
#include "GenSnd.h"
#include "GenPriv.h"

#include "SMOD.h"

#define MOD_STAND_ALONE		0


#if MOD_STAND_ALONE == 1
void main(unsigned char *pSample, long length, long param1, long param2)
#else
void VolumeAmpScaler(unsigned char *pSample, long length, long param1, long param2)
#endif
{
	register long	count, scaleCount, scale;
	unsigned char	scaledLookup[256];

	if (pSample && length && param1 && param2 && (param1 != param2))
	{
		// build new scaling table
		scaleCount = param2 / 2;
		for (count = 0; count < 256; count++)
		{
			scale = (128 - count) * param1;
			if (scale < 0)
			{
				scale -= scaleCount;
			}
			else
			{
				scale += scaleCount;
			}
			scale = scale / param2;
			// Clip samples to max headroom
			if (scale > 127)
			{
				scale = 127;
			}
			if (scale < -128)
			{
				scale = -128;
			}
			scaledLookup[count] = scale + 128;
		}
		// Scale the samples via a the new lookup table
		for (count = 0; count < length; count++)
		{
			scale = pSample[count];
			pSample[count] = scaledLookup[scale];
		}
	}
}

// EOF of SMOD-Amplifier/Volume Scaler.c
