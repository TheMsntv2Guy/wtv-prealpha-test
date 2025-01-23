/* ===========================================================================
	MemoryManager.c

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#include "Headers.h"

#include "BoxAbsoluteGlobals.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYMANAGER_PRIVATE_H__
#include "MemoryManager.Private.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#ifdef SIMULATOR
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
	#ifdef FOR_MAC
		#ifndef __MACINTOSHUTILITIES_H__
		#include "MacintoshUtilities.h"
		#endif
		#ifdef MEMORY_TRACKING
			#ifndef __MEMORYGLANCE_H__
			#include "MemoryGlance.h"
			#endif
			#ifndef __MEMORYSEISMOGRAPH_H__
			#include "MemorySeismograph.h"
			#endif
		#endif
	#endif
#endif

// needed for refresh hack in DumpHeapBlocks
#ifdef HARDWARE
#include "BoxUtils.h"
#include "HWRegs.h"
#endif



#ifdef	DEBUG
static Boolean gInMemoryManager = false;
#define	ENTERMEMORYMANAGER()	if ( !IsError(gInMemoryManager) )  gInMemoryManager = true;
#define	EXITMEMORYMANAGER()	gInMemoryManager = false;
#else
#define	ENTERMEMORYMANAGER()
#define	EXITMEMORYMANAGER()	
#endif

/* ===========================================================================
	globals/local constants
=========================================================================== */

#ifdef FOR_MAC
Boolean gUseMacMemory;
#endif

#define HeapEnd()		(READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize))
#define kMaxReasonableLength		5*1024*1024

#if defined(DEBUG) && defined(MEMORY_TRACKING)

	MemoryTag	gTagSlots[kTagSlots];
	
	typedef struct
	{
		void*		fBase;
		ulong		fSize;
		const char*	fTagName;
		const char*	fSourceFile;
		ulong		fLineNumber;
	} CheckOneForObservedParameters;
	
	static Boolean
	CheckOneForObserved(const void* pointee, const void* pointer, const char* fieldName,
						CheckOneForObservedParameters* parameters);
	static Boolean
	CheckForObserved(void* base, ulong size, const char* tagName,
					 const char* sourceFile, ulong lineNumber);

#endif /* of #if defined(DEBUG) && defined(MEMORY_TRACKING) */

/* -------------------------------------------------------------------------- */
/* c++ interfaces to memory management */

#ifndef NO_C_PLUS_PLUS		

#undef new
#undef delete

extern "C"
{
	void __pure_virtual_called(void);
	void __pure_virtual_called()
	{
		Complain(("Pure virtual function called"));
	}
}

const char* kCPlusObjectTagString = "C++";	// so we know this alloc is for a C++ object

void* operator new(uint length)
{	
#ifdef FOR_MAC
	if (gUseMacMemory)
		return calloc(1,length);
	else
#endif
		return AllocateTaggedZero((ulong)length, kCPlusObjectTagString);
}

void operator delete(void* base)
{
#ifdef FOR_MAC
	if (base >= READ_AG(agSysHeapBase) && base < READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize))
#endif
		FreeTaggedMemory(base, nil);
#ifdef FOR_MAC
	else
		free(base);
#endif
}

#endif

#ifdef FOR_MAC
void* PostMacNew(void* object)
{
        gUseMacMemory = false;
        return object;
}
#endif

/* -------------------------------------------------------------------------- */
/* standard memory management */

#undef AllocateMemory
#undef AllocateZero
#undef FreeMemory
#undef AllocateTaggedMemory
#undef AllocateTaggedZero
#undef FreeTaggedMemory
#undef ReallocateMemory
#undef ReallocateTaggedMemory

const ulong kCheckMemoryPattern = 0xDEADDEAD;


/* state description; could be put in an object if we need multiple areas */
static FreeBlock	*gFirstFree;

#ifdef DEBUG
Boolean gMemoryManagementInitialized = false;
#endif
#ifdef DEBUG
	void InitializeMemoryManagement(void);	// make MWC happy
#endif

void InitializeMemoryManagement(void)
{
	ImportantMessage(("Initializing Memory Mgr..."));
	
	Postulate(sizeof(FreeBlock) == sizeof(UsedBlock));
	Postulate(offsetof(UsedBlock, data) == 4);
	Postulate(IsPowerOfTwo(kBlockRoundingSize));
	Postulate(IsPowerOfTwo(sizeof(FreeBlock)));
	Postulate(IsMultipleOf(sizeof(FreeBlock), kBlockRoundingSize));
	Assert((long)READ_AG(agSysHeapSize) > 0);
	Assert(!gMemoryManagementInitialized);
	
	// allocate the first free block (like the first continent)
	gFirstFree = (FreeBlock *)READ_AG(agSysHeapBase);
	gFirstFree->length = READ_AG(agSysHeapSize);
	gFirstFree->next = nil;
	
	Message(("gFirstFree: 0x%x", gFirstFree));
	Message(("gFirstFree->length: 0x%x", gFirstFree->length));
	Message(("gFirstFree->next: 0x%x", gFirstFree->next));
	
#ifdef DEBUG
	gMemoryManagementInitialized = true;
#endif	

#if defined(DEBUG) && defined(MEMORY_TRACKING)		// do we want tagged memory mgmt?
	FillFreeBlock(gFirstFree);
#endif
}

