// ===========================================================================
//	Simulator.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __BOOT_H__
#include "Boot.h"
#endif
#ifndef __BOXABSOLUTEGLOBALS_H__
#include "BoxAbsoluteGlobals.h"
#endif
#ifndef __HW_DISPLAY__
#include "HWDisplay.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYMANAGERPRIVATE_H__
#include "MemoryManager.Private.h"
#endif
//#ifndef __OBJECTSTORE_H__
//#include "ObjectStore.h"
//#endif
#ifndef __SCREEN_H__
#include "Screen.h"
#endif
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif
#ifndef __STATUS_H__
#include "Status.h"
#endif

Simulator* gSimulator;

// --------------------------------------------------------------------------------
//
// Note: this file should contain no Macintosh code.  Mac-specific simulator code
// should be placed in MacSimulator.c.
//
// --------------------------------------------------------------------------------

const char kScreenAllocTag[] 			= "Screen Buffer";


// On the box, these are allocated in HWDisplay.c


// --------------------------------------------------------------------------------
//	Preferences
// --------------------------------------------------------------------------------

// These are from the System.
const char kPrefsCacheScramble[]			= "Cache Scramble";
const char kPrefsCacheValidate[]			= "Cache Validate";
const char kPrefsDisplayMode[]				= "Display Mode";
const char kPrefsFontMonospaced[]			= "Font (Monospaced)";
const char kPrefsFontProportional[]			= "Font (Proportional)";
const char kPrefsFontMonospacedSizeRecord[]	= "Font size record (Monospaced)";
const char kPrefsFontProportionalSizeRecord[]	= "Font size record (Proportional)";
const char kPrefsInAttractMode[]			= "In Attract Mode";
const char kPrefsLoggingEnabled[]			= "Logging Enabled";
const char kPrefsServerType[]				= "Server Type";
const char kPrefsUsePhone[]					= "Use Phone";
const char kPrefsUserAgent[]				= "User Agent";

// These are from the Simulator
const char kPrefsFidoEnabled[]				= "Fido Enabled";
const char kPrefsForceAntialiasText[]		= "Force antialias text";
const char kPrefsForceRegistration[]		= "Force registration in preregistration mode";
const char kPrefsInDemoMode[]				= "In Demo Mode";
const char kPrefsIREnabled[]				= "IR Enabled";
const char kPrefsLimitNetSpeed[]			= "Limit Net Speed";
const char kPrefsLoginName[]				= "Login Server Name";
const char kPrefsLoginPort[]				= "Login Server Port";
const char kPrefsPreregistrationName[]		= "Preregistration Server Name";
const char kPrefsPreregistrationPort[]		= "Preregistration Server Port";
const char kPrefsProxyName[]				= "Proxy Server Name";
const char kPrefsProxyPort[]				= "Proxy Server Port";
const char kPrefsRAMSize[]					= "RAM Size";
const char kPrefsScreenBitMapFormat[]		= "BitMap Format";
const char kPrefsStrictDebugging[]			= "Strict Debugging";
const char kPrefsTrivialMessagesEnabled[]	= "Trivial Messages";
const char kPrefsUseBackBuffer[]			= "Use Back Buffer";
const char kPrefsUseFlickerFilter[]			= "Use Flicker Filter";
const char kPrefsUseNVStore[]				= "Use NV Store";
const char kPrefsUseROMStore[]				= "Use ROM Store";

// --------------------------------------------------------------------------------
// Simulator::GetXXX defaults
// --------------------------------------------------------------------------------

const Boolean		kDefaultFidoEnabled				= false;
const Boolean		kDefaultForceAntialiasText		= false;
const Boolean		kDefaultForceRegistration		= false;
const Boolean		kDefaultInDemoMode				= false;
const Boolean		kDefaultIREnabled				= true;

const ulong			kDefaultLimitNetSpeed			= 0;
const ulong			kDefaultRAMSize					= (960L*1024);
const BitMapFormat	kDefaultScreenBitMapFormat		= rgb16Format;
const Boolean		kDefaultStrictDebugging			= false;
const Boolean		kDefaultTrivialMessagesEnabled	= false;

