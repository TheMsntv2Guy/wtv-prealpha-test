// ===========================================================================
//	Macintosh.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CACHEWINDOW_H__
#include "CacheWindow.h"
#endif
#ifndef __LAYER_H__
#include "Layer.h"
#endif
#ifndef __LOCALNET_H__
#include "LocalNet.h"
#endif
#ifndef __LEDWINDOW_H__
#include "LEDWindow.h"
#endif
#ifndef __MACINTOSH_H__
#include "Macintosh.h"
#endif
#ifndef __MACINTOSHDIALOGS_H__
#include "MacintoshDialogs.h"
#endif
#ifndef __MACINTOSHIR_H__
#include "MacintoshIR.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __MEMORYCHECKPOINTING_H__
#include "MemoryCheckpointing.h"
#endif
#ifndef __MEMORYGLANCE_H__
#include "MemoryGlance.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYSEISMOGRAPH_H__
#include "MemorySeismograph.h"
#endif
#ifndef __MEMORYWINDOW_H__
#include "MemoryWindow.h"
#endif
#ifndef __MESSAGEWINDOW_H__
#include "MessageWindow.h"
#endif
#ifndef __MISCSTATWINDOW_H__
#include "MiscStatWindow.h"
#endif
#ifndef __NETWORK_H__
#include "Network.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __REMOTEWINDOW_H__
#include "RemoteWindow.h"
#endif
#ifndef __SERIAL_H__
#include "Serial.h"
#endif
#ifndef __SERVICEWINDOW_H__
#include "ServiceWindow.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __SIMULATORSTATE_H__
#include "SimulatorState.h"
#endif
#ifndef __SIMULATORWINDOW_H__
#include "SimulatorWindow.h"
#endif
#ifndef __SOCKETWINDOW_H__
#include "SocketWindow.h"
#endif
#ifndef SOUNDMUSICSYS_DRIVER
#include "SoundMusicSystem.h"
#endif
#ifndef __TELLYIO_H__
#include "TellyIO.h"
#endif
#ifndef __TESTING_H__
#include "Testing.h"
#endif
#ifndef __SONGDATA_H__
#include "SongData.h"
#endif



// ---------------------------------------------------------------------------
//	local prototypes
// ---------------------------------------------------------------------------
static pascal long MyGrowZone(Size cbNeeded);
static Boolean LowOnReserve(void);
static void RecoverReserve(void);
static Boolean AllocateReserve(void);
static Boolean FailLowMemory(long memRequest);
static MacPoint GetGlobalMouse(void);
static MacPoint GetGlobalTopLeft(WindowPtr window);

static void OpenDocument(FSSpecPtr file);
static pascal Boolean SFFilter(CInfoPBPtr p, Boolean *unused);
static pascal short SFGetHook(short mySFItem, DialogPtr dialog, void *unused);

static void DoKeyDown(char key, short modifiers, Boolean autoRepeat, WindowPtr window);
static void DoUpdate(WindowPtr window);
static void DoActivate(WindowPtr window, Boolean becomingActive);

static pascal OSErr OpenApplicationEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
static pascal OSErr OpenDocumentsEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
static pascal OSErr PrintDocumentsEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
static pascal OSErr QuitApplicationEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
static void InstallAppleEventHandlers(void);

static void AdjustCursor(RgnHandle region);
static ulong WaitNextEventTimeout(void);
static void EventLoop(void);
static void InstallZapTCP(void);

static void Initialize();
static void Finalize();


// ---------------------------------------------------------------------------
//	global variables
// ---------------------------------------------------------------------------
short		gAppResRef;				//set up by Initialize
Boolean		gInBackground;			//maintained by Initialize and DoEvent
Handle		gReserveMemory;
Boolean		gTerminate;

#if GENERATINGCFM && !defined(__MWERKS__)
	QDGlobals	qd;
#endif

#if GENERATINGCFM
	//	allocate the RoutineDescriptors for Power Mac toolbox calls
	CreateRoutineDescriptor(uppGrowZoneProcInfo, MyGrowZone);
	CreateRoutineDescriptor(uppFileFilterYDProcInfo, SFFilter);
	CreateRoutineDescriptor(uppDlgHookYDProcInfo, SFGetHook);
	pascal void DefaultOutline(WindowPtr window, short theItem);
	CreateRoutineDescriptor(uppUserItemProcInfo, DefaultOutline);
#endif





// ---------------------------------------------------------------------------
//	Some of these should be transferred to MacintoshUtilities.h
// ---------------------------------------------------------------------------
Boolean IsDAWindow(WindowPtr window)
{
	return ((window != nil) && (((WindowPeek)window)->windowKind >= 0));
}

Boolean IsDocWindow(WindowPtr)
{
	return false;
}

Boolean IsModalWindow(WindowPtr)
{
	//	This test will return true for a window that needs to be treated as a
	//	modal window.  To include additional windows, add the window’s refCon
	//	value to the set of modal windows.
	return false;
}

MacBoolean RequestSaveFile(FSSpec* destSpec, const char* defaultCName)
{
	StandardFileReply reply;
	Str255 defaultPName;
	
	int defaultNameLength = strlen(defaultCName);
	if (defaultNameLength > 255)
		defaultNameLength = 255;
	
	defaultPName[0] = defaultNameLength;
	strncpy((char*)&(defaultPName[1]), defaultCName, defaultNameLength);
	
	StandardPutFile("\pSave as:", defaultPName, &reply);
	
	if (reply.sfGood)
	{	*destSpec = reply.sfFile;
		return true;
	}
	return false;
}

