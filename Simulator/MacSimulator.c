// ===========================================================================
//	MacSimulator.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#include <palettes.h>
#include <video.h>

#include "LEDWindow.h"
#include "MacSimulator.h"
#include "MacintoshDialogs.h"
#include "MacintoshUtilities.h"
#include "MemoryManager.h"
#include "MiscStatWindow.h"
#include "ObjectStore.h"
#include "PerfDump.h"
#include "Screen.h"
#include "Simulator.rsrc.h"
#include "SimulatorWindow.h"

MacSimulator* gMacSimulator;

// ===========================================================================
//	Preferences

const char kPrefsFileName[]				= "WebTV Preferences";

const char kPrefsFileVersion[]			= "Preference file version number";
const char kPrefsPreventComplains[]		= "Prevent Complains";
const char kPrefsUseMacFlickerFilter[]	= "Use Mac Flicker Filter";
const char kPrefsUseJapanese[]			= "Use Japanese";
const char kPrefsUseQuickdraw[]			= "Use Quickdraw";
const char kPrefsUseTVWDEF[]			= "Use TV WDEF";

//  To force everyone to change a preferences, bump this and add a new case
const ulong kDefaultPrefsFileVersion = 4;

// ===========================================================================
//	MacSimulator::GetXXX defaults

const Boolean kDefaultUseMacFlickerFilter	= true;
const Boolean kDefaultUseQuickdraw			= false;
const Boolean kDefaultUseTVWDEF				= true;

// ===========================================================================

#ifndef __BOX_UTILITIES__
#include "BoxUtils.h"
#endif

ulong GetBoxLEDs(void)
{
	return gMacSimulator->GetBoxLEDs();
}

void SetBoxLEDs(ulong bits)
{
	gMacSimulator->SetBoxLEDs(bits);
}

// ===========================================================================
// MacSimulator

MacSimulator::MacSimulator(void)
{
	if (gMacSimulator != nil)
		Complain(("Tried to create two MacSimulators!"));
	
	gMacSimulator = this;
	SetUseMacFlickerFilter(kDefaultUseMacFlickerFilter);
	SetUseQuickdraw(kDefaultUseQuickdraw);
	SetUseTVWDEF(kDefaultUseTVWDEF);
	fIsProfiling = false;
}

MacSimulator::~MacSimulator(void)
{
	SavePreferences();
	gMacSimulator = nil;
}

const char*
MacSimulator::GetClientSerialNumber() const
{
	static char number[128];
	
	if (*number == 0) {
		Handle handle;
		long count;
		
		if ((handle = GetResource('STR ', -16096)) != nil) {
			count = MAX(**handle, sizeof (number-1));
			CopyMemory((*handle)+1, number, count);
			number[count] = 0;
			RemoveCharacters(number, " ");
		} else
			strcpy(number, "demo");
	}
	
	return number;
}

void MacSimulator::LoadPreferences()
{
	// open preferences for a bit
	OpenPreferences();

	// update preferences, if need be
	UpdatePreferences();

	// suck out the prefs
	Boolean booleanValue;
#ifdef DEBUG
	if (GetPreferenceBoolean(kPrefsPreventComplains, &booleanValue))	SetPreventComplains(booleanValue);
#endif
	if (GetPreferenceBoolean(kPrefsUseMacFlickerFilter, &booleanValue))	SetUseMacFlickerFilter(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseFlickerFilter, &booleanValue))	SetUseFlickerFilter(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseJapanese, &booleanValue))			SetUseJapanese(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseQuickdraw, &booleanValue))		SetUseQuickdraw(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseTVWDEF, &booleanValue))			SetUseTVWDEF(booleanValue);

	// get generic simulator prefs
	Simulator::LoadPreferences();

	// and close the file
	ClosePreferences();
}

void MacSimulator::SavePreferences()
{
	// open preferences for a bit
	OpenPreferences();

	// write out the preferences
#ifdef DEBUG
	SetPreferenceBoolean(kPrefsPreventComplains, GetPreventComplains());
#endif
	SetPreferenceBoolean(kPrefsUseMacFlickerFilter, GetUseMacFlickerFilter());
	SetPreferenceBoolean(kPrefsUseJapanese, GetUseJapanese());
	SetPreferenceBoolean(kPrefsUseQuickdraw, GetUseQuickdraw());
	SetPreferenceBoolean(kPrefsUseTVWDEF, GetUseTVWDEF());

	// write generic simulator prefs
	Simulator::SavePreferences();

	// and close the file
	ClosePreferences();
}

// ---------------------------------------------------------------------------
//	Getters/Setters
// ---------------------------------------------------------------------------

void MacSimulator::SetBoxLEDs(ulong bits)
{
	fBoxLEDs = bits;
#ifdef DEBUG_LEDWINDOW
	if (gLEDWindow != nil)
	{	gLEDWindow->SetBoxLEDs(bits);
	}
#endif /* DEBUG_LEDWINDOW */
}


