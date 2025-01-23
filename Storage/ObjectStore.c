/* ===========================================================================
    ObjectStore.c

    Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#include "Headers.h"

#ifndef __BOXABSOLUTEGLOBALS_H__
#include "BoxAbsoluteGlobals.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __OBJECTSTORE_H__
#include "ObjectStore.h"
#endif
#ifndef __OBJECTSTORE_PRIVATE_H__
#include "ObjectStore.Private.h"
#endif

#if defined(FOR_MAC)
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
#endif




const char kRAMFilesystemTag[] = "RAM Filesystem";

/* ===========================================================================
    implementations
=========================================================================== */

/* initialize the Filesystems:
 				/RAM	RAM filesystem, can override files in /ROM
				/ROM	ROM filesystem
 */

#ifdef INCLUDE_FINALIZE_CODE
void FinalizeFilesystems(void)
{
	Filesystem* fs = READ_AG(agFirstFilesystem);

#if defined(FOR_MAC)
	gMacSimulator->DisposeROMFileSystemRoot();
#elif !defined(SIMULATOR)
	// cleanup ROM filesystem...
#else
	#error "FinalizeFilesystems() is not defined for non-Mac, non-HARDWARE build"
#endif

	if (fs != nil)
	{	FreeTaggedMemory(fs, kRAMFilesystemTag);
		WRITE_AG(agFirstFilesystem, 0);
	}
}
#endif /* INCLUDE_FINALIZE_CODE */

void InitializeFilesystems(void)
{
Filesystem *fs;

	Message(("Initializing Filesystems..."));

	/* allocate zeroed memory for the RAM FS */
	
	fs = (Filesystem *)AllocateTaggedZero(sizeof(Filesystem), kRAMFilesystemTag);
	
	strncpy(fs->root.name, kMainRAMDirectoryName, kMaxFilenameLen);		/* now fill in the non-zero fields */
	WRITE_AG(agFirstFilesystem,fs);						/* set the head of the linked list to the RAM FS */

#if defined(FOR_MAC)
	fs->root.next = (FSNode *)gMacSimulator->GetROMFilesystemRoot();
#elif !defined(SIMULATOR)
	fs->root.next = (FSNode *)(0x9FFFC000 - sizeof(ROMFSHeader) - sizeof(FSNode));	/* ROM FS grows from top of ROM down */
#else
	#error "InitializeFilesystems() is not defined for non-Mac, non-HARDWARE build"
#endif

	Message(("RAM Filesystem is @ 0x%x",fs));
	Message(("ROM Filesystem is @ 0x%x",fs->root.next));
	
	FileHasChanged(&(fs->root));						/* and finally, checksum the node */
}

	
static FSNode *ResolveAtNode(FSNode *node, const char *name, Boolean acceptParent)
{
char	*nodeChar = node->name;
char	*nameChar = (char *)name;
FSNode	*nextNode;
	
	if (node == nil)
		return nil;
				
	if(	IsWarning((*node->name == '\0' || strrchr(node->name, '/') != nil)))
		return nil;
		
	for(;;)
	{
	char	chNode = tolower(*nodeChar);
	char	chName = tolower(*nameChar);
	
	if (chNode != chName)
		break;
		
	if (chName == '\0')
		return node;	/* matched whole string, so done */
	nodeChar++; nameChar++;
	}
		
	/* if we get here then there was a mismatch, or else
	 * we ran out of node space.
	 */
	 
	/* if partial match then search child directories */
	if (*nodeChar == '\0' && *nameChar == '/')
	{
		nextNode = ResolveAtNode(node->first, nameChar + 1, acceptParent);

		/* none of my children matched, but I'm close enough */
		if (nextNode == nil && acceptParent && strrchr(nameChar + 1, '/') == nil)
			return node;
			
		return nextNode;
	}
	
	/* otherwise, try the next of my siblings */
	return ResolveAtNode(node->next, name, acceptParent);
}
	
	
	
FSNode *Resolve(const char *name, Boolean acceptParent)
{
	if(IsWarning((*name == '\0' || name[strlen(name) - 1] == '/')))
		return nil;
	
	if (name[0] != '/')
		return nil;
	
	return ResolveAtNode(&READ_AG(agFirstFilesystem)->root, name + 1, acceptParent);
}

void Create(const char *name, char *data, ulong length)
{
FSNode	*parent;
FSNode	*node = (FSNode*)AllocateTaggedZero(sizeof(FSNode), "FS node");
		
	Assert(strrchr(name, '/') != nil);
		
	parent = Resolve(name, true);

	Assert(parent != nil);

#ifndef GOOBER_HACK			// ��� bringup hack for Goober ROM
	if (parent != nil)
		if (IsError(IsNotDirectoryNode(parent)))
			return; 
#endif /* GOOBER_HACK */

	node->data = data;
	node->dataLength = length;
	node->parent = parent;
	node->first = nil;
	(void)strncpy(node->name, strrchr(name, '/') + 1, kMaxFilenameLen - 1);
	
	/* add to front of list */
	
	if (parent != nil)
	{
		node->next = parent->first;
		parent->first = node;
	}
		
	/* checksum it, re-checksum it's parent */

	FileHasChanged(node);	
	
	if (parent != nil)
		FileHasChanged(parent);
}

	
	
