#include "fidoBitmap.h"

void yMapBuilder::draw(const fidoBitMap& map, signed16dot16 baseX, signed16dot16 baseY,
	signed16dot16 scaleX, signed16dot16 scaleY, const fidoRectangle& clip)
{
	Assert(scaleX > 0);
	Assert(scaleY > 0);
	bool runLengthEncoded = map.runLengthEncoded();
	int xOneHalf = map.xSize >> 1;
	int yOneHalf = map.ySize >> 1;
	int xShift = map.shift();
	if (runLengthEncoded == false) {
		if (map.xSize >= 256) {
			// divide the original bitmap into halves and try again
			fidoBitMap xHalf = map;
			xHalf.xSize = xOneHalf;
			draw(xHalf, baseX, baseY, scaleX, scaleY, clip);
			xHalf.xSize = map.xSize - xOneHalf;
			xHalf.bitsPtr += xOneHalf >> xShift;
			draw(xHalf, baseX + xOneHalf * scaleX, baseY, scaleX, scaleY, clip);
			return;
		}
		if (map.ySize >= 256) {
			fidoBitMap yHalf = map;
			yHalf.ySize = yOneHalf;
			draw(yHalf, baseX, baseY, scaleX, scaleY, clip);
			yHalf.ySize = map.ySize - yOneHalf;
			yHalf.bitsPtr += yOneHalf * (map.rowLongs + 1);
			draw(yHalf, baseX, baseY + yOneHalf * scaleY, scaleX, scaleY, clip);
			return;
		}
	}
// !!! may want multiply or fracttoint function to take care of rounding and truncation
	signed16dot16 top = baseY;
	celRecord cel;
	cel.mode = map.mode;	// may not be a full cel
	cel.topOffset = 0;
	cel.set_textureRowLongs(map.rowLongs);		
	cel.bottomOffset = 0;
// !!! add rounding?
	int bottom = s16dot16::roundDown(baseY + map.ySize * scaleY) >> s16dot16::shift;
// !!! check top < bottom?
// !!! allow negative scaleY?
	if (bottom > clip.bottom)
		bottom = clip.bottom;
	else if (bottom <= clip.top)
		return;
	bottom++;
// !!! need to offset these to start at pixel centers, and make sure that the end at pixel centers
	cel.vStart = 0;
	if (top < clip.top) {
		cel.vStart = s16dot16::roundTo8dot8(s16dot16::divide(clip.top - top, scaleY)); // account for fraction 
		top = clip.top;
	} else if (top >= clip.bottom)
		return;
	signed16dot16 left = baseX;
	cel.uStart = 0;
// !!! ? can't clip on left if bitmap is run length encoded
	if (left < clip.left) {
		cel.uStart = s16dot16::roundTo8dot8(s16dot16::divide(clip.left - left, scaleX)); // account for fraction 
		left = clip.left;
	} else if (left >= clip.right)
		return;
	cel.set_xLeftStart(s16dot16::roundTo10dot10(left + originX));
	signed16dot16 right = baseX + map.xSize * scaleX;
	if (right > clip.right)
		right = clip.right;
	else if (right <= clip.left)
		return;
	cel.set_xRightStart(s16dot16::roundTo10dot10(right + originX));
	cel.set_dux(map.xSize == 1 ? 0 :
		s16dot16::roundTo8dot8(s16dot16::divide(s16dot16::one, scaleX)));
	cel.dvRowAdjust = map.ySize == 1 ? 0 :
		s16dot16::roundTo8dot8(s16dot16::divide(s16dot16::one, scaleY));
	celRecord* celPtr;
	if (cel.get_dux() == cel.dvRowAdjust && cel.uStart == 0 && cel.vStart == 0) {
		cel.set(map.bitsPtr);	// texture base
		if (cel.get_dux() == s8dot8::one && map.colors == nil && map.alpha == map.opaque &&  
			(cel.get_textureRowLongs() + 1) == s10dot10::roundDown(cel.get_xRightStart()) -
				s10dot10::roundUp(cel.get_xLeftStart()) >> xShift)
		{
			celPtr = celPool->nextMicro();
			*celPtr = cel;
			celPtr->convert(yEntry::micro);
		} else {
			celPtr = celPool->nextMini();
			*celPtr = cel;
			celPtr->convert(yEntry::mini);
			goto finishMini;
		}
	} else {
		Assert(runLengthEncoded == false);
		celPtr = celPool->next();
		*celPtr = cel;
		celPtr->convert(yEntry::maxi);
		celPtr->set(&map.bitsPtr[yOneHalf * (map.rowLongs + 1) + (xOneHalf >> xShift)]);	// texture base
		celPtr->dvx = 0;
		celPtr->wStart = u0dot16::one;
		celPtr->dwx = 0;
		celPtr->dwRowAdjust = 0;
		celPtr->duRowAdjust = 0;
		celPtr->dxLeft = 0;
		celPtr->dxRight = 0;
	finishMini:
		celPtr->set(map.colors);	// codebook base
		celPtr->globalAlpha = map.alpha;
		celPtr->uMask = map.uMask;
	}
	int intTop = s16dot16::roundUp(top) >> s16dot16::shift;
	celPtr->setLastCelBit();
	add(celPtr, intTop, bottom);
	fidoRectangle bitBounds;
	bitBounds.left = left;
	bitBounds.top = ff(intTop);
	bitBounds.right = right;
	bitBounds.bottom = ff(bottom);
	bounds.onion(bitBounds);
}

