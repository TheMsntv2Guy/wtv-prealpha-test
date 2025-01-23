// ===========================================================================
//	FileStream.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef SIMULATOR

#include <stdio.h>

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __FILESTREAM_H__
#include "FileStream.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#ifdef FOR_MAC
	#ifndef __LOCALNET_H__
	#include "LocalNet.h"
	#endif
	#include "MacintoshUtilities.h"
#endif



// =============================================================================

FileStream::~FileStream()
{
}

#ifdef FOR_MAC
static void ConvertToHFS (char* path)
{
    char* pch = path;
    
    if (*pch == '/')
    	memmove(pch, pch + 1, strlen(pch));
    else
    {
    	// support relative pathnames on Macintosh
    	// by prepending a colon.
    	memmove(pch + 1, pch, strlen(pch)+1);
    	*pch = ':';
    }
	
    while (*++pch != 0)
    	if (*pch == '/')
			*pch = ':';
		else if (*pch == '.' && pch[1] == '.')
		{
			*pch++ = ':';
			Assert(strlen(pch) > 2);
    		memmove(pch, pch + 2, strlen(pch));
		}
}
#endif /* FOR_MAC */

Boolean FileStream::Initialize(const Resource* resource)
{
	Assert(resource != nil);
	const char* url = resource->GetURL();

#ifdef FOR_MAC
	if ((gLocalNet != nil) && !EqualStringN("file:", url, 5))
	{
		if (gLocalNet->GetActiveRead())
		{
			if (gLocalNet->GetURLInCache(url))
			{
				long localNetSize = gLocalNet->GetURLSize(url);
				if (localNetSize > 0)
				{
					Ptr localNetBuffer = NewSimulatorMemory(localNetSize);
					if (localNetBuffer != nil)
					{
						long localNetSizeGotten = gLocalNet->GetURLData(url, localNetBuffer, localNetSize);
						if (localNetSizeGotten == localNetSize) {
							CacheStream::Initialize(gRAMCache, resource->GetCacheEntry());
							resource->GetCacheEntry()->SetStatus(kNoError);
							Write(localNetBuffer, localNetSizeGotten);
							if ( GetDataLength() == localNetSizeGotten )
							resource->GetCacheEntry()->SetStatus(kComplete);
							else {
								Message(("Could not load local net file '%s' %d bytes into cache",url,localNetSizeGotten));
						}
						}
						DisposeSimulatorMemory(localNetBuffer);
						return true;
					}
					else {
						Complain(("Simulator failed allocating local net buffer - make the app bigger"));
					}
				}
			}
		}
		
		if (gLocalNet->GetExclusiveRead())
		{
			if (EqualStringN("http:", url, 5))
				Complain(("Trying to find \"%s\" in LocalNet, but it does not exist.", resource->GetCacheEntry()->GetName()));
			return false;
		}
	}
#endif

	// Make a modifiable copy of the url.
	const long pathSize = strlen(url) + 32;
	char* path = (char*)AllocateBuffer(pathSize);
	strncpy(path, url, pathSize);
	
	// Check if path is for a file.
	if (!EqualStringN(path, "file://", 7)){
		FreeBuffer(path, pathSize);
		return false;
	}
	
	// Copy and convert file path.
	ulong	bufferSize = strlen(path) + 1;
	if (bufferSize < 256) bufferSize = 256;
	char* buffer = (char*) AllocateBuffer(bufferSize);
	strncpy(buffer, path + strlen("file://"), bufferSize);

#ifdef FOR_MAC
	ConvertToHFS(buffer);
#endif

	switch (GuessDataType(buffer))
	{
	case kDataTypeHTML:
		{
			FILE* fp;
		
			if ((fp = fopen(buffer, "r")) == nil) {
				Message(("Cannot open file \"%s\"", buffer));
				FreeBuffer(buffer, bufferSize);
				FreeBuffer(path, pathSize);
				return false;
			}
		
			CacheStream::Initialize(gRAMCache, resource->GetCacheEntry());
			SetStatus(kNoError);
	
			for (;;) {
				if (fgets(buffer, bufferSize, fp) == nil)
					break;
				Write(buffer, strlen(buffer));
			}
			
			fclose(fp);
		}
		break;
	
	default:
		{
			short refnum;
			OSErr error;
			
			c2pstr(buffer);	
			
			if ((error = FSOpen((StringPtr)buffer, 0, &refnum)) != noErr) {
				p2cstr((StringPtr)buffer);
				Message(("Cannot open file \"%s\"", buffer));
				FreeBuffer(buffer, bufferSize);
				FreeBuffer(path, pathSize);
				return false;
			}
				
			CacheStream::Initialize(gRAMCache, resource->GetCacheEntry());
			SetStatus(kNoError);
			
			for (;;) {
				long	count = bufferSize;
				FSRead(refnum, &count, buffer);
				if (count == 0)
					break;
				Write(buffer, count);
			}
			
			FSClose(refnum);
		}
	}

	FreeBuffer(buffer, bufferSize);
	FreeBuffer(path, pathSize);
	return true;
}
	
FileStream* FileStream::NewStream(const Resource* resource)
{
	Assert(resource != nil);

	FileStream* stream = new(FileStream);
	
	if (!IsError(stream == nil)) {
		if (!stream->Initialize(resource)) {
			delete(stream);
			return nil;
		}
	}
	return stream;
}

#endif /* SIMULATOR */

// =============================================================================
