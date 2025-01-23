// ===========================================================================
//	System.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include "BoxAbsoluteGlobals.h"
#include "MemoryManager.h"
#include "BoxUtils.h"
#include "Cache.h"
#include "Code.h"
#include "HWDisplay.h"
#include "HWAudio.h"
#include "Network.h"
#include "PageViewer.h"
#include "Song.h"
#include "System.h"
#include "AlertWindow.h"
#include "Keyboard.h"
#include "Menu.h"
#include "Screen.h"
#include "Boot.h"
#include "Tourist.h"
#include "PerfDump.h"

#ifndef __STATUS_H__
#include "Status.h"
#endif

#include "dnr.h"
#include "ppp.h"	/* for TCPIdle() */

#include "HWKeyboard.h"
#include "IR.h"
#include "FlashStorage.h"

#ifdef HARDWARE
	#include "BoxHWEquates.h"
	#include "HWRegs.h"
	#include "BoxBoot.h"
	#include "SystemGlobals.h"
	#include "BoxUtils.h"
	#include "CrashLogC.h"
	#include "Exceptions.h"
	#include "SmartCard.h"
#else
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"	/* for gSimulator->GetUseNVStore() */
	#endif
#endif

#ifndef __TELLYIO_H__
#include "TellyIO.h"
#endif

#ifndef __CLIENTFUNCTIONS_H__
#include "ClientFunctions.h"
#endif

#ifdef DEBUG_BOXPRINT
#include "BoxPrintDebug.h"
#endif

// ===========================================================================
// Global variables

System* gSystem;

const FontSizeRecord kDefaultFontMonospacedSizeRecord	= {12, 14, 16, 18, 20, 22, 26, 32};
const FontSizeRecord kDefaultFontProportionalSizeRecord	= {12, 14, 16, 18, 20, 22, 26, 32};

const char* kAboutURL				= "file://rom/About.html";
const char* kConnectSetupURL		= "file://rom/ConnectSetup.html";
const char* kPhoneSetupURL			= "file://rom/PhoneSetup.html";
const char* kPowerOffURL			= "file://rom/PowerOff.html";

const char* kSetupURL				= "wtv-setup:/get";

// =============================================================================
// Class System

System::System()
{
	if (gSystem != nil)
		Complain(("Tried to create two Systems!"));
	gSystem = this;

	PostulateFinal(false);	// add test network ip to kDefaultServerAddress

	SetBootURL(kDefaultBootURL);
	SetDefaultBackgroundColor(kDefaultBackgroundColor);
	SetDefaultLinkColor(kDefaultLinkColor);
	SetDefaultTextColor(kDefaultTextColor);
	SetDefaultVisitedLinkColor(kDefaultVisitedLinkColor);

#ifdef DEBUG_CACHE_SCRAMBLE
	SetCacheScramble(kDefaultCacheScramble);
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
	SetCacheValidate(kDefaultCacheValidate);
#endif /* DEBUG_CACHE_VALIDATE */
	SetDisplayMode(kDefaultDisplayMode);
	SetFavoriteURL(kDefaultFavoriteURL);
	SetFontMonospaced(kDefaultFontMonospaced);
	
	SetFontMonospacedSizeRecord(&kDefaultFontMonospacedSizeRecord);
	SetFontProportional(kDefaultFontProportional);
	SetFontProportionalSizeRecord(&kDefaultFontProportionalSizeRecord);
	SetHomeURL(kDefaultHomeURL);
	SetInAttractMode(kDefaultInAttractMode);

	SetLoggingEnabled(kDefaultLoggingEnabled);
	SetNameServer(kDefaultNameServer);
	SetUseJapanese(kDefaultUseJapanese);
	SetUseFlickerFilter(kDefaultUseFlickerFilter);
#if	defined(FOR_MAC) && defined(SIMULATOR)
	{
		Boolean CapsLockKeyDown(void);
		
		MachineLocation where;
		ReadLocation(&where);
		if ( where.u.gmtDelta / (60*60) == 9  ) {		
			// if we are in Japan use Krueger's name server
			SetUseJapanese(true);
			SetNameServer(QuadChar(192,244,1,1));
		}
	}
#endif
	SetPowerOffURL(kPowerOffURL);
	SetPreregistrationService(kDefaultPreregistrationService);

	SetServerName(kDefaultServerName);
	SetServerPort(kDefaultServerPort);
	SetServerType(kDefaultServerType);
	SetStartupURL(kDefaultStartupURL);
	SetUsePhone(kDefaultUsePhone);

	SetUserAgent(kDefaultUserAgent);
	SetUsingHardKeyboard(false);
	SetUsingPhoneSetup(false);
	SetUsingConnectSetup(false);
}

