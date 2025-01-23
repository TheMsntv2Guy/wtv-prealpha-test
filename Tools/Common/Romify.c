/* This tool converts a Mac HFS directory and sub-directories
 * into a format that is suitable for the WebTV ROM Filesystem.
 * The output goes to stdout.
 */

#include "MacintoshUndefines.h"
#include "Types.h"
#include "Strings.h"
#include "Memory.h"
#include "Files.h"
#include "MacintoshRedefines.h"

#include "WTVTypes.h"
#include "Debug.h"
#include "ObjectStore.h"
#include "ObjectStore.Private.h"
//

#undef Complain
#define Complain(x)		Debugger()

#include <CursorCtl.h>
#include <StdIO.h>
#include <StdArg.h>
#include <StdLib.h>
#include <String.h>

#define kBadAddress			0xBADDBADD
#define	kDefaultROMTop		0x80110000 - sizeof(ROMFSHeader)			/* 1MB ROM */

#define Public
#define Private

Public Boolean		gDebug = false;
Public Boolean		gProgressLevel = 0;

Public ulong		gROMTop = kDefaultROMTop;
Public ulong		gROMOffset = kDefaultROMTop;
Public char			*gDirectoryName = nil;
Public Handle		gDataHdl = nil;
Public Handle		gMetaDataHdl = nil;
Public ulong		gDataOffset = 0;
Public ulong		gMetaDataOffset = 0;
Public ulong		gLastDirOffset = 0;
Public ulong		gDataSize = 0;
Public ulong		gMetaDataSize = 0;
Public FILE			*gOutputFile = stdout;
Public FILE			*gErrorFile = stderr;

Private void
Fatal(ulong exitStatus, char *format, ...)
	{
    va_list     list;
	int			arg1, arg2, arg3, arg4, arg5, arg6;

	(void)fprintf(gErrorFile, "### ");

    va_start(list, format);
	arg1 = va_arg(list, int);
	arg2 = va_arg(list, int);
	arg3 = va_arg(list, int);
	arg4 = va_arg(list, int);
	arg5 = va_arg(list, int);
	arg6 = va_arg(list, int);
	(void)fprintf(gErrorFile, format, arg1, arg2, arg3, arg4, arg5, arg6);
	va_end(list);

	(void)fprintf(gErrorFile, "\n");
	exit(exitStatus);
	}

Private void
Progress(ulong progressLevel, char *format, ...)
	{
    va_list     list;
	int			arg1, arg2, arg3, arg4, arg5, arg6;

	if (gProgressLevel < progressLevel)
		return;
		
	(void)fprintf(gErrorFile, "# ");

    va_start(list, format);
	arg1 = va_arg(list, int);
	arg2 = va_arg(list, int);
	arg3 = va_arg(list, int);
	arg4 = va_arg(list, int);
	arg5 = va_arg(list, int);
	arg6 = va_arg(list, int);
	(void)fprintf(gErrorFile, format, arg1, arg2, arg3, arg4, arg5, arg6);
    va_end(list);

	(void)fprintf(gErrorFile, "\n");
	}

/* stops iteration if true returned */
typedef Boolean		(EachFunction)(short vRefNum, long dirID, StringPtr fileName, Boolean isDirectory, void *parameters);
typedef void		(EndEachFunction)(Boolean wereChildren, void *parameters);

