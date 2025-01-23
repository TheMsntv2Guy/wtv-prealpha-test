// =============================================================================
//	version info
//
#include "Headers.h"
#include "System.h"
#include "SystemVersion.h"


// These have to be static due to initialization issues
static char gVersionString[128];
static ulong gBuildNumber;

const char * System::GetVersion() const
{
	// version consists of: "Phase (build #)" where
	//		Phase = Alpha, Beta, etc. and is defined in System.h
	//		Build # = is the build date, or the official build number

	strcpy(gVersionString, kSoftwarePhase);
	strcat(gVersionString, kSoftwareBuild);
	
	return gVersionString;
}



ulong System::GetVersionNumber() const
{	
	gBuildNumber = kSoftwareBuildNumber;
	return gBuildNumber;
}