System::~System()
{
	gSystem = nil;
}

#ifdef DEBUG_BOXPRINT
void
System::BoxPrintDebug(long whatToPrint) const
{
	if (whatToPrint == 0) {
		whatToPrint = kBoxPrintURLs | kBoxPrintServers
					  | kBoxPrintUserAgent | kBoxPrintMailCount
					  | kBoxPrintFlags | kBoxPrintPreregistration;
	}

	if (whatToPrint & kBoxPrintURLs) {
		BoxPrint("BootURL: %s", (0 == *fBootURL) ? "<nil>" : fBootURL);
		BoxPrint("FavoriteURL: %s", (0 == *fFavoriteURL) ? "<nil>" : fFavoriteURL);
		BoxPrint("HomeURL: %s", (0 == *fHomeURL) ? "<nil>" : fHomeURL);
		BoxPrint("LogURL: %s", (0 == *fLogURL) ? "<nil>" : fLogURL);
		BoxPrint("NotificationsURL: %s", (0 == *fNotificationsURL) ? "<nil>" : fNotificationsURL);
		BoxPrint("ReconnectURL: %s", (0 == *fReconnectURL) ? "<nil>" : fReconnectURL);
		BoxPrint("SearchURL: %s", (0 == *fSearchURL) ? "<nil>" : fSearchURL);
		BoxPrint("StartupURL: %s", (0 == *fStartupURL) ? "<nil>" : fStartupURL);
	}
	if (whatToPrint & kBoxPrintServers) {
		BoxPrint("NameServer: %d.%d.%d.%d",
					(fNameServer >> 24) & 0xff,
					(fNameServer >> 16) & 0xff,
					(fNameServer >> 8) & 0xff,
					fNameServer & 0xff);
		const char* serverTypeString = kEmptyString;
		switch (fServerType) {
			case kServerTypePreregistration:
				serverTypeString = "Preregistration"; break;
			case kServerTypeLogin:
				serverTypeString = "Login"; break;
			case kServerTypeProxy:
				serverTypeString = "Proxy"; break;
			case kServerTypeNone:
				serverTypeString = "No server"; break;
			default:
				serverTypeString = "<???>"; break;
		}
		BoxPrint("Server: %s:%hu (%d=%s)", fServerName, fServerPort, fServerType, serverTypeString);
	}
	if (whatToPrint & kBoxPrintUserAgent) {
		BoxPrint("User agent: %s", (0 == *fUserAgent) ? "<nil>" : fUserAgent);
	}
	if (whatToPrint & kBoxPrintMailCount) {
		BoxPrint("Mail count: %d", fMailCount);
	}
	if (whatToPrint & kBoxPrintFlags) {
		BoxPrint("CacheScramble: %s  "
				 "CacheValidate: %s  "
				 "InAttractMode: %s  "
				 "IsOn: %s  "
				 "LoggingEnabled: %s  "
				 "UsePhone: %s",
#ifdef DEBUG_CACHE_SCRAMBLE
				 fCacheScramble ? "yes" : "no",
#else
				 "disabled",
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
				 fCacheValidate ? "yes" : "no",
#else
				 "disabled",
#endif /* DEBUG_CACHE_VALIDATE */
				 fInAttractMode ? "yes" : "no",
				 fIsOn ? "yes" : "no",
				 fLoggingEnabled ? "yes" : "no",
				 fUsePhone ? "yes" : "no");
	}
	if (whatToPrint & kBoxPrintPreregistration)
		BoxPrint("Preregistration: %s",
				  (0==*fPreregistrationService) ? "<nil>" : fPreregistrationService);
}
#endif

