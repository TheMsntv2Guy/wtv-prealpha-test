#include "fido.h"
#include "fidoSim.h"

// only 1 display list can be drawn at a time.
void displayList::draw()
{
	Assert(this);
	// terminate this list if it is open ended (if it does not already have either a terminator or a cel switch)
	if (yEnd == nil) {	// never been terminated before
		if (terminator == nil)
			terminator = builder.terminate();
	} else if (yEnd != yMap.end) {	// has been terminated before, but new stuff has been added
	// if stuff has been added, wait for vblank so the new stuff can appear	
		spot::waitForVBlank();
	// re-add the terminator at the end of the list
		yMap.end->full = terminator->full;
	// turn the old terminator into a no-op, by changing yMap top/bottom to inverted values
		terminator->swap();
		terminator = yMap.end++;
	}
	yEnd = yMap.end;
}

// cels added to the end of node will follow the cels that switch the yMap and celBase back to the parent
// for these cels to become active, the child's restore entry needs to be moved to the new end of the list
yEntry& displayNode::add(displayNode& child)
{
	// add new cels to the parent to set the child's y list and cel base
	celRecord* celPtr = cels.nextMicro();
	celPtr->set(child.yMap.map);
	cels.nextMicro()->set(child.cels.cels);
	// add the child to the parent's y list
	int intTop = s16dot16::round(child.bounds.top);
	int intBottom = s16dot16::round(child.bounds.bottom);
	yEntry& value = builder.add(celPtr, intTop, intBottom);
	// update the parent's bounds to include the child
	bounds.onion(child.bounds);
	// append resetting the parent's y list and cel base to the end of the child
	celPtr = child.cels.nextMicro();
	celPtr->set(yMap.end);
	child.cels.nextMicro()->set(cels.cels);
	Assert(child.yEnd == nil);
	Assert(child.terminator == nil);
	child.terminator = &child.builder.add(celPtr, intTop, intBottom);
	child.yEnd = child.yMap.end;
	return value;
}

void displayNode::enable(yEntry& entry)
{
	Assert(entry.bits.top > entry.bits.bottom);
	entry.swap();
}

// !!! in real running version, enable and disable could be defined to be the same
void displayNode::disable(yEntry& entry)
{
	Assert(entry.bits.top < entry.bits.bottom);
	entry.swap();
}

void displayRoot::draw()
{	
	displayList::draw();
	fido::init(yMapBase, celBase); 
	simulate();
}

void 	spot::waitForVBlank()
{
}
