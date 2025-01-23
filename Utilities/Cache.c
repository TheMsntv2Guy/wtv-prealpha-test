// ===========================================================================
//	Cache.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __BOXABSOLUTEGLOBALS_H__
#include "BoxAbsoluteGlobals.h"
#endif
#ifdef DEBUG_BOXPRINT
#include "BoxPrintDebug.h"
#endif
#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __FILESTREAM_H__
#include "FileStream.h"
#endif
#ifndef __HTTP_H__
#include "HTTP.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __NETWORK_H__
#include "Network.h"
#endif
#ifndef __STORESTREAM_H__
#include "StoreStream.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif

#ifdef TEST_CLIENTTEST
	#ifndef __CLIENTTEST_H__
	#include "ClientTest.h"
	#endif
#endif

#ifdef DEBUG_CACHE_VIEWASHTML
	#ifndef __PAGEVIEWER_H__
	#include "PageViewer.h"		/* for gPageViewer */
	#endif
#endif

#ifdef SIMULATOR
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
#endif

// ===========================================================================
//	#defines/consts
// ===========================================================================

#ifdef DEBUG_CACHE_VALIDATE
	const long kCacheSentinel = 0xabad0b0e;
#endif

// #define kErrorEntry		0

static const long kCacheDefaultDataLength = 400L * 1024;
static const long kCacheDefaultEntryCount = 256;

// ===========================================================================

inline long
ExtendForAlignment(long value)
{
	const long kAlignment = 4;
	const long kAlignmentMask = kAlignment-1;
	
	if ((value & kAlignmentMask) != 0)
		return (value & ~kAlignmentMask) + (value >= 0 ? kAlignment : -kAlignment);
	
	return value;
}

// ===========================================================================
// Class Cache

Cache::~Cache()
{
	RemoveAll();
	
	if (fData != nil) {	
		FreeTaggedMemory(fData, "Cache::fData");
		fData = nil;
	}

	if (fEntry != nil) {	
		FreeTaggedMemory(fEntry, "Cache::fEntry");
		fEntry = nil;
	}

	if (fSortedEntry != nil) {	
		FreeTaggedMemory(fSortedEntry, "Cache::fSortedEntry");
		fSortedEntry = nil;
	}
}

CacheEntry* 
Cache::Add(const char* name, long dataCapacity)
{
	return Add(name, nil, 0, dataCapacity);
}

CacheEntry* 
Cache::Add(const char* name, const char* postData, long postDataLength, long dataCapacity)
{
	CacheEntry* entry;

	if (IsError(fData == nil || fEntry == nil))
		return nil;
		
	if (IsError(name == nil || *name == 0))
		return nil;
	
	if (IsError(dataCapacity < 0))
		dataCapacity = 0;

	entry = Find(name, postData, postDataLength);
	if (IsWarning(entry != nil))
		return entry;
				
	PushDebugChildURL("<Cache>");
		
	// Find cache entry.
	entry = FindFreeEntry();
	 
	// Set attributes.
	entry->SetName(name, this);
	entry->SetPostData(postData, postDataLength, this);	
	ResizeData(entry, dataCapacity);

	LoadIfLocal(entry);

	PopDebugChildURL();

	return entry;
}

#ifdef DEBUG_BOXPRINT
void
Cache::BoxPrintDebug(long whatToPrint) const
{
#ifdef DEBUG_CACHE_VALIDATE
	if (!IsValid()) {
		BoxPrint("Cache is invalid");
		return;
	}
#endif
	
	if (whatToPrint == 0) {
		whatToPrint = kBoxPrintEntriesShort | kBoxPrintDataShort;
	}

	if (whatToPrint & kBoxPrintDataShort) {
		long usedCount = GetUsedCount();
		long percentUsed = (fDataLength == 0) ? 100 : (usedCount*100)/fDataLength;
		BoxPrint("Cache size: %ld  Used: %ld (%%%ld)  Data starts at: 0x%X",
				fDataLength, usedCount, percentUsed, fData);
	}
	
	if (whatToPrint & kBoxPrintEntriesShort) {
		int i;
		fEntry[0].BoxPrintDebug(CacheEntry::kBoxPrintOneLinerHeader);
		for (i=0; i<fEntryCount; i++) {
			if (!fEntry[i].IsFree()) {
				BoxPrint("Entry #%d", i);
				CacheEntry::StaticBoxPrintDebug(&(fEntry[i]), 0);
			}
		}
	}
}
#endif

int 
Cache::CompareForPending(CacheEntry** a, CacheEntry** b)
{
	// Return negative if a should be retrieved from the network before b.
		
	return (*b)->GetPriority() - (*a)->GetPriority();
}

int 
Cache::CompareForPurge(CacheEntry** a, CacheEntry** b)
{
	// Return negative if a should be purged before b.
	
	long priorityA = (*a)->GetPriority();
	long priorityB = (*b)->GetPriority();

	if ((*a)->HasExpired())
		return -1;
	
	if ((*b)->HasExpired())
		return 1;
	
	if (priorityA < priorityB)
		return -1;
	
	if (priorityA > priorityB)
		return 1;
		
	return (*a)->GetLastUsed() - (*b)->GetLastUsed();
}