void
System::CheckSettingsLoaded()
{
	Error		status;
	DataStream*	stream;

	if (fSettingsResource == nil)
		return;		// already done
	if ((status = fSettingsResource->GetStatus()) == kPending || status == kNoError)
		return;		// still waiting
	
	if (status == kComplete) {
		Message(("Loading settings from service"));
		
		stream = fSettingsResource->NewStream();
		Message(("Got stream for settings"));
		if (!IsError(stream == nil)) {
			ulong	bufferLength = stream->GetDataLength() + 1;
			// EMAC: allow bank settings.
			//Assert(bufferLength < 512);
			if(bufferLength >= 512) {
				char*	buffer = (char*)AllocateMemory(bufferLength);
				Message(("Allocated %d bytes for settings", bufferLength));
				
				CopyMemory(stream->GetData(), buffer, bufferLength - 1);
				buffer[bufferLength - 1] = '\0';
				Message(("Copied settings from %x into buffer", stream->GetData()));
				ExecuteClientFunction("client:SetSetupValue", (char*)buffer);
				Message(("Freeing buffer for settings"));
				FreeMemory(buffer);
			}
		}
	}
	
	fSettingsResource->SetStatus(kAborted);
	delete(fSettingsResource);
	fSettingsResource = nil;
}

Boolean 
System::DispatchInput(Input* input)
{
#ifdef FOR_MAC
	if ((input->data == kPowerKey || input->data == 16) && fIsOn &&
				(input->device == kWebTVIRKeyboard || input->device == kPCKeyboard))
#else
	if ((input->data == kPowerKey) && fIsOn && 
				(input->device == kWebTVIRKeyboard || input->device == kPCKeyboard))
#endif
	{
		gPowerOffAlert->Show();

		return true;
	}
	
	return HandlesInput::DispatchInput(input);
}

#ifdef INCLUDE_FINALIZE_CODE
void
System::Finalize(void)
{
}
#endif /* INCLUDE_FINALIZE_CODE */

const char*
System::GetCardNumber() const
{
	PostulateFinal(false);	// read card number
	return nil;
}

const char*
System::GetClientSerialNumber() const
{
#ifdef HARDWARE
	static char serialNumber[17];
	
	if (*serialNumber == 0)
		snprintf(serialNumber, sizeof(serialNumber),
				"%08x%08x", READ_AG(agSerialNumberHi), READ_AG(agSerialNumberLo));
	
	return serialNumber;

#else
	return "demo";
#endif
}

const char*
System::GetUserSignature() const
{
	PostulateFinal(false); // Compute from user ID and private key
	return nil;
}

void
System::Initialize(void)
{
	LateBoot();
}

void
System::LoadSettings()
{
	// Load the settings from the server
	// this should only be called once, at
	// the beginning of connecting to service

	if (IsError(fSettingsResource != nil))
		return;
		
	fSettingsResource = new(Resource);	
	fSettingsResource->SetURL(kSetupURL);
	fSettingsResource->SetPriority(kPersistent);
	fSettingsResource->SetStatus(kNoError);
}

Boolean
System::PowerInput()
{
	if (fIsOn)
		PowerOff();
	else
		PowerOn();

	return true;
}

