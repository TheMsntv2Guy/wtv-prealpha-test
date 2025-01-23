#include "fidoSim.h"

// high level goal:
// support move, scrolling, add new layer, delete layer
// scrolling may need to delete or compress stuff that goes outside of physical area
// big cases: thumbnails of recent; currently overlay is about 350 high
// scroll of page also about 350 pixels
// after scroll, new page needs to be constructed from old:
	// break first page into part to be scrolled away, part to be saved
	// put part to be saved in its own layer
	// add to part to be saved new part to display
	// scroll
	// remove part that was scrolled away from display list
// for NTSC, there is a margin of 271 below the bottom line of the screen, and 273 above
// perhaps a more sane approach from a memory point of view is to limit bands of data to 256 pixels (or fewer for pal)
	// 128 would be pushing it but could still work for horizontal NTSC and PAL (may have 1 pixel overlap on right on PAL)
// this would probably be a good thing in general
// then, display list parts get added and removed to the scrolling layer more chunkily
// the scroll routine is the same as the indented steps above, repeated

#define exchange(a, b, c) do { c temp = a; a = b; b = temp; } while (0)

// !!! can't tell a start from an end without parsing entire ymap
#if 0	// the below is all out of date
it needs to be replaced by routines that manage
	layers
		add/insert
		delete
	cels
		move x
	cels/yMap
		move y
assume that a layer is a collection of cels
cels are not individually edited (except to be moved)
once the cels are removed from the visible layer, they can safely be disposed
creating cels is fast enough that it is not worth keeping them once they have scrolled off the screen

int yMapContainer::discard(celArrayPool* pool, unsigned short* ptr, activeCels* activeCelBits, int limit)
{
	int disposed = 0;
	unsigned short y = *ptr++;
	while (y < limit) {
		int index, realIndex;
		do {
			index = *ptr++;
			realIndex = index & ~yMap::terminal;
			bool end = activeCelBits->check(realIndex);
			celRecord* cel = &pool->cels[realIndex];
	//		Assert(cel != nil);
			activeCelBits->toggle(realIndex);
			if (end) {
				pool->remove(cel);	// !!! alternative is to slide all down, and fix up celList(s)
			}
			// !!! better yet, copy the cels still used to next available location
			// !!! have to do in 2 passes, to detect end pair, or do xor bit array
		} while (index == realIndex);
		if (*ptr == yMap::pad)
			ptr++;
		y += *ptr++;
	}
	// try shrinking cels disposed
	return disposed;
}

int yMapContainer::discardLeft(celArrayPool* /* cels*/, int /* limit */)
{
	return 0;
}

int yMapContainer::discardRight(celArrayPool* /*cels*/, int /* limit */)
{
	return 0;
}

// !!! cel starting at 0 (or 1) is special; it is assumed to go infinitely up
// !!! if cel crosses some boundary, resize it to move top down
// !!! do we keep track of real top somewhere to undo resize?
int yMapContainer::discardAbove(celArrayPool* pool, int limit)
{
	activeCels activeCelBits;
	activeCelBits.clear();
	unsigned short* ptr = map.base;
	int discarded = discard(pool, ptr, &activeCelBits, limit);
	if (discarded) {
		// !!! copy remaining cels to next unused location
	}
	return discarded;
}

// ? never need to move bottom up?
int yMapContainer::discardBelow(celArrayPool* pool, int limit)
{
	activeCels activeCelBits;
	activeCelBits.clear();
	unsigned short* ptr = map.base;
	unsigned short y = *ptr++;
	while (y < limit) {
		activeCelBits.update(ptr);
		y += *ptr++;
	}
	return discard(pool, ptr, &activeCelBits, yMap::beyondLast - 1);
}
#endif

#if 0
// !!! a more sensible interface takes a newly built fido list and goes from one to the other
// this way, the intermediate operations do not need to delete cels, merely move them out of the y-list
// of course, this must operate on a copy of the yMaps that fido is currently
// !!! add a parameter representing new cels to add
void fidoData::scrollUp(displayList* next)
{
	// choose correct ymap to update based on which one is next 
	if (currentScanline & 1 ^ currentScanline < fidoLimits::scrollLine) { // odd ymap, not about to switch maps
		scrollUp(next, yMap0);
		scrollUp(next, yMap1);
	} else {
		scrollUp(next, yMap1);
		scrollUp(next, yMap0);
	}
	// once done, move old cels above top into next
}

void fidoData::scrollUp(displayList* next, yMap* map)
	unsigned short* ptr = map->base;
	unsigned short y = *ptr++;	// !!! always zero?
	// need parallel list at same time so that real bounds can be accessed
	do {	// active parts of loop are synchronized w/ vblank
		while (theresTime())	// wait for vblank !!! this could benefit from knowing what scanline is being displayed
			// !!! and which ymap is being scanned
			next->idle();
		// resize cels above top
		*ptr -= newTop;	// 
		if (y < limit)	// if this is the 
		// move cels below top
		
		// add cels on bottom
		ptr++;
	} while ();
}

// how scrollUp is used:
/* 
{	displayList next;
	do
		next.draw(); 
	while ();	// .. until all parts of new screen are drawn
	next.idle() = callOut;	// can call out on idle
	fidoData::scrollUp(&next, callOut);	// .. scrolls, puts old into next so scope will dispose
}	
*/
#endif

#if 0
// scanning! Steve suggests that it work on the data directly at vblank time, minus the
// time required to update the cel list. I am skeptical.
void fidoData::scroll(celArrayPool* pool, int dx, int dy)
{
	yMapContainer* map0Copy = yMap0->copy();
	yMapContainer* map1Copy = yMap1->copy();
	pool->list = celBase->copy();
	int newLast = lastCel;
	if (dx < 0) {
		int leftLimit = fidoLimits::visibleLeft - dx;
		map0Copy->discardLeft(pool, leftLimit);
		map1Copy->discardLeft(pool, leftLimit);
	} else if (dx > 0) {
		int rightLimit = fidoLimits::visibleRight - dx;
		map0Copy->discardRight(pool, rightLimit);
		map1Copy->discardRight(pool, rightLimit);
	}
	if (dy < 0) {
		// discard the y-entries and cels that fall off screen
		// !!! assume that each cel is used only once in either y-map
		int topLimit = fidoLimits::visibleTop - dy;
		newLast -= map0Copy->discardAbove(pool, topLimit);
		newLast -= map1Copy->discardAbove(pool, topLimit);
	} else if (dy > 0) {
		int bottomLimit = fidoLimits::visibleBottom - dy;
		newLast -= map0Copy->discardBelow(pool, bottomLimit);
		newLast -= map1Copy->discardBelow(pool, bottomLimit);
	}
	if (dy & 1) {
		exchange(map0Copy, map1Copy, yMap*); // !!! worth using an exchange macro?
		map0Copy->base[0] ^= 1; // flip from one to zero, and vice versa
		map1Copy->base[0] ^= 1;
	} 
	// !!! at some safe time, set up fido registers to new copys of cels and maps
	exchange(yMap0, map0Copy, yMap*);
	exchange(yMap1, map1Copy, yMap*);
	celList* oldList = celBase;
	celBase = pool->list;
// end of safe time
	lastCel = newLast;
	delete(oldList);
}
#endif


celRecord* fidoData::pick(int /* x */, int /* y */)
{
	// !!! search y list for possibilities
	return nil;	
}

void fidoData::move(celRecord* cel, int dx, int dy)
{
	int x, y;
	cel->get(x, y);
	remove(cel);
	add(cel, x + dx, y + dy);
}




