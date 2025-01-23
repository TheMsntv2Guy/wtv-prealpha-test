#include "yList.h"

// flipping this value makes a no-op since yBottom < yTop
const long yEntry::termination = 0xffffe000;	// xored, should be ytop == 1, ybottom == 0

// a new entry is added to the yMap
yEntry& yMapBuilder::add(celRecord* cel, int top, int bottom)
{
	Assert(this);
	Assert(cel != nil);
	top += originY;
	bottom += originY;
	yEntryContainer& map = celPool->map;
	int index = celPool->getIndex(cel);
	if (map.map < map.end) {
		int lastTop, lastBottom, lastIndex;
		yEntry::hint lastHint;
		map.end[-1].get(lastBottom, lastTop, lastHint, lastIndex);
		celRecord* priorCel = &celPool->cels[lastIndex];
//  if the next consecutive slot has been used, let the old entry work
		if (top == lastTop && bottom == lastBottom &&
			index == lastIndex + priorCel->indexSize() &&
			map.lastCelBase == celPool->cels)
		{
			priorCel->clearLastCelBit();
			return map.end[-1];
		}
	}
	map.lastCelBase = celPool->cels;
	yEntry& result = *map.end;
	map.lastFull = *map.end++ = yEntry(bottom, top, cel->get(), index);
	if (map.end >= &map.map[map.size])
		map.grow(cel, celPool);
	return result;
}


// whenever the ymap is extended, it must be in an wide open top-to-bottom yEntry
void yEntryContainer::grow(celRecord* last, celArrayPool* celPool)
{
	bool blockEnd = true;
	if (last)
		blockEnd = last->testLastCelBit();
// note that asking for another micro record might cause the cel array pool to grow
	// which adds an entry into the y map
	celRecord* cel = celPool->nextSpecialMicro();	// won't grow cel list
	// this makes the ymap change effective under all circumstances
// !!! ? should be constants, somewhere?
	Assert(end <= &map[size + 1]);
	*end = yEntry(510, -512, yEntry::micro, celPool->getIndex(cel));
// note that to delete this allocation, cels must be parsed to find 
//  a cel that reloads the ymap base; the y map entry that points to this also
//  points to the allocation
	end = map = AllocateArray(yEntry, size + 2);	// one extra for terminating list after add
	cel->set(map);
	if (blockEnd)
		cel->setLastCelBit();
// similarly, if the cel base is extended, it can't piggy tail onto the previous cel; it
// must have its own top-to-bottom yEntry
}


// this can't be inline currently, because of the circular nature of distroy()
celArrayPool::~celArrayPool()
{
	destroy();
}

// if called, the current pool does not have enough space for this cel
// there must one microcel remaining to switch this run to a new cel list
celRecord* celArrayPool::grow()
{
	// save old list
#if 0
	if (switchCelBase)	// hack to check for index range errors
		switchCelBase->xPosition = getIndex();
	else
		fido::data.lastCel = getIndex();
#endif
	Assert(lastCel >= cels);
	bool blockEnded = lastCel->testLastCelBit();
	if (list)	// !!! this is bogus; it doesn't copy the contents, not even the cel base
		list->list = list;	// (so far, it isn't really used anywhere)
	list = new celArrayPool(map, size);
	int indexLast = getIndex(last);
	Assert(list->cels);
	lastCel->setLastCelBit();
	last->set(list->cels);	// set cel to load new cel base (lastCel mode bit must be set)
	Assert(map.end <= &map.map[map.size]);
	*map.end++ = yEntry(510, -512, yEntry::micro, indexLast);
	last = cels = list->cels;
	// if previous cel lastCel mode bit was clear, duplicate the last y map entry
	if (map.end >= &map.map[map.size])
		map.grow(nil, this);
	if (blockEnded == false)
		*map.end++ = map.lastFull;
	return cels;
}


// special because it will never grow the block
celRecord* celArrayPool::nextSpecialMicro()
{
	Assert(this);
	Assert(last >= cels);
	Assert(last < (celRecord*) ((char*) &cels[size] + celRecord::microSize * 2));
	lastCel = last;
	last = last->nextMicro();
	return lastCel;
}

// at first glance, it may appear that there is an unused cel in the grow case.
// it is difficult to use, since after the cel base is changed by grow, the old
// base isn't around any more. The old base is needed to compute the index to the cel. 
celRecord* celArrayPool::nextMicro()
{
	Assert(this);
	Assert(last >= cels);
	Assert(last < &cels[size]);
	celRecord* result = last;
	last = last->nextMicro();
	if (last >= &cels[size]) {
		grow();
		return nextMicro();
	}
	return lastCel = result;
}


celRecord* celArrayPool::nextMini()
{
	Assert(this);
	Assert(last >= cels);
	Assert(last < &cels[size]);
	celRecord* result = last;
	last = last->nextMini();
	if (last >= &cels[size]) {
		grow();
		return nextMini();
	}
	return lastCel = result;
}


celRecord* celArrayPool::next()
{
	Assert(this);
	Assert(last >= cels);
	Assert(last < &cels[size]);
	celRecord* result = last++;
	if (&last[1] >= &cels[size]) {
		grow();
		return next();
	}
	return lastCel = result;
}
