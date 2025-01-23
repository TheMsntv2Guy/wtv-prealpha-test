// ===========================================================================
//	Simulator.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"		/* for Error */
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"			/* for BitMapFormat */
#endif
#ifndef __SYSTEM_H__
#include "System.h"				/* for System */
#endif
#ifndef __HW_DISPLAY__
//#include "HWDisplay.h"
#endif



// ==============================================================================

class Simulator : public System {
public:
							Simulator(void);
	virtual					~Simulator(void);
	
	virtual void			Initialize(void);
	virtual void			Finalize(void);

	// Getters
	Boolean					GetFidoEnabled() const				{ return fFidoEnabled; };
	Boolean					GetForceAntialiasText() const		{ return fForceAntialiasText; };
	Boolean					GetInDemoMode() const				{ return fInDemoMode; };
	Boolean					GetIREnabled() const				{ return fIREnabled; };
	Boolean					GetIsInitialized() const			{ return fIsInitialized; };
	ulong					GetLimitNetSpeed() const			{ return fLimitNetSpeed; };
	const char*				GetLoginName();
	ushort					GetLoginPort();
	const char*				GetPreregistrationName();
	ushort					GetPreregistrationPort();
	const char*				GetProxyName();
	ushort					GetProxyPort();
	char*					GetRAMBaseAddress() const			{ return fRAMBaseAddress; };
	ulong					GetRAMSize() const					{ return fRAMSize; };
	void*					GetScreenBaseAddress()				{ return fScreenBaseAddress; };
	BitMapFormat			GetScreenBitMapFormat() const		{ return fScreenBitMapFormat; };
	ulong					GetScreenRowBytes() const;
	ulong					GetScreenSize() const;
	Boolean					GetStrictDebugging() const			{ return fStrictDebugging; };
	Boolean					GetTrivialMessagesEnabled() const 	{ return fTrivialMessagesEnabled; };
	Boolean					GetUseBackBuffer() const			{ return fUseBackBuffer; };
	Boolean					GetUseNVStore() const				{ return fUseNVStore; };
	Boolean					GetUseROMStore() const				{ return fUseROMStore; };

	// Setters
	void					SetFidoEnabled(Boolean flag)			{ fFidoEnabled = flag; };
	void					SetForceAntialiasText(Boolean flag)		{ fForceAntialiasText = flag; };
	void					SetInDemoMode(Boolean flag)				{ fInDemoMode = flag; };
	void					SetIREnabled(Boolean flag)				{ fIREnabled = flag; };
	void					SetIsInitialized(Boolean flag)			{ fIsInitialized = flag; };
	void					SetLimitNetSpeed(ulong speed)			{ fLimitNetSpeed = speed; };
	void					SetLoginName(const char* name);
	void					SetLoginPort(ushort port);
	void					SetPreregistrationName(const char* name);
	void					SetPreregistrationPort(ushort port);
	void					SetProxyName(const char* name);
	void					SetProxyPort(ushort port);
	void 					SetRAMSize(ulong ramSize)				{ fRAMSize = ramSize; };
	void					SetScreenBitMapFormat(BitMapFormat bmf) { fScreenBitMapFormat = bmf; };
	void					SetStrictDebugging(Boolean flag)		{ fStrictDebugging = flag; };
	void					SetTrivialMessagesEnabled(Boolean flag)	{ fTrivialMessagesEnabled = flag; };
	void					SetUseBackBuffer(Boolean flag)			{ fUseBackBuffer = flag; };
	void					SetUseNVStore(Boolean flag)				{ fUseNVStore = flag; };
	void					SetUseROMStore(Boolean flag)			{ fUseROMStore = flag; };

	// Operations
	virtual void 			EmergencyExit(Error);
	void 					FinalizeScreenRAM(void);
	void					HandleKey(char key, short modifiers, Boolean autoRepeat);
	void 					InitializeScreenRAM(void);

