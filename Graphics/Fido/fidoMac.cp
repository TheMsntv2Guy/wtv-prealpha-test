#include "fidoMac.h"
#include "fidoSim.h"
#include "fido.h"

#ifdef FIDO_STANDALONE
#include "miscnew.h"
#include "gammaTest.h"
#else
// !!! add webtv interface to mac here
#endif

int windowXSize = 350;
int windowYSize = 300;

#ifndef FIDO_STANDALONE
PixMapHandle fidoMac::pixMapHandle = nil;
#endif

void fidoMac::windowClip(fidoRectangle* clip)
{
	if (clip->left < ff(windowBorder))
		clip->left = ff(windowBorder);
	if (clip->top < ff(windowBorder))
		clip->top = ff(windowBorder);
	if (clip->right > ff(windowXSize))
		clip->right = ff(windowXSize);
	if (clip->bottom > ff(windowYSize))
		clip->bottom = ff(windowYSize);
}

void fidoMac::choose(int windowNumber)
{
#ifdef FIDO_STANDALONE
	if (locked > 0)
		unlock();
	select(windowNumber);
	lock();
#else
	windowNumber++;
// !!! add webtv interface to mac here
#endif
}

int fidoMac::init(char *name, int xo, int yo)
{
#ifdef FIDO_STANDALONE
	int windowNumber = openwindow(windowXSize, windowYSize, xo, yo, name);
	
	choose(windowNumber);
	return windowNumber;
#else
	name++;
	xo++;
	yo++;
// !!! add webtv interface to mac here
	return 0;
#endif
}


void fidoMac::clear(int windowNumber)
{
#ifdef FIDO_STANDALONE
	choose(windowNumber);
	memset(vram, -1, cysize * cxsize * 4);
	unlock();
#else
	windowNumber++;
// !!! add webtv interface to mac here
#endif
}


#ifndef FIDO_STANDALONE
static void MakePixMap(PixMap& pixMap, char* base, int depth, int rowBytes, Rect& bounds)
{
	pixMap.baseAddr = base;
	pixMap.rowBytes = rowBytes | 0x8000;
	pixMap.pixelType = (depth <= 8) ? 0 : 16;
	pixMap.pixelSize = depth;
	pixMap.bounds = bounds;
	DisposeCTable(pixMap.pmTable);
	pixMap.pmTable = (pixMap.pixelSize <= 8) ? GetCTable(depth) : 0;
}
#endif


void fido::copyToFrameBuffer()
{
#ifdef FIDO_STANDALONE
	if (locked <= 0)
		lock();
	int y = currentY - fidoLimits::visibleTop;
	Assert(y >= 0);
	if (y < cysize) {
#if _KEEP_RGB_
		int lineLength = sizeof(scan.cheat) < cxsize * 4 ? sizeof(scan.cheat) : cxsize * 4;
		memcpy(vram+y*vwide, (char*) scan.cheat, lineLength);
#else
		int lineLength = fidoLimits::maxPixels< cxsize ? fidoLimits::maxPixels : cxsize;
		rgbPixel* pixel = (rgbPixel*) (vram+y*vwide);
		for (int count = 0; count < lineLength; count++)
			scan.scanline[count].toRGB(*pixel++);
#endif
	}
#if REFRESH_ON_EVERY_SCAN_LINE
	RefreshMyWindow();
#endif
#else
	static Rect srcRect = {0, 0, 1, fidoLimits::maxPixels};
	if (fidoMac::pixMapHandle && (char*) scan.cheat != (*fidoMac::pixMapHandle)->baseAddr) {
		DisposePixMap(fidoMac::pixMapHandle);
		fidoMac::pixMapHandle = nil;
	}
	if (fidoMac::pixMapHandle == nil) {
		fidoMac::pixMapHandle = NewPixMap();
		MakePixMap(**fidoMac::pixMapHandle, (char*) scan.cheat, 32, fidoLimits::maxPixels * 4, srcRect);
	}
	Rect dest;
	dest.top = y;
	dest.left = 0;
	dest.bottom = y + 1;
	dest.right = fidoLimits::maxPixels;
	GrafPtr curPort; GetPort(&curPort);
	CopyBits((BitMap *)*fidoMac::pixMapHandle, &curPort->portBits, &srcRect, &dest, srcCopy, nil);
#endif
	currentY += 2;
}

void fidoMac::done()
{
#ifdef FIDO_STANDALONE
	RefreshMyWindow();
	unlock();
#else
// !!! add webtv interface to mac here
	if (pixMapHandle) {
		DisposePixMap(pixMapHandle);
		pixMapHandle = nil;
	}
#endif
}

texture* fidoMac::makeYUAVPicture(const char* filename, int& xSize, int& ySize)
{
#ifdef FIDO_STANDALONE
	int offscreen = readpicture(filename);
	if (offscreen < 0) {
		char inFolder[256];
		strcpy(inFolder, ":pictures:");
		strcat(inFolder, filename);
		offscreen = readpicture(inFolder);
	}
	Assert(offscreen >= 0);
	select(offscreen);
	xSize = cxsize;
	ySize = cysize;
	lock();
	texture* pict = newArray(texture, cxsize * cysize);
	Assert(pict);
	texture* pictPtr = pict;
	for(int j=0; j<cysize; j++) {
		for(int i=0; i<cxsize; i++) {
			rgbPixel rgb;
			char *p1=vram+j*vwide+i*4+1;
			rgb.r=*p1++;
			rgb.g=*p1++;
			rgb.b=*p1++;
			yuvPixel yuv;
			rgb.toYUV(yuv);
		// !!! could supply rgb here since we've already got it
			pictPtr++->fullColorPixel.setY(yuv.y, yuv.u, 0xFF /* alpha */, yuv.v);
		}
	}
	unlock();
	freepicture(offscreen);
	return pict;	
#else
	filename, xSize, ySize;
// !!! add webtv interface to mac here
	return 0;
#endif
}

void displayList::simulate()
{
//	fido::vblankTask();
	fido::frameTask(fidoLimits::visibleTop);
//	fido::vblankTask();
	fido::frameTask(fidoLimits::visibleTop + 1);
	fidoMac::done();
}

#ifdef FIDO_STANDALONE
#include <Events.h>

void fidoMac::wait()
{
	while (!Button())
		;
}
#endif

