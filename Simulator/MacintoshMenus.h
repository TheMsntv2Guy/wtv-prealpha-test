// ===========================================================================
//	MacintoshMenus.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MACINTOSHMENUS_H__
#define __MACINTOSHMENUS_H__




// ---------------------------------------------------------------------------
//	External interface
// ---------------------------------------------------------------------------

void InitializeMenus(void);
void AdjustMenus(ushort modifiers);
void DoMenuCommand(long menuResult, ushort modifiers);


#define IsArrowKey(key)	 (key == '\34' || key == '\35' || key == '\36' || key == '\37')

// ---------------------------------------------------------------------------
//	The following constants are used to identify menus and items. The menu IDs
//	have an “m” prefix and the item numbers within each menu have an “i” prefix.
// ---------------------------------------------------------------------------

enum {
	mApple					= 128,		//Apple menu and items
	iAbout					= 1,

	mFile					= 129,		//File menu and items
	iNew					= 1,
	iOpen					= 2,
	// ---- divider ---- = 3,
	iReload					= 4,
	iReloadAll				= 5,
	iClose					= 6,
	iSave					= 7,
	iSaveAs					= 8,
	iSaveSimulatorState		= 9,
	// ---- divider ---- = 10,
	iViewSource				= 11,
	// ---- divider ---- = 12,
	iPageSetup				= 13,
	iPrint					= 14,
	// ---- divider ---- = 15,
	iQuit					= 16,

	mEdit					= 130,		//Edit menu and items
	iUndo					= 1,
	// ---- divider ---- = 2,
	iCut					= 3,
	iCopy					= 4,
	iPaste					= 5,
	iClear					= 6,
	// ---- divider ---- = 7,
	iSelectAll				= 8,
	// ---- divider ---- = 9,
	iFind					= 10,
	iFindAgain				= 11,
	iFindSelection			= 12,
	// ---- divider ---- = 13,
	iSavePrefs				= 14,

	mHardware				= 131,
	iExtraRAM				= 1,
	iUseROMStore			= 2,
	iFidoEnabled			= 3,
	iIREnabled				= 4,
	iUseNVStore				= 5,	
	// ---- divider ---- = 6,
	iLEDWindow				= 7,
	iNewRemote				= 8,
	// ---- divider ---- = 9,
	iPower					= 10,

	mNetwork				= 132,
	iConnectTo				= 1,
	iSetUserAgent			= 2,
	// ---- divider ---- = 3,
	iUsePhone				= 4,
	iLimitSpeed				= 5,
	// ---- divider ---- = 6,
	iServiceWindow			= 7,
	iSocketWindow			= 8,
	// ---- divider ---- = 9,
	iSetLocalNetDirectory	= 10,
	iLocalNetWritePolicy	= 11,
	iLocalNetReadPolicy		= 12,
	
	mDisplay				= 133,		//Display configuration
	iUseTVWDEF				= 1,
	iSaveScreenshot			= 2,
	// ---- divider ---- = 3,
	iUseMacFlickerFilter	= 4,
	iUseFlickerFilter		= 5,
	iUseBackBuffer			= 6,
	iForceAntialiasText		= 7,
	// ---- divider ---- = 8,
	iUseQuickdraw			= 9,
	iUseJapanese			= 10,
	iProportionalFontName	= 11,
	iMonospacedFontName		= 12,
	// ---- divider ---- = 13,
	iProportionalFontSize	= 14,
	iMonospacedFontSize		= 15,
	iScreenBuffer			= 16,
	iTVStandard				= 17,
	
	mTesting				= 134,		//Test execution
	iTestAll				= 1,
	// ---- divider ---- = 2,
	iTestMemoryManager		= 3,
	iTestObjectStore		= 4,
	iTestUtilities			= 5,
	iTestProtocols			= 6,
	iTestTCP				= 7,

	mDebug					= 135,		//Debugging utilities
	iMessageWindow			= 1,
	iTrivialMessagesEnabled	= 2,
	iLoggingEnabled			= 3,
	iStrictDebuggingEnabled	= 4,
	// ---- divider ---- = 5,
	iAttractMode			= 6,
	iDemoMode				= 7,
	iMonkeyMode				= 8,
	iComplain				= 9,
	// ---- divider ---- = 10,
	iMiscStatWindow			= 11,
	
	mMemory					= 136,
	iMemoryWindow			= 1,
	iMemoryCheckpoint		= 2,
	iMemoryDifference		= 3,
	// ---- divider ---- = 4,
	iSeismograph			= 5,
	iGlanceByURL			= 6,
	iGlanceByClass 			= 7,
	iGlanceByTag			= 8,

	mCache					= 137,
	iCacheWindow			= 1,
	iClientViewCacheAsHTML	= 2,
	// ---- divider ---- = 3,
	iCacheScramble			= 4,
	iCacheValidate			= 5,
	// ---- divider ---- = 6,
	iCachePurgeAll			= 7,

	mProfile				= 138,	
	iPerfDump				= 1,
	// ---- divider ---- = 2,
	iProfile				= 3,
	// ---- divider ---- = 4,
	iProfileUnits			= 5,

	mFunctions				= 139,		//Builtin functions
	
	// Heirarchical sub-menus
	
	mSetServer				= 200,

	mUserAgent				= 201,		//Mozilla, Artemis, etc.

	mLocalNetWritePolicy	= 202,
	iDontWriteToLocalNet	= 1,
	iWriteToLocalNet		= 2,
	
	mLocalNetReadPolicy		= 203,
	iDontReadFromLocalNet	= 1,
	iReadFromLocalNetThenNet= 2,
	iReadFromLocalNetOnly	= 3,
	
	mProportionalFontName	= 205,	
	mMonospacedFontName		= 206,
		
	mScreenBuffer			= 207,
	iYUV422					= 1,
	iYUV					= 2,
	iRGB16					= 3,
	iRGB32					= 4,

	mTVStandard				= 208,
	iNTSC					= 1,
	iPAL					= 2
};

// ---------------------------------------------------------------------------

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MacintoshMenus.h multiple times"
	#endif
#endif /* __MACINTOSHMENUS_H__ */