const Boolean		kDefaultUseBackBuffer			= true;
const Boolean		kDefaultUseNVStore				= false;
const Boolean		kDefaultUseROMStore				= true;




//=======================================================================================
// Initialization and Finalization Methods
//=======================================================================================

Simulator::Simulator()
{
	if (gSimulator != nil)
		Complain(("Tried to create two Simulators!"));
	gSimulator = this;

	fIsInitialized = false;

	SetFidoEnabled(kDefaultFidoEnabled);
	SetForceAntialiasText(kDefaultForceAntialiasText);
	SetForceRegistration(kDefaultForceRegistration);
	SetInDemoMode(kDefaultInDemoMode);
	SetIREnabled(kDefaultIREnabled);
	
	SetLimitNetSpeed(kDefaultLimitNetSpeed);
	SetLoginName(kDefaultLoginServerName);
	SetLoginPort(kDefaultLoginServerPort);
	SetPreregistrationName(kDefaultPreregistrationServerName);
	SetPreregistrationPort(kDefaultPreregistrationServerPort);
	
	SetProxyName(kDefaultProxyServerName);
	SetProxyPort(kDefaultProxyServerPort);
	SetRAMSize(kDefaultRAMSize);						fRAMBaseAddress = nil;
	SetScreenBitMapFormat(kDefaultScreenBitMapFormat);	fScreenBaseAddress = nil;
	SetStrictDebugging(kDefaultStrictDebugging);
	
	SetTrivialMessagesEnabled(kDefaultTrivialMessagesEnabled);
	SetUseBackBuffer(kDefaultUseBackBuffer);
	SetUseNVStore(kDefaultUseNVStore);
	SetUseROMStore(kDefaultUseROMStore);
}

Simulator::~Simulator()
{
	SavePreferences();
	gSimulator = nil;
}

void Simulator::Initialize()
{
	if (fIsInitialized)
	{
		Complain(("Simulator::Initialize():  Tried to initialize the simulator more than once!"));
		return;
	}

	Log(("\n\n\nInitializing simulator..."));
	
	if (fRAMBaseAddress != nil)
		DisposeSimulatorMemory(fRAMBaseAddress);

	fRAMBaseAddress = NewSimulatorMemory(GetRAMSize() + 8, true);

	if (fRAMBaseAddress == nil)
		EmergencyExit(kLowMemory);

#ifdef DEBUG	
	ClearMemoryTags();
#endif

	fIsInitialized = true;

	System::Initialize();
}

void Simulator::Finalize()
{
	if (!fIsInitialized)
	{
		Complain(("Simulator::Finalize():  Tried to finalize the simulator when it wasn't initialized!"));
		return;
	}

	Log(("\n\n\nFinalizing simulator..."));

#if 0	
	LateShutdown();

	if (fRAMBaseAddress != nil)
	{	DisposeSimulatorMemory(fRAMBaseAddress);
		fRAMBaseAddress = nil;
	}
#endif

	fIsInitialized = false;
}

//=======================================================================================