void SavePageToDisk(FSSpec* saveFile, const char* urlName)
{
	OSErr err;
	short refNum;
	
	err = FSpCreate(saveFile, 'MPS ', 'TEXT', smSystemScript);
	if (err != noErr)
	{	Complain(("Cannot write to file %s (error %d)", &(saveFile->name[1]), err));
		return;
	}
	
	err = FSpOpenDF(saveFile, fsWrPerm, &refNum);
	if (err != noErr)
	{	Complain(("Cannot open file %s (error %d)", &(saveFile->name[1]), err));
		return;
	}
	
	CacheEntry*	entry = gRAMCache->Find(urlName);
	if (entry == nil)
	{	Complain(("Cannot find %s in RAM cache", urlName));
	}
	else
	{	
		char* data = entry->GetData();
		long dataLength = entry->GetDataLength();
		long dataWritten = dataLength;
		
		if (data == nil)
		{	Complain(("Cannot find data for %s in gRAMCache", urlName));
		}
		else
		{
			err = FSWrite(refNum, &dataWritten, data);
			if (err != noErr)
			{	Complain(("Error writing to file %s, (error %d)", &(saveFile->name[1]), err));
			}
			else if (dataWritten != dataLength)
			{	Complain(("WARNING: only wrote %d of %d bytes to %s",
							dataWritten, dataLength, &(saveFile->name[1])));
			}
		}
	}
	
	FSClose(refNum);
	Message(("Done saving page '%s' to file '%s'", urlName, &(saveFile->name[1])));
}

void SetWindowHeight(WindowPtr window, short newHeight)
{
	if (newHeight != window->portRect.bottom - window->portRect.top)
		SizeWindow(window, window->portRect.right - window->portRect.left, newHeight, true);
}


// ---------------------------------------------------------------------------
//	Macintosh muck-a-muck (initialization dirty work)
// ---------------------------------------------------------------------------

static short NumToolboxTraps(void)
{
	/*
		InitGraf is always implemented (trap 0xA86E). If the trap table is big
		enough, trap 0xAA6E will always point to either Unimplemented or some other
		trap, but will never be the same as InitGraf. Thus, you can check the size
		of the trap table by asking if the address of trap 0xA86E is the same as 0xAA6E.
	*/
	if ( GetToolboxTrapAddress(_InitGraf) == GetToolboxTrapAddress(_InitGraf + 0x0200) )
		return (0x0200);
	else
		return (0x0400);
}


/*
Check to see if a given trap is implemented.  GetTrapAddress may screw up and
wrap around into the trap table if you give it a toolbox trap that is out of
range, so check for this first.
*/

static Boolean TrapExists(short theTrap)
{
	unsigned		trapAddress;

	if ( !(theTrap & kOSTrapBit) )						// is this an OS trap?
		trapAddress = (unsigned)GetOSTrapAddress(theTrap);
	else {												// no, this is a tool trap
		theTrap &= kTrapNumberMask;						// get the trap number
		if ( theTrap < NumToolboxTraps() )				// can this tool trap exist?
			trapAddress = (unsigned)GetToolTrapAddress(theTrap);
		else
			return (false);
	}
	return ( (unsigned)GetToolboxTrapAddress(_Unimplemented) != trapAddress );
}


/*
This is a very basic grow zone procedure.  My application keeps a reserve
handle of memory in case the Memory Manager gets a request for some memory
that is not available in my heap.  If memory were to get tight (<32k),
the Toolbox will crash the system, especially Quickdraw.  Before releasing
the reserve handle I make sure it isn’t the GZSaveHnd.  This handle cannot
be touched by the grow zone procedure.

WARNING:  The grow zone procedure will be called and A5 may not be valid.
Read Tech Note #136 and 208
*/

static pascal long MyGrowZone(Size cbNeeded)
{
#pragma unused (cbNeeded)
	long theA5;
	long result;

	theA5= SetCurrentA5();
	if (((*gReserveMemory) != nil) && (gReserveMemory != GZSaveHnd())) {
		EmptyHandle(gReserveMemory);
		result = kSizeOfReserve;					//released this much memory
	} else
		result = 0;								//this may release more memory
	theA5= SetA5(theA5);
	return result;
}


/*
Before my application attempts to use more memory, I call this routine
to check if I’m already using my reserve memory.  If so, then I better
prepare to die or get my reserve back.
*/

static Boolean LowOnReserve(void)
{
	return (*gReserveMemory) == nil;		// empty handle is low reserve
}


/*
This is called from the event loop if LowOnReserve returns that I’m out of
the reserve memory.  This will recover the reserve memory block.  If this
fails, it will be called the next time through the event loop.
*/

static void RecoverReserve(void)
{
	ReallocateHandle(gReserveMemory, kSizeOfReserve);
}


/*
This is called at startup time to allocate the reserve memory block used
in the grow zone procedure.  If I’m unable to obtain this reserve, then
return false to signal a failure.
*/

static Boolean AllocateReserve(void)
{
	gReserveMemory = NewHandle(kSizeOfReserve);
	return gReserveMemory != nil;
}


/*
Call PurgeSpace and see if the requested amount of memory exists in the
heap including a minimal amount of heap space.  Also, if my grow zone’s
reserve has been release, that’s considered a failure.  I don’t perform
any purging here.  The Memory Manager will do this if it needs the space.
This routine can be called with a memRequest == 0.  This checks if the heap
space is getting critical.
*/

static Boolean FailLowMemory(long memRequest)
{
	long	total;
	long	contig;

	PurgeSpace(&total, &contig);
	return (total < (memRequest + kMinSpace)) || LowOnReserve();
}


/*
Get the global coordinates of the mouse.  Get the global coordinates by
calling GetMouse and LocalToGlobal.  This assumes the current port is a
valid graf port.  When wouldn’t it be?
*/

static MacPoint GetGlobalMouse(void)
{
	MacPoint globalPt;

	GetMouse(&globalPt);
	LocalToGlobal(&globalPt);
	return globalPt;
}


/*
Try and determine the window’s title bar height.  This isn’t easy,
especially if the window is invisible.  We all know how the standard Apple
System WDEF works, at least at this date we do.  I assume that method as
shown below.  I could do what MacApp does with the Function CallWDefProc.
If the user has installed a replacement for the System WDEF, then it’s
their problem to deal with.  My method will work as long as Apple doesn’t
change how the WDEF in System 6.0 calculates the title bar height.  This
will allow my application to work on international Macs that have a larger
system font than Chicago.  I check the window’s variant code for one that
includes a title bar.  I will have to change this routine to adjust for
the modal-moveable window type, which hasn’t been defined yet.

In this routine, I violate my rule about not using the GetPort, SetPort,
SetPort sequence mentioned at the start of the file. Mostly, I do this
because it’s not all that apparent that a routine called GetWTitleHeight
will change the port, so I make sure that it doesn’t.
*/

