// ===========================================================================
//	CacheEntry.c
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

#ifdef HARDWARE
	#ifndef __BOXABSOLUTEGLOBALS_H__
	#include "BoxAbsoluteGlobals.h"
	#endif
#endif

#ifdef SIMULATOR
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
	#ifdef FOR_MAC
		#ifndef __LOCALNET_H__
		#include "LocalNet.h"
		#endif
		#ifndef __MACINTOSHUTILITIES_H__
		#include "MacintoshUtilities.h"
		#endif
	#endif
#endif


// ===========================================================================

inline long
ExtendForAlignment(long value)
{
	const long kAlignment = 4;
	const long kAlignmentMask = kAlignment-1;
	
	if ((value & kAlignmentMask) != 0)
		return (value & ~kAlignmentMask) + kAlignment;
	
	return value;
}

// ===========================================================================
// Class CacheEntry

void 
CacheEntry::BeginUse()
{
	fUserCount++;
	SetDebugModifiedTime();
}

void 
CacheEntry::BeginUseData()
{
	BeginUse();
	
	if (fDataUserCount == kDataNeverUsed)
		fDataUserCount = 0;

	fDataUserCount++;
}

void 
CacheEntry::EndUse()
{
	if (IsError(fUserCount == 0))
		return;
		
	fUserCount--;
	fLastUsed = Now();
	
	// Expire bad cache entries.
	if (TrueError(fStatus))
		fExpires = 1;
	
	SetDebugModifiedTime();
}

void 
CacheEntry::EndUseData()
{
	if (Error(fDataUserCount <= 0))
		return;

	fDataUserCount--;
	EndUse();
	
	// Prevent premature network reload.
	if (fDataUserCount == 0 && HasExpired())
		fPriority = (Priority)0;
}

const char*
CacheEntry::GetAuthorization() const
{
	if (fAuthorizationLength == 0)
		return nil;
	
	return fBase + GetAuthorizationOffset();
}

long
CacheEntry::GetAuthorizationCapacity() const
{
	if (fAuthorizationLength == 0)
		return 0;
	
	return ExtendForAlignment(fAuthorizationLength);
}

long
CacheEntry::GetAuthorizationOffset() const
{
	return GetNameOffset() + GetNameCapacity();
}

char*
CacheEntry::GetData() const
{
	if (GetDataCapacity() == 0)
		return nil;
	
	return fBase + GetDataOffset();
}

long
CacheEntry::GetDataCapacity() const
{
	// Returns total number of bytes available for data.
	
	if (fLength == 0)
		return 0;
	
	return fLength - GetDataOffset();
}

long
CacheEntry::GetDataOffset() const
{
	// Return offset of data from fBase
	
	return GetPostDataOffset() + GetPostDataCapacity();
}

ulong 
CacheEntry::GetLastUsed() const
{
	if (HasUsers())
		return Now();
	
	return fLastUsed;
}

const char*
CacheEntry::GetName() const
{
	if (fNameLength == 0)
		return nil;
	
	return fBase + GetNameOffset();
}

long
CacheEntry::GetNameCapacity() const
{
	if (fNameLength == 0)
		return 0;
	
	return ExtendForAlignment(fNameLength);
}

long
CacheEntry::GetNameOffset() const
{
	return 0;
}

uchar 
CacheEntry::GetPercentComplete(ulong expectedSize) const
{
	uchar total;
	
	if (GetStatus() != kPending && GetStatus() != kNoError)
		return 100;
	
	ulong	length = GetLength();
	if (length == 0)
		length = expectedSize;
	
	total = (uchar)(100 * GetDataLength() / length);
	if (total > 100)
		total = 100;
	return total;
}

char*
CacheEntry::GetPostData() const
{
	if (fPostDataLength == 0)
		return nil;
	
	return fBase + GetPostDataOffset();
}

long
CacheEntry::GetPostDataCapacity() const
{
	if (fPostDataLength == 0)
		return 0;
		
	return ExtendForAlignment(fPostDataLength);
}

long
CacheEntry::GetPostDataOffset() const
{
	return GetAuthorizationOffset() + GetAuthorizationCapacity();
}

Boolean 
CacheEntry::HasExpired() const
{
	ulong gmt;
	
	if (fExpires == 0)
		return false;
	
	if (fDataUserCount > 0)
		return false;
	
	if (fDataUserCount == kDataNeverUsed && fDataLength != 0)
		return false;
	
	if ((gmt = gClock->GetDateTimeGMT()) == 0)
		return false;
	
	if (fExpires > gmt)
		return false;
	
	return true;
}