int 
Cache::CompareForRemove(CacheEntry** a, CacheEntry** b)
{
	// Return negative if a should be removed before b.

	long priorityA = (*a)->GetPriority();
	long priorityB = (*b)->GetPriority();

	if ((*a)->HasExpired())
		return -1;
	
	if ((*b)->HasExpired())
		return 1;
	
	if (priorityA < priorityB)
		return -1;
	
	if (priorityA > priorityB)
		return 1;
		
	return (*a)->GetLastUsed() - (*b)->GetLastUsed();
}

void
Cache::Delete(const char* name)
{
	Delete(name, nil, 0);
}

void
Cache::Delete(const char* name, const char* postData, long postDataLength)
{
	CacheEntry* entry;

	if (IsWarning(name == nil || *name == 0))
		return;
	
	if ((entry = Find(name, postData, postDataLength)) == nil)
		return;

	Remove(entry);	
}
	
void
Cache::DisableLoadingAll()
{
	// Disable network loading.
	
	long i;
	
	for (i = 0; i < fEntryCount; i++)
		if (fEntry[i].GetPriority() < kPersistent)
			fEntry[i].SetPriority((Priority)0);
}

#if defined DEBUG_CACHEWINDOW
Boolean	
Cache::EachEntry(EachEntryFunction* function, void* parameters)
{
	long i;
	
	if (IsError(function == nil))
		return false;
	
	for (i = 0; i < fEntryCount; i++)
		if (!fEntry[i].IsFree())
			if ((*function)(fEntry[i].GetName(), fEntry[i].GetData(), fEntry[i].GetDataLength(), parameters))
				return true;
			
	return false;
}
#endif

#ifdef INCLUDE_FINALIZE_CODE
void
Cache::Finalize(void)
{
	if (gRAMCache == nil)
		return;		// when does this happen? -Phil
	
	// want gRAMCache to be nil so nobody tries to
	// make a call to gRAMCache while we're deleting it
	Cache*	cache = gRAMCache;
	gRAMCache = nil;		
	delete(cache);			
}
#endif

CacheEntry*
Cache::Find(const char* name) const
{
	return Find(name, nil, 0);
}

CacheEntry*
Cache::Find(const char* name, const char* postData, long postDataLength) const
{
	long i;
	
	if (name == nil || *name == 0)
		return nil;
	
	for (i = 0; i < fEntryCount; i++)
		if (fEntry[i].IsNamed(name, postData, postDataLength)) {
			if (fEntry[i].GetStatus() != kPersistent && fEntry[i].HasExpired()) {
				fEntry[i].SetDataLength(0);
				fEntry[i].SetStatus(kNoError);
			}
			return &fEntry[i];
		}
	
	return nil;
}

CacheEntry* 
Cache::FindFreeEntry()
{
	long i;
	
	// Find a free entry.
	for (i = 0; i < fEntryCount; i++)
		if (fEntry[i].IsFree())
			return &fEntry[i];
	
	// Free up a used entry.
	SortForRemove();
	if (fSortedEntryCount == 0)
		return fCacheFullEntry;
	Remove(fSortedEntry[0]);
	return fSortedEntry[0];
}

long 
Cache::GetUsedCount() const
{
	CacheEntry* last;
	long used;
	
	if (IsError(fData == nil || fEntry == nil))
		return 0;

	last = &fEntry[fEntryCount-1];	
	used = last->GetBase() + last->GetLength() - fData;
	return used;
}

#ifdef SIMULATOR
void 
Cache::Initialize(long dataLength, long entryCount)
{
	PushDebugChildURL("<Cache>");
	CacheEntry* entry = (CacheEntry*)AllocateTaggedZero(sizeof (CacheEntry) * entryCount, "Cache::fEntry");

	ulong allocSize = dataLength;
#ifdef DEBUG_CACHE_VALIDATE
	allocSize += sizeof(long);
#endif

	char* data = (char*)AllocateTaggedZero(allocSize, "Cache::fData");
#ifdef DEBUG_CACHE_VALIDATE
	if (data != nil)
		*(long*)(&(data[dataLength])) = kCacheSentinel;
#endif

	Initialize(data, dataLength, entry, entryCount);
	PopDebugChildURL();
}
#endif

void 
Cache::Initialize(char* data, long dataLength, CacheEntry* entry, long entryCount)
{
	Assert(fData == nil || fEntry == nil);
	Assert(data != nil && ((long)data & 0x3) == 0);;
	Assert(dataLength > 0 && IsMultipleOf(dataLength, 4));
	Assert(entry != nil && ((long)entry & 0x3) == 0);;
	Assert(entryCount > 0);
	
	fData = data;
	fDataLength = dataLength;
	fEntry = entry;
	fEntryCount = entryCount;
	fSortedEntry = (CacheEntry**)AllocateTaggedZero(entryCount * sizeof (CacheEntry*),
													"Cache::fSortedEntry");
	fSortedEntryCount = 0;
	RemoveAll();
}

void 
Cache::Initialize()
{
	if (IsWarning(gRAMCache != nil))
		return;
	
	PushDebugChildURL("<Cache>");

#ifdef DEBUG_CACHE_VALIDATE
	char* data = (char*)AllocateTaggedZero(kCacheDefaultDataLength + sizeof(long), "Cache::fData");
	*(long*)(&(data[kCacheDefaultDataLength])) = kCacheSentinel;
#else
	char* data = (char*)AllocateTaggedZero(kCacheDefaultDataLength, "Cache::fData");
#endif

	CacheEntry* entry = (CacheEntry*)AllocateTaggedZero(sizeof(CacheEntry) * kCacheDefaultEntryCount, "Cache::fEntry");
	gRAMCache = new(Cache);

	gRAMCache->Initialize(data, kCacheDefaultDataLength, entry, kCacheDefaultEntryCount);

	PopDebugChildURL();
}