/*
Given a window, this will return the top left point of the window’s
port in global coordinates.  Something this doesn’t include, is the
window’s drag region (or title bar).  This returns the top left point
of the window’s content area only.

In this routine, I violate my rule about not using the GetPort, SetPort,
SetPort sequence mentioned at the start of the file. Mostly, I do this
because it’s not all that apparent that a routine called GetGlobalTopLeft
will change the port, so I make sure that it doesn’t.
*/

//#pragma segment Main
static MacPoint GetGlobalTopLeft(WindowPtr window)
{
	GrafPtr theGraf;
	MacPoint globalPt;

	GetPort(&theGraf);
	SetPort(window);
	globalPt = TopLeft(window->portRect);
	LocalToGlobal(&globalPt);
	SetPort(theGraf);
	return globalPt;
}


void DoCloseWindow(WindowPtr window)
{
	Boolean		result = true;
	
	StdWindow* stdWindowPtr = StdWindow::GetStdWindow(window);
	if (stdWindowPtr != nil)
	{	stdWindowPtr->Close();
	}
	else if (IsRemoteWindow(window))
	{	CloseRemoteWindow(window);
	}
	else
	{
		switch (GetWRefCon(window)) 
		{
	
			case rSimulatorWindow:
				break;
	
#ifdef DEBUG_MEMORYWINDOW
			case rMemoryWindow:
				CloseMemoryWindow((WindowPtr)window);
				break;
#endif
				
			default:
				if (IsDAWindow(window))						// we can close DAs too
					CloseDeskAcc(((WindowPeek)window)->windowKind);
				break;
		}
	}
}


/*
Clean up the application and exit.  I close all of the windows so that
they can update their documents, if any.  Dispose of the SoundUnit and
close the status window if the user really wants to quit.
*/

void Terminate(void)
{
	gTerminate = true;
}



/*
pascal void DoErrorSound(short soundNo)
{
	#pragma unused (soundNo)
	SysBeep(30);							//does the right thing now
}
*/


/*
Boolean PositionAvailable(MacPoint newPt, short wTitleHeight)
{
	Boolean taken = false;
	WindowPtr oldWindow = LMGetWindowList();

	while ((oldWindow != nil) && !taken) {
		if (IsDocWindow(oldWindow) && ((WindowPeek)oldWindow)->visible)
		{
			MacPoint oldPt = GetGlobalTopLeft(oldWindow);
			taken = (ABS(newPt.h - oldPt.h) + ABS(newPt.v - oldPt.v))
						<= ((kWindowPosStaggerH + kWindowPosStaggerV + wTitleHeight) / 2);
		}
		oldWindow = (WindowPtr)(((WindowPeek)oldWindow)->nextWindow);
	}
	return !taken;
}
*/


/*
OSErr CreateDocument(short resRef, FSSpecPtr file)
{
	#pragma unused (resRef, file)
	return noErr;
}
*/

/*
Believe it or not, but this turned out to be one of the more difficult
routines to write.  This was the best approach I could thing of to avoid
serious problems.  I don’t center the Standard File dialog because it
wouldn’t be under the File menu where the user probably has the mouse.  I
use OpenRFPerm because it allows for permissions and doesn’t depend on the
current directory.  First problem with opening a resource file was to
check if the file already being open.  Read Tech Notes #116 and #185.
GetFInfo seems to do the trick.  At least it does report all files that
are open on this machine.  It may be incorrect for files on AppleShare
opened by other machines.  What I’m worried about is when opening a
resource file for the second time on the same machine may return the
previous opener’s resource map.  This is very dangerous, and if the second
opener calls CloseResFile there will be a crash.  I want to be friendly
and if the user tries to open the file the second time and I’ve already
got it on the screen, then I’ll bring that window forward.  To test for
this, I compare the file’s name, DirID and vRefNum.  The vRefNum is
dynamic and needs to be converted into a volume name if I were to save
this information for future use.

There are a couple pitfalls, even with this method.  The user can open the
file, then move it to another directory.  This same thing confuses all
other applications I’ve tested.  Opening a resource file will allocate a
resource map handle into my heap, which may be large depending on the
number of resources in that file.  Additionally this loads all resources
marked “preload.”  So I set ResLoad to false to prevent the preloading.  I
test if the amount of memory I believe my document will require is
available after opening the resource file.  If true, I test if there are
any sound resources in that file and if not alert the user.  After all
this, create the new document.  If after creating the new document I find
that memory is too low, will close it and return an error.  If it does
fail, then close the file before anything else or there may not be enough
memory to show the alert.  All of these tests created a number of
IF-THEN-ELSE blocks and became unyielding.  C programmers get a break, so
gimme me one too.  I use the MPW Pascal EXIT.

BUG NOTE: Don’t open a resource file that is already open.  OpenResFile
may return an existing resource map when it gets opWrErr from the file
system.  If this happens, the resource file will not be unique and this
is very bad.  Another problem is if I get a read-only path and someone
else opens it for read/write.  This is also very bad.  Read Tech Notes
#116 and #185 hint at this problem, but I think a more comprehensive one
is in order.

BUG NOTE: GetWDInfo fails with nsvErr if the working directory returned
from Standard File is the root of an A/UX volume.  I could work around
this,  but it would be dependant on the current version of A/UX.  Read
Tech Note #229.  I believe this is also true for TOPS.

*/

static void OpenDocument(FSSpecPtr file)
{
	#pragma unused (file)
	PostulateFinal(false);
}

static pascal Boolean SFFilter(CInfoPBPtr p, Boolean *unused)
{
	#pragma unused (p, unused)
#define kShowIt			false				//false means I do not filter out...
#define kDoNotShowIt	true				//the file and true means that I do.

	return kDoNotShowIt;
}

static pascal short SFGetHook(short mySFItem, DialogPtr dialog, void *unused)
{
	#pragma unused (dialog, unused)
	return mySFItem;
}


