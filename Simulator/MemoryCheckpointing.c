// ===========================================================================
//	MemoryCheckpointing.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#if defined(DEBUG) && defined(MEMORY_TRACKING)

#include <errno.h>
#include <stdio.h>

#ifndef __MEMORYCHECKPOINTING_H__
#include "MemoryCheckpointing.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYMANAGER_PRIVATE_H__
#include "MemoryManager.Private.h"
#endif
#ifndef __SOUND_H__
//#include "Sound.h"
#endif

// ===========================================================================
// static helper functions

static void PrintOneBlock(FILE* fp, const char* blockName, ulong base, ulong length, Boolean used, const char* urlName,	const char* sourceFile, ulong lineNumber);
static void MemoryDumpTo(FILE* fp);
static Boolean SlotChanged(int i);
static Boolean SlotPersists(int i);
static void ReportDifferencesToFile(FILE* fp);
static void ReportPersistenceToFile(FILE* fp);
// ===========================================================================

static char*		gCurrentCheckpointFileName = nil;
static ulong		gCurrentDifferenceFileNumber = 0;
static MemoryTag	gCheckpointTagSlots[kTagSlots];

Boolean
MemoryCheckpointed(void)
{
	return gCurrentCheckpointFileName != nil;
}

static void PrintOneBlock(FILE* fp, const char* blockName,
	ulong base, ulong length, Boolean used, const char* urlName,
	const char* sourceFile, ulong lineNumber)
{	
	fprintf(fp, "%08X	%06X	", base, length);
	if (used)
	{
		MemoryTag	*tag = FindTag((void*)(((UsedBlock*)base)->data));
		ulong		tagIndex = (tag != nil) ? tag - gTagSlots : 0;
		
		fprintf(fp, "%04d	%s '%s'", tagIndex, blockName, urlName);
		for (int spacesNeeded = 59 - (strlen(blockName) + strlen(urlName) + 3); spacesNeeded > 0; spacesNeeded--)
			fprintf(fp, " ");
		fprintf(fp, "(File :Å:'%s'; Line %d)\n", sourceFile, lineNumber);
	}
	else
		fprintf(fp, "		¥free\n");
}

static void
MemoryDumpTo(FILE* fp)
{
	void*			block = nil;
	Boolean			used;
	ulong			length;
	char			buffer[256];
	char			sourceFile[256];
	char			urlName[256];
	ulong			lineNumber;
	ulong			usedCount = 0, freeCount = 0, cplusCount = 0;
	ulong			usedByteCount = 0, freeByteCount = 0, cplusByteCount = 0;
	Boolean			isCPlus;

	fprintf(fp, "Address		Length	Tag		Block Name, URL                                            Source Location\n");
	fprintf(fp, "--------	------	-----	------------------------------------------------------     ------------------------------------\n");
	
	while ((block = NextMemoryBlock(block, &used, &length, buffer, urlName, &isCPlus, sourceFile, &lineNumber)) != nil)
	{
		if (used)
		{
			usedCount++; usedByteCount += length;
			if (isCPlus)
			{	cplusCount++; cplusByteCount += length; }
		}
		else
		{	freeCount++; freeByteCount += length; }

		PrintOneBlock(fp, buffer, (ulong)block, length, used, urlName, sourceFile, lineNumber);
	}
	
	fprintf(fp, "--------	------	-----	------------------------------------------------------     ------------------------------------\n");
	fprintf(fp, "   Used blocks:\t%d (%d bytes)\n"
					"   Free blocks:\t%d (%d bytes)\n"
					"   C++ objects:\t%d (%d bytes)\n",
		usedCount, usedByteCount, freeCount, freeByteCount, cplusCount, cplusByteCount);
	fprintf(fp, "----------------------------------------------------------------------------------------------------------------------------------\n");
	
	// checkpoint the tags
	Assert(sizeof(gTagSlots) == sizeof(gCheckpointTagSlots));

	Message(("Finished checkpointing memory to file '%s'.", gCurrentCheckpointFileName));
}