void
System::PowerOff()
{
	gNetwork->Deactivate();

#ifdef HARDWARE
	KillDisplay();
	
	if(SPOTVersion() != kSPOT1)
		StopAudioDMA();
	
	disable_cpu_ints(1);	/* clear IEc, stops bugs dead */
#endif
	
	// note: should be in separate method
	gStatusIndicator->Hide();
	gConnectIndicator->Hide();
	gConnectWindow->Hide();
	gSaveSplash->Hide();
	gMenuLayer->Hide();
	gAlertWindow->Hide();
	
	gScreen->CloseAllPanels();
	
	gPageViewer->ExecuteURL(GetPowerOffURL());
		
	fIsOn = false;

	gScreen->Draw();	// force black
	SetBoxLEDs(GetBoxLEDs() & ~(kBoxLEDPower | kBoxLEDConnect));

	SaveState();
	
#ifdef HARDWARE
	Reboot(kWarmBoot);
#endif
}



void
System::PowerOn()
{
	Message(("Powering on system"));

	fIsOn = true;
	SetBoxLEDs(GetBoxLEDs() | kBoxLEDPower);
	
	RestoreState();

#ifdef HARDWARE
	if (ColdBooted() && (gRAMCache->Find(kDownloadedTellyScriptURL) != nil))
	{
		Message(("System:  Cold boot..."));

	    gAlertWindow->SetURL("file://rom/phone/unplugged.html");
		gAlertWindow->Show();
		ShowStartupScreen();
	}
	else
#endif						/* simulator always looks like warm boot */
	{						
		Message(("System:  Warm boot..."));

		gNetwork->Activate();
		ShowStartupScreen();
	}
	
	FlushInput();
	
#ifdef HARDWARE
	CheckSmartCard();		/* see if a smart card is already present */
	EnableDisplay();
#endif
}



void
System::RestoreState(void)
{
#ifdef HARDWARE		
	
	/* HACK - boot rom leaves NV storage in bad state after flash */
	if((NVGetFlags() & 0xffff0000) != 0)	/* did boot rom leave NV storage in bogus state? */
	{
		Message(("Boot ROM left NV storage in bad state.  Nuking NV storage."));
		NVInit(kNVPhaseSaveFB);				/* screen's not on yet, so this is OK */		
		NVCommit();							/* update checksum, write, free buf */
	}
	else
		Message(("Boot ROM left NV storage in good state."));	
	/* END HACK */
	
	
	NVInit(kNVPhaseRestore);				/* boot code will have checked sanity */
											/*  (and reset, if needed)            */
	gNetwork->RestoreState();
	
#else /* HARDWARE */
	
	if (gSimulator->GetUseNVStore()) 
	{
		NVInit(kNVPhaseRestore);			/* Assume warm boot */
		gNetwork->RestoreState();
	}

#endif /* HARDWARE */
}



void
System::SaveState(void)
{
	Message(("System: saving state..."));
	
	NVInit(kNVPhaseSaveFB);
										/* call state savers here... */
	gNetwork->SaveState();
	
	NVCommit();
}



Boolean
System::SetAttribute(const char* name, char* value)
{
	if (EqualString(name, "wtv-boot-url")) {
		SetBootURL(value);
		return true;
	}
	
	if (EqualString(name, "wtv-favorite-url")) {
		SetFavoriteURL(value);
		return true;
	}
	
	if (EqualString(name, "wtv-home-url")) {
		SetHomeURL(value);
		return true;
	}
	
	if (EqualString(name, "wtv-log-url")) {
		SetLogURL(value);
		return true;
	}
	
	if (EqualString(name, "wtv-mail-count")) {
		SetMailCount((ushort)atoi(value));
		return true;
	}
	
	if (EqualString(name, "wtv-name-server")) {
		SetNameServer(ParseAddress(value));
		return true;
	}
	
	if (EqualString(name, "wtv-notifications-url")) {
		SetNotificationsURL(value);
		return true;
	}
	
	if (EqualString(name, "wtv-reconnect-url")) {
		SetReconnectURL(value);
		return true;
	}
	
	if (EqualString(name, "wtv-search-url")) {
		SetSearchURL(value);
		return true;
	}
	
	return false;
}

