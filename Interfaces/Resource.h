// ===========================================================================
//	Resource.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"	/* for Priority and CacheEntry functions (for inlines) */
#endif
#ifndef __LIST_H__
#include "List.h"		/* for Listable */
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"	/* for CopyString */
#endif

class CacheStream;
class ContentView;
class MIDI;
class DataStream;
class FileStream;
class HTTPCommand;
class ServiceList;
class StoreStream;
class VisitedList;
class VisitedURL;

// =============================================================================
// Constants

const kDefaultImageExpectedSize		= 16*1024;
const kDefaultHTMLExpectedSize		= 4*1024;

// =============================================================================
// Class Resource

class Resource : public Listable {
public:
							Resource();
							Resource(const Resource&);
							~Resource();
	
	Resource&				operator=(const Resource&);
	Boolean					operator==(const Resource&) const;
	Boolean					operator!=(const Resource&) const;

	char*					CopyURL(const char* tagName) const;
	const char*				GetAuthorization() const;
	CacheEntry*				GetCacheEntry() const;
	long					GetDataExpected() const;
	long					GetDataLength() const;
	DataType				GetDataType() const;
	const char*				GetDataTypeString() const;
	ulong					GetExpires() const;
	ulong					GetLastModified() const;
	uchar					GetPercentComplete(ulong dataExpected) const;
	char*					GetPostData() const;
	long					GetPostDataLength() const;
	Priority				GetPriority() const;
	ulong					GetTimeoutAt() const;	// time at which this resource times out
	Error					GetStatus() const;
	Boolean					HasExpired() const;
	Boolean					HasURL() const;
	Boolean					IsDataTrusted() const;
	Boolean					IsLocal() const;
	Boolean					IsValid() const;
	
	void					SetAuthorization(const char*);
	void					SetDataType(DataType);
	void					SetIsDataTrusted(Boolean);	
	void					SetPriority(Priority);
	void					SetStatus(Error);
	void					SetTimeoutAt(ulong);
	void					SetURL(CacheEntry*);
	void					SetURL(const char* url);
	void					SetURL(const char* url, const char* postData, long postDataLength);
	void					SetURL(const char* partial, const Resource* parent);
	void					SetURL(const char* partial, const Resource* parent, const char* postData, long postDataLength);
	
	char*					LockData();
	DataStream*				NewStream() const;
	CacheStream*			NewStreamForAppend() const;
	CacheStream*			NewStreamForWriting() const;
	void					Purge();
	void					Reset();
	void					UnlockData();

#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintCacheEntry		= kFirstBoxPrint << 0,
		kBoxPrintTimeoutAt		= kFirstBoxPrint << 1,
		kLastBoxPrint = kBoxPrintTimeoutAt
	};
	void					BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const Resource* resource, long whatToPrint);
#endif /* DEBUG_BOXPRINT */

protected:
	friend					ContentView;
	friend					MIDI;
	friend					FileStream;
	friend					HTTPCommand;
	friend					ServiceList;
	friend					StoreStream;
	friend					VisitedList;
	friend					VisitedURL;
	const char*				GetURL() const;

protected:
	CacheEntry*				fCacheEntry;
	ulong					fTimeoutAt;
};

inline char*
Resource::CopyURL(const char* USED_FOR_MEMORY_TRACKING(tagName)) const
{
	return fCacheEntry != nil ? CopyString(fCacheEntry->GetName(), tagName) : nil;
}

inline CacheEntry*
Resource::GetCacheEntry() const
{
	return fCacheEntry;
}

inline const char*
Resource::GetAuthorization() const
{
	return fCacheEntry != nil ? fCacheEntry->GetAuthorization() : nil;
}

inline long
Resource::GetDataExpected() const
{
	return fCacheEntry != nil ? fCacheEntry->GetDataExpected() : 0;
}

inline long
Resource::GetDataLength() const
{
	return fCacheEntry != nil ? fCacheEntry->GetDataLength() : 0;
}

inline DataType
Resource::GetDataType() const
{
	return fCacheEntry != nil ? fCacheEntry->GetDataType() : (DataType)0;
}

inline ulong
Resource::GetExpires() const
{
	return fCacheEntry != nil ? fCacheEntry->GetExpires() : 0;
}

inline uchar
Resource::GetPercentComplete(ulong expectedSize) const
{
	return fCacheEntry != nil ? fCacheEntry->GetPercentComplete(expectedSize) : 0;
}

inline char*
Resource::GetPostData() const
{
	return fCacheEntry != nil ? fCacheEntry->GetPostData() : nil;
}

inline long
Resource::GetPostDataLength() const
{
	return fCacheEntry != nil ? fCacheEntry->GetPostDataLength() : 0;
}

inline ulong
Resource::GetLastModified() const
{
	return fCacheEntry != nil ? fCacheEntry->GetLastModified() : 0;
}

inline Priority
Resource::GetPriority() const
{
	return fCacheEntry != nil ? fCacheEntry->GetPriority() : (Priority)0;
}

inline const char*
Resource::GetURL() const
{
	return fCacheEntry != nil ? fCacheEntry->GetName() : nil;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline ulong
Resource::GetTimeoutAt() const
{
	return fTimeoutAt;
}
#endif

inline Boolean
Resource::HasExpired() const
{
	return fCacheEntry != nil ? fCacheEntry->HasExpired() : false;
}

inline Boolean
Resource::HasURL() const
{
	return fCacheEntry != nil && fCacheEntry->GetName() != nil;
}

inline Boolean
Resource::IsDataTrusted() const
{
	return fCacheEntry != nil && fCacheEntry->IsDataTrusted();
}

inline Boolean
Resource::IsValid() const
{
	return fCacheEntry != nil;
}

inline void 
Resource::SetDataType(DataType value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetDataType(value);
}

inline void
Resource::SetIsDataTrusted(Boolean value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetIsDataTrusted(value);
}

inline void 
Resource::SetPriority(Priority value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetPriority(value);
}

inline void
Resource::SetStatus(Error value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetStatus(value);
}

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Resource.h multiple times"
	#endif
#endif /* __RESOURCE_H__ */