#ifdef INCLUDE_FINALIZE_CODE
void FinalizeMemoryManagement(void)
{
#ifdef DEBUG
		gMemoryManagementInitialized = false;
#endif /* DEBUG */
}
#endif /* INCLUDE_FINALIZE_CODE */

static void RemoveFromFreeList(FreeBlock* freeBlock)
{
	FreeBlock		**curFreeBlock;
	
	for (curFreeBlock = &gFirstFree; *curFreeBlock != nil; curFreeBlock = &(*curFreeBlock)->next)
		if (*curFreeBlock == freeBlock)
			{
			*curFreeBlock = freeBlock->next;
			return;
			}
			
	Trespass();	// should have found it
}

static void CoalesceNewFree(FreeBlock* freeBlock)
{
	FreeBlock		*otherFreeBlock = (FreeBlock*)((char*)freeBlock + freeBlock->length);
	
	if (otherFreeBlock < (FreeBlock*)HeapEnd() && otherFreeBlock->length > 0)
	{
		RemoveFromFreeList(otherFreeBlock);
		freeBlock->length += otherFreeBlock->length;
	}
	
	// now look for the block before us; there can be at most one
	for (otherFreeBlock = gFirstFree; otherFreeBlock != nil; otherFreeBlock = otherFreeBlock->next)
		if ((char*)otherFreeBlock + otherFreeBlock->length == (char*)freeBlock)
		{
			// found him, so tag me on the end
			RemoveFromFreeList(freeBlock);
			otherFreeBlock->length += freeBlock->length;
		}
}

static void SortNewFree(void)
{
	FreeBlock*		freeBlock = gFirstFree;
	FreeBlock**		freeBlockPointer;
	
	Assert(freeBlock != nil);
	
	// pop it off list
	gFirstFree = freeBlock->next;
	
	// find the one right before this one
	for (freeBlockPointer = &gFirstFree; *freeBlockPointer != nil; freeBlockPointer = &(*freeBlockPointer)->next)
		if (*freeBlockPointer > freeBlock)
			break;
			
	freeBlock->next = *freeBlockPointer;
	*freeBlockPointer = freeBlock;
}

#define IsValidFreeBlock(freeBlock) \
	((freeBlock) == nil || \
		(((ulong)freeBlock >= (ulong)READ_AG(agSysHeapBase) \
		&& (ulong)(freeBlock) < (ulong)READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize))))
		
#ifdef DEBUG

static void CheckCoalescedAndSorted(void)
{
	FreeBlock*		freeBlock;
	FreeBlock*		previousFreeBlock = (FreeBlock*)0xFFFFFFFF;
	ulong			freeCount = 0;
	const ulong		kMaxFreeCount = 16*1024*1024;	// just to catch cycles
	const ulong		kMaxFreeLength = 16*1024*1024;	// just to catch bad blocks

	for (freeBlock = gFirstFree; freeBlock != nil; freeBlock = freeBlock->next, previousFreeBlock = freeBlock)
	{
		FreeBlock*		nextFreeBlock;
		long			length = freeBlock->length;
		
		if (length < 0 || length >= kMaxFreeLength)
		{
			Complain(("Block at 0x%x has bad length (%d).", (int)freeBlock, (int)length));
			break;
		}

		if (!IsValidFreeBlock(freeBlock->next))
		{
			Complain(("Bad next field (%x) for free block at %x", (int)freeBlock->next, (int)freeBlock));
			break;
		}
		
		nextFreeBlock = (FreeBlock*)((char*)freeBlock + freeBlock->length);
		if (nextFreeBlock < (FreeBlock*)HeapEnd() && nextFreeBlock->length > 0)
		{
			Complain(("Blocks at 0x%x and 0x%x were not coalesced correctly.",
				(int)freeBlock, (int)nextFreeBlock));
			break;
		}
		if (freeBlock > previousFreeBlock)
		{
			Complain(("Blocks at 0x%x and 0x%x were not sorted correctly.",
				(int)previousFreeBlock, (int)freeBlock));
			break;
		}
		
		if (++freeCount >= kMaxFreeCount)
		{
			Complain(("Free list has a cycle!"));
			break;
		}
	}
}
#else
#define CheckCoalescedAndSorted()	_void
#endif