void
Simulator::LoadPreferences()
{
	ushort			ushortValue;
	ulong			ulongValue;
	Boolean			booleanValue;
	char*			stringValue;
	FontSizeRecord	fontSizeRecord;
	ServerType		serverType;

	// The formatting is weird just to make it look like SavePreferences...
	// ...it's important that Load/Save stay in synch...

	// These preferences are from the system
#ifdef DEBUG_CACHE_SCRAMBLE
	if (GetPreferenceBoolean(kPrefsCacheScramble, &booleanValue))			SetCacheScramble(booleanValue);
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
	if (GetPreferenceBoolean(kPrefsCacheValidate, &booleanValue))			SetCacheValidate(booleanValue);
#endif /* DEBUG_CACHE_VALIDATE */
	if (GetPreferenceULong(kPrefsDisplayMode, &ulongValue))					SetDisplayMode((DisplayMode)ulongValue);
	if (GetPreferenceULong(kPrefsFontMonospaced, &ulongValue))				SetFontMonospaced(ulongValue);
	if (GetPreferenceFontSizeRecord(kPrefsFontMonospacedSizeRecord, &fontSizeRecord))	SetFontMonospacedSizeRecord(&fontSizeRecord);
	
	if (GetPreferenceULong(kPrefsFontProportional, &ulongValue))			SetFontProportional(ulongValue);
	if (GetPreferenceFontSizeRecord(kPrefsFontProportionalSizeRecord, &fontSizeRecord))	SetFontProportionalSizeRecord(&fontSizeRecord);
	if (GetPreferenceBoolean(kPrefsInAttractMode, &booleanValue))			SetInAttractMode(booleanValue);
	if (GetPreferenceBoolean(kPrefsLoggingEnabled, &booleanValue))			SetLoggingEnabled(booleanValue);
	if (GetPreferenceServerType(kPrefsServerType, &serverType))				SetServerType(serverType);

	if (GetPreferenceBoolean(kPrefsUsePhone, &booleanValue))				SetUsePhone(booleanValue);
	if (GetPreferenceString(kPrefsUserAgent, &stringValue))					SetUserAgent(stringValue);

	// These preferences are from the simulator
	if (GetPreferenceBoolean(kPrefsFidoEnabled, &booleanValue))				SetFidoEnabled(booleanValue);
	if (GetPreferenceBoolean(kPrefsForceAntialiasText, &booleanValue))		SetForceAntialiasText(booleanValue);
	if (GetPreferenceBoolean(kPrefsForceRegistration, &booleanValue))		SetForceRegistration(booleanValue);
	if (GetPreferenceBoolean(kPrefsInDemoMode, &booleanValue))				SetInDemoMode(booleanValue);
	if (GetPreferenceBoolean(kPrefsIREnabled, &booleanValue))				SetIREnabled(booleanValue);
	
	if (GetPreferenceULong(kPrefsLimitNetSpeed, &ulongValue))				SetLimitNetSpeed(ulongValue);
	if (GetPreferenceString(kPrefsLoginName, &stringValue))					SetLoginName(stringValue);
	if (GetPreferenceUShort(kPrefsLoginPort, &ushortValue))					SetLoginPort(ushortValue);
	if (GetPreferenceString(kPrefsPreregistrationName, &stringValue))		SetPreregistrationName(stringValue);
	if (GetPreferenceUShort(kPrefsPreregistrationPort, &ushortValue))		SetPreregistrationPort(ushortValue);
	
	if (GetPreferenceString(kPrefsProxyName, &stringValue))					SetProxyName(stringValue);
	if (GetPreferenceUShort(kPrefsProxyPort, &ushortValue))					SetProxyPort(ushortValue);
	if (GetPreferenceULong(kPrefsRAMSize, &ulongValue))						SetRAMSize(ulongValue);
	if (GetPreferenceULong(kPrefsScreenBitMapFormat, &ulongValue))			SetScreenBitMapFormat((BitMapFormat)ulongValue);
	if (GetPreferenceBoolean(kPrefsStrictDebugging, &booleanValue))			SetStrictDebugging(booleanValue);
	
	if (GetPreferenceBoolean(kPrefsTrivialMessagesEnabled, &booleanValue))	SetTrivialMessagesEnabled(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseBackBuffer, &booleanValue))			SetUseBackBuffer(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseFlickerFilter, &booleanValue))		SetUseFlickerFilter(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseNVStore, &booleanValue))				SetUseNVStore(booleanValue);
	if (GetPreferenceBoolean(kPrefsUseROMStore, &booleanValue))				SetUseROMStore(booleanValue);

	// copy the right server prefs into the system's space
	switch (GetServerType())
	{
		case kServerTypeLogin:
			SetServerName(GetLoginName());
			SetServerPort(GetLoginPort());
			break;
		case kServerTypePreregistration:
			SetServerName(GetPreregistrationName());
			SetServerPort(GetPreregistrationPort());
			break;
		case kServerTypeProxy:
			SetServerName(GetProxyName());
			SetServerPort(GetProxyPort());
			break;
		case kServerTypeNone:
			SetServerName(nil);
			SetServerPort(0);
			break;
	}
}

