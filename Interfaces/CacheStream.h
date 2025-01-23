// ===========================================================================
//	CacheStream.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CACHESTREAM_H__
#define __CACHESTREAM_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"			/* for Priority */
#endif
#ifndef __STREAM_H__
#include "Stream.h"				/* for DataStream */
#endif

class Cache;
class CacheEntry;
class CacheStream;
class Network;
class Resource;

// ===========================================================================

typedef enum {
	kWriteNormally = 0,
	kWriteIgnoreUntilBoundary,
	kWriteIgnoreUntilBoundaryCRLF,
	kWriteReadHeaders,
	kWriteUntilBoundary,
	kWriteUntilBoundaryFinal
} WriteState;

class CacheStream : public DataStream {
public:
							CacheStream(); // John: make this protected
	virtual					~CacheStream();
	
	Cache*					GetCache() const;
	virtual char*			GetData() const;
	virtual const char*		GetDataAsString();	// guarantees null termination
	virtual long			GetDataExpected() const;
	virtual long			GetDataLength() const;
	virtual DataType		GetDataType() const;
	virtual ulong			GetExpires() const;
	virtual ulong			GetLastModified() const;
	virtual uchar			GetPercentComplete(ulong expectedSize) const;
	virtual Priority		GetPriority() const;
	virtual Error			GetStatus() const;
	virtual Boolean			IsDataTrusted() const;
	
	virtual Boolean			SetAttribute(const char* name, char* value);
	void					SetBoundary(const char*);
	virtual void			SetDataExpected(long);
	virtual void			SetDataType(DataType);
	virtual void			SetExpires(ulong);
	virtual void			SetIsDataTrusted(Boolean);
	virtual void			SetLastModified(ulong);
	virtual void			SetPriority(Priority);
	virtual void			SetStatus(Error);
	virtual void			SetName(const char*);

	void					Initialize(Cache*, const char* name);
	void					Initialize(Cache*, CacheEntry*);
	virtual void			Rewind();
	virtual void			Write(const void* data, long count);
#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintCacheEntry		= kFirstBoxPrint << 0,
		kLastBoxPrint = kBoxPrintCacheEntry
	};
	virtual void			BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const CacheStream* cacheStream, long whatToPrint);
#endif /* DEBUG_BOXPRINT */

protected:
	friend					Network;
	friend					Resource;
	static CacheStream*		NewStream(CacheEntry*);
	virtual const char*		GetName() const;

protected:
	virtual long			GetLengthIncrement() const;
	virtual	long			GetLengthInitial() const;
	Boolean					MakeAvailable(long length);
	void					WriteIgnoreUntilBoundary(const char* data, long length);
	void					WriteIgnoreUntilBoundaryCRLF(const char* data, long length);
	void					WriteNormally(const char* data, long length);
	void					WriteReadHeaders(const char* data, long length);
	void					WriteUntilBoundary(const char* data, long length);
	void					WriteUntilBoundaryFinal(const char* data, long length);

protected:
	enum { kBoundaryCharacters = 76 };
	enum { kHeaderCharacters = 128 };
	
	char					fBoundary[kBoundaryCharacters];
	char					fHeader[kHeaderCharacters];
	Cache*					fCache;
	CacheEntry*				fCacheEntry;
	
	short					fBoundaryCount;
	short					fBoundaryFinalCount;
	
	WriteState				fWriteState;
};

// ===========================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include CacheStream.h multiple times"
	#endif
#endif /* __CACHESTREAM_H__ */
