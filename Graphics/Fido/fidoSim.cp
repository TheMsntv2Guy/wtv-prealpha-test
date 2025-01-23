#include "fido.h"
#include "fidoBitmap.h"
#include "fidoSim.h"
#ifdef FIDO_STANDALONE
#include "miscnew.h"
#endif


#if _SIMULATE_FIDO_

bool scanRecord::runPixel(int pixelIndex)
{
	Assert(this);
	static unsigned char repeatIndex;
	
	if (fido::scan.repeat) {
		if (pixelIndex == 0)
			return true;
		do
			drawYUAV(colors[repeatIndex], fido::globalAlpha);
		while (--pixelIndex >= 0);
		fido::scan.repeat = false;
	} else if (fido::runLengthEncoded && pixelIndex < fido::multipleRunLengthCount) {
		fido::scan.repeat = true;
		repeatIndex = pixelIndex;
	} else {
		drawYUAV(colors[pixelIndex], fido::globalAlpha);
		stats->line[0].pixelDraw++;
	}
	return false;
}


void scanRecord::micro(unsigned char* vector4Ptr, int count)
{
	Assert(this);
	Assert(vector4Ptr);
	int byteRead = 0;
	while (--count >= 0 &&
		fido::scan.runPixel(*vector4Ptr >> 4) == false &&
		fido::scan.runPixel(*vector4Ptr++ & 0x0f) == false)
	{
		byteRead++;
	}
	stats->line[0].longRead += byteRead + 3 >> 2;
}

void scanRecord::mini8(unsigned char* vector8Ptr, int count)
{
	Assert(this);
	Assert(vector8Ptr);
	int byteRead = 0;
	while (--count >= 0 && fido::scan.runPixel(*vector8Ptr++) == false)
		byteRead++;
	stats->line[0].longRead += byteRead + 3 >> 2;
}

