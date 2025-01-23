// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#include "Headers.h"
#include <FCntl.h>
#include <ErrNo.h>

/* because MWC has badd ErrNo.h */
#ifndef EFAULT
#define EFAULT		14 /* Bad address */
#define EMFILE		24 /* Too many open files */
#endif

Public int	open(const char *fileName, int openFlags);
Public void	seek(int fileNumber, ulong position);
Public int	remove(const char *oldName);
Public int	rename(const char *oldName, const char *newName);
Public int	read(int fileNumber, char *buffer, ulong count);
Public int	close(int fileNumber);

typedef struct
	{
	char		*data;
	ulong		length;
	ulong		position;
	ulong		lastModified;
	Boolean		inUse: 1;
	unsigned	reserved: 31;
	} OpenFileTableEntry;

#define kMaxFileNumber	32
typedef struct
	{
	OpenFileTableEntry	entries[kMaxFileNumber];
	} OpenFileTable;

Private OpenFileTable	gOpenFileTable;
#ifdef SIMULATOR
#define kFileTableOffset	4096
#else
#define kFileTableOffset	0
#endif


#ifdef DEBUG
extern Boolean			gObjectStoreInitialized;
#endif

#ifdef SIMULATOR
Public int		old_open(const char *fileName, int openFlags);
#endif

#ifdef FOR_MAC
Private void
ConvertFromHFS(char *pathName)
	{
    char	*pch = pathName;
    
	if (*pch == ':')
    	memmove(pch, pch + 1, strlen(pch));
	else
    	{
		memmove(pch + 1, pch, strlen(pch));
    	*pch = '/';
		}
	
    while (*++pch != 0)
    	if (*pch == ':')
			*pch = '/';
	}
#endif

Public int
my_open(const char *fileName, int openFlags)
	{
	ObjectNode			*node;
	OpenFileTableEntry	*entry, *entryMax;

#ifdef SIMULATOR
#ifdef FOR_MAC
	if (strncmp(fileName, "RAM:", 4) != 0 && strncmp(fileName, "ROM:", 4) != 0)
#else
	if (strncmp(fileName, "/RAM", 4) != 0 && strncmp(fileName, "/ROM", 4) != 0)
#endif
		return old_open(fileName, openFlags);
#endif

	Assert(gObjectStoreInitialized);
	Assert(openFlags == O_RDONLY);
	
#ifdef FOR_MAC
	{
	char				convertedFileName[256];
	
	strncpy(convertedFileName, fileName, sizeof(convertedFileName) - 1);
	ConvertFromHFS(convertedFileName);
	if ((node = Resolve(convertedFileName, false)) == nil)
		{ errno = EFAULT; return -1; }
	}
#else
	if ((node = Resolve(fileName, false)) == nil)
		{ errno = EFAULT; return -1; }
#endif
		
	entry = gOpenFileTable.entries;
	entryMax = entry + kMaxFileNumber;
	while (entry < entryMax)
		{
		if (!entry->inUse)
			{
			entry->data = node->data;
			entry->length = node->dataLength;
			entry->position = 0;
			entry->lastModified = Now();
			entry->inUse = true;
			return (entry - gOpenFileTable.entries) + kFileTableOffset;
			}
		}
		
	errno = EMFILE;
	return -1;
	}

#ifdef SIMULATOR
Public int		old_lseek(int fileNumber, long position, int whence);
#endif

Public int
my_lseek(int fileNumber, long position, int whence)
	{
	OpenFileTableEntry		*entry;
	
#ifdef SIMULATOR
	if (fileNumber < kFileTableOffset)
		return old_lseek(fileNumber, position, whence);
	fileNumber -= kFileTableOffset;
#endif
	
	Assert(gObjectStoreInitialized);
	Assert(fileNumber < kMaxFileNumber);
	Assert(gOpenFileTable.entries[fileNumber].inUse);
	
	entry = &gOpenFileTable.entries[fileNumber];
	switch (whence)
		{
		case SEEK_SET:
			if (position > entry->length)
				{ errno = EFAULT; return -1; }
			entry->position = position;
			break;
		case SEEK_CUR:
			if (entry->position + position > entry->length)
				{errno = EFAULT; return -1; }
			entry->position += position;
			break;
		case SEEK_END:
		default:
			Trespass();
			break;
		}
	}
	
Public int
remove(const char *oldName)
	{
	Assert(gObjectStoreInitialized);
	
	/* Note: have to deal with open file */
	Remove(oldName);
	return 0;
	}

Public int
rename(const char *oldName, const char *newName)
	{
	Assert(gObjectStoreInitialized);
	
	SetName(oldName, newName);
	return 0;
	}

#ifdef SIMULATOR
Public int		old_read(int fileNumber, char *buffer, ulong count);
#endif

Public int
my_read(int fileNumber, char *buffer, ulong count)
	{
	OpenFileTableEntry	*entry;

#ifdef SIMULATOR
	if (fileNumber < kFileTableOffset)
		return old_read(fileNumber, buffer, count);
	fileNumber -= kFileTableOffset;
#endif

	entry = &gOpenFileTable.entries[fileNumber];

	Assert(gObjectStoreInitialized);
	Assert(fileNumber < kMaxFileNumber);
	Assert(gOpenFileTable.entries[fileNumber].inUse);

	CopyMemory(entry->data + entry->position, buffer, count);
	if (entry->position + count > entry->length)
		{ errno = EFAULT; return -1; }
	entry->position += count;
	
	return count;
	}

#ifdef SIMULATOR
Public int		old_close(int fileNumber);
#endif

Public int
my_close(int fileNumber)
	{
#ifdef SIMULATOR
	if (fileNumber < kFileTableOffset)
		return old_close(fileNumber);
	fileNumber -= kFileTableOffset;
#endif

	Assert(gObjectStoreInitialized);
	Assert(fileNumber < kMaxFileNumber);
	Assert(gOpenFileTable.entries[fileNumber].inUse);
	
	gOpenFileTable.entries[fileNumber].inUse = false;
	return 0;
	}