void MacSimulator::SetUseMacFlickerFilter(Boolean enabled)
{
	CntrlParam *paramBlock;
	OSErr myerr;
	unsigned char foo[16];
	GDHandle theGD=GetDeviceList();


	fUseMacFlickerFilter = enabled;

	while (theGD)
		{
		if (((*theGD)->gdRect.bottom - (*theGD)->gdRect.top) == 480)
			{
			paramBlock = (CntrlParam *)NewPtrClear(sizeof(CntrlParam));
			paramBlock->ioCRefNum = (*theGD)->gdRefNum;
			paramBlock->csCode = 0x18;
			paramBlock->csParam[0] = (short)((long)&foo>>16);
			paramBlock->csParam[1] = (short)&foo;
			foo[0]=0x00;
			foo[1]=0x00;
			foo[2]=0x00;
			foo[3]=0x04;
			foo[4]=0x00;
		//	foo[5]=0x82;
			foo[5]=0x81;
			foo[6]=0x00;
			foo[7]=0x00;
			foo[8]=0x00;
			foo[9]=0x00;
			foo[10]=0x00;
			foo[11]=0x00;
			foo[12]=0x00;
		//	foo[13]= enabled ? 0x01 : 0x02;
			foo[13]= enabled ? 0x01 : 0x00;
			foo[14]=0x00;
			foo[15]=0x00;
			myerr= PBControl((ParmBlkPtr) paramBlock,false);
			DisposePtr((Ptr)paramBlock);
			}
		theGD = GetNextDevice(theGD);
		}
}

void MacSimulator::SetUseQuickdraw(Boolean flag)
{
	if (fUseQuickdraw && (!flag))
	{
		SetFontProportional(kDefaultFontProportional);
		SetFontMonospaced(kDefaultFontMonospaced);
	}
	fUseQuickdraw = flag;
}

void MacSimulator::SetUseTVWDEF(Boolean flag)
{
	fUseTVWDEF = flag;
	if (gSimulatorWindow != nil)
	{	gSimulatorWindow->SetUseTVWDEF(flag);
	}
}

Boolean MacSimulator::GetCanUseMacFlickerFilter()
{
	GDHandle theGD=GetDeviceList();

	while (theGD)
		{
		if (((*theGD)->gdRect.bottom - (*theGD)->gdRect.top) == 480)
			{
			if ( (*(*theGD)->gdPMap)->pixelSize == 16 )
				return true;
			}
		theGD = GetNextDevice(theGD);
		}
	return false;
}



// ---------------------------------------------------------------------------
//	Operations
// ---------------------------------------------------------------------------

void MacSimulator::CheckAbort()
{	
	/* abort if command period is pressed */
	const kCommandPeriodMask = 0x808000;
	
	KeyMap theKeys;
	GetKeys(theKeys);

	if (((theKeys)[1] & kCommandPeriodMask) == kCommandPeriodMask)
		ExitToShell();
}

//void MacSimulator::DrawWindow()
//{
//	Rect theRect;
//	short familyNumber;
//	StringPtr displayStr = "\pStandby for Web TV...";
//	WindowPtr window = GetWindow();
//
//	SetPort(window);
//	theRect = window->portRect;
//
//	if (GetIsOn())
//	{
//		SetGray(192);
//		PaintRect(&theRect);
//	}
//	else
//	{
//		SetGray(0);
//		PaintRect(&theRect);
//		
//		PenNormal();
//		MoveTo((theRect.left + theRect.right - StringWidth(displayStr) - 120) / 2, (theRect.top + theRect.bottom) / 2);
//		GetFNum("\pBrush Script", &familyNumber);
//		if (familyNumber != 0) {
//			TextMode(srcXor);
//			TextFont(familyNumber);
//			TextSize(24);
//			DrawString(displayStr);
//		}
//	}
//}

void MacSimulator::EmergencyExit(Error error)
{
	switch (error)
	{
		case kLowMemory:
			::EmergencyExit(sLowMemory);
			break;
		
		default:
			Trespass();
			break;
	}
}

void MacSimulator::ForceUpdate()
{
	WindowPtr window = GetWindow();
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(window);
	InvalRect(&(window->portRect));
	SetPort(savePort);
}

void MacSimulator::ForceUpdateAll()
{
	WindowPeek	window;
	GrafPtr savePort;
	GetPort(&savePort);
	
	for (window = (WindowPeek)FrontWindow(); window != nil; window = window->nextWindow)
	{	if ((window->refCon != rSimulatorWindow) && window->visible)
		{
			SetPort(&(window->port));
			InvalRect(&(window->port.portRect));
		}
	}
		
	SetPort(savePort);
}

