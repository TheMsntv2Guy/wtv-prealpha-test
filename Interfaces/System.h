// ===========================================================================
//	System.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifndef __INPUT_H__
#include "Input.h"	/* for HandlesInput */
#endif
//#ifndef __SONGDATA_H__
//#include "SongData.h"	... why should System.h include SongData?!
//#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"	/* for HasAttributes */
#endif

#include	"HWDisplay.h"

// =============================================================================
//	default server info

#define kDefaultBackgroundColor				0x00c6c6c6
#define kDefaultBootURL						"wtv-1800:preregister"
#define kDefaultCacheScramble				false
#define kDefaultCacheValidate				false
#define kDefaultDevelopmentServerName		"foobar.artemis.com"
#define kDefaultDisplayMode					kDisplayModeNTSC
#define kDefaultFavoriteURL					""
#define kDefaultFontMonospaced				kMonaco
#define kDefaultFontProportional			kHelvetica
#define kDefaultHomeURL						"wtv-1800:preregister"
#define kDefaultInAttractMode				false
#define kDefaultLinkColor					0x002222bb
#define kDefaultLoggingEnabled				false
#define kDefaultLoginServerPort				1601
#define kDefaultPreregistrationServerPort	1615
#define kDefaultPreregistrationService		"wtv-1800"
#define kDefaultProductionServerName		"10.0.0.2"
#define kDefaultProxyServerName				"10.0.0.2"
#define kDefaultProxyServerPort				80
#define kDefaultServerPort					1615
#define kDefaultServerType					kServerTypePreregistration
#define kDefaultStartupURL					"file://ROM/Startup.html"
//#define kDefaultTestServerName				"testimony.artemis.com" // EMAC: disablinng so we can connect
#define kDefaultTestServerName				"10.0.0.2"
#define kDefaultTextColor					kBlackColor
#define kDefaultUseFlickerFilter			true
#define kDefaultUseJapanese					false
#define kDefaultUsePhone					true
#define kDefaultUserAgent					"Mozilla/1.1N, Artemis/0.0"
#define kDefaultVisitedLinkColor			0x008822bb

#if defined EXTERNAL_BUILD
#define kDefaultNameServer					QuadChar(10,0,0,2)
#define kDefaultLoginServerName				kDefaultProductionServerName
#define kDefaultPreregistrationServerName	kDefaultProductionServerName
#define kDefaultServerName					kDefaultProductionServerName

#elif defined DEBUG
#define kDefaultNameServer					QuadChar(10,0,0,2)
#define kDefaultLoginServerName				kDefaultTestServerName
#define kDefaultPreregistrationServerName	kDefaultTestServerName
#define kDefaultServerName					kDefaultTestServerName

#else
#define kDefaultNameServer					QuadChar(10,0,0,2)
#define kDefaultLoginServerName				kDefaultTestServerName
#define kDefaultPreregistrationServerName	kDefaultTestServerName
#define kDefaultServerName					kDefaultTestServerName
#endif

// =============================================================================

typedef struct {
	uchar size[8];	// picked eight out of thin air...now there's a dialog box 
					// in MacintoshDialogs.c that allows you to enter eight values,
					// so if you make this less than eight, it'll overwrite the
					// array, and if you make this LESS than eight, it just won't
					// be able to edit the extra entries in the array.
} FontSizeRecord;

typedef enum {
	kServerTypePreregistration = 0,
	kServerTypeLogin,
	kServerTypeProxy,
	kServerTypeNone,
	kNumServerTypes
} ServerType;

const long	kFontSizesSmall = -1;
const long	kFontSizesMedium = 0;
const long	kFontSizesLarge = 1;

class System : public HandlesInput, public HasAttributes {
public:
							System();
	virtual					~System();
	
	virtual void			Initialize();
#ifdef INCLUDE_FINALIZE_CODE
	virtual void			Finalize();
#endif /* INCLUDE_FINALIZE_CODE */
	
