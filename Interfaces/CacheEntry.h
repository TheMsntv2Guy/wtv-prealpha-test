// ===========================================================================
//	CacheEntry.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CACHEENTRY_H__
#define __CACHEENTRY_H__

#ifndef __UTILITIES_H__
#include "Utilities.h"	/* for HasDebugModifiedTime */
#endif

class Cache;
class CacheStream;
class CacheWindow;
#ifdef TEST_CLIENTTEST
class ClientTestStream;
#endif
// ===========================================================================

typedef enum Priority
{
	kBackground = 1,
	kNearby,
	kVisible,
	kSelectable,
	kImmediate,
	kPersistent
} Priority;

// ===========================================================================

class CacheEntry : public HasDebugModifiedTime {
public:
	const char*				GetAuthorization() const;
	char*					GetData() const;
	long					GetDataCapacity() const;
	long					GetDataExpected() const;
	long					GetDataLength() const;
	DataType				GetDataType() const;
	ulong					GetExpires() const;
	ulong					GetLastModified() const;
	const char*				GetName() const;
	uchar					GetPercentComplete(ulong expectedSize) const;
	char*					GetPostData() const;
	long					GetPostDataLength() const;
	Priority				GetPriority() const;
	Error					GetStatus() const;
	Boolean					HasData() const;
	Boolean					HasDataUsers() const;
	Boolean					HasExpired() const;
	Boolean					HasUsers() const;
	Boolean					IsDataTrusted() const;
	Boolean					IsNamed(const char* name, const char* postData=nil, long postDataLength=0) const;

#ifdef DEBUG_CACHE_VALIDATE
	Boolean					IsValid() const;
#else
	Boolean					IsValid() const { return true; };
#endif

	void					SetAuthorization(const char*, Cache*);
	void					SetDataExpected(long);
	void					SetDataLength(long);
	void					SetDataType(DataType);
	void					SetExpires(ulong);
	void					SetIsDataTrusted(Boolean);
	void					SetIsLocked(Boolean);
	void					SetLastModified(ulong);
	void					SetName(const char*, Cache*);
	void					SetPostData(const char* data, long length, Cache* cache);
	void					SetPriority(Priority);
	void					SetStatus(Error);

	void					BeginUse();
	void					BeginUseData();
	void					EndUse();
	void					EndUseData();

	enum {
		kDataNeverUsed = -1,
		kDataExpectedUnknown = -1
	};
#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintName			= kFirstBoxPrint << 0,
		kBoxPrintLength			= kFirstBoxPrint << 1,
		kBoxPrintStatus			= kFirstBoxPrint << 2,
		kBoxPrintFlags			= kFirstBoxPrint << 3,
		kBoxPrintOneLiner		= kFirstBoxPrint << 4,
		kBoxPrintOneLinerHeader = kFirstBoxPrint << 5,
		kLastBoxPrint = kBoxPrintOneLinerHeader
	};
	void			BoxPrintDebug(long whatToPrint) const;
	static void		StaticBoxPrintDebug(const CacheEntry* cacheEntry, long whatToPrint);
#endif /* DEBUG_BOXPRINT */
#ifdef TEST_CLIENTTEST
	friend Boolean ClientTestCache(ClientTestStream* stream);
#endif /* TEST_CLIENTTEST */
	
	Boolean					IsLocal() const;

protected:
	friend					Cache;
	friend					CacheStream;
	friend					CacheWindow;
	
	long					GetAuthorizationCapacity() const;
	long					GetAuthorizationLength() const;
	long					GetAuthorizationOffset() const;
	char*					GetBase() const;
	long					GetDataOffset() const;
	short					GetDataUserCount() const;
	ulong					GetLastUsed() const;
	long					GetLength() const;
	long					GetNameCapacity() const;
	long					GetNameLength() const;
	long					GetNameOffset() const;
	long					GetPostDataCapacity() const;
	long					GetPostDataOffset() const;
	short					GetUserCount() const;
	Boolean					IsConstant() const;
	Boolean					IsFree() const;
	Boolean					IsLocked() const;
	Boolean					IsPending() const;
	Boolean					IsPurgable() const;
	Boolean					IsRemovable() const;
	
	void					SetBase(char*);
	void					SetIsConstant(Boolean);
	void					SetIsLocal(Boolean);
	void					SetLength(long);
	
	void					Move(long offset);
	void					Reset();

protected:
	// Note: update Reset() when adding or changing fields. 
	
	long					fAuthorizationLength;
	char*					fBase;
	long					fDataExpected;
	long					fDataLength;
	ulong					fExpires;
	ulong					fLastModified;
	ulong					fLastUsed;
	long					fLength;
	long					fNameLength;
	long					fPostDataLength;
	
	short					fDataUserCount;		// = kDataNeverUsed
	short					fUserCount;
	
	DataType				fDataType;
	Priority				fPriority;
	Error					fStatus;
	
	Boolean					fIsConstant;
	Boolean					fIsLocal;
	Boolean					fIsLocked;
	Boolean					fIsDataTrusted;
};

inline long
CacheEntry::GetAuthorizationLength() const
{
	return fAuthorizationLength;
}

inline char*
CacheEntry::GetBase() const
{
	return fBase;
}

inline long
CacheEntry::GetDataExpected() const
{
	return fDataExpected;
}

inline long
CacheEntry::GetDataLength() const
{
	return fDataLength;
}

inline DataType
CacheEntry::GetDataType() const
{
	return fDataType;
}

inline short
CacheEntry::GetDataUserCount() const
{
	return fDataUserCount;
}

inline ulong
CacheEntry::GetExpires() const
{
	return fExpires;
}

inline ulong
CacheEntry::GetLastModified() const
{
	return fLastModified;
}

inline long
CacheEntry::GetLength() const
{
	return fLength;
}

inline long
CacheEntry::GetNameLength() const
{
	return fNameLength;
}

inline long
CacheEntry::GetPostDataLength() const
{
	return fPostDataLength;
}

inline Priority
CacheEntry::GetPriority() const
{
	return fPriority;
}

inline Error
CacheEntry::GetStatus() const
{
	return fStatus;
}

inline short
CacheEntry::GetUserCount() const
{
	return fUserCount;
}

inline Boolean
CacheEntry::HasData() const
{
	return fDataLength != 0 || fDataUserCount > 0;
}

inline Boolean
CacheEntry::HasDataUsers() const
{ 
	return fDataUserCount > 0;
}

inline Boolean
CacheEntry::HasUsers() const
{ 
	return fUserCount != 0;
}

inline Boolean
CacheEntry::IsConstant() const
{
	return fIsConstant;
}

inline Boolean
CacheEntry::IsDataTrusted() const
{
	return fIsDataTrusted;
}

inline Boolean
CacheEntry::IsFree() const
{
	return GetName() == nil;
}

inline Boolean
CacheEntry::IsLocal() const
{
	return fIsLocal;
}

inline Boolean
CacheEntry::IsLocked() const
{
	return fIsLocked;
}

#endif /* __CACHEENTRY_H__ */