/*
VERSION 1.1: This routine used to be inside of OpenDocument.  The
standard file code was removed to here in order to support opening
documents being open from the Finder.  I've switched to using a constant
for the topLeft point of the Standard File dialog.  It saves a few bytes
of code, so what the hell?  I found a cosmetic problem with the cursor
not being restored back to arrow after returning Standard File if there
was an error dialog shown immediately.  So, it is immediately set back to
the arrow.
*/

struct SubstitutionPair
{
	char*		ifThis;
	char*		thenThis;
};
typedef struct SubstitutionPair SubstitutionPair;

static SubstitutionPair	gSubstitutionPairs[] = 
{
	{ "about", 	"file://ROM/About.html" },
	{ "next", 	"http://www.next.com/" },
	{ "pin", 	"http://alma.netmedia.com/apreal/" },
	{ "pix", 	"http://www.pixelsight.com/" },
	{ "yoyo", 	"http://www.yoyo.com/" },
	{ "hot", 	"http://www.hot.presence.com/g/p/H3/" },
	{ "fox", 	"http://www.foxsports.com/Sports/Games/Fourth/" },
	{ "sports", "http://www.foxsports.com/" },
	{ "dilbert","http://www.unitedmedia.com/comics/dilbert/" },
	{ "mtv",	"http://mtv.com/" },
	{ "crawl",	"http://webcrawler.com/" },
	{ "be",		"http://www.be.com/" },
	{ "pjpeg",	"http://home.netscape.com/eng/mozilla/2.0/relnotes/demo/pjpegdemo.html" },
	{ "car",	"http://www.his.com/~titusb/ferraris_old/f355.html" },
	{ "weird",	"http://pscinfo.psc.edu/~rsnodgra/Ferrari/" },
	{ "big",	"http://www.gyf.com/" },
	{ "sleaze",	"http://metaverse.com/vibe/sleaze/index.html" },
	{ "kpt",	"http://the-tech.mit.edu/KPT/KPT.html" },
	{ "graph",	"http://www.graph.com/" },
	{ "mci",	"http://www.internetMCI.com/" },
	{ "im",		"http://mlds-www.arc.nasa.gov/mlds/en_US/exampleTable.html" },
	{ "im2",	"http://mlds-www.arc.nasa.gov/mlds/en_US/exampleMLDS.html" },
	{ "reg",	"http://arcadia/csw/html/registration/register.html" },
	{ "etak",	"http://www.etak.com/Webview/usa.html" },
	{ "news",	"http://arcadia/goldman/cgi-bin/wwwnntp" },
	{ "gill",	"http://home.texoma.com/personal/johnmoats/gilligan.htm" },
	{ "elle",	"http://www.ellemag.com/member/contents.html" },
	{ "tvnet",	"http://tvnet.com/TVnet.html" },
	{ "mpeg",	"http://arcadia/mpeg/index.html" },
	{ "sony",	"http://www.sony.co.jp/index.html" },
	{ "toshiba", "http://www.toshiba.co.jp/index.html" },
	{ "hitachi", "http://www.hitachi.co.jp/index.html" },
	{ "bugs", "http://mandarin/cgi-bin/wwwgnats" },
	{ "ui", "http://arcadia/ui/" },
	{ "code", "http://arcadia/webtv/client/code.html" },
	{ "fav", "http://arcadia/webtv/client/favorites.html" },
	{ "Tourist","client:Tourist?"
					"URL1=http://www.artemis.com/&Depth1=5&"
					"URL2=http://arcadia/webtv/client/home.html&Depth2=3&"
					//"URL3=http://www.stanford.edu/&Depth3=2&"
					//"LoadTime=7200&"
					//"ShowTime=300&"
					"MaxRetries=2&"
					"ScrollThrough=true"
					}
};

static void SubsituteCommonNames(char *url)
{
	int		i;
	
	for (i = 0; i < sizeof(gSubstitutionPairs)/sizeof(gSubstitutionPairs[0]); i++)
		if (strcmp(url, gSubstitutionPairs[i].ifThis) == 0)
		{
			strcpy(url, gSubstitutionPairs[i].thenThis);
			return;
		}
}

void HandleOpen(void)
{
	static const char *defaultPrefix = "http://";
	static char urlName[256] = "";
	Boolean isDefault = (urlName[0] == 0 );
	
	if ( isDefault )
		strcpy(urlName,defaultPrefix);
	if (!QueryUser(rURLDialog, urlName,isDefault))
		return;
	
	SubsituteCommonNames(urlName);
	
	gPageViewer->ExecuteURL(urlName, nil);
	gMacSimulator->ForceUpdateAll();
}

void HandleAbout(void)
{
	paramtext(
#ifdef DEBUG
			  "(DEBUG build)",
#else
			  "",
#endif /* DEBUG */
			  (gSystem == nil) ? "????" : gSystem->GetVersion(),
			  "",
			  "");
	AlertUser(rAboutBoxDialog);
}

void HandleReload(void)
{
	gPageViewer->Reload();
	gMacSimulator->ForceUpdateAll();
}

void HandleReloadAll(void)
{
	gPageViewer->ReloadAll();
	gMacSimulator->ForceUpdateAll();
}

// ----------------------------------------------------------------------------



/*
When the user types a key, I check if it is one that I’m looking for.
This routine only handles the non-command key events.  Here I’m looking
for a arrow key or the return and enter keys for the default button.

VERSION 1.1: The enter and return keys only work if there is a selection.
Now supporting the backspace and delete keys which clear the selection.
*/

static void
DoKeyDown(char key, short modifiers, Boolean autoRepeat, WindowPtr window)
{
	switch (GetWRefCon(window)) 
	{

#ifdef DEBUG_MEMORYWINDOW
		case rMemoryWindow:
			MemoryWindowHandleKey(window, key);
			break;
#endif

		default:
			gMacSimulator->HandleKey(key, modifiers, autoRepeat);
			break;
	}
}