void
System::SetBootURL(const char* value)
{
	TrivialMessage(("System::SetBootURL to '%s'",value));
	CopyStringIntoCharArray(fBootURL, value);
}

void
System::SetDisplayMode(DisplayMode mode)
{
	fDisplayMode = mode;
	::SetDisplayMode(mode);
}

void
System::SetFavoriteURL(const char* value)
{
	CopyStringIntoCharArray(fFavoriteURL, value);
}

void
System::SetFontSizes(long fontSizesSelector)
{
	FontSizeRecord fontSizeRecord[/* kNumFontSizeRecords */] = 
	{
		{ {12, 14, 14, 16, 18, 20, 24, 28} },	// small
		{ {12, 14, 16, 18, 20, 22, 26, 32} },	// medium
		{ {16, 18, 20, 24, 28, 32, 36, 42} }	// large
	};
	
	fFontSizesSelector = (short)fontSizesSelector;
	SetFontMonospacedSizeRecord(&(fontSizeRecord[fontSizesSelector+1]));
	SetFontProportionalSizeRecord(&(fontSizeRecord[fontSizesSelector+1]));
}

void
System::SetHomeURL(const char* value)
{
	CopyStringIntoCharArray(fHomeURL, value);
}

void
System::SetLogURL(const char* value)
{
	CopyStringIntoCharArray(fLogURL, value);
}

void
System::SetMailCount(ushort value)
{
	ulong boxLEDs = GetBoxLEDs() | kBoxLEDMessage; // force on
	if (value == 0)
		boxLEDs -= kBoxLEDMessage;	// if no mail, turn it off
	SetBoxLEDs(boxLEDs);
	fMailCount = value;
}

void
System::SetNameServer(ulong value)
{
	if (IsError(value == 0))
		return;
	
	fNameServer = value;
}

void
System::SetNotificationsURL(const char* value)
{
	CopyStringIntoCharArray(fNotificationsURL, value);
}

void
System::SetPowerOffURL(const char* value)
{
	CopyStringIntoCharArray(fPowerOffURL, value);
}

void
System::SetPreregistrationService(const char* value)
{
	CopyStringIntoCharArray(fPreregistrationService, value);
}

void
System::SetReconnectURL(const char* value)
{
	CopyStringIntoCharArray(fReconnectURL, value);
}

void
System::SetSearchURL(const char* value)
{
	CopyStringIntoCharArray(fSearchURL, value);
}

void
System::SetServerName(const char* nameOrAddress)
{
	snprintf(fServerName, sizeof(fServerName), "%s", nameOrAddress);
}

void
System::SetStartupURL(const char* value)
{
	CopyStringIntoCharArray(fStartupURL, value);
}

void
System::SetUserAgent(const char* value)
{
	CopyStringIntoCharArray(fUserAgent, value);
}

void
System::ShowStartupScreen()
{
#if 0
	Assert(gPageViewer != nil);
	gPageViewer->ExecuteURL("file://rom/Startup.html", nil);

	// NOTE: for now, let's not show the startup screen
	// unless the simulate phone flag is set to avoid
	// waiting unnecessarily while developing.
	
	if (!GetUsePhone())
	{
		Assert(gPageViewer != nil);
		gPageViewer->ExecuteURL("file://rom/Startup.html", nil);
	}
	else
	{
		// No status for loading first page from network
		// if startup is turned off.
		gStatusIndicator->SetDisabled(true);
	}
#endif

	gScreen->SetIsDirty(false);
	gScreen->Draw();
}


