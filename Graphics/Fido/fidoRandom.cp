#include "fidoMath.h"
#include "fidoRandom.h"

void fidoRandom::next()
{
	if (++seed.lo == 0)
		++seed.hi;
	const unsigned short ahi = 0xafe9;
	const unsigned short alo = 0x6a0d;
	unsigned short rhi = seed.lo >> 16;
	unsigned short rlo = seed.lo;
	unsigned long temp = (unsigned long)ahi * rlo + (unsigned long)alo * rhi;
	unsigned long hiTemp = (unsigned long)ahi * rhi + (temp >> 16);
	unsigned long loTemp = (unsigned long)alo * rlo;
	temp <<= 16;
	loTemp += temp;
	hiTemp += loTemp < temp;
	hiTemp += 0x9e3779b9 * seed.lo + 0xafe96a0d * seed.hi;
	seed.hi = hiTemp;
	seed.lo = loTemp;
}

unsigned long fidoRandom::get(long count, long focus)
{
	if (focus > 4)
		return get(31, focus-1) + get(31, focus-1) >> 32 - count;
	if (focus < 0)
		return get(32, -focus) + 0x80000000 >> 32 - count;
	next();
	if (focus > 0)
	{	unsigned long mask = 0xffffffff >> focus;
		unsigned long rand = seed.hi;
		unsigned long result = 0;
		short i = 1 << focus;
		
		while (i--)
		{	result += rand & mask;
			rand = rand << 5 | rand >> 27;
		}
		return result >> 32 - count;
	}
	return (unsigned long)seed.hi >> 32 - count;
}
