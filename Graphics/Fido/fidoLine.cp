#include "fidoBitmap.h"
#include "fidoLine.h"
#include "yList.h"

// !!! lines could be a good test case for separating matrix generation from edge generation
// the lineDescription below specifies the edges; the matrix corresponding is unimportant because
// the line can be constrained to remain within one pixel, e.g. u, v, etc. == 0
// the more general solution below is an interesting direction for describing line color ramps, however
void yMapBuilder::draw(const fidoLine& line, const codebook& fillColor, signed16dot16 penWidth, const fidoMapping& matrix,
	const fidoRectangle& clip)
{
	const codebook* fillColorPtr = findFill(fillColor);
	fidoBitMap bitmap((const texture*) fillColorPtr, 1, 1, celRecord::directFullYUAV_1_a);
	bitmap.deleteTexture = firstFill(fillColorPtr);
	fidoPoint diff;
	diff.x = line.end.x - line.start.x;
	diff.y = line.end.y - line.start.y;
	const signed16dot16 length = s16dot16::root(diff.x, diff.y);
	penWidth >>= 1;
	fidoPoint penDisplacement;
	penDisplacement.x = math::multiplyDivide(penWidth, diff.y, length);
	penDisplacement.y = math::multiplyDivide(penWidth, diff.x, length);
	fidoPoint box[3];
	box[0].x = line.start.x + penDisplacement.x;
	box[0].y = line.start.y - penDisplacement.y; 
	box[1].x = line.end.x + penDisplacement.x;
	box[1].y = line.end.y - penDisplacement.y;
	box[2].x = line.start.x - penDisplacement.x;
	box[2].y = line.start.y + penDisplacement.y;
	fidoMapping map;
	map.array[0][0] = box[2].x - box[0].x;	map.array[0][1] = box[2].y - box[0].y;	map.array[0][2] = 0;
	map.array[1][0] = box[1].x - box[0].x;	map.array[1][1] = box[1].y - box[0].y;	map.array[1][2] = 0;
	map.array[2][0] = box[0].x;			map.array[2][1] = box[0].y;			map.array[2][2] = s2dot30::one;
	map.concatenate(matrix);
	draw(bitmap, map, clip);
}