/*
	The ROM Filesystem is normally present in the upper part of the ROM.  It is built upside-down, e.g., 
	it grows from the top of the ROM towards the bottom.
	
	The uppermost part of the filesystem holds the nodes.  The data is right under them.
	
	The root node is at the very top of the ROM.  root->first will point at the second node, which
	is right under the root node, sizeof(FSNode) bytes away.  So, (root->first + sizeof(FSNode)) will
	tell us what size ROM this filesystem was built for (ROMTop).
	
	ROMTop - the # of bytes in the ROM FS = the base of the ROM FS in a real ROM. (ROMFSBase)
	
	(An address in the ROM FS - ROMFSBase) + Simulator ROM FS Base = the normalized address in 
	the simulator ROM FS.
	
 */

static ulong FileSize(short vRefNum, long dirID, Str255 fileName)
{
	// Note: Mac-specific code
	
	HParamBlockRec	fi;

	fi.fileParam.ioCompletion = nil;
	fi.fileParam.ioNamePtr = fileName;
	fi.fileParam.ioVRefNum = vRefNum;
	fi.fileParam.ioDirID = dirID;
	fi.fileParam.ioFDirIndex = 0;
	(void)PBHGetFInfo(&fi, false);
	
	return fi.fileParam.ioFlLgLen;
}

static void NormalizeNode(FSNode* node, ulong simbase, ulong romfsbase)
{
	for(;;)
	{
		if (node->data != nil)
			node->data = (char *)((ulong)node->data - romfsbase) + simbase; 

		if (node->parent != nil)
			node->parent = (FSNode *)(((ulong)node->parent - romfsbase) + simbase); 

		if (node->first != nil)
		{ 
			node->first = (FSNode *)(((ulong)node->first - romfsbase) + simbase); 
			NormalizeNode(node->first, simbase, romfsbase);
		}
		
		if (node->next == nil)
			break;
			
		node->next = (FSNode *)(((ulong)node->next - romfsbase) + simbase); 
		node = node->next;
	}
}

static void NormalizeStore(FSNode* storeroot, ulong storesize, char *simbase)
{
	ulong	romtop, romfsbase;

	Assert(storeroot->first != nil);
	
	romtop = (ulong)storeroot->first + 2*sizeof(FSNode);
	
	romfsbase = romtop - storesize;
	
	NormalizeNode(storeroot, (ulong)simbase, romfsbase);
}

static void*	gROMStoreAddress = nil;
static void*	gROMFilesystemRoot = nil;

void* MacSimulator::GetROMFilesystemRoot()
{
	short			refNum;
	OSErr			err;
	long			count;
		
	if (gROMStoreAddress != nil) 
		return gROMStoreAddress;

	// try for generate one first, then checked in one
	count = FileSize(0, 0, "\p:ROM Store");
	if ((err = FSOpen("\p:ROM Store", 0, &refNum)) != noErr)
	{
		count = FileSize(0, 0, "\p:Content:ROM Store");
		if ((err = FSOpen("\p:Content:ROM Store", 0, &refNum)) != noErr)
		{
			Complain(("Cannot open ROM Object Store (error %d)\n", err));
			return nil;
		}
	}
	gROMStoreAddress = NewSimulatorMemory(count);
	
	if (gROMStoreAddress == nil) {
		Message(("Cannot allocate %d bytes (err %d)", count, (int)err));
		EmergencyExit(kLowMemory);
	}

	(void)FSRead(refNum, &count, gROMStoreAddress);
	(void)FSClose(refNum);
	
	gROMFilesystemRoot = (void*)((ulong)gROMStoreAddress + count - sizeof(FSNode));
	
	NormalizeStore((FSNode *)gROMFilesystemRoot, count, (char *)gROMStoreAddress);
	
	return gROMFilesystemRoot;
}

void MacSimulator::DisposeROMFileSystemRoot()
{
	if (gROMStoreAddress) {
		DisposeSimulatorMemory((char*)gROMStoreAddress);
	}
	gROMStoreAddress = nil;
	gROMFilesystemRoot = nil;
}

Boolean MacSimulator::GoTo(short resID)
{
	WindowPtr window = GetWindow();
	
	if ((fPictHdl = Get1Resource('PICT', resID)) == nil)
		return false;

	if ((fHitRectHdl = (Rect **)Get1Resource('rct#', resID)) == nil)
		return false;

	if ((fNextPictsHdl = (short **)Get1Resource('npct', resID)) == nil)
		return false;
	
	SetPort(window);
	InvalRect(&((WindowPeek)window)->port.portRect);
	
	return true;
}

//Boolean MacSimulator::HandleUpdate()
//{
//	if (gScreen != nil)
//	{
//		UpdateMacPort();
//		return true;
//	}
//
//	return false;
//
//}

void MacSimulator::PostPendingPowerOn()
{
	fPendingPowerOn = true;
	PostEvent(nullEvent, 0);
}

void MacSimulator::PowerOff()
{
	PerfDumpMark("PowerOff");
	PowerOffVisualEffect();
	ForceUpdateAll();
	Simulator::PowerOff();
}