	// Getters
	const char*				GetBootURL() const						{ return *fBootURL == 0 ? nil : fBootURL; };
#ifdef DEBUG_CACHE_SCRAMBLE
	Boolean					GetCacheScramble() const				{ return fCacheScramble; };
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
	Boolean					GetCacheValidate() const				{ return fCacheValidate; };
#endif /* DEBUG_CACHE_VALIDATE */
	const char*				GetCardNumber() const;
	virtual const char*		GetClientSerialNumber() const;
	ulong					GetDefaultBackgroundColor() const		{ return fDefaultBackgroundColor; };
	ulong					GetDefaultTextColor() const				{ return fDefaultTextColor; };
	ulong					GetDefaultLinkColor() const				{ return fDefaultLinkColor; };
	ulong					GetDefaultVisitedLinkColor() const		{ return fDefaultVisitedLinkColor; };
#ifdef SIMULATOR
	DisplayMode				GetDisplayMode() const					{ return fDisplayMode; };
#endif
	long					GetFontSizes()							{ return (long)fFontSizesSelector; }
	const char*				GetFavoriteURL() const					{ return *fFavoriteURL == 0 ? nil : fFavoriteURL; };
	ulong					GetFontMonospaced() const				{ return fFontMonospaced; };
	const FontSizeRecord*	GetFontMonospacedSizeRecord() const		{ return &fFontMonospacedSizeRecord; };
	ulong					GetFontProportional() const				{ return fFontProportional; };
	const FontSizeRecord*	GetFontProportionalSizeRecord() const	{ return &fFontProportionalSizeRecord; };
	Boolean					GetForceRegistration() const			{ return fForceRegistration; };
	const char*				GetHomeURL() const						{ return *fHomeURL == 0 ? nil : fHomeURL; };
#ifdef SIMULATOR
	Boolean					GetInAttractMode() const				{ return fInAttractMode; };
#endif
	Boolean					GetInStereo() const						{ return fInStereo; };
	Boolean					GetIsOn() const							{ return fIsOn; };
	const char*				GetLogURL() const						{ return *fLogURL == 0 ? nil : fLogURL; };
#ifdef SIMULATOR
	Boolean					GetLoggingEnabled() const 				{ return fLoggingEnabled; };
#endif
#ifdef FOO_DEBUG
	Boolean					GetLoginRequired() const				{ return (fServerType == kServerTypeLogin); };
#endif
	const char*				GetNotificationsURL() const				{ return *fNotificationsURL == 0 ? nil : fNotificationsURL; };
	ulong					GetNameServer() const					{ return fNameServer; };
	const char*				GetPowerOffURL() const					{ return *fPowerOffURL == 0 ? nil : fPowerOffURL; };
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	Boolean					GetPreregistrationRequired() const		{ return (fServerType == kServerTypePreregistration); };
	const char*				GetPreregistrationService() const		{ return *fPreregistrationService == 0 ? nil : fPreregistrationService; };	
#endif
	const char*				GetReconnectURL() const					{ return *fReconnectURL == 0 ? nil : fReconnectURL; };
	const char*				GetSearchURL() const					{ return *fSearchURL == 0 ? nil : fSearchURL; };
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	ulong					GetServerAddress() const				{ return fServerAddress; };
#endif
	const char*				GetServerName() const					{ return *fServerName == 0 ? nil : fServerName; };
	ushort					GetServerPort() const					{ return fServerPort; };
	ServerType				GetServerType() const					{ return fServerType; };
	Boolean					GetSongsMuted() const					{ return fSongsMuted; };
	Boolean					GetSoundEffectsMuted() const			{ return fSoundEffectsMuted; };
	const char*				GetStartupURL() const					{ return *fStartupURL == 0 ? nil : fStartupURL; };
	virtual Boolean			GetUsePhone() const						{ return fUsePhone; };
	Boolean					GetUseFlickerFilter() const				{ return fUseFlickerFilter; };
	Boolean					GetUseJapanese() const					{ return fUseJapanese; };
	Boolean					GetUseProxy() const						{ return (fServerType == kServerTypeProxy); };
	const char*				GetUserAgent() const					{ return *fUserAgent == 0 ? nil : fUserAgent; };
	const char*				GetUserSignature() const;
	Boolean					GetUsingHardKeyboard() const			{ return fUsingHardKeyboard; };
	Boolean					GetUsingPhoneSetup() const				{ return fUsingPhoneSetup; };
	Boolean					GetUsingConnectSetup() const			{ return fUsingConnectSetup; };
	const char*				GetVersion() const;
	ulong					GetVersionNumber() const;

	// Setters
	virtual Boolean			SetAttribute(const char* name, char* value);
	void					SetBootURL(const char*);
#ifdef DEBUG_CACHE_SCRAMBLE
	void					SetCacheScramble(Boolean value) 		{ fCacheScramble = value; };
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
	void					SetCacheValidate(Boolean value) 		{ fCacheValidate = value; };
#endif /* DEBUG_CACHE_VALIDATE */
	void					SetDefaultBackgroundColor(ulong color)	{ fDefaultBackgroundColor = color; };
	void					SetDefaultLinkColor(ulong color)		{ fDefaultLinkColor = color; };
	void					SetDefaultTextColor(ulong color)		{ fDefaultTextColor = color; };
	void					SetDefaultVisitedLinkColor(ulong color)	{ fDefaultVisitedLinkColor = color; };
	void 					SetDisplayMode(DisplayMode mode);
	void					SetFavoriteURL(const char*);
	void					SetFontMonospaced(ulong newFont)		{ fFontMonospaced = newFont; };
	void					SetFontMonospacedSizeRecord(const FontSizeRecord* fsr) { fFontMonospacedSizeRecord = *fsr; };
	void					SetFontProportional(ulong newFont)		{ fFontProportional = newFont; };
	void					SetFontProportionalSizeRecord(const FontSizeRecord* fsr) { fFontProportionalSizeRecord = *fsr; };
	void					SetFontSizes(long fontSizesSelector);
#ifdef SIMULATOR
	void					SetForceRegistration(Boolean value)		{ fForceRegistration = value; };
#endif
	void					SetHomeURL(const char*);
	void					SetInAttractMode(Boolean value)			{ fInAttractMode = value; };
	void					SetInStereo(Boolean value)				{ fInStereo = value; };
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	void					SetIsOn(Boolean value)					{ fIsOn = value; };
#endif
	void					SetLogURL(const char*);
	void					SetLoggingEnabled(Boolean value)		{ fLoggingEnabled = value; };
	void					SetMailCount(ushort);
	void					SetNameServer(ulong address);
	void					SetNotificationsURL(const char*);
	void					SetPowerOffURL(const char*);
	void					SetPreregistrationService(const char*);
	void					SetReconnectURL(const char*);
	void					SetSearchURL(const char*);
	void					SetServerAddress(ulong address);
	void					SetServerName(const char* name);
	void					SetServerPort(ushort port)				{ fServerPort = port; };
	void					SetServerType(ServerType serverType)	{ fServerType = serverType; };
	void					SetSongsMuted(Boolean value)			{ fSongsMuted = value; };
	void					SetSoundEffectsMuted(Boolean value)		{ fSoundEffectsMuted = value; };
	void					SetStartupURL(const char*);
	void					SetUseFlickerFilter(Boolean flag)		{ fUseFlickerFilter = flag; };
	void					SetUseJapanese(Boolean flag)			{ fUseJapanese = flag; };
	void					SetUserAgent(const char*);
	void					SetUsePhone(Boolean value)				{ fUsePhone = value; };
	void					SetUsingHardKeyboard(Boolean value)		{ fUsingHardKeyboard = value; };
	void					SetUsingPhoneSetup(Boolean value)		{ fUsingPhoneSetup = value; };
	void					SetUsingConnectSetup(Boolean value)		{ fUsingConnectSetup = value; };
	void					SaveState(void);
	void					RestoreState(void);