Private Boolean
EachFileInFolder(short vRefNum, long dirID, EachFunction *function, EndEachFunction *endFunction, void *parameters)
	{
    CInfoPBRec  	dir;
	Str255			fileName;
	OSErr			err;

    dir.dirInfo.ioNamePtr = fileName;
    dir.dirInfo.ioDrParID = dirID;
	dir.dirInfo.ioVRefNum = vRefNum;
	dir.dirInfo.ioFDirIndex = -1;
	dir.dirInfo.ioDrDirID = dirID;
	if ((err = PBGetCatInfoSync(&dir)) != noErr)
		{
		(void)p2cstr(fileName);
		Fatal(5, "Cannot read directory \"%s\" (error %d)\n", fileName, err);
		}

	if (function != nil && (*function)(vRefNum, dir.dirInfo.ioDrParID, fileName, true, parameters))
		return true;

	dir.dirInfo.ioDrDirID = dirID;
	dir.dirInfo.ioFDirIndex = 1;

	while (PBGetCatInfoSync(&dir) == noErr)
		{
		Boolean		isDirectory = (dir.dirInfo.ioFlAttrib & (1<<4)) != 0;
				
		if (isDirectory)
		{
			if (EachFileInFolder(vRefNum, dir.dirInfo.ioDrDirID, function, endFunction, parameters))
				return true;
		}
		else
		{
			if (function != nil && (*function)(vRefNum, dirID, fileName, isDirectory, parameters))
				return true;
		}
		
		/* check next file in folder */
		dir.dirInfo.ioFDirIndex++;
		dir.dirInfo.ioDrDirID = dirID;	/* PBHGetFInfo() clobbers ioDirID */
		}
		
	if (endFunction != nil)
		(*endFunction)(dir.dirInfo.ioFDirIndex > 1, parameters);
	
	return false;
	}

Private ulong
FileSize(short vRefNum, long dirID, Str255 fileName)
	{
	HParamBlockRec	fi;

	fi.fileParam.ioCompletion = nil;
	fi.fileParam.ioNamePtr = fileName;
	fi.fileParam.ioVRefNum = vRefNum;
	fi.fileParam.ioDirID = dirID;
	fi.fileParam.ioFDirIndex = 0;
	(void)PBHGetFInfo(&fi, false);
	
	return fi.fileParam.ioFlLgLen;
	}
	
typedef struct
	{
	ulong		totalMetaSize;
	ulong		totalDataSize;
	} MeasureOneParameters;
	
Private Boolean
MeasureOne(short vRefNum, long dirID, Str255 fileName, Boolean isDirectory, MeasureOneParameters *parameters)
	{
	ulong			size = isDirectory ? 0 : FileSize(vRefNum, dirID, fileName);

	if ((size & 3) == 0)			/* If there is no zero padding then check to see it is a text file and pad it */
		{
		FInfo	fndrInfo;
		OSErr	anErr;
		
		anErr = HGetFInfo(vRefNum, dirID, fileName, &fndrInfo);
		if (anErr == noErr && fndrInfo.fdType == 'TEXT')
			size += 4;
		}
		
	Progress(2, "Measuring %P (%X,%X)...", fileName, vRefNum & 0xFFFF, dirID);
	Progress(3, "\tsize(%P) == %d bytes", fileName, size);
	
	Postulate((sizeof(FSNode) & 0x3) == 0);
	parameters->totalMetaSize += sizeof(FSNode);
	parameters->totalDataSize += (size + 0x3) & ~0x3;
	
	SpinCursor(32);
	return false;
	}
	
	
typedef struct
	{
	MeasureOneParameters	*sizes;
	FSNode					*parents[32];
	FSNode					*parentAddresses[32];
	ulong					index;
	} RomifyOneParameters;
	