void MacSimulator::PowerOn()
{
	PerfDumpMark("PowerOn");
	static Boolean	gAlreadyInitializedOnce = false;
	
	SetPort(GetWindow());
	PowerOnVisualEffect();
	
	if (!gAlreadyInitializedOnce)
	{
		Initialize();
		gAlreadyInitializedOnce = true;
	}
	fIsInitialized = true;

	System::PowerOn();
	ForceUpdateAll();
}

void MacSimulator::PowerOffVisualEffect()
{
	Rect* portRect = &(GetWindow()->portRect);
	short i;
	short windowWidthDiv2 = (portRect->right - portRect->left + 1) / 2;
	short windowHeightDiv2 = (portRect->bottom - portRect->top + 1) / 2;
	short windowHMid = (portRect->right + portRect->left + 1) / 2;
	short windowVMid = (portRect->bottom + portRect->top + 1) / 2;
	Rect paintRect1, paintRect2;
	
	SetPort((GrafPtr)fWindow);
	HideCursor();
	
	paintRect1 = *portRect;
	paintRect1.bottom = portRect->top + 1;
	paintRect2 = *portRect;
	paintRect2.top = portRect->bottom - 1;

	for (i = 0; i < windowHeightDiv2 - 1; i++)
	{
		PaintRect(&paintRect1);
		PaintRect(&paintRect2);
		if ((i % 16) == 0)
			DelayTicks(1);
		paintRect1.bottom++; paintRect1.top++;
		paintRect2.bottom--; paintRect2.top--;
	}
	
	DelayTicks(10);

	paintRect1 = *portRect;
	paintRect1.right = portRect->left + 1;
	paintRect2 = *portRect;
	paintRect2.left = portRect->right - 1;

	for (i = 0; i < windowWidthDiv2; i++)
	{
		PaintRect(&paintRect1);
		PaintRect(&paintRect2);
		if ((i % 16) == 0)
			DelayTicks(2);
		paintRect1.right++; paintRect1.left++;
		paintRect2.right--; paintRect2.left--;
	}

	DelayTicks(20);

	ShowCursor();
	InvalRect(portRect);
}

void MacSimulator::ShowStartupScreen()
{
	Simulator::ShowStartupScreen();
	ForceUpdate();
}

void MacSimulator::UserLoopIteration()
{
	PerfDumpEnter("Mac::UserLoop");

#ifdef DEBUG_MISCSTATWINDOW
	MiscStatWindow::Roundtrip();
#endif
	SetPort(GetWindow());
	Simulator::UserLoopIteration();

	PerfDumpExit("Mac::UserLoop");
}


static void CenterRect(Rect *r, const Rect* inRect)
{
	Ordinate width = r->right - r->left;
	Ordinate height = r->bottom - r->top;
	Ordinate dH = ((inRect->right-inRect->left) - width) / 2;
	Ordinate dV = ((inRect->bottom-inRect->top) - height) / 2;
	r->left = inRect->left + dH; r->right = r->left + width;
	r->top = inRect->top + dV; r->bottom = r->top + height;
}

static Boolean DrawGarbage(CGrafPtr port, const Rect boundsRect)
{
	Assert((port->portVersion & 0xC000) == 0xC000);	// must truly be a CGrafPtr
	
	Assert((gScreenDevice.rowBytes & 0x3) == 0);

	// only do effect if squarely on a 16-bit device
	LocalToGlobal((MacPoint*)&boundsRect.top);
	LocalToGlobal((MacPoint*)&boundsRect.bottom);
	
	Rect	portRect = port->portRect;
	LocalToGlobal((MacPoint*)&portRect.top);
	LocalToGlobal((MacPoint*)&portRect.bottom);
	GDHandle	deepestDevice = GetMaxDevice(&portRect);
	if (deepestDevice == nil)
		return false;
	Assert(((*(*deepestDevice)->gdPMap)->rowBytes & 0x3) == 0);
	if ((*(*deepestDevice)->gdPMap)->pixelSize != 16)
		return false;
	if ((*deepestDevice)->gdRect.top > boundsRect.top)
		return false;
	if ((*deepestDevice)->gdRect.left > boundsRect.left)
		return false;
	if ((*deepestDevice)->gdRect.bottom < boundsRect.bottom)
		return false;
	if ((*deepestDevice)->gdRect.right < boundsRect.right)
		return false;
		
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort((GrafPtr)port);
	
	ClipRect(&boundsRect);
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	ulong		dstRowBytes = (*(*deepestDevice)->gdPMap)->rowBytes & 0x7FFF;
	ulong*		dstBase = (ulong*)((*(*deepestDevice)->gdPMap)->baseAddr
					- ((*deepestDevice)->gdRect.top*dstRowBytes)
					- ((*deepestDevice)->gdRect.left<<1) + (boundsRect.top*dstRowBytes) + (boundsRect.left<<1));
	ulong		longCount = (boundsRect.right - boundsRect.left)>>1;
	ulong		dstRowBump = (dstRowBytes>>2) - longCount;

	ulong*	dstLong = dstBase;
	long		seed = (long)Random() + TickCount();

	long		rowsRemaining = boundsRect.bottom - boundsRect.top;
	while (--rowsRemaining >= 0)
	{
		long		longsRemaining = longCount;	/* two half-words in 16-bit mode */
		while (--longsRemaining >= 0)
		{	*dstLong++ = seed; seed *= 57; }
	
		dstLong += dstRowBump;
	}
	
	ValidRect(&boundsRect);		// assumes the current port is a window

	ClipRect(&port->portRect);
	SetPort(savePort);
	
	return true;
}

