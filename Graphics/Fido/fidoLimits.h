#ifndef _FIDOLIMITS_H_
#define _FIDOLIMITS_H_

struct fidoLimits {
	enum {
		maxPixels = 768,		//  PAL max visible
		unlikelyGlyphsOnLine = 100 // while not an error, it is unlikely that one scan line has this many glyphs (assert)
	};
	static int verticalBlankTop;
	static int physicalLeft;
	static int physicalTop;
	static int physicalRight;
	static int physicalBottom;
	static int visibleLeft;
	static int visibleTop;
	static int visibleRight;
	static int visibleBottom;
// if (0, 0) is upper left corner:
	static const int minX;
	static int maxX;
	static const int minY;
	static int maxY;
	enum videoMode {
		ntsc = 1,
		pal
	};
	static videoMode mode;
	static void setup(videoMode);
};

#endif // _FIDOLIMITS_H_