void
Simulator::SavePreferences()
{
	// copy the right server prefs into the system's space
	switch (GetServerType())
	{
		case kServerTypeLogin:
			SetLoginName(GetServerName());
			SetLoginPort(GetServerPort());
			break;
		case kServerTypePreregistration:
			SetPreregistrationName(GetServerName());
			SetPreregistrationPort(GetServerPort());
			break;
		case kServerTypeProxy:
			SetProxyName(GetServerName());
			SetProxyPort(GetServerPort());
			break;
		case kServerTypeNone:
			break;
	}

	// These preferences are from the system
#ifdef DEBUG_CACHE_SCRAMBLE
	SetPreferenceBoolean(kPrefsCacheScramble, GetCacheScramble());
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
	SetPreferenceBoolean(kPrefsCacheValidate, GetCacheValidate());
#endif /* DEBUG_CACHE_VALIDATE */
	SetPreferenceULong(kPrefsDisplayMode, (ulong)GetDisplayMode());
	SetPreferenceULong(kPrefsFontMonospaced, GetFontMonospaced());
	SetPreferenceFontSizeRecord(kPrefsFontMonospacedSizeRecord, GetFontMonospacedSizeRecord());
	
	SetPreferenceULong(kPrefsFontProportional, GetFontProportional());
	SetPreferenceFontSizeRecord(kPrefsFontProportionalSizeRecord, GetFontProportionalSizeRecord());
	SetPreferenceBoolean(kPrefsInAttractMode, GetInAttractMode());
	SetPreferenceBoolean(kPrefsLoggingEnabled, GetLoggingEnabled());
	SetPreferenceServerType(kPrefsServerType, GetServerType());
	
	SetPreferenceBoolean(kPrefsUsePhone, GetUsePhone());
	SetPreferenceString(kPrefsUserAgent, GetUserAgent());

	// These preferences are from the simulator
	SetPreferenceBoolean(kPrefsFidoEnabled, GetFidoEnabled());
	SetPreferenceBoolean(kPrefsForceAntialiasText, GetForceAntialiasText());
	SetPreferenceBoolean(kPrefsForceRegistration, GetForceRegistration());
	SetPreferenceBoolean(kPrefsInDemoMode, GetInDemoMode());
	SetPreferenceBoolean(kPrefsIREnabled, GetIREnabled());
	
	SetPreferenceULong(kPrefsLimitNetSpeed, GetLimitNetSpeed());
	SetPreferenceString(kPrefsLoginName, GetLoginName());
	SetPreferenceUShort(kPrefsLoginPort, GetLoginPort());
	SetPreferenceString(kPrefsPreregistrationName, GetPreregistrationName());
	SetPreferenceUShort(kPrefsPreregistrationPort, GetPreregistrationPort());
	
	SetPreferenceString(kPrefsProxyName, GetProxyName());
	SetPreferenceUShort(kPrefsProxyPort, GetProxyPort());
	SetPreferenceULong(kPrefsRAMSize, GetRAMSize());
	SetPreferenceULong(kPrefsScreenBitMapFormat, (ulong)GetScreenBitMapFormat());
	SetPreferenceBoolean(kPrefsStrictDebugging, GetStrictDebugging());
	
	SetPreferenceBoolean(kPrefsTrivialMessagesEnabled, GetTrivialMessagesEnabled());
	SetPreferenceBoolean(kPrefsUseBackBuffer, GetUseBackBuffer());
	SetPreferenceBoolean(kPrefsUseFlickerFilter, GetUseFlickerFilter());
	SetPreferenceBoolean(kPrefsUseNVStore, GetUseNVStore());
	SetPreferenceBoolean(kPrefsUseROMStore, GetUseROMStore());
}

void
Simulator::OpenPreferences()
{
}

void
Simulator::ClosePreferences()
{
}

//=======================================================================================
//	Getters/Setters
//=======================================================================================