Boolean 
CacheEntry::IsNamed(const char* target, const char* postData, long postDataLength) const
{
	const char* name;
	
	if (IsError(target == nil || *target == 0))
		return false;
	
	if (fPostDataLength != postDataLength)
		return false;
	
	if ((name = GetName()) == nil)
		return false;
	
	if (strcmp(name, target) != 0)
		return false;
	
	if (fPostDataLength != 0) {
		if (postData == nil)
			return false;
		if (memcmp(GetPostData(), postData, postDataLength) != 0)
			return false;
	}
	
	return true;
}

Boolean 
CacheEntry::IsPending() const
{
	if (IsFree())
		return false;
	
	if (fPriority == (Priority)0)
		return false;

	if (fDataUserCount > 0)
		return false;
		
	if (TrueError(fStatus))
		return false;
	
	if (fStatus >= kComplete)
		return false;

	return true;
}

Boolean 
CacheEntry::IsPurgable() const
{
	if (fPriority == kPersistent)
		return false;
	
	if (fDataUserCount > 0)
		return false;
	
	if (GetDataCapacity() == 0)
		return false;
	
	return true;
}

Boolean 
CacheEntry::IsRemovable() const
{
	if (fPriority == kPersistent)
		return false;
	
	if (fUserCount != 0)
		return false;
	
	if (IsFree())
		return false;

	return true;
}

#ifdef DEBUG_CACHE_VALIDATE
Boolean
CacheEntry::IsValid() const
{
	long maxLength = gRAMCache->GetLength();
	char* cacheData = gRAMCache->GetData();
	
	short maxUsers = 1000; // oh, just invented this number
	
	if (IsError((fAuthorizationLength < 0) || (fAuthorizationLength > maxLength)))
		return false;
	
	if (IsError((fBase < cacheData) || (fBase >= cacheData + maxLength - fLength)))
		return false;
	
	if (IsError((fDataExpected < -1) || (fDataExpected > maxLength)))
		return false;
	
	if (IsError((fDataLength < 0) || (fDataLength > maxLength)))
		return false;
	
	// fExpires?
	// fLastModified?
	// fLastUsed?
	
	if (IsError((fLength < 0) || (fLength > maxLength)))
		return false;
	
	if (IsError((fNameLength < 0) || (fNameLength > maxLength)))
		return false;
	
	if (IsError((fPostDataLength < 0) || (fPostDataLength > maxLength)))
		return false;
	
	if (IsError((fDataUserCount < -1) || (fDataUserCount > maxUsers)))
		return false;
	
	if (IsError((fUserCount < 0) || (fUserCount > maxUsers)))
		return false;
	// fDataType?

	if (IsError(fPriority > kPersistent))
		return false;
	
	// fStatus?
	return true;
}
#endif /* DEBUG_CACHE_VALIDATE */

void 
CacheEntry::Move(long offset)
{	
	if (fLength != 0)
		CopyMemory(fBase, fBase + offset, fLength);
	fBase += offset;
}

void 
CacheEntry::Reset()
{
	fAuthorizationLength = 0;
	// fBase is managed by the cache
	fDataExpected = kDataExpectedUnknown;
	fDataLength = 0;
	fDataType = (DataType)0;
	fDataUserCount = kDataNeverUsed;
	fExpires = 0;
	fIsConstant = false;
	fIsLocal = false;
	fIsDataTrusted = false;
	fLastModified = 0;
	fLastUsed = 0;
	fLength = 0;
	fNameLength = 0;
	fPostDataLength = 0;
	fPriority = (Priority)0;	// prevents network from doing its thing
	fStatus = kNoError;
	fUserCount = 0;	

	SetDebugModifiedTime();
}

void 
CacheEntry::SetAuthorization(const char* authorization, Cache* cache)
{
	long bytesRequired;
	long authorizationLength;
	
	if (IsWarning(authorization == nil || *authorization == 0))
		return;
	
	if (fIsConstant)
		return;
	
	// Resize the cache entry to accomodate the authorization.
	authorizationLength = strlen(authorization) + 1;
	bytesRequired = ExtendForAlignment(authorizationLength);
	bytesRequired -= GetAuthorizationCapacity();

	if (bytesRequired > 0)
		if (!cache->Resize(this, fLength + bytesRequired))
			return;

	char* d = fBase + GetPostDataOffset();
	CopyMemory(d, d + bytesRequired, GetPostDataCapacity() + GetDataCapacity() - bytesRequired);
	strcpy(fBase + GetAuthorizationOffset(), authorization);
	fAuthorizationLength = authorizationLength;
	fLastUsed = Now();
	SetDebugModifiedTime();
}

