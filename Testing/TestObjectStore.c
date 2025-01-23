// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#include "Headers.h"
#include "Testing.Private.h"
#include "ObjectStore.h"

void TestObjectStore(void)
{
	char		*data1 = "abcdefg";
	char		*data2 = "hijk";
	char		*data4 = "qrstuv";
		
	Message(("Testing Object Store..."));
	
	Create("/RAM/File1", data1, strlen(data1) + 1);
	Create("/RAM/File2", data2, strlen(data2) + 1);
	Create("/RAM/Dir3", nil, 0);
	Create("/RAM/Dir3/File4", data4, strlen(data4) + 1);
	
	Assert(Resolve("/RAM/File1", false) != nil);
	Assert(Resolve("/RAM/File2", false) != nil);
	Assert(Resolve("/RAM/Dir3/File4", false) != nil);

	Assert(strcmp(Resolve("/RAM/File1", false)->data, data1) == 0);
	Assert(strcmp(Resolve("/RAM/File2", false)->data, data2) == 0);
	Assert(strcmp(Resolve("/RAM/Dir3/File4", false)->data, data4) == 0);

	// CW does not automatically make the ROM file
#if defined FOR_MPW	|| !defined __MWERKS__
	Assert(strncmp(Resolve("/ROM/Test", false)->data,
		"This is a test object in the ROM.", 33) == 0);
#endif
		
	Message(("Object Store testing completed."));
}

#ifdef COMPILE_AS_TOOL	
int main(int argc, char *argv[])
{
	#pragma unused (argc, argv)
	
	InitializeObjectStore();
	TestObjectStore();
	
	return 0;
}
#endif
