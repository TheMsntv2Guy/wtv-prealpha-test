#include "fidoSim.h"

// see Clarisworks spreadsheet rgb-yuv2 for how these numbers were derived
// the only change here is that y is biased by 0 instead of 16; the 16 bias happens on the output stage of the hardware

void rgbPixel::toYUV(yuvPixel& pixel)
{
	pixel.y = 66 * r + 129 * g + 25 * b >> 8;
	pixel.u = -38 * r + -74 * g + 113 * b >> 8;
	pixel.v = 113 * r + -94 * g + -18 * b >> 8;
}

void yuvPixel::toRGB(rgbPixel& pixel)
{
	int intY = y * 299;
	int r = intY + 409 * v >> 8;
	pixel.r = r < 0 ? 0 : r > 255 ? 255 : r;
	int g = intY - v * 208 - u * 99 >> 8;
	pixel.g = g < 0 ? 0 : g > 255 ? 255 : g;
	int b = intY + 517 * u >> 8;
	pixel.b = b < 0 ? 0 : b > 255 ? 255 : b;
}

void yuyvPixel::toRGB(rgbPixel& pixel, rgbPixel& pixel2)
{
	yuvPixel yuv;
	yuv.y = y1;
	yuv.u = u;
	yuv.v = v;
	yuv.toRGB(pixel);
	yuv.y = y2;
	yuv.toRGB(pixel2);
}

void yuavPixel::toRGB(rgbPixel& pixel)
{
	yuvPixel yuv;
	yuv.y = y;
	yuv.u = u;
	yuv.v = v;
	yuv.toRGB(pixel);
}

codebook& codebook::setYY(unsigned char y1, signed char u, unsigned char y2, signed char v)
{
	yy.y1 = y1;
	yy.y2 = y2;
	yy.u = u;
	yy.v = v;
	DebugCode(type = yyType);
#if _KEEP_RGB_
	yuvPixel temp;
	temp.y = y1;
	temp.u = u;
	temp.v = v;
	temp.toRGB(rgb1);
	temp.y = y2;
	temp.toRGB(rgb2);
#endif
	return *this;
}

codebook& codebook::setY(unsigned char y0, signed char u, unsigned char a, signed char v)
{
	y.y = y0;
	y.u = u;
	y.a = a;
	y.v = v;
	DebugCode(type = yType);
#if _KEEP_RGB_
	yuvPixel temp;
	temp.y = y0;
	temp.u = u;
	temp.v = v;
	temp.toRGB(rgb1);
#endif
	return *this;
}

bool codebook4BitRamp::equal(const codebook& compare) const
{
	return color.equal(compare);
}

void codebook4BitRamp::construct(const codebook& col)
{
	color = col;
	codebook* colorPtr = ramp;
	for (int counter = 0; counter < 16; counter++)	// copy colors
		*colorPtr++ = color;
	int alpha = color.getAlpha();
	colorPtr = ramp;
	colorPtr++->setAlpha(0);		// repeat transparent
	colorPtr++->setAlpha(alpha);	// repeat full
	Assert(fido::multipleRunLengthCount == 2 || fido::multipleRunLengthCount == 3);
	if (fido::multipleRunLengthCount >= 3)
		colorPtr++->setAlpha(alpha >> 1);
	const int steps = 15 - fido::multipleRunLengthCount;
	int step = alpha / steps;
	int mod = alpha % steps;
	int total = 0;
	int remainder = 0;
	for (int counter = 0; counter < steps; counter++) {
		colorPtr++->setAlpha(total);
		total += step;
		if ((remainder += mod) >= steps) {
			remainder -= steps;
			total++;
		}
	}
	colorPtr->setAlpha(alpha);
}
	
