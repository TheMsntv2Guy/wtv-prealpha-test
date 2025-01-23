#include "WTVTypes.h"
#include "Debug.h"
#include "boxansi.h"
#include "SystemVersion.h"  /* mostly for auto dependency stuff */

void ShowBuildInfo(void);

void ShowBuildInfo(void)
{
	Message(("\n"));
	Message(("### ROM built by : %s",BUILDER));
	Message(("### ROM built on : %s",BUILD_DATE));
}