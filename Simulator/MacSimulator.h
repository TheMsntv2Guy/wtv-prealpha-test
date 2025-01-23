// ===========================================================================
//	MacSimulator.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MACSIMULATOR_H__
#define __MACSIMULATOR_H__
 
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif

class MacSimulator : public Simulator
{
protected:
	enum
	{
		kPreferencesResourceType	= 'pref',
		kPreferencesCreator 		= 'RSED',
		kPreferencesFileType		= 'rsrc'
	};
public:
							MacSimulator(void);
	virtual					~MacSimulator(void);

	// Getters
	ulong					GetBoxLEDs() const 				{ return fBoxLEDs; };
	virtual const char* 	GetClientSerialNumber() const;
	Boolean					GetCanUseMacFlickerFilter();
	Boolean					GetPendingPowerOn() const		{ return fPendingPowerOn; };
#ifdef DEBUG
	Boolean					GetPreventComplains() const		{ return gPreventDebuggerBreaks; };
#endif
	Boolean					GetUseMacFlickerFilter() const	{ return fUseMacFlickerFilter; };
	Boolean					GetUseQuickdraw() const			{ return fUseQuickdraw; };
	Boolean					GetUseTVWDEF() const			{ return fUseTVWDEF; };
	//short					GetQuickdrawFontID(void);
	//short					GetQuickdrawFontSize(void);
	//short					GetQuickdrawFontStyle(void);
	WindowPtr				GetWindow() const				{ return (WindowPtr)fWindow; };

	// Setters
	void					SetBoxLEDs(ulong bits);
	void 					SetPendingPowerOn(Boolean flag)	{ fPendingPowerOn = flag; };
#ifdef DEBUG
	void					SetPreventComplains(Boolean flag) { gPreventDebuggerBreaks = flag; };
#endif
	void					SetUseMacFlickerFilter(Boolean);
	void					SetUseQuickdraw(Boolean flag);
	void					SetUseTVWDEF(Boolean flag);
	//void					SetQuickdrawFontID(short fontID);
	//void					SetQuickdrawFontSize(short fontSize);
	///void					SetQuickdrawFontStyle(short fontStyle);
	void 					SetWindow(WindowPtr window)		{ fWindow = (WindowPeek)window; };

	// Operations
	void 					CheckAbort();
	virtual void			EmergencyExit(Error);
	void 					ForceUpdate();
	void 					ForceUpdateAll();
	void*					GetROMFilesystemRoot();
	void 					DisposeROMFileSystemRoot();
	Boolean					GoTo(short resID);
	void 					PostPendingPowerOn();
	virtual void			PowerOff();
	virtual void			PowerOn();
	virtual void			ShowStartupScreen();
	virtual void			UserLoopIteration();
	
protected:
	void 					PowerOffVisualEffect();
	void 					PowerOnVisualEffect();

public:
	// Preferences
	virtual Boolean			GetPreference(const char* name, void** buffer, size_t* bufLength) const;
	Boolean					GetPreferencePoint(const char* name, struct MacPoint* value) const ;
	Boolean					GetPreferenceRect(const char* name, struct Rect* value) const;
	virtual Boolean			RemovePreference(const char* name);
	virtual Boolean			SetPreference(const char* name, const void* buffer, size_t bufLength);
	Boolean					SetPreferencePoint(const char* name, struct MacPoint* value);
	Boolean					SetPreferenceRect(const char* name, struct Rect* value);

	virtual void			LoadPreferences();
	virtual void			SavePreferences();
	virtual void			OpenPreferences();
	virtual void			ClosePreferences();
	virtual void			UpdatePreferences();
	
	// Performance Profiling
	void					InitializeProfiler();
	Boolean					GetIsProfiling();
	void					StartProfiling();
	void					StopProfiling();
	void					FinalizeProfiler();

private:
	struct WindowRecord*	fWindow;
	char **					fPictHdl;
	struct Rect				**fHitRectHdl;
	short					**fNextPictsHdl;
	short					fResFileID;

	ulong					fBoxLEDs;
	Boolean					fIsProfiling;
	Boolean					fPendingPowerOn;
	Boolean					fUseMacFlickerFilter;
	Boolean					fUseQuickdraw;
	Boolean					fUseTVWDEF;
};

// ===========================================================================
//	Default simulator settings

extern const Boolean kDefaultUseTVWDEF;

// ===========================================================================

extern MacSimulator* gMacSimulator;

// ===========================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MacSimulator.h multiple times"
	#endif
#endif /* __MACSIMULATOR_H__ */