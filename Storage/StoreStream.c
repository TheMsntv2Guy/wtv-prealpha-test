// ===========================================================================
//	StoreStream.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __STORESTREAM_H__
#include "StoreStream.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#ifdef SIMULATOR
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
#endif




// ===========================================================================
//	#defines
// ===========================================================================

#define kRelativeOffset			(-1)	// because current file usage is relative




// ===========================================================================
//	implementation
// ===========================================================================

StoreStream::StoreStream()
{
}

StoreStream::~StoreStream()
{
}

char* StoreStream::GetData() const
{
	return fNode->data;
}

long StoreStream::GetDataExpected() const
{
	return fNode->dataLength;
}

long StoreStream::GetDataLength() const
{
	return fNode->dataLength;
}

DataType StoreStream::GetDataType() const
{
	return fResource.GetDataType();
}

const char* StoreStream::GetName() const
{
	return fResource.GetURL();
}

Error StoreStream::GetStatus() const
{
	return fResource.GetStatus();
}

Boolean StoreStream::Initialize(const Resource* resource)
{
	static const char kFilePrefix[] = "file://";
	static const kFilePrefixSize = sizeof(kFilePrefix) - 1;

	if (IsError(resource == nil))
		return false;
	
	const char* url = resource->GetURL();
	
	if (IsError(url == nil))
		return false;
	
	if (!EqualStringN(url, kFilePrefix, kFilePrefixSize))
		return false;

#ifdef SIMULATOR
	{
		const char kFileROMPrefix[] = "file://ROM/";
		const kFileROMPrefixSize = (sizeof(kFileROMPrefix) - 1);
		
		if (EqualStringN(url, kFileROMPrefix,kFileROMPrefixSize ) && !gSimulator->GetUseROMStore())
				return false;
	}
#endif

	if ((fNode = Resolve(url + kFilePrefixSize + kRelativeOffset, false)) == nil)
		return false;
	
	TrivialMessage(("Creating StoreStream for %s", resource->GetURL()));
	fResource = *resource;
	return true;
}

StoreStream* StoreStream::NewStream(const Resource* resource)
{
	Assert(resource != nil);
		
	StoreStream* stream = new(StoreStream);

	if (!stream->Initialize(resource)) {
		delete(stream);
		return nil;
	}
	
	return stream;
}

void StoreStream::SetDataType(DataType value)
{
	fResource.SetDataType(value);
}

void StoreStream::SetStatus(Error)
{
	// Do nothing, ROM status cannot change.
}

void StoreStream::Write(const void* UNUSED(data), long UNUSED(length))
{
	Trespass();
}

const char* StoreStream::GetDataAsString()
{
	long 	dataLength 	= GetDataLength();
	char* 	data      	= GetData();
	
	Assert(data[dataLength] == '\0');
	return data;
}

// =============================================================================

