// ===========================================================================
//	MacintoshMenus.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CLIENTFUNCTIONS_H__
#include "ClientFunctions.h"
#endif
#ifndef __FUNCTIONS_H__
#include "Functions.h"
#endif
#ifndef __LOCALNET_H__
#include "LocalNet.h"
#endif
#ifndef __MACINTOSHDIALOGS_H__
#include "MacintoshDialogs.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MACINTOSH_H__
#include "Macintosh.h"
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
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYWINDOW_H__
#include "MemoryWindow.h"
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
#ifndef __SIMULATORWINDOW_H__
#include "SimulatorWindow.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __SIMULATORSTATE_H__
#include "SimulatorState.h"
#endif
#ifndef __TESTING_H__
#include "Testing.h"
#endif
#ifndef __TEWINDOW_H__
#include "TEWindow.h"
#endif
#ifndef __VIEWSOURCE_H__
#include "ViewSource.h"
#endif

// ---------------------------------------------------------------------------

#define kCheckMark	0x12
#define kSelectMark	0x13

// ---------------------------------------------------------------------------

static Boolean ToggleMenuItemMark(short menuID, short menuItem);
static void SelectMenuItemMark(short menuID, short menuItem, short lowItem, short highItem);
static void HandleDeskAcc(short menuItem);
void AdjustMenus(ushort modifiers);

// ---------------------------------------------------------------------------
//	local functions
//
static void AdjustAppleMenu(ushort modifiers);
static void AdjustFileMenu(ushort modifiers);
static void AdjustEditMenu(ushort modifiers);
static void AdjustHardwareMenu(ushort modifiers);
static void AdjustNetworkMenu(ushort modifiers);
static void AdjustDisplayMenu(ushort modifiers);
static void AdjustTestingMenu(ushort modifiers);
static void AdjustDebugMenu(ushort modifiers);
static void AdjustMemoryMenu(ushort modifiers);
static void AdjustCacheMenu(ushort modifiers);
static void AdjustProfileMenu(ushort modifiers);
static void AdjustFunctionsMenu(ushort modifiers);
static void AdjustUserAgentMenu(ushort modifiers);
static void AdjustLocalNetWritePolicyMenu(ushort modifiers);
static void AdjustLocalNetReadPolicyMenu(ushort modifiers);
static void AdjustProportionalFontNameMenu(ushort modifiers);
static void AdjustMonospacedFontNameMenu(ushort modifiers);
static void AdjustScreenBufferMenu(ushort modifiers);
static void AdjustTVStandardMenu(ushort modifiers);

static void DoAppleMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoFileMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoEditMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoHardwareMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoNetworkMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoDisplayMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoTestingMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoDebugMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoMemoryMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoCacheMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoProfileMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoFunctionsMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoUserAgentMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoLocalNetWritePolicyMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoLocalNetReadPolicyMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoProportionalFontNameMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoMonospacedFontNameMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoScreenBufferMenuCommand(short menuID, short menuItem, ushort modifiers);
static void DoTVStandardSizeMenuCommand(short menuID, short menuItem, ushort modifiers);

// ---------------------------------------------------------------------------
//	Handy tables to make our own menus of things...
// ---------------------------------------------------------------------------

//	Keep these two in synch
enum
{
	kUserAgentArtemis = 0,
	kUserAgentMozilla_1_1_N,
	kUserAgentMozilla_1_1_N_Artemis,
	kUserAgentMozilla_2_0_N,
	kUserAgentLynx,
	kUserAgentAOL,
	// ... add more here, if desired ... 
	kUserAgentChoose,
	kNumUserAgents,
	kUserAgentDefault = kUserAgentArtemis
};

const char* kUserAgentName[/*kNumUserAgents*/] =
{
	"Artemis/0.0",					// kUserAgentArtemis
	"Mozilla/1.1N",					// kUserAgentMozilla_1_1_N
	"Mozilla/1.1N, Artemis/0.0", 	// kUserAgentMozilla_1_1_N_Artemis
	"Mozilla/2.0N",					// kUserAgentMozilla_2_0_N
	"Lynx",							// kUserAgentLynx
	"aolbrowser",					// kUserAgentAOL
	// ... add more here, if desired ... 
	"Choose…"						// kUserAgentChoose
};

// -----------------------------------------------------------------

//	Keep these two in synch

typedef struct {
	const char*	name;
	ulong		address;
	ushort		port;
	ServerType	serverType;
} SetServerRecord;

const ulong kExtraRAMSize = 3072L * 1024;
const ulong kMaxNetSpeed = 19200;

// ---------------------------------------------------------------------------

void InitializeMenus(void)
{
	Handle		menuBar = GetNewMBar(rMenuBar);		//read menus into menu bar
	MenuHandle	menu = nil;
	int i;
		
	if (menuBar == nil)
		EmergencyExit(sNoMenus);					//wow, how’d that happen?
	SetMenuBar(menuBar);							//install menus
	DisposeHandle(menuBar);
	
	AddResMenu(GetMenu(mApple), 'DRVR');			//add DA names to Apple menu
	
	BuildFunctionsMenu();

	// add system version strings to heirarchical menu
	menu = GetMenu(mUserAgent);
	InsertMenu(menu, -1);
	for (i=0; i<kNumUserAgents; i++)
	{	insertmenuitem(menu, "foo", kNumUserAgents);
		setmenuitemtext(menu, i+1, kUserAgentName[i]);
	}
	
	{
		menu = GetMenu(mProfile);
		int numProfileUnits = GetNumProfileUnits();
		for (i=0; i<numProfileUnits; i++) {
			insertmenuitem(menu, "foo", numProfileUnits + iProfileUnits);
			setmenuitemtext(menu, i + iProfileUnits, GetProfileUnitName(i));
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
#else
			DisableItem(menu, i+iProfileUnits);
#endif
		}
	}
	
	menu = GetMenu(mLocalNetWritePolicy);
	InsertMenu(menu, -1);

	menu = GetMenu(mLocalNetReadPolicy);
	InsertMenu(menu, -1);

	menu = GetMenu(mProportionalFontName);
	AppendResMenu(menu, 'FONT');
	InsertMenu(menu, -1);
		
	menu = GetMenu(mMonospacedFontName);
	AppendResMenu(menu, 'FONT');
	InsertMenu(menu, -1);
	
	menu = GetMenu(mScreenBuffer);
	InsertMenu(menu, -1);

	menu = GetMenu(mTVStandard);
	InsertMenu(menu, -1);

	DrawMenuBar();
	
	AdjustMenus(0);
}