	// Operations
	virtual Boolean			DispatchInput(Input*);
	void					LoadSettings();
	virtual void			PowerOff();
	virtual void			PowerOn();
	virtual void			ShowStartupScreen();
	virtual void			UserLoopIteration();
	virtual void			WriteAttributes(Stream*);

#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintURLs				= kFirstBoxPrint << 0,
		kBoxPrintServers			= kFirstBoxPrint << 1,
		kBoxPrintUserAgent			= kFirstBoxPrint << 2,
		kBoxPrintMailCount			= kFirstBoxPrint << 3,
		kBoxPrintFlags				= kFirstBoxPrint << 4,
		kBoxPrintPreregistration	= kFirstBoxPrint << 5,
		kLastBoxPrint = kBoxPrintPreregistration
	};
	void					BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const System* network, long whatToPrint);
#endif

protected:
	void					CheckSettingsLoaded();
	virtual Boolean			PowerInput();

protected:
	char					fBootURL[256];
	ulong					fDefaultBackgroundColor;
	ulong					fDefaultLinkColor;
	ulong					fDefaultTextColor;
	ulong					fDefaultVisitedLinkColor;
	char					fFavoriteURL[128];
	ulong					fFontMonospaced;
	ulong					fFontProportional;
	FontSizeRecord			fFontMonospacedSizeRecord;
	FontSizeRecord			fFontProportionalSizeRecord;
	char					fHomeURL[256];
	ulong					fLastInputTime;
	char					fLogURL[256];
	ulong					fNameServer;
	char					fNotificationsURL[256];
	char					fPowerOffURL[256];
	char					fPreregistrationService[256];
	char					fReconnectURL[256];
	char					fSearchURL[256];
	ulong					fServerAddress;
	char					fServerName[32];	
	char					fStartupURL[256];
	char					fUserAgent[256];

	ushort					fMailCount;	
	ushort					fServerPort;
	Resource*				fSettingsResource;

	DisplayMode				fDisplayMode;
	ServerType				fServerType;	// prereg, login, proxy, none
	
#ifdef DEBUG_CACHE_SCRAMBLE
	Boolean					fCacheScramble;
#endif /* DEBUG_CACHE_SCRAMBLE */
#ifdef DEBUG_CACHE_VALIDATE
	Boolean					fCacheValidate;
#endif /* DEBUG_CACHE_VALIDATE */
	Boolean					fUseJapanese;
	Boolean					fForceRegistration;
	Boolean					fInAttractMode;
	Boolean					fIsOn;
	Boolean					fLoggingEnabled;
	Boolean					fUseFlickerFilter;
	Boolean					fUsePhone;
	Boolean					fUsingHardKeyboard;
	Boolean					fUsingPhoneSetup;
	Boolean					fUsingConnectSetup;
	Boolean					fSongsMuted;
	Boolean					fSoundEffectsMuted;
	Boolean					fInStereo;
	Boolean					fColdBoot;

	short					fFontSizesSelector;
};

// =============================================================================
//	Some default URLs

extern const char* kAboutURL;
extern const char* kConnectSetupURL;
extern const char* kPhoneSetupURL;
extern const char* kPowerOffURL;

// =============================================================================

extern System* gSystem;
extern const FontSizeRecord kDefaultFontMonospacedSizeRecord;
extern const FontSizeRecord kDefaultFontProportionalSizeRecord;

#ifndef SIMULATOR
	void UserTaskMain(void);
#endif

// =============================================================================

#endif /* __SYSTEM_H__ */
