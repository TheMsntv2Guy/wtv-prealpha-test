// ===========================================================================
//	CacheStream.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include "Cache.h"
#include "CacheStream.h"
#include "Clock.h"
#include "FileStream.h"
#include "HTTP.h"
#include "MemoryManager.h"
#include "Network.h"
#include "StoreStream.h"

#ifdef DEBUG_BOXPRINT
#include "BoxPrintDebug.h"
#endif

#ifdef HARDWARE
	#include "BoxAbsoluteGlobals.h"
#endif

#ifdef SIMULATOR
	#include "Simulator.h"
	#ifdef FOR_MAC
		#include "LocalNet.h"
		#include "MacintoshUtilities.h"
	#endif
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif

// ===========================================================================
// local prototypes

static char* ParseBoundary(char* value);

// ===========================================================================
// Class CacheStream

CacheStream::CacheStream()
{
}

CacheStream::~CacheStream()
{
	if (fCacheEntry != nil)
		fCacheEntry->EndUseData();
}

#ifdef DEBUG_BOXPRINT
void
CacheStream::BoxPrintDebug(long whatToPrint) const
{
	if (whatToPrint == 0) {
		whatToPrint = kBoxPrintCacheEntry;
	}

	if (whatToPrint & kBoxPrintCacheEntry) {
		if (fCacheEntry == nil) {
			BoxPrint("CacheEntry: <none>");
		} else {
			fCacheEntry->BoxPrintDebug(0);
		}
	}
}
#endif

char*
CacheStream::GetData() const
{
	if (fCacheEntry == nil)
		return nil;
	return fCacheEntry->GetData();
}

const char*
CacheStream::GetDataAsString()
{
	long dataLength;
	char* data;
	
	if (fCacheEntry == nil)
		return nil;
	
	if (IsWarning(fCacheEntry->GetStatus() != kComplete))
		return nil;
	
	dataLength = fCacheEntry->GetDataLength();
	data = fCacheEntry->GetData();
	
	if (dataLength == 0 || data[dataLength-1] != '\0') {
		MakeAvailable(dataLength + 1);
		data[dataLength++] = '\0';
		fCacheEntry->SetDataLength(dataLength);
	}
	
	return data;
}

long 
CacheStream::GetDataExpected() const
{
	if (fCacheEntry == nil)
		return 0;
	return fCacheEntry->GetDataExpected();
}

long 
CacheStream::GetDataLength() const
{
	if (fCacheEntry == nil)
		return 0;
	return fCacheEntry->GetDataLength();
}

DataType 
CacheStream::GetDataType() const
{
	if (fCacheEntry == nil)
		return (DataType)0;
	return fCacheEntry->GetDataType();
}

ulong 
CacheStream::GetExpires() const
{
	if (fCacheEntry == nil)
		return 0;
	return fCacheEntry->GetExpires();
}

ulong 
CacheStream::GetLastModified() const
{
	if (fCacheEntry == nil)
		return 0;
	return fCacheEntry->GetLastModified();
}

long 
CacheStream::GetLengthIncrement() const
{
	return 1024;
}

long 
CacheStream::GetLengthInitial() const
{
	return 0;
}

const char* 
CacheStream::GetName() const
{
	if (fCacheEntry == nil)
		return nil;
	return fCacheEntry->GetName();
}

uchar 
CacheStream::GetPercentComplete(ulong expectedSize) const
{
	if (fCacheEntry == nil)
		return 0;
	return fCacheEntry->GetPercentComplete(expectedSize);
}

Priority 
CacheStream::GetPriority() const
{
	if (fCacheEntry == nil)
		return (Priority)0;
	return fCacheEntry->GetPriority();
}

Error 
CacheStream::GetStatus() const
{
	if (fCacheEntry == nil)
		return kNoError;
	return fCacheEntry->GetStatus();
}

