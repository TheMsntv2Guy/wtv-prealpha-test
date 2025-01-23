#ifndef _FIDO_MAC_H_
#define _FIDO_MAC_H_

#ifndef _FIDO_DEBUG_H_
#include "fidoDebug.h"
#endif

#ifdef DEBUG
	#define REFRESH_ON_EVERY_SCAN_LINE 0	// normally zero, unless debugging
#else
	#define REFRESH_ON_EVERY_SCAN_LINE 0
#endif

extern int windowXSize;
extern int windowYSize;
const int windowBorder = 5;

struct fidoMac {
	static int init(char *name = "Untitled", int xo = 10, int yo = 30);
	static void choose(int window);
	static void done();
	static struct texture* makeYUAVPicture(const char* filename, int& xSize,
		int& ySize);
	static void windowClip(struct fidoRectangle* clip);
	static void setSize(int xSize, int ySize)
		{ windowXSize = xSize; windowYSize = ySize; }
	static void clear(int windowNumber);
	static void wait();
#ifndef FIDO_STANDALONE
	static PixMapHandle pixMapHandle;
#endif
};

#endif // _FIDO_MAC_H_