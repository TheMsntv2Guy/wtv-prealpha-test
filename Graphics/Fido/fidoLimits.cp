#include "fidoDebug.h"
#include "fidoLimits.h"
#include "fidoSim.h"

int fidoLimits::verticalBlankTop;
int fidoLimits::physicalLeft;
int fidoLimits::physicalTop;
int fidoLimits::physicalRight;
int fidoLimits::physicalBottom;
int fidoLimits::visibleLeft;
int fidoLimits::visibleTop;
int fidoLimits::visibleRight;
int fidoLimits::visibleBottom;
// if (0, 0) is upper left corner:
const int fidoLimits::minX = 0;
int fidoLimits::maxX;
const int fidoLimits::minY = 0;
int fidoLimits::maxY;
fidoLimits::videoMode fidoLimits::mode;

void fidoLimits::setup(videoMode m)
{
	mode = m;
	// NTSC screen: 560 x 420
	if (mode == ntsc) {
		verticalBlankTop = -274;
		physicalLeft = -320;
		physicalTop = -239;
		physicalRight = 320;
		physicalBottom = 240;
		visibleLeft = physicalLeft + 40;
		visibleTop = physicalTop + 30;
		visibleRight = physicalRight - 40;
		visibleBottom = physicalBottom - 30;
	} else if (mode == pal) {
		verticalBlankTop = -332;
		physicalLeft = -384;
		physicalTop = -279;
		physicalRight = 384;
		physicalBottom = 280;
		visibleLeft = physicalLeft + 40;
		visibleTop = physicalTop + 30;
		visibleRight = physicalRight - 40;
		visibleBottom = physicalBottom - 30;
	} else
		Assert(0);
	maxX = visibleRight - visibleLeft;
	maxY = visibleBottom - visibleTop;
// !!! don't know where to initialize fido power up stuff
// !!! should have constant somewhere for white
	fido::initColor.setY(219, 0, 255, 0);
	fido::scan.stats = &fido::stats;
}

