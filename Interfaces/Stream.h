// ===========================================================================
//	Stream.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __STREAM_H__
#define __STREAM_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"		/* for Priority */
#endif
#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"	/* for Error */
#endif
#ifndef __LINKABLE_H__
#include "Linkable.h"		/* for Linkable */
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"		/* for HasAttributes */
#endif

// =============================================================================

class Stream : public Linkable {
public:
							Stream();
	virtual					~Stream();
	
	virtual long			GetDataExpected() const;
	virtual uchar			GetPercentComplete(ulong dataExpected) const;
	virtual Error			GetStatus() const;

	virtual void			SetDataExpected(long);
	virtual void			SetStatus(Error);
	
	int						Printf(const char* format, ...);
	virtual long			Read(void* data, long count);		// returns count actually read
	int						VPrintf(const char* format, va_list list);
	virtual void			Write(const void* data, long count);
	void					WriteAttribute(const char* name, long value);
	void					WriteAttribute(const char* name, const char* value);
	void					WriteNumeric(long);
	void					WriteString(const char*);
	void					WriteHTMLEscapedString(const char*);
};

// =============================================================================
// A DataStream is a stream that is named, supports random access, and has priority.

class DataStream : public Stream, public HasAttributes {
public:
			 				DataStream();
	virtual	 				~DataStream();

	char*					CopyName(const char* tagName) const;
	virtual char*			GetData() const;
	virtual const char*		GetDataAsString();	// guarantees null termination
	virtual long			GetDataLength() const;
	virtual DataType		GetDataType() const;
	virtual ulong			GetExpires() const;
	virtual ulong			GetLastModified() const;
	virtual const char*		GetName() const;
	long					GetPending() const;
	virtual uchar			GetPercentComplete(ulong dataExpected) const;
	long					GetPosition() const;
	virtual Priority		GetPriority() const;
	virtual Error			GetStatus() const;
	virtual Boolean			IsDataTrusted() const;
	
	virtual Boolean			SetAttribute(const char* name, char* value);
	virtual void			SetDataType(DataType);
	virtual void			SetExpires(ulong);
	virtual void			SetIsDataTrusted(Boolean);
	virtual void			SetLastModified(ulong);
	virtual void			SetName(const char*);
	virtual void			SetPriority(Priority);
	virtual void			SetStatus(Error);
	
	void					FastForward();
	DataType				ParseDataType(char* value);
	virtual long			Read(void* data, long length);			// returns actual length
	virtual const char*		ReadNext(long length);					// length must be <= pending
	virtual void			Rewind();
	void					Unread(long length);
	void					WriteQuery(const char* name, long value);
	void					WriteQuery(const char* name, const char* value);

protected:
	void					WriteQueryString(const char*);

protected:
	long					fPosition;
};

inline long 
DataStream::GetPending() const
{
	return GetDataLength() - fPosition;
}

inline long 
DataStream::GetPosition() const
{
	return fPosition;
}

// =============================================================================

class MemoryStream : public DataStream {
public:
	virtual					~MemoryStream();
	
	virtual char*			GetData() const;
	const char*				GetDataAsString();	// guarantees null termination
	virtual long			GetDataExpected() const;
	virtual long			GetDataLength() const;
	virtual DataType		GetDataType() const;
	virtual ulong			GetExpires() const;
	virtual ulong			GetLastModified() const;
	virtual Priority		GetPriority() const;
	virtual Error			GetStatus() const;
	
	virtual void			SetDataType(DataType);
	virtual void			SetExpires(ulong);
	virtual void			SetLastModified(ulong);
	virtual void			SetName(const char*);
	virtual void			SetPriority(Priority);
	virtual void			SetStatus(Error);

	char*					RemoveData();	// Transfers ownership to the caller
	void					Reset();
	virtual void			Write(const void* data, long count);

protected:
	virtual long			GetLengthIncrement() const;
	virtual	long			GetLengthInitial() const;
	virtual const char*		GetName() const;
	void					MakeAvailable(long length);

protected:
	char*					fData;
	long					fDataExpected;
	long					fDataLength;
	long					fExpires;
	long					fLastModified;
	char*					fName;

	DataType				fDataType;
	Priority				fPriority;
	Error					fStatus;
};

// =============================================================================

class RC4Stream : public DataStream {
public:
							RC4Stream();
	virtual					~RC4Stream();

	virtual	void			SetKey(void* key);	
	
	virtual long			Read(void* data, long count);
	virtual void			Write(const void* data, long count);

protected:
	void*			fKey;
};

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Stream.h multiple times"
	#endif
#endif /* __STREAM_H__ */
