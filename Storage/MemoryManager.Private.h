// ===========================================================================
//	MemoryManager.Private.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MEMORYMANAGER_PRIVATE_H__
#define __MEMORYMANAGER_PRIVATE_H__

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif

// ===========================================================================

const kBlockRoundingSize = 4;
const kBlockMinAlignment = 4;
const kLargestUnderflowSize = 8;

typedef struct FreeBlock	FreeBlock;
struct FreeBlock
{
	long		length;
	FreeBlock	*next;
};

typedef struct UsedBlock	UsedBlock;
struct UsedBlock
{
	signed		length :28;
	unsigned	underflow: 4;
	char		data[4];
};

#ifdef SIMULATOR
const kTagSlots	= 16384;
#else
const kTagSlots	= 4096;
#endif

extern MemoryTag	gTagSlots[kTagSlots];
MemoryTag* 			FindTag(void* base);
void	 			ClearMemoryTags(void);

#ifdef MEMORY_TRACKING						
static void FillFreeBlock(FreeBlock *freeBlock);
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MemoryManager.Private.h multiple times"
	#endif
#endif /* __MEMORYMANAGER_PRIVATE_H__ */
