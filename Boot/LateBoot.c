// ===========================================================================
//	LateBoot.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include "Authorization.h"
#include "Boot.h"
#include "BoxAbsoluteGlobals.h"
#include "Cache.h"
#include "Clock.h"
#include "Cookie.h"
#include "DateTime.h"
#ifdef SIMULATOR
	#include "FlashStorage.h"
#endif
#include "HTTP.h"
#include "MemoryManager.h"
#include "Network.h"
#include "Screen.h"
#include "Serial.h"
#include "Socket.h"
#include "SongData.h"
#include "System.h"
#include "WTVProtocol.h"

#if defined HARDWARE
	#include "HWExpansion.h"
#endif

#ifdef SIMULATOR
	#include "Simulator.h"
	#ifdef FOR_MAC
		#include "MacSimulator.h"
	#endif
#endif

// ===========================================================================
//	globals
// ===========================================================================

#ifdef SIMULATOR
AbsoluteGlobals	gSimAbsoluteGlobals;
#endif

// ===========================================================================
//	prototypes
// ===========================================================================

#ifndef SIMULATOR
	extern "C" void BoxDebugger(void);
#endif

void LateBoot(void)
{

#ifdef SIMULATOR
	WRITE_AG(agSysHeapBase, gSimulator->GetRAMBaseAddress());
	WRITE_AG(agSysHeapSize, gSimulator->GetRAMSize());
#endif

	InitializeMemoryManagement();
	InitializeFilesystems();

#ifndef	SIMULATOR
	gSystem = new(System);
#endif	
	InitializeGraphics();
	
#ifdef FOR_MAC
	if (gMacSimulator->GetUsePhone())
		SerialInitialize(kMODEM_PORT);
#endif

/*	BoxDebugger(); */

	Message(("Initializing audio..."));
	AudioSetup();

	Message(("Initializing clock..."));
	Clock::Initialize();

	Message(("Initializing cache..."));
	Cache::Initialize();
	
	TinySocket::Initialize();
	
	Message(("Initializing network..."));
	AuthorizationList::Initialize();
	CookieList::Initialize();
	Network::Initialize();
	HTTPProtocol::Initialize();
	WTVPCommand::Initialize();
	
	Message(("Initializing screen..."));
	Screen::Initialize();
	
#ifdef HARDWARE
	ExpansionLateInit();
	
	Message(("all initialized, calling UserTaskMain()..."));
	UserTaskMain();	
	Trespass();
#endif

#ifdef SIMULATOR
	NVSanityCheck();
#endif /* SIMULATOR */


}

/* note that these must be in reverse order of above */
#ifdef INCLUDE_FINALIZE_CODE
void LateShutdown(void)
{
	Message(("Finalizing screen..."));
	Screen::Finalize();
	
	Message(("Finalizing network..."));
	Network::Finalize();
	HTTPProtocol::Finalize();
	WTVPCommand::Finalize();
	
	TinySocket::Finalize();
	
#ifdef FOR_MAC
	Message(("Finalizing serial hardware..."));
	if (ModemInitialized())
		SerialFinalize(kMODEM_PORT);
#endif	
	
	Message(("Finalizing cache..."));
	Cache::Finalize();
	
	Message(("Finalizing clock..."));
	Clock::Finalize();
	
	Message(("Finalizing audio..."));
	AudioCleanup();
	
	Message(("Finalizing DataTimeParser..."));
	DateTimeParser::Finalize();
	
	FinalizeGraphics();
	FinalizeFilesystems();
	FinalizeMemoryManagement();
}
#endif /* INCLUDE_FINALIZE_CODE */
