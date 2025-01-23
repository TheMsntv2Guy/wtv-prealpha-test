#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <Types.h>
#include <Memory.h>
#include <Resources.h>

int Usage(void)
{
	fprintf(stderr, "### Usage: RezCat <file> <resource-type> <resource-id>\n");
	return 1;
}

int main(int argc, char *argv[])
{
	char*	resourceFileName;
	char*	resourceTypeName;
	long	resourceType;
	int		resourceID;
	short	refNum;
	Handle	hdl;
	
	if (argc != 4)
		return Usage();

	resourceFileName = argv[1];
	resourceTypeName = argv[2];
	resourceID = atoi(argv[3]);
		
	if (strlen(resourceTypeName) != 4)
		return Usage();
	resourceType = (resourceTypeName[0]<<24) + (resourceTypeName[1]<<16) + 
		 (resourceTypeName[2]<<8) + resourceTypeName[3];
		 
	if ((refNum = openresfile(resourceFileName)) < 0)
	{
		fprintf(stderr, "### Cannot open \"%s\" (error %d)\n",
			resourceFileName, ResError());
		return 2;
	}
	
	if ((hdl = Get1Resource(resourceType, resourceID)) == nil)
	{
		fprintf(stderr, "### Error getting resource '%s' (error %d)\n",
			resourceTypeName, ResError());
		CloseResFile(refNum);
		return 3;
	}
	
	HLock(hdl);
	fwrite(StripAddress(*hdl), 1, GetHandleSize(hdl), stdout);
	HUnlock(hdl);
	
	if (errno != 0)
	{
		fprintf(stderr, "### Error writing to stdout (error %d)\n", errno);
		CloseResFile(refNum);
		return 3;
	}
		
	CloseResFile(refNum);
	return 0;
}