#ifdef DEBUG_CACHE_VALIDATE
Boolean
Cache::IsValid() const
{
	long i;
	
	if (this == gRAMCache) // don't want to misfire for unit test
		if (IsError(fEntryCount != kCacheDefaultEntryCount))
			return false;

	if (IsError(*(long*)(&(fData[fDataLength])) != kCacheSentinel)) {
		Complain(("Overwrote end of cache"));
	}


	for (i = 0; i < fEntryCount; i++)
		if (!fEntry[i].IsValid())
			return false;

	if (IsError(fSortedEntryCount > fEntryCount))
		return false;
	
	return true;
}
#endif

void 
Cache::LoadIfLocal(CacheEntry* entry)
{
	// If the resource in RAM, ROM, or in the file system, mark it as
	// kComplete so that we don't attempt to read it in from the network.
	
	Resource resource;
	DataStream* stream;

	resource.SetURL(entry);
	stream = StoreStream::NewStream(&resource);
	
#ifdef SIMULATOR
	if (stream == nil)
		stream = FileStream::NewStream(&resource);
#endif

	if (stream != nil) {
		entry->SetDataType(GuessDataType(entry->GetName()));
		entry->SetIsDataTrusted(true);
		entry->SetIsLocal(true);
		entry->SetStatus(kComplete);
		delete(stream);
	}
}

char*
Cache::LockData(CacheEntry* entry)
{
	long entryDataCapacity;
	long entryIndex;
	long lockedIndex;
	long i;
	
	if (IsError(entry == nil))
		return nil;
	
	if (entry->IsLocal())
		return entry->GetData();
	
	if (entry->IsLocked())
		return fLockedEntry->GetData();
	
	if (entry->IsConstant())
		return nil;
	
	if (IsWarning(entry->GetStatus() != kComplete))
		return nil;
	
	if (IsWarning(fLockedEntry->GetDataLength() != 0))
		return nil;

	entryDataCapacity = entry->GetDataCapacity();

	// Swap entry data with everything between it and the locked entry header.
	//     From: [fLockedEntry header]...[entry header][entry data]
	//       To: [fLockedEntry header][entry data]...[entry header]
	SwapMemory(fLockedEntry->GetBase() + fLockedEntry->GetDataOffset(),
		entry->GetBase() + entry->GetDataOffset() - (fLockedEntry->GetBase() + fLockedEntry->GetDataOffset()),
		entryDataCapacity);
	
	// Adjust the base pointers of all entries touched.
	entryIndex = entry - fEntry;
	lockedIndex = fLockedEntry - fEntry;
	for (i = lockedIndex + 1; i <= entryIndex; i++)
		fEntry[i].SetBase(fEntry[i].GetBase() + entryDataCapacity);

	// Adjust length fields.
	fLockedEntry->SetLength(fLockedEntry->GetLength() + entryDataCapacity);
	fLockedEntry->SetDataLength(entry->GetDataLength());
	entry->SetLength(entry->GetLength() - entryDataCapacity);
	entry->SetDataLength(0);
	entry->SetIsLocked(true);
	return fLockedEntry->GetData();
}

Boolean 
Cache::MakeAvailable(long length, CacheEntry* growEntry)
{
	Priority priority = growEntry->GetPriority();
	long i;

	if (IsError(fData == nil || fEntry == nil))
		return true;
	
	if ((length -= GetFreeCount()) <= 0)
		return true;

	SortForPurge();

	for (i = 0; i < fSortedEntryCount; i++) {
		if (fSortedEntry[i]->GetPriority() > priority)
			if (!TrueError(fSortedEntry[i]->GetStatus()))
				continue;
			
		if (fSortedEntry[i] == growEntry)
			continue;
			
		if (fSortedEntry[i]->IsRemovable()) {
			length -= fSortedEntry[i]->GetLength();
			Remove(fSortedEntry[i]);
			
		} else if (!fSortedEntry[i]->IsLocal()) {
			length -= fSortedEntry[i]->GetDataCapacity();
			Purge(fSortedEntry[i]);

		} else
			continue;

		if (length <= 0)
			return true;
	}

	return false;
}

CacheEntry* 
Cache::NextPendingEntry()
{
	SortForPending();
	
	if (fSortedEntryCount == 0)
		return nil;
		
	return fSortedEntry[0];
}

void 
Cache::Purge(CacheEntry* entry)
{
	Message(("Cache: purging %s (%d)", entry->GetName(), entry->GetDataUserCount()));

	ResizeData(entry, 0);
	entry->SetStatus(kStreamReset);
	entry->SetPriority((Priority)0);

#ifdef SIMULATOR
	// The simulator needs to LoadIfLocal disk-based resources.
 	LoadIfLocal(entry);
#endif
}

#ifdef SIMULATOR
void 
Cache::PurgeAll()
{
	long i;

	SortForPurge();
	
	for (i = 0; i < fSortedEntryCount; i++)
		Purge(fSortedEntry[i]);
}
#endif

