// ===========================================================================
//	Region.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef _LIMITS
#include "limits.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __REGION_H__
#include "Region.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

// =============================================================================
// Class Region.

Region::Region()
{
	fList.SetDataSize(sizeof (Rectangle));
	fList.SetListIncrement(4);
}

Region::~Region()
{
	Reset();
}

void Region::Add(const Rectangle* r)
{
	if (IsError(r == nil))
		return;
		
	if (r->right <= r->left || r->bottom <= r->top)
		return;
	
	long count = fList.GetCount();
	long i;
	Rectangle 	regionBounds;
	
	GetBounds(&regionBounds);
	
	/* if the new rectangle contains the whole region, 
		then replace region with new rectangle */
	   
	if ( RectangleContainedInRectangle(&regionBounds,r) ) {
		Reset();
		fList.Add(r);
		return;
	}
	for (i = 0; i < count; i++) {
		Rectangle *ir = (Rectangle*)fList.At(i);
		
		/* if the new rect is completely inside an existing one, ignore it */
		
		if ( RectangleContainedInRectangle(r,ir) )
			return;
			
		/* if it's a taller version of an existing rect, grow vertically */
		
		if ( r->right == ir->right && r->left == ir->left ) {
			if ( r->top < ir->top )
				ir->top = r->top;
			if ( r->bottom > ir->bottom )
				ir->bottom = r->bottom;
			return;
		}
		
		/* if it's a wider version of an existing rect, grow it horizontally */
		
		if ( r->top == ir->top && r->bottom == ir->bottom ) {
			if ( r->left < ir->left )
				ir->left = r->left;
			if ( r->right > ir->right )
				ir->right = r->right;
			return;
		}
	}
	
	/* default case - add the new rectangle to the region */
	
	fList.Add(r);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void Region::Add(const Region* region)
{
	if (IsError(region == nil))
		return;
		
	fList.AddAll(&region->fList);
}
#endif

static inline Boolean PointInRect(Ordinate x, Ordinate y, const Rectangle* r)
{
	return x >= r->left && x <= r->right && y >= r->top && y <= r->bottom;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean Region::Contains(Ordinate x, Ordinate y) const
{
	long count = fList.GetCount();
	long i;
		
	for (i = 0; i < count; i++)
		if (PointInRect(x, y, (Rectangle*) fList.At(i)))
			return true;
	
	return false;
}
#endif

void Region::GetBounds(Rectangle *rect) const
{
	long count = fList.GetCount();
	long i;
	
	rect->top = rect->left = LONG_MAX;
	rect->bottom = rect->right = LONG_MIN;
	
	for (i = 0; i < count; i++) {
		Rectangle r = *(Rectangle*)fList.At(i);
		rect->top = MIN(rect->top, r.top);
		rect->left = MIN(rect->left, r.left);
		rect->bottom = MAX(rect->bottom, r.bottom);
		rect->right = MAX(rect->right, r.right);
	}
}

#if defined(DEBUG) && defined(SIMULATOR)
void Region::Draw(short thickness, Color color)
{
	long count = fList.GetCount();
	long i;
	
	for (i = 0; i < count; i++) {
		Rectangle bounds = *(Rectangle*)fList.At(i);
		FrameRectangle(gScreenDevice, bounds, thickness, color, 0x40);
	}
}
#endif

Boolean Region::Intersects(const Rectangle* r) const
{
	long count = fList.GetCount();
	long i;
	
	for (i = 0; i < count; i++)
		if (RectanglesIntersect(r, (Rectangle*)fList.At(i)))
			return true;

	return false;
}

Boolean Region::IsEmpty() const
{
	return fList.GetCount() == 0;
}

Boolean Region::IsRectangle() const
{
	long count = fList.GetCount();
	long i;
	Rectangle r;
	Rectangle bounds;
	
	if ( count <= 1 )
		return true;
	GetBounds(&bounds);
	for (i = 0; i < count; i++) {
		r = *(Rectangle*)fList.At(i);
		if ( r.left != bounds.left || r.right != bounds.right )
			return false;
	}
	return true;
}

Region* Region::NewCopy() const
{
	Region* copy = new(Region);
	long count = fList.GetCount();
	long i;
	
	for (i = 0; i < count; i++)
		copy->Add((Rectangle*)fList.At(i));
	
	return copy;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
DataIterator* Region::NewIterator() const
{
	return fList.NewIterator();
}
#endif

void Region::Offset(Ordinate x, Ordinate y)
{
	long count = fList.GetCount();
	long i;
	
	for (i = 0; i < count; i++) {
		Rectangle* r = (Rectangle*)fList.At(i);
		r->top += y;
		r->left += x;
		r->bottom += y;
		r->right += x;
	}
}

void Region::Reset()
{
	fList.DeleteAll();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean Region::Contains(const Rectangle* bounds) const
{
	Rectangle regionBounds;
	
	GetBounds(&regionBounds);
	return RectangleContainedInRectangle(bounds, &regionBounds);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean Region::IsWithin(const Rectangle* bounds) const
{
	Rectangle regionBounds;
	
	GetBounds(&regionBounds);
	return RectangleContainedInRectangle(&regionBounds, bounds);
}
#endif


int Region::GetSubRegionCount() const
{
	long count = fList.GetCount();
	long i;
	long srCount = 1;
	Rectangle r,nr;
	
	if ( IsRectangle() )
		return 1;
	r = *(Rectangle*)fList.At(0);
	
	for (i = 1; i < count; i++) {
		nr = *(Rectangle*)fList.At(i);
		if ( nr.left >= r.right || nr.right < r.left )
			srCount++;
		r = nr;
	}
	return srCount;
}

Region *Region::GetSubRegion(int index) const
{
	long count = fList.GetCount();
	long i;
	long srIndex = 0;
	long srCount = GetSubRegionCount();
	Rectangle r,nr;


	if ( srCount == 1 || index >= srCount )
		return nil;
	Region* copy = new(Region);
	

	r = *(Rectangle*)fList.At(0);
	if ( index == srIndex )
		copy->Add(&r);
	
	for (i = 1; i < count; i++) {
		nr = *(Rectangle*)fList.At(i);
		if ( nr.left >= r.right || nr.right < r.left )
			srIndex++;
		if ( index == srIndex )
			copy->Add(&nr);
		r = nr;
		if ( srIndex > index )	
			break;
	}
	Assert(copy->GetSubRegionCount() == 1);
	return copy;
}




#ifdef TEST_SIMULATORONLY
void Region::Test()
{
	Region region;
	Rectangle r;
	
	Assert(region.fList.GetDataSize() == sizeof (Rectangle));
	Assert(region.IsEmpty());
	
	r.top = r.left = 0;
	r.bottom = r.right = 10;
	region.Add(&r);
	Assert(!region.IsEmpty());
	Assert(region.Intersects(&r));
	Assert(region.Contains(5, 5));
	Assert(!region.Contains(15, 15));
	
	r.top = r.left = 100;
	r.bottom = r.right = 110;
	region.Add(&r);
	Assert(region.Intersects(&r));
	Assert(region.Contains(5, 5));
	Assert(region.Contains(105, 105));
	Assert(!region.Contains(50, 50));
	Assert(!region.Contains(150, 150));
	
	r.top = r.left = 5;
	r.bottom = r.right = 15;
	Assert(region.Intersects(&r));

	r.top = r.left = -5;
	r.bottom = r.right = 5;
	Assert(region.Intersects(&r));

	r.top = r.left = -500;
	r.bottom = r.right = 500;
	Assert(region.Intersects(&r));

	r.top = r.left = 50;
	r.bottom = r.right = 60;
	Assert(!region.Intersects(&r));
	
	region.Reset();
	Assert(region.IsEmpty());
}
#endif /* TEST_SIMULATORONLY */

//==================================================================================