void* AllocateMemory(ulong length, Boolean nilAllowed)
{
	FreeBlock*		freeBlock;
	FreeBlock*		previousFreeBlock;
	UsedBlock*		usedBlock;
	ulong			requestedLength = length;
	ulong			freeLength;
	
	Assert(gMemoryManagementInitialized);
	if IsError((long)length < 0 || length > kMaxReasonableLength)
		return nil;	// note: should crash here
	
	ENTERMEMORYMANAGER();
	CheckCoalescedAndSorted();

	// round up to nearest block size, and include header
	// note: we leave room for the free header, even if used header
	length += (kBlockRoundingSize - 1) + (sizeof(UsedBlock) - offsetof(UsedBlock, data));
	length &= ~(kBlockRoundingSize - 1);
	
	// it must be big enough to return to free list
	if (length < sizeof(FreeBlock))
		length = sizeof(FreeBlock);
	
	// first-fit algorithm
	previousFreeBlock = nil;
	for (freeBlock = gFirstFree; freeBlock != nil; previousFreeBlock = freeBlock, freeBlock = freeBlock->next)
	{
		if (freeBlock->length < length)
			continue;
			
		// this is our guy; take whole block if necessary or carve out an area for him
		if ((freeLength = freeBlock->length) - length < sizeof(FreeBlock))
		{
			if (previousFreeBlock == nil)
				gFirstFree = freeBlock->next;
			else
				previousFreeBlock->next = freeBlock->next;
			usedBlock = (UsedBlock *)freeBlock;
			length = freeLength;

			CoalesceNewFree(freeBlock);
		}
		else
		{
			FreeBlock*		nextFreeBlock = freeBlock->next;
			
			// allocate the used block from the beginning of this free block
			usedBlock = (UsedBlock *)freeBlock;
			freeBlock = (FreeBlock *)((char*)freeBlock + length);
			freeBlock->length = freeLength - length;
			freeBlock->next = nextFreeBlock;
			Assert(freeBlock->length >= sizeof(FreeBlock));

			// make the previous one in the free list point to the new free block
			if (previousFreeBlock == nil)
				gFirstFree = freeBlock;
			else
				previousFreeBlock->next = freeBlock;
		}
		
		usedBlock->length = -length;			// mark size as negative, for debugging
		Assert(length - (sizeof(UsedBlock) - offsetof(UsedBlock, data)) - requestedLength >= 0);
		Assert(length - (sizeof(UsedBlock) - offsetof(UsedBlock, data)) - requestedLength < sizeof(FreeBlock));
		usedBlock->underflow = length - (sizeof(UsedBlock) - offsetof(UsedBlock, data)) - requestedLength;
		Assert(usedBlock->underflow <= kLargestUnderflowSize);
		CheckCoalescedAndSorted();
		EXITMEMORYMANAGER();
		return (void *)(&usedBlock->data);
	}
	EXITMEMORYMANAGER();
	
	if (!nilAllowed)
		Complain(("Ran out of memory allocating block of size %d", requestedLength));
	return nil;	
}

ulong MemorySize(void* base)
{
	ulong s;
	ENTERMEMORYMANAGER();
	UsedBlock*			usedBlock = (UsedBlock*)((char*)base - (sizeof(UsedBlock) - offsetof(UsedBlock, data)));
	
	Assert(-usedBlock->length >= sizeof(UsedBlock));
	s = ((ulong)-usedBlock->length) - (sizeof(UsedBlock) - offsetof(UsedBlock, data)) - usedBlock->underflow;
	EXITMEMORYMANAGER();
	return s;
}

#ifdef DEBUG
Boolean gFillFreeMem = true;
#endif

#ifdef DEBUG
static void FillMemory(void* base, ulong length, ulong pattern)
{
	#ifdef SIMULATOR
		if (!gSimulator->GetStrictDebugging())
			return;
	#endif
		
		Assert(IsMultipleOf((ulong)base, kBlockMinAlignment));
		
		ulong		*longPtr = (ulong*)base;
	
		length /= sizeof(ulong);
		while (length-- != 0)
			*longPtr++ = pattern;
}
#else
#define FillMemory(base, length, pattern)		0
#endif

#ifdef DEBUG
static void
FillFreeBlock(FreeBlock *freeBlock)
{
	Assert(freeBlock->length >= sizeof(*freeBlock));
	FillMemory(freeBlock + 1, freeBlock->length - sizeof(*freeBlock), kCheckMemoryPattern);
}
#endif

void FreeMemory(void* base)
{
	FreeBlock*		freeBlock;
#ifdef DEBUG
	const ulong		kMaxFreeLength = 16*1024*1024;	// just to catch bad blocks
#endif

	if (IsError(base == nil))
		return;
	ENTERMEMORYMANAGER();
		
	CheckCoalescedAndSorted();

	freeBlock = (FreeBlock*)((char*)base - (sizeof(UsedBlock) - offsetof(UsedBlock, data)));
	
	Assert(gMemoryManagementInitialized);

#ifdef HARDWARE	
	Assert((ulong)freeBlock < (ulong)(READ_AG(agSysStackBase) - 0x8000));
#endif

	Assert(freeBlock->length < 0);
	freeBlock->length = -((UsedBlock*)freeBlock)->length;
	Assert(freeBlock->length >= sizeof(FreeBlock) && freeBlock->length < kMaxFreeLength);
	freeBlock->next = gFirstFree;
	gFirstFree = freeBlock;
	
#ifdef DEBUG
	if (gFillFreeMem) {
		FillFreeBlock(freeBlock);
	}
#endif
	
	CoalesceNewFree(freeBlock);
	SortNewFree();
	CheckCoalescedAndSorted();
	EXITMEMORYMANAGER();
}