ulong Simulator::GetScreenRowBytes() const 
{
	return (GetDisplayWidth() * (GetScreenBitMapFormat() & 0xff)) / 8;
};

ulong Simulator::GetScreenSize() const
{
	return GetDisplayHeight() * GetScreenRowBytes();
};

//=======================================================================================
//	Operations
//=======================================================================================

void Simulator::EmergencyExit(Error UNUSED(error))
{
	Complain(("EmergencyExit() not yet implemented...I advise you to quit IMMEDIATELY!"));
	Assert(false);
}

void Simulator::FinalizeScreenRAM(void)
{
	if (fScreenBaseAddress != nil)
	{	DisposeSimulatorMemory((char*)fScreenBaseAddress);
		fScreenBaseAddress = nil;
	}
}

void Simulator::HandleKey(char key, short modifiers, Boolean autoRepeat)
{
	Input input;

	input.rawData = 0;					// could make this look like PC keyboard scancodes
	input.device = kPCKeyboard;			// so it looks like remote button emulation keys
	input.modifiers = 0;
	input.time = Now();
	
	if (autoRepeat)
		input.modifiers |= kAutoRepeat;
	if (modifiers & 0x0100)
		input.modifiers |= kAltModifier;
	if (modifiers & 0x0200)
		input.modifiers |= kShiftModifier;

	// NOTE: These should all be moved somewhere inside the system because we will
	// have to support both an IR keyboard and a PC-compatible keyboard.
	PostulateFinal(false);

	const ulong kSimulatorDelKey	 	= 0x7F;		/* back (future: options) */
	const ulong kSimulatorClearKey		= 0x1B;		/* back */
	const ulong kSimulatorInsKey		= 0x05;		/* recent */
	const ulong kSimulatorUpKey			= 0x1E;		/* up arrow */		
	const ulong kSimulatorLeftKey		= 0x1C;		/* left arrow */			
	const ulong kSimulatorDownKey		= 0x1F;		/* down arrow */
	const ulong kSimulatorRightKey		= 0x1D;		/* right arrow */		
	const ulong kSimulatorEnterKey		= 0x03;		/* execute */
	const ulong kSimulatorEndKey		= 0x04;		/* options */										
	const ulong kSimulatorHomeKey		= 0x01;		/* home */		
	const ulong kSimulatorPageUpKey		= 0x0B;		/* scroll up */
	const ulong kSimulatorPageDownKey 	= 0x0C;		/* scroll down */
	
#if 0
	Removed because new Sony Remote was confusing Sejin PC IR receiver.
	This was causing spurious power offs.
	const ulong kSimulatorPauseKey		= 0x10;		/* power */
#endif
	
	switch (key) {
		case kSimulatorDelKey:		input.data = kOptionsKey; 		break;
		case kSimulatorClearKey:	input.data = kBackKey;			break;
		case kSimulatorInsKey:		input.data = kRecentKey;		break; 
		case kSimulatorUpKey:		input.data = kUpKey; 			break;
		case kSimulatorLeftKey:		input.data = kLeftKey; 			break;
		case kSimulatorDownKey:		input.data = kDownKey; 			break;
		case kSimulatorRightKey:	input.data = kRightKey;			break;
		case kSimulatorEnterKey:	input.data = kExecuteKey; 		break;
		case kSimulatorEndKey:		input.data = kBackKey; 			break;
		case kSimulatorHomeKey:		input.data = kHomeKey; 			break;
		case kSimulatorPageUpKey:	input.data = kScrollUpKey; 		break;
		case kSimulatorPageDownKey:	input.data = kScrollDownKey; 	break;

		// other keyboard input
		default:					input.data = key;				break;
	}
	
	PostInput(&input);
}

void Simulator::InitializeScreenRAM(void)
{
	BitMapFormat format = GetScreenBitMapFormat();
	if ( (format & 0xff) != 16 && (format & 0xff) != 32 )
	{	SetScreenBitMapFormat(rgb16Format);
	}
	
	fScreenBaseAddress = (char*)NewSimulatorMemory(GetScreenSize() + 8, true);
	if (fScreenBaseAddress == nil)
	{	EmergencyExit(kLowMemory);
	}
}