	// Preferences
	virtual Boolean			GetPreference(const char* name, void** buffer, size_t* bufLength) const;
	Boolean					GetPreferenceBoolean(const char* name, Boolean* valuePtr) const;
	Boolean					GetPreferenceChar(const char* name, char* valuePtr) const;
	Boolean					GetPreferenceUChar(const char* name, unsigned char* valuePtr) const;
	Boolean					GetPreferenceShort(const char* name, short* valuePtr) const;
	Boolean					GetPreferenceUShort(const char* name, unsigned short* valuePtr) const;
	Boolean					GetPreferenceLong(const char* name, long* valuePtr) const;
	Boolean					GetPreferenceULong(const char* name, unsigned long* valuePtr) const;
	Boolean					GetPreferenceString(const char* name, const char** valuePtr) const;
	Boolean					GetPreferenceFontSizeRecord(const char* name, FontSizeRecord* valuePtr) const;
	Boolean					GetPreferenceServerType(const char* name, ServerType* valuePtr) const;

	virtual Boolean			SetPreference(const char* name, const void* buffer, size_t bufLength);
	Boolean					SetPreferenceBoolean(const char* name, Boolean value);
	Boolean					SetPreferenceUChar(const char* name, unsigned char value);
	Boolean					SetPreferenceChar(const char* name, char value);
	Boolean					SetPreferenceUShort(const char* name, unsigned short value);
	Boolean					SetPreferenceShort(const char* name, short value);
	Boolean					SetPreferenceULong(const char* name, unsigned long value);
	Boolean					SetPreferenceLong(const char* name, long value);
	Boolean					SetPreferenceString(const char* name, const char* value);
	Boolean					SetPreferenceFontSizeRecord(const char* name, const FontSizeRecord* value);
	Boolean					SetPreferenceServerType(const char* name, ServerType value);

	virtual Boolean			RemovePreference(const char* name);

	virtual void			LoadPreferences();
	virtual void			SavePreferences();
	virtual void			OpenPreferences();
	virtual void			ClosePreferences();
	
protected:
	Boolean					fFidoEnabled;
	Boolean					fForceAntialiasText;
	Boolean					fInDemoMode;
	Boolean					fIREnabled;
	Boolean					fIsInitialized;
	ulong					fLimitNetSpeed;
	char					fLoginName[256];
	ushort					fLoginPort;
	char					fPreregistrationName[256];
	ushort					fPreregistrationPort;
	char					fProxyName[256];
	ushort					fProxyPort;
	ulong					fRAMSize;
	BitMapFormat			fScreenBitMapFormat;
	Boolean					fStrictDebugging;
	Boolean					fTrivialMessagesEnabled;
	Boolean					fUseBackBuffer;
	Boolean					fUseNVStore;
	Boolean					fUseROMStore;

	// no setters for these...they get set when Simulator gets Initialized
	char*					fRAMBaseAddress;
	void*					fScreenBaseAddress;
};

inline void
Simulator::SetLoginName(const char* name)
{
	snprintf(fLoginName, sizeof(fLoginName), "%s", (name==nil) ? kEmptyString : name);
	if (GetServerType() == kServerTypeLogin) {
		SetServerName(name);
	}
}

inline void
Simulator::SetLoginPort(ushort port)
{
	fLoginPort = port;
	if (GetServerType() == kServerTypeLogin) {
		SetServerPort(port);
	}
}

inline void
Simulator::SetPreregistrationName(const char* name)
{
	snprintf(fPreregistrationName, sizeof(fPreregistrationName), "%s", (name==nil) ? kEmptyString : name);
	if (GetServerType() == kServerTypePreregistration) {
		SetServerName(name);
	}
}

inline void
Simulator::SetPreregistrationPort(ushort port)
{
	fPreregistrationPort = port;
	if (GetServerType() == kServerTypePreregistration) {
		SetServerPort(port);
	}
}

inline void
Simulator::SetProxyName(const char* name)
{
	snprintf(fProxyName, sizeof(fProxyName), "%s", (name==nil) ? kEmptyString : name);
	if (GetServerType() == kServerTypeProxy) {
		SetServerName(name);
	}
}