void* ReallocateMemory(void *address, ulong newSize)
{
	ulong oldLength = MemorySize(address);	
	CheckCoalescedAndSorted();
	
	if (newSize < oldLength)
	{
		UsedBlock*			usedBlock = (UsedBlock*)((char*)address - (sizeof(UsedBlock) - offsetof(UsedBlock, data)));
		ulong				newLength = -usedBlock->length;
		long				newUnderflow = usedBlock->underflow + newLength - newSize;
		
		Assert(usedBlock->length < 0);
		
		/* suck up more underflow if possible, or else lower block size */
		if (newUnderflow < sizeof(UsedBlock))
		{
			usedBlock->underflow = newUnderflow;
			Assert(usedBlock->underflow <= kLargestUnderflowSize);
		}
		else
		{
			// round up to nearest block size, and include header
			// note: we leave room for the free header, even if used header
			ulong	newBlockSize = newSize + (kBlockRoundingSize - 1) + (sizeof(UsedBlock) - offsetof(UsedBlock, data));
			newBlockSize &= ~(kBlockRoundingSize - 1);
			if (newBlockSize < sizeof(FreeBlock))
				newBlockSize = sizeof(FreeBlock);
		
			// shrink it, from end
			if (newLength - newBlockSize > sizeof(FreeBlock))
			{
				FreeBlock*	freeBlock = (FreeBlock*)((char*)usedBlock + newBlockSize);
				
				freeBlock->length = newLength - newBlockSize;
				Assert(freeBlock->length >= sizeof(FreeBlock));
				freeBlock->next = gFirstFree;
				gFirstFree = freeBlock;
				
				usedBlock->length += freeBlock->length;
				usedBlock->underflow = newBlockSize - newSize;
				Assert(usedBlock->underflow <= kLargestUnderflowSize);
				
				CoalesceNewFree(freeBlock);
				SortNewFree();
			}
		}
	}
	else
	{
		// uh oh, need to grow
		void*			newBase = AllocateMemory(newSize);
		
		if (newBase != nil)
			CopyMemory(address, newBase, oldLength);
		FreeMemory(address);
		address = newBase;
	}
	
	CheckCoalescedAndSorted();
	return address;
}

/* -------------------------------------------------------------------------- */
/* memory utilities */

void* 
AllocateZero(ulong length, Boolean nilAllowed)
{
	char* result = (char*) AllocateMemory(length, nilAllowed);
	
	if (result != nil)
		ZeroMemory(result, length);

	return (void*)result;
}

void 
CopyMemory(const void* src, void* dest, long length)
{
	if (src == dest || length == 0)
		return;
	
	if (IsError(length < 0))
		return;
	
	if (IsError(src == nil || dest == nil))
		return;

	memmove(dest, src, length);
}

static void
ReverseMemory(long* data, long count)
{
	for (; count >= 2; count -= 2, data++) {
		register long temp = *data;
		*data = data[count-1];
		data[count-1] = temp;
	}
}

void 
SwapMemory(void* data, long part1, long part2)
{
	if (IsError(data == nil))
		return;

	if (IsError(part1 <= 0 || (part1 & 0x3) != 0))
		return;

	if (IsError(part2 <= 0 || (part2 & 0x3) != 0))
		return;
	
	ReverseMemory((long*)data, part1 / 4);
	ReverseMemory((long*)data + (part1 / 4), part2 / 4);
	ReverseMemory((long*)data, (part1 + part2) / 4);
}

void 
ZeroMemory(void* p, ulong length)
{
	memset(p, 0, length);
}
	
/* -------------------------------------------------------------------------- */
/* memory tracking */

#ifdef MEMORY_TRACKING
static Boolean
CheckOneForObserved(const void* /*pointee*/, const void* pointer, const char* fieldName, CheckOneForObservedParameters* parameters)
{
	if (pointer < parameters->fBase || pointer >= (char*)parameters->fBase + parameters->fSize)
		return false;
		
	Complain(("Block at %p (%s, (File %s; Line %d)) is changing, but field %s points to %p",
		parameters->fBase, parameters->fTagName,
		parameters->fSourceFile, parameters->fLineNumber,
		fieldName, pointer));
	return true;
}

static Boolean
CheckForObserved(void* base, ulong size, const char* tagName, const char* sourceFile, ulong lineNumber)
{
	CheckOneForObservedParameters	parameters;
	
	parameters.fBase = base;
	parameters.fSize = size;
	parameters.fTagName = tagName;
	parameters.fSourceFile = sourceFile;
	parameters.fLineNumber = lineNumber;
	return EachObservation((ObservingFunction*)CheckOneForObserved, &parameters);
}

#ifdef DEBUG
	static void* FilledMemory(void* base, ulong length, ulong pattern)
	{
	#ifdef SIMULATOR
		if (!gSimulator->GetStrictDebugging())
			return nil;
	#endif
			
		Assert(IsMultipleOf((ulong)base, kBlockMinAlignment));
		
		ulong		result = 0;
		ulong		*longPtr = (ulong*)base;
	
		length /= sizeof(ulong);
		while (length-- != 0)
			if (*longPtr++ != pattern)
				return (void*)(longPtr - 1);
			
		return nil;
	}
#else
	#define FilledMemory(base, length, pattern)		(nil)
#endif