Boolean
CacheStream::IsDataTrusted() const
{
	return fCacheEntry != nil && fCacheEntry->IsDataTrusted();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
CacheStream::Initialize(Cache* cache, const char* name)
{
	PostulateFinal(false); // this routine should be removed! -- JRM

	if (IsError(cache == nil || name == nil || *name == 0))
		return;
	
#ifdef DEBUG_CACHE_SCRAMBLE
	if (gSystem->GetCacheScramble()) {
		cache->Scramble();
	}
#endif /* DEBUG_CACHE_SCRAMBLE */
	
	fCache = cache;
	
	if ((fCacheEntry = fCache->Find(name)) == nil)
		fCacheEntry = fCache->Add(name, GetLengthInitial());

	fCacheEntry->BeginUseData();

#ifdef DEBUG_CACHE_VALIDATE
	if (gSystem->GetCacheValidate()) {
		cache->IsValid();
	}
#endif /* DEBUG_CACHE_VALIDATE */
}
#endif

void 
CacheStream::Initialize(Cache* cache, CacheEntry* entry)
{
	if (IsError(cache == nil || entry == nil))
		return;
		
	fCache = cache;
	fCacheEntry = entry;
	fCacheEntry->BeginUseData();
}

Boolean 
CacheStream::MakeAvailable(long newCapacity)
{
	long capacity;
	long increment;

	capacity = fCacheEntry->GetDataCapacity();	
	if (newCapacity <= capacity)
		return true;
		
	increment = GetLengthIncrement();
	if (newCapacity < capacity + increment)
		newCapacity = capacity + increment;
	
	return fCache->ResizeData(fCacheEntry, newCapacity);
}

CacheStream* 
CacheStream::NewStream(CacheEntry* entry)
{
	CacheStream* stream;
	
	if (IsError(entry == nil))
		return nil;
		
	TrivialMessage(("Creating CacheStream for %s", entry->GetName()));
	stream = new(CacheStream);
	if (stream != nil) {
		stream->Initialize(gRAMCache, entry);
	}
	return stream;
}

void
CacheStream::Rewind()
{
	DataStream::Rewind();
	fWriteState = kWriteNormally;
}

static char*
ParseBoundary(char* value)
{
	if ((value = SkipString(value, "multipart/mixed")) == nil)
		return nil;
	
	value = SkipCharacters(value, "; ");
	
	if ((value = SkipString(value, "boundary")) == nil)
		return nil;
	
	value = SkipCharacters(value, "= ");

	if (*value == '"')
		value[strlen(value)-1] = 0;
	
	return value;
}

Boolean
CacheStream::SetAttribute(const char* name, char* value)
{
	char* boundary;
	
	if (EqualString(name, "content-type"))
		if ((boundary = ParseBoundary(value)) != nil) {
			SetBoundary(boundary);
			return true;
		}
	
	return DataStream::SetAttribute(name, value);
}

void
CacheStream::SetBoundary(const char* boundary)
{
	if (IsWarning(boundary == nil))
		return;

	if (IsError(strlen(boundary) > kBoundaryCharacters-3))
		return;
	
	strcpy(fBoundary, "--");
	strcat(fBoundary, boundary);
	fWriteState = kWriteIgnoreUntilBoundary;
}

void 
CacheStream::SetDataType(DataType type)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetDataType(type);
}

void 
CacheStream::SetDataExpected(long value)
{
	if (fCache == nil || fCacheEntry == nil)
		return;

	fCacheEntry->SetDataExpected(value);

	if (value > 0)
		fCache->ResizeData(fCacheEntry, value);
}

void 
CacheStream::SetExpires(ulong expires)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetExpires(expires);
}

void
CacheStream::SetIsDataTrusted(Boolean value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetIsDataTrusted(value);
}

void 
CacheStream::SetLastModified(ulong value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetLastModified(value);
}

void 
CacheStream::SetName(const char* name)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetName(name, fCache);
}

void 
CacheStream::SetPriority(Priority value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetPriority(value);
}

void 
CacheStream::SetStatus(Error status)
{
	if (fCacheEntry == nil)
		return;

	fCacheEntry->SetStatus(status);
	
	if (status == kComplete) {
		fCache->ResizeData(fCacheEntry, GetDataLength());

#if defined(FOR_MAC)
		if ((gLocalNet != nil) && gLocalNet->GetActiveWrite())
			gLocalNet->SaveCacheEntry(fCacheEntry);
#endif
	} else if (TrueError(status) && status != kTruncated)
		fCache->ResizeData(fCacheEntry, 0);
}

#ifdef DEBUG_BOXPRINT
void
CacheStream::StaticBoxPrintDebug(const CacheStream* cacheStream, long whatToPrint)
{
	if (cacheStream == nil) {
		BoxPrint("CacheStream: (nil)");
		return;
	}
	
	BoxPrint("CacheStream: (%#6x)", cacheStream);
	AdjustBoxIndent(1);
	cacheStream->BoxPrintDebug(whatToPrint);
	AdjustBoxIndent(-1);
}
#endif

void 
CacheStream::Write(const void* data, long length)
{
	if (length == 0)
		return;

#ifdef DEBUG_CACHE_SCRAMBLE
	if (gSystem->GetCacheScramble()) {
		fCache->Scramble();
	}
#endif /* DEBUG_CACHE_SCRAMBLE */

	switch (fWriteState) {
	case kWriteNormally:				WriteNormally((const char*)data, length); break;
	case kWriteIgnoreUntilBoundary:		WriteIgnoreUntilBoundary((const char*)data, length); break;
	case kWriteIgnoreUntilBoundaryCRLF:	WriteIgnoreUntilBoundaryCRLF((const char*)data, length); break;
	case kWriteReadHeaders:				WriteReadHeaders((const char*)data, length); break;
	case kWriteUntilBoundary:			WriteUntilBoundary((const char*)data, length); break;
	case kWriteUntilBoundaryFinal:		WriteUntilBoundaryFinal((const char*)data, length); break;
	default:
		IsError("unknown write state");
		WriteNormally((const char*)data, length);
	}

#ifdef DEBUG_CACHE_VALIDATE
	if (gSystem->GetCacheValidate()) {
		fCache->IsValid();
	}
#endif /* DEBUG_CACHE_VALIDATE */
}

