// ===========================================================================
//	Simulator.rsrc.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SIMULATOR_RSRC_H__
#define __SIMULATOR_RSRC_H__


//The following constants are the resource IDs.
enum {
	rPhilsTVWDEF			= 128,		// Phil's custom WDEF for Simulator window

	rMenuBar				= 1000,		//application’s menu bar

	rExitAlert				= 1000,		//emergency exit user alert
	rUserAlert				= 1001,		//error message user alert
	rLargeUserAlert			= 1002,		//large error message user alert
	
	rSimulatorWindow		= 1001,		// simulator-containing window
	rMessageWindow			= 1002,		// window for diagnostic messages
	rRemoteWindow			= 1003,		// window for remote control simulation
	rMemoryWindow			= 1004,		// memory window
	
	rGetNameDLOG			= 2000,		//get a name for the sound dialog
	rNameItem				= 3,		//edit text item in rGetNameDLOG
	rUserItem				= 5,		//user item to help draw default outline

	rCustomGetFileDLOG		= 2001,		//dialog template for CustomGetFile
	rURLDialog				= 2002,		//gets the URL address
	rMemorySizeID			= 2003,		//determine simulated RAM size
	rMemoryRangeID			= 2004,		//define tracking range for memory window
	rFileNameDialog			= 2005,		//gets a file name
	rFindDialog				= 2006,		//find...search in TEWindow (look in FindDialogBox.c)
	rUserIDDialog			= 2007,		//gets a user name
	rGenericAlertDialog		= 2008,		//use paramtext for items ^0^1^2^3
	rGenericQueryDialog		= 2009,		//use paramtext for items ^0^1
	rGenericQuery2Dialog	= 2010,		//use paramtext for items ^0^1^2^3
	rFontSizeDialog			= 2011,		//input eight font sizes
	rConnectToDialog		= 2012,		//input servers for connections
	rPreferencesAlertDialog	= 2013,		//use paramtext for items ^0^1^2^3
	rAboutBoxDialog			= 3000,		//^0 => build number, ^1 => (NON)DEBUG
	rRemotePicture			= 1000,		//picture for remote control window
	rStartupPicture			= 1001,		//picture for startup screen

	rSystemPreferencesIcon	= -3971,	//mac system file resource id for preference icon types
	rSaveAsPreferencesIcon	= -16455,	//our resource id for preference icon types in settings file

	rUpArrowPicture			= 1100,		//picture for scrolling down arrow
	rDownArrowPicture		= 1101,		//picture for scrolling up arrow
	
	rLEDBackgroundPicture	= 2000,		// background of LED window
	rLEDPowerOnPicture		= 2001,		// power light (when ON)
	rLEDPowerOffPicture		= 2002,		// power light (when OFF)
	rLEDConnectOnPicture	= 2003,		// connect light (when ON)
	rLEDConnectOffPicture	= 2004,		// connect light (when OFF)
	rLEDMessageOnPicture	= 2005,		// message light (when ON)
	rLEDMessageOffPicture	= 2006,		// message light (when OFF)
	
	rPhoneClickSnd			= 1200,
	rSlideSnd				= 1201,

	rHandCursor				= 1000		//cursor resource for our documents
};


//The following are resource IDs for messages.
enum {
	sErrStrings				= 1000,		//error string STR# ID
	sStandardErr			= 1,		//An error has occurred.
	sMemErr					= 2,		//A Memory Manager error has occurred.
	sResErr					= 3,		//A Resource Manager error has occurred.
	sCurInUseErr			= 4,		//That file is currently in use.
	sWavesBroken			= 5,		//The wave table synthesizer is not available.
	sWrongVersion			= 6,		//This system does not support the Sound Manager...
	sLowMemory				= 7,		//Memory is too low to continue...
	sNoMenus				= 8,		//Could not find application’s menu resources.
	sInitSoundErr			= 9,		//Could not initialize the SoundUnit.
	sSoundErr				= 10,		//The Sound Manager has encountered an error.
	sNewDocErr				= 11,		//Could not create a new document.
	sInitStatusErr			= 12,		//Error initializing the status window.
	sEditErr				= 13,		//Could not complete the edit command.
	sDocErr					= 14,		//There is a problem with this document.

	sMsgStrings				= 1001,		//message string STR# ID
	sPlayingMsg				= 1,		//playing a sound
	sHyperMsg				= 2,		//playing a sound the Hyper way
	sScaleMsg				= 3,		//playing scale
	sMelodyMsg				= 4,		//playing melody
	sTimbresMsg				= 5,		//playing various timbres
	sCounterPtMsg			= 6,		//playing 4 part counter point

	sSMErrStrings			= 1002		//strings describing Sound Manager errors
};



#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include Simulator.rsrc.h multiple times"
#endif
#endif /* __SIMULATOR_RSRC_H__ */