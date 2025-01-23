#ifndef _YLIST_H_
#define _YLIST_H_

#ifndef _SIMULATE_FIDO_
#define _SIMULATE_FIDO_ 1
#endif

#ifndef _FIDO_DEBUG_H_
#include "fidoDebug.h"
#endif

#ifndef _FIDOCOLOR_H_
#include "fidoColor.h"
#endif

#ifndef _FIDOLIMITS_H_
#include "fidoLimits.h"
#endif

#ifndef _FIDOMATH_H_
#include "fidoMath.h"
#endif


struct fidoEdge {	// same as fidoRectangle, but xStart and xEnd are different from left and right
	signed16dot16 xStart;
	signed16dot16 yStart;	// could be short, but as short it is a pain, since fraction part needs to be 1/2
	signed16dot16 xEnd;
	signed16dot16 yEnd;
// may not need these; but they're here now for debugging and attempts at additional precision
	signed16dot16 xTrueStart; // unclipped values for computing consistent slopes
	signed16dot16 yTrueStart; // unclipped values for computing consistent slopes
	signed16dot16 xTrueEnd; // unclipped values for computing consistent slopes
	signed16dot16 yTrueEnd; // unclipped values for computing consistent slopes
#ifdef DEBUG
	int finals;
	struct {
		struct celRecord* cel;
		int y;
		signed16dot16 x;
		signed16dot16 u;
		signed16dot16 v;
		signed16dot16 w;
	} final[4];
#endif
};

struct yEntry {
	enum hint {
		micro,
		mini,
		maxi,
		reserved
	};
	static const long termination;	// !!! this and others like it should be #define s in the nondebug version 
	union  {
	// !!! verify that this packs correctly on 68K, PPC, (done) MIPS (undone)
		struct {
			signed bottom : 10;
			signed top : 10;
			unsigned size : 2;
			unsigned index : 10;
		} bits;
		long full;
	};
	yEntry();
	yEntry(int bottom, int top, hint, unsigned int index);
	void swap(); 	// swaps top and bottom to enable/disable layer
	bool terminate() const;	// check for entry equal to terminate condition
	void get(int& bottom, int& top, hint&, unsigned int& index) const;
};

class yEntryContainer {
public:
	int size;
	struct celRecord* lastCelBase;
	yEntry* map;
	yEntry* end;
	yEntry lastFull;
	yEntryContainer(int size = 64);		// reasonable default?
	~yEntryContainer();
//	void addLast(int index);	// copy last entry, and change new entry to index
	void grow(struct celRecord*, class celArrayPool* celPool);
};