void
CacheStream::WriteIgnoreUntilBoundary(const char* data, long length)
{
	long boundaryCount = strlen(fBoundary);
	long i;
	
	for (i = 0; i < length; i++) {
		if (data[i] != fBoundary[fBoundaryCount]) {
			fBoundaryCount = 0;
			continue;
		}
		
		if (++fBoundaryCount == boundaryCount) {
			fBoundaryCount = 0;
			fWriteState = kWriteIgnoreUntilBoundaryCRLF;
			WriteIgnoreUntilBoundaryCRLF(&data[i+1], length-i-1);
			return;
		}
	}
}

void
CacheStream::WriteIgnoreUntilBoundaryCRLF(const char* data, long length)
{
	long i;
	
	for (i = 0; i < length; i++)
		switch (data[i]) {
		case 0x0D:
			break;
		
		case 0x0A:
			fWriteState = kWriteReadHeaders;
			WriteReadHeaders(&data[i+1], length-i-1);
			return;
		
		default:
			IsError(data[i] != 0x0D || data[i] != 0x0A);
			fWriteState = kWriteReadHeaders;
			WriteReadHeaders(&data[i+1], length-i-1);
			return;
		}
}

void
CacheStream::WriteNormally(const char* data, long length)
{
	Error status;
	
	if (fCacheEntry == nil)
		return;

	if (IsError(length < 0))
		return;

	if (length == 0)
		return;
	
	status = GetStatus();
	if (status == kStreamReset || TrueError(status))
		return;

	if (!MakeAvailable(fPosition + length)) {
		SetPriority((Priority)0);
		SetStatus(kTruncated);
		return;
	}
	
	CopyMemory(data, fCacheEntry->GetData() + fPosition, length);
	fCacheEntry->SetDataLength(fPosition += length);
}

void
CacheStream::WriteReadHeaders(const char* data, long length)
{
	long count = strlen(fHeader);
	long i;

	for (i = 0; i < length; i++) {
		switch (data[i]) {
		case 0x0a:	break;
		case 0x0d:	continue;
		default:
			if (IsError(count == sizeof (fHeader))) {
				SetStatus(kGenericError);
				return;
			}
			fHeader[count++] = data[i];
			continue;
		}
		
		// Advance the state if no header was specified.
		if (count == 0) {
			fWriteState = kWriteUntilBoundary;
			WriteUntilBoundary(&data[i+1], length-i-1);
			return;
		}
		
		// Process the header.
		SetAttributeString(fHeader);
		*fHeader = 0;
		count = 0;
	}
}

void
CacheStream::WriteUntilBoundary(const char* data, long length)
{
	const char* nonBoundaryData = data;
	long nonBoundaryCount = 0;
	long i;
	
	for (i = 0; i < length; i++) {
		// Check for non-matching character.
		if (data[i] != fBoundary[fBoundaryCount]) {
			// Write out partial matches.
			if (fBoundaryCount != 0) {
				WriteNormally(fBoundary, fBoundaryCount);
				fBoundaryCount = 0;
			}
			
			// Increment non-boundary count.
			if (nonBoundaryCount++ == 0)
				nonBoundaryData = &data[i];
			continue;
		}
		
		// Write out non-boundary count.
		if (nonBoundaryCount != 0) {
			WriteNormally(nonBoundaryData, nonBoundaryCount);
			nonBoundaryCount = 0;
		}
		
		// Check for match.
		if (fBoundary[++fBoundaryCount] == 0) {
			fBoundaryCount = 0;
			fWriteState = kWriteUntilBoundaryFinal;
			WriteUntilBoundaryFinal(&data[i+1], length-i-1);
			return;
		}
	}
	
	if (nonBoundaryCount != 0)
		WriteNormally(nonBoundaryData, nonBoundaryCount);
}

void
CacheStream::WriteUntilBoundaryFinal(const char* data, long length)
{
	long i;
	
	for (i = 0; i < length; i++) {
		// Check for non-matching character.
		if (data[i] != '-') {
			fBoundaryFinalCount = 0;
			fWriteState = kWriteIgnoreUntilBoundary;
			WriteIgnoreUntilBoundary(&data[i+1], length-i-1);
			return;
		}
		
		// Check for match.
		if (++fBoundaryFinalCount == 2) {
			SetStatus(kComplete);
			fBoundaryFinalCount = 0;
			fWriteState = kWriteIgnoreUntilBoundary;
			WriteIgnoreUntilBoundary(&data[i+1], length-i-1);
			return;
		}
	}
}
	
// ===========================================================================