#if _SIMULATE_FIDO_
// given a Y-Map, scan out the contents
void scanRecord::drawYUAV(codebook& pixel, unsigned char globalAlpha)
{
	unsigned alpha = pixel.y.a;
	// !!! cheat for now; pixelIndex should never be allowed to be out of range
	// to fix this, clipping needs to be added to text
	Assert(intX >= fidoLimits::visibleLeft && intX <= fidoLimits::visibleRight);
	int pixelIndex = intX++ - fidoLimits::visibleLeft;
	if (pixelIndex < 0)
		return;
	if (pixelIndex >= fidoLimits::maxPixels)
		return;
#if _KEEP_RGB_
	rgbPixel* currentCheat = &cheat[pixelIndex];
#endif
	yuvPixel* currentScan = &scanline[pixelIndex];
	if (alpha && pixel.y.y < 0xFF) {
		if (alpha != 0xFF || globalAlpha != 0xFF || fido::backgroundAlpha == celRecord::bgAlphaForce1) {
			alpha = alpha * globalAlpha >> 8;	// !!! how does fido implement this?
			unsigned char oneOver;
			switch (fido::backgroundAlpha) {
				case celRecord::bgAlphaForce0:
					oneOver = 0;
				break;
				case celRecord::bgAlphaForce1:
					oneOver = 0xFF;
				break;
				case celRecord::bgAlpha1Minus:
				case celRecord::bgAlpha1White:
					oneOver = 0xFF - alpha;
				break;
			}
			if (fido::backgroundAlpha != celRecord::bgAlpha1White) {
		// !!! since we still accidentally walk outside of the source texture sometimes, this may be turned off
		#define CHECK_IT 0
		#if CHECK_IT
				Assert(pixel.y.y <= 219);
				Assert(pixel.y.u >= -112 && pixel.y.u <= 112);
				Assert(pixel.y.v >= -112 && pixel.y.v <= 112);
				Assert(currentScan->y >= 219);
				Assert(currentScan->u >= -112 && currentScan->u <= 112);
				Assert(currentScan->v >= -112 && currentScan->v <= 112);
		#endif
			// !!! note: this loses precision since it multiplies by 255 and divides by 256
			// hardware doesn't know what to do about this yet
				currentScan->y = pixel.y.y * alpha + currentScan->y * oneOver >> 8;
				currentScan->u = pixel.y.u * alpha + currentScan->u * oneOver >> 8;
				currentScan->v = pixel.y.v * alpha + currentScan->v * oneOver >> 8;			
#if _KEEP_RGB_
				currentCheat->r = math::udiv255(pixel.rgb1.r * alpha + currentCheat->r * oneOver);
				currentCheat->g = math::udiv255(pixel.rgb1.g * alpha + currentCheat->g * oneOver);
				currentCheat->b = math::udiv255(pixel.rgb1.b * alpha + currentCheat->b * oneOver);
#endif
			} else {
				currentScan->y = pixel.y.y * alpha + 219 * oneOver >> 8;
				currentScan->u = pixel.y.u * alpha >> 8;
				currentScan->v = pixel.y.v * alpha >> 8;				
#if _KEEP_RGB_
				currentCheat->r = math::udiv255(pixel.rgb1.r * alpha + oneOver);
				currentCheat->g = math::udiv255(pixel.rgb1.g * alpha + oneOver);
				currentCheat->b = math::udiv255(pixel.rgb1.b * alpha + oneOver);
#endif
			}
		} else {
			currentScan->y = pixel.y.y;
			currentScan->u = pixel.y.u;
			currentScan->v = pixel.y.v;
#if _KEEP_RGB_
			currentCheat->r = pixel.rgb1.r;
			currentCheat->g = pixel.rgb1.g;
			currentCheat->b = pixel.rgb1.b;
#endif
		}
	}
}
#endif

yuvPixel commonColors[] = {
	{0,		0,		0},		// black
	{65,		-38,		112},	// red
	{128,	-74,		-94},	// green
	{24,		112,		-18},	// blue
	{194,	-112,	18},		// yellow
	{90,		74,		94},		// magenta
	{153,	38,		-112},	// cyan
	{219,	0,		0}		// white
};