/* еее This file is obsolete */

#if 0

// ===========================================================================
//	jpMarkers.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __JPINCLUDES_H__
#include "jpIncludes.h"
#endif

void
Scaled1DIDCT_v1(register short *src,register short *dst, Boolean dontScale)
{
	register  short	d0,d1,d2,d3,d4,d5,d6,d7;
	register  short	e0,e1,e2,e3,e4;
	register  short b1_3 = kb1;
	register  short b2 = kb2;
	register  short b4 = kb4;
	register  short b5 = kb5;

	e0 = *src++;	e1 = *src++;	e2 = *src++;	e3 = *src++;
	d0 = *src++;	d5 = *src++;	d4 = *src++;	d3 = *src++;

	d1 = e0 - d0;	// d1 = s[0] - s[4]
	d0 += e0;		// d0 = s[4] + s[0]			
	e0 = d5 - e3;	// e0 = s[5] - s[3]
	e3 += d5;		// e3 = s[3] + s[5]
	d2 = e2;		// save s[2]
	e2 = e1 - d3;	// e2 = s[1] - s[7]
	e1 += d3;		// e1 = s[1] + s[7]
	d3 = d2 + d4;	// d3 = s[2] + s[6]
	d2 -= d4;		// d2 = s[2] - s[6]

	if ( (d0|d1|d3|d2|e1|e2|e3) == 0 ) {
		if ( !dontScale ) 
			e0 = O_SCALE(e0);
		dst[0*kOutSpace]=dst[1*kOutSpace]=dst[2*kOutSpace]=dst[3*kOutSpace]=dst[4*kOutSpace]=dst[5*kOutSpace]=dst[6*kOutSpace]=dst[7*kOutSpace] = e0;
	}

//	Top Half part A

	d2 = MUL(d2,b1_3) - d3;

//	Bottom Half

	d4 = MUL(b5,e0 - e2);
	d7 = e1 + e3;
	d6 = (MUL(b4,e2) - d4) - d7;
	e2 = d1 - d2;
	d5 = MUL(b1_3,e1 - e3) - d6;
	e1 = d1 + d2;
	e3 = d0 - d3;
	e4 = d5 + (d4 - MUL(b2, e0));
	e0 = d0 + d3;
	
//	Output stage

	if (dontScale) {
	 	dst[0*kOutSpace] = e0 + d7;
	 	dst[1*kOutSpace] = e1 + d6;
	 	dst[2*kOutSpace] = e2 + d5;
	 	dst[3*kOutSpace] = e3 - e4;
	 	dst[4*kOutSpace] = e3 + e4;
	 	dst[5*kOutSpace] = e2 - d5;
	 	dst[6*kOutSpace] = e1 - d6;
	 	dst[7*kOutSpace] = e0 - d7;
 	} else {
#ifdef	DO_ROUNDING
 		e0 += (1 << (O_SCALE_F-1));
 		e1 += (1 << (O_SCALE_F-1));
 		e2 += (1 << (O_SCALE_F-1));
 		e3 += (1 << (O_SCALE_F-1));
#endif
	 	dst[0*kOutSpace] = O_SCALE(e0 + d7);
	 	dst[1*kOutSpace] = O_SCALE(e1 + d6);
	 	dst[2*kOutSpace] = O_SCALE(e2 + d5);
	 	dst[3*kOutSpace] = O_SCALE(e3 - e4);
	 	dst[4*kOutSpace] = O_SCALE(e3 + e4);
	 	dst[5*kOutSpace] = O_SCALE(e2 - d5);
	 	dst[6*kOutSpace] = O_SCALE(e1 - d6);
	 	dst[7*kOutSpace] = O_SCALE(e0 - d7);
 	}
}


#endif
		

