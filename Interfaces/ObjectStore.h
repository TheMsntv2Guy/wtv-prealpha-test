/* ===========================================================================
    ObjectStore.h

    Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __OBJECTSTORE_H__
#define __OBJECTSTORE_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define kMainRAMDirectoryName	"RAM"

#define kMaxFilenameLen		28

typedef struct FSNode	FSNode;
struct FSNode
{
	FSNode		*next;				/* pointer to next file's FSNode */
	FSNode		*parent;			/* pointer to parent directory */
	FSNode		*first;				/* 0 for object, or pointer to first FSNode in dir */
	char		*data;				/* 0 for directory, or pointer to file data */
	ulong		dataLength;					/* size of object */
	ulong		nodeChecksum;
	ulong		dataChecksum;				/* 0 if data = nil */
	char		name[kMaxFilenameLen];
};


typedef struct Filesystem Filesystem;
struct Filesystem
{
	FSNode		root;			/* root node of Filesystem */
	Boolean		writeable;		/* can be written to */
};


typedef struct						/* only used by downloader & boot code to verify ROMFS */
{
	unsigned long		romfs_length;
	unsigned long		romfs_checksum;
	
} ROMFSHeader;


#ifdef INCLUDE_FINALIZE_CODE
void		FinalizeFilesystems(void);
#endif /* INCLUDE_FINALIZE_CODE */
void		InitializeFilesystems(void);
FSNode		*Resolve(const char *name, Boolean acceptParent);
void		Create(const char *name, char *data, ulong length);
char*		Remove(const char *name);
void		SetName(const char *name, const char *newName);

ulong 		Checksum(uchar *p, ulong length);
void		FileHasChanged(FSNode *node);
Boolean		FileIsConsistent(FSNode *node);
Boolean		FilesystemsAreConsistent(void);

char 		*GetFirstFilesystemName(void);
char 		*GetNextFilesystemName(uchar *before);

FSNode 		*GetFirstFSNode(uchar *filesystemName);
FSNode 		*GetNextFSNode(FSNode *fsn);
char 		*GetFirstFileName(uchar *filesystemName);


/* Private Stuff */

/* CAUTION!  When using these, BE SURE that (node != nil) or you will
	be unhappy on the R3000.
 */
 
#define IsDirectoryNode(node)		(node->data == nil)
#define IsNotDirectoryNode(node)	(node->data != nil)

#ifdef __cplusplus
}
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include ObjectStore.h multiple times"
	#endif
#endif /* __OBJECTSTORE_H__ */