static Boolean FadeToBlack(CGrafPtr port, const Rect boundsRect)
{
	Assert((port->portVersion & 0xC000) == 0xC000);	// must truly be a CGrafPtr
	
	Assert((gScreenDevice.rowBytes & 0x3) == 0);

	// only do effect if squarely on a 16-bit device
	LocalToGlobal((MacPoint*)&boundsRect.top);
	LocalToGlobal((MacPoint*)&boundsRect.bottom);
	
	Rect	portRect = port->portRect;
	LocalToGlobal((MacPoint*)&portRect.top);
	LocalToGlobal((MacPoint*)&portRect.bottom);
	GDHandle	deepestDevice = GetMaxDevice(&portRect);
	if (deepestDevice == nil)
		return false;
	Assert(((*(*deepestDevice)->gdPMap)->rowBytes & 0x3) == 0);
	if ((*(*deepestDevice)->gdPMap)->pixelSize != 16)
		return false;
	if ((*deepestDevice)->gdRect.top > boundsRect.top)
		return false;
	if ((*deepestDevice)->gdRect.left > boundsRect.left)
		return false;
	if ((*deepestDevice)->gdRect.bottom < boundsRect.bottom)
		return false;
	if ((*deepestDevice)->gdRect.right < boundsRect.right)
		return false;
		
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort((GrafPtr)port);
	
	ClipRect(&boundsRect);
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	ulong		dstRowBytes = (*(*deepestDevice)->gdPMap)->rowBytes & 0x7FFF;
	ulong*		dstBase = (ulong*)((*(*deepestDevice)->gdPMap)->baseAddr
					- ((*deepestDevice)->gdRect.top*dstRowBytes)
					- ((*deepestDevice)->gdRect.left<<1) + (boundsRect.top*dstRowBytes) + (boundsRect.left<<1));
	ulong		longCount = (boundsRect.right - boundsRect.left)>>1;
	ulong		dstRowBump = (dstRowBytes>>2) - longCount;
	ulong*		dstLong = dstBase;

	long		rowsRemaining = boundsRect.bottom - boundsRect.top;
	for (int i = 0; i < 50; i++)
		while (--rowsRemaining >= 0)
		{
			long		longsRemaining = longCount;	/* two half-words in 16-bit mode */
			while (--longsRemaining >= 0)
			{
				ulong	tmp = *dstLong/2 + 0x80018001;
				if (tmp < *dstLong)
					tmp = 0xFFFFFFFF;
				*dstLong++ = tmp;
			}
		
			dstLong += dstRowBump;
		}
	
	ValidRect(&boundsRect);		// assumes the current port is a window

	ClipRect(&port->portRect);
	SetPort(savePort);
	
	return true;
}