void HandleNewMemoryCheckpoint(void)
{
	FILE*			fp;

	Message(("Checkpointing memory to file '%s'...", gCurrentCheckpointFileName));
	
	gCurrentCheckpointFileName = "Memory Checkpoint";
#ifdef FOR_MAC
	(void)create(gCurrentCheckpointFileName, 0, 'MPS ', 'TEXT');
#endif
	if ((fp = fopen(gCurrentCheckpointFileName, "w")) == nil)
	{	Complain(("Cannot open file %s (err %d)\n", gCurrentCheckpointFileName, errno)); return; }

	MemoryDumpTo(fp);
	fclose(fp);

	CopyMemory(gTagSlots, gCheckpointTagSlots, sizeof(gTagSlots));

	Message(("Finished checkpointing memory to file '%s'.", gCurrentCheckpointFileName));
}

static Boolean SlotChanged(int i)
{
	// note: does not look at modify time
	
	if (gTagSlots[i].fInUse != gCheckpointTagSlots[i].fInUse)
		return true;
	if (gTagSlots[i].fBase != gCheckpointTagSlots[i].fBase)
		return true;
	if (gTagSlots[i].fLength != gCheckpointTagSlots[i].fLength)
		return true;
	if (gTagSlots[i].fOwnerURL != gCheckpointTagSlots[i].fOwnerURL)
		return true;
	if (gTagSlots[i].fClassNumber != gCheckpointTagSlots[i].fClassNumber)
		return true;
	if (gTagSlots[i].fSourceFile != gCheckpointTagSlots[i].fSourceFile)
		return true;
	if (gTagSlots[i].fLineNumber != gCheckpointTagSlots[i].fLineNumber)
		return true;
	if (gTagSlots[i].fCreateTime != gCheckpointTagSlots[i].fCreateTime)
		return true;
	if (gTagSlots[i].fName != gCheckpointTagSlots[i].fName)
		return true;
	return false;
}


static Boolean SlotPersists(int i)
{
	// note: does not look at modify time
	
	if (gTagSlots[i].fInUse != gCheckpointTagSlots[i].fInUse)
		return false;
	if (gTagSlots[i].fCreateTime != gCheckpointTagSlots[i].fCreateTime)
		return false;
		
	return gTagSlots[i].fInUse != false;
}

static void ReportDifferencesToFile(FILE* fp)
{
	int		i;
	
	fprintf(fp, "New or Changed:\n");
	for (i = 0; i < kTagSlots; i++)
		if (SlotChanged(i))
			PrintOneBlock(fp, gTagSlots[i].fName, (ulong)gTagSlots[i].fBase - (sizeof(UsedBlock) - offsetof(UsedBlock, data)), gTagSlots[i].fLength,
				true, gTagSlots[i].fOwnerURL, gTagSlots[i].fSourceFile, gTagSlots[i].fLineNumber);
}

static void ReportPersistenceToFile(FILE* fp)
{
	int		i;
	
	fprintf(fp, "Still There:\n");
	for (i = 0; i < kTagSlots; i++)
		if (SlotPersists(i))
			PrintOneBlock(fp, gTagSlots[i].fName, (ulong)gTagSlots[i].fBase - (sizeof(UsedBlock) - offsetof(UsedBlock, data)), gTagSlots[i].fLength,
				true, gTagSlots[i].fOwnerURL, gTagSlots[i].fSourceFile, gTagSlots[i].fLineNumber);
}

void ReportMemoryDifference(void)
{
	if (!MemoryCheckpointed())
	{
		SysBeep(20);
		ImportantMessage(("Nothing to checkpoint against"));
		return;
	}
	
	gCurrentDifferenceFileNumber++;
	char differenceFileName[256];
	strcpy(differenceFileName, "Memory Difference");
	if (gCurrentDifferenceFileNumber != 1)
		sprintf(differenceFileName + strlen(differenceFileName),
			" #%d", gCurrentDifferenceFileNumber);
	
	Message(("Saving diffs to file '%s'...", differenceFileName));
	
#ifdef FOR_MAC
	(void)create(differenceFileName, 0, 'MPS ', 'TEXT');
#endif
	FILE* fp = fopen(differenceFileName, "w");
	if (fp == nil)
	{	Complain(("Cannot open file %s (err %d)\n", differenceFileName, errno)); return; }
	
	fprintf(fp, "Current Memory Dump:\n");
	MemoryDumpTo(fp);
	fprintf(fp, "\n\n");
	ReportDifferencesToFile(fp);
	fprintf(fp, "\n\n");
	ReportPersistenceToFile(fp);
	
	fclose(fp);
	
	Message(("Done saving diffs to file '%s'.", differenceFileName));
}

#endif /* defined(DEBUG) && defined(MEMORY_TRACKING) */
