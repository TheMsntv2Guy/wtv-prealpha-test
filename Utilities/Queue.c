// ===========================================================================
//	Queue.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __QUEUE_H__
#include "Queue.h"
#endif




//===========================================================================

Queue::Queue()
{
	fQHead = nil;
}

Queue::~Queue()
{
	while (fQHead != nil)
		DestroyEntry(fQHead);
}

void Queue::AddEntry(QEntry *q, Boolean atHead)
{
	// Add an entry to the queue
	
	q->header.qSignature = QuadChar('q','S','i','g');
	q->header.next = 0;
	q->header.prev = 0;

	if (atHead || !fQHead)
	{
		q->header.next = fQHead;
		if (fQHead)
			fQHead->header.prev = q;
		fQHead = q;
	}
	else
	{
		QEntry *qq = fQHead;
		while (qq->header.next)
			qq = qq->header.next;
		qq->header.next = q;
		q->header.prev = qq;
	}
}

Error Queue::CreateEntry(Boolean atHead, long param1, long param2, long param3)
{
	// Allocate and Enqueue
	
	QEntry *q = (QEntry *)AllocateTaggedMemory(sizeof(QHeader), "QEntry");
	
	if (q == nil)
		return kLowMemory;
		
	q->header.param1 = param1;
	q->header.param2 = param2;
	q->header.param3 = param3;
	AddEntry(q, atHead);
	
	return kNoError;
}

Error Queue::RemoveEntry(QEntry *q)
{
	// Remove an entry from the queue
	
	Assert(q->header.qSignature == QuadChar('q','S','i','g'));
	
	if (q->header.prev != nil)
		(q->header.prev)->header.next = q->header.next;
	else
		fQHead = q->header.next;

	if (q->header.next != nil)
		(q->header.next)->header.prev = q->header.prev;
	q->header.next = q->header.prev = 0;
	
	return kNoError;
}

Error Queue::DestroyEntry(QEntry *q)
{
	// Deallocate and dequeue
	
	RemoveEntry(q);
	FreeTaggedMemory(q, "QEntry");
	return kNoError;
}

QEntry* Queue::GetHead()
{
	return fQHead;
}

void Queue::PrintElements()
{
#ifdef DEBUG
	QEntry *q = fQHead;
	short	i = 0;

	while (q != nil)
	{
		Message(("%2d>%8X P[%8X] N[0x%8X] <%8X><%8X><%8X>",
			i,(long)q,(long)q->header.prev,(long)q->header.next,q->header.param1,
			q->header.param2,q->header.param3));
		i++;
		q = q->header.next;
	}
#endif
}
