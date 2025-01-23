// Copyright(c) 1996 Artemis Research, Inc. All rights reserved.

#include "Headers.h"

#ifndef __CODE_H__
#include "Code.h"
#endif
#ifndef __INPUT_H__
#include "Input.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifdef HARDWARE
#include "Caches.h"
#endif

typedef struct {
	ulong	version;
	ulong	sizeofCodeParameters;
} CodeParameters;

typedef Boolean (*CodeFunction)(CodeParameters* codeParameters);

static ObjectList*	gCodeList = nil;

#ifdef FOO_DEBUG
static Boolean
TestFunction(CodeParameters* UNUSED(codeParameters))
{
#ifdef FOO_FOR_MAC
	if (codeParameters->version == 0)
		SysBeep(10);
#endif
	return true;
}
static Boolean
EndTestFunction(CodeParameters*)
{
	return true;
}

static void WriteTestFunction()
{
#ifdef FOR_MAC
	(void)create("TestFunction", 0, 'MPS ', 'TEXT');
#endif
	FILE*	fp = fopen("TestFunction", "w");
	
	if (fp == nil)
		return;
	ulong	twoZeros[2] = { 0, 0 };
	(void)fwrite(twoZeros, 1, 8, fp);
	(void)fwrite(*(char **)TestFunction, 1,
		*(char **)EndTestFunction - *(char **)TestFunction, fp);
#ifdef FOR_MAC
	SysBeep(40);
#endif
	fclose(fp);
}
#endif

// ===========================================================================

Code::~Code()
{
}

void
Code::NewCode(const Resource* resource)
{
	Code*	code = new(Code);
	
	if (gCodeList == nil)
		gCodeList = new(ObjectList);

	if (IsError(code == nil || gCodeList == nil))
		return;
	code->fResource = *resource;
	gCodeList->Add(code);
}

Boolean
Code::Idle()
// returns whether it was removed from list
{
	PerfDump perfdump("Code::Idle");

	CacheEntry*		entry;
	
	if (fResource.GetStatus() != kComplete)
		return false;

	Assert(fResource.GetDataType() == kDataTypeMIPSCode);
	entry = fResource.GetCacheEntry();
	Boolean	result = true;
	if IsError(entry == nil || entry->GetDataLength() < 8)
		goto Done;
		
	// set up the function and parameters
	{
		CodeParameters	codeParameters;
		codeParameters.version = 0;
		codeParameters.sizeofCodeParameters = sizeof(codeParameters);
		CodeFunction	codeFunction = (CodeFunction)entry->GetData();
		if IsError(codeFunction == nil)
			goto Done;
			
		// go do it
	#ifdef FOO_DEBUG
		if (CapsLockKeyDown())
			WriteTestFunction();
		codeFunction = TestFunction;
	#elif defined FOR_MAC
		((ulong*)codeFunction)[0] = (ulong)codeFunction + 8;
		((ulong*)codeFunction)[1] = 0xC97000;
		//FlushCodeCacheRange(codeFunction, entry->GetDataLength());
	#elif defined HARDWARE
		ClearCache();
	#endif
		result = (*codeFunction)(&codeParameters);
	}	
	// and remove it
Done:
	if (result)
		gCodeList->Remove(this);
	return result;
}

void
Code::IdleAll()
{
	if (gCodeList == nil)
		return;

	ulong	count = gCodeList->GetCount();
	
	for (int i = 0; i < count; i++)
	{
		Code*	current = (Code*)gCodeList->At(i);
		if (current->Idle()) {
			i--; count--;	// deal with removal
		}
	}
		
}