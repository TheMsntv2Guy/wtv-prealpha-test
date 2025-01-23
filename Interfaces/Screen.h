// ===========================================================================
//	Screen.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SCREEN_H__
#define __SCREEN_H__

#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif
#ifndef __INPUT_H__
#include "Input.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

class ContentView;
class Layer;
class Panel;
class TextField;

// ===========================================================================

class Screen : public HandlesInput, public HasBounds {
public:
							Screen();
	virtual					~Screen();

	Panel*					GetCurrentPanel() const;
	TextField*				GetKeyboardTarget() const;	
	ContentView*			GetTopContentView() const;
	Layer*					GetTopLayer() const;
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	Boolean					GetIsDirty()						{ return fIsDirty; };
#endif
	void					SetIsDirty(Boolean newValue)		{ fIsDirty = newValue; };
	Boolean					GetInDrawingLoop()					{ return fInDrawingLoop; };
	void					SetInDrawingLoop(Boolean newValue)	{ fInDrawingLoop = newValue; };

	void					CloseAllPanels();	
	virtual Boolean			DispatchInput(Input*);
	void					Draw();
	virtual Boolean			HomeInput();
	virtual Boolean			KeyboardInput(Input*);
	virtual Boolean			OptionsInput();
	virtual Boolean			RecentInput();
	void					RedrawNow();
	void					ReloadPage();
	virtual Boolean			SearchInput();
	void					SetCurrentPanel(Panel*);
	void					SlideAreaDown(const Rectangle* bounds, ulong scrollIncrement,
										  ulong distance, ushort shadowWidth = 0);
	void					SlideAreaUp(const Rectangle* bounds, ulong scrollIncrement,
										ulong distance, ulong startOffset = 0, ushort shadowWidth = 0);
	void					SlidePageAndAreaDown(const Rectangle* pageBounds, ulong scrollIncrement, ulong distance);
	void					SlideViewAndAreaDown(const Rectangle* viewBounds, const Rectangle* areaBounds,
												 ulong scrollIncrement, ulong distance);
	void					SlideViewAndAreaUp(const Rectangle* viewBounds, const Rectangle* areaBounds,
											   ulong scrollIncrement, ulong distance);
	void					DisableOptionsPanel();
	void					EnableOptionsPanel();
	void					HideOptionsPanel();
	void					ShowOptionsPanel();

#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize();
#endif /* INCLUDE_FINALIZE_CODE */
	static void				Initialize();
	
protected:
	void					CreateBorders();
	void					CreateImages();
	void					CreateLayers();
	void					CreateSoundEffects();
	void					DestroyBorders();
	void					DestroyImages();
	void					DestroyLayers();
	void					DestroySoundEffects();
	void					Layout();

protected:
	Boolean					fIsDirty;
	Boolean					fInDrawingLoop;
#ifdef FIDO_INTERCEPT
public:
#endif
	Layer*					fLayerList;			// front to back order
#ifdef FIDO_INTERCEPT
protected:
#endif
	Panel*					fCurrentPanel;
};

// ===========================================================================
//	globals
extern class Screen* 				gScreen;

extern class Region*				gSelectionRegion;
#ifdef FIDO_INTERCEPT
extern class Region*				gFidoSelectionRegion;
#endif

// ===========================================================================
// Screen Layers

extern class AlertWindow* 			gAlertWindow;
extern class ConnectIndicator*		gConnectIndicator;
extern class ConnectWindow*			gConnectWindow;
extern class Panel*					gFindPanel;
extern class Panel*					gAutoPanel;
extern class Panel*					gGoPanel;
extern class InfoPanel*				gInfoPanel;
extern class Keyboard*				gKeyboard;
extern class LoginPanel*			gLoginPanel;
extern class MenuLayer*				gMenuLayer;
extern class OptionsPanel*			gOptionsPanel;
extern class PageViewer* 			gPageViewer;
extern class PowerOffAlert*			gPowerOffAlert;
extern class RecentPanel*			gRecentPanel;
extern class Panel*					gSavePanel;
extern class SplashWindow*			gSaveSplash;
extern class Panel*					gSendPanel;
extern class SplashWindow*			gSendSplash;
extern class StatusIndicator*		gStatusIndicator;
extern class SystemLogo* 			gSystemLogo;

#define kAlertWindowURL				"AlertWindow.html"
#define kConnectWindowURL			"ConnectWindow.html"
#define kFindPanelURL				"file://ROM/FindPanel.html"
#define kAutoPanelURL				"file://ROM/AutoPanel.html"
#define kGoPanelURL					"file://ROM/GotoPanel.html"
#define kInfoPanelURL				"InfoPanel.html"
#define kLoginPanelURL				"file://ROM/LoginPanel.html"
#define kMenuLayerURL				"Menu.html"
#define kOptionsPanelURL			"file://ROM/OptionsPanel.html"
#define kRecentPanelURL				"RecentPanel.html"
#define kSavePanelURL				"file://ROM/SavePanel.html"
#define kSaveSplashURL				"file://ROM/SaveSplash.html"
#define kSendPanelURL				"file://ROM/SendPanel.html"
#define kSendSplashURL				"file://ROM/SendSplash.html"
#define kSystemLogoURL				"file://ROM/images/logo.gif"