void 
Cache::Remove(CacheEntry* entry)
{
	long index;
	long length;
	long i;
	
#ifdef DEBUG_CACHE_SCRAMBLE
	if (gSystem->GetCacheScramble())
		Scramble();
#endif

	if (IsError(fData == nil || fEntry == nil))
		return;
	
	if (IsWarning(entry == nil))
		return;
	
	if (IsError(entry->HasUsers()))
		return;
		
	Message(("Cache: removing %s", entry->GetName()));
	index = entry - fEntry;
	length = entry->GetLength();
	for (i = index + 1; i < fEntryCount; i++)
		fEntry[i].Move(-length);
	entry->Reset();

#ifdef DEBUG_CACHE_VALIDATE
	if (gSystem->GetCacheValidate())
		IsValid();
#endif
}

void 
Cache::RemoveAll()
{
	long i;
	
	if (IsError(fData == nil || fEntry == nil))
		return;

	for (i = 0; i < fEntryCount; i++) {
		fEntry[i].Reset();
		fEntry[i].SetBase(fData);
	}
	
	// Reserve the locked entry.
	fLockedEntry = Add("cache:locked");
	fLockedEntry->SetStatus(kComplete);
	fLockedEntry->SetPriority(kPersistent);
	fLockedEntry->BeginUse();

	// Reserve the error entry.
	fCacheFullEntry = Add("cache:full");
	fCacheFullEntry->SetStatus(kCacheFull);
	fCacheFullEntry->BeginUse();
	fCacheFullEntry->SetIsConstant(true);

#ifdef DEBUG_CACHE_SCRAMBLE
	// Reserve the scramble entry.
	fScrambleEntry = Add("cache:scramble");
	fScrambleEntry->SetStatus(kComplete);
	fScrambleEntry->BeginUse();
#endif
}

Boolean 
Cache::Resize(CacheEntry* entry, long length)
{
	long increment;
	long index;
	long i;

	if (IsError(fData == nil || fEntry == nil))
		return false;
	
	if (IsError(entry == nil))
		return false;
	
	if (IsError(length < 0))
		return false;
	
	length = ExtendForAlignment(length);
	increment = length - entry->GetLength();	
	index = entry - fEntry;

	// Free up space, if necessary.
	if (increment > 0)
		if (!MakeAvailable(increment, entry))
			return false;
	
	// Move entries following entry.
	if (increment > 0)
		for (i = fEntryCount - 1; i > index; i--)
			fEntry[i].Move(increment);
	else if (increment < 0)
		for (i = index + 1; i < fEntryCount; i++)
			fEntry[i].Move(increment);
	
	// Set length.
	entry->SetLength(length);
	return true;
}

Boolean
Cache::ResizeData(CacheEntry* entry, long capacity)
{
	if (IsError(entry == nil))
		return false;
	
	if (entry->IsConstant())
		return false;
	
	PostulateFinal(false); // Cache should not know so much about CacheEntry layout
	return Resize(entry, entry->GetNameCapacity() +
			entry->GetAuthorizationCapacity() +
			entry->GetPostDataCapacity() +
			capacity);
}

#ifdef DEBUG_CACHE_SCRAMBLE
void
Cache::Scramble()
{
#ifdef DEBUG_CACHE_VALIDATE
	IsValid();
#endif

	ResizeData(fScrambleEntry, Now() & 0xFFF);

#ifdef DEBUG_CACHE_VALIDATE
	IsValid();
#endif
}
#endif

Boolean
Cache::SetAttribute(const char* name, char* value)
{
	if (EqualString(name, "wtv-purgeall")) {
		long valueLength = strlen(value);
		long i;
	
		for (i = 0; i < fEntryCount; i++)
			if (EqualStringN(fEntry[i].GetName(), value, valueLength))
				if (fEntry[i].IsPurgable())
					Purge(&fEntry[i]);
		return true;
	}
	
	return false;
}

void 
Cache::SortForPending()
{
	long i;
	
	fSortedEntryCount = 0;

	for (i = 0; i < fEntryCount; i++)
		if (fEntry[i].IsPending())
			fSortedEntry[fSortedEntryCount++] = &fEntry[i];
		
	if (fSortedEntryCount == 0)
		return;
		
	QuickSort(fSortedEntry, fSortedEntryCount, sizeof (*fSortedEntry), (QuickSortCompare)CompareForPending);
}

void 
Cache::SortForPurge()
{
	long i;
	
	fSortedEntryCount = 0;
	
	for (i = 0; i < fEntryCount; i++)
		if (fEntry[i].IsPurgable())
			fSortedEntry[fSortedEntryCount++] = &fEntry[i];
	
	if (fSortedEntryCount == 0)
		return;
		
	QuickSort(fSortedEntry, fSortedEntryCount, sizeof (*fSortedEntry), (QuickSortCompare)CompareForPurge);
}

void 
Cache::SortForRemove()
{
	long i;
	
	fSortedEntryCount = 0;
	
	for (i = 0; i < fEntryCount; i++)
		if (fEntry[i].IsRemovable())
			fSortedEntry[fSortedEntryCount++] = &fEntry[i];
	
	if (fSortedEntryCount == 0)
		return;
		
	QuickSort(fSortedEntry, fSortedEntryCount, sizeof (CacheEntry*), (QuickSortCompare)CompareForRemove);
}