//=======================================================================================
// Preference methods...for reading/writing preferences
//=======================================================================================

Boolean
Simulator::GetPreference(const char* UNUSED(name), void** UNUSED(buffer), size_t* UNUSED(bufLength)) const
{
	Assert(false);	// Simulator::GetPreference() not implemented 
	return false;
}
Boolean
Simulator::GetPreferenceBoolean(const char* name, Boolean* value) const
{
	Boolean* prefPtr = nil;
	size_t size = sizeof(Boolean);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(Boolean));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceChar(const char* name, char* value) const
{
	char* prefPtr = nil;
	size_t size = sizeof(char);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(char));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceUChar(const char* name, unsigned char* value) const
{
	unsigned char* prefPtr = nil;
	size_t size = sizeof(unsigned char);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(unsigned char));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceShort(const char* name, short* value) const
{
	short* prefPtr = nil;
	size_t size = sizeof(short);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(short));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceUShort(const char* name, unsigned short* value) const
{
	unsigned short* prefPtr = nil;
	size_t size = sizeof(unsigned short);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(unsigned short));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceLong(const char* name, long* value) const
{
	long* prefPtr = nil;
	size_t size = sizeof(long);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(long));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceULong(const char* name, unsigned long* value) const
{
	unsigned long* prefPtr = nil;
	size_t size = sizeof(unsigned long);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(unsigned long));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceString(const char* name, const char** valuePtr) const
{
	char* prefPtr = nil;
	size_t size;
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	*valuePtr = prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceFontSizeRecord(const char* name, FontSizeRecord* value) const
{
	FontSizeRecord* prefPtr = nil;
	size_t size = sizeof(FontSizeRecord);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(FontSizeRecord));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::GetPreferenceServerType(const char* name, ServerType* value) const
{
	ServerType* prefPtr = nil;
	size_t size = sizeof(ServerType);
	if (!GetPreference(name, &prefPtr, &size))
		return false;
	Assert(size == sizeof(ServerType));
	*value = *prefPtr;
	return true;
}
Boolean
Simulator::RemovePreference(const char* UNUSED(name))
{
	Assert(false);	// Simulator::RemovePreference() not implemented 
	return false;
}
Boolean
Simulator::SetPreference(const char* UNUSED(name), const void* UNUSED(buffer), size_t UNUSED(bufLength))
{
	Assert(false);	// Simulator::SetPreference() not implemented
	return false;
}
Boolean
Simulator::SetPreferenceBoolean(const char* name, Boolean value)
{	return SetPreference(name, &value, sizeof(Boolean));
}
Boolean
Simulator::SetPreferenceChar(const char* name, char value)
{	return SetPreference(name, &value, sizeof(char));
}
Boolean
Simulator::SetPreferenceUChar(const char* name, unsigned char value)	
{	return SetPreference(name, &value, sizeof(unsigned char));
}
Boolean
Simulator::SetPreferenceShort(const char* name, short value)
{	return SetPreference(name, &value, sizeof(short));
}
Boolean
Simulator::SetPreferenceUShort(const char* name, unsigned short value)
{	return SetPreference(name, &value, sizeof(unsigned short));
}
Boolean
Simulator::SetPreferenceLong(const char* name, long value)	
{	return SetPreference(name, &value, sizeof(long));
}
Boolean
Simulator::SetPreferenceULong(const char* name, unsigned long value)
{	return SetPreference(name, &value, sizeof(unsigned long));
}
Boolean
Simulator::SetPreferenceString(const char* name, const char* const value)
{	return SetPreference(name, value, strlen(value)+1);
}
Boolean
Simulator::SetPreferenceFontSizeRecord(const char* name, const FontSizeRecord* value)
{	return SetPreference(name, value, sizeof(FontSizeRecord));
}
Boolean
Simulator::SetPreferenceServerType(const char* name, ServerType value)
{	return SetPreference(name, &value, sizeof(ServerType));
}