void 
CacheEntry::SetBase(char* value)
{
	if (value != fBase)
		SetDebugModifiedTime();

	fBase = value;
}

void 
CacheEntry::SetDataExpected(long value)
{
	if (fIsConstant || value == fDataExpected)
		return;
		
	fDataExpected = value;
	SetDebugModifiedTime();
	
	// Reset fields if we are going back to an unknown state.
	if (fDataExpected == kDataExpectedUnknown) {
		if (fDataUserCount == 0)
			fDataUserCount = kDataNeverUsed;
		fExpires = 0;
	}
}

void 
CacheEntry::SetDataLength(long value)
{
	if (IsError(value < 0))
		value = 0;
	
	if (IsError(value > GetDataCapacity()))
		value = GetDataCapacity();
		
	if (fIsConstant)
		return;
	
	if (value != fDataLength)
		SetDebugModifiedTime();
	
	fDataLength = value;
}

void 
CacheEntry::SetDataType(DataType value)
{
	if (fIsConstant)
		return;
	
	if (value != fDataType)
		SetDebugModifiedTime();

	fDataType = value;
}

void 
CacheEntry::SetExpires(ulong expires)
{
	if (fIsConstant)
		return;
	
	if (expires != fExpires)
		SetDebugModifiedTime();
	
	fExpires = expires;
}

void
CacheEntry::SetIsConstant(Boolean value)
{
	if (value != fIsConstant)
		SetDebugModifiedTime();	
	
	fIsConstant = value;
}

void
CacheEntry::SetIsDataTrusted(Boolean value)
{
	if (value == fIsDataTrusted)
		return;
	
	fIsDataTrusted = value;
	SetDebugModifiedTime();
}

void
CacheEntry::SetIsLocal(Boolean value)
{
	if (value == fIsLocal)
		return;
	
	fIsLocal = value;
	SetDebugModifiedTime();
}

void
CacheEntry::SetIsLocked(Boolean value)
{
	fIsLocked = value;
}

void 
CacheEntry::SetLastModified(ulong value)
{
	if (fIsConstant)
		return;

	if (value != fLastModified)
		SetDebugModifiedTime();	

	fLastModified = value;
}

void 
CacheEntry::SetLength(long value)
{
	if (fIsConstant)
		return;
		
	long newDataLength = MIN(value - GetDataOffset(), fDataLength);
	
	if (IsError(newDataLength < 0))
		newDataLength = 0;

	if ((fLength != value) || (fDataLength != newDataLength))
		SetDebugModifiedTime();	
	
	fDataLength = newDataLength;
	fLength = value;
}

void 
CacheEntry::SetName(const char* value, Cache* cache)
{
	long bytesRequired;
	long nameLength;
	
	if (IsError(value == nil || *value == 0))
		return;
	
	if (fIsConstant)
		return;
	
	// Resize the cache entry to accomodate the name.
	nameLength = strlen(value) + 1;
	bytesRequired = ExtendForAlignment(nameLength);
	bytesRequired -= GetNameCapacity();

	if (bytesRequired > 0) {
		Priority oldPriority = fPriority;
		fPriority = kImmediate;				// High priority for name.
		Boolean suceeded = cache->Resize(this, fLength + bytesRequired);
		fPriority = oldPriority;

		if (!suceeded) {
			SetStatus(kCacheFull);
			return;
		}
	}

	char* d = fBase + GetAuthorizationOffset();
	CopyMemory(d, d + bytesRequired, GetAuthorizationCapacity() + GetPostDataCapacity() + GetDataCapacity() - bytesRequired);
	strcpy(fBase + GetNameOffset(), value);
	fNameLength = nameLength;

	fLastUsed = Now();

	SetDebugModifiedTime();
}

void 
CacheEntry::SetPostData(const char* data, long length, Cache* cache)
{
	long bytesRequired;
	
	if (data == nil || length == 0 || fIsConstant)
		return;
	
	// Resize the cache entry to accomodate the post data.
	bytesRequired = ExtendForAlignment(length);
	bytesRequired -= GetPostDataCapacity();

	if (bytesRequired > 0) {
		Priority oldPriority = fPriority;
		fPriority = kImmediate;				// High priority for post data.
		Boolean suceeded = cache->Resize(this, fLength + bytesRequired);
		fPriority = oldPriority;

		if (!suceeded) {
			SetStatus(kCacheFull);
			return;
		}
	}

	char* d = fBase + GetDataOffset();
	CopyMemory(d, d + bytesRequired, GetDataCapacity() - bytesRequired);
	CopyMemory(data, fBase + GetPostDataOffset(), length);

	fPostDataLength = length;
	fLastUsed = Now();

	SetDebugModifiedTime();
}