#ifdef DEBUG_BOXPRINT
void
Cache::StaticBoxPrintDebug(const Cache* cache, long whatToPrint)
{
	if (cache == nil) {
		BoxPrint("Cache: <nil>");
	} else {
		BoxPrint("Cache: <%#6x>", cache);
		AdjustBoxIndent(1);
		cache->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif

#ifdef TEST_CLIENTTEST
// --------------------------------------------------------------------------
// helper to ClientTestCache()

static void CacheTestResultToStream(ClientTestStream* stream, Boolean& failed, int& numPassed, int& numAttempted, const char* message);

static void
CacheTestResultToStream(ClientTestStream* stream, Boolean& failed, int& numPassed,
						int& numAttempted, const char* message)
{
	numAttempted++;
	stream->LevelPrintf(failed ? kClientTestPrintLevelFailedTests
							   : kClientTestPrintLevelAllTests,
						"ClientTestCache() test[%d] %s %s\n",
						numAttempted,
						message,
						failed ? "failed" : "passed");
	if (!failed) {
		numPassed++;
	}
	failed = false;
}

// --------------------------------------------------------------------------

Boolean
ClientTestCache(ClientTestStream* stream)
{
	const kTestEntryCount = 16;

	char* cacheData;
	long cacheDataLength;
	Boolean failTest;
	int testsPassed = 0;
	int testsAttempted = 0;
	int startFromIndex = 1;
#ifdef DEBUG_CACHE_SCRAMBLE
	startFromIndex++;
#endif /* DEBUG_CACHE_SCRAMBLE */

	Cache* cache = new(Cache);
	cache->Initialize(4096, kTestEntryCount);
	
	if ((cache->fData == nil) || (cache->fEntry == nil) || (cache->fSortedEntry == nil)) {
		stream->LevelPrintf(kClientTestPrintLevelPassFail,
							"ClientTestCache()  Not enough memory to allocate cache.");
		delete(cache);
		return false;
	}
	
	// Account for preallocated entries
	cacheData = cache->fEntry[startFromIndex].GetBase();
	cacheDataLength = cache->fDataLength - cache->fCacheFullEntry->GetLength();
#ifdef DEBUG_CACHE_SCRAMBLE
	cacheDataLength -= cache->fScrambleEntry->GetLength();
#endif

	// Test RemoveAll
	failTest = IsError(cache->fEntry[0].GetBase() != cache->fData);
	failTest = IsError(cache->fEntry[startFromIndex].GetBase() != cacheData) || failTest;
	cache->RemoveAll();
	failTest = IsError(cache->fEntry[0].GetBase() != cache->fData) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex].GetBase() != cacheData) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"RemoveAll...step 1");
	
	CacheEntry* entry = cache->FindFreeEntry();	
	failTest = IsError(entry != &cache->fEntry[startFromIndex]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"RemoveAll...step 2");
	
	CacheEntry* fred = cache->Add("fred", 0);
	failTest = IsError(fred != &cache->fEntry[startFromIndex]) || failTest;
	failTest = IsError(fred->GetBase() != &cacheData[0]) || failTest;
	failTest = IsError(!fred->IsNamed("fred")) || failTest;
	failTest = IsError(fred->GetLength() != 8) || failTest;
	failTest = IsError(cache->Find("fred") != fred) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength - 8) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+1].GetBase() != &cacheData[8]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"RemoveAll...step 3");
	
	entry = cache->FindFreeEntry();	
	failTest = IsError(entry != &cache->fEntry[startFromIndex+1]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"RemoveAll...step 4");
	
	CacheEntry* wilma = cache->Add("wilma",10);
	failTest = IsError(wilma != &cache->fEntry[startFromIndex+1]) || failTest;
	failTest = IsError(wilma->GetData() != &cacheData[16]) || failTest;
	failTest = IsError(!wilma->IsNamed("wilma")) || failTest;
	failTest = IsError(wilma->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("wilma") != wilma) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-28) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+2].GetBase() != &cacheData[28]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"RemoveAll...step 5");
	
	CacheEntry* betty = cache->Add("betty", 10);
	failTest = IsError(betty != &cache->fEntry[startFromIndex+2]) || failTest;
	failTest = IsError(betty->GetData() != &cacheData[36]) || failTest;
	failTest = IsError(!betty->IsNamed("betty")) || failTest;
	failTest = IsError(betty->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("betty") != betty) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-48) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+3].GetBase() != &cacheData[48]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"RemoveAll...step 6");
	
	cache->Remove(wilma);
	failTest = IsError(!cache->fEntry[startFromIndex+1].IsFree()) || failTest;
	failTest = IsError(betty != &cache->fEntry[startFromIndex+2]) || failTest;
	failTest = IsError(betty->GetData() != &cacheData[16]) || failTest;
	failTest = IsError(!betty->IsNamed("betty")) || failTest;
	failTest = IsError(betty->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("betty") != betty) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-28) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+2].GetBase() != &cacheData[8]) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+3].GetBase() != &cacheData[28]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"RemoveAll...step 7");
	
	// Test resizing name with no data.
	fred->SetName("fred-flintstone", cache);
	failTest = IsError(fred->GetLength() != 16) || failTest;
	failTest = IsError(betty != &cache->fEntry[startFromIndex+2]) || failTest;
	failTest = IsError(betty->GetData() != &cacheData[24]) || failTest;
	failTest = IsError(!betty->IsNamed("betty")) || failTest;
	failTest = IsError(betty->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("betty") != betty) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-36) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+2].GetBase() != &cacheData[16]) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+3].GetBase() != &cacheData[36]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"Resizing name with no data...step 1");

	fred->SetName("fred", cache);
	failTest = IsError(fred->GetLength() != 16) || failTest;
	failTest = IsError(betty != &cache->fEntry[startFromIndex+2]) || failTest;
	failTest = IsError(betty->GetData() != &cacheData[24]) || failTest;
	failTest = IsError(!betty->IsNamed("betty")) || failTest;
	failTest = IsError(betty->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("betty") != betty) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-36) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+2].GetBase() != &cacheData[16]) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+3].GetBase() != &cacheData[36]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"Resizing name with no data...step 2");

	// Test resizing data & alignment (entry following resize of 5 should still be
	// longword aligned).
	cache->ResizeData(fred, 5);
	failTest = IsError(fred != &cache->fEntry[startFromIndex]) || failTest;
	failTest = IsError(fred->GetData() != &cacheData[8]) || failTest;
	failTest = IsError(fred->GetDataCapacity() != 8) || failTest;
	strcpy(fred->GetData(), "abcdefg");
	failTest = IsError(!fred->IsNamed("fred")) || failTest;
	failTest = IsError(fred->GetLength() != 16) || failTest;
	failTest = IsError(cache->Find("fred") != fred) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-36) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+1].GetBase() != &cacheData[16]) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+1].GetData() != nil) || failTest;
	failTest = IsError(!cache->fEntry[startFromIndex+1].IsFree()) || failTest;
	failTest = IsError(betty != &cache->fEntry[startFromIndex+2]) || failTest;
	failTest = IsError(betty->GetData() != &cacheData[24]) || failTest;
	failTest = IsError(!betty->IsNamed("betty")) || failTest;
	failTest = IsError(betty->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("betty") != betty) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+2].GetBase() != &cacheData[16]) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+3].GetBase() != &cacheData[36]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"Resizing data & alignment");

	// Test resizing name with data.
	fred->SetName("fred-flintstone-bedrock", cache);
	failTest = IsError(fred->GetLength() != 32) || failTest;
	failTest = IsError(fred->GetData() != &cacheData[24]) || failTest;
	failTest = IsError(fred->GetDataCapacity() != 8) || failTest;
	failTest = IsError(strcmp(fred->GetData(), "abcdefg") != 0) || failTest;
	failTest = IsError(betty != &cache->fEntry[startFromIndex+2]) || failTest;
	failTest = IsError(betty->GetData() != &cacheData[40]) || failTest;
	failTest = IsError(!betty->IsNamed("betty")) || failTest;
	failTest = IsError(betty->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("betty") != betty) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-52) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+2].GetBase() != &cacheData[32]) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+3].GetBase() != &cacheData[52]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"Resizing name w/data...step 1");

	fred->SetName("fred", cache);
	failTest = IsError(fred->GetLength() != 32) || failTest;
	failTest = IsError(fred->GetData() != &cacheData[8]) || failTest;
	failTest = IsError(fred->GetDataCapacity() != 24) || failTest;
	failTest = IsError(strcmp(fred->GetData(), "abcdefg") != 0) || failTest;
	failTest = IsError(betty != &cache->fEntry[startFromIndex+2]) || failTest;
	failTest = IsError(betty->GetData() != &cacheData[40]) || failTest;
	failTest = IsError(!betty->IsNamed("betty")) || failTest;
	failTest = IsError(betty->GetLength() != 20) || failTest;
	failTest = IsError(cache->Find("betty") != betty) || failTest;
	failTest = IsError(cache->GetFreeCount() != cacheDataLength-52) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+2].GetBase() != &cacheData[32]) || failTest;
	failTest = IsError(cache->fEntry[startFromIndex+3].GetBase() != &cacheData[52]) || failTest;
	CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
							"Resizing name w/data...step 2");
	
	{ 	// Test sorting.
		cache->RemoveAll();
		CacheEntry* two = cache->Add("two", 2);
		two->fLastUsed = 2;
		CacheEntry* four = cache->Add("four", 4);
		four->fLastUsed = 4;
		CacheEntry* one = cache->Add("one", 1);
		one->fLastUsed = 1;
		CacheEntry* three = cache->Add("three", 3);
		three->fLastUsed = 3;
		cache->SortForRemove();
	}

	{	// Test expiration.
		CacheEntry* test;
		
		if (gClock->GetDateTimeGMT() == 0)
			gClock->SetDateTime(0xFFFF);

		test = cache->Add("expire test", 1);
		test->SetDataLength(0);
		test->SetExpires(1);
		test->SetStatus(kComplete);
		failTest = IsError(!test->HasExpired()) || failTest;
		test->BeginUseData();
		failTest = IsError(test->HasExpired()) || failTest;
		test->EndUseData();
		failTest = IsError(!test->HasExpired()) || failTest;
		cache->RemoveAll();
		CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
								"Expiration...step 1");

		test = cache->Add("expire test", 1);
		test->SetDataLength(1);
		test->SetStatus(kComplete);
		test->SetExpires(1);
		failTest = IsError(test->HasExpired()) || failTest;
		test->BeginUseData();
		failTest = IsError(test->HasExpired()) || failTest;
		test->EndUseData();
		failTest = IsError(!test->HasExpired()) || failTest;
		cache->RemoveAll();
		CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
								"Expiration...step 2");

		test = cache->Add("expire test", 1);
		test->SetDataLength(0);
		test->SetExpires(0x7FFFFFFF);
		failTest = IsError(test->HasExpired()) || failTest;
		test->BeginUseData();
		failTest = IsError(test->HasExpired()) || failTest;
		test->EndUseData();
		failTest = IsError(test->HasExpired()) || failTest;
		cache->RemoveAll();
		CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
								"Expiration...step 3");

		test = cache->Add("expire test", 1);
		test->SetDataLength(1);
		test->SetExpires(0x7FFFFFFF);
		failTest = IsError(test->HasExpired()) || failTest;
		test->BeginUseData();
		failTest = IsError(test->HasExpired()) || failTest;
		test->EndUseData();
		failTest = IsError(test->HasExpired()) || failTest;
		cache->RemoveAll();
		CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
								"Expiration...step 4");
	}
	
	{	// Test too many entries
		long i;
		cache->RemoveAll();
		CacheEntry* entryArray[kTestEntryCount-2];
		
		for (i = 0; i < kTestEntryCount-2; i++) {
			char addName[11 + 1]; /* 11 for %ld, 1 for NULL */
			snprintf(addName, sizeof(addName), "%ld", i);
			entryArray[i] = cache->Add(addName, 0);
			entryArray[i]->BeginUse();
		}
		for (i = 0; i < kTestEntryCount-2; i++) {
			char findName[11 + 1]; /* 11 for %ld, 1 for NULL */
			snprintf(findName, sizeof(findName), "%ld", i);
			failTest = IsError(cache->Find(findName) != entryArray[i]) || failTest;
		}
		failTest = IsError(cache->Add("too many") != cache->fCacheFullEntry) || failTest;
		CacheTestResultToStream(stream, failTest, testsPassed, testsAttempted,
								"Try to make too many entries");
		cache->RemoveAll();
	}
	
	delete(cache);
	
	stream->LevelPrintf(kClientTestPrintLevelSubUnitPassFail,
					"ClientTestCache %s",
					failTest ? "***** fail *****" : "pass");
	stream->LevelPrintf(kClientTestPrintLevelScore,
					"(passed %d of %d tests)", testsPassed, testsAttempted);
	stream->LevelPrintf(kClientTestPrintLevelSubUnitPassFail, "\n");

	return !failTest;
}
#endif /* TEST_CLIENTTEST */

