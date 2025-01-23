#include "Headers.h"
#include "HWRegs.h"
#include "Fence.h"


#ifndef BOOTROM

void SetFence1Range(ulong bottom, ulong top)
{
ulong mask;

	*(volatile ulong*)kBusFenceAddress1Reg = bottom;
	
	mask = top - bottom;	/* assumes top > bottom */
		
	mask = ~mask;			/* invert to form AND mask */

	*(volatile ulong*)kBusFenceMask1Reg = mask;	/* low 5 bits are don't care */
	
	Message(("Fence 1 range = %08x to %08x", bottom, top));
}



void SetFence2Range(ulong bottom, ulong top)
{
ulong mask;

	*(volatile ulong*)kBusFenceAddress2Reg = bottom;
	
	mask = top - bottom;	/* assumes top > bottom */
		
	mask = ~mask;			/* invert to form AND mask */

	*(volatile ulong*)kBusFenceMask2Reg = mask;	/* low 5 bits are don't care */

	Message(("Fence 2 range = %08x to %08x", bottom, top));
}

#endif /* BOOTROM */


ulong EnableFences(ulong enable)
{
ulong oldEnable;

#ifdef DEBUG
	
	if(enable & kFence1ReadIntMask)
		Message(("Fence 1 READ PROTECT enabled"));
		
	if(enable & kFence2ReadIntMask)
		Message(("Fence 2 READ PROTECT enabled"));
		
	if(enable & kFence1WriteIntMask)
		Message(("Fence 2 WRITE PROTECT enabled"));
		
	if(enable & kFence2WriteIntMask)
		Message(("Fence 2 WRITE PROTECT enabled"));
		
#endif

	oldEnable = *(volatile ulong *)kBusErrEnableSetReg;

	*(volatile ulong*)kBusErrEnableSetReg = (oldEnable | enable);
	
	return oldEnable;
}



ulong DisableFences(ulong disable)
{
ulong oldEnable;

#ifdef DEBUG
	
	if(disable & kFence1ReadIntMask)
		Message(("Fence 1 READ PROTECT disabled"));
		
	if(disable & kFence2ReadIntMask)
		Message(("Fence 2 READ PROTECT disabled"));
		
	if(disable & kFence1WriteIntMask)
		Message(("Fence 2 WRITE PROTECT disabled"));
		
	if(disable & kFence2WriteIntMask)
		Message(("Fence 2 WRITE PROTECT disabled"));
		
#endif

	oldEnable = *(volatile ulong *)kBusErrEnableSetReg;
	
	*(volatile ulong*)kBusErrEnableClearReg = disable;
	
	return oldEnable;
}