#ifdef DEBUG_BOXPRINT
void
System::StaticBoxPrintDebug(const System* system, long whatToPrint)
{
	if (system == nil) {
		BoxPrint("System: <nil>");
	} else {
		BoxPrint("System: <%#06x>", system);
		AdjustBoxIndent(1);
		system->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif



#ifndef SIMULATOR
void UserTaskMain(void)
{
	Message(("UserTaskMain!!"));

	Message(("EMAC: F1=Debug print enable/disable"));
	Message(("EMAC: F2=Screen enable/disable"));
	Message(("EMAC: F3=Power button"));
	Message(("EMAC: F4=Debug breaks enable/disable"));
	Message(("EMAC: F5=Monkey mode enable/disable"));
	Message(("EMAC: F6=Set slow ROM speed (no effect in MAME)"));
	Message(("EMAC: F7=SPOT refresh hack gVBump increase (no effect since I removed this hack to fix video in MAME)"));

	if (ColdBooted())
	{
		Message(("System:  Cold boot, invalidating Crash Log..."));
		((CrashLogStruct*)kCrashLogBase)->crashSig = 0;		/* power loss, all bets off on crash log */
	}
	else
	{
		Message(("crashsig is @ %08x, sig = %08x", &(((CrashLogStruct*)kCrashLogBase)->crashSig),
													((CrashLogStruct*)kCrashLogBase)->crashSig ));
		if(((CrashLogStruct*)kCrashLogBase)->crashSig == kValidCrashLogSig)
		{
			Message(("System:  Warm boot, Valid Crash Log, turning on to send..."));
			gSystem->PowerOn();
		}
	}

	for (;;)
		gSystem->UserLoopIteration();
}
#endif /* #ifndef SIMULATOR */



void
System::UserLoopIteration()
{
	Input		nextInput;

	PerfDump perfdump("System::UserLoop");

#ifdef SPOT1
	// workaround for video shift problem
	// if we detect that video has shifted, kill the display
	// wait a frame, then re-enable it
	SPOT1ShiftHack();
#endif /* SPOT1 */	

#ifdef DEBUG_MONKEY
	if (gMonkeyEnable)
		DoTheMonkey();
#endif /* DEBUG_MONKEY */

	NextInput(&nextInput);
	
	IRIdle();
	TCPIdle(true);
	DNRHandlerLoop();

	ProfileUnitEnter("Network::Idle");
	gNetwork->Idle();
	ProfileUnitExit("Network::Idle");

	MIDI::Idle();
	Code::IdleAll();
	
#ifdef HARDWARE
	HWKeyboardIdle();
	SmartCardIdle();
#endif /* HARDWARE */

#ifdef DEBUG_TOURIST
	if (gTourist != nil)
		gTourist->Idle();
#endif /* DEBUG_TOURIST */
	
	CheckSettingsLoaded();

#ifdef DEBUG_CACHE_SCRAMBLE
	if (GetCacheScramble())
		gRAMCache->Scramble();
#endif /* DEBUG_CACHE_SCRAMBLE */

#ifdef DEBUG_CACHE_VALIDATE
	if (GetCacheValidate()) {
		gRAMCache->IsValid();
	}
#endif /* DEBUG_CACHE_VALIDATE */

	do {
		if (DispatchInput(&nextInput) || gScreen->DispatchInput(&nextInput))
			fLastInputTime = Now();
	} while (NextInput(&nextInput));

#ifdef FOR_MAC
	extern Boolean gInBackground;
	if (!gInBackground)
#endif /* FOR_MAC */
	{
		gScreen->Draw();
		UpdateScreenBits();
	}
}

void
System::WriteAttributes(Stream* stream)
{
	const char* p;

	if ((p = GetCardNumber()) != nil)
		stream->WriteAttribute("wtv-card-number", p);

	if ((p = GetClientSerialNumber()) != nil)
		stream->WriteAttribute("wtv-client-serial-number", p);

	if ((p = GetUserSignature()) != nil)
		stream->WriteAttribute("wtv-user-signature", p);

	stream->WriteAttribute("wtv-system-version", GetVersionNumber());
}

// ============================================================================