#ifdef TEST_SIMULATORONLY
void
Cache::TestLocking()
{
#define kOtherData			"0123456789"
#define kOtherDataLength	(strlen(kOtherData) + 1)
#define kTestData			"abcdefghijklmnop"
#define kTestDataLength		(strlen(kTestData) + 1)

	Cache* cache = new(Cache);
	cache->Initialize(4096, 16);
	
	CacheEntry* other = cache->Add("other", 0);
	cache->ResizeData(other, kOtherDataLength);
	strcpy(other->GetData(), kOtherData);
	other->SetDataLength(kOtherDataLength);
	other->SetStatus(kComplete);
	
	CacheEntry* test = cache->Add("test", 0);
	cache->ResizeData(test, kTestDataLength);
	strcpy(test->GetData(), kTestData);
	test->SetDataLength(kTestDataLength);
	test->SetStatus(kComplete);

	Assert(EqualString(other->GetData(), kOtherData));
	Assert(other->GetDataLength() == kOtherDataLength);
	Assert(EqualString(test->GetData(), kTestData));
	Assert(test->GetDataLength() == kTestDataLength);
	Assert(cache->fLockedEntry->GetData() == nil);
	Assert(cache->fLockedEntry->GetDataLength() == 0);

	char* result = cache->LockData(test);
	Assert(result == cache->fLockedEntry->GetData());
	Assert(EqualString(other->GetData(), kOtherData));
	Assert(other->GetDataLength() == kOtherDataLength);
	Assert(test->GetData() == nil);
	Assert(test->GetDataLength() == 0);
	Assert(EqualString(cache->fLockedEntry->GetData(), kTestData));
	Assert(cache->fLockedEntry->GetDataLength() == kTestDataLength);
	
	cache->UnlockData(test);
	Assert(EqualString(other->GetData(), kOtherData));
	Assert(other->GetDataLength() == kOtherDataLength);
	Assert(EqualString(test->GetData(), kTestData));
	Assert(test->GetDataLength() == kTestDataLength);
	Assert(cache->fLockedEntry->GetData() == nil);
	Assert(cache->fLockedEntry->GetDataLength() == 0);
	cache->RemoveAll();
	delete(cache);
}
#endif