// ---------------------------------------------------------------------------

static Boolean ToggleMenuItemMark(short menuID, short menuItem)
{
	short			markChar;
	MenuHandle		menuHdl = GetMenu(menuID);

	if (menuHdl == nil)
		EmergencyExit(sResErr);
		
	GetItemMark(menuHdl, menuItem, &markChar);
	markChar = (markChar != kCheckMark) ? kCheckMark : ' ';
	SetItemMark(menuHdl, menuItem, markChar);
	
	return markChar == kCheckMark;
}

static void SelectMenuItemMark(short menuID, short menuItem, short lowItem, short highItem)
{
	short			currentItem;
	MenuHandle		menuHdl = GetMenu(menuID);

	if (menuHdl == nil)
		EmergencyExit(sResErr);
	
	for (currentItem = lowItem; currentItem <= highItem; currentItem++)
		SetItemMark(menuHdl, currentItem, ' ');
		
	SetItemMark(menuHdl, menuItem, kSelectMark);
}



// ---------------------------------------------------------------------------

static void AdjustAppleMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mApple);
	Assert(menuHandle != nil);

	// ok, so this is a waste of time...just for completeness
}

static void AdjustFileMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mFile);
	Assert(menuHandle != nil);

	WindowPtr window = FrontWindow();
	if (IsDAWindow(window) || GetWRefCon(window) != rSimulatorWindow || IsDocWindow(window))
		EnableItem(menuHandle, iClose);
	else
		DisableItem(menuHandle, iClose);

	if (StdWindow::GetStdWindow(window) != nil)
		DisableItem(menuHandle, iPrint);
	else
		EnableItem(menuHandle, iPrint);

	if (gMacSimulator->GetIsOn())
	{
		EnableItem(menuHandle, iOpen);
		EnableItem(menuHandle, iReload);
		EnableItem(menuHandle, iReloadAll);
		EnableItem(menuHandle, iSave);
		EnableItem(menuHandle, iSaveAs);
		EnableItem(menuHandle, iSaveSimulatorState);
		EnableItem(menuHandle, iViewSource);
	}
	else
	{
		DisableItem(menuHandle, iOpen);
		DisableItem(menuHandle, iReload);
		DisableItem(menuHandle, iReloadAll);
		DisableItem(menuHandle, iSave);
		DisableItem(menuHandle, iSaveAs);
		DisableItem(menuHandle, iSaveSimulatorState);
		DisableItem(menuHandle, iViewSource);
	}
}

static void AdjustEditMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mEdit);
	Assert(menuHandle != nil);

	Boolean allowEdit = IsDAWindow(FrontWindow());
	
	if (allowEdit) {
		EnableItem(menuHandle, iUndo);
		EnableItem(menuHandle, iCut);
		EnableItem(menuHandle, iCopy);
		EnableItem(menuHandle, iClear);
		EnableItem(menuHandle, iPaste);
	} else {
		DisableItem(menuHandle, iUndo);
		DisableItem(menuHandle, iCut);
		DisableItem(menuHandle, iCopy);
		DisableItem(menuHandle, iClear);
		DisableItem(menuHandle, iPaste);
	}
	
	DisableItem(menuHandle, iSelectAll);
	DisableItem(menuHandle, iFind);
	DisableItem(menuHandle, iFindAgain);
	DisableItem(menuHandle, iFindSelection);
}

static void AdjustHardwareMenu(ushort modifiers)
{
	MenuHandle menuHandle = GetMenu(mHardware);
	Assert(menuHandle != nil);
	Str255 menuName;
	ulong RAMSize = gMacSimulator->GetRAMSize();

	snprintf((char*)menuName, sizeof(menuName), "%s (current=%dKB)",
							  (modifiers & optionKey) ? "Set RAM size…" : "Use extra RAM",
							  RAMSize/1024);
	c2pstr((char*)menuName);
	SetMenuItemText(menuHandle, iExtraRAM, menuName);

	if (gMacSimulator->GetIsOn())
		DisableItem(menuHandle, iExtraRAM);
	else
		EnableItem(menuHandle, iExtraRAM);
	CheckItem(menuHandle, iExtraRAM, 	kDefaultRAMSize < RAMSize);

	CheckItem(menuHandle, iUseROMStore,	gMacSimulator->GetUseROMStore());	
	CheckItem(menuHandle, iFidoEnabled,	gMacSimulator->GetFidoEnabled());
	CheckItem(menuHandle, iIREnabled,	gMacSimulator->GetIREnabled());
	CheckItem(menuHandle, iUseNVStore,	gMacSimulator->GetUseNVStore());
	DisableItem(menuHandle, iLEDWindow);
}

static void AdjustNetworkMenu(ushort modifiers)
{
	MenuHandle menuHandle = GetMenu(mNetwork);
	Assert(menuHandle != nil);

	Boolean usePhone = gMacSimulator->GetUsePhone();
	ulong limitNetSpeed = gMacSimulator->GetLimitNetSpeed();

	CheckItem(menuHandle, iUsePhone, usePhone);
	if (gMacSimulator->GetIsOn())
		DisableItem(menuHandle, iUsePhone);
	else
		EnableItem(menuHandle, iUsePhone);

	char buffer[256];	// limit speed buffer

	CheckItem(menuHandle, iLimitSpeed, limitNetSpeed != 0);
	
	if (usePhone) {
		DisableItem(menuHandle, iLimitSpeed);
		snprintf(buffer, sizeof(buffer), "Limit speed");
	} else {
		EnableItem(menuHandle, iLimitSpeed);
		if (modifiers & optionKey) {
			snprintf(buffer, sizeof(buffer), "Set limit speed… (current=%d)", limitNetSpeed);
		} else if (limitNetSpeed != 0) {
			snprintf(buffer, sizeof(buffer), "Limit speed (current=%d)", limitNetSpeed);
		} else {
			snprintf(buffer, sizeof(buffer), "Limit speed (current=∞)", limitNetSpeed);
		}
	}
	setmenuitemtext(menuHandle, iLimitSpeed, buffer);

	DisableItem(menuHandle, iServiceWindow);
	DisableItem(menuHandle, iSocketWindow);
	
	if ((gLocalNet != nil) && gLocalNet->GetRoot())
	{	EnableItem(menuHandle, iLocalNetWritePolicy);
		EnableItem(menuHandle, iLocalNetReadPolicy);
	}
	else
	{	DisableItem(menuHandle, iLocalNetWritePolicy);
		DisableItem(menuHandle, iLocalNetReadPolicy);
	}
}

