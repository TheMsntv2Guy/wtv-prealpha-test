#ifndef _FIDO_BITMAP_CPP_
#define _FIDO_BITMAP_CPP_

#ifndef _FIDO_BITMAP_H_
#include "fidoBitmap.h"
#endif

inline bool fidoBitMap:: runLengthEncoded() const
{
	int maskedMode = mode & celRecord::celTypeMask;
	return maskedMode == celRecord::vq4MiniRLE || maskedMode == celRecord::vq4MicroRLE ||
		maskedMode == celRecord::vq8MiniRLE;
}

inline int fidoBitMap::shift(celRecord::celTypes mode)
{
	extern const char shiftCount[16];
	Assert(mode >> 4 < 16 && mode > 0 && shiftCount[mode >> 4] >= 0);
	return shiftCount[mode >> 4];
}

inline int fidoBitMap::shift() const
{
	return shift(mode);
} 

inline fidoBitMap::bitDepth fidoBitMap::depth() const
{
	return (fidoBitMap::bitDepth) shift();
}

inline int fidoBitMap::setPatternMask(int repeat)
{
	return repeat - 1;
}

inline int fidoBitMap::getPatternMask(int mask) const
{
	return (unsigned char) mask + 1;
}

inline void fidoBitMap::setSize(int x, int y)
{
	Assert( x > 0 && x < 65536 && y > 0 && y < 65536);
	xSize = x;
	ySize = y;
}

// !!! this will change as patterns can get bigger
inline void fidoBitMap::setPattern(int x, int y) 
{
	uMask = setPatternMask(x);
	vMask = setPatternMask(y);
}

inline void fidoBitMap::setRowBytes(int row)
{
	Assert((row & ~3) == 0 && (row >> 2) - 1 < 65536);
	rowLongs = ((row == 0 ? computeRowBytes() : row) >> 2) - 1;
}

inline fidoBitMap::fidoBitMap(const texture* bits, int x, int y, celRecord::celTypes modeFormat, int a, int rowBytes, 
	int px, int py, codebook* clut)
	: bitsPtr(bits), alpha(a), mode(modeFormat), colors(clut), deleteTexture(false)
{
	setSize(x, y);
	setRowBytes(rowBytes);
	setPattern(px, py);
}

inline void fidoBitMap::setAlphaMode(celRecord::celTypes alpha)
{
	Assert((alpha & ~celRecord::bgAlphaMask) == 0);
	mode = (celRecord::celTypes) (mode & ~celRecord::bgAlphaMask | alpha);
}

inline fidoBitMap::fidoBitMap()
{
}

inline int fidoBitMap::getSizeX() const
{
	return xSize;
}

inline int fidoBitMap::getSizeY() const
{
	return ySize;
}

inline void fidoBitMap::getPattern(int& x, int& y) const
{
	x = getPatternMask(uMask);
	y = getPatternMask(vMask);
}

#if 0
inline int fidoBitMap::getRowBytes()
{
	return (rowLongs + 1) << 2;
}
#endif

inline edgeBuilder::edgeBuilder(const fidoBitMap& map) :
		xOneHalf(map.xSize >> 1), yOneHalf (map.ySize >> 1), xShift(map.shift())
{
}
		
inline void edgeBuilder::build(const fidoMapping& originalMatrix)
{
	matrixFixUp.setIdentity();
	matrixFixUp.translate(ff(xOneHalf), ff(yOneHalf));
	matrixFixUp.concatenate(originalMatrix);
	matrixFixUp.invert(&inverse);
}

inline void edgeBuilder::build(fidoRectangle& rectangle) {	// builds rectangle points in top-bottom left-right sorted order
	corners[0].x = corners[2].x = rectangle.left;
	corners[0].y = corners[1].y = rectangle.top;
	corners[1].x = corners[3].x = rectangle.right;
	corners[2].y = corners[3].y = rectangle.bottom;
}

#endif // _FIDO_BITMAP_CPP_
