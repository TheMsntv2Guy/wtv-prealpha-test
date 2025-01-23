// ===========================================================================
//	PerfDumpToKScope.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "PerfDumpToKScope.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CursorCtl.h>

// ===========================================================================
//	typedefs

enum SymbolType {
	kSymbolTypeNone,
	kSymbolTypeIn,
	kSymbolTypeOut,
	kSymbolTypeMark,
	kSymbolTypeUnknown
};

struct SymbolLookup {
	const char*	symbolTypeString;
	SymbolType	symbolType;
	long		symbolLevel;
};

SymbolLookup gSymbolLookup[] =
	{
		{"None",	kSymbolTypeNone,	0},
		{"Enter",	kSymbolTypeIn,		1},
		{"Exit",	kSymbolTypeOut,		-1},
		{"Mark",	kSymbolTypeMark,	0},
		{"???",		kSymbolTypeUnknown,	0}
	};


struct Symbol {
	const char*		name;
	SymbolType		type;
	short			initDepth;
	short			currDepth;
	short			maxDepth;
	short			index;
	Symbol*			next;
};

// ===========================================================================
//	local functions

static SymbolType LookupSymbolType(const char* typebuf);
static short LookupSymbolLevel(const char* typebuf);
static short LookupSymbolLevel(SymbolType symbolType);
static Symbol* LookupSymbol(const char* namebuf, const char* typebuf);
static Symbol* LookupSymbol(const char* namebuf, SymbolType symbolType);

static void InitializeSymbols(void);
static void FinalizeSymbols(void);

static Symbol* NewSymbol(const char* name, const char* typebuf);
static Symbol* NewSymbol(const char* name, SymbolType symbolType);
static void DeleteSymbol(Symbol* symbol);

static void AppendSymbol(Symbol* symbol);
static void AddSymbol(const char* namebuf, const char* typebuf);
static void WriteSymbols(void);
static void PreWriteEvents(short outRefNum);
static void WriteEvent(short outRefNum, const char* namebuf, const char* typebuf, const char* timebuf);
static void PostWriteEvents(short outRefNum);

// ===========================================================================
//	implementations


Symbol* gSymbolList = nil;


static SymbolType
LookupSymbolType(const char* typebuf)
{
	SymbolType symbolType = kSymbolTypeUnknown;
	
	for (int i=0; i<(sizeof(gSymbolLookup)/sizeof(gSymbolLookup[0])); i++) {
		if (strcmp(typebuf, gSymbolLookup[i].symbolTypeString) == 0) {
			symbolType = gSymbolLookup[i].symbolType;
			break;
		}
	}
	
	return symbolType;	
}


static short
LookupSymbolLevel(const char* typebuf)
{
	short symbolLevel = 0;
	
	for (int i=0; i<(sizeof(gSymbolLookup)/sizeof(gSymbolLookup[0])); i++) {
		if (strcmp(typebuf, gSymbolLookup[i].symbolTypeString) == 0) {
			symbolLevel = gSymbolLookup[i].symbolLevel;
			break;
		}
	}
	
	return symbolLevel;	
}


static short
LookupSymbolLevel(SymbolType symbolType)
{
	short symbolLevel = 0;
	
	for (int i=0; i<(sizeof(gSymbolLookup)/sizeof(gSymbolLookup[0])); i++) {
		if (symbolType == gSymbolLookup[i].symbolType) {
			symbolLevel = gSymbolLookup[i].symbolLevel;
			break;
		}
	}
	
	return symbolLevel;	
}


static Symbol*
LookupSymbol(const char* namebuf, const char* typebuf)
{
	SymbolType symbolType = LookupSymbolType(typebuf);

	return LookupSymbol(namebuf, symbolType);
}


static Symbol*
LookupSymbol(const char* namebuf, SymbolType symbolType)
{
	Symbol* symbol = gSymbolList;
	
	while (symbol != nil) {
		if ((strcmp(symbol->name, namebuf) == 0)
			&& ((symbol->type == symbolType) ||
					((symbol->type == kSymbolTypeIn)
					&& (symbolType == kSymbolTypeOut)))) {
			break;
		}
		symbol = symbol->next;
	}

	return symbol;
}