void MacSimulator::PowerOnVisualEffect()
{
	if (!GetInDemoMode())
 		return;

	WindowPtr simulatorWindowPtr = GetWindow();

	Rect* portRect = &(simulatorWindowPtr->portRect);
	short windowWidthDiv2 = (portRect->right - portRect->left + 1) / 2;
	short windowHeightDiv2 = (portRect->bottom - portRect->top + 1) / 2;
	short windowHMid = (portRect->right + portRect->left + 1) / 2;
	short windowVMid = (portRect->bottom + portRect->top + 1) / 2;
	Rect eraseRect1;
	RGBColor grayColor = { 80*256, 80*256, 80*256 };
	RGBColor blackColor = { 65536, 65536, 65536 };
	
	SetPort((GrafPtr)simulatorWindowPtr);
	PaintRect(portRect);
	
	eraseRect1.top = windowVMid;
	eraseRect1.left = windowHMid;
	eraseRect1.bottom = windowVMid;
	eraseRect1.right = windowHMid;
	
	HideCursor();
	
	RGBBackColor(&grayColor);
	
	short i;
	for (i = 0; i < windowWidthDiv2; i++)
	{
		if (!DrawGarbage((CGrafPtr)simulatorWindowPtr, eraseRect1))
			EraseRect(&eraseRect1);
		if ((i % 32) == 0)
			DelayTicks(1);
		eraseRect1.left--; eraseRect1.right++;
	}
	
	for (i = 0; i < windowHeightDiv2; i++)
	{
		if (80 + i/4 <= 198)
		{
			grayColor.red = 256 * (80 + i/4);
			grayColor.green = 256 * (80 + i/4);
			grayColor.blue = 256 * (80 + i/4);
			RGBBackColor(&grayColor);
		}
		eraseRect1.top--; eraseRect1.bottom++;
		if (!DrawGarbage((CGrafPtr)simulatorWindowPtr, eraseRect1))
			EraseRect(&eraseRect1);
	}
	TextMode(srcXor);
	SetPort((GrafPtr)simulatorWindowPtr);
	for ( i = 0; i < 40; i++)
		DrawGarbage((CGrafPtr)simulatorWindowPtr, eraseRect1);

	PicHandle	hdl = GetPicture(rStartupPicture);
	Rect		frame;
	if (hdl != nil)
	{
		frame = (*hdl)->picFrame;
		CenterRect(&frame, &eraseRect1);
	}
	
	for ( i = 0; i < 130; i++)
	{
		DrawGarbage((CGrafPtr)simulatorWindowPtr, eraseRect1);
		if (hdl != nil)
			DrawPicture(hdl, &frame);
	}
		
	FadeToBlack((CGrafPtr)simulatorWindowPtr, eraseRect1);
	PaintRect(&eraseRect1);
	ShowCursor();
}

// ===========================================================================


// ---------------------------------------------------------------------------
// Preference methods...for reading/writing preferences
//
//	To do:  close resource file when we're not using it -or-
//			make a copy of the resource file, use it, and copy back when done
// ---------------------------------------------------------------------------

Boolean
MacSimulator::GetPreference(const char* name, void** buffer, size_t* bufLength) const
{
	Boolean result = false;
	Boolean closeWhenDone = false;

	if (fResFileID == -1)
	{	
		MacSimulator* unConstThis = (MacSimulator*)this;
		unConstThis->OpenPreferences();
		closeWhenDone = true;
	}
	
	if (fResFileID != -1)	// is there an open preferences file?
	{
		short oldResFileID = CurResFile();
		UseResFile(fResFileID);
		Str255 pascalName;
		snprintf((char*)pascalName, sizeof(pascalName),
				"%.*s", sizeof(pascalName)-1, name);
		c2pstr((char*)pascalName);
		Handle prefHandle = GetNamedResource(kPreferencesResourceType, pascalName);
		
		if (prefHandle == nil)
		{
			//Complain(("Couldn't get value for preference named \"%s\".", name));
		}
		else
		{
			LoadResource(prefHandle);
			OSErr err = ResError();
			if (err != noErr)
			{
				Complain(("Error loading resource for preference named \"%s\".", name));
			}
			else
			{
				if (bufLength != nil)
				{	*bufLength = GetHandleSize(prefHandle);
				}
				*buffer = *prefHandle;
				result = true;
			}
		}
		
		UseResFile(oldResFileID);
		if (closeWhenDone)
		{	MacSimulator* unConstThis = (MacSimulator*)this;
			unConstThis->ClosePreferences();
		}
	}
	return result;
}

Boolean
MacSimulator::GetPreferencePoint(const char* name, struct MacPoint* value) const
{
	struct MacPoint* prefPtr = nil;
	size_t size = sizeof(struct MacPoint);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(struct MacPoint));
	*value = *prefPtr;
	return true;
}
Boolean
MacSimulator::GetPreferenceRect(const char* name, struct Rect* value) const
{
	struct Rect* prefPtr = nil;
	size_t size = sizeof(struct Rect);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(struct Rect));
	*value = *prefPtr;
	return true;
}

Boolean
MacSimulator::RemovePreference(const char* name)
{
	Boolean result = false;
	Boolean closeWhenDone = false;

	if (fResFileID == -1)
	{	OpenPreferences();
		closeWhenDone = true;
	}
	
	if (fResFileID != -1)	// is there an open preferences file?
	{
		short oldResFileID = CurResFile();
		UseResFile(fResFileID);
		Str255 pascalName;
		snprintf((char*)pascalName, sizeof(pascalName),
				 "%.*s", sizeof(pascalName)-1, name);
		c2pstr((char*)pascalName);
		Handle oldPrefHandle = GetNamedResource(kPreferencesResourceType, pascalName);
		if (oldPrefHandle != nil)
		{	RemoveResource(oldPrefHandle);
		}
		result = true;
		UseResFile(oldResFileID);
		if (closeWhenDone)
		{	ClosePreferences();
		}
	}
	if (!result)
	{	Complain(("MacSimulator::RemovePreference() was unable to remove preference \"%s\"", name));
	}
	return result;
}

