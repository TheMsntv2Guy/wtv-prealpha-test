#ifndef _FIDO_H_
#include "fido.h"
#endif

#ifndef _FIDO_CPP_
#define _FIDO_CPP_


inline displayList::displayList() : cels(yMap), builder(&cels), yEnd(nil), terminator(nil)
{
	yMapBase = yMap.map;
	celBase = cels.cels;
}

inline void displayList::reset()
{
	cels.init();
	celBase = cels.cels;
	builder.init();
	yMapBase = yMap.map;
}

inline void displayLeaf::setClip()
{
	realClip = clip;
	if (realClip.left < ff(fidoLimits::minX))
		realClip.left = ff(fidoLimits::minX);
	if (realClip.top < ff(fidoLimits::minY))
		realClip.top = ff(fidoLimits::minY);
	if (realClip.right > ff(fidoLimits::maxX))
		realClip.right = ff(fidoLimits::maxX);
	if (realClip.bottom > ff(fidoLimits::maxY))
		realClip.bottom = ff(fidoLimits::maxY);
}

inline void displayLeaf::setClip(signed16dot16 left, signed16dot16 top, signed16dot16 right, signed16dot16 bottom)
{
	clip.set(left, top, right, bottom);
	setClip();
}

inline void displayLeaf::setClip(fidoRectangle& userClip)
{
	clip = userClip;
	setClip();
}

inline void displayLeaf::getClip(fidoRectangle& userClip)
{
	userClip = clip;
}

inline fidoRectangle& displayLeaf::getClip()
{
	return clip;
}

// !!! needs work
inline const fidoFont* displayLeaf::find(fontTypes::family fontID, fontTypes::face style, int size)
{
	while ((font = fontDirectory::find(fontID, style, size)) == nil && size > fontTypes::size0) {
		size >>= 1;
		matrix.scale(ff(2), ff(2), matrix.name.moveX, matrix.name.moveY);
	}
	return font;
}

inline displayLeaf::displayLeaf() : font(nil)
{
	matrix.setIdentity();
	fillColor.setY(0, 0, 255, 0);
	clip.set(ff(fidoLimits::minX), ff(fidoLimits::minY), ff(fidoLimits::maxX), ff(fidoLimits::maxY));
	bounds.set(ff(fidoLimits::maxX), ff(fidoLimits::maxY), ff(fidoLimits::minX), ff(fidoLimits::minY));
}

inline void displayLeaf::add(const char* charStream, int length)
{
	if (length == 0)
		length = strlen(charStream);
	if (matrix.order() <= matrix.translated)
		builder.draw(charStream, length, font, fillColor, matrix.name.moveX, matrix.name.moveY, clip);
	else
		builder.draw(charStream, length, font, fillColor, matrix, clip);
	bounds.onion(builder.bounds);
}
	
inline void displayLeaf::add(const class fidoBitMap& map)
{
	if (matrix.order() <= matrix.scaled)
		builder.draw(map, matrix.name.moveX, matrix.name.moveY, 
			matrix.name.scaleX, matrix.name.scaleY, clip);
	else
		builder.draw(map, matrix, clip);
	bounds.onion(builder.bounds);
}
		
inline void displayLeaf::add(const fidoRectangle& rectangle)
{
	if (matrix.order() <= matrix.identity)
		builder.draw(rectangle, fillColor, clip);
	else
		builder.draw(rectangle, fillColor, matrix, clip);
	bounds.onion(builder.bounds);
}
		
inline void displayLeaf::add(const class fidoLine& line)
{
	builder.draw(line, fillColor, pen, matrix, clip);
	bounds.onion(builder.bounds);
}
	
inline void displayLeaf::add(const fidoRectangle& rectangle, const class fidoPattern& pattern)
{
	builder.draw(rectangle, pattern, clip);
	bounds.onion(builder.bounds);
}

inline void displayNode::add(const char* text, int length)
{
	displayLeaf::add(text, length);
}

inline void displayNode::add(const fidoBitMap& bits)
{
	displayLeaf::add(bits);
}

inline void displayNode::add(const class fidoLine& line)
{
	displayLeaf::add(line);
}

inline void displayNode::add(const fidoRectangle& rectangle)
{
	displayLeaf::add(rectangle);
}

inline void displayNode::add(const fidoRectangle& rectangle, const class fidoPattern& pattern)
{
	displayLeaf::add(rectangle, pattern);
}

#endif // _FIDO_CPP_