//void HandleSimulatorClick(WindowPtr window, EventRecord *event)
//{
//	ControlHandle control;
//		
//	//if (gMacSimulator->HandleClick(event->where))
//	//	return;
//		
//	SetPort((GrafPtr)window);
//	GlobalToLocal(&(event->where));
//	if (FindControl(event->where, window, &control) != 0)
//	{	if (TrackControl(control, event->where, nil) != 0)
//		{
//		}
//	}
//}


/*
This is called when an update event is received for any of my windows.  It
calls the appropriate window’s update routine to draw its contents.  As an
efficiency measure that does not have to be followed, it calls the drawing
routine only if the visRgn is non-empty.  This will handle situations
where calculations for drawing or drawing itself is very time-consuming.
Why does QD give you an update with an empty updateRgn?
*/

static void
DoUpdate(WindowPtr window)
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(window);

	BeginUpdate(window);							//setup the visRgn, clears updateRgn
	if (! EmptyRgn(window->visRgn)) 
	{ 												//if updating to be done
		SetPort(window);							//set to the current port

		switch (GetWRefCon(window)) 
		{										//call the window’s drawing routine
			//case rSimulatorWindow:
			//	if (gMacSimulator->HandleUpdate())
			//		break;
			//	gMacSimulator->DrawWindow();
			//	break;
		
#ifdef DEBUG_MEMORYWINDOW
			case rMemoryWindow:
				DrawMemoryWindow(window);
				break;
#endif
		}
	}
	EndUpdate(window);								//restores the visRgn
	SetPort(savePort);
}


/*
This is called when a window is to be activated or deactivated.  For the
document window I activate all the controls and the list.  For the status
window I activate the one and only control.  This is also called for
suspend and resume events while running MultiFinder.
*/

static void
DoActivate(WindowPtr window, Boolean becomingActive)
{
	#pragma unused (becomingActive)
	GrafPtr savePort;
	
	if (window != nil) {
		switch (GetWRefCon(window)) {
			//case rSimulatorWindow:
			//	break;
			case rMemoryWindow:
				GetPort(&savePort);
				SetPort(window);
				InvalRect(&((GrafPtr)window)->portRect);	// allow it to redraw
				SetPort(savePort);
				break;
		}
	}
}


// ---------------------------------------------------------------------------
//	Required AppleEvents
// ---------------------------------------------------------------------------
static pascal OSErr OpenApplicationEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	#pragma unused (theAppleEvent, reply, handlerRefcon)
	return (noErr);
}

static pascal OSErr OpenDocumentsEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	#pragma unused (reply, handlerRefcon)
	FSSpec				file;
	AEDescList			doList;
	long				i;
	long				itemsInList;
	Size				actualSize;
	AEKeyword			keyword;
	DescType			returnedType;
	OSErr				theErr;

	Complain(("Got here"));

	// get the direct parameter--a descriptor list--and put it into doList
	theErr = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &doList);
	if (theErr == noErr)
	{
		if (doList.descriptorType != typeAEList)
		{
			AEDisposeDesc (&doList);
			theErr = paramErr;
		}
		else
		{
			// count the number of descriptor records in the list
			theErr = AECountItems(&doList, &itemsInList);
			if (theErr == noErr)
			{
				//get each descriptor record from the list, coerce the returned data
				//to an FSSpec and open the associated file

				for (i = 1; i <= itemsInList; i++)
				{
					theErr = AEGetNthPtr(&doList, i, typeFSS, &keyword, &returnedType,
											&file, sizeof(file), &actualSize);
					if (theErr != noErr)
						OpenDocument(&file);
				}
			}
			AEDisposeDesc (&doList);
		}
	}
	return (theErr);
}

static pascal OSErr PrintDocumentsEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	#pragma unused (theAppleEvent, reply, handlerRefcon)
	return (errAEEventNotHandled);
}

static pascal OSErr QuitApplicationEvent(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	#pragma unused (theAppleEvent, reply, handlerRefcon)
	Terminate();
	return noErr;
}

#if GENERATINGCFM
CreateRoutineDescriptor(uppUserItemProcInfo, OpenApplicationEvent);
CreateRoutineDescriptor(uppUserItemProcInfo, OpenDocumentsEvent);
CreateRoutineDescriptor(uppUserItemProcInfo, PrintDocumentsEvent);
CreateRoutineDescriptor(uppUserItemProcInfo, QuitApplicationEvent);
#endif

static void InstallAppleEventHandlers(void)
{
	//	Install the four core AppleEvent handlers.

	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, 
				GetRoutineAddress(QuitApplicationEvent), 0, false);

	AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
				GetRoutineAddress(OpenDocumentsEvent), 0, false);

	AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
				GetRoutineAddress(PrintDocumentsEvent), 0, false);

	AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
				GetRoutineAddress(OpenApplicationEvent), 0, false);
}

// ---------------------------------------------------------------------------

static void DragPersistentWindow(WindowPtr window, MacPoint where, Rect *bounds)
{
	DragWindow(window, where, bounds);
	
	switch(GetWRefCon(window))
	{
		//case rSimulatorWindow:
		//	GrafPtr savePort;
		//	GetPort(&savePort);
		//	SetPort(window);
		//	MacPoint topLeft;
		//	topLeft.v = window->portRect.top;
		//	topLeft.h = window->portRect.left;
		//	LocalToGlobal(&topLeft);
		//	gMacSimulator->SetPreferencePoint(kPrefsSimulatorWindowPosition, &topLeft);
		//	SetPort(savePort);
		//	break;

#ifdef DEBUG_MEMORYWINDOW
		case rMemoryWindow:
			MemoryWindowChanged(window);
			break;
#endif
	}
}