void 
CacheEntry::SetPriority(Priority value)
{
	if (fIsConstant || fPriority == value)
		return;
	
	// This just changes too darn often...the cache window flickers like mad
	//	SetDebugModifiedTime();

	fPriority = value;
}

void 
CacheEntry::SetStatus(Error value)
{
	if (fIsConstant || fStatus == value)
		return;
		
	fStatus = value;

	SetDebugModifiedTime();
	
	if (fStatus == kStreamReset)
		fDataLength = 0;
	
#ifdef DEBUG_CACHEWINDOW
	if (fStatus == kStreamReset)
		gRAMCache->SetResetCount(gRAMCache->GetResetCount() + 1);
#endif

}

//===========================================================================

#ifdef DEBUG_BOXPRINT
// -------------------------------------------------
#include "BoxPrintDebug.h"

void
CacheEntry::StaticBoxPrintDebug(const CacheEntry* cacheEntry, long whatToPrint)
{
	if (cacheEntry == nil) {
		BoxPrint("CacheEntry: <nil>");
	} else if (whatToPrint & kBoxPrintOneLiner) {
		cacheEntry->BoxPrintDebug(whatToPrint);
	} else {
		BoxPrint("CacheEntry: <%#6x>", cacheEntry);
		AdjustBoxIndent(1);
		cacheEntry->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}

void
CacheEntry::BoxPrintDebug(long whatToPrint) const
{
#ifdef DEBUG_CACHE_VALIDATE
	if (!IsValid()) {
		BoxPrint("<INVALID CACHE ENTRY!!!>");
		return;
	}
#endif /* DEBUG_CACHE_VALIDATE */
	
	if (whatToPrint == 0) {
		whatToPrint = kBoxPrintName | kBoxPrintLength | kBoxPrintStatus | kBoxPrintFlags;
	}
	if (whatToPrint & kBoxPrintName) {
		const char* name = GetName();
		if (name == nil) {
			name = "<no name>";
		}
		BoxPrint("Name: %s", name);
	}
	if (whatToPrint & kBoxPrintLength) {
		BoxPrint("Length:  %d of %d expected",
				 fDataLength, fDataExpected);
	}
	if (whatToPrint & kBoxPrintStatus) {
		BoxPrint("Type: '%c%c%c%c',  Priority: %d,  Status: %d",
				 (fDataType >> 24) & 0xff,
				 (fDataType >> 16) & 0xff,
				 (fDataType >> 8) & 0xff,
				 fDataType & 0xff,
				 fPriority,
				 (int)fStatus);
	}
	if (whatToPrint & kBoxPrintFlags) {
		BoxPrint("HasUsers/HasDataUsers: %s/%s,  HasData: %s,  HasExpired: %s",
				 HasUsers() ? "yes" : "no",
				 HasDataUsers() ? "yes" : "no",
				 HasData() ? "yes" : "no",
				 HasExpired() ? "yes" : "no");
		BoxPrint("IsFree: %s,  IsPending: %s,  IsPurgable: %s, IsRemovable: %s",
				 IsFree() ? "yes" : "no",
				 IsPending() ? "yes" : "no",
				 IsPurgable() ? "yes" : "no",
				 IsRemovable() ? "yes" : "no");
	}
	if (whatToPrint & kBoxPrintOneLiner) {
		DataType dataType = GetDataType();	
		if (!isprint((dataType >> 24) & 0x0ff)) {
			dataType = (DataType)(QuadChar('?','?','?','?'));
		}
		
		BoxPrint("<%#8x> Status=%d (%2d) %2d(%2d) %6d(%6d)  '%c%c%c%c'  %s",
				(long)this,
				(int)fStatus,
				GetPriority(), 
				GetUserCount(),
				GetDataUserCount(),
				GetLength(),
				GetDataLength(),
				(char)((dataType>>24) & 0x0ff),
				(char)((dataType>>16) & 0x0ff),
				(char)((dataType>>8) & 0x0ff),
				(char)(dataType & 0x0ff),
				GetName());
	}
	if (whatToPrint & kBoxPrintOneLinerHeader) {
		BoxPrint("Address   Error/Priority    Users  Length  Type   Name");
	}
}

#endif /* DEBUG_BOXPRINT */