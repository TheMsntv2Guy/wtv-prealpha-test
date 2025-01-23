// ===========================================================================
//	MemoryManager.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MEMORYMANAGER_H__
#define __MEMORYMANAGER_H__

// ===========================================================================

#if defined SIMULATOR && !defined GENERATING68K
	#ifndef _ALLOCA
	#include "alloca.h"
	#endif
#endif
#ifndef __CLASSES_H__
#include "Classes.h"
#endif
#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

// ===========================================================================

void InitializeMemoryManagement(void);		/* this will init tags if MEMORY_TRACKING is on */
#ifdef INCLUDE_FINALIZE_CODE
void FinalizeMemoryManagement(void);
#endif /* INCLUDE_FINALIZE_CODE */

void*	AllocateMemory(ulong length, Boolean nilAllowed = false);
void*	AllocateZero(ulong length, Boolean nilAllowed = false);
void	CopyMemory(const void* source, void* destination, long length);
void	FreeMemory(void *address);
ulong	MemorySize(void *address);
void*	ReallocateMemory(void *address, ulong newSize);
void	SwapMemory(void* data, long part1, long part2);
void	ZeroMemory(void* data, ulong length);

#ifdef DEBUG_MISCSTATWINDOW
long	GetFreeMemorySize(void);
long	GetUsedMemorySize(void);
long	GetFreeListMemorySize(void);
#endif /* DEBUG_MISCSTATWINDOW */

// ===========================================================================
// covers for new and delete
#ifdef FOR_MAC
	extern Boolean			gUseMacMemory;
	void*					PostMacNew(void* object);
	#define newMac(class)   (gUseMacMemory = true, (class *)PostMacNew((void*)new class))
#endif

// ===========================================================================
#ifdef MEMORY_TRACKING
	#ifndef DEBUG
	#error "#define-ing MEMORY_TRACKING requires #define-ing DEBUG"
	#endif

	extern const char* kCPlusObjectTagString;

	void*	DoAllocateTaggedMemory(ulong length, const char *tagName, const char *ownerURL, const char *sourceFile, ulong lineNumber, Boolean nilAllowed = false);
	#define AllocateMemory(length) \
				DoAllocateTaggedMemory((length), "untagged", GetDebugURL(), __FILE__, __LINE__)
	#define	AllocateTaggedMemory(length, tag) \
				DoAllocateTaggedMemory((length), (tag), GetDebugURL(), __FILE__, __LINE__)
	#define AllocateTaggedMemoryNilAllowed(length, tag) \
				DoAllocateTaggedMemory((length), (tag), GetDebugURL(), __FILE__, __LINE__, true)
	
	void*	DoAllocateTaggedZero(ulong length, const char *tagName, const char *ownerURL, const char *sourceFile, ulong lineNumber, Boolean nilAllowed = false);
	#define AllocateZero(length) \
				DoAllocateTaggedZero((length), "untagged", nil, __FILE__, __LINE__)
	#define	AllocateTaggedZero(length, tag) \
				DoAllocateTaggedZero((length), (tag), GetDebugURL(), __FILE__, __LINE__)
	#define AllocateTaggedZeroNilAllowed(length, tag) \
				DoAllocateTaggedZero((length), (tag), GetDebugURL(), __FILE__, __LINE__, true)
	
	void	DoFreeTaggedMemory(void *base, const char *tagName, const char *sourceFile, ulong lineNumber);
	#define FreeMemory(base) \
				DoFreeTaggedMemory((base), "untagged", __FILE__, __LINE__)
	#define	FreeTaggedMemory(base, tag) \
				DoFreeTaggedMemory((base), (tag), __FILE__, __LINE__)
	
	void*	DoReallocateTaggedMemory(void *base, ulong size, const char *tagName, const char *sourceFile, ulong lineNumber);
	#define ReallocateMemory(base, size) \
				DoReallocateTaggedMemory((base), (size), "untagged", __FILE__, __LINE__)
	#define	ReallocateTaggedMemory(base, size, tag) \
				DoReallocateTaggedMemory((base), (size), (tag), __FILE__, __LINE__)
	
	void*	TaggedNew(void *newObject, const char* tagName, ulong classNumber, const char* ownerURL, const char* sourceFile, ulong lineNumber);
	void	TaggedDelete(void *object, const char* sourceFile, ulong lineNumber);
	
	// debug interface to tagged memory
	void*	NextMemoryBlock(void* block, Boolean* used, ulong* length, char* name, char* ownerURL, Boolean* isCPlus, char* sourceFile, ulong* lineNumber);
	ulong	MemoryTime(void);
	
	void VerifyMemory();

#else	/* of #ifdef MEMORY_TRACKING */

	#define newArray(_x, _n) 	new _x[_n]
	
	#define	AllocateTaggedMemory(length, tag) \
				AllocateMemory(length)
	#define	AllocateTaggedMemoryNilAllowed(length, tag) \
				AllocateMemory(length, true)
	
	#define	AllocateTaggedZero(length, tag) \
				AllocateZero(length)
	#define	AllocateTaggedZeroNilAllowed(length, tag) \
				AllocateZero(length, true)
	
	#define	FreeTaggedMemory(base, tag) \
				FreeMemory(base)
	
	#define	ReallocateTaggedMemory(base, size, tag) \
				ReallocateMemory(base, size)

#endif /* of #else of #ifdef MEMORY_TRACKING */

/* -------------------------------------------------------------------------- */

void DumpFreeBlocks(void);
void DumpHeapBlocks(Boolean detailed);

typedef struct MemoryTag
{
	Boolean			fInUse;
	Boolean			fIsCPlus;
	uchar			reserved[2];
	void*			fBase;
	ulong			fLength;
	const char*		fOwnerURL;
	
	ClassNumber		fClassNumber;
	const char*		fSourceFile;
	ulong			fLineNumber;
	ulong			fCreateTime;
	ulong			fModifyTime;
	const char*		fName;
} MemoryTag;

/* -------------------------------------------------------------------------- */

#if !defined __MWERKS__ && defined _ALLOCA && !defined GENERATING68K
/* NOTE:  You need to be careful when using this on a new processor that alloca
 * and the stack usage of the compiler are sympatico.  This is most likely when
 * alloca is built into the compiler itself.  Beware of alloca implemented as an
 * inline assembly function.
 */
	#define kMaxReasonableStackSize			256
	#define AllocateBuffer(size)			(((size) <= kMaxReasonableStackSize) ? alloca(size) : AllocateMemory(size))	
	#define FreeBuffer(bufferPtr, size)		if ((size) > kMaxReasonableStackSize) FreeMemory(bufferPtr);
#else /* !_ALLOCA */
	#define AllocateBuffer(size)			AllocateMemory(size)
	#define FreeBuffer(bufferPtr, size) 	FreeMemory(bufferPtr)
#endif /* !ALLOCA */

/* -------------------------------------------------------------------------- */

#define AllocateStructCopy(src)			AllocateCopy(src, sizeof(*src))

/* -------------------------------------------------------------------------- */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MemoryManager.h multiple times"
	#endif
#endif /* __MEMORYMANAGER_H__ */
