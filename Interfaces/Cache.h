// ===========================================================================
//	Cache.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CACHE_H__
#define __CACHE_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"			/* for Priority */
#endif

#ifndef __UTILITIES_H__
#include "Utilities.h"			/* for HasDebugModifiedTime */
#endif

class Cache;
class CacheEntry;

#ifdef TEST_CLIENTTEST
class ClientTestStream;
#endif

// ===========================================================================

typedef Boolean (EachEntryFunction)(const char* name, const char* data, ulong dataLength, void* parameters);

extern Cache* gRAMCache;

class Cache : public HasDebugModifiedTime, public HasAttributes {
public:
	virtual					~Cache();
	
	long					GetEntryCount() const;
	long					GetFreeCount() const;
	long					GetLength() const;
	long					GetUsedCount() const;
	
	virtual Boolean			SetAttribute(const char* name, char* value);

	CacheEntry*				Add(const char* name, long dataCapacity=0);
	CacheEntry*				Add(const char* name, const char* postData, long postDataLength, long dataCapacity=0);
	void					Delete(const char* name);
	void					Delete(const char* name, const char* postData, long postDataLength);
	void					DisableLoadingAll();
	CacheEntry*				Find(const char* name) const;
	CacheEntry*				Find(const char* name, const char* postData, long postDataLength) const;
	void					Initialize(long dataLength, long entryCount);
	void					Initialize(char* data, long dataLength, CacheEntry*, long entryCount);
	char*					LockData(CacheEntry*);
	CacheEntry*				NextPendingEntry();
	void					Purge(CacheEntry*);
	void					PurgeAll();
	void					Remove(CacheEntry*);
	void					RemoveAll();
	Boolean					Resize(CacheEntry*, long length);
	Boolean					ResizeData(CacheEntry*, long length);
	void					UnlockData(CacheEntry*);

#ifdef DEBUG_CACHE_VALIDATE
	char*					GetData() const { return fData; };
	Boolean					IsValid() const;
#endif
	
#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize();	
#endif
	static void				Initialize();

#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintDataShort		= kFirstBoxPrint << 0,
		kBoxPrintDataLong		= kFirstBoxPrint << 1,
		kBoxPrintEntriesShort	= kFirstBoxPrint << 2,
		kBoxPrintEntriesLong	= kFirstBoxPrint << 3,
		kLastBoxPrint = kBoxPrintEntriesLong
	};
	void					BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const Cache* cache, long whatToPrint);
#endif
	
#ifdef DEBUG_CACHEWINDOW
	long					GetResetCount() const;
	void					SetResetCount(long value);
	CacheEntry*				GetCacheEntry(long index);
	long					fResetCount;
#endif

#if defined(DEBUG_CACHEWINDOW) || defined(DEBUG_MEMORYWINDOW)
	Boolean					EachEntry(EachEntryFunction* function, void* parameters);
#endif

#ifdef DEBUG_CACHE_SCRAMBLE
	void					Scramble();
	CacheEntry*				fScrambleEntry;
#endif

#ifdef DEBUG_CACHE_VIEWASHTML
	void					ViewCacheAsHTML(Boolean showContents = false);
#endif

#ifdef TEST_CLIENTTEST
	friend Boolean ClientTestCache(ClientTestStream* stream);
#endif

#ifdef TEST_SIMULATORONLY
	static void				TestLocking();
#endif

protected:
	CacheEntry*				FindFreeEntry();
	void					LoadIfLocal(CacheEntry*);
	Boolean					MakeAvailable(long capacity, CacheEntry*);
	void					SortForPending();
	void					SortForPurge();
	void					SortForRemove();

private:
	static int 				CompareForPending(CacheEntry** a, CacheEntry** b);
	static int 				CompareForPurge(CacheEntry** a, CacheEntry** b);
	static int 				CompareForRemove(CacheEntry** a, CacheEntry** b);

protected:
	CacheEntry*				fCacheFullEntry;
	char*					fData;
	long					fDataLength;
	CacheEntry*				fEntry;			// array of CacheEntrys
	long					fEntryCount;	// number of entries in the array
	CacheEntry*				fLockedEntry;	// entry containing locked data
	CacheEntry**			fSortedEntry;
	long					fSortedEntryCount;
};

#ifdef DEBUG_CACHEWINDOW
inline CacheEntry*
Cache::GetCacheEntry(long index)
{
	return ((fEntry == nil) && (index >= 0) && (index < fEntryCount)) ? nil : &(fEntry[index]);
}
#endif

#ifdef SIMULATOR
inline long
Cache::GetEntryCount() const 
{ 
	return fEntryCount; 
};
#endif

inline long
Cache::GetFreeCount() const 
{ 
	return fDataLength - GetUsedCount(); 
};

inline long
Cache::GetLength() const 
{ 
	return fDataLength; 
};

#ifdef DEBUG_CACHEWINDOW
inline long
Cache::GetResetCount() const
{
	return fResetCount;
}
#endif

#ifdef DEBUG_CACHEWINDOW
inline void
Cache::SetResetCount(long value) 
{
	fResetCount = value;
}
#endif

// ===========================================================================

#endif /* __CACHE_H__ */