class celRecord {
public:
friend class fido;
	enum size {
		microSize = 8,
		miniSize = 16,
		fullSize = 48
	};
	enum celTypes {
		lastCel = 0x01,			// set if last cel in 
		textureMem = 0x02, 	// texture type selection
		bgAlphaForce1 = 4,		// force the background value to be full
		bgAlpha1Minus = 8,		// normal, multiply background by 1 - alpha
		bgAlpha1White = 12,		// brighten background, multiply by 2 * (1 - alpha)
		directFullYUYV = 0x10,	// 16 bit full
		vq8MiniYUYV = 0x20,	// 8 bit mini 2x2
		directMiniYUYV = 0x30,	// 16 bit mini
		vq8FullYUAV = 0x40,	// 8 bit full 
		directFullYUAV = 0x50,	// 32 bit full
		directFullYUAV_1_a = 0x58,	// 32 bit full with 1-a
		vq8MiniYUAV = 0x60,	// 8 bit mini 
		directMiniYUAV = 0x70,	// 32 bit mini
		vq4FullYUAV = 0x80,	// 4 bit full 
		loadCodeBook = 0x90,	// load 4 bit code book (micro) and others
		loadCodeBookLast = 0x91,
		loadYMapBase = 0x94,
		loadYMapBaseLast = 0x95,
		loadYMapMaster = 0x96,
		loadYMapMasterLast = 0x97,
		loadCelsBase = 0x98,
		loadCelsBaseLast = 0x99,
		loadCelsMaster = 0x9A,
		loadCelsMasterLast = 0x9B,
		loadRAMAddressMSB = 0x9C,
		loadRAMAddressMSBLast = 0x9D,
		loadROMAddressMSB = 0x9E,
		loadROMAddressMSBLast = 0x9F,
		vq4MiniYUAV = 0xA0,	// 4 bit mini 
		vq4MiniRLE = 0xB0,		// 4 bit mini run length encoded
		vq4MicroYUAV = 0xC0,	// 4 bit micro
		vq4MicroRLE = 0xD0,	// 4 bit micro run length encoded
		vq4MicroRLE_1_a = 0xD8,
		reserved = 0xE0,		// 
		vq8MiniRLE = 0xF0,		// 8 bit mini run length encoded
		bgAlphaForce0 = 0,		// force the background to ignored
		vq8FullYUYV = 0x00,	// 8 bit full 2x2
		bgAlphaMask = 0x0C,	// background alpha
		miniCelType = 0x20,
		specialCelType = 0x90,
		specialMask = 0x9E,
		microCelType = 0xC0,
		celTypeMask = 0xF0		// codebook type selection
	};
#define USE_BIT_FIELDS 1
#if USE_BIT_FIELDS
// micro cel is 8 bytes
	unsigned 		mode : 8;
	unsigned  		topOffset : 8; 
	unsigned		reserved0 : 2; 	// used to keep track of allocations
private:
	unsigned 		textureRowLongsLow : 4;	// for micro, only 4 bits
	signed		xLeftStart : 10;	// (big cel) for micro, only 10 bits
public:
	unsigned 		bottomOffset : 8; 
private:
	unsigned		textureBase : 24;	// really only 24 bits
// mini cel is 16 bytes
	unsigned 		textureRowLongsHigh : 8;	// for micro, only 4 bits
	unsigned		duxdvRowAdjust : 8;		// used as dvRowAdjust in mini cel record
	unsigned		reserved1 : 6;
	signed		xRightStart : 10;
public:
	unsigned		globalAlpha : 8;
private:
	unsigned		codebookBase : 24;	//
public:
// long cel is 48 bytes
// initializer
	signed8dot8	uStart;
	signed8dot8	duRowAdjust;
	unsigned0dot16	wStart;
	signed0dot16	dwRowAdjust;
	signed8dot8	vStart;
	signed8dot8	dvRowAdjust;
private:
	unsigned		reserved2 : 2;
	unsigned		xLeftStartFrac : 10;
public:
	signed		dxLeft : 20;		// !!! ? could be 8dot10
private:
	unsigned		reserved3 : 2;
	unsigned		xRightStartFrac : 10;
public:
	signed		dxRight : 20;
	signed8dot8	dvx;
	signed0dot16	dwx;
	signed		uMask : 8;		// bits to mask in u and v
	signed		vMask : 8;		// bits to mask in u and v
private:
	signed		duxMSN : 	4;
	unsigned		duxLSN : 4;
	unsigned		reserved4 : 8;
	long			reserved5;
public:
#else
// micro cel is 8 bytes
	celTypes		mode;
	unsigned char 	topOffset; 
	// unsigned : 2; 	// add 2 bits reserved here
	unsigned short 	textureRowLongs;	// for micro, only 4 bits
	signed10dot10	xLeftStart;	// (big cel) for micro, only 10 bits
	unsigned char 	bottomOffset; 
	const texture*	textureBase;	// really only 24 bits
// mini cel is 16 bytes
	signed10dot10	xRightStart;
	signed8dot8	dux;			// used as dvRowAdjust in mini cel record
	unsigned char	globalAlpha;
	const codebook* codebookBase;	// 
// long cel is 48 bytes
// initializer
	signed8dot8	uStart;
	signed8dot8	duRowAdjust;
	unsigned0dot16	wStart;
	signed0dot16	dwRowAdjust;
	signed8dot8	vStart;
// static
	signed8dot8	dvRowAdjust;
	signed10dot10	dxLeft;		// !!! ? could be 8dot10
	signed10dot10	dxRight;
	signed8dot8	dvx;
	signed0dot16	dwx;
	unsigned char	uMask;		// bits to mask in u and v
	unsigned char	vMask;		// bits to mask in u and v
#endif
	unsigned short get_textureRowLongs() const;
	signed10dot10 get_xLeftStart() const;
	signed10dot10 get_xRightStart() const;
	signed8dot8 get_dux() const;
	void set_textureRowLongs(unsigned short);
	void set_textureRowLongsMicro(unsigned short);
	void set_xLeftStart(signed10dot10);
	void set_xLeftStartMini(signed10dot10);
	void set_xRightStart(signed10dot10);
	void set_xRightStartMini(signed10dot10);
	void set_dux(signed8dot8);
	void set_duxMini(signed8dot8);
	const texture* get(const texture*&) const;
	const codebook* get(const codebook*&) const;
	void set(const texture*);
	void set(const codebook*);
// support data and functions
	void construct(const fidoMapping& inverse, const class fidoBitMap& map,
		NonDebugCode(const) fidoEdge* leftEdgePtr, NonDebugCode(const) fidoEdge* rightEdgePtr);
	void convert(yEntry::hint);
	void get(int& x, int& y) const;
	yEntry::hint get() const;
	static yEntry::hint get(celTypes mode);
	int indexSize(yEntry::hint) const;
	int indexSize() const;
// !!! these need to be restructured to support returning correct offsets
	celRecord* nextMini();
	celRecord* nextMicro();
	int rowBytes() const;
	void scan(class fidoStats&) const;
	void scanMicro(int height) const;
	void set(const yEntry*);
	void set(const celRecord*);
	void clearLastCelBit();
	void setLastCelBit();
	bool testLastCelBit() const;
};

// this is the only place cels are allocated except one; yMapBuilder->current
// !!! when celArrayPool runs out of space and stuffs itself in list, it is unnecessarily inefficient
// list instead could point to:
struct celPoolLink {
	celPoolLink& next;
	celRecord* cels;
};
// celPoolLinks could be allocated in turn from pools; then they could look like:
struct betterCelPoolLink {
	betterCelPoolLink& next;
	celRecord* celsList[16];
	celRecord** last;
};
// but this assumes that the caller didn't know how many cels to allocate up front
// and also guessed way too small

