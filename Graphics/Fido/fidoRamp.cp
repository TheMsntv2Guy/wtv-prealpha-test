#include "fidoBitmap.h"
#include "fidoRamp.h"

fidoRamp::fidoRamp(codebook& s, codebook& e, int st) : start(s), end(e), steps(st)
{
	bits = newArray(FidoTexture, steps + 1);
	Assert(bits);
	texture* bitsPtr = bits;
	int divisor = steps - 1;
	signed16dot16 stepY = ff(end.y.y - start.y.y) / divisor;
	signed16dot16 stepU = ff(end.y.u - start.y.u) / divisor;
	signed16dot16 stepA = ff(end.y.a - start.y.a) / divisor;
	signed16dot16 stepV = ff(end.y.v - start.y.v) / divisor;
	signed16dot16 sumY = ff(start.y.y);
	signed16dot16 sumU = ff(start.y.u);
	signed16dot16 sumA = ff(start.y.a);
	signed16dot16 sumV = ff(start.y.v);
	bitsPtr++->fullColorPixel = start;
	for( int counter = 0; counter < divisor; counter++) {
		sumY += stepY;
		sumA += stepA;
		sumU += stepU;
		sumV += stepV;
		bitsPtr++->fullColorPixel.setY(s16dot16::round(sumY), 
			s16dot16::round(sumU), s16dot16::round(sumA), s16dot16::round(sumV));
	}
}


void yMapBuilder::draw(const fidoRectangle& rectangle, const fidoRamp& ramp, const fidoRectangle& clip)
{
	// construct bitmap
	fidoBitMap bitmap(ramp.bits, 1, ramp.steps, celRecord::directFullYUAV_1_a);
	// construct position and scale from rectangle
	draw(bitmap, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
		(rectangle.bottom - rectangle.top) / ramp.steps, clip);
	
}


void yMapBuilder::draw(const fidoRectangle& rectangle, const fidoRamp& ramp, const fidoMapping& matrix,
	const fidoRectangle& clip)
{
	fidoBitMap bitmap(ramp.bits, 1, ramp.steps, celRecord::directFullYUAV_1_a);
	// construct mapping from rectangle and mapping
	if (matrix.order() <= matrix.translated) {
		draw(bitmap, matrix.name.moveX + rectangle.left, matrix.name.moveY + rectangle.top,
			rectangle.right - rectangle.left, (rectangle.bottom - rectangle.top) / ramp.steps, clip);
	} else if (matrix.order() <= matrix.scaled) {
		draw(bitmap, matrix.name.moveX + s16dot16::multiply(rectangle.left, matrix.name.scaleX),
			matrix.name.moveY + s16dot16::multiply(rectangle.top, matrix.name.scaleY),
			s16dot16::multiply(rectangle.right - rectangle.left, matrix.name.scaleX),
			s16dot16::multiply(rectangle.bottom - rectangle.top, matrix.name.scaleY) / ramp.steps, clip);
	} else {
		fidoMapping rectMap;
	// !!! ? need constructor for mapping
		rectMap.name.scaleX = rectangle.right - rectangle.left;
		rectMap.name.skewY = 0;	
		rectMap.name.u = 0;
		rectMap.name.skewX = 0;
		rectMap.name.scaleY = (rectangle.bottom - rectangle.top) / ramp.steps;
		rectMap.name.v = 0;
		rectMap.name.moveX = rectangle.left;
		rectMap.name.moveY = rectangle.top; 
		rectMap.name.w = s2dot30::one;
		rectMap.concatenate(matrix);
		draw(bitmap, rectMap, clip);
	}
}

// !!! this doesn't make sense, yet
// I probably want to specify a matrix with the pattern, and split up draw(bitmap, mapping, clip)
// to take a list of transformed points rather than transforming the rectangle formed by the bitmap
void yMapBuilder::draw(const fidoRectangle& rectangle, const fidoPattern& pattern, const fidoRectangle& clip)
{
	fidoRectangle clippedRect = rectangle;
	if (clippedRect.left < clip.left) {
		if (clippedRect.left >= clip.right)
			return;
		clippedRect.left = clip.left;
	}
	if (clippedRect.top < clip.top) {
		if (clippedRect.top >= clip.bottom)
			return;
		clippedRect.top = clip.top;
	}
	if (clippedRect.bottom > clip.bottom) {
		if (clippedRect.bottom <= clip.top)
			return;
		clippedRect.bottom = clip.bottom;
	}
	if (clippedRect.right > clip.right) {
		if (clippedRect.right <= clip.left)
			return;
		clippedRect.right = clip.right;
	}	
	// construct bitmap
	fidoBitMap bitmap(pattern.bits, pattern.xRepeat, pattern.yRepeat, celRecord::directFullYUAV_1_a, fidoBitMap::opaque, 0, 
		pattern.xRepeat, pattern.yRepeat);
	// construct position and scale from rectangle
	edgeBuilder build(bitmap);
	build.leftEdgeEnd = &build.leftEdges[1];
	build.leftEdges[0].xStart = build.leftEdges[0].xEnd = build.leftEdges[0].xTrueStart = build.leftEdges[0].xTrueEnd = clippedRect.left;
	build.leftEdges[0].yStart = s16dot16::roundUp(build.leftEdges[0].yTrueStart = clippedRect.top);
	build.leftEdges[0].yEnd = s16dot16::roundDown(build.leftEdges[0].yTrueEnd = clippedRect.bottom);
	DebugCode(build.leftEdges[0].finals = 0);
	build.rightEdgeEnd = &build.rightEdges[1];
	build.rightEdges[0].xStart = build.rightEdges[0].xEnd = build.rightEdges[0].xTrueStart = build.rightEdges[0].xTrueEnd = clippedRect.right;
	build.rightEdges[0].yStart = s16dot16::roundUp(build.rightEdges[0].yTrueStart = clippedRect.top);
	build.rightEdges[0].yEnd = s16dot16::roundDown(build.rightEdges[0].yTrueEnd = clippedRect.bottom);
	DebugCode(build.rightEdges[0].finals = 0);
// !!! could carry inverse around with pattern
	pattern.matrix.invert(&build.inverse);
	constructEdgeCel(bitmap, build);
}

