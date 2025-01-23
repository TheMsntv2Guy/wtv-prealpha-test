#ifndef _FIDOCOLOR_CPP_
#define _FIDOCOLOR_CPP_

#ifndef _FIDOCOLOR_H_
#include "fidoColor.h"
#endif

inline void rgbPixel::toYUV(yuavPixel& pixel)
{
	yuvPixel temp;
	toYUV(temp);
	pixel.y = temp.y;
	pixel.u = temp.u;
	pixel.a = 0xFF;
	pixel.v = temp.v;
}

inline bool codebook::equal(const codebook& compare) const
{
	Assert(type == codebook::yType);
	Assert(compare.type == codebook::yType);
	return compare.y.y == y.y &&
		compare.y.u == y.u &&
		compare.y.a == y.a &&
		compare.y.v == y.v;
}

inline unsigned long codebook::getPixel() const
{
	return pixel;
}

inline codebook& codebook::setAlpha(unsigned char alpha)
{
	CheapAssert(type == yType);
	y.a = alpha;
	return *this;
}
 
inline unsigned char codebook::getAlpha() const
{
	return y.a;
}

inline codebook& codebook::set(common index)
{
	extern yuvPixel commonColors[];
	type = yType;
	return setY(commonColors[index].y, commonColors[index].u, 0xFF, commonColors[index].v);
}

#endif // _FIDOCOLOR_CPP_