Private Boolean
RomifyOne(short vRefNum, long dirID, Str255 fileName, Boolean isDirectory, RomifyOneParameters *parameters)
	{
	Ptr				dataPtr;
	ulong			size = FileSize(vRefNum, dirID, fileName);
	short			refNum;
	OSErr			err;
	FSNode			*node, *nodeAddress;

	(void)p2cstr(fileName);
	if (isDirectory)
		Progress(1, "Romifying :%s: (%x,%x)...", fileName, vRefNum & 0xFFFF, dirID);
	else
		Progress(1, "Romifying %s (%x,%x)...", fileName, vRefNum & 0xFFFF, dirID);
	(void)c2pstr((char *)fileName);

	HLock(gMetaDataHdl);
	
	gMetaDataOffset -= sizeof(FSNode);										/* make a hole for the FSNode */
	node = (FSNode *)(StripAddress(*gMetaDataHdl) + gMetaDataOffset);		/* and fill it in */
	node->parent = parameters->parentAddresses[parameters->index-1];

	Progress(3, "node is @ %x", gROMTop - (gMetaDataSize - gMetaDataOffset));
	Progress(3, "node->parent = %x", node->parent);
	
	if (!isDirectory)
		{
		if ((err = HOpen(vRefNum, dirID, fileName, fsWrPerm, &refNum)) != noErr)
			{
			(void)p2cstr(fileName);
			Fatal(3, "Cannot open file \"%s\" (error %d)\n", fileName, err);
			}
		
		node->dataLength = size;
		gDataOffset -= ((size + 3) & ~3);							/* make a hole for the data, rounded to 4 byte boundary */
		
		if ((size & 3) == 0)			/* If there is no zero padding then check to see it is a text file and pad it */
			{
			FInfo	fndrInfo;
			OSErr	anErr;
			
			anErr = HGetFInfo(vRefNum, dirID, fileName, &fndrInfo);
			if (anErr == noErr && fndrInfo.fdType == 'TEXT')
				{
				gDataOffset -= 4;
				(void)p2cstr(fileName);
				Progress(1, "Padding %s", fileName);
				(void)c2pstr((char *)fileName);
				}
			}

		node->data = (char *)(gROMTop - parameters->sizes->totalMetaSize - (gDataSize - gDataOffset));	/* and make the FSNode ref it */

		Progress(3, "node->data = %x", node->data);
		Progress(3, "node->dataLength = %x", node->dataLength);

		dataPtr = StripAddress(*gDataHdl) + gDataOffset;									/* make an offset into our block */
		//Assert(gDataOffset <= parameters->sizes->totalDataSize);
		SpinCursor(32);
		
		if ((err = FSRead(refNum, (long *)&size, dataPtr)) != noErr)						/* and read it */
			Fatal(4, "Cannot read from file \"%s\" (error %d)\n", fileName, err);
		SpinCursor(32);
		
		(void)FSClose(refNum);
		}

	nodeAddress = (FSNode *)(gROMTop - (gMetaDataSize - gMetaDataOffset));
	if (isDirectory)
		{
		gLastDirOffset = gMetaDataOffset;
		node->first = nodeAddress - 1;
		parameters->parents[parameters->index] = node;
		parameters->parentAddresses[parameters->index++] = nodeAddress;
		}
	else
		node->next = nodeAddress - 1;

	Progress(3, "node->first = %x", node->first);
	Progress(3, "node->next = %x", node->next);

	strncpy(node->name, (const char *)&fileName[1], (ulong)fileName[0]);	/* and finally, give it a name */
	node->name[(ulong)fileName[0]] = '\0';

	Progress(3, "node->name = %s", node->name);

	HUnlock(gMetaDataHdl);

	return false;
	}
	
Private void
RomifyOneEnd(Boolean wereChildren, RomifyOneParameters *parameters)
	{
	ulong			index = --parameters->index;
	FSNode			*next;
	
	Progress(1, "Completed romifying directory of depth %d", parameters->index);
	
	if (wereChildren)
	{
		((FSNode *)(StripAddress(*gMetaDataHdl) + gMetaDataOffset))[0].next = nil;
		Progress(3, "This node had children.  Setting node->next for %s to %x.",
									((FSNode *)(StripAddress(*gMetaDataHdl) + gMetaDataOffset))[0].name,
									((FSNode *)(StripAddress(*gMetaDataHdl) + gMetaDataOffset))[0].next);	
	}
	else
	{
		parameters->parents[index]->first = 0;
		Progress(3, "Empty directory, setting node->first for %s to 0", parameters->parents[index]->name);	
	}
	
	next = index == 0 ? nil : (FSNode *)(gROMTop - (gMetaDataSize - gMetaDataOffset) - sizeof(FSNode));
	
	parameters->parents[index]->next = next;

	Progress(3, "Setting node->next for %s to %x", parameters->parents[index]->name, parameters->parents[index]->next);	
	}
	
Private void
GetBaseDirID(short *vRefNum, long *dirID)
	{
	OSErr					err;

	Progress(1, "Looking up directory %s", gDirectoryName);

	*vRefNum = 0; *dirID = 0;
	c2pstr(gDirectoryName);
	err = HGetVol((StringPtr)gDirectoryName, vRefNum, dirID);
	(void)p2cstr((StringPtr)gDirectoryName);

	if (err != noErr)
		Fatal(2, "Cannot find directory \"%s\" (error %d)\n", gDirectoryName, err);
	}

