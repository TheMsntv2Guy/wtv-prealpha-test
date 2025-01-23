#include "WTVTypes.h"
#include "Debug.h"
#include "Interrupts.h"
#include "HWRegs.h"

#include "HWRegs.h"
#include "HWExpansion.h"

#include "boxansi.h"


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void ExpansionEarlyInit()
{
	Message(("Expansion device early init...\n"));
}
#endif


#ifdef APPROM
void ExpansionLateInit()
{
	Message(("Expansion device late init...\n"));
}
#endif