Boolean
MacSimulator::SetPreference(const char* name, const void* buffer, size_t bufLength)
{
	Boolean result = false;
	Boolean closeWhenDone = false;
	
	if (fResFileID == -1)
	{	OpenPreferences();
		closeWhenDone = true;
	}
	
	if (fResFileID != -1)	// is there an open preferences file?
	{
		short oldResFileID = CurResFile();
		UseResFile(fResFileID);
		Str255 pascalName;
		snprintf((char*)pascalName, sizeof(pascalName),
				"%.*s", sizeof(pascalName)-1, name);
		c2pstr((char*)pascalName);
		Handle oldPrefHandle = GetNamedResource(kPreferencesResourceType, pascalName);
		short theID;
		
		if (oldPrefHandle == nil)
		{
			theID = UniqueID(kPreferencesResourceType);
		}
		else
		{
			// use theID from old preference resource
			Str255 oldName;
			ResType oldType;
			GetResInfo(oldPrefHandle, &theID, &oldType, oldName);
			Assert(oldType == kPreferencesResourceType);
			RemoveResource(oldPrefHandle);
		}
		Handle newPrefHandle = NewHandle(bufLength);
		if (newPrefHandle == nil)
		{
			Complain(("Unable to allocate handle for preference of size %d", bufLength));
		}
		else
		{	memcpy(*newPrefHandle, buffer, bufLength);
			AddResource(newPrefHandle, kPreferencesResourceType, theID, pascalName);
			if (ResError() == noErr)
				result = true;
		}
		UseResFile(oldResFileID);
		if (closeWhenDone)
		{	ClosePreferences();
		}
	}
	if (!result)
	{	Complain(("MacSimulator::SetPreference() was unable to set preference \"%s\"", name));
	}
	return result;
}

Boolean
MacSimulator::SetPreferencePoint(const char* name, struct MacPoint* value)
{	return SetPreference(name, value, sizeof(struct MacPoint));
}

Boolean
MacSimulator::SetPreferenceRect(const char* name, struct Rect* value)
{	return SetPreference(name, value, sizeof(struct Rect));	
}

// ---------------------------------------------------------------------------


void MacSimulator::UpdatePreferences()
{
	ulong prefsFileVersion = 0;
	Boolean foundPrefsFileVersion = GetPreferenceULong(kPrefsFileVersion, &prefsFileVersion);
	
	
	if (prefsFileVersion < 3) {
		const char* name;
		Boolean resetServerName = false;

		GetPreferenceString(kPrefsLoginName, &name);
		if (EqualString("arcadia.artemis.com", name)) {
			SetPreferenceString(kPrefsLoginName, kDefaultLoginServerName);
			resetServerName = true;
		} 
		GetPreferenceString(kPrefsPreregistrationName, &name);
		if (EqualString("arcadia.artemis.com", name)) {
			SetPreferenceString(kPrefsPreregistrationName, kDefaultPreregistrationServerName);
			resetServerName = true;
		}
		if (resetServerName) {
			paramtext("Changing \"arcadia.artemis.com\" to \"" kDefaultLoginServerName "\"", "", "", "");
			AlertUser(rPreferencesAlertDialog);
		}
	}
	
	if (prefsFileVersion < 4) {
		const char* name;
		Boolean resetServerName = false;
		
		GetPreferenceString(kPrefsLoginName, &name);
		if (EqualString("testdummy.artemis.com", name)) {
			SetPreferenceString(kPrefsLoginName, kDefaultTestServerName);
			resetServerName = true;
		}
		
		GetPreferenceString(kPrefsPreregistrationName, &name);
		if (EqualString("testdummy.artemis.com", name)) {
			SetPreferenceString(kPrefsPreregistrationName, kDefaultTestServerName);
			resetServerName = true;
		}
		if (resetServerName) {
			paramtext("Noticed that you were attaching to testdummy.artemis.com.  "
					  "The new test server is \"" kDefaultTestServerName "\".  Your "
					  "\"Connect toÉ\" preference has been switched to the new server.",
					  "", "", "");
			AlertUser(rPreferencesAlertDialog);
		}
	}
	
	// Let's say you want to force everyone to NOT use the phone.
	//   1.  Set the default to not-use-phone elsewhere in the program.
	//   2.  Remove the current phone pref by bumping the version by one
	//       (i.e., increase kDefaultPrefsFileVersion by one) and removing
	//       the old preference:
	//
	//	          if (prefsFileVersion < 5) {
	//	              RemovePreference("Use Phone Preference Name");
	//                paramtext("FYI:  Removed your old phone preference","","","");
	//                AlertUser(rGenericAlertDialog);
	//	          }
	//
	//            if (prefsFileVersion < 6) {
	//                RemovePreference("Simulator Window Position");
	//				  // don't bother warning them...it'll be obvious...
	//            }
	
	if (prefsFileVersion != kDefaultPrefsFileVersion) {
		SetPreferenceULong(kPrefsFileVersion, kDefaultPrefsFileVersion);
	}
}

