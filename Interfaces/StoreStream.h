// ===========================================================================
//	StoreStream.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __STORESTREAM_H__
#define __STORESTREAM_H__

#ifndef __OBJECTSTORE_H__
#include "ObjectStore.h"		/* FSNode */
#endif

#ifndef __RESOURCE_H__
#include "Resource.h"			/* Resource */
#endif

#ifndef __STREAM_H__
#include "Stream.h"				/* DataStream */
#endif

// =============================================================================

class StoreStream : public DataStream
{
public:
							StoreStream();
	virtual					~StoreStream();
	
	virtual char*			GetData() const;
	virtual const char*		GetDataAsString();	// guarantees null termination
	virtual long			GetDataExpected() const;
	virtual long			GetDataLength() const;
	virtual DataType		GetDataType() const;
	virtual Error			GetStatus() const;

	virtual void			SetDataType(DataType);
	virtual void			SetStatus(Error);

	Boolean					Initialize(const Resource*);
	virtual void			Write(const void* data, long count);
	
	static StoreStream*		NewStream(const Resource*);

protected:
	virtual const char*		GetName() const;

	FSNode*					fNode;
	Resource				fResource;
};

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include StoreStream.h multiple times"
	#endif
#endif /* __STORESTREAM_H__ */
