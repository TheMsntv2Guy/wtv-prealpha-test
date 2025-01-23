#include "WTVTypes.h"

#include "HWRegs.h"
#include "Interrupts.h"
#include "Exceptions.h"
#include "BoxUtils.h"


ulong mysteryIntCause = 0;		/* written to by notLevel4 in Interrupts.s */
ulong mysteryIntSR = 0;			/* written to by notLevel4 in Interrupts.s */
ulong mysteryIntEPC = 0;		/* written to by notLevel4 in Interrupts.s */