void
Cache::UnlockData(CacheEntry* entry)
{
	long entryDataCapacity;
	long entryIndex;
	long lockedIndex;
	long i;

	if (IsError(entry == nil))
		return;
	
	if (entry->IsLocal())
		return;
	
	if (IsWarning(!entry->IsLocked()))
		return;

	if (entry->IsConstant())
		return;

	entryDataCapacity = fLockedEntry->GetDataCapacity();

	// Swap entry data with everything between it and the locked entry header.
	//     From: [fLockedEntry header][entry data]...[entry header]
	//       To: [fLockedEntry header]...[entry header][entry data]
	SwapMemory(fLockedEntry->GetBase() + fLockedEntry->GetDataOffset(),
		entryDataCapacity,
		entry->GetBase() + entry->GetDataOffset() - (fLockedEntry->GetBase() + fLockedEntry->GetDataOffset() + entryDataCapacity));
	
	// Adjust the base pointers of all entries touched.
	entryIndex = entry - fEntry;
	lockedIndex = fLockedEntry - fEntry;
	for (i = lockedIndex + 1; i <= entryIndex; i++)
		fEntry[i].SetBase(fEntry[i].GetBase() - entryDataCapacity);

	// Adjust length fields.
	entry->SetLength(entry->GetLength() + entryDataCapacity);
	entry->SetDataLength(fLockedEntry->GetDataLength());
	fLockedEntry->SetLength(fLockedEntry->GetLength() - entryDataCapacity);
	fLockedEntry->SetDataLength(0);
	entry->SetIsLocked(false);
}