static void FillFreeBlocks(void)
{
	for (FreeBlock* freeBlock = gFirstFree; freeBlock != nil; freeBlock = freeBlock->next)
		FillFreeBlock(freeBlock);
}

static void* FilledFreeBlock(FreeBlock *freeBlock)
{
	Assert(freeBlock->length >= sizeof(*freeBlock));
	return FilledMemory(freeBlock + 1, freeBlock->length - sizeof(*freeBlock), kCheckMemoryPattern);
}

static void VerifyFreeList(void)
{
#ifdef SIMULATOR
	if (!gSimulator->GetStrictDebugging())
		return;
#endif
	
	FreeBlock *freeBlock;

	for (freeBlock = gFirstFree; freeBlock != nil; freeBlock = freeBlock->next)
	{
		if (freeBlock->length < 0)
			Complain(("Bad block size (%d) for block at %p", freeBlock->length, freeBlock));
		if (freeBlock->next != nil && !IsValidPtr(freeBlock->next))
			Complain(("Bad next field (%p) for block at %p", freeBlock->next, freeBlock));
#if 0
		{
		void		*writtenAddress;
		if ((writtenAddress = FilledFreeBlock(freeBlock)) != nil)
			Complain(("a:Someone wrote to free memory, starting at 0x%08x", writtenAddress));
		}
#endif
	}
}

static void VerifyBlocks(void)
{
#ifdef SIMULATOR
	if (!gSimulator->GetStrictDebugging())
		return;
#endif
	 
	UsedBlock	*usedBlock = (UsedBlock*)READ_AG(agSysHeapBase);
	UsedBlock	*lastUsedBlock = nil;
	
	while (usedBlock < (UsedBlock*)HeapEnd())
	{
	long		curLength = usedBlock->length;
	Boolean		curUsed = curLength < 0;
	
	if (curUsed)
		curLength = -curLength;
	else
		curLength = ((FreeBlock*)usedBlock)->length;
		
	if (curLength < sizeof(FreeBlock))
		Complain(("Bad block size (%d) for block at %p (last good @ %p)",
			curLength, usedBlock, lastUsedBlock));
			
	lastUsedBlock = usedBlock;	// maintained for debugging
	usedBlock = (UsedBlock*)((char*)usedBlock + curLength);
	}
}


#ifdef	DEBUG

void ClearMemoryTags(void)
{
	ulong		i;
	MemoryTag	*tag = gTagSlots;
	
	for (i = 0; i < kTagSlots; i++, tag++)
		tag->fInUse = false;
}

void VerifyMemory()
{
	VerifyFreeList();
	VerifyBlocks();
}

#endif

static MemoryTag* NextFreeTag(void)
{
	ulong		i;
	MemoryTag	*tag = gTagSlots;
	
	for (i = 0; i < kTagSlots; i++, tag++)
		if (!tag->fInUse)
			return tag;
			
	return nil;
}

MemoryTag* FindTag(void* base)
{
	ulong		i;
	MemoryTag	*tag = gTagSlots;
	
	if(tag)
	{
		for (i = 0; i < kTagSlots; i++, tag++)
			if ((tag->fBase == base) && tag->fInUse)
				return tag;
	}		
	
	return nil;
}

static ulong gMemoryTime = 0;

static void* AllocateTaggedCommon(Boolean shouldZero, ulong length, const char* tagName, const char* ownerURL,
								  const char* sourceFile, ulong lineNumber, Boolean nilAllowed)
{
	void		*base;
	MemoryTag	*tag;
	
	VerifyFreeList();
	VerifyBlocks();

	base = (shouldZero ? AllocateZero : AllocateMemory)(length, nilAllowed);
	
	if (base == nil)
	{
//		Complain(("AllocateTaggedCommon() failed to allocate %d to %s in file %s, line %d\n",		length, tagName, sourceFile, lineNumber));
		return base;
	}

	tag = NextFreeTag();

	Assert(base != nil && IsMultipleOf((ulong)base, kBlockMinAlignment));
	length += (kBlockRoundingSize - 1) + (sizeof(UsedBlock) - offsetof(UsedBlock, data));
	length &= ~(kBlockRoundingSize - 1);
	if (length < sizeof(FreeBlock))
		length = sizeof(FreeBlock);

	if (!shouldZero)
		{
		Postulate(sizeof(FreeBlock) == sizeof(UsedBlock));
#if 0
		void* writtenAddress;
		if ((writtenAddress = FilledMemory(base, length - (sizeof(UsedBlock) - offsetof(UsedBlock, data)), kCheckMemoryPattern)) != nil)
			Complain(("b:Someone wrote to free memory, starting at 0x%08x", writtenAddress));
#endif		
		}
		
	if (tag == nil)
	{
		Complain(("Memory tagging cannot handle > %d blocks simultaneously", kTagSlots));
		return base;
	}

	tag->fInUse = true;
	tag->fBase = base;
	tag->fLength = length;
	tag->fName = tagName;
	tag->fOwnerURL = UniqueName(ownerURL);
	tag->fSourceFile = sourceFile;
	tag->fClassNumber = kNoClassNumber;
	tag->fLineNumber = lineNumber;
	tag->fCreateTime = Now();
	tag->fModifyTime = Now();
	tag->fIsCPlus = false;	// unless otherwise proven
	
	VerifyFreeList();
	VerifyBlocks();
	gMemoryTime++;


#if defined(MEMORYGLANCE_ACTIVE)
	if (gURLGlanceWindow != nil)
		gURLGlanceWindow->TaggedAlloc(tag);
	if (gBlockGlanceWindow != nil)
		gBlockGlanceWindow->TaggedAlloc(tag);
	if (gTagItemWindow != nil)
		gTagItemWindow->TaggedAlloc(tag);
#endif
#if defined(MEMORYSEISMOGRAPH_ACTIVE)
	if (gMemorySeismographWindow != nil)
		gMemorySeismographWindow->TaggedAlloc(tag);
#endif
	
	return base;
}