// ===========================================================================
// Borders

extern class BorderImage* 			gAlertBorder;
extern class BorderImage*			gButtonBorder;
extern class BorderImage*			gMenuBorder;
extern class BorderImage*			gPanelBorder;
extern class BorderImage*			gRuleBorder;
extern class BorderImage*			gSelectionBorder;
extern class BorderImage*			gTextFieldBorder;

#define kAlertBorderURL				"file://ROM/Borders/alert.bif"
#define kButtonBorderURL			"file://ROM/Borders/buttonborder.bif"
#define kMenuBorderURL				"file://ROM/Borders/menuborder.bif"
#define kPanelBorderURL				"file://ROM/Borders/panelborder.bif"
#define kRuleBorderURL				"file://ROM/Borders/ruleborder.bif"
#define kSelectionBorderURL			"file://ROM/Borders/selection.bif"
#define kTextFieldBorderURL			"file://ROM/Borders/textfield.bif"

// ===========================================================================
// ImageData

extern class ImageData* 			gAlertImage;
extern class ImageData* 			gCheckBoxImage;
extern class ImageData* 			gCheckedBoxImage;
extern class ImageData*				gContinueImage;
extern class ImageData*				gMenuIconImage;
extern class ImageData*				gMenuUpIconImage;
extern class ImageData*				gMenuDownIconImage;
extern class ImageData*				gRadioButtonOffImage;
extern class ImageData*				gRadioButtonOnImage;
extern class ImageData*				gSaveImage;

#define kAlertImageURL				"file://ROM/Images/Television.gif"
#define kCheckBoxImageURL			"file://ROM/Images/Checkbox.gif"
#define kCheckedBoxImageURL			"file://ROM/Images/Checkedbox.gif"
#define kContinueImageURL			"file://ROM/Images/ContinueButton.gif"
#define kMenuIconImageURL			"file://ROM/Images/MenuIcon.gif"
#define kMenuUpIconImageURL			"file://ROM/Images/MenuIcon.gif"
#define kMenuDownIconImageURL		"file://ROM/Images/MenuIcon.gif"
#define kPowerOffImageURL			"file://ROM/Images/PowerOffButton.gif"
#define kRadioButtonOffImageURL		"file://ROM/Images/RadioButtonOff.gif"
#define kRadioButtonOnImageURL		"file://ROM/Images/RadioButtonOn.gif"
#define kSaveImageURL				"file://ROM/Images/Chest3.gif"

// ===========================================================================
// ImageMapCursor

extern class ImageMapCursor*			gImageMapCursorActive;
extern class ImageMapCursor*			gImageMapCursorNormal;

#define kImageMapCursorActive 			"file://ROM/Images/ActiveImageCursor.gif"
#define kImageMapCursorActiveHotspotX	32
#define kImageMapCursorActiveHotspotY	32

#define kImageMapCursorNormal 			"file://ROM/Images/ImageCursor.gif"
#define kImageMapCursorNormalHotspotX	32
#define kImageMapCursorNormalHotspotY	32

// ===========================================================================
// Sound Effects

extern class SoundEffect*		gEndLinkSound;
extern class SoundEffect*		gErrorSound;
extern class SoundEffect*		gLimitSound;
extern class SoundEffect*		gModemSound;
extern class SoundEffect*		gPopupSound;
extern class SoundEffect*		gSaveSound;
extern class SoundEffect*		gScroll1Sound;
extern class SoundEffect*		gScroll2Sound;
extern class SoundEffect*		gStartLinkSound;
extern class SoundEffect*		gSubmitSound;
extern class SoundEffect*		gTypeSound;


#define kEndLinkSound		"file://ROM/Sounds/EndLink.mid"
#define kEndLinkVolume		200

#define kErrorSound			"file://ROM/Sounds/Error.mid"
#define kErrorVolume		400

#define kLimitSound			"file://ROM/Sounds/Limit.mid"
#define kLimitVolume		400

#define kModemSound			"file://ROM/Sounds/Modem.mid"
#define kModemVolume		350

#define kPopupSound			"file://ROM/Sounds/Popup.mid"
#define kPopupVolume		300

#define kSaveSound			"file://ROM/Sounds/Save.mid"
#define kSaveVolume			300

#define kScroll1Sound		"file://ROM/Sounds/Scroll1.mid"
#define kScroll1Volume		200

#define kScroll2Sound		"file://ROM/Sounds/Scroll2.mid"
#define kScroll2Volume		150

#define kStartLinkSound		"file://ROM/Sounds/StartLink.mid"
#define kStartLinkVolume	200

#define kSubmitSound		"file://ROM/Sounds/Submit.mid"
#define kSubmitVolume		200

#define kTypeSound			"file://ROM/Sounds/Type.mid"
#define kTypeVolume			200
// ===========================================================================

#endif /* __SCREEN_H__ */