void DoEvent(EventRecord *event)
{
	WindowPtr window;
	MacPoint where;
	short part;
	short err;
	char key;

	window = LMGetWindowList();
	
	while (window != nil)
	{
		StdWindow* stdWindowPtr = StdWindow::GetStdWindow(window);
		if (stdWindowPtr != nil)
		{	if (stdWindowPtr->HandleEvent(event))
			{	return;
			}
		}
		window = (WindowPtr)(((WindowPeek)window)->nextWindow);
	}
	
	switch (event->what)
	{
		case mouseDown:
			part = FindWindow(event->where, &window);
			if (	(IsModalWindow(FrontWindow()) && (window != FrontWindow()))
				 || (IsModalWindow(FrontWindow()) && (part == inMenuBar)))
			{
				SysBeep(30);							//click outside of modal window
				return;									//break out of routine
			}
			switch (part)
			{
				case inMenuBar:								//process the menu command
					AdjustMenus(event->modifiers);
					DoMenuCommand(MenuSelect(event->where), event->modifiers);
					break;

				case inSysWindow:							//let system handle the mouseDown
					SystemClick(event, window);
					break;

				case inContent:
					WindowPtr oldWindow = FrontWindow();
					
					if (IsRemoteWindow(window))				// the Remote wants clicks regardless
					{	MouseDownInRemoteWindow(window, event->where, event->when);
					}
					else if (window != oldWindow)
					{	SelectWindow(window);
					}
					else 
					{
						switch (GetWRefCon(window))
						{
							//case rSimulatorWindow:
							//	HandleSimulatorClick(window, event);
							//	break;

#ifdef DEBUG_MEMORYWINDOW
							case rMemoryWindow:
								MouseDownInMemoryWindow(window, event->where, (event->modifiers & optionKey) != 0);
								break;
#endif
								
							default:
								DragPersistentWindow(window, event->where, &qd.screenBits.bounds);
								break;

						}
					}
					break; //inContent

				case inDrag:						//pass screenBits.bounds to get all gDevices
					DragPersistentWindow(window, event->where, &qd.screenBits.bounds);
					break;

				case inGrow:
					break;

				case inGoAway:
					if (TrackGoAway(window, event->where))
						DoCloseWindow(window);
					break;

			} //switch (part)
			break; //mouseDown

// This fixes an interaction problem between the Sony remote control and
// the Sejin IR keyboard. This is a temporary fix for the Mac simulator.		
		case autoKey:
			Boolean		acceptableRepeatKey = false;
			switch (event->message & keyCodeMask) {
#define kSelectionDownKeyCode	0x7d00
#define kSelectionLeftKeyCode	0x7B00
#define kSelectionUpKeyCode		0x7E00
#define kSelectionRightKeyCode	0x7C00

				case kSelectionDownKeyCode:
				case kSelectionLeftKeyCode:
				case kSelectionUpKeyCode:
				case kSelectionRightKeyCode:
					acceptableRepeatKey = true; break;
			}
			if (!acceptableRepeatKey)
				break;
			// otherwise, fall through
			
		case keyDown:
			window = FrontWindow();

#ifdef PROPRESENTER
// Offset the char codes for the HOME(1->6) and END(4->9) keys to keep them from 
//		colliding with char codes from the ProPresenter

#define kHomeKeyCode 0x7300
#define kEndKeyCode  0x7700


			Message(("Macintosh.c: This is the key: %x, %x", event->message & keyCodeMask, event->message & charCodeMask));
			
			if ((event->message & keyCodeMask) == kHomeKeyCode || (event->message & keyCodeMask) == kEndKeyCode)
				event->message += 5;
#endif
				
			key = event->message & charCodeMask;
			
			// Command key down?	
			if ((event->modifiers & cmdKey) != 0) {
		
				if (key == kPeriod)
					break;

				// Avoid adjusting menus if it's an arrow key.
				if ((event->what == keyDown) && (!IsModalWindow(window)) && !IsArrowKey(key)) {
					long menuChoice;

					AdjustMenus(event->modifiers);				//adjust items properly
					menuChoice = MenuKey(key);
					
					if (HiWord(menuChoice) != 0) {
						DoMenuCommand(menuChoice, event->modifiers);
						break;
					}
				}
			}

			// Normal keys.
			if (window != nil)							//if there’s a window
				DoKeyDown(key, event->modifiers, event->what == autoKey, window);
			break;

		case activateEvt:						//true for activate, false for deactivate
			DoActivate((WindowPtr)event->message, event->modifiers & activeFlag);
			break;

		case updateEvt:							//call DoUpdate with the window to update
			DoUpdate((WindowPtr)event->message);
			break;

		case diskEvt:								//Call DIBadMount in response to a diskEvt
			if (HiWord(event->message) != noErr) {
				where.v = kRecordTop;
				where.h = kRecordLeft;
				err = DIBadMount(where, event->message);
			}
			break;

		case osEvt:
			switch (event->message >> 24) {					//get high byte of message

				case suspendResumeMessage:
					if ((event->message & suspendResumeMessage) != 0)
						gInBackground = false;				//it was a resume event
					else {
						gInBackground = true;				//it was a suspend event
						window = FrontWindow();				//get front window
						if (window != nil) 					//don’t use a nil window
							HiliteWindow(window, false);	//then properly activate it
					}
					DoActivate(FrontWindow(), !gInBackground);
					break; //suspendResumeMessage
			}
			break; //osEvt

		case kHighLevelEvent:
			//AEProcessAppleEvent(event);
			break;
			
	} //switch (event->what)
}


/*
Change the cursor’s shape, depending on its current position.  This also
calculates the region where the current cursor resides (for
WaitNextEvent).  This is based on its current position in global
coordinates.  If the mouse is ever outside of that region, an event is
generated causing this routine to be called again by the event loop.  This
allows me to change the region to where the mouse is currently located.
If there is more to the event than just “the mouse moved,” this gets
called before the event is processed to make sure the cursor is the right
one.  In any (ahem) event, this is called again before I fall back into
WaitNextEvent.  Extreme short values are used to create a wide open
region.  -SHRT_MAX - 1 is the largest negative short (-32768) and SHRT_MAX
is the largest positive short (32767).

BUG NOTE: The largest positive value for a region’s size is SHRT_MAX - 1
due to a very old bug that still remains to this day.
*/

static void
AdjustCursor(RgnHandle region)
{
	WindowPeek	window;

	if (gInBackground)
		return;	
		
	window = (WindowPeek)FrontWindow();
	if (IsDAWindow((WindowPtr)window)) 
		return;
	
	CopyRgn(window->strucRgn, region);

	if (IsRemoteWindow(window) && PtInRgn(GetGlobalMouse(), window->strucRgn))
		SetCursor(*(GetCursor(rHandCursor)));
	else
		SetCursor(&qd.arrow);
}


