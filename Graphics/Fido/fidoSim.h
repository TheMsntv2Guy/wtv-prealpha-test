#ifndef _FIDOSIM_H_
#define _FIDOSIM_H_

#ifndef _SIMULATE_FIDO_
#define _SIMULATE_FIDO_ 1
#endif

#include "yList.h"

#if _SIMULATE_FIDO_
class fidoStats {
// copy the zero index element into the current line element after accumulating
public:
	struct {
		long yMapFetch;
		long celFetch;
		long celScan;
		long longRead;
		long pixelDraw;
	} line[561];
};

class scanRecord {
#if _KEEP_RGB_
	rgbPixel cheat[fidoLimits::maxPixels];
#endif
	yuvPixel scanline[fidoLimits::maxPixels];
public:
	struct codebook* colors;
	fidoStats* stats;
	signed8dot8 workingU;
	signed8dot8 workingV;
	unsigned0dot16 workingW;
	signed10dot10 currentX;
	int intX;
	bool repeat;
	void drawYUAV(struct codebook& pixel, unsigned char globalAlpha);
	bool runPixel(int pixel);
	void micro(unsigned char* vector4Ptr, int count);
	void mini8(unsigned char* vector8Ptr, int count);
};

// there can be more than one of these
// !!! ideally, you could throw away displaylist after it was built, keep this, and draw to your heart's content
// the trouble is doesn't have enough information to delete the objects once display list is gone
// the cel could have a bit that says "an allocated block starts here" , but there's no space for a corresponding bit in 
// the ymap
// one squirrelly way would be to use a second bit in the cel to say "the y map entry that points to me is at the start of
// an allocated block"; or, we just assume that whenever the celBlockBase or yMapBase changes, there is a corresponding
// allocation
// some textures or codebooks are allocated by display list; others were given to it. To know which ones to delete also 
// requires more bits. The two unused bits in the micro cel could say: delete the texture, delete the codebook
class fidoData {
public:
	const yEntry* yMap;
	celRecord* celBase; // points to list of cels that Fido knows about
	celRecord* get(int index) const;
// !!! may want to put fidoData (or pointer to) in celArrayPool 
	void scroll(celArrayPool* pool, int dx, int dy);
	void move(celRecord*, int dx, int dy);
	celRecord* pick(int x, int y);
	void remove(celRecord*);
	void add(celRecord*, int x, int y);
};

// there is only one of these
class fido {
friend scanRecord;
	static codebook codebookCache[16]; // preloaded codebook in Fido
	static int yMapIndex;	// array of counts and indices
	static unsigned char globalAlpha;
	static enum celRecord::celTypes backgroundAlpha;
	static bool runLengthEncoded;
	static short currentY;	// scan line number
	static int topLine;		// last top value read from yMap
	static fidoData working;
// !!! these aren't used, yet
	static unsigned char ramAddressMSB;
	static unsigned char romAddressMSB;
public:
	enum {
		 multipleRunLengthCount = 3
	};
	static fidoStats stats;
	static codebook initColor;
	static scanRecord scan;
	static fidoData master;
	static void frameTask(int yStart);
	static void copyToFrameBuffer();
	static void init(yEntry* map, celRecord* list);
	static void loadCodebookCache(const celRecord* cel);
	static void loadYMapBase(const celRecord* cel);
	static void loadCelsBase(const celRecord* cel);
	static void loadYMapMaster(const celRecord* cel);
	static void loadCelsMaster(const celRecord* cel);
// !!! these aren't used, yet
	static void loadRAMAddressMSB(const celRecord* cel);
	static void loadROMAddressMSB(const celRecord* cel);
};

class spot {
public:
	static void waitForVBlank();
};

#endif

#ifndef _FIDOSIM_CPP_
#include "fidoSim.cpp"
#endif

#endif // _FIDOSIM_H_