char* Remove(const char *name)
{
FSNode	*node, *parent, **nodeAddress, *curNode;
char* data;

	if ((node = Resolve(name, false)) == nil)			/* note: have to check for read-only */
		return nil;
	
	data = node->data;
	parent = node->parent;								
	nodeAddress = &parent->first;
	while ((curNode = *nodeAddress) != node)			/* find the node before */
		nodeAddress = &curNode->next;
	
	*nodeAddress = node->next;							/* point to next node */
	FreeTaggedMemory(node, "FS node");					/* and free it */
	return data;
}

	
	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void SetName(const char *name, const char *newName)
{
FSNode	*node;
		
	if ((node = Resolve(name, false)) == nil)
		return;
		
	(void)strncpy(node->name, newName, kMaxFilenameLen);
	
	FileHasChanged(node);
}
#endif


ulong Checksum(uchar *p, ulong length)
{
	ulong iii;
	ulong sum = 0;
		
	for (iii=0;iii!=length;iii++)
	{	
		sum += (ulong)(*(uchar *)p);
		p++;
	}
	
	return(sum);
}
	
	
	
void FileHasChanged(FSNode *node)
{
	/* just recompute the checksums */
	
	if (node->data)
		node->dataChecksum = Checksum((uchar *)node->data,node->dataLength);
	else
		node->dataChecksum = 0;
	
	node->nodeChecksum = 0;			/* NOTE that the node checksum assumes that the node checksum field is 0! */
	
	node->nodeChecksum = Checksum((uchar *)node,sizeof(FSNode));
	
	Message(("File @ 0x%x changed:",node));
	Message(("	New nodeChecksum = %x",node->nodeChecksum));
	Message(("	New dataChecksum = %x",node->dataChecksum));
}
	
	
	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean FileIsConsistent(FSNode *node)
{
	ulong	nodeSumCopy;

	/* first, check the data */
	
	if (node->data != nil)
	{
		if (node->dataChecksum != Checksum((uchar *)node->data,node->dataLength))
		{
			Message(("### File @ 0x%x DATA Inconsistent!",node));
			return(false);
		}
	}

	/* then check the node, minus the node checksum */
	
	nodeSumCopy = node->nodeChecksum;
	node->nodeChecksum = 0;
	if (nodeSumCopy != Checksum((uchar *)node,sizeof(FSNode)))
	{
		Message(("### File @ 0x%x NODE Inconsistent!",node));
		node->nodeChecksum = nodeSumCopy;
		return(false);
	}
	node->nodeChecksum = nodeSumCopy;
		
	return(true);
}
#endif


/* This currently only checks the RAM Filesystem.
	We don't need to check the ROM Filesystem, but there may be other RAM ones... 
 */
 
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean FilesystemsAreConsistent(void)
{
FSNode *curNode;
	
	Message(("Verifying RAM Filesystem."));
	
	curNode = &(READ_AG(agFirstFilesystem)->root);
	
	if (!FileIsConsistent(curNode))		/* root is a special case */
		return(false);
	
	curNode = curNode->first;		
	
	while (curNode)
	{
		if (!FileIsConsistent(curNode))
			return(false);
		curNode = curNode->next;
	}
	
	Message(("Filesystems are OK."));
	return(true);
}
#endif


static Filesystem *GetFilesystemByName(uchar *name)
{
Filesystem *fs;
	
	fs = READ_AG(agFirstFilesystem);							/* start at first one */
	
	while( (strcmp((const char *)name,(const char *)fs->root.name) != 0) && (fs != 0) )		/* look for this one */
		fs = (Filesystem *)fs->root.next;

	return(fs);		/* nil if not found */
}
	
	
	
/* filesystem iterator utils */
	
char *GetFirstFilesystemName(void)
{
	return(READ_AG(agFirstFilesystem)->root.name);
}
	
	
	
char *GetNextFilesystemName(uchar *before)
{
Filesystem *fs;

	fs = GetFilesystemByName(before);
	
	if(fs)
	{
		fs = (Filesystem *)fs->root.next;
		if(fs)
			return(fs->root.name);
	}

	return(0);
}
	


/* file iterator utils */

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
FSNode *GetFirstFSNode(uchar *filesystemName)
{
Filesystem *fs;
	
	fs = GetFilesystemByName(filesystemName);
	return(fs->root.first);
}
#endif

	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
FSNode *GetNextFSNode(FSNode *fsn)
{
	return(fsn->next);
}
#endif

	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
char *GetFirstFileName(uchar *filesystemName)
{
FSNode *fsn;
	
	fsn = GetFirstFSNode(filesystemName);
	
	if (fsn)
		return(fsn->name);
	else
		return(0);
}
#endif


