#include "fidoBitmap.h"
#include "yList.h"

void fidoRectangle::onion(fidoRectangle& operand)
{
	if (operand.left < left)
		left = operand.left;
	if (operand.top < top)
		top = operand.top;
	if (operand.right < right)
		right = operand.right;
	if (operand.bottom < bottom)
		bottom = operand.bottom;
}

const codebook* yMapBuilder::findFill(const codebook& match)
{
	if (fillList == nil || fillPointer >= &fillList->block[fillBlockList::max])
		grow(fillList);
	Assert(fillPointer >= fillList->block);
	Assert(fillPointer < &fillList->block[fillBlockList::max]);
	const codebook* last = fillPointer;
	fillBlockList* block = fillList;
	const codebook* check = fillList->block;	// first
	while (last > check && check->equal(match) == false) {
		if (++check >= last) {
			if ((block = block->next) == nil)
				break;
			check = block->block;
			last = &check[fillBlockList::max];
		}
	}
	if (last <= check) {
		check = fillPointer;
		*fillPointer++ = match;
	}
	return check;
}

// there's a cheat in bitmap draw that pushes steps to zero if bitmap is 1 dimensional

void yMapBuilder::draw(const fidoRectangle& rectangle, const codebook& fillColor, const fidoRectangle& clip)
{
	// turn current color into entry in fill list
	// !!! could return color hint to caller, address of fill so that fill list does not have to be
	// searched for match	
	const codebook* fillColorPtr = findFill(fillColor);
	// construct bitmap
	fidoBitMap bitmap((const texture*) fillColorPtr, 1, 1, celRecord::directFullYUAV_1_a);
	bitmap.deleteTexture = firstFill(fillColorPtr);
	// construct position and scale from rectangle
	draw(bitmap, rectangle.left, rectangle.top, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, clip);
	
}

void yMapBuilder::draw(const class fidoRectangle& rectangle, const codebook& fillColor, const fidoMapping& matrix,
	const fidoRectangle& clip)
{
	const codebook* fillColorPtr = findFill(fillColor);
	fidoBitMap bitmap((const texture*) fillColorPtr, 1, 1, celRecord::directFullYUAV_1_a);
	bitmap.deleteTexture = firstFill(fillColorPtr);
	// construct mapping from rectangle and mapping
	if (matrix.order() <= matrix.translated) {
		draw(bitmap, matrix.name.moveX + rectangle.left, matrix.name.moveY + rectangle.top,
			rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, clip);
	} else if (matrix.order() <= matrix.scaled) {
		draw(bitmap, matrix.name.moveX + s16dot16::multiply(rectangle.left, matrix.name.scaleX),
			matrix.name.moveY + s16dot16::multiply(rectangle.top, matrix.name.scaleY),
			s16dot16::multiply(rectangle.right - rectangle.left, matrix.name.scaleX),
			s16dot16::multiply(rectangle.bottom - rectangle.top, matrix.name.scaleY), clip);
	} else {
		fidoMapping rectMap;
	// !!! need constructor for mapping
		rectMap.name.scaleX = rectangle.right - rectangle.left;	rectMap.name.skewY = 0;		rectMap.name.u = 0;
		rectMap.name.skewX = 0;		rectMap.name.scaleY = rectangle.bottom - rectangle.top;	rectMap.name.v = 0;
		rectMap.name.moveX = rectangle.left;		rectMap.name.moveY = rectangle.top; 	rectMap.name.w = s2dot30::one;
		rectMap.concatenate(matrix);
		draw(bitmap, rectMap, clip);
	}
}