static Symbol*
NewSymbol(const char* name, const char* typebuf)
{
	return NewSymbol(name, LookupSymbolType(typebuf));
}

static Symbol*
NewSymbol(const char* name, SymbolType symbolType)
{
	int namelen = strlen(name) + 1;
	Symbol* symbol = (Symbol*)NewPtr(sizeof(Symbol) + namelen);
	if (symbol != nil) {
		char* destname = (char*)symbol + sizeof(Symbol);
		strcpy(destname, name);
		symbol->name = destname;
	}
	
	symbol->type = (symbolType == kSymbolTypeOut) ? kSymbolTypeIn : symbolType;
	symbol->initDepth = 0;
	symbol->currDepth = 0;
	symbol->maxDepth = 0;
	symbol->index = 0;
	
	return symbol;
}

static void
DeleteSymbol(Symbol* symbol)
{
	if (symbol != nil) {
		DisposePtr((char*)symbol);
	}
}

static void
AppendSymbol(Symbol* symbol)
{
	if (gSymbolList == nil) {
		gSymbolList = symbol;
	} else {
		Symbol* nextToLast = gSymbolList;
		while (nextToLast->next != nil) {
			nextToLast = nextToLast->next;
		}
		nextToLast->next = symbol;
	}
	symbol->next = nil;
}

// ===========================================================================

static void
InitializeSymbols(void)
{
	gSymbolList = nil;
}

static void
FinalizeSymbols(void)
{
	while (gSymbolList != nil) {
		Symbol* tempSymbol = gSymbolList->next;
		gSymbolList = nil;
		DeleteSymbol(gSymbolList);
		gSymbolList = tempSymbol;
	}
}

static void
AddSymbol(const char* namebuf, const char* typebuf)
{
	SymbolType symbolType = LookupSymbolType(typebuf);
	Symbol* symbol = LookupSymbol(namebuf, symbolType);
	short level = LookupSymbolLevel(typebuf);	

	if (symbol == nil) {
		symbol = NewSymbol(namebuf, typebuf);
		if (symbol == nil)
			return;
		AppendSymbol(symbol);
	} else if (level < 0) {
		symbol->currDepth += level;	// if symbol is new, don't bother popping out
	}
	
	if (symbol->currDepth > symbol->maxDepth)
		symbol->maxDepth = symbol->currDepth;
	
	if (symbol->currDepth < 0) {
		short adjustment = -symbol->currDepth;
		symbol->initDepth += adjustment;
		symbol->currDepth = 0;
		symbol->maxDepth += adjustment;
	}
		
	if (level > 0) {
		symbol->currDepth += level;
	}
}