class celArrayPool {
#if 0
	celRecord* switchCelBase;
#endif
	celRecord* lastCel;	// last cel allocated
	int size;
	celArrayPool* list;
	class yEntryContainer& map;
	int getIndex();
	celRecord* grow();
	void construct();
	void destroy();
public:
	celRecord* last;	// pointer to next free cel available
	celRecord* cels;
	celArrayPool(class yEntryContainer& map, int size = 8);
	~celArrayPool();
	celRecord* next();
	celRecord* nextMini();
	celRecord* nextMicro();
	celRecord* nextSpecialMicro();	// the same as nextMicro, but won't attempt to grow the celPool under any conditions
	int getIndex(celRecord*);
	void init();
	void remove(celRecord*);
// !!! this needs to take into account mini and micro cels
	int count();
};


struct celLink {
	enum {
		min = 64
	};
	celLink* next;			// could be offset into linkList
	int index;				// offset into celBase
};

// this only exists while a new cel list is being built
class yMapBuilder {
friend celArrayPool;
friend class fidoBitMap;
	struct clutBlockList {	// used by text
		enum { max = 8 };
		codebook4BitRamp block[max];
		clutBlockList* next;
		bool blockZeroReferenced;
	} *clutList;
// state for the most recently loaded 4 bit clut
	codebook4BitRamp* clutPointer;
	struct {
		const codebook4BitRamp* clutPointer;
		int top;
		int bottom;
	} lastClut;
	
	struct fillBlockList {	// used by lines, rectangles
		enum { max = 16 };
		codebook block[max];	// used by lines, rectangles
		fillBlockList* next;
		bool blockZeroReferenced;
	} *fillList;
	codebook* fillPointer;
	
	signed16dot16 originX;
	short originY;
	celArrayPool* celPool;
// !!! could only construct these blocks when used
	void construct();
	void destroy();
	const codebook4BitRamp* findClut(const codebook& match);
	const codebook* findFill(const codebook& match);
	bool firstClut(const codebook4BitRamp* textClutPtr);
	bool firstFill(const codebook* fillColorPtr);
	void grow(clutBlockList*);
	void grow(fillBlockList*);
public:	
	fidoRectangle bounds;
//	celRecord* lastCel;	// used to finish off cel list
	yMapBuilder(celArrayPool* cels);
	~yMapBuilder();
	yEntry& add(celRecord* cel, int yStart, int yEnd);
	celRecord* addCelGlyph(texture* base, unsigned short bitsOffset, int rowLongs,
		int xPos, int topOffset, int bottomOffset);
	celRecord* addCelGlyph(struct glyphState&, int index, int xPosition, int width);
	void constructEdgeCel(const class fidoBitMap& map, NonDebugCode(const) class edgeBuilder&);
	void draw(const char* charStream,  int length, const union fidoFont* font,
		codebook& textColor, const fidoMapping& matrix, const fidoRectangle& clip);
	void draw(const char* charStream,  int length, const union fidoFont* font,
		codebook& textColor, signed16dot16 x, signed16dot16 y, const fidoRectangle& clip);
	void draw(const class fidoBitMap& map, signed16dot16 baseX, signed16dot16 baseY,
		signed16dot16 scaleX, signed16dot16 scaleY, const fidoRectangle& clip);
	void draw(const class fidoBitMap& map, const fidoMapping& matrix, const fidoRectangle& clip);
	void draw(const fidoRectangle& rectangle, const codebook& fillColor, const fidoRectangle& clip);
	void draw(const fidoRectangle& rectangle, const codebook& fillColor, const fidoMapping& matrix,
		const fidoRectangle& clip);
	void draw(const class fidoLine& line, const codebook& fillColor, signed16dot16 penWidth,
		const fidoMapping& matrix, const fidoRectangle& clip);
	void draw(const fidoRectangle& rectangle, const class fidoRamp& fillColor, const fidoRectangle& clip);
	void draw(const fidoRectangle& rectangle, const class fidoRamp& fillColor, const fidoMapping& matrix,
		const fidoRectangle& clip);
	void draw(const class fidoLine& line, const class fidoRamp& fillColor, signed16dot16 penWidth,
		const fidoMapping& matrix, const fidoRectangle& clip);
	void draw(const fidoRectangle& rectangle, const class fidoPattern& fillColor, const fidoRectangle& clip);
	void draw(const fidoRectangle& rectangle, const class fidoPattern& fillColor, const fidoMapping& matrix,
		const fidoRectangle& clip);
	void draw(const class fidoLine& line, const class fidoPattern& fillColor, signed16dot16 penWidth,
		const fidoMapping& matrix, const fidoRectangle& clip);
	void init();
	yEntry* terminate();
};

#ifndef _YLIST_CPP_
#include "yList.cpp"
#endif

#endif // _YLIST_H_