static ulong
WaitNextEventTimeout(void)
{
	if (gInBackground)
		return LONG_MAX;
		
	return 0;
}

/*
Get events forever, and handle them by calling DoEvent.  Since this
application requires System 6.0x or later I know that _WaitNextEvent is
always there, even without MultiFinder.  MultiFinder’s sleep is used to
determine how often I want to receive events, regardless if an event has
actually occurred.  In this application, I don’t perform any background
processing so I’m being super friendly by using LONG_MAX as a sleep
value.  This also helps keep Virtual Memory from paging me in just to run
my event loop and find out that nothing happened.  The application will
only be called upon for events that must be handled.  Also call
AdjustCursor each time through the loop.  AdjustCursor will return the
current region containing the mouse and is passed to WaitNextEvent.  If
the mouse travels out of the region, another event is generated.  I have
to call AdjustCursor just before doing the event to make sure the right
cursor is shown.  
*/

enum { kImportantEvents = mDownMask|mUpMask|keyDownMask|autoKeyMask };

static Boolean irIsSet = false;


static void
EventLoop(void)
{
	RgnHandle		cursorRgn = NewRgn();
	EventRecord		event;
	WindowPeek		frontWindow;

#if defined FIDO_INTERCEPT
	gMacSimulator->PostPendingPowerOn();
#endif
	while (!gTerminate) {

		if (LowOnReserve())
			RecoverReserve();

		AdjustCursor(cursorRgn);			// get the right cursor

		if (WaitNextEvent(everyEvent, &event, WaitNextEventTimeout(), cursorRgn))
		{
			if ( !(gInBackground && event.what == 6) ) {
				AdjustCursor(cursorRgn);		// get the right cursor				
				DoEvent(&event);
			}
			
			// overcome goofiness induced by CW debugger
			if ((frontWindow = (WindowPeek)FrontWindow()) != nil)
				gInBackground = !!!frontWindow->hilited;
		}
		else
		{
			MoviesTask(nil, 0);
#ifdef DEBUG_MEMORYWINDOW
			IdleMemoryWindows();			// give them time to update
#endif
			StdWindow::IdleAll();
		}
						
		if (gMacSimulator->GetPendingPowerOn())
		{
			gMacSimulator->PowerOn();
			gMacSimulator->SetPendingPowerOn(false);
			continue;
		}

		if (gMacSimulator->GetIsInitialized())
		{
			SetPort(gMacSimulator->GetWindow());
			//gMacSimulator->StartProfiling();
			gMacSimulator->UserLoopIteration();
			//gMacSimulator->StopProfiling();

			// Hack Alert: this does not allow the IR to be turned off
			// after booting...			
			if (!irIsSet) 
			{
				if (gMacSimulator->GetIREnabled())
					MacintoshIRInitialize();
				
				irIsSet = true;
			}
			continue;
		}
	}
}

/*
Set up the whole world.  Initialize global variables, Toolbox managers,
and menus.  If a failure occurs here, I will consider that the application
is in such bad shape that I should just exit.  Your error handling may
differ, but the checks should still be made.  I ask for a set of master
pointer blocks and all permanent storage at this point to cut down on
memory fragmentation.  I may be opening large numbers of resources and
documents so I allocate some extra master pointer blocks at the start.

VERSION 1.1:  Now supports opening of documents from the Finder.
*/

#if GENERATINGCFM
typedef UniversalProcPtr ExitToShellUPP;
#else
typedef ProcPtr/*Register68kProcPtr*/ ExitToShellUPP;
#endif

enum { uppExitToShellProcInfo = kPascalStackBased };

#if GENERATINGCFM
#define NewExitToShellProc(userRoutine)		\
		(ExitToShellUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppExitToShellProcInfo, GetCurrentArchitecture())
#else
#define NewExitToShellProc(userRoutine)		\
		((ExitToShellUPP)(userRoutine))
#endif

pascal void				ExitToShellPatch(void);
static UniversalProcPtr	Previous_ETS_Addr;	// previous trap address of _ExitToShell
ExitToShellUPP			exitToShellUPP = NewExitToShellProc((void *)ExitToShellPatch);

static void InstallZapTCP(void)
{
	Previous_ETS_Addr = NGetTrapAddress(_ExitToShell, ToolTrap);
	NSetTrapAddress(exitToShellUPP, _ExitToShell, ToolTrap);
}


