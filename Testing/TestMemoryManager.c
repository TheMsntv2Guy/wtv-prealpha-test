// ===========================================================================
//	TestMemoryManager.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifdef TEST_SIMULATORONLY

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __TESTING_PRIVATE_H__
#include "Testing.Private.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#ifdef MEMORY_TRACKING

#define kMaxTestNames	16
static char	*names[kMaxTestNames] =
{
	"a", "b", "c", "d",
	"e", "f", "g", "h",
	"i", "j", "k", "l",
	"m", "n", "o", "p"
};

static ulong UsedBlocks(void)
{
	void*			block = nil;
	Boolean			used;
	ulong			count = 0;

	while ((block = NextMemoryBlock(block, &used, nil, nil, nil, nil, nil, nil)) != nil)
		if (used)
			count++;
		
	return count;
}

static ulong FreeBlocks(void)
{
	void*			block = nil;
	Boolean			used;
	ulong			count = 0;

	while ((block = NextMemoryBlock(block, &used, nil, nil, nil, nil, nil, nil)) != nil)
		if (!used)
			count++;
		
	return count;
}

static void FillMemory(void* base, ulong length, ulong pattern)
{
	ulong		result = 0;
	ulong		*longPtr = (ulong*)base;

	Assert(IsMultipleOf((ulong)length, sizeof(ulong)));
	
	length /= sizeof(ulong);
	while (length-- != 0)
		*longPtr++ = pattern;
}

static void
TestSwapMemory()
{
	char buffer[256];
	
	strcpy(buffer, "abcdefghijklmnop");
	SwapMemory(buffer, 4, 12);
	Assert(strcmp(buffer, "efghijklmnopabcd") == 0);
	
	strcpy(buffer, "abcdefghijklmnop");
	SwapMemory(buffer, 8, 8);
	Assert(strcmp(buffer, "ijklmnopabcdefgh") == 0);
	
	strcpy(buffer, "abcdefghijklmnop");
	SwapMemory(buffer, 12, 4);
	Assert(strcmp(buffer, "mnopabcdefghijkl") == 0);
}

void TestMemoryManager(void)
{
	int		i, j;
	void*	addresses[64];
	ulong	usedBlockCount = UsedBlocks();
	ulong	freeBlockCount = FreeBlocks();
	ulong	newUsedBlockCount;
		
	Message(("Testing Memory Manager (1/8): In Order..."));
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		newUsedBlockCount = UsedBlocks();
		Assert(newUsedBlockCount == usedBlockCount + i);
		addresses[i] = AllocateTaggedMemory(i, names[i % kMaxTestNames]);
		Assert(addresses[i] != nil);
	}
	newUsedBlockCount = UsedBlocks();
	Assert(newUsedBlockCount == usedBlockCount + sizeof(addresses)/sizeof(void*));
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
		FreeTaggedMemory(addresses[i], names[i % kMaxTestNames]);
	newUsedBlockCount = UsedBlocks();
	Assert(newUsedBlockCount == usedBlockCount);
	
	Message(("Testing Memory Manager (2/8): Bad Tags..."));
#if 0
	// do it incorrectly
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		addresses[i] = AllocateTaggedMemory(i, names[i % kMaxTestNames]);
		Assert(addresses[i] != nil);
	}
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
		FreeTaggedMemory(addresses[i], names[(i+1) % kMaxTestNames]);
#endif

	// deallocate in different order than allocation
	Message(("Testing Memory Manager (3/8): Reverse deallocation..."));
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		addresses[i] = AllocateTaggedMemory(i, names[i % kMaxTestNames]);
		Assert(addresses[i] != nil);
	}
	newUsedBlockCount = UsedBlocks();
	Assert(newUsedBlockCount == usedBlockCount + sizeof(addresses)/sizeof(void*));
	for (i = sizeof(addresses)/sizeof(void*) - 1; i >= 0 ; i--)
		FreeTaggedMemory(addresses[i], names[i % kMaxTestNames]);
	newUsedBlockCount = UsedBlocks();
	Assert(newUsedBlockCount == usedBlockCount);

	Message(("Testing Memory Manager (4/8): Reallocation..."));
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		addresses[i] = AllocateTaggedMemory(sizeof(addresses)/sizeof(void*)/2, names[i % kMaxTestNames]);
		Assert(addresses[i] != nil);
		for (j = 0; j < sizeof(addresses)/sizeof(void*)/2; j++)
			((char*)addresses[i])[j] = (j & 0xFF);
	}
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		addresses[i] = ReallocateTaggedMemory(addresses[i], i, names[i % kMaxTestNames]);
		Assert(addresses[i] != nil);
	}
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		ulong		blockSize = sizeof(addresses)/sizeof(void*)/2;
		if (blockSize > i)
			blockSize = i;
		for (j = 0; j < blockSize; j++)
			if (((char*)addresses[i])[j] != (j & 0xFF))
				Complain(("Block %d is not filled correctly at offset %d", i, j));
		FreeTaggedMemory(addresses[i], names[i % kMaxTestNames]);
	}
		
	// leave some around
	Message(("Testing Memory Manager (5/8): Extra Allocation..."));
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		addresses[i] = AllocateTaggedMemory(i*64, names[i % kMaxTestNames]);
		Assert(addresses[i] != nil);
	}
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
		if (i % 3)
			FreeTaggedMemory(addresses[i], names[i % kMaxTestNames]);

	// test c++ allocation
	Message(("Testing Memory Manager (6/8): C++ Allocation..."));
	usedBlockCount = UsedBlocks();
#ifdef __cpplusplus
	for (i = 0; i < 64; i++)
	{
		Point	*point = new(Point);
		delete(point);
	}
#endif
	newUsedBlockCount = UsedBlocks();
	Assert(newUsedBlockCount == usedBlockCount);

	Message(("Testing Memory Manager (7/8): Temporary Allocation..."));
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
	{
		addresses[i] = AllocateBuffer(i*8);
#if defined FOR_MAC && defined _ALLOCA && !defined GENERATING68K
		Assert(addresses[i] != nil);
		if (i*8 <= kMaxReasonableStackSize && !(*(char**)0x2aa < (char*)addresses[i] && (char*)addresses[i] >= **(char***)0x2aa))
			Complain(("alloca returned block 0x%08X in Mac heap (heap ends at 0x%08X)", (char*)addresses[i], **(char***)0x2aa));
#endif
	}
	for (i = 0; i < sizeof(addresses)/sizeof(void*); i++)
		FreeBuffer(addresses[i], i*8);
	newUsedBlockCount = UsedBlocks();
	Assert(newUsedBlockCount == usedBlockCount);
	
	Message(("Testing Memory Manager (8/8): SwapMemory..."));
	TestSwapMemory();
		
	Message(("Memory Manager testing completed."));
}

#endif /* MEMORY_TRACKING */

#endif /* TEST_SIMULATORONLY */