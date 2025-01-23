// ===========================================================================
//	Resource.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __CACHESTREAM_H__
#include "CacheStream.h"
#endif
#ifndef __FILESTREAM_H__
#include "FileStream.h"
#endif
#ifndef __LAYER_H__
#include "Layer.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __SCREEN_H__
#include "Screen.h"
#endif
#ifndef __STORESTREAM_H__
#include "StoreStream.h"
#endif
#ifndef __URLPARSER_H__
#include "URLParser.h"
#endif

// ===========================================================================
// Global variables

Cache* gRAMCache; // John: Is this the right place for this?

// ===========================================================================
// Class Resource

Resource::Resource()
{
	// Enable stack allocation
	fCacheEntry = nil;
}

Resource::Resource(const Resource& other)
{
	// Enable stack allocation
	fCacheEntry = other.fCacheEntry;
	
	if (fCacheEntry != nil)
		fCacheEntry->BeginUse();
}

Resource::~Resource()
{
	if (fCacheEntry != nil)
		fCacheEntry->EndUse();
}

Resource& 
Resource::operator=(const Resource& other)
{
	if (this == &other)
		return *this;
	
	if (fCacheEntry != nil)
		fCacheEntry->EndUse();
	
	fCacheEntry = other.fCacheEntry;

	if (fCacheEntry != nil)
		fCacheEntry->BeginUse();
	
	return *this;
}

Boolean 
Resource::operator==(const Resource& other) const
{
	return fCacheEntry == other.fCacheEntry;
}

Boolean 
Resource::operator!=(const Resource& other) const
{
	return fCacheEntry != other.fCacheEntry;
}

const char* Resource::GetDataTypeString() const
// Note: Change ParseData() when this changes
{
	DataType	dataType = GetDataType();
	
	switch (dataType) {
		case kDataTypeAnimation:
			return "x-wtv-animation";
		case kDataTypeBitmap:
		case kDataTypeXBitMap:
			return "image/wtv-bitmap";
		case kDataTypeFidoImage:
			return "image/fido";
		case kDataTypeGIF:
			return "image/gif";
		case kDataTypeHTML:
			return "text/html";
		case kDataTypeJPEG:
			return "image/jpeg";
		case kDataTypeMIDI:
			return "audio/x-midi";
		case kDataTypeMIPSCode:
			return "code/x-wtv-code-mips";
		case kDataTypeMPEGAudio:
			return "audio/x-mpeg";
		case kDataTypePPCCode:
			return "code/x-wtv-code-ppc";
		case kDataTypeRealAudioMetafile:
			return "audio/x-pn-realaudio";
		case kDataTypeTEXT:
			return "text/plain";
		case kDataTypeTellyScript:
			return "text/tellyscript";

		case kDataTypeURL:
		case kDataTypeRealAudioProtocol:
		case kDataTypeImage:
		case kDataTypeUnprintable:
		case kDataTypeBorder:
		default:
			break;
	}

	Trespass();
	return "text/html";
}

Error 
Resource::GetStatus() const
{
	if (fCacheEntry == nil)
		return kNoError;
	
	Error status = fCacheEntry->GetStatus();
	
	if (status == kPending && fTimeoutAt != 0 && Now() >= fTimeoutAt) {
		PostulateFinal(false);	// should we let it complete, anyway?
		Resource* This = (Resource*)this;
		This->fTimeoutAt = 0;
		This->fCacheEntry->SetStatus(status = kTimedOut);
	}
	
	return status;
}

Boolean
Resource::IsLocal() const
{
	return fCacheEntry == nil ? false : fCacheEntry->IsLocal();
}

char*
Resource::LockData()
{
	if (fCacheEntry == nil)
		return nil;
	
	return gRAMCache->LockData(fCacheEntry);
}

DataStream* 
Resource::NewStream() const
{
	DataStream* stream;
	
	if (fCacheEntry == nil)
		return nil;
	
	// Get data from the cache.
	if (fCacheEntry->HasData())
		return CacheStream::NewStream(fCacheEntry);
	
	// Check for resources in RAM, ROM, or the file system.
	if (fCacheEntry->GetStatus() == kComplete) {
		if ((stream = StoreStream::NewStream(this)) != nil)
			return stream;
#ifdef SIMULATOR
		if ((stream = FileStream::NewStream(this)) != nil)
			return stream;
#endif
	}
	
	return nil;
}

CacheStream* 
Resource::NewStreamForAppend() const
{
	if (fCacheEntry == nil)
		return nil;
	
	if (fCacheEntry->HasDataUsers())
		return nil;
	
	return CacheStream::NewStream(fCacheEntry);
}

CacheStream* 
Resource::NewStreamForWriting() const
{
	if (fCacheEntry == nil)
		return nil;
	
	if (fCacheEntry->HasDataUsers())
		return nil;
	
	fCacheEntry->SetDataLength(0);
	fCacheEntry->SetStatus(kNoError);
	return CacheStream::NewStream(fCacheEntry);
}