static void AdjustDisplayMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mDisplay);
	Assert(menuHandle != nil);

	SetItemMark(menuHandle, iUseTVWDEF, gMacSimulator->GetUseTVWDEF() ? kCheckMark : ' ');
	DisableItem(menuHandle, iSaveScreenshot);
	if (gMacSimulator->GetCanUseMacFlickerFilter())
	{
		SetItemMark(menuHandle, iUseMacFlickerFilter, gMacSimulator->GetUseMacFlickerFilter() ? kCheckMark : ' ');	
		EnableItem(menuHandle,iUseMacFlickerFilter);
	}
	else
	{
		gMacSimulator->SetUseMacFlickerFilter(false);
		SetItemMark(menuHandle, iUseMacFlickerFilter, ' ');	
		DisableItem(menuHandle,iUseMacFlickerFilter);
	}	
	if (gMacSimulator->GetScreenBitMapFormat() == yuv422Format)
	{
		SetItemMark(menuHandle, iUseFlickerFilter, gSystem->GetUseFlickerFilter() ? kCheckMark : ' ');		
		EnableItem(menuHandle,iUseFlickerFilter);
	}
	else
	{	if (gSystem->GetUseFlickerFilter())
		{	gSystem->SetUseFlickerFilter(false);
		}
		SetItemMark(menuHandle, iUseFlickerFilter,' ');		
		DisableItem(menuHandle,iUseFlickerFilter);
	}
	SetItemMark(menuHandle, iUseBackBuffer, gMacSimulator->GetUseBackBuffer() ? kCheckMark : ' ');		
	SetItemMark(menuHandle, iForceAntialiasText, gMacSimulator->GetForceAntialiasText() ? kCheckMark : ' ');

	SetItemMark(menuHandle, iUseJapanese, gMacSimulator->GetUseJapanese() ? kCheckMark : ' ');		
	if (gMacSimulator->GetUseQuickdraw())
	{
		SetItemMark(menuHandle, iUseQuickdraw, kCheckMark);
		EnableItem(menuHandle, iProportionalFontName);
		EnableItem(menuHandle, iMonospacedFontName);
	}
	else
	{
		SetItemMark(menuHandle, iUseQuickdraw, ' ');
		DisableItem(menuHandle, iProportionalFontName);
		DisableItem(menuHandle, iMonospacedFontName);
	}
}

static void AdjustTestingMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mTesting);
	Assert(menuHandle != nil);

	int item = CountMItems(menuHandle);
	Boolean testingEnabled = false;
#ifdef DEBUG
	testingEnabled = gMacSimulator->GetIsOn();
#endif	
	while (item > 0)
	{	
		if ((!testingEnabled) || (item == 2))
		{	DisableItem(menuHandle, item);
		}
		else
		{	EnableItem(menuHandle, item);
		}
		item--;
	}
}

static void AdjustDebugMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mDebug);
	Assert(menuHandle != nil);

	SetItemMark(menuHandle, iTrivialMessagesEnabled,
				gMacSimulator->GetTrivialMessagesEnabled()	? kCheckMark : ' ');	
	SetItemMark(menuHandle, iLoggingEnabled,
				gMacSimulator->GetLoggingEnabled()			? kCheckMark : ' ');		
	SetItemMark(menuHandle, iStrictDebuggingEnabled,
				gMacSimulator->GetStrictDebugging()			? kCheckMark : ' ');		
#ifdef DEBUG
	SetItemMark(menuHandle, iMemoryCheckpoint,
				MemoryCheckpointed()						? kCheckMark : ' ');		
#else
	SetItemMark(menuHandle, iMemoryCheckpoint, ' ');
#endif

	SetItemMark(menuHandle, iAttractMode,
				(gMacSimulator->GetIsOn()
					&& gMacSimulator->GetInAttractMode() )	? kCheckMark : ' ');		
	SetItemMark(menuHandle, iDemoMode,
				gMacSimulator->GetInDemoMode()              ? kCheckMark : ' ');		
#ifdef DEBUG
	SetItemMark(menuHandle, iComplain,
				gMacSimulator->GetPreventComplains()		? ' ' : kCheckMark);
	EnableItem(menuHandle, iComplain);
#endif

#ifdef DEBUG
	if (gMacSimulator->GetIsOn())
	{
		EnableItem(menuHandle, iMemoryCheckpoint);
		if (MemoryCheckpointed())
			EnableItem(menuHandle, iMemoryDifference);
		else
			DisableItem(menuHandle, iMemoryDifference);
	}
	else
#endif /* DEBUG */
	{	DisableItem(menuHandle, iMemoryCheckpoint);
		DisableItem(menuHandle, iMemoryDifference);
	}

	if (gMacSimulator->GetIsOn())
		EnableItem(menuHandle, iAttractMode);
	else
		DisableItem(menuHandle, iAttractMode);

#ifdef DEBUG_MONKEY
	CheckItem(menuHandle, iMonkeyMode, gMonkeyEnable);
	EnableItem(menuHandle, iMonkeyMode);
	SetMenuItemText(menuHandle, iMonkeyMode, "\pMonkey Mode");
#endif
	DisableItem(menuHandle, iMiscStatWindow); /* it will turn itself on */
}

static void AdjustMemoryMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mMemory);
	Assert(menuHandle != nil);

#ifdef DEBUG_MEMORYWINDOW
	EnableItem(menuHandle, iMemoryWindow);
	SetMenuItemText(menuHandle, iMemoryWindow, "\pMemoryWindow");
#endif /* DEBUG_MEMORYWINDOW */
#if !(defined(DEBUG) && defined(MEMORY_TRACKING))
	DisableItem(menuHandle, iMemoryCheckpoint);
	DisableItem(menuHandle, iMemoryDifference);