int fidoBitMap::computeRowBytes()
{
	Assert(xSize > 0);
	int result;
	switch (depth()) {
		case indexed4:
			result = xSize + (-xSize & 7) >> 1;
		break;
		case indexed8:
			result = xSize + (-xSize & 3);
		break;
		case yuyv16: 
			result = xSize + (xSize & 1) << 1;
		break;
		case yuav32:
			result = xSize << 2;
		break;
		default:
			Assert(0);
	}
	return result;
}

void celRecord::construct(const fidoMapping& inverseRef, const fidoBitMap& map,
	NonDebugCode(const) fidoEdge* leftEdgePtr, NonDebugCode(const) fidoEdge* rightEdgePtr)
{
// map inverse so that range of w is > 0 && <= 1
	fidoMapping inverse = inverseRef;
	signed16dot16 left = leftEdgePtr->xStart;
	signed16dot16 top = leftEdgePtr->yStart;
//	signed16dot16 deltaX = leftEdgePtr->xEnd - left;
//	signed16dot16 deltaY = leftEdgePtr->yEnd - top;
	signed16dot16 deltaX = leftEdgePtr->xTrueEnd - leftEdgePtr->xTrueStart;
	signed16dot16 deltaY = leftEdgePtr->yTrueEnd - leftEdgePtr->yTrueStart;
	bool vertical = left == leftEdgePtr->xEnd;
	signed16dot16 deltaLeft = vertical == false ? s16dot16::divide(deltaX, deltaY) : 0;
	if (top < rightEdgePtr->yStart) {
		if (vertical == false)
			left += math::multiplyDivide(rightEdgePtr->yStart - top, deltaX, deltaY);
		top = rightEdgePtr->yStart;
	}
	topOffset = 0;
	bottomOffset = 0;
// advance left to next pixel center, if it is not already sitting on one
// !!! this doesn't make sense, really; but it is sensible (at least on the first scanline) to inset left so that the first
// and final pixel on that scanlines are equidistant and up to 1/2 pixel from the edge
	left = s16dot16::roundUp(left);
	dxLeft = s16dot16::checkTo10dot10(deltaLeft);
//	signed16dot16 deltaRight = s16dot16::divide(rightEdgePtr->xEnd - rightEdgePtr->xStart,
//		rightEdgePtr->yEnd - rightEdgePtr->yStart);
	if (rightEdgePtr->xStart != rightEdgePtr->xEnd)
		dxRight = s16dot16::checkTo10dot10(s16dot16::divide(rightEdgePtr->xTrueEnd - rightEdgePtr->xTrueStart,
			rightEdgePtr->yTrueEnd - rightEdgePtr->yTrueStart));
	else
		dxRight = 0;
	if (inverse.name.u != 0 || inverse.name.v != 0) {
	// !!! rework this to do nothing if name.u < 0 && name.v < 0, and to only look down and to the right if w can increase
		signed16dot16 wTopRight = s2dot30::multiply(top, inverse.name.v) + s2dot30::to16dot16(inverse.name.w);
		signed16dot16 wTopLeft = s2dot30::multiply(left, inverse.name.u) + wTopRight;
		signed2dot30 wRow = inverse.name.v + s16dot16::multiply(deltaLeft, inverse.name.u);
		// compute w at four corners
		signed16dot16 wMax = wTopLeft;
		wTopRight += s2dot30::multiply(rightEdgePtr->xStart, inverse.name.u);
		if (wTopRight > wMax)
			wMax = wTopRight;
		long bottom = leftEdgePtr->yEnd;
		if (rightEdgePtr->yEnd < bottom)
			bottom = rightEdgePtr->yEnd;
		bottom >>= s16dot16::shift;
		int height = bottom - (top >> s16dot16::shift);
		signed16dot16 wBottomLeft;
		wBottomLeft = wTopLeft + math::multiply(wRow, height, s2dot30::shift - s16dot16::shift);	// !!! too precise?
		if (wBottomLeft > wMax)
			wMax = wBottomLeft;
		signed10dot10 leftFinalx = get_xLeftStart() + dxLeft * height;
		signed10dot10 rightFinalx = get_xRightStart() + dxRight * height;
		signed16dot16 wBottomRight = wBottomLeft + s2dot30::multiply(s10dot10::as16dot16(rightFinalx - leftFinalx),
			inverse.name.u);
		if (wBottomRight > wMax)
			wMax = wBottomRight;
		int shiftDown = 0;
	// !!! this could use some zero bit counting function if there is a clever MIPS way of doing it
		while (wMax > s16dot16::one) {
			wMax >>= 1;
			shiftDown++;
		}
		if (shiftDown) {
			for (int counter = 0; counter < 9; counter++)
				inverse.array[0][counter] >>= shiftDown;
		}
		// this used to try to reuse wTopLeft as wStart, but this discards rounding
	} 
	if (map.xSize > 1) {
	// !!! revise this to use 32dot32 multiplyAdd ?
		uStart = s16dot16::checkTo8dot8(s16dot16::multiply(left, inverse.name.scaleX) +
			s16dot16::multiply(top, inverse.name.skewX) + inverse.name.moveX);
		set_dux(s16dot16::checkTo8dot8(inverse.name.scaleX));
		duRowAdjust = s16dot16::checkTo8dot8(inverse.name.skewX +
			s16dot16::multiply(deltaLeft, inverse.name.scaleX));
	} else {
		set_dux(0);
		uStart = duRowAdjust = 0;
	}
	if (map.ySize > 1) {
		vStart = s16dot16::checkTo8dot8(s16dot16::multiply(left, inverse.name.skewY) +
			s16dot16::multiply(top, inverse.name.scaleY) + inverse.name.moveY); // v starting value: signed8dot8
		dvx = s16dot16::checkTo8dot8(inverse.name.skewY);
		dvRowAdjust = s16dot16::checkTo8dot8(inverse.name.scaleY +
			s16dot16::multiply(deltaLeft, inverse.name.skewY));
	} else
		vStart = dvx = dvRowAdjust = 0;
	wStart = s2dot30::checkToU0dot16(s16dot16::multiply(left, inverse.name.u) + 
		s16dot16::multiply(top, inverse.name.v) + inverse.name.w);
	dwx = s2dot30::checkToS0dot16(inverse.name.u);
	dwRowAdjust = s2dot30::checkToS0dot16(inverse.name.v + s16dot16::multiply(deltaLeft, inverse.name.u));
#ifdef DEBUG
	int finals = leftEdgePtr->finals;
	Assert(finals >= 0 && finals < sizeof(leftEdgePtr->final) / sizeof(leftEdgePtr->final[0]));
	long bottom = leftEdgePtr->yEnd;
	if (rightEdgePtr->yEnd < bottom)
		bottom = rightEdgePtr->yEnd;
	bottom >>= s16dot16::shift;
	int height = bottom - (top >> s16dot16::shift);
	leftEdgePtr->final[finals].y = bottom;
	leftEdgePtr->final[finals].x = s10dot10::as16dot16(get_xLeftStart() + dxLeft * height);
	Assert(leftEdgePtr->final[finals].x >= ff(fidoLimits::visibleLeft) &&
		leftEdgePtr->final[finals].x <= ff(fidoLimits::visibleRight));
	leftEdgePtr->final[finals].u = s8dot8::as16dot16(uStart + duRowAdjust * height);
	leftEdgePtr->final[finals].v = s8dot8::as16dot16(vStart + dvRowAdjust * height);
	leftEdgePtr->final[finals].w = s0dot16::as2dot30(wStart + dwRowAdjust * height);
	finals = rightEdgePtr->finals;
	Assert(finals >= 0 && finals < sizeof(rightEdgePtr->final) / sizeof(rightEdgePtr->final[0]));
	rightEdgePtr->final[finals].y = bottom;
	rightEdgePtr->final[finals].x = s10dot10::as16dot16(get_xRightStart() + dxRight * height);
	Assert(rightEdgePtr->final[finals].x >= ff(fidoLimits::visibleLeft) &&
		rightEdgePtr->final[finals].x <= ff(fidoLimits::visibleRight));
	rightEdgePtr->final[finals].u = s8dot8::as16dot16(uStart + duRowAdjust * height);
	rightEdgePtr->final[finals].v = s8dot8::as16dot16(vStart + dvRowAdjust * height);
	rightEdgePtr->final[finals].w = s0dot16::as2dot30(wStart + dwRowAdjust * height);
	Assert(leftEdgePtr->final[leftEdgePtr->finals].x < rightEdgePtr->final[finals].x);
	leftEdgePtr->finals++;
	rightEdgePtr->finals++;
#endif
}


