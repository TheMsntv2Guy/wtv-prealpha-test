// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#include "Headers.h"

#ifdef DEBUG

struct ObservationEntry
{
	const void*	fPointee;
	const void*	fPointer;
	const char*	fFieldName;
};
typedef struct ObservationEntry ObservationEntry;

#define kObservationEntries		2048
static ObservationEntry		gObservationEntries[kObservationEntries];

void NoteObserving(const void* pointee, const void* pointer, const char* fieldName)
{
	ObservationEntry*	entry = gObservationEntries;
	ObservationEntry*	maxEntry = entry + kObservationEntries;
	
	/* first check for a match */
	Assert(entry == gObservationEntries);
	for (; entry < maxEntry; entry++)
		if (entry->fPointee == pointee && fieldName == entry->fFieldName)
		{	entry->fPointer = pointer; return; }
	
	/* otherwise, check for a free slot */
	for (entry = gObservationEntries; entry < maxEntry; entry++)
		if (entry->fPointee == nil)
		{
			entry->fPointee = pointee;
			entry->fPointer = pointer;
			entry->fFieldName = fieldName;
			return;
		}
		
	Complain(("No more debugging observation slots available"));
}

#ifdef MEMORY_TRACKING
int EachObservation(ObservingFunction* function, void* parameters)
{
	ObservationEntry*	entry = gObservationEntries;
	ObservationEntry*	maxEntry = entry + kObservationEntries;
	
	for (; entry < maxEntry; entry++)
		if (entry->fPointee != nil)
			if ((*function)(entry->fPointee, entry->fPointer, entry->fFieldName, parameters))
				return true;
				
	return false;
}
#endif

#endif /* DEBUG */