void* DoAllocateTaggedMemory(ulong length, const char* tagName, const char* ownerURL, const char* sourceFile, ulong lineNumber, Boolean nilAllowed)
{
	return AllocateTaggedCommon(false, length, tagName, ownerURL, sourceFile, lineNumber, nilAllowed);
}
	
void* DoAllocateTaggedZero(ulong length, const char* tagName, const char* ownerURL, const char* sourceFile, ulong lineNumber, Boolean nilAllowed)
{
	return AllocateTaggedCommon(true, length, tagName, ownerURL, sourceFile, lineNumber, nilAllowed);
}

void DoFreeTaggedMemory(void* base, const char* tagName, const char* sourceFile, ulong lineNumber)
{
	MemoryTag	*tag = FindTag(base);

	if (tag == nil)
	{
		const char*	displayName = (tagName == nil) ? "" : tagName;
		Complain(("Cannot find tag for \"%s\" block at 0x%x", displayName, (ulong)base));
		return;
	}

#if defined(MEMORYGLANCE_ACTIVE)
	if (gURLGlanceWindow)
		gURLGlanceWindow->TaggedFree(tag);
	if (gBlockGlanceWindow)
		gBlockGlanceWindow->TaggedFree(tag);
	if (gTagItemWindow)
		gTagItemWindow->TaggedFree(tag);
#endif
#if defined(MEMORYSEISMOGRAPH_ACTIVE)
	if (gMemorySeismographWindow)
		gMemorySeismographWindow->TaggedFree(tag);
#endif
	
#if 0 /* <MAK> - what is this?? */
	// note: if fModifyTime is zero then we are checking the object name elsewhere
	//if (tag != nil && tag->fName != nil && strcmp(tag->fName, "URLParser::fURLParts") == 0)
	//	if ((char*)&gURLParser <= (char*)base && (char*)base < (char*)(&gURLParser + 1))
	//		Complain(("Deleting global URL parser field"));
#endif /* #if 0 */

	Assert(IsValidPtr(base));
	tag->fInUse = false;
	if (tag->fModifyTime != 0)				// if zeroed then already set
	{
		tag->fSourceFile = sourceFile;
		tag->fLineNumber = lineNumber;
	}
	tag->fModifyTime = Now();				// mark when destroyed too!
	
	if (tag->fModifyTime != 0 && tagName != nil)
		if (strcmp(tagName, tag->fName) != 0)
			Complain(("Bad tag name"));
			
	VerifyFreeList();
	VerifyBlocks();

	CheckForObserved(base, -((UsedBlock*)((char *)base - (sizeof(UsedBlock) - offsetof(UsedBlock, data))))->length - (sizeof(UsedBlock) - offsetof(UsedBlock, data)),
		tagName, sourceFile, lineNumber);
	
	FreeMemory(base);
	FillFreeBlocks();

	VerifyFreeList();
	VerifyBlocks();
	gMemoryTime++;
}

void* DoReallocateTaggedMemory(void* base, ulong size, const char* tagName, const char* sourceFile, ulong lineNumber)
{
	MemoryTag*	tag = FindTag(base);
	void*		result;
	ulong		createTime;

	if (tag == nil)
	{
		const char*		displayName = (tagName == nil) ? "" : tagName;
		Complain(("Cannot find tag for \"%s\" block at 0x%x", displayName, (ulong)base));
		return nil;
	}

	Assert(tag->fBase == base);
	
	createTime = tag->fCreateTime;
	
	if (tagName != nil)
		if (strcmp(tagName, tag->fName) != 0)
			Complain(("Bad tag name"));

	CheckForObserved(base, MemorySize(base), tagName, sourceFile, lineNumber);
	
#if defined(MEMORYGLANCE_ACTIVE)
	if (gURLGlanceWindow != nil)
		gURLGlanceWindow->TaggedFree(tag);
	if (gBlockGlanceWindow != nil)
		gBlockGlanceWindow->TaggedFree(tag);
	if (gTagItemWindow != nil)
		gTagItemWindow->TaggedFree(tag);
#endif
#if defined(MEMORYSEISMOGRAPH_ACTIVE)
	if (gMemorySeismographWindow != nil)
		gMemorySeismographWindow->TaggedFree(tag);
#endif
	if ((result = ReallocateMemory(base, size)) == nil)
	{
		tag->fInUse = false;
		Complain(("Cannot reallocate %d bytes", size));
	}
	else
	{
		tag->fBase = result;
		CheckForObserved(result, size, tagName, sourceFile, lineNumber);
	}

	tag->fSourceFile = sourceFile;
	tag->fLineNumber = lineNumber;
	tag->fCreateTime = createTime;	// keep it the same
	tag->fModifyTime = Now();
	tag->fLength = size;

#if defined(MEMORYGLANCE_ACTIVE)
	if (gURLGlanceWindow != nil)
		gURLGlanceWindow->TaggedAlloc(tag);
	if (gBlockGlanceWindow != nil)
		gBlockGlanceWindow->TaggedAlloc(tag);
	if (gTagItemWindow != nil)
		gTagItemWindow->TaggedAlloc(tag);
#endif
#if defined(MEMORYSEISMOGRAPH_ACTIVE)
	if (gMemorySeismographWindow != nil)
		gMemorySeismographWindow->TaggedAlloc(tag);
#endif

	VerifyFreeList();
	VerifyBlocks();
	gMemoryTime++;
	
	return result;
}