void edgeBuilder::constructEdges(fidoEdge*& edge, const fidoEdge edgeBase[4], const fidoRectangle& clip,
	const fidoPoint& point1, const fidoPoint& point2)
{
	fidoEdge* edgePtr = edge;
	DebugCode(edgePtr->finals = 0);
	edgePtr->yStart = s16dot16::roundUp(point1.y);
	edgePtr->yEnd = s16dot16::roundDown(point2.y);
	edgePtr->xTrueStart = point1.x;
	edgePtr->yTrueStart = point1.y;
	edgePtr->xTrueEnd = point2.x;
	edgePtr->yTrueEnd = point2.y;
// clip edge list top and bottom
	signed16dot16 clipLimit = s16dot16::roundUp(clip.top);
	if (edgePtr->yStart < clipLimit)
		edgePtr->yStart = clipLimit;
	clipLimit = s16dot16::roundDown(clip.bottom);
	if (edgePtr->yEnd > clipLimit)
		edgePtr->yEnd = clipLimit;
// compute x values on pixel centers
	if (edgePtr->yStart <= edgePtr->yEnd) {	// check for edges that do not cross pixel centers
		fidoPoint delta = {point2.x - point1.x, point2.y - point1.y};
		edgePtr->xStart = point1.x + math::multiplyDivide(edgePtr->yStart - point1.y, delta.x, delta.y);
		edgePtr->xEnd = point2.x - math::multiplyDivide(point2.y - edgePtr->yEnd, delta.x, delta.y);
// clip edge list left and right
		signed16dot16 left, right;
		if (edgePtr->xStart <= edgePtr->xEnd) {
			left = edgePtr->xStart;
			right = edgePtr->xEnd;
		} else {
			left = edgePtr->xEnd;
			right = edgePtr->xStart;
		}
// this may construct 2 edges, a slanted one and a vertical one
		fidoPoint midEnd, midStart;
		if (left < clip.left) {	
			if (right <= clip.left) {
				midEnd.x = midStart.x = edgePtr->xStart = clip.left;
				midEnd.y = edgePtr->yEnd;
			 	midStart.y = midEnd.y + 1;
			} else {
				midEnd.y = s16dot16::roundDown(point1.y + math::multiplyDivide(clip.left - point1.x,
					delta.y, delta.x));
				midStart.y = midEnd.y + s16dot16::one;
				Assert(midStart.y <= edgePtr->yTrueEnd && midEnd.y >= edgePtr->yTrueStart);
				midEnd.x = point1.x + math::multiplyDivide(midEnd.y - point1.y, delta.x, delta.y);
				Assert(edgePtr->xTrueStart <= edgePtr->xTrueEnd ?
					midEnd.x <= edgePtr->xTrueEnd && midEnd.x >= edgePtr->xTrueStart : 
					midEnd.x >= edgePtr->xTrueEnd && midEnd.x <= edgePtr->xTrueStart);
				midStart.x = point1.x + math::multiplyDivide(midStart.y - point1.y, delta.x, delta.y);
				Assert(edgePtr->xTrueStart <= edgePtr->xTrueEnd ?
					midStart.x <= edgePtr->xTrueEnd && midStart.x >= edgePtr->xTrueStart : 
					midStart.x >= edgePtr->xTrueEnd && midStart.x <= edgePtr->xTrueStart);
			}
			if (clip.left >= edgePtr->xStart) {
				if (edgePtr > edgeBase) {
					Assert(edgePtr[-1].xStart == edgePtr[-1].xEnd);
					edgePtr[-1].yEnd = edgePtr[0].yEnd;
					edgePtr[-1].xEnd = edgePtr[0].xEnd;
					edgePtr[-1].xTrueStart = edgePtr[0].xTrueStart;
					edgePtr[-1].yTrueStart = edgePtr[0].yTrueStart;
					edgePtr[-1].xTrueEnd = edgePtr[0].xTrueEnd;
					edgePtr[-1].yTrueEnd = edgePtr[0].yTrueEnd;
					--edgePtr; // combine with previous straight edge
				}
				Assert(edgePtr - edgeBase + 1 < 4);
				edgePtr[1] = edgePtr[0];
				edgePtr[1].xEnd = edgePtr->xEnd;
				edgePtr->yEnd = midEnd.y;
				edgePtr->xStart = edgePtr->xEnd = clip.left;
				if (edgePtr->yStart <= edgePtr->yEnd)
					edgePtr++;
				edgePtr->xStart = midStart.x;
				edgePtr->yStart = midStart.y;
			} else {
				Assert(edgePtr - edgeBase + 1 < 4);
				edgePtr[1] = edgePtr[0];
				edgePtr->yEnd = midEnd.y;
				edgePtr->xEnd = midEnd.x;
				if (edgePtr->yStart <= edgePtr->yEnd)
					edgePtr++;
				edgePtr->yStart = midStart.y;
				edgePtr->xStart = edgePtr->xEnd = clip.left;
			}
		}
		if (right > clip.right) {
			if (left >= clip.right) {
				midEnd.x = midStart.x = edgePtr->xStart = clip.right;
				midEnd.y = edgePtr->yEnd;
			 	midStart.y = midEnd.y + 1;
			} else {
				midEnd.y = s16dot16::roundDown(point1.y + math::multiplyDivide(clip.right - point1.x,
					delta.y, delta.x));
				midStart.y = midEnd.y + s16dot16::one;
				Assert(midStart.y <= edgePtr->yTrueEnd && midEnd.y >= edgePtr->yTrueStart);
				midEnd.x = point1.x + math::multiplyDivide(midEnd.y - point1.y, delta.x, delta.y);
				Assert(edgePtr->xTrueStart <= edgePtr->xTrueEnd ?
					midEnd.x <= edgePtr->xTrueEnd && midEnd.x >= edgePtr->xTrueStart : 
					midEnd.x >= edgePtr->xTrueEnd && midEnd.x <= edgePtr->xTrueStart);
				midStart.x = point1.x + math::multiplyDivide(midStart.y - point1.y, delta.x, delta.y);
				Assert(edgePtr->xTrueStart <= edgePtr->xTrueEnd ?
					midStart.x <= edgePtr->xTrueEnd && midStart.x >= edgePtr->xTrueStart : 
					midStart.x >= edgePtr->xTrueEnd && midStart.x <= edgePtr->xTrueStart);
			}
			if (clip.right <= edgePtr->xStart) {
				if (edgePtr > edgeBase) {
					Assert(edgePtr[-1].xStart == edgePtr[-1].xEnd);
					edgePtr[-1].yEnd = edgePtr[0].yEnd;
					edgePtr[-1].xEnd = edgePtr[0].xEnd;
					edgePtr[-1].xTrueStart = edgePtr[0].xTrueStart;
					edgePtr[-1].yTrueStart = edgePtr[0].yTrueStart;
					edgePtr[-1].xTrueEnd = edgePtr[0].xTrueEnd;
					edgePtr[-1].yTrueEnd = edgePtr[0].yTrueEnd;
					--edgePtr; // combine with previous straight edge
				}
				Assert(edgeBase - edgeBase + 1 < 4);
				edgePtr[1] = edgePtr[0];
				edgePtr->xStart = edgePtr->xEnd = clip.right;
				edgePtr->yEnd = midEnd.y;
				if (edgePtr->yStart <= edgePtr->yEnd)
					edgePtr++;
				edgePtr->xStart = midStart.x;
				edgePtr->yStart = midStart.y;
			} else {
				Assert(edgeBase - edgeBase + 1 < 4);
				edgePtr[1] = edgePtr[0];
				edgePtr->xEnd = midEnd.x;
				edgePtr->yEnd = midEnd.y;
				if (edgePtr->yStart <= edgePtr->yEnd)
					edgePtr++;
				edgePtr->xStart = edgePtr->xEnd = clip.right;
				edgePtr->yStart = midStart.y;
			}
		}
		if (edgePtr->yStart <= edgePtr->yEnd)
			edgePtr++;
	}
	edge = edgePtr;
}