// !!! ? many incomplete cases here
void celRecord::scan(fidoStats& stats) const
{
	Assert(this);
	celTypes type = (celTypes) (mode & celTypeMask);
	if (type == specialCelType) {
		switch (mode & specialMask) {
			case loadCodeBook:
				fido::loadCodebookCache(this);
			break;
			case loadYMapBase:
				fido::loadYMapBase(this);
			break;
			case loadYMapMaster:
				fido::loadYMapMaster(this);
			break;
			case loadCelsBase:
				fido::loadCelsBase(this);
			break;
			case loadCelsMaster:
				fido::loadCelsMaster(this);
			break;
			case loadRAMAddressMSB:
				fido::loadRAMAddressMSB(this);
			break;
			case loadROMAddressMSB:
				fido::loadROMAddressMSB(this);
			break;
			default:
				Assert(0);
		}
		return;
	}
	fido::runLengthEncoded = type == vq4MiniRLE || type == vq4MicroRLE || type == vq8MiniRLE;
	fido::backgroundAlpha = (celTypes) (mode & bgAlphaMask);
	codebook color;
	DebugCode(color.type = codebook::yType);
	int height = fido::currentY - fido::topLine;
	Assert(height >= 0 && height <= fidoLimits::physicalBottom);
	fido::scan.currentX = get_xLeftStart();
	int shiftCount = fidoBitMap::shift(type);
	if (fido::runLengthEncoded || type == vq4MicroYUAV) {
		int intLeft = s10dot10::roundUp(fido::scan.currentX);
		fido::scan.intX = intLeft;
		Assert(intLeft >= fidoLimits::visibleLeft && intLeft < fidoLimits::visibleRight);
		fido::scan.repeat = false;
		texture* base;
		fido::scan.colors = fido::codebookCache;
		unsigned char* basePtr = (unsigned char*) (&get(base)->forAggregateInitializer +
			height * (get_textureRowLongs() + 1));
		int count = (get_textureRowLongs() + 1) << 2;
		if (type == vq4MicroRLE || type == vq4MicroYUAV) {
			fido::globalAlpha = 0xFF;
			fido::scan.micro(basePtr, count);
		} else {
			fido::globalAlpha = globalAlpha;
			codebook* codesBase;
			get(codesBase);
			if (codesBase)
				fido::scan.colors = codesBase;
		// !!! doesn't support xRightStart; assume we don't have to
			if (type == vq4MiniRLE)
				fido::scan.micro(basePtr, count);
			else if (type == vq8MiniRLE)
				fido::scan.mini8(basePtr, count);
			else
				Assert(0);
		}
		return;
	}
	signed10dot10 right = get_xRightStart();
	Assert(fido::scan.currentX <= right);	// !!! should probably be currentX < right, but for now, allow empty scans
	if ((type & miniCelType) == 0) 
	{	// big cel
	// !!! to describe pixel centers, these two calcs. need to add 1/2 (make sure numbers are prerounded)
		fido::scan.currentX += dxLeft * height;
		right += dxRight * height;	
		Assert(fido::scan.currentX <= right);	// !!! should probably be currentX < right, but for now, allow empty scans
		fido::scan.workingU = uStart + duRowAdjust * height;
		fido::scan.workingV = vStart + dvRowAdjust * height;
		fido::scan.workingW = wStart + dwRowAdjust * height;
		DebugCode(signed16dot16 realU = (fido::scan.workingU << 8) / fido::scan.workingW);
		DebugCode(signed16dot16 realV = (fido::scan.workingV << 8) / fido::scan.workingW);
	} else {
		fido::scan.workingU = 0;
		fido::scan.workingV = get_dux() * height;
		fido::scan.workingW = u0dot16::one;
	}
	int intLeft = s10dot10::roundUp(fido::scan.currentX);
	fido::scan.intX = intLeft;
	int intRight = s10dot10::roundDown(right);	// subtract 1 so that 1/2 is not enclosed on right
	short currentY = fido::currentY;	// ! Codewarrior 7 won't let me set a breakpoint
	Assert(fido::currentY >= fidoLimits::visibleTop && fido::currentY <= fidoLimits::visibleBottom);
	while (fido::scan.intX < intRight) {	// use pixel center rule
		Assert(fido::scan.intX >= fidoLimits::visibleLeft && fido::scan.intX <= fidoLimits::visibleRight);
// int u and v are truncated, not rounded, becauses pixels are defined by their centers, not the nearest grid line
// for instance, the values .1 and .9 still mean pixel 0, the .9 does not round up and mean pixel 1
		int intU = s8dot8::divide0dot16(fido::scan.workingU, fido::scan.workingW) >> s8dot8::shift;
		Assert(intU >= -128 && intU <= 127);
		int intV = s8dot8::divide0dot16(fido::scan.workingV, fido::scan.workingW) >> s8dot8::shift;
		Assert(intV >= -128 && intV <= 127);
		DebugCode(currentY ^= 1; currentY ^= 1; intLeft ^= 1; intLeft ^= 1);	// ! Codewarrior 7 won't let me set a breakpoint
		if ((type & miniCelType) == 0) {
			intU &= uMask;
			intV &= vMask;
		}
		bool do2by2 = type == vq8FullYUYV || type == vq8MiniYUYV;
		int offset = (intV & ~do2by2) * (get_textureRowLongs() + 1);
		offset += intU >> shiftCount;
		codebook codeColor;
		texture* base;
		codebook* codesBase;
		get(base);
		if (shiftCount >= 2) {		// 4 or 8 bit
			unsigned long pixel = base[offset].forAggregateInitializer;
			stats.line[0].longRead++;
			if (shiftCount == 3) 
				pixel = pixel >> (intU & 7 & ~do2by2) & 0x0F;	// lookup using nybble
			else
				pixel = pixel >> (intU & 3 & ~do2by2) & 0xFF;	// lookup using byte
			if (do2by2) {
				pixel <<= 1;	// codebook uses 64 bits/entry
				if (intV & 1)
					pixel += 1;	// choose odd entry
			}
			get(codesBase);
			Assert(codesBase != nil || shiftCount == 3);
			if (codesBase == nil)
				codeColor = fido::codebookCache[pixel];
			else {
				codeColor = codesBase[pixel];
				stats.line[0].longRead++;
			}
		} else {
			codeColor = base[offset].fullColorPixel;
			stats.line[0].longRead++;
		}
		 if (do2by2 || type == directFullYUYV || type == directMiniYUYV) {
			CheapAssert(codeColor.type == codebook::yyType);
			color.y.y = intU & 1 ? codeColor.yy.y1 : codeColor.yy.y2;
			color.y.u = codeColor.yy.u;
			color.y.a = 0xFF;
			color.y.v = codeColor.yy.v;
#if _KEEP_RGB_
			color.rgb1 = intU & 1 ? codeColor.rgb1 : codeColor.rgb2;
#endif
		} else { // yuav
// this assert detects walking outside of the bitmap
// !!! cheat for now						Assert(codeColor.type == codebook::yuavType);
			color.y.y = codeColor.y.y;
			color.y.u = codeColor.y.u;
			color.y.a = codeColor.y.a;
			color.y.v = codeColor.y.v;
#if _KEEP_RGB_
			color.rgb1 = codeColor.rgb1;
#endif
		}
		if ((type & miniCelType) == 0) {
			fido::scan.workingU += get_dux();
			fido::scan.workingV += dvx;
			fido::scan.workingW += dwx;
		} else
			fido::scan.workingU += get_dux();
		fido::scan.drawYUAV(color, globalAlpha);	// advances intX
	} // end while pixel
}