void MacSimulator::OpenPreferences()
{
	Boolean newlyCreated = false;
	short oldResFileID = CurResFile();
	OSErr err, resError;
	FSSpec prefFSSpec;
	Str255 pascalName;
	snprintf((char*)pascalName, sizeof(pascalName),
			"%.*s", sizeof(pascalName)-1, kPrefsFileName);
	c2pstr((char*)pascalName);
	
	err = FSMakeFSSpec(0, 0, pascalName, &prefFSSpec);
	
	if (err == fnfErr)
	{
		err = FSpCreate(&prefFSSpec, kPreferencesCreator, kPreferencesFileType, smSystemScript);
		if (err != noErr)
		{
			Complain(("Can't create preferences file (FSpCreate() returned %d)", err));
			return;
		}
		FSpCreateResFile(&prefFSSpec, kPreferencesCreator, kPreferencesFileType, smSystemScript);
		resError = ResError();
		if (resError != noErr)
		{
			Complain(("Can't create resource map for preferences file (FSpCreateResFile() made ResError() return %d, ResError() returned %d)", err, resError));
			goto OpenPreferences_exit;
		}
		newlyCreated = true;
	}
	else if (err != noErr)
	{
		Complain(("Can't find preferences file (FSMakeFSSpec() returned %d)", err));
		goto OpenPreferences_exit;
	}
	
	fResFileID = FSpOpenResFile(&prefFSSpec, fsRdWrPerm);
	if (fResFileID == -1)
	{
		resError = ResError();
		Complain(("Can't open preferences file (FSpOpenResFile() made ResError() return %d)", resError));
		goto OpenPreferences_exit;
	}
	if (newlyCreated) {
		SetPreferenceULong(kPrefsFileVersion, kDefaultPrefsFileVersion);
	}	
OpenPreferences_exit:
	UseResFile(oldResFileID);
}

void MacSimulator::ClosePreferences()
{
	short oldResFileID = CurResFile();

	if (fResFileID != -1)
		UpdateResFile(fResFileID);
	
	CloseResFile(fResFileID);
	
	if (oldResFileID != fResFileID)
		UseResFile(oldResFileID);
	
	fResFileID = -1;
}

// ---------------------------------------------------------------------------
//
// Settings Methods
//
//static void TransferResource(ResType resourceType, short resourceID, short UNUSED(saveAsResourceID))
//{
//	Handle resourceHandle;
//	
//	if ((resourceHandle = GetResource(resourceType, resourceID)) == nil)
//	{	
//		Complain(("Cannot find resource '%c%c%c%c' %d to put in settings file",
//			resourceType>>24, (resourceType>>16)&0xFF, (resourceType>>8)&0xFF, resourceType&0xFF, resourceID));
//		return;
//	}
//
//	LoadResource(resourceHandle);	// just in case someone turned off auto-loading
//	HNoPurge(resourceHandle);
//	DetachResource(resourceHandle);
//	AddResource(resourceHandle, resourceType, resourceID, nil);
//}
//
//static void TransferSettingsResources(void)
//{
//	TransferResource('icl4', rSystemPreferencesIcon, rSaveAsPreferencesIcon);
//	TransferResource('icl8', rSystemPreferencesIcon, rSaveAsPreferencesIcon);
//	TransferResource('ICN#', rSystemPreferencesIcon, rSaveAsPreferencesIcon);
//	TransferResource('ics#', rSystemPreferencesIcon, rSaveAsPreferencesIcon);
//	TransferResource('ics4', rSystemPreferencesIcon, rSaveAsPreferencesIcon);
//	TransferResource('ics8', rSystemPreferencesIcon, rSaveAsPreferencesIcon);
//}





// ---------------------------------------------------------------------------
// Profiler methods
// ---------------------------------------------------------------------------

void MacSimulator::InitializeProfiler()
{
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
 	OSErr err = ProfilerInit(collectDetailed, bestTimeBase, 500, 50);
	if (err)
		Complain(("ProfilerInit failed with %d", err));
	ProfilerSetStatus(false);
#endif
	
	fIsProfiling = false;
}

Boolean MacSimulator::GetIsProfiling()
{
	return fIsProfiling;
}

void MacSimulator::StartProfiling()
{
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
	if (ProfilerGetStatus())
		Complain(("Profiler already running!"));
	ProfilerSetStatus(true);
	
	fIsProfiling = true;
#endif
}

void MacSimulator::StopProfiling()
{
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
	if (!ProfilerGetStatus())
		Complain(("Profiler not running!"));
	ProfilerSetStatus(false);
	
	fIsProfiling = false;
#endif
}

void MacSimulator::FinalizeProfiler()
{
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
	if (fIsProfiling)
		StopProfiling();
	ProfilerDump("\pWebTV profile");
 	ProfilerTerm();

	fIsProfiling = false;
#endif
}

