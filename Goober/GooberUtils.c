#include "WTVTypes.h"
#include "HWRegs.h"
#include "BoxUtils.h"

#include "GooberUtils.h"

ulong gooberLEDs = 0;


void SetGooberLEDs(ulong bits)
{
	gooberLEDs = bits;
	*(volatile ulong *)kGooberLEDs = bits;	
}


ulong GetGooberLEDs(void)
{	
	return(gooberLEDs);
}


ulong	hbCnt = 0;
ulong	pats[7] = { 0x3, 0xa, 0xc, 0x14, 0x30, 0xa0, 0xc0 };
ulong	index = 0;
ulong	magic = 10000;
ulong 	toggle = 0;

void	GooberHeartbeat(void)
{
	hbCnt++;
	
	if (hbCnt == magic)
	{
		if( GOOBER_IS_PRESENT )
			SetGooberLEDs(pats[index]);
			
		index++;
		if (index == 7)
			index = 0;
			
		SetBoxLEDs( GetBoxLEDs() ^ 2 );		/* toggle middle board LED */
		
		hbCnt = 0;
	}
}





