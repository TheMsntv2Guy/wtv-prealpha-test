#ifndef _FIDO_RAMP_H_
#define _FIDO_RAMP_H_

#ifndef _YLIST_H_
#include "yList.h"
#endif

class fidoRamp {
public:
	texture* bits;
	codebook start;
	codebook end;
	int steps;
	fidoRamp(codebook& s, codebook& e, int st); 	
};

class fidoPattern {
public:
	texture* bits;
	fidoMapping matrix;
	int xRepeat;
	int yRepeat;
	fidoPattern(texture* b, int x, int y) : bits(b), xRepeat(x), yRepeat(y)
		{ matrix.setIdentity(); }
};

#endif  // _FIDO_RAMP_H_

