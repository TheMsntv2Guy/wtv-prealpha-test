// ===========================================================================
//	Linkable.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __LINKABLE_H__
#include "Linkable.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif

//==================================================================================

Linkable::Linkable()
{
	fNext = nil;
	fPrevious = nil;
}

Linkable::~Linkable()
{
	Remove();
}

void Linkable::Add(Linkable* item)
{
	// Add new item to the end of the chain
	
	if (IsWarning(item == nil))
		return;
	
	Last()->AddAfter(item);
}

void Linkable::AddAfter(Linkable* item)
{
	// Add item after me
	
	if (IsWarning(item == nil))
		return;
	
	Linkable* oldNext = fNext;
	Linkable* first = item->First();
	
	fNext = first;
	first->fPrevious = this;
	if (oldNext != nil)
	{
		Linkable* last = item->Last();
		last->fNext = oldNext;
		oldNext->fPrevious = last;
	}
}

Linkable* Linkable::At(long index) const
{
	// Get item at index
	
	if (IsError(index < 0))
		return nil;
	
	Linkable* item = First();
	long i;
	
	for (i = 0; i < index && item; i++)
		item = item->Next();
	return item;
}

#ifdef SIMULATOR
long Linkable::Count() const
{
	// Number of items in list
	
	long count = 0;
	Linkable* item;
	for (item = First(); item; item = item->Next())
		count++;
	return count;
}
#endif

void Linkable::DeleteAll()
{
	// Delete everything in list
	
	Linkable* item;
	Linkable* nextItem;
	
	for (item = First(); item; item = nextItem) {
		nextItem = item->Next();
		delete(item);
	}
}

Linkable* Linkable::First() const
{
	// Get the first item in this list
	
	Linkable*	item = (Linkable*)this;
	
	for (; item->Previous() != nil; item = item->Previous())
		;
	return item;
}

Linkable* Linkable::Last() const
{
	// Get the last item in this list
	
	Linkable* item = (Linkable*)this;
	
	for (; item->Next(); item = item->Next())
		;
	return item;
}

void Linkable::Remove()
{
	// Remove from list
	
	if (fNext != nil)
		fNext->fPrevious = fPrevious;
	if (fPrevious != nil)
		fPrevious->fNext = fNext;
	fPrevious = fNext = 0;
}

void Linkable::RemoveAllAfter()
{
	// Remove everyone after me.
	
	while (fNext)
		fNext->Remove();
}

//==================================================================================

LinkedList::LinkedList()
{
}

LinkedList::~LinkedList()
{
}

void LinkedList::Add(Linkable* item)
{
	// Add new item to the end of the chain
	
	if (IsWarning(item == nil))
		return;

	if (fLast != nil) {
		fLast->AddAfter(item);
		fLast = item->Last();
	}
	else {
		fFirst = item->First();
		fLast = item->Last();
	}	
}

void LinkedList::AddAfter(Linkable* item, Linkable* after)
{
	// Add new item to the end of the chain
	
	if (IsWarning(item == nil))
		return;
		
	if (IsWarning(after == nil)) {
		Add(item);
		return;
	}

	if (fLast == after)
		fLast = item->Last();

	after->AddAfter(item);
}

#ifdef SIMULATOR
long LinkedList::Count() const
{
	// Number of items in list
	
	long count = 0;

	if (fFirst != nil)
		count = fFirst->Count();
		
	return count;
}
#endif

void LinkedList::DeleteAll()
{
	// Delete everything in list

	if (fFirst != nil) {
		fFirst->DeleteAll();
		fFirst = nil;
		fLast = nil;
	}	
}

Linkable* LinkedList::First() const
{
	return fFirst;
}

Linkable* LinkedList::Last() const
{
	return fLast;
}

void LinkedList::Remove(Linkable* item)
{
	// Remove from list
	
	if (fFirst == item)
		fFirst = item->Next();
	if (fLast == item)
		fLast = item->Previous();
	
	item->Remove();
}

//==================================================================================

