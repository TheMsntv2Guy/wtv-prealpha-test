#include "WTVTypes.h"
#include "boxansi.h"
#include "SystemGlobals.h"
#include "HWRegs.h"
#include "Input.h"
#include "SmartCard.h"

ulong gSmartCardEvent = 0;

void CheckSmartCard(void)
{
	if( (*(volatile ulong*)kDevSmartCardReg & kSmartCardInsert) != 0 )
		PostSmartCardEvent(kSmartCardInserted);
}

void SmartCardIdle(void)
{
	switch(gSmartCardEvent)  
	{
		case 1:
			PostSmartCardEvent(kSmartCardInserted);
			break;
		case 2:
			PostSmartCardEvent(kSmartCardRemoved);
			break;
		default:
			break;
	}
}

void PostSmartCardEvent(ulong event)
{
	Input input; 
	
	input.data = event;
	input.device = kWebTVSmartCard;
	input.modifiers = 0;		
	input.rawData = event;
	input.time = gSystemTicks;			/* This sucks, add timestamp to kb posts */
	PostInput(&input);
	gSmartCardEvent = 0;
}