void edgeBuilder::sort4Corners()
{
	for (int counter = 0; counter < 4; counter++)
		matrixFixUp.map(corners[counter]);		
	// sort points
	static const struct {
		int first;
		int second;
	} sortPairs[] = {{1, 0}, {3, 2}, {2, 0}, {3, 1}, {2, 1}};
	for (int index = 0; index < sizeof(sortPairs) / sizeof(sortPairs[0]); index++)
		if (corners[sortPairs[index].first].lessThan(corners[sortPairs[index].second]))
			corners[sortPairs[index].first].exchange(corners[sortPairs[index].second]);
}


void edgeBuilder::constructEdgePointOrder(int* leftPtr, int* rightPtr)
{	
	bool oneToLeft = corners[1].x < corners[3].x - math::multiplyDivide(corners[3].y - corners[1].y,
			corners[3].x - corners[0].x, corners[3].y - corners[0].y);
	bool twoToLeft = corners[2].x < corners[3].x - math::multiplyDivide(corners[3].y - corners[2].y,
			corners[3].x - corners[0].x, corners[3].y - corners[0].y);
	*leftPtr++ = 0;
	*rightPtr++ = 0;
	if (oneToLeft)
		*leftPtr++ = 1;
	else
		*rightPtr++ = 1;
	if (twoToLeft)
		*leftPtr++ = 2;
	else
		*rightPtr++ = 2;
	*leftPtr++ = 3;
	*rightPtr++ = 3;
}