#endif
	DisableItem(menuHandle, iSeismograph);		// ...if these are around,
	DisableItem(menuHandle, iGlanceByURL);		// windows will enable themselves
	DisableItem(menuHandle, iGlanceByClass);	// ...if they didn't get allocated
	DisableItem(menuHandle, iGlanceByTag);		// menu items will stay disabled
}

static void AdjustCacheMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mCache);
	Assert(menuHandle != nil);

	DisableItem(menuHandle, iCacheWindow);	// CacheWindow can turn it on
#ifdef DEBUG_CACHE_VIEWASHTML
	if (gMacSimulator->GetIsOn()) {
		EnableItem(menuHandle, iClientViewCacheAsHTML);
	} else {
		DisableItem(menuHandle, iClientViewCacheAsHTML);
	}
	SetMenuItemText(menuHandle, iClientViewCacheAsHTML, "\pClient:ViewCacheAsHTML");
#endif
	if (gMacSimulator->GetIsOn()) {
		EnableItem(menuHandle, iCachePurgeAll);
	} else {
		DisableItem(menuHandle, iCachePurgeAll);
	}
#ifdef DEBUG_CACHE_SCRAMBLE
	CheckItem(menuHandle, iCacheScramble, gMacSimulator->GetCacheScramble());
	EnableItem(menuHandle, iCacheScramble);
	SetMenuItemText(menuHandle, iCacheScramble, "\pScramble");
#endif /* DEBUG_CACHE_SCRAMBLE */

#ifdef DEBUG_CACHE_VALIDATE
	CheckItem(menuHandle, iCacheValidate, gMacSimulator->GetCacheValidate());
	EnableItem(menuHandle, iCacheValidate);
	SetMenuItemText(menuHandle, iCacheValidate, "\pValidate");
#endif /* DEBUG_CACHE_VALIDATE */
}

static void AdjustProfileMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mProfile);
	Assert(menuHandle != nil);

#ifdef DEBUG_PERFDUMP
	CheckItem(menuHandle, iPerfDump, GetPerfDumpActive());
	SetMenuItemText(menuHandle, iPerfDump, "\pPerfDump");
	EnableItem(menuHandle, iPerfDump);
#endif /* DEBUG_PERFDUMP */
	
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
	CheckItem(menuHandle, iProfile, gMacSimulator->GetIsProfiling());
	SetMenuItemText(menuHandle, iProfile, "\pProfiling");
	EnableItem(menuHandle, iProfile);

	int numProfileUnits = GetNumProfileUnits();
	for (int i=0; i<numProfileUnits; i++) {
		CheckItem(menuHandle, i + iProfileUnits, GetProfileUnitOn(i));
	}
#endif
}

static void AdjustFunctionsMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mFunctions);
	Assert(menuHandle != nil);
	
	// why not give the functions menu a chance?
}

static void AdjustUserAgentMenu(ushort UNUSED(modifiers))
{
	/*
	MenuHandle menuHandle = GetMenu(mUserAgent);
	Assert(menuHandle != nil);

	long userAgentID = gMacSimulator->GetUserAgentNumber();
	for (short item=CountMItems(menuHandle); item>0; item--)
	{
		SetItemMark(menuHandle, item, (item==userAgentID+1) ? kCheckMark : ' ');
	}
	*/
	MenuHandle menuHandle = GetMenu(mUserAgent);
	Assert(menuHandle != nil);
	Boolean chooseSelected = true;
	char buffer[256];

	const char* userAgentName = gMacSimulator->GetUserAgent();
	for (short item=0; item<kNumUserAgents-1; item++)
	{
		char mark = ' ';
		if (strcmp(userAgentName, kUserAgentName[item]) == 0)
		{	chooseSelected = false;
			mark = kCheckMark;
		}
		SetItemMark(menuHandle, item+1, mark);
	}

	if (chooseSelected)
	{	snprintf(buffer, sizeof(buffer), "%s (%s)",
			kUserAgentName[kUserAgentChoose], userAgentName);
	}
	else
	{	snprintf(buffer, sizeof(buffer), "%s",
			kUserAgentName[kUserAgentChoose]);
	}
	setmenuitemtext(menuHandle, kUserAgentChoose+1, buffer);
	SetItemMark(menuHandle, kUserAgentChoose+1, chooseSelected ? kCheckMark : ' ');
}
static void AdjustLocalNetWritePolicyMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mLocalNetWritePolicy);
	Assert(menuHandle != nil);

	Boolean writeToLocalNet = (gLocalNet != nil)
								&& gLocalNet->GetRoot()
								&& gLocalNet->GetActiveWrite();
	SetItemMark(menuHandle, iDontWriteToLocalNet,
				(!writeToLocalNet)                ? kCheckMark : ' ');
	SetItemMark(menuHandle, iWriteToLocalNet,
				writeToLocalNet                   ? kCheckMark : ' ');
}
static void AdjustLocalNetReadPolicyMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mLocalNetReadPolicy);
	Assert(menuHandle != nil);

	Boolean readFromLocalNet = (gLocalNet != nil)
								&& gLocalNet->GetRoot()
								&& gLocalNet->GetActiveRead();
	Boolean readFromLocalNetOnly = readFromLocalNet
								&& (gLocalNet != nil) 
								&& gLocalNet->GetExclusiveRead();
	SetItemMark(menuHandle, iDontReadFromLocalNet,
				(!readFromLocalNet)                         ? kCheckMark : ' ');
	SetItemMark(menuHandle, iReadFromLocalNetThenNet,
				(readFromLocalNet && !readFromLocalNetOnly) ? kCheckMark : ' ');
	SetItemMark(menuHandle, iReadFromLocalNetOnly,
				readFromLocalNetOnly                        ? kCheckMark : ' ');
}

static void AdjustProportionalFontNameMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mProportionalFontName);
	Assert(menuHandle != nil);

	Str255 itemString;
	
	ulong fontFamilyID = gSystem->GetFontProportional();
	short shortFontID;

	int count = CountMItems(menuHandle);
	
	for (int item=1; item<=count; item++)
	{
		GetMenuItemText(menuHandle, item, itemString);
		GetFNum(itemString, &shortFontID);
		SetItemMark(menuHandle, item, (fontFamilyID == shortFontID) ? kCheckMark : ' ');
	} 
}

static void AdjustMonospacedFontNameMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mMonospacedFontName);
	Assert(menuHandle != nil);

	Str255 itemString;
	
	ulong fontFamilyID = gSystem->GetFontMonospaced();
	short shortFontID;
	
	int count = CountMItems(menuHandle);
	
	for (int item=1; item<=count; item++)
	{
		GetMenuItemText(menuHandle, item, itemString);
		GetFNum(itemString, &shortFontID);
		SetItemMark(menuHandle, item, (fontFamilyID == shortFontID) ? kCheckMark : ' ');
	} 
}

static void AdjustScreenBufferMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mScreenBuffer);
	Assert(menuHandle != nil);

	BitMapFormat bitMapFormat = gMacSimulator->GetScreenBitMapFormat();

	if (gMacSimulator->GetIsOn())
	{
		DisableItem(menuHandle, iYUV422);
		DisableItem(menuHandle, iYUV);
		DisableItem(menuHandle, iRGB16);
		DisableItem(menuHandle, iRGB32);
	}
	else
	{
		EnableItem(menuHandle, iYUV422);
		EnableItem(menuHandle, iYUV);
		EnableItem(menuHandle, iRGB16);
		EnableItem(menuHandle, iRGB32);
	}
	SetItemMark(menuHandle, iYUV422, (bitMapFormat==yuv422Format) ? kCheckMark : ' ');
	SetItemMark(menuHandle, iYUV,    (bitMapFormat==yuvFormat) ? kCheckMark : ' ');
	SetItemMark(menuHandle, iRGB16,  (bitMapFormat==rgb16Format) ? kCheckMark : ' ');
	SetItemMark(menuHandle, iRGB32,  (bitMapFormat==rgb32Format) ? kCheckMark : ' ');
}

static void AdjustTVStandardMenu(ushort UNUSED(modifiers))
{
	MenuHandle menuHandle = GetMenu(mTVStandard);
	Assert(menuHandle != nil);
	
	DisplayMode displayMode = gMacSimulator->GetDisplayMode();
	
	if (gMacSimulator->GetIsOn())
	{	DisableItem(menuHandle, iNTSC);
		DisableItem(menuHandle, iPAL);
	}
	else
	{	EnableItem(menuHandle, iNTSC);
		EnableItem(menuHandle, iPAL);
	}
	
	SetItemMark(menuHandle, iNTSC,  (displayMode == kDisplayModeNTSC)  ? kCheckMark : ' ');
	SetItemMark(menuHandle, iPAL,   (displayMode == kDisplayModePAL)   ? kCheckMark : ' ');
}

// ---------------------------------------------------------------------------

void AdjustMenus(ushort modifiers)
{
	AdjustAppleMenu(modifiers);
	AdjustFileMenu(modifiers);
	AdjustEditMenu(modifiers);
	AdjustHardwareMenu(modifiers);
	AdjustNetworkMenu(modifiers);
	AdjustDisplayMenu(modifiers);
	AdjustTestingMenu(modifiers);
	AdjustDebugMenu(modifiers);
	AdjustMemoryMenu(modifiers);
	AdjustCacheMenu(modifiers);
	AdjustProfileMenu(modifiers);
	AdjustFunctionsMenu(modifiers);
	AdjustUserAgentMenu(modifiers);
	//AdjustSetServerMenu(modifiers);
	AdjustLocalNetWritePolicyMenu(modifiers);
	AdjustLocalNetReadPolicyMenu(modifiers);
	AdjustProportionalFontNameMenu(modifiers);
	AdjustMonospacedFontNameMenu(modifiers);
	AdjustScreenBufferMenu(modifiers);
	AdjustTVStandardMenu(modifiers);
}

// ---------------------------------------------------------------------------

static void DoAppleMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mApple);
	switch (menuItem)
	{
		case iAbout:
			HandleAbout();
			break;
		
		default:					//all non-About items in this menu are DAs
		{
			Str255 daName;
			GetItem(GetMenu(mApple), menuItem, daName);
			(void)OpenDeskAcc(daName);
		}
			break;
	}	
}

static void DoFileMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mFile);
	switch (menuItem)
	{
		case iNew:
			TEWindow* teWindow = newMac(TEWindow);
			teWindow->SetTitle("Untitled");
			if (!(teWindow->GetVisible()))
			{	teWindow->ShowWindow();
			}
			break;
		case iOpen:
			HandleOpen();
			break;
		case iReload:
			HandleReload();
			break;
		case iReloadAll:
			HandleReloadAll();
			break;
		case iClose:
			DoCloseWindow(FrontWindow());
			break;
		case iSave:
			//if ((resource = gPageViewer->GetResource()) != nil)
			//	if ((url = resource->GetURL()) != nil)
			//		SavePageToDisk(url, url);
			//break;
		case iSaveAs:
			{
				const Resource* resource = gPageViewer->GetResource();
				if (resource != nil && resource->HasURL())
				{	
					char* url = resource->CopyURL("iSaveAs");
					char* partialURL = strrchr(url, ':');
					if (partialURL == nil)
					{	partialURL = url;
					}
					else
					{	partialURL++;	// move past ':'
					}
					FSSpec destFile;
					if (RequestSaveFile(&destFile, partialURL))
					{	SavePageToDisk(&destFile, url);
					}
					FreeTaggedMemory(url, "iSaveAs");
				}
			}
			break;
		case iSaveSimulatorState:
			FSSpec simStateFile;
			if (RequestSaveFile(&simStateFile, "SimulatorState"))
			{	SaveSimulatorStateAs(&simStateFile);
			}
			break;
		case iViewSource:
			ViewSource();
			break;
		case iPrint:
			break;
		case iQuit:
			Terminate();
			break;
	}
}

static void DoEditMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mEdit);	//call SystemEdit for DA editing && MultiFinder
	if (! SystemEdit(menuItem - 1))
	{
		if (IsDocWindow(FrontWindow()))
		{
			switch (menuItem)
			{
				case iCut:
					break;
				case iCopy:
					break;
				case iPaste:
					break;
				case iClear:
					break;
			}
		}
	}
	if (menuItem == iSavePrefs)
	{
		StdWindow::SavePrefsAll();
		if (gMacSimulator != nil)
			gMacSimulator->SavePreferences();
		if (gLocalNet != nil)
			gLocalNet->SavePrefs();
	}
}

