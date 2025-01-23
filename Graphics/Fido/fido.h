#ifndef _FIDO_H_
#define _FIDO_H_

#ifndef _Y_LIST_H_
#include "yList.h"
#endif

#ifndef _FIDO_TEXT_H_
#include "fidoText.h"
#endif

class displayList {
protected:
	yEntry* terminator;	// pointer to the terminator or pop-up-a-level cels
	yEntry* yEnd;		// next free y entry
	yEntry* yMapBase;
	celRecord* celBase;
	void simulate();
	yEntryContainer yMap;
	celArrayPool cels;
	yMapBuilder builder;
	void lock();
public:
	displayList();
	void draw();
	void reset();
	void strip();	// remove scaffolding used while constructing
};

class displayLeaf : public displayList {
// clip is carried by display list; then, this additional clipping can be done when clip is set
	fidoRectangle clip;
	fidoRectangle realClip;
public:
	fidoRectangle bounds;	// set by adds, or can be explicitly set by user
	fidoMapping matrix;
	codebook fillColor;
	signed16dot16 pen;
	const fidoFont* font;
	displayLeaf();
	void add(const char* text, int length = 0);
	void add(const class fidoBitMap& map);
	void add(const fidoRectangle& rectangle);
	void add(const class fidoLine& line);
// !!! either each primitive takes a pattern, or the display list contains the current pattern
	// text and bitmaps probably can't use patterns
	void add(const fidoRectangle& rectangle, const class fidoRamp& ramp);
	void add(const fidoRectangle& rectangle, const class fidoPattern& pattern);
	void add(const fidoLine& line, const class fidoRamp& ramp);
	void add(const fidoLine& line, const class fidoPattern& pattern);
	const fidoFont* find(fontTypes::family fontID, fontTypes::face style, int size);
	void getClip(fidoRectangle& userClip);
	fidoRectangle& getClip();
	signed16dot16 measure(const char* text, int length = 0);	// length of zero means C string
	void setClip();
	void setClip(signed16dot16 left, signed16dot16 top, signed16dot16 right, signed16dot16 bottom);
	void setClip(fidoRectangle& userClip);
};

// for containing multiple display leaves
class displayNode : public displayLeaf {
public:
	void add(const char* text, int length = 0);
	void add(const class fidoBitMap& map);
	void add(const fidoRectangle& rectangle);
	void add(const class fidoLine& line);
	void add(const fidoRectangle& rectangle, const class fidoRamp& ramp);
	void add(const fidoRectangle& rectangle, const class fidoPattern& pattern);
	void add(const fidoLine& line, const class fidoRamp& ramp);
	void add(const fidoLine& line, const class fidoPattern& pattern);
	struct yEntry& add(displayNode& node);	// returns an index
	void enable(struct yEntry&);
	void disable(struct yEntry&);
};

// the top-most list handed directly to the hardware
class displayRoot : public displayNode {
public:
	void draw();	// this one attaches this to the chip
};

// a variant that automatically puts objects in the correct bucket
class displayBucket : public displayNode {
public:
	int buckets;
	// allocate the desired number of buckets by subdividing the clipping rectangle
	
	// get the bounds of the object to be drawn, the put the objects in the correct bucket
		// it would be possible to support strict (objects are split) or sloppy (no splitting) buckets
	void add(const char* text, int length = 0);
	void add(const class fidoBitMap& map);
	void add(const fidoRectangle& rectangle);
	void add(const class fidoLine& line);
	void add(const fidoRectangle& rectangle, const class fidoRamp& ramp);
	void add(const fidoRectangle& rectangle, const class fidoPattern& pattern);
	void add(const fidoLine& line, const class fidoRamp& ramp);
	void add(const fidoLine& line, const class fidoPattern& pattern);
};

// a variant that allows synchronized moving and adding (for instance, scrolling)
class displaySync : public displayBucket {
	// for scrolling, we'd like to support an accordian-style set of buckets
		// the 128 pixel locations on all four sides of the screen are reserved for holding areas
		// the bucket is constructed such that moving it 1 pixel makes it visible
			// note that very large bitmaps (>128) require fudging their edges to keep them in number space
		// the accordian panels are kept out of the display list until they become visible
	// each bucket has its coordinates stored outside of the leaf data
		// when the bucket is made displayable, the leaf coordinates are updated
};

#ifndef _FIDO_CPP_
#include "fido.cpp"
#endif

#endif // _FIDO_H_