inline void
Simulator::SetProxyPort(ushort port)
{
	fProxyPort = port;
	if (GetServerType() == kServerTypeProxy) {
		SetServerPort(port);
	}
};

inline const char*
Simulator::GetLoginName()
{
	if (GetServerType() == kServerTypeLogin) {
		SetLoginName(GetServerName());
	}
	return fLoginName;
}

inline ushort
Simulator::GetLoginPort()
{
	if (GetServerType() == kServerTypeLogin) {
		fLoginPort = GetServerPort();
	}
	return fLoginPort;
}

inline const char*
Simulator::GetPreregistrationName()
{
	if (GetServerType() == kServerTypePreregistration) {
		SetPreregistrationName(GetServerName());
	}
	return fPreregistrationName;
}

inline ushort
Simulator::GetPreregistrationPort()
{
	if (GetServerType() == kServerTypePreregistration) {
		fPreregistrationPort = GetServerPort();
	}
	return fPreregistrationPort;
}

inline const char*
Simulator::GetProxyName()
{
	if (GetServerType() == kServerTypeProxy) {
		SetProxyName(GetServerName());
	}
	return fProxyName;
}

inline ushort
Simulator::GetProxyPort()
{
	if (GetServerType() == kServerTypeProxy) {
		fProxyPort = GetServerPort();
	}
	return fProxyPort;
}

// =============================================================================
//	Default simulator settings

extern const Boolean		kDefaultFidoEnabled;
extern const Boolean		kDefaultForceAntialiasText;
extern const Boolean		kDefaultForceRegistration;
extern const Boolean		kDefaultInDemoMode;
extern const Boolean		kDefaultIREnabled;

extern const ulong			kDefaultLimitNetSpeed;
extern const ulong			kDefaultRAMSize;
extern const BitMapFormat	kDefaultScreenBitMapFormat;
extern const Boolean 		kDefaultStrictDebugging;
extern const Boolean		kDefaultTrivialMessagesEnabled;

extern const Boolean		kDefaultUseBackBuffer;
extern const Boolean		kDefaultUseNVStore;
extern const Boolean		kDefaultUseROMStore;

// =============================================================================
//	Preferences

// These are from the System.
extern const char kPrefsCacheScramble[];
extern const char kPrefsCacheValidate[];
extern const char kPrefsDisplayMode[];
extern const char kPrefsFontMonospaced[];
extern const char kPrefsFontProportional[];
extern const char kPrefsFontMonospacedSizeRecord[];
extern const char kPrefsFontProportionalSizeRecord[];
extern const char kPrefsInAttractMode[];
extern const char kPrefsLoggingEnabled[];
extern const char kPrefsServerType[];
extern const char kPrefsUsePhone[];
extern const char kPrefsUserAgent[];

// These are from the Simulator
extern const char kPrefsFidoEnabled[];
extern const char kPrefsForceAntialiasText[];
extern const char kPrefsForceRegistration[];
extern const char kPrefsInDemoMode[];
extern const char kPrefsIREnabled[];
extern const char kPrefsLimitNetSpeed[];
extern const char kPrefsLoginName[];
extern const char kPrefsLoginPort[];
extern const char kPrefsPreregistrationName[];
extern const char kPrefsPreregistrationPort[];
extern const char kPrefsProxyName[];
extern const char kPrefsProxyPort[];
extern const char kPrefsRAMSize[];
extern const char kPrefsScreenBitMapFormat[];
extern const char kPrefsStrictDebugging[];
extern const char kPrefsTrivialMessagesEnabled[];
extern const char kPrefsUseBackBuffer[];
extern const char kPrefsUseFlickerFilter[];
extern const char kPrefsUseNVStore[];
extern const char kPrefsUseROMStore[];
// =============================================================================

extern Simulator* gSimulator;

#ifndef FOR_MAC
#error "No system defined for this platform"
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Simulator.h multiple times"
	#endif
#endif /* __SIMULATOR_H__ */