void fido::frameTask(int yStart)
{
	currentY = yStart;
	Assert(master.yMap);
	Assert(master.celBase);
	do {
		memset(&fido::stats.line[0], 0, sizeof(fido::stats.line[0]));
		working.celBase = master.celBase;
		const yEntry* currentYMapPtr = working.yMap = master.yMap;
		yMapIndex = 1;
	// !!! what about the overscan color?
		yuvPixel fill;
		fill.y = initColor.y.y;	fill.u = initColor.y.u;	fill.v = initColor.y.v;
		for (int counter = 0; counter < sizeof(scan.scanline)/sizeof(scan.scanline[0]); ++counter)
			scan.scanline[counter] = fill;
#if _KEEP_RGB_
		for (int counter = 0; counter < sizeof(scan.cheat)/sizeof(scan.cheat[0]); ++counter)
			scan.cheat[counter] = initColor.rgb1;
#endif
		while (currentYMapPtr->terminate() == false) {
			int bottomLine;
			yEntry::hint celHintSize;
			int index;
			currentYMapPtr->get(bottomLine, topLine, celHintSize, index);
			stats.line[0].yMapFetch++;
			if (currentY >= topLine && currentY < bottomLine) {
				do {
					stats.line[0].celFetch++;
					celRecord* cel = working.get(index);
					if (currentY - topLine >= cel->topOffset && bottomLine - currentY >= cel->bottomOffset) {
						stats.line[0].celScan++;
						cel->scan(stats);
					}
					if (cel->testLastCelBit() != false)
						break;
					index += cel->indexSize();
				} while (1);
			}
			currentYMapPtr = &working.yMap[yMapIndex];
			yMapIndex++;
		}
		Assert(currentY - fidoLimits::visibleTop >= 0);
		Assert(currentY - fidoLimits::visibleTop + 1 < sizeof(stats.line)/sizeof(stats.line[0]));
		stats.line[currentY - fidoLimits::visibleTop + 1] = stats.line[0];
		copyToFrameBuffer();	// advances currentY by 2
	} while (currentY < fidoLimits::visibleBottom);
}


// data that is inside FIDO
codebook fido::codebookCache[16]; // preloaded codebook in Fido
codebook fido::initColor;
int fido::yMapIndex;
unsigned char fido::globalAlpha;
enum celRecord::celTypes fido::backgroundAlpha;
bool fido::runLengthEncoded;
short fido::currentY;
int fido::topLine;		// last top value read from yMap
unsigned char fido::ramAddressMSB;
unsigned char fido::romAddressMSB;
scanRecord fido::scan;
fidoStats fido::stats;
fidoData fido::working;
fidoData fido::master;

#endif // _SIMULATE_FIDO_