Private void
EndData(void)
	{
	if (gMetaDataHdl != nil)
		DisposeHandle(gMetaDataHdl);
	if (gDataHdl != nil)
		DisposeHandle(gDataHdl);
	}
	
Private void
BuildData(void)
	{
	MeasureOneParameters	parameters1 = {0,0};
	RomifyOneParameters		parameters2 = {&parameters1,0};
	short					vRefNum;
	long					dirID;
	OSErr					err;
	
	GetBaseDirID(&vRefNum, &dirID);
	(void)EachFileInFolder(vRefNum, dirID, MeasureOne, nil, &parameters1);
	
	Progress(1, "Total meta size: %d", parameters1.totalMetaSize);
	Progress(1, "Total data size: %d", parameters1.totalDataSize);

	gMetaDataHdl = TempNewHandle(parameters1.totalMetaSize,&err);
	gDataHdl = TempNewHandle(parameters1.totalDataSize,&err);		/* this must be zeroed so that pad bytes are zeros */
	if (gMetaDataHdl == nil || gDataHdl == nil)
	{
		EndData();
		Fatal(6, "Not enough memory (need %d kbytes)", (parameters1.totalMetaSize + parameters1.totalDataSize)/1024);
	}
	memset(*gMetaDataHdl,0,parameters1.totalMetaSize);
	memset(*gDataHdl,0,parameters1.totalDataSize);
	
	gMetaDataSize = gMetaDataOffset = parameters1.totalMetaSize;		/* start at the top of each block & grow DOWN */
	gDataSize = gDataOffset = parameters1.totalDataSize;
	
	(void)EachFileInFolder(vRefNum, dirID, RomifyOne, RomifyOneEnd, &parameters2);
	
	Progress(1, "Fixing up node->next for last directory node...\n");
	((FSNode *)(StripAddress(*gMetaDataHdl) + gLastDirOffset))[0].next = nil;	/* fix up last node's next pointer */
	}

Private void
WriteHandle(Handle hdl)
	{
	ulong			size = GetHandleSize(hdl);
	
	HLock(hdl);
	(void)fwrite(StripAddress(*hdl), size, 1, gOutputFile);
	HUnlock(hdl);
	}
	
Private void
WriteData(void)
	{
	WriteHandle(gDataHdl);				/* data is lower in ROM */
	WriteHandle(gMetaDataHdl);			/* nodes are at top of ROM */
	}
	
Private Boolean
ParseOptions(char **argv)
	{
	char	*argument;
	char	*pch;
	Boolean	gotFileName = false;

	*argv++;
	while ((argument = *argv++) != nil)
		{
		if (*argument != '-')
			{
			if (gDirectoryName != nil)
				return false;
			gDirectoryName = argument;
			continue;
			}
			
		switch (*++argument)
			{
			case 'R': case 'r':
				if (strncmp(argument + 1, "omtop=", 6) != 0)
					return false;
				(void)sscanf(argument + 7, "%x", &gROMTop);
				gROMTop -= sizeof(ROMFSHeader);
				break;
				
			case 'D': case 'd':
				if (strcmp(argument + 1, "ebug") != 0)
					return false;
				gDebug = true;
				break;

			case 'P': case 'p':
				gProgressLevel = 1;
				if ((pch = strrchr(argument, '=')) != nil)
					gProgressLevel = atoi(pch+1);
				break;
				
			default:
				return false;
			}
		}
		
	return gDirectoryName != nil;
	}

Public int
main(int argc, char *argv[])
	{
	#pragma unused (argc)

	InitCursorCtl(nil);
	
	if (!ParseOptions(argv))
		Fatal(1, "Usage: Romify -romtop=<top addr + 1 (hex)> [-progress] <directory name>\n");

	Progress(1, "ROM Top = %x",gROMTop);

	BuildData();
	WriteData();
	EndData();
	
	return 0;
	}