#ifdef DEBUG
#include "Headers.h"
#include "BoxUtils.h"
#include "Input.h"
#include "HWKeyboard.h"
#include "AlertWindow.h"
#include "PageViewer.h"

int	gMonkeyEnable = false;

typedef struct MonkeyWeight {
	ulong key;
	ulong weight;
} MonkeyWeight;

MonkeyWeight gMonkeyWeights[] = {
	kRecentKey,2,
	kOptionsKey,2,
	kBackKey,4,
	kExecuteKey,30,
	kLeftKey,30,
	kUpKey,30,
	kRightKey,50,
	kDownKey,50,
	kScrollDownKey,40,
	kScrollUpKey,30
};

void DoTheMonkey(void)
{

	ulong n;
	Input input; 
	ulong weight = 0;
	ulong totalWeight = 0;
	int i;
	
	input.device = kPCKeyboard;
	input.time = Now();
	if (gAlertWindow->IsVisible()) 
	{
		input.data = kExecuteKey;
		PostInput(&input);
	}
	else if (gPageViewer->Completed()) 
	{
		for(i=0;i<sizeof(gMonkeyWeights)/sizeof(MonkeyWeight);i++)
			totalWeight += gMonkeyWeights[i].weight;

		n = GetRandom() % totalWeight;

		for(i=0;i<sizeof(gMonkeyWeights)/sizeof(MonkeyWeight);i++)
		{
			weight += gMonkeyWeights[i].weight;
			if(weight > n)
			{
				input.data = gMonkeyWeights[i].key;
				break;
			}
		}
		
		PostInput(&input);
	}
}

#endif