void* TaggedNew(void *newObject, const char* tagName, ulong classNumber, const char* /*ownerURL*/, const char* sourceFile, ulong lineNumber)
{
	MemoryTag*	tag = FindTag(newObject);
	
	if (newObject == nil)
		return nil;
		
	if ((tag = FindTag(newObject)) == nil)
	{
		const char*		displayName = (tagName == nil) ? "" : tagName;
		Complain(("Cannot find tag for \"%s\" block at 0x%x", displayName, (ulong)newObject));
		return nil;
	}

	// readjust certain fields
	tag->fIsCPlus = true;
	tag->fClassNumber = (ClassNumber)classNumber;
	tag->fSourceFile = sourceFile;
	tag->fLineNumber = lineNumber;
	tag->fName = tagName;

#if defined(MEMORYGLANCE_ACTIVE)
	if (gURLGlanceWindow != nil)
		gURLGlanceWindow->TaggedNew(tag);
	if (gBlockGlanceWindow != nil)
		gBlockGlanceWindow->TaggedNew(tag);
	if (gTagItemWindow != nil)
		gTagItemWindow->TaggedNew(tag);
#endif
#if defined(MEMORYSEISMOGRAPH_ACTIVE)
	if (gMemorySeismographWindow != nil)
		gMemorySeismographWindow->TaggedNew(tag);
#endif

	return newObject;
}

void TaggedDelete(void* object, const char* sourceFile, ulong lineNumber)
{
	// Allow nil object.
	if (object == nil)
		return;

#ifdef FOR_MAC				// Must be a Mac object
	if (object < READ_AG(agSysHeapBase) || object > READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize))
		return;
#endif
		
	MemoryTag* tag = FindTag(object);

	if (tag == nil)
	{
		Complain(("Cannot find tag for object at 0x%x", (ulong)object));
		return;
	}

#if defined(MEMORYGLANCE_ACTIVE)
	if (gURLGlanceWindow != nil)
		gURLGlanceWindow->TaggedDelete(tag);
	if (gBlockGlanceWindow != nil)
		gBlockGlanceWindow->TaggedDelete(tag);
	if (gTagItemWindow != nil)
		gTagItemWindow->TaggedDelete(tag);
#endif
#if defined(MEMORYSEISMOGRAPH_ACTIVE)
	if (gMemorySeismographWindow != nil)
		gMemorySeismographWindow->TaggedDelete(tag);
#endif

	
	tag->fModifyTime  = 0;		// signify already set
	tag->fSourceFile = sourceFile;
	tag->fLineNumber = lineNumber;
}

static MemoryTag	gStandinTag =
{
	false, false, {0,0}, nil, 0, "<Unknown url>", kNoClassNumber, "<Unknown source file>", 1, 0, 0, "<Unknown>"
};

void* NextMemoryBlock(void* block, Boolean* used, ulong* length, 
	char* name, char* ownerURL, Boolean *isCPlus, char* sourceFile, ulong* lineNumber)
{
	Boolean		curUsed;
	long		curLength;
	MemoryTag	*tag;

	// can pass in nil for first call to get first block
	if (block == nil)
		block = (UsedBlock*)READ_AG(agSysHeapBase);
	else
	{
		curLength = ((UsedBlock*)block)->length;
		if (curLength < 0)
			curLength = -curLength;
		else
			curLength = ((FreeBlock*)block)->length;
			
		block = (char*)block + curLength;

		// over the end?
		if ((ulong)block >= (ulong)READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize))
			return nil;
	}
		
	curLength = ((UsedBlock*)block)->length;
	curUsed = (curLength < 0);
	if (curUsed)
		curLength = -curLength;
	else
		curLength = ((FreeBlock*)block)->length;
	tag = FindTag((UsedBlock*)((UsedBlock*)block)->data);
	if (curUsed && tag == nil)
	{
		Complain(("Cannot find tag for block at 0x%x", (int)block));
		tag = &gStandinTag;
	}
		
	if (used != nil)
		*used = curUsed;
	if (length != nil)
		*length = curLength;
	if (name != nil)
	{
		*name = 0;
		if (curUsed)
			strcpy(name, tag->fName);
	}
	
	if (ownerURL != nil)
	{
		*ownerURL = 0;
		if (curUsed)
			strcpy(ownerURL, tag->fOwnerURL);
	}

	if (isCPlus != nil)
	{
		*isCPlus = tag->fIsCPlus;
	}

	if (sourceFile != nil)
	{
		*sourceFile = 0;
		if (curUsed)
			strcpy(sourceFile, tag->fSourceFile);
	}
	if (lineNumber != nil)
	{
		*lineNumber = 0;
		if (curUsed)
			*lineNumber = tag->fLineNumber;
	}
		
	return block;
}