void edgeBuilder::constructEdges(const int edgeCount, const fidoRectangle& clip, 
	const int* edgePoints, fidoEdge* const firstEdge, fidoEdge*& lastEdge)
{
	fidoEdge* edgePtr = firstEdge;
	int index = 0;
	do {
		constructEdges(edgePtr, firstEdge, clip, corners[edgePoints[index]], corners[edgePoints[index + 1]]);
		Assert(edgePtr - firstEdge < edgeCount || edgePtr - firstEdge == edgeCount && edgePoints[index + 1] == edgeCount - 1);
	} while (edgePoints[++index] != edgeCount - 1);
	lastEdge = edgePtr;
}


// separated into edge list builder and cel list builder so edge list can be built from other convex geometries
void yMapBuilder::draw(const fidoBitMap& map, const fidoMapping& originalMatrix, const fidoRectangle& clip)
{
	edgeBuilder build(map);
	if (map.xSize >= 256) {	// !!! 256 should be constant somewhere
		// divide the original bitmap into halves and try again
		fidoBitMap xHalf = map;
		xHalf.xSize = build.xOneHalf;
		draw(xHalf, originalMatrix, clip);
		xHalf.xSize = map.xSize - build.xOneHalf;
		xHalf.bitsPtr += build.xOneHalf >> build.xShift;
		fidoMapping nearX;
		nearX.setIdentity();
		nearX.translate(ff(build.xOneHalf), 0);
		nearX.concatenate(originalMatrix);
		draw(xHalf, nearX, clip);
		return;
	}
	if (map.ySize >= 256) {
		fidoBitMap yHalf = map;
		yHalf.ySize = build.yOneHalf;
		draw(yHalf, originalMatrix, clip);
		yHalf.ySize = map.ySize - build.yOneHalf;
		yHalf.bitsPtr += build.yOneHalf * (map.rowLongs + 1);
		fidoMapping nearY;
		nearY.setIdentity();
		nearY.translate(0,  ff(build.yOneHalf));
		nearY.concatenate(originalMatrix);
		draw(yHalf, nearY, clip);
		return;
	}
	// construct four points of bitmap

// this offset allows for more range in the cel elements by moving (0, 0) from the top left of the bitmap
// to the center
	build.corners[1].y = build.corners[3].y =
		(build.corners[0].y = build.corners[2].y = -ff(build.yOneHalf)) + ff(map.ySize);
	build.corners[2].x = build.corners[3].x = 
		(build.corners[0].x = build.corners[1].x = -ff(build.xOneHalf)) + ff(map.xSize);
// compensate for offsetting corners
	build.build(originalMatrix);
	// compute points of rectangle through matrix
	build.sort4Corners();
// is it completely clipped out?
// !!! need to add check for completely clipped out on left or right side
	if (build.corners[0].y >= clip.bottom || build.corners[3].y <= clip.top)
		return;
	int leftEdgePoints[4];
	int rightEdgePoints[4];
	build.constructEdgePointOrder( leftEdgePoints, rightEdgePoints);	
	// construct edge list
	build.constructEdges(4, clip, leftEdgePoints, build.leftEdges, build.leftEdgeEnd);
	build.constructEdges(4, clip, rightEdgePoints, build.rightEdges, build.rightEdgeEnd);
	// walk edge list, constructing triangles and trapeziums
	constructEdgeCel(map, build);
}