static void DoHardwareMenuCommand(short menuID, short menuItem, ushort modifiers)
{
	Assert(menuID == mHardware);
	switch (menuItem)
	{
		case iExtraRAM:
			ulong RAMSize = gMacSimulator->GetRAMSize();
			if (modifiers & optionKey)
			{
				char buffer[256];
				ulong newSize;
				snprintf(buffer, sizeof(buffer), "%d", RAMSize/1024);
				paramtext("Enter new RAM size in KB", "", "", "");
				QueryUser(rGenericQueryDialog, buffer);
				if (sscanf(buffer, "%lu", &newSize) == 1)
					RAMSize = newSize*1024L;
			}
			else if (kDefaultRAMSize == RAMSize)
			{	RAMSize = kExtraRAMSize;
			}
			else
			{	RAMSize = kDefaultRAMSize;
			}
			gMacSimulator->SetRAMSize(RAMSize);
			break;
		case iUseROMStore:
			gMacSimulator->SetUseROMStore(ToggleMenuItemMark(menuID, menuItem));
			break;
		case iFidoEnabled:
			gMacSimulator->SetFidoEnabled(ToggleMenuItemMark(menuID, menuItem));
			break;
		case iIREnabled:
			gMacSimulator->SetIREnabled(ToggleMenuItemMark(menuID, menuItem));
			break;
		case iUseNVStore:
			gMacSimulator->SetUseNVStore(ToggleMenuItemMark(menuID, menuItem));
			break;
		case iNewRemote:
			NewRemoteWindow(false);
			break;
		case iPower:
			if (gMacSimulator->GetIsOn())
				gMacSimulator->PowerOff();
			else
				gMacSimulator->PowerOn();
	}
}

static void DoNetworkMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort modifiers)
{
	Assert(menuID == mNetwork);
	switch (menuItem)
	{
		case iConnectTo:
			SetServerConfiguration();
			break;
		case iUsePhone:
			gMacSimulator->SetUsePhone(!gMacSimulator->GetUsePhone());
			break;
		case iLimitSpeed:
			if (modifiers & optionKey)
			{
				char buffer[256];
				ulong limitSpeed = gMacSimulator->GetLimitNetSpeed();
				
				snprintf(buffer, sizeof(buffer), "%d", limitSpeed);
				paramtext("Enter new maximum baud rate", "", "", "");
				if (QueryUser(rGenericQueryDialog, buffer))
				{
					if (sscanf(buffer, "%lu", &limitSpeed))
					{
						char* fraction = strchr(buffer, '.');
						if (fraction != nil)
						{
							if (limitSpeed < 300)
								limitSpeed *= 1000;
							fraction++;
							if (isdigit(*fraction))
								limitSpeed += (*fraction++ - '0')*100;
							if (isdigit(*fraction))
								limitSpeed += (*fraction++ - '0')*10;
							if (isdigit(*fraction))
								limitSpeed += *fraction++ - '0';
						}
					}
					gMacSimulator->SetLimitNetSpeed(limitSpeed);
				}
			}
			else if (gMacSimulator->GetLimitNetSpeed() == 0)
				gMacSimulator->SetLimitNetSpeed((ulong)kMaxNetSpeed);
			else
				gMacSimulator->SetLimitNetSpeed((ulong)0);
			break;
		case iSetLocalNetDirectory:
			if (!IsWarning(gLocalNet == nil))
				gLocalNet->SetRoot();
			break;
	}
}

static void DoDisplayMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mDisplay);
	
	Boolean refreshDisplay = false;
	FontSizeRecord fontSizeRecord;
	Boolean getUseTVWDEF = false;
	
	switch (menuItem)
	{
		case iUseTVWDEF:
			getUseTVWDEF = gMacSimulator->GetUseTVWDEF();
			gMacSimulator->SetUseTVWDEF(!getUseTVWDEF);
			if ((!getUseTVWDEF) && (gSimulatorWindow != nil))
				gSimulatorWindow->CenterWindowOnScreen();
			break;
		case iUseMacFlickerFilter:
			gMacSimulator->SetUseMacFlickerFilter(!gMacSimulator->GetUseMacFlickerFilter());
			if (gMacSimulator->GetUseMacFlickerFilter())
			{	if (gSystem->GetUseFlickerFilter())
					gSystem->SetUseFlickerFilter(false);
				refreshDisplay = true;
			}
			break;
		case iUseFlickerFilter:
			gSystem->SetUseFlickerFilter(!gSystem->GetUseFlickerFilter());
			refreshDisplay = true;
			if (gSystem->GetUseFlickerFilter())
			{	if (gMacSimulator->GetUseMacFlickerFilter())
					gMacSimulator->SetUseMacFlickerFilter(false);
			}
			break;
		case iUseBackBuffer:
			gMacSimulator->SetUseBackBuffer(!gMacSimulator->GetUseBackBuffer());
			break;
		case iForceAntialiasText:
			gMacSimulator->SetForceAntialiasText(!gMacSimulator->GetForceAntialiasText());
			refreshDisplay = true;
			break;
		case iUseQuickdraw:
			gMacSimulator->SetUseQuickdraw(!gMacSimulator->GetUseQuickdraw());
			if (gPageViewer != nil)
				gPageViewer->ReLayout();
			break;
		case iUseJapanese:
			gMacSimulator->SetUseJapanese(!gMacSimulator->GetUseJapanese());
			break;
		case iProportionalFontSize:
			fontSizeRecord = *(gSystem->GetFontProportionalSizeRecord());
			if (SetFontSizeRecord(&fontSizeRecord, gSystem->GetFontProportional(), &kDefaultFontProportionalSizeRecord)) {
				gSystem->SetFontProportionalSizeRecord(&fontSizeRecord);
				if (gPageViewer != nil)
					gPageViewer->ReLayout();
			}
			break;
		case iMonospacedFontSize:
			fontSizeRecord = *(gSystem->GetFontMonospacedSizeRecord());
			if (SetFontSizeRecord(&fontSizeRecord, gSystem->GetFontMonospaced(), &kDefaultFontMonospacedSizeRecord)) {
				gSystem->SetFontMonospacedSizeRecord(&fontSizeRecord);
				if (gPageViewer != nil)
					gPageViewer->ReLayout();
			}
			break;
	}
	if (refreshDisplay)
	{
		Rectangle	all;
		if ( gScreen )
			gScreen->GetBounds(&all);
		if ( gPageViewer )
			gPageViewer->InvalidateBounds(&all);
	}
}