#ifdef DEBUG_CACHE_VIEWASHTML
void
Cache::ViewCacheAsHTML(Boolean UNUSED(showContents))
{
	const char kRAMStoreCacheHTML[] = "file://RAM/CacheContents.html";

	// delete previous kRAMStoreCacheHTML
	char* data = ::Remove(kRAMStoreCacheHTML + strlen("file:/"));
	if (data != nil)
		FreeTaggedMemory(data, nil);	// whatever tag was used is just fine...

	// write to a MemoryStream	
	MemoryStream* stream = new(MemoryStream);
	if (stream == nil)
		return;
	stream->WriteString("<HTML>\n");
	stream->WriteString("<HEAD><TITLE>Client:ViewCacheAsHTML</TITLE></HEAD>\n");
	stream->WriteString("<BODY>\n");
	Boolean isValid = true;
#ifdef DEBUG_CACHE_VALIDATE
	isValid = IsValid();
	stream->WriteString("cache-&gt;IsValid()=");
	stream->WriteString(isValid ? "true\n" : "false\n");
#endif /* DEBUG_CACHE_VALIDATE */
	if (isValid) {
		stream->WriteString("<TABLE>\n");
		stream->WriteString("<CAPTION>Client:ViewCacheAsHTML</CAPTION>\n");
		stream->WriteString("<FONT SIZE=\"1\">\n");
		stream->WriteString("<TH>Status/Priority</TH>");
		stream->WriteString("<TH>Users</TH>");
		stream->WriteString("<TH>Length</TH>");
		stream->WriteString("<TH>Type</TH>");
		stream->WriteString("<TH>Name</TH>");
		stream->WriteString("<TR>\n");
		
		// write a table row for each CacheEntry, from priority 6 down to 0
		for (int priority = kPersistent; priority>=0; priority--) { 
			for (int i=0; i<fEntryCount; i++) {
				if (fEntry[i].GetPriority() == priority) {
					stream->WriteString("<TD>");
#ifdef DEBUG_NAMES
					stream->WriteString(GetErrorString(fEntry[i].GetStatus()));
#else
					stream->WriteNumeric(fEntry[i].GetStatus());
#endif
					stream->WriteString(" (");
					stream->WriteNumeric(fEntry[i].GetPriority());
					stream->WriteString(")");
					
					stream->WriteString("<TD>");
					stream->WriteNumeric(fEntry[i].GetUserCount());
					if (fEntry[i].GetUserCount() != fEntry[i].GetDataUserCount()) {	
						stream->WriteString("(");
						stream->WriteNumeric(fEntry[i].GetDataUserCount());
						stream->WriteString(")");
					}
					stream->WriteString("<TD>");
					stream->WriteNumeric(fEntry[i].GetDataLength());
					if (fEntry[i].GetLength() != fEntry[i].GetDataLength()) {
						stream->WriteString("<BR><B>(" /*)*/);
						stream->WriteNumeric(fEntry[i].GetLength());
						stream->WriteString(/*(*/ ")</B>");
					}
					
					stream->WriteString("<TD>");
					DataType type = fEntry[i].GetDataType();
					if (! (isalnum((type>>24)&0xff)
							&& isalnum((type>>16)&0xff)
							&& isalnum((type>>8)&0xff)
							&& isalnum(type&0xff)))
						type = kDataTypeUnprintable;
					stream->Write((void*)&type, 4);
					
					stream->WriteString("<TD>");
					stream->WriteString("<A HREF=\"");
					stream->WriteString(fEntry[i].GetName());
					stream->WriteString("\">");
					stream->WriteString(fEntry[i].GetName());
					stream->WriteString("</A>");
					
					stream->WriteString("<TR>");
				}
			}
		}
		stream->WriteString("</FONT>\n");
		stream->WriteString("</TABLE>\n");
	}
	stream->WriteString("</BODY>\n");
	stream->WriteString("</HTML>\n");
	
	ulong length = stream->GetDataLength();
	data = stream->RemoveData();

	Create(kRAMStoreCacheHTML + strlen("file:/"), data, length);
	
	delete(stream);
	
	gPageViewer->ExecuteURL(kRAMStoreCacheHTML);
}
#endif /* DEBUG_CACHE_VIEWASHTML */

// =============================================================================

