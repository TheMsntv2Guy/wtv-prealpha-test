#ifndef _FIDO_RANDOM_H_
#define _FIDO_RANDOM_H_

#ifndef _FIDOMATH_H_
#include "fidoMath.h"
#endif

class fidoRandom {
	void next();
public:
	signed32dot32 seed;
	unsigned long get(long bits = 32, long focus = 0);
};

#endif // _FIDO_RANDOM_H_