static void DoTestingMenuCommand(short USED_FOR_DEBUG(menuID), short USED_FOR_DEBUG(menuItem), ushort UNUSED(modifiers))
{
	Assert(menuID == mTesting);
#ifdef DEBUG
	ExecuteTest((unsigned long)menuItem);
#endif
	// SetItemMark(GetMenu(menuID), menuItem, kCheckMark);
}

static void DoDebugMenuCommand(short menuID, short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mDebug);
	switch (menuItem) {
		case iTrivialMessagesEnabled:
			gMacSimulator->SetTrivialMessagesEnabled(ToggleMenuItemMark(menuID, menuItem));
			break;
			
		case iLoggingEnabled:
			gMacSimulator->SetLoggingEnabled(ToggleMenuItemMark(menuID, menuItem));
			break;
			
		case iStrictDebuggingEnabled:
			gMacSimulator->SetStrictDebugging(ToggleMenuItemMark(menuID, menuItem));
			break;
			
		case iAttractMode:
			gMacSimulator->SetInAttractMode(ToggleMenuItemMark(menuID, menuItem));
			break;
			
		case iDemoMode:
			gMacSimulator->SetInDemoMode(ToggleMenuItemMark(menuID, menuItem));
			break;
		
#ifdef DEBUG_MONKEY
		case iMonkeyMode:
			gMonkeyEnable = !gMonkeyEnable;
			break;			
#endif /* DEBUG_MONKEY */
		
#ifdef DEBUG
		case iComplain:
			gMacSimulator->SetPreventComplains(!gMacSimulator->GetPreventComplains());
			break;
#endif /* DEBUG */
	}
}

static void DoMemoryMenuCommand(short USED_FOR_DEBUG(menuID),
								short USED_FOR_DEBUG(menuItem),
								ushort modifiers)
{
	Boolean optionKeyPressed = ((modifiers & optionKey) != 0);
#if defined(DEBUG) && defined(MEMORY_TRACKING)
	Assert(menuID == mMemory);

	switch (menuItem) {
#ifdef DEBUG_MEMORYWINDOW
		case iMemoryWindow:
			{
				ulong	baseOffset = 0;
				ulong	length = gMacSimulator->GetRAMSize();
				char	queryResult1[256];
				char	queryResult2[256];
				
				if (optionKeyPressed) {
					snprintf(queryResult1, sizeof(queryResult1), "%d", baseOffset);
					snprintf(queryResult2, sizeof(queryResult2), "%d", length);
					if (!QueryUser2(rMemoryRangeID, queryResult1, queryResult2))
						break;
					sscanf(queryResult1, "%d", &baseOffset);
					sscanf(queryResult2, "%d", &length);
				}
				NewMemoryWindow(0, 0, baseOffset, length, (ulong)-1);
			}
			break;
#endif /* DEBUG_MEMORYWINDOW */

		case iMemoryCheckpoint:
			HandleNewMemoryCheckpoint();
			break;
			
		case iMemoryDifference:
			ReportMemoryDifference();
			break;
	}
#endif /* defined(DEBUG) && defined(MEMORY_TRACKING) */
}

static void DoCacheMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mCache);
	switch (menuItem) {
#ifdef DEBUG_CACHE_VIEWASHTML
		case iClientViewCacheAsHTML:
			ExecuteClientFunction("client:ViewCacheAsHTML", nil);
			break;
#endif /* DEBUG_CACHE_VIEWASHTML */
#ifdef DEBUG_CACHE_SCRAMBLE
		case iCacheScramble:
			gMacSimulator->SetCacheScramble(!gMacSimulator->GetCacheScramble());
			break;
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
		case iCacheValidate:
			gMacSimulator->SetCacheValidate(!gMacSimulator->GetCacheValidate());
			break;
#endif /* DEBUG_CACHE_VALIDATE */
		case iCachePurgeAll:
			gRAMCache->PurgeAll();
			break;
	}
}

static void DoProfileMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mProfile);
#ifdef DEBUG_PERFDUMP
	if (menuItem == iPerfDump) {
		if (GetPerfDumpActive()) {
			PerfDumpFinalize();
		} else {
			PerfDumpInitialize();
		}	
	} else
#endif /* DEBUG_PERFDUMP */	
	if (menuItem == iProfile) {
		SetProfileUnitEnabled(!GetProfileUnitEnabled());
	} else if (menuItem >= iProfileUnits) {
		int index = menuItem - iProfileUnits;
		Boolean isOn = GetProfileUnitOn(index);
		SetProfileUnitOn(index, !isOn);	
	}
}

static void DoFunctionsMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mFunctions);
	HandleFunctionsMenu(menuItem);
}

static void DoUserAgentMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	static Str255 defaultUserAgentName;
	Assert(menuID == mUserAgent);
	if (menuItem-1 != kUserAgentChoose)
	{	gMacSimulator->SetUserAgent(kUserAgentName[menuItem-1]);
	}
	else
	{
		// get user agent name from user
		char newUserAgent[256];
		snprintf(newUserAgent, sizeof(newUserAgent),
				"%.*s", sizeof(newUserAgent)-1, gMacSimulator->GetUserAgent());
		
		paramtext("Choose a user agent:", "", "", "");
		if (QueryUser(rGenericQueryDialog, newUserAgent))
		{	gMacSimulator->SetUserAgent(newUserAgent);
		}
	}
}

static void DoLocalNetWritePolicyMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mLocalNetWritePolicy);

	switch (menuItem) {
		case iDontWriteToLocalNet:
			if (!IsWarning(gLocalNet == nil))
				gLocalNet->SetActiveWrite(false);
			break;
		case iWriteToLocalNet:
			if (!IsWarning(gLocalNet == nil))
				gLocalNet->SetActiveWrite(true);
			break;
	}
}

static void DoLocalNetReadPolicyMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mLocalNetReadPolicy);

	switch (menuItem) {
		case iDontReadFromLocalNet:
			if (!IsWarning(gLocalNet == nil)) {
				gLocalNet->SetActiveRead(false);
				gLocalNet->SetExclusiveRead(false);
			}
			break;
		case iReadFromLocalNetThenNet:
			if (!IsWarning(gLocalNet == nil)) {
				gLocalNet->SetActiveRead(true);
				gLocalNet->SetExclusiveRead(false);
			}
			break;
		case iReadFromLocalNetOnly:
			if (!IsWarning(gLocalNet == nil)) {
				gLocalNet->SetActiveRead(true);
				gLocalNet->SetExclusiveRead(true);
			}
			break;
	}
}

static void DoProportionalFontNameMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mProportionalFontName);
	MenuHandle menuHandle = GetMenu(mProportionalFontName);

	Str255 itemString;
	GetMenuItemText(menuHandle, menuItem, itemString);
	
	ulong fontFamilyID = gSystem->GetFontProportional();

	short shortFontID;
	GetFNum(itemString, &shortFontID);
	fontFamilyID = shortFontID;

	gSystem->SetFontProportional(fontFamilyID);

	short count = CountMItems(menuHandle);
	while (count > 0)
	{
		SetItemMark(menuHandle, count, (count==menuItem) ? kCheckMark : ' ');
		count--;
	}
	if (gPageViewer != nil)
		gPageViewer->ReLayout();
}

static void DoMonospacedFontNameMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mMonospacedFontName);
	MenuHandle menuHandle = GetMenu(mMonospacedFontName);

	Str255 itemString;
	GetMenuItemText(menuHandle, menuItem, itemString);
	
	ulong fontFamilyID = gSystem->GetFontMonospaced();

	short shortFontID;
	GetFNum(itemString, &shortFontID);
	fontFamilyID = shortFontID;

	gSystem->SetFontMonospaced(fontFamilyID);

	short count = CountMItems(menuHandle);
	while (count > 0)
	{
		SetItemMark(menuHandle, count, (count==menuItem) ? kCheckMark : ' ');
		count--;
	}
	if (gPageViewer != nil)
		gPageViewer->ReLayout();
}

static void DoScreenBufferMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mScreenBuffer);
	
	switch (menuItem)
	{
		case iYUV422:
			gMacSimulator->SetScreenBitMapFormat(yuv422Format);
			if ( gMacSimulator->GetIsInitialized() )
				gMacSimulator->InitializeScreenRAM();
			break;
		
		case iYUV:
			gMacSimulator->SetScreenBitMapFormat(yuvFormat);
			if ( gMacSimulator->GetIsInitialized() )
				gMacSimulator->InitializeScreenRAM();
			break;
		
		case iRGB16:
			gMacSimulator->SetScreenBitMapFormat(rgb16Format);
			if ( gMacSimulator->GetIsInitialized() )
				gMacSimulator->InitializeScreenRAM();
			break;
		
		case iRGB32:
			gMacSimulator->SetScreenBitMapFormat(rgb32Format);
			if ( gMacSimulator->GetIsInitialized() )
				gMacSimulator->InitializeScreenRAM();
			break;
	}
}

static void DoTVStandardMenuCommand(short USED_FOR_DEBUG(menuID), short menuItem, ushort UNUSED(modifiers))
{
	Assert(menuID == mTVStandard);
	
	switch (menuItem)
	{
		case iNTSC:
			gMacSimulator->SetDisplayMode(kDisplayModeNTSC);
			if (gSimulatorWindow != nil)
			{	gSimulatorWindow->SetDisplayMode(kDisplayModeNTSC);
			}
			break;

		case iPAL:
			gMacSimulator->SetDisplayMode(kDisplayModePAL);
			if (gSimulatorWindow != nil)
			{	gSimulatorWindow->SetDisplayMode(kDisplayModePAL);
			}
			break;
	}
}

// ---------------------------------------------------------------------------

void DoMenuCommand(long menuResult, ushort modifiers)
{
	short menuID = HiWord(menuResult);
	short menuItem = LoWord(menuResult);

	switch (menuID) {

		case mApple:
			DoAppleMenuCommand(menuID, menuItem, modifiers);
			break;
		case mFile:
			DoFileMenuCommand(menuID, menuItem, modifiers);
			break;
		case mEdit:
			DoEditMenuCommand(menuID, menuItem, modifiers);
			break;
		case mHardware:
			DoHardwareMenuCommand(menuID, menuItem, modifiers);
			break;
		case mNetwork:
			DoNetworkMenuCommand(menuID, menuItem, modifiers);
			break;
		case mDisplay:					
			DoDisplayMenuCommand(menuID, menuItem, modifiers);
			break;
		case mTesting:
			DoTestingMenuCommand(menuID, menuItem, modifiers);
			break;
		case mDebug:
			DoDebugMenuCommand(menuID, menuItem, modifiers);
			break;
		case mMemory:
			DoMemoryMenuCommand(menuID, menuItem, modifiers);
			break;
		case mCache:
			DoCacheMenuCommand(menuID, menuItem, modifiers);
			break;
		case mProfile:
			DoProfileMenuCommand(menuID, menuItem, modifiers);
			break;
		case mFunctions:
			DoFunctionsMenuCommand(menuID, menuItem, modifiers);
			break;
		//case mSetServer:
		//	DoSetServerMenuCommand(menuID, menuItem, modifiers);
		//	break;
		case mUserAgent:
			DoUserAgentMenuCommand(menuID, menuItem, modifiers);
			break;
		case mLocalNetWritePolicy:
			DoLocalNetWritePolicyMenuCommand(menuID, menuItem, modifiers);
			break;
		case mLocalNetReadPolicy:
			DoLocalNetReadPolicyMenuCommand(menuID, menuItem, modifiers);
			break;
		case mProportionalFontName:
			DoProportionalFontNameMenuCommand(menuID, menuItem, modifiers);
			break;
		case mMonospacedFontName:
			DoMonospacedFontNameMenuCommand(menuID, menuItem, modifiers);
			break;
		case mScreenBuffer:
			DoScreenBufferMenuCommand(menuID, menuItem, modifiers);
			break;
		case mTVStandard:
			DoTVStandardMenuCommand(menuID, menuItem, modifiers);
			break;
	}

	HiliteMenu(0);						//unhighlight what MenuSelect or MenuKey hilited
}