ulong MemoryTime(void)
{
	return gMemoryTime;
}

#endif /* MEMORY_TRACKING */

void DumpFreeBlocks(void)
{
	FreeBlock*	freeBlock = gFirstFree;
	
	if (!IsValidFreeBlock(freeBlock))
	{
		Message(("Bad gFirstFree (%x)", (int)freeBlock));
		return;
	}
	
	while (freeBlock != nil)
	{
		if (!IsValidFreeBlock(freeBlock))
		{	Message(("Bad free block at 0x%x", (int)freeBlock)); return; }
		if (!IsValidFreeBlock(freeBlock->next))
		{	Message(("Bad next field (0x%x) for free block at 0x%x", (int)freeBlock->next, (int)freeBlock)); return; }
		
		freeBlock = freeBlock->next;
	}
}

void MemContents(char* addr,char *hex,char *ascii);

void MemContents(char* addr,char *hex,char *ascii)
{
	int i;
	char c;
	
	sprintf(hex,(char*)"%08x %08x %08x %08x  ",
		*(ulong*)(addr), *(ulong*)(addr+4), *(ulong*)(addr+8), *(ulong*)(addr+12) );
		
	for(i = 0;i<16;i++) {
		c = *(addr+i);
		if(!isprint(c))
			c = '.';
		ascii[i] = c;
	}
	ascii[i] = '\0';
}

void DumpHeapBlocks(Boolean detailed)
{
	void* 		block;
	long		curLength;
	long		totalUsed = 0;
	long		totalFree = 0;
	long		countUsed = 0;
	long		countFree = 0;
	char		hex[80];
	char		ascii[17];
	
	if (detailed)
		DumpFreeBlocks();
		
	block = (long*)READ_AG(agSysHeapBase);

	while ((ulong)block < (ulong)READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize))
	{
		curLength = ((FreeBlock*)block)->length;
		if (curLength < 0)
		{	// used block
			curLength = -((UsedBlock*)block)->length;
			totalUsed += curLength;
			if(detailed) {
				MemContents((char*)block,hex,ascii);
				Message(("  %8x Used: %6x (%6d)  %s %s",block,curLength,curLength,hex,ascii));
			}
			countUsed++;
		}
		else
		{	// free block
			totalFree += curLength;
			if(detailed) {
				MemContents((char*)block,hex,ascii);
				Message(("* %8x Free: %6x (%6d)  %s %s",block,curLength,curLength,hex,ascii));
			}
			countFree++;
		}
		if (curLength == 0)
			break;
		block = (void*)((ulong)block + curLength);

#ifdef	HARDWARE		
		if(detailed) {
			static n = 0;
			/* refresh RAM on SPOT3 every other block */
			if((SPOTVersion() == kSPOT3) && ((++n % 2) == 0))
				for(int i=0;i<1024;i++)
					*(volatile ulong*)kMemCmdReg = 0x48000000;
		}
#endif		
	}
	if(detailed)
		Message((""));
		
	Message(("Total Used: %8x (%7d) bytes, %4x (%5d) blocks",  totalUsed,totalUsed,countUsed,countUsed));
	Message(("Total Free: %8x (%7d) bytes, %4x (%5d) blocks\n",totalFree,totalFree,countFree,countFree));
}

#ifdef DEBUG_MISCSTATWINDOW
long GetFreeListMemorySize(void)
{
	FreeBlock* block = gFirstFree;
	long freeMemorySize = 0;
	while (block != nil) {
		freeMemorySize += block->length;
		block = block->next;
	}
	return freeMemorySize;
}

long GetFreeMemorySize(void)
{
	void* block = (long*)READ_AG(agSysHeapBase);
	long totalFree = 0;
	
	while ((ulong)block < (ulong)READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize)) {
		long curLength = ((FreeBlock*)block)->length;
		if (curLength >= 0) {
			// free block
			totalFree += curLength;
		}
		if (curLength == 0) {
			break;
		}
		block = (void*)((ulong)block + curLength);
	}
	return totalFree;
}

long GetUsedMemorySize(void)
{
	void* block = (long*)READ_AG(agSysHeapBase);
	long totalUsed = 0;
	
	while ((ulong)block < (ulong)READ_AG(agSysHeapBase) + READ_AG(agSysHeapSize)) {
		long curLength = ((FreeBlock*)block)->length;
		if (curLength < 0) {
			// used block
			curLength = -((UsedBlock*)block)->length;
			totalUsed += curLength;
		}
		if (curLength == 0) {
			break;
		}
		block = (void*)((ulong)block + curLength);
	}
	return totalUsed;
}
#endif
 