void 
Resource::Purge()
{
	if (fCacheEntry != nil)
		gRAMCache->Purge(fCacheEntry);
}

void 
Resource::Reset()
{
	if (fCacheEntry != nil)
		fCacheEntry->EndUse();
	fCacheEntry = nil;
}

void
Resource::SetAuthorization(const char* value)
{
	if (fCacheEntry != nil)
		fCacheEntry->SetAuthorization(value, gRAMCache);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
Resource::SetTimeoutAt(ulong value)
{
	fTimeoutAt = value;
}
#endif

void
Resource::SetURL(CacheEntry* entry)
{
	if (IsError(entry == nil))
		return;
	
	Reset();
	fCacheEntry = entry;

	if (fCacheEntry != nil)
		fCacheEntry->BeginUse();
}

void
Resource::SetURL(const char* url)
{
	SetURL(url, nil, 0);
}

static const char*
FindLastCharacter(const char* s, char ch)
{
	const char*	pch = s + strlen(s);
	
	while (pch > s)
		if (*--pch == ch)
			return pch;
			
	return nil;
}

#define AGGRESSIVE_MATCHING	1

void 
Resource::SetURL(const char* url, const char* postData, long postDataLength)
{
	CacheEntry* entry;
	static const char cachePrefix[] = "file://ROM/Cache/";
	char*		cacheName = nil;
#ifdef AGGRESSIVE_MATCHING	
	ulong		cacheNameLength;

	// don't do if already in ROM
	if (EqualStringN(url, "wtv-", 4) || EqualStringN(url, "http://arcadia/", 15)) {	
		const char*	leafName = FindLastCharacter(url, '/');
		if (leafName++ != nil) {
			cacheNameLength = sizeof(cachePrefix) + strlen(leafName);
			cacheName = (char*)AllocateBuffer(cacheNameLength);
			
			snprintf(cacheName,cacheNameLength, "%s%s", cachePrefix, leafName);
			if (Resolve(cacheName + 6, false) != nil)
				url = cacheName;
			else {
				FreeBuffer(cacheName, cacheNameLength);
				cacheName = nil;
			}
		}
	}
#endif /* AGGRESSIVE_MATCHING */	

	PushDebugChildURL(url);
	
#ifdef DEBUG_CACHE_SCRAMBLE
	if (gSystem->GetCacheScramble()) {
		gRAMCache->Scramble();
	}
#endif /* DEBUG_CACHE_SCRAMBLE */
	
	if ((entry = gRAMCache->Find(url, postData, postDataLength)) == nil)
		entry = gRAMCache->Add(url, postData, postDataLength);

	if (IsError(entry == nil)) {
		PopDebugChildURL();
		goto Done;
	}	
	SetURL(entry);
	PopDebugChildURL();
	
Done:
	if (cacheName != nil)
		FreeBuffer(cacheName, cacheNameLength);

#ifdef DEBUG_CACHE_VALIDATE
	if (gSystem->GetCacheValidate()) {
		gRAMCache->IsValid();
	}
#endif /* DEBUG_CACHE_VALIDATE */
}

void 
Resource::SetURL(const char* partial, const Resource* parent)
{
	SetURL(partial, parent, nil, 0);
}

void 
Resource::SetURL(const char* partial, const Resource* parent, const char* postData, long postDataLength)
{
	const char* parentURL = nil;
	char* fullURL;
	
	if (IsError(parent == nil) || (parentURL = parent->GetURL()) == nil) {
		SetURL(partial);
		return;
	}
	
	URLParser urlParser;
	urlParser.SetURL(parentURL);
	fullURL = urlParser.NewURL(partial, "URL");
	SetURL(fullURL, postData, postDataLength);
	FreeTaggedMemory(fullURL, "URL");
}

void
Resource::UnlockData()
{
	if (fCacheEntry != nil)
		gRAMCache->UnlockData(fCacheEntry);
}

// =============================================================================

#ifdef DEBUG_BOXPRINT
// -------------------------------------------------
#include "BoxPrintDebug.h"

void
Resource::StaticBoxPrintDebug(const Resource* resource, long whatToPrint)
{
	if (resource == nil) {
		BoxPrint("Resource: <nil>");
	} else {
		BoxPrint("Resource: <%#06x>", resource);
		AdjustBoxIndent(1);
		resource->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}

void
Resource::BoxPrintDebug(long whatToPrint) const
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
	if (whatToPrint & kBoxPrintTimeoutAt) {
		ulong now = Now();
		BoxPrint("TimeoutAt: %d (Now() = %d, %d ticks away)",
				fTimeoutAt, now, now - fTimeoutAt);
	}
}

#endif /* DEBUG_BOXPRINT */