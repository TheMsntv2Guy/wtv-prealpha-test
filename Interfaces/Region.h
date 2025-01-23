// ===========================================================================
//	Region.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================
#ifndef __REGION_H__
#define __REGION_H__

#ifndef __GRAPHICS_H__
#include "Graphics.h"			/* Color, Ordinate, Rectangle */
#endif
#ifndef __LIST_H__
#include "List.h"				/* DataIterator, DataList */
#endif




//==================================================================================
// Region class to collect rectangles.

class Region {
public:
							Region();
	virtual					~Region();
							
	virtual void			GetBounds(Rectangle*) const;
	Boolean					IsEmpty() const;
	Boolean					IsRectangle() const;

	void					Add(const Rectangle*);
	void					Add(const Region*);
	Region*					NewCopy() const;
	Boolean					Contains(Ordinate x, Ordinate y) const;
	Boolean					Contains(const Rectangle*) const;
	long					GetCount() const;
	Boolean					IsWithin(const Rectangle* bounds) const;
	Boolean					Intersects(const Rectangle*) const;
	DataIterator*			NewIterator() const;	// iterates through Rectangle*
	void					Offset(Ordinate x, Ordinate y);
	const Rectangle*		RectangleAt(long index) const;
	void					Reset();
	int						GetSubRegionCount() const;
	Region*					GetSubRegion(int indx) const;
#ifdef SIMULATOR
	void					Draw(short thickness, Color color);
#endif /* SIMULATOR */
#ifdef TEST_SIMULATORONLY
	static void				Test();
#endif /* TEST_SIMULATORONLY */

protected:
	DataList				fList;
};

//==================================================================================

inline long Region::GetCount() const
{
	return fList.GetCount();
}

inline const Rectangle* Region::RectangleAt(long index) const
{
	return (Rectangle*)fList.At(index);
}

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Region.h multiple times"
	#endif
#endif