static void
WriteSymbols(void)
{
	// go through symbols and calculate indices
	Symbol* symbol = gSymbolList;
	short index = 1;	// start from 1
	short entries = 0;
	while (symbol != nil) {
		symbol->index = index;
		index += symbol->maxDepth + 1;
		entries += symbol->maxDepth + 1;
		if (symbol->type == kSymbolTypeIn) {
			index += symbol->maxDepth + 1;
		}
		symbol = symbol->next;
	}
	
	// write symbols to the handle
	long handleLength = entries * 256;
	Handle symbolHandle = NewHandle(handleLength);
	if (symbolHandle == nil)
		return;
	
	HLock(symbolHandle);
	
	symbol = gSymbolList;
	long lengthUsed = 0;
	char* currLocation = (char*)(*symbolHandle);
	
	if (currLocation == nil)
		return;
	
	lengthUsed += sizeof(short);
		*(((short*)currLocation)++) = entries;
	
	while (symbol != nil) {
		index=0;
		while (index<=symbol->maxDepth) {
		
			if (lengthUsed + 512 > handleLength) {
				HUnlock(symbolHandle);
				SetHandleSize(symbolHandle, handleLength + 1024);
				HLock(symbolHandle);
				currLocation = ((char*)(*symbolHandle)) + lengthUsed;
				
				handleLength = GetHandleSize(symbolHandle);
				if (lengthUsed + 512 > handleLength) {
					// resize failed
					DisposeHandle(symbolHandle);
					return;
				}
			}

			short marker = symbol->index + index;
			short partner = 0;
			if (symbol->type == kSymbolTypeIn) {
				partner = marker + symbol->maxDepth + 1;
			}
			
			lengthUsed += sizeof(short);
				*(((short*)currLocation)++) = marker;	// marker
			lengthUsed += sizeof(short);
				*(((short*)currLocation)++) = partner;	// partner
			lengthUsed += sizeof(long);
				*(((long*)currLocation)++) = 0; 		// extra long
			lengthUsed += sizeof(short);
				*(((short*)currLocation)++) = 0;		// red
			lengthUsed += sizeof(short);
				*(((short*)currLocation)++) = 0;		// green
			lengthUsed += sizeof(short);
				*(((short*)currLocation)++) = 0;		// blue
			lengthUsed += sizeof(short);
				*(((short*)currLocation)++) = 0;		// extra short

			// name
			long namelength = sprintf(&(currLocation[1]), (symbol->maxDepth>0) ? "%s%d" : "%s", symbol->name, index+1);	
			lengthUsed += namelength+1;
				*currLocation = namelength;
				currLocation += namelength+1;
			
			// extra string
			lengthUsed++;
				*currLocation++ = 0;
			
			index++;
		}
		symbol = symbol->next;
	}
	HUnlock(symbolHandle);
	SetHandleSize(symbolHandle, lengthUsed);
	AddResource(symbolHandle, 'petl', 129, "\pPerfDump names");
}

static void
PreWriteEvents(short outRefNum)
{
#pragma unused ( outRefNum )
	Symbol* symbol = gSymbolList;
	
	while (symbol != nil) {
		symbol->currDepth = symbol->initDepth;
		symbol = symbol->next;
	}
	
	// go through symbols and write "in" events for symbols w/initDepth > 0
}

static void
WriteEvent(short outRefNum, const char* namebuf, const char* typebuf, const char* timebuf)
{
	short eventBuffer[7] = {0};
	long eventBufferSize = sizeof(eventBuffer);
	
	SymbolType symbolType = LookupSymbolType(typebuf);
	short symbolLevel = LookupSymbolLevel(symbolType);
	Symbol* symbol = LookupSymbol(namebuf, symbolType);
	
	if (symbol == nil)
		return;
	
	if (symbolLevel < 0)
		symbol->currDepth += symbolLevel;

	eventBuffer[0] = symbol->index + symbol->currDepth;
	if (symbolType == kSymbolTypeOut) {
		eventBuffer[0] += symbol->maxDepth+1;
	}
	
	long timeval = 0;
	sscanf(timebuf, "%ld", &timeval);
	eventBuffer[1] = timeval>>16;
	eventBuffer[2] = (short)timeval;
	
	if (symbolLevel > 0)
		symbol->currDepth += symbolLevel;

	FSWrite(outRefNum, &eventBufferSize, (void*)(&(eventBuffer[0])));
}

static void
PostWriteEvents(short outRefNum)
{
#pragma unused ( outRefNum )
	// go through symbols and write "out" events for symbols w/currDepth > 0
}

// ===========================================================================