void yMapBuilder::constructEdgeCel(const fidoBitMap& map, NonDebugCode(const) edgeBuilder& build)
{
	NonDebugCode(const) fidoEdge* leftEdgePtr = build.leftEdges;
	NonDebugCode(const) fidoEdge* rightEdgePtr = build.rightEdges;
	Assert(leftEdgePtr->yStart == rightEdgePtr->yStart);
	fidoRectangle bitBounds;
	bitBounds.top = leftEdgePtr->yStart;
	while (leftEdgePtr < build.leftEdgeEnd) {
		Assert(rightEdgePtr < build.rightEdgeEnd);
		Assert(bitBounds.top >= leftEdgePtr->yStart && bitBounds.top >= rightEdgePtr->yStart &&
			bitBounds.top <= leftEdgePtr->yEnd && bitBounds.top <= rightEdgePtr->yEnd);
		bitBounds.bottom = leftEdgePtr->yEnd < rightEdgePtr->yEnd ? leftEdgePtr->yEnd : rightEdgePtr->yEnd;
		Assert(bitBounds.top <= bitBounds.bottom);
		if (bitBounds.top < bitBounds.bottom && (leftEdgePtr->xStart != leftEdgePtr->xEnd ||
			rightEdgePtr->xStart != rightEdgePtr->xEnd || leftEdgePtr->xStart != rightEdgePtr->xStart))
		{
			bitBounds.left = leftEdgePtr->xStart;
			if (bitBounds.top > leftEdgePtr->yStart && bitBounds.left != leftEdgePtr->xEnd)
				bitBounds.left += math::multiplyDivide(bitBounds.top - leftEdgePtr->yStart,
					leftEdgePtr->xTrueEnd - leftEdgePtr->xTrueStart, leftEdgePtr->yTrueEnd - leftEdgePtr->yTrueStart);
			bitBounds.right = rightEdgePtr->xStart;
			if (bitBounds.top > rightEdgePtr->yStart && bitBounds.right != rightEdgePtr->xEnd)
				bitBounds.right += math::multiplyDivide(bitBounds.top - rightEdgePtr->yStart,
					rightEdgePtr->xTrueEnd - rightEdgePtr->xTrueStart, rightEdgePtr->yTrueEnd - rightEdgePtr->yTrueStart);
			Assert(bitBounds.left <= bitBounds.right);
			celRecord* cel = celPool->next();
			cel->mode = map.mode;
			cel->convert(yEntry::maxi);
			cel->globalAlpha = map.alpha;
			cel->set_textureRowLongs(map.rowLongs);
			cel->uMask = map.uMask;
			cel->vMask = map.vMask;
			cel->set(map.colors);	// codebook base
			cel->set(cel->uMask != -1 || cel->vMask != -1 ? map.bitsPtr : 
				&map.bitsPtr[build.yOneHalf * (map.rowLongs + 1) + (build.xOneHalf >> build.xShift)]);	// texture base
			cel->set_xLeftStart(s16dot16::roundTo10dot10(bitBounds.left + originX));
			cel->set_xRightStart(s16dot16::roundTo10dot10(bitBounds.right + originX));
		#ifdef DEBUG
			leftEdgePtr->final[leftEdgePtr->finals].cel = cel;
			rightEdgePtr->final[rightEdgePtr->finals].cel = cel;
		#endif
			cel->construct(build.inverse, map, leftEdgePtr, rightEdgePtr);
				// !!! could add logic to reconfigure edges so this can try again
				// compute top and bottom mid-x values
				// compute reasonable mid-y value
				// compute new matrix translation (more complicated if perspective?)
				// while this could be really sophisticated, for now, just recall root routine with smaller bitmaps
			int intTop = s16dot16::round(bitBounds.top - 1);
			int intBottom = s16dot16::round(bitBounds.bottom);
			cel->setLastCelBit();
			add(cel, intTop, intBottom);
			signed16dot16 bottomLeft = bitBounds.left + ((intBottom - intTop) * cel->dxLeft <<
				s16dot16::shift - s10dot10::shift);
			if (bottomLeft < bitBounds.left)
				bitBounds.left = bottomLeft;
		//	bitBounds.top = ff(intTop);
			signed16dot16 bottomRight = bitBounds.right + ((intBottom - intTop) * cel->dxRight <<
				s16dot16::shift - s10dot10::shift);
			if (bottomRight < bitBounds.right)
				bitBounds.right = bottomRight;
		//	bitBounds.bottom = ff(intBottom);
			bounds.onion(bitBounds);
		} // end of if bounds.top < bounds.bottom
	// advance to next edge pair
		bitBounds.top = bitBounds.bottom + s16dot16::one;
		if (bitBounds.bottom >= leftEdgePtr->yEnd)
			leftEdgePtr++;
		if (bitBounds.bottom >= rightEdgePtr->yEnd)
			rightEdgePtr++;
	}
	Assert(leftEdgePtr == build.leftEdgeEnd && rightEdgePtr == build.rightEdgeEnd);
}

enum { s4 = 3, s8 = 2, s16 = 1, s32 = 0, sUnused = -1};
const char shiftCount[16] = {
	s8, s16, s8, s16,
	s8, s32, s8, s32,
	s4, sUnused, s4, s4,
	s4, s4, sUnused, s8};

