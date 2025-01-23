//
//	SimulatorWindow.h
//

#ifndef __SIMULATORWINDOW_H__
#define __SIMULATORWINDOW_H__

#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif

struct SimulatorWindowPrefs : public StdWindowPrefs
{
	DisplayMode			displayMode;
	Boolean				isCentered;
};

enum
{
	kSimulatorWindowProcIDNormal = noGrowDocProc,
	kSimulatorWindowProcIDPhilTV = (rPhilsTVWDEF << 4),
	kDefaultSimulatorWindowProcID = kSimulatorWindowProcIDPhilTV
};

class SimulatorWindow : public StdWindow
{
public:
							SimulatorWindow();
	virtual					~SimulatorWindow();

// get/set display characteristics
	DisplayMode				GetDisplayMode() const 		{ return fDisplayMode; };
	void					SetDisplayMode(DisplayMode);
	void					SetUseTVWDEF(Boolean flag);

// put window in an appropriate location
	void					CenterWindowOnScreen(GDHandle gdHandle = nil);

// override StdWindow functionality...
	virtual void			Close();
	virtual	void			Draw();

	virtual void			DoAdjustMenus(ushort modifiers);
	virtual Boolean			DoMenuChoice(long menuChoice, ushort modifiers);

	virtual void			DragWindow(struct MacPoint where);
	virtual void			ShowWindow();

	virtual Boolean			SavePrefs(StdWindowPrefs* prefPtr = nil);
	virtual Boolean			RestorePrefs(StdWindowPrefs* prefPtr = nil);
	virtual long			GetPrefsSize(void);

protected:
	DisplayMode				fDisplayMode;
	Boolean					fIsCentered;
	Boolean					fUseTVWDEF;
};

extern SimulatorWindow* gSimulatorWindow;

//extern const char kPrefsSimulatorWindowPosition[];

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include SimulatorWindow.h multiple times"
	#endif
#endif /* __SIMULATORWINDOW_H__ */