static void Initialize(void)
{

	EventRecord event;
	short count;
	Boolean ignoreResult;

	gTerminate = false;
	gInBackground = false;
	gAppResRef = CurResFile();
	for (count = 1; count <= kNumberOfMasters; count++)
		MoreMasters();
		
	// init managers
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();
	InstallZapTCP();
	EnterMovies();

	/*
	This code is necessary to pull the application into the foreground.  I use
	EventAvail because I don’t want to remove any events the user may have
	done, such as typing ahead.  Until the application has made a few calls (3
	seems to be the magic number) to the Event Manager, MultiFinder keeps me
	in the background.   Splashscreens and Alerts will remain in a background
	layer until we get a few events.  This is documented in Tech Note #180.
	*/
	const kBroughtToFront = 3;
	for (count = 1; count <= kBroughtToFront; count++)
		ignoreResult = EventAvail(everyEvent, &event);

	if (! TrapExists(_Gestalt))
		EmergencyExit(sWrongVersion);

	long gestaltValue;
	if (Gestalt(gestaltSystemVersion, &gestaltValue) != noErr)
		EmergencyExit(sStandardErr);

	if (gestaltValue < 0x0700)
		EmergencyExit(sWrongVersion);

	/*
	Before I go any further, I want my reserve memory.  This is an emergency
	reserve (sorta like my old VW had) when memory runs low.  If I cannot
	obtain this reserve, then I’ll bail.  It’s also important to obtain my
	reserve before testing if I have the desired amount of memory to run
	this application.  Also, FailLowMemory will consider the memory reserve.
	*/

	if (! AllocateReserve())
		EmergencyExit(sLowMemory);
	SetGrowZone(GetRoutineAddress(MyGrowZone));

	/*
	Create the simulator
	*/

	gMacSimulator = newMac(MacSimulator);
	gMacSimulator->LoadPreferences();

	InitializeMenus();
	
	gSimulatorWindow = newMac(SimulatorWindow);
#ifdef DEBUG
	gDebugMessageWindow = newMac(DebugMessageWindow);
#endif /* DEBUG */

	InitRemoteWindows();
#ifdef DEBUG_MEMORYWINDOW
	InitializeMemoryWindows();
#endif /* DEBUG_MEMORYWINDOW */
	//InitCursorCtl(nil);								//MPW’s handy cursor routines

//
//	Initialize persistent debugging tools
//
#if defined(MEMORYGLANCE_ACTIVE)
	gURLGlanceWindow = newMac(URLGlanceWindow);
	gBlockGlanceWindow = newMac(BlockGlanceWindow);
	gTagItemWindow = newMac(TagItemWindow);
#endif /* MEMORYGLANCE_ACTIVE */

#if defined(DEBUG_MEMORYSEISMOGRAPH)
	gMemorySeismographWindow = newMac(MemorySeismographWindow);
#endif /* DEBUG_MEMORYSEISMOGRAPH */

#ifdef DEBUG_CACHEWINDOW
	gCacheWindow = newMac(CacheWindow);
#endif /* DEBUG_CACHEWINDOW */

#ifdef DEBUG_LEDWINDOW
	gLEDWindow = newMac(LEDWindow);
#endif /* DEBUG_LEDWINDOW */

#ifdef DEBUG_MISCSTATWINDOW
	gMiscStatWindow = newMac(MiscStatWindow);
#endif /* DEBUG_MISCSTATWINDOW */

#ifdef DEBUG_SERVICEWINDOW
	gServiceWindow = newMac(ServiceWindow);
#endif /* DEBUG_SERVICEWINDOW */

#ifdef DEBUG_SOCKETWINDOW
	gSocketWindow = newMac(SocketWindow);
#endif /* DEBUG_SOCKETWINDOW */

	gLocalNet = newMac(LocalNet);
	
	gMacSimulator->InitializeProfiler();
	InitializeProfileUnit();

/*
Last, I want to make sure that enough memory is free for my application to
run.  It is possible that user may have adjusted the SIZE resource to too
small a setting or for some other reason the application started up in a
very small memory partition.  It’s also possible for a situation to arise
where the heap may have been of the requested size taken from the SIZE
resource, but a large scrap was loaded which left too little memory.  I
want to make sure that my free memory is not being modified by the scrap’s
presence.  So, I unload it to disk but if the application will run once
the scrap is unloaded, then you’ll probably not get it back into memory.
Thus losing the clipboard contents.  I preform this check after
initializing all the Toolbox and the basic features of this application,
such as showing the about box.
*/
	if (FailLowMemory(kMinSpace)) {
		if (UnloadScrap() != noErr)
			EmergencyExit(sLowMemory);
		else {
			if (FailLowMemory(kMinSpace))
				EmergencyExit(sLowMemory);
		}
	}

	InstallAppleEventHandlers();
				
#if !GENERATINGCFM
{
	static AppFile gAppFile;

	short	msgType, appCount;
	CountAppFiles(&msgType, &appCount);
	
	gAppFile.fName[0] = 0;
	if (appCount > 0)
	{
		GetAppFiles(1, &gAppFile);
		p2cstr(gAppFile.fName);
		LoadSimulatorStateAs((FSSpec*)gAppFile.fName);
	}
}
#endif
}

static void Finalize(void)
{
	if (gLocalNet != nil) {
		gLocalNet->SavePrefs();
		delete gLocalNet;
		gLocalNet = nil;
	}
	
	StdWindow::SavePrefsAll();	// save everyone's preferences
	StdWindow::DeleteAll();		// and then delete all the StdWindows

	FinalizeProfileUnit();
	gMacSimulator->FinalizeProfiler();
	gMacSimulator->SavePreferences();
#ifdef DEBUG
	CloseLogFile();
#endif
}

/*
This routine is contained in the MPW runtime library.  It will be placed
into the code segment used to initialize the A5 globals.  This external
reference to it is done so that we can unload that segment, named %A5Init.
*/

#ifdef applec
extern void _DataInit(void);
#endif



/*
If you have stack requirements that differ from the default, then you
could use SetApplLimit to increase StackSpace at this point, before
calling MaxApplZone.
*/

void main(void)
{
#ifdef applec
	•••foo
	UnloadSeg(_DataInit);			//note that _DataInit must not be in Main!
#endif
	MaxApplZone();					//expand the heap so code segments load at the top
	
	Initialize();					//initialize the program
//	UnloadSeg(Initialize);			//note that Initialize must not be in Main!
	EventLoop();					//call the main event loop
	Finalize();						//non-critical cleanup (ExitToShell does essential stuff)
	ExitToShell();					//we're out of here!
}

Boolean		gInExitToShellPatch = false;

// this is our _ExitToShell patch, which will be called when the app crashes or quits.
// it even gets called on force-quits and bus errors.
//
pascal void ExitToShellPatch(void)
{
	long savedA5 = SetCurrentA5();
	//void UnpatchJVBL(void);

	if (!gInExitToShellPatch)			// only you can prevent reentrancy
	{
		gInExitToShellPatch = true;

#ifdef FOR_MAC
		if (!gMacSimulator->GetUsePhone())		/* cleanup MacTCP connections */
			if (gNetwork != nil)
				gNetwork->KillProtocols();
#endif
		
#ifdef INCLUDE_FINALIZE_CODE
		AudioCleanup();
#endif
		//UnpatchJVBL();
		
		if (MacintoshIRInitialized())	
			MacintoshIRFinalize();		// be a nice guy & close up the serial port

		if (ModemInitialized())
			SerialFinalize(kMODEM_PORT);
	}
	
	SetA5(savedA5);
	
	NSetTrapAddress(Previous_ETS_Addr,_ExitToShell,ToolTrap);
	ExitToShell();						// Call through to previous occupant
}
