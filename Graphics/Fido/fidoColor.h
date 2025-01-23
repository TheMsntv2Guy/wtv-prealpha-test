#ifndef _FIDOCOLOR_H_
#define _FIDOCOLOR_H_

#ifndef _FIDOMATH_H_
#include "fidoMath.h"
#endif

struct YCbCrPixel {
	unsigned char y;
	unsigned char cb;
	unsigned char cr;
	void toRGB(struct rgbPixel& pixel);
};

struct yuyvPixel {
	unsigned char y1;
	signed char u;
	unsigned char y2;
	signed char v;
	void toRGB(struct rgbPixel& pixel, struct rgbPixel& pixel2);
};

struct yuavPixel {
	unsigned char y;
	signed char u;
	unsigned char a;
	signed char v;
	void toRGB(struct rgbPixel& pixel);
};

struct yuvPixel {
	unsigned char y;
	signed char u;
	signed char v;
	void toRGB(struct rgbPixel& pixel);
};

// for cheating !
struct rgbPixel {
	unsigned char dummy;	// unsigned : 8 padded to long!; omitting dummy name also omited placeholder
	unsigned char r;
	unsigned char g;
	unsigned char b;
	void toYUV(yuvPixel& pixel);
	void toYUV(yuavPixel& pixel);
};

struct codebook {
	union {
		yuyvPixel yy;
		yuavPixel y;
		unsigned long pixel;
	};
#ifdef DEBUG
	enum colorType {	// at least for debugging, need the codebook type
		yyType,
		yType
	};
	colorType type;
#endif
	enum common {
		black,
		red,
		green,
		blue,
		yellow,
		magenta,
		cyan,
		white
	};
#if _KEEP_RGB_
	struct rgbPixel rgb1; // for cheating !
	struct rgbPixel rgb2; // for yuyv cheating !
#endif
	codebook& setYY(unsigned char y1, signed char u, unsigned char y2, signed char v);
	codebook& setY(unsigned char y, signed char u, unsigned char a, signed char v);
	codebook& set(common);
	unsigned long getPixel() const;
	codebook& setAlpha(unsigned char alpha);
	unsigned char getAlpha() const;
	bool equal(const codebook&) const;
};

// although I'd like to keep rgb values transparently around here,
// it is difficult to do and keep this to 1 long per entry for a reasonable ROM
// description
struct texture {
	union {
		unsigned long forAggregateInitializer;
		unsigned char vector4[4];
		unsigned char vector8[4];
		unsigned char vq[4]; 
		codebook fullColorPixel; // (includes yuyv, yuav)
	};
};

class codebook4BitRamp {
public:
	void construct(const codebook& color);
	codebook color;
	codebook ramp[16];
	bool equal(const codebook&) const;
};

#ifndef _FIDOCOLOR_CPP_
#include "fidoColor.cpp"
#endif

#endif // _FIDOCOLOR_H_
