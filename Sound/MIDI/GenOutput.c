#include "GenSnd.h"
#include "GenPriv.h"
#include "X_API.h"
#include "Headers.h"

static INT32 PV_interp1 = 0, PV_interp2 = 0;

void PV_Generate16outputStereo(OUTSAMPLE16 FAR * dest16)
{
	register INT32 *sourceL, *sourceR;
	register LOOPCOUNT		a;
	register INT32 i, overflow_test, k8000;

	register INT32 b, c;

	/* Convert intermediate 16-bit sample format to 16 bit output samples:
	*/
	
	sourceL = &MusicGlobals->songBufferLeftMono[0];
	sourceR = &MusicGlobals->songBufferRightStereo[0];
	
	b = PV_interp1; c = PV_interp2;

#if 0
	if (b != MusicGlobals->s1Left)
		Message(("Different!"));
	if (c != MusicGlobals->s1Right)
		Message(("Different!"));
#endif

	k8000 = 0x8000;
	for (a = MusicGlobals->Four_Loop; a > 0; --a)
	{
		i = sourceL[0] >> OUTPUT_SCALAR;
		overflow_test = i + k8000;
		dest16[0] = (i + b) >> 1;
		dest16[2] = i; b = i;
		i = sourceR[0] >> OUTPUT_SCALAR;
		overflow_test |= i + k8000;
		dest16[1] = (i + c) >> 1;
		dest16[3] = i; c = i;

		i = sourceL[1] >> OUTPUT_SCALAR;
		overflow_test |= i + k8000;
		dest16[4] = (i + b) >> 1;
		dest16[6] = i; b = i;
		i = sourceR[1] >> OUTPUT_SCALAR;
		overflow_test |= i + k8000;
		dest16[5] = (i + c) >> 1;
		dest16[7] = i; c = i;

		i = sourceL[2] >> OUTPUT_SCALAR;
		overflow_test |= i + k8000;
		dest16[8] = (i + b) >> 1;
		dest16[10] = i; b = i;
		i = sourceR[2] >> OUTPUT_SCALAR;
		overflow_test |= i + k8000;
		dest16[9] = (i + c) >> 1;
		dest16[11] = i; c = i;

		i = sourceL[3] >> OUTPUT_SCALAR;
		overflow_test |= i + k8000;
		dest16[12] = (i + b) >> 1;
		dest16[14] = i; b = i;
		i = sourceR[3] >> OUTPUT_SCALAR;
		overflow_test |= i + k8000;
		dest16[13] = (i + c) >> 1;
		dest16[15] = i; c = i;

		if (overflow_test & 0xFFFF0000)
		{
			i = sourceL[0] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[0] = i - k8000;
			dest16[2] = i - k8000;
			i = sourceR[0] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[1] = i - k8000;
			dest16[3] = i - k8000;

			i = sourceL[1] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[4] = i - k8000;
			dest16[6] = i - k8000;
			i = sourceR[1] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[5] = i - k8000;
			dest16[7] = i - k8000;

			i = sourceL[2] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[8] = i - k8000;
			dest16[10] = i - k8000;
			i = sourceR[2] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[9] = i - k8000;
			dest16[11] = i - k8000;

			i = sourceL[3] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[12] = i - k8000; b =  i - k8000;
			dest16[14] = i - k8000;
			i = sourceR[3] >> OUTPUT_SCALAR;
			i += k8000;
			if (i & 0xFFFF0000)
				{ if (i > 0) i = 0xFFFF; else i = 0;}
			dest16[13] = i - k8000; c =  i - k8000;
			dest16[15] = i - k8000;
		}
		sourceL += 4; sourceR += 4; dest16 += 16;
	}
	PV_interp1 = b; PV_interp2 = c;
	MusicGlobals->s1Left = b;
	MusicGlobals->s1Right = c;
}
