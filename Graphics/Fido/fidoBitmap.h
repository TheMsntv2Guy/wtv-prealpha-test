#ifndef _FIDO_BITMAP_H_
#define _FIDO_BITMAP_H_

#ifndef _YLIST_H_
#include "yList.h"
#endif

class fidoBitMap {
public:
	const texture* bitsPtr;
	unsigned short xSize;	// in pixels
	unsigned short ySize;	// in pixels
	unsigned short rowLongs;
private:	
	int computeRowBytes();	// given x and mode return tight bounding rowBytes
	enum bitDepth {
		yuav32,
		yuyv16,
		indexed8,
		indexed4
	};
	bitDepth depth() const;
public:
	enum {
		transparent = 0,
		blend = 0x80,
		opaque = 0xFF
	};
	unsigned char alpha;
	bool deleteTexture;
public:	
	char uMask;		// for patterns
	char vMask;
	int setPatternMask(int repeat);
	int getPatternMask(int mask) const;
	celRecord::celTypes mode;

	codebook* colors; // for indexed modes 
	
// !!! unfortunately, since enums invisibly are promoted to ints, this autoinitialization is not as useful as it
// could be; the alternative is in the debug version to pass classes around for the enum guys
	fidoBitMap(const texture* bits, int x, int y, celRecord::celTypes modeFormat = celRecord::directFullYUAV_1_a, int a = opaque, int rowBytes = 0, 
		int px = 0, int py = 0, codebook* clut = nil);
	fidoBitMap();
	void setSize(int x, int y);
	int getSizeX() const;
	int getSizeY() const;
	void setPattern(int x, int y);
	void getPattern(int& x, int& y) const;
	void setRowBytes(int row);
	void setAlphaMode(celRecord::celTypes);
//	int getRowBytes();
	static int shift(celRecord::celTypes mode);
	bool runLengthEncoded() const;
	int shift() const; // bits/pixel 
};

struct edgeBuilder {
	int xOneHalf;
	int yOneHalf;
	int xShift;
	// !!! the mapping could carry its inverse around with it, to avoid recalculating it
	fidoMapping matrixFixUp;
	fidoMapping inverse;
	fidoPoint corners[4];
	fidoEdge leftEdges[4], rightEdges[4];
	fidoEdge* leftEdgeEnd, * rightEdgeEnd;
	edgeBuilder(const fidoBitMap& map);
	void build(const fidoMapping& originalMatrix);
	void build(fidoRectangle& rectangle);
	void constructEdgePointOrder(int* leftPtr, int* rightPtr);
	void sort4Corners();
	void constructEdges(const int edgeCount, const fidoRectangle& clip, 
		const int* edgePoints, fidoEdge* const firstEdge, fidoEdge*& lastEdge);
	static void constructEdges(fidoEdge*& edge, const fidoEdge edgeBase[4], const fidoRectangle& clip,
		const fidoPoint& point1, const fidoPoint& point2);
};

#ifndef _FIDO_BITMAP_CPP_
#include "fidoBitmap.cpp"
#endif

#endif // _FIDO_BITMAP_H_