void
PerfDumpToKScope(const FSSpecPtr inSpecPtr)
{
	int cursorFrame = 0;

	if (inSpecPtr == nil)
		return;
	
	FSSpec inSpec;
	FSSpec outSpec;
	OSErr err;
	
	err = FSMakeFSSpec(inSpecPtr->vRefNum, inSpecPtr->parID, inSpecPtr->name, &inSpec);
	if (err != noErr) {
		return;	
	}
	
	InitializeSymbols();

	short saveVRefNum = 0;
	long saveParID = 0;
	
	// open working directory
	err = HGetVol(nil, &saveVRefNum, &saveParID);
	err = HSetVol(nil, inSpec.vRefNum, inSpec.parID);
	if (err != noErr) {
		goto reset_defaultDirectory;
	}
	
	// open file
	char filename[64];
	memcpy(filename, &(inSpec.name[1]), inSpec.name[0]);
	filename[inSpec.name[0]] = '\0';
	
	FILE* fp_in = fopen(filename, "r");
	
	if (fp_in == nil) {
		goto reset_defaultDirectory;
	}
	

	// verify that file is a PerfDump file
	int version;
	if (fscanf(fp_in, "PerfDump version %d", &version) != 1) {
		goto close_fp_in;
	}

	if (version != 1) {
		//DebugStr("\pThis version of PerfDumpToKScope doesn't understand this version of PerfDump file");
		goto close_fp_in;
	}

	while (true) {
		int ch = fgetc(fp_in);
		
		if (ch==EOF)
			goto close_fp_in;
		
		if ((ch=='\n') || (ch=='\r'))
			break;
	}
	
	fpos_t dataPos;
	
	if (fgetpos(fp_in, &dataPos) != 0) {
		goto close_fp_in;
	}
	
	// create output filespec
	char outfilename[64];
	const char suffix[] = ".kscope";
	int lettersToPrint = inSpec.name[0];
	if (lettersToPrint > 32 - sizeof(suffix))
		lettersToPrint = 32 - sizeof(suffix);
	
	outfilename[0] = sprintf(&(outfilename[1]), "%.*s%s", lettersToPrint,
							&(inSpec.name[1]), suffix);
	err = FSMakeFSSpec(inSpec.vRefNum, inSpec.parID, (StringPtr)outfilename, &outSpec);

	if (err == noErr) {
		FSpDelete(&outSpec);	// get rid of old one before we start doing things to it
		err = FSMakeFSSpec(inSpec.vRefNum, inSpec.parID, (StringPtr)outfilename, &outSpec);
	}
	
	if (err != fnfErr)
		goto close_fp_in;
		
	
	FSpCreateResFile(&outSpec, kPerfDumpToKScopeSignature, kPerfDumpToKScopeType, smSystemScript);
	err = ResError();
	if (err != noErr) {
		goto close_fp_in;
	}
	
	// pass #1...collect symbols from PerfDump file
	char namebuf[128], typebuf[128], timebuf[128], filebuf[128], linebuf[128];
	
	while (fscanf(fp_in, "%s %s %s %s %s\n", &namebuf, &typebuf, &timebuf, &filebuf, &linebuf)==5) {
		AddSymbol(namebuf, typebuf);
		if ((cursorFrame++ & 0xff) == 0)
			SpinCursor(0);
	}

	short oldResFile = CurResFile();

		// write symbols to PerfDump file
		short outRefNum = FSpOpenResFile(&outSpec, fsRdWrPerm);
		if (outRefNum == -1) {
			short resErr = ResError();
			goto close_fp_in;
		}
		UseResFile(outRefNum);
		WriteSymbols();
		CloseResFile(outRefNum);

	UseResFile(oldResFile);
	
	// pass #2...copy events from PerfDump file, write to output
	if (fsetpos(fp_in, &dataPos) != 0) {
		goto close_and_delete_outSpec;
	}
	
	err = FSpOpenDF(&outSpec, fsWrPerm, &outRefNum);	
	
	PreWriteEvents(outRefNum);
	while (fscanf(fp_in, "%s %s %s %s %s\n", &namebuf, &typebuf, &timebuf, &filebuf, &linebuf)==5) {
		WriteEvent(outRefNum, namebuf, typebuf, timebuf);	
		if ((cursorFrame++ & 0xff) == 0)
			SpinCursor(0);
	}
	PostWriteEvents(outRefNum);

	FSClose(outRefNum);


	if (false) {
close_and_delete_outSpec:
		FSClose(outRefNum);
delete_outSpec:
		FSpDelete(&outSpec);
	}
	
close_fp_in:
	fclose(fp_in);
	
reset_defaultDirectory:
	HSetVol(nil, saveVRefNum, saveParID);

	FinalizeSymbols();

	SetCursor ( &qd.arrow );	// change this for code resources!

	return;
}


