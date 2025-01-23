// ===========================================================================
//	SetUIElement.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __ALERTWINDOW_H__
#include "AlertWindow.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PANEL_H__
#include "Panel.h"
#endif
#ifndef __SCREEN_H__
#include "Screen.h"
#endif
#ifndef __SETUIELEMENT_H__
#include "SetUIElement.h"
#endif
#ifndef __SONGDATA_H__
#include "SongData.h"
#endif
#ifndef __DEBUG_H__
#include "Debug.h"
#endif

// ===========================================================================

typedef void* (SetUIFunction)(void* object, const char* value, ulong param);

struct SetUITableEntry {
	const char* 	name;
	const char* 	defaultValue;
	void** 			object;
	SetUIFunction*	setUI;
	ulong			param;
};

#define SetUIPanelEntry(name) \
	{#name, k ## name ## URL, \
	(void**)&g ## name, (SetUIFunction*)SetUIPanel, 0}

#define SetUIWindowEntry(name) \
	{#name, k ## name ## URL, \
	(void**)&g ## name, (SetUIFunction*)SetUIWindow, 0}

#define SetUIBorderEntry(name) \
	{#name "Border", k ## name ## BorderURL, \
	(void**)&g ## name ## Border, (SetUIFunction*)SetUIBorderImage, 0}

#define SetUIImageEntry(name) \
	{#name "Image", k ## name ## ImageURL, \
	(void**)&g ## name ## Image, (SetUIFunction*)SetUIImageData, 0}

#define SetUICursorEntry(name) \
	{#name "Cursor", \
	kImageMapCursor ## name, \
	(void**)&gImageMapCursor ## name, (SetUIFunction*)SetUIImageMapCursor, \
	((kImageMapCursor ## name ## HotspotX << 16) && 0xffff0000) \
	+ (kImageMapCursor ## name ## HotspotY && 0x0ffff)}

#define SetUISoundEntry(name) \
	{#name "Sound", k ## name ## Sound, \
	(void**)&g ## name ## Sound, (SetUIFunction*)SetUISound, \
	k ## name ## Volume}

// ===========================================================================

const SetUITableEntry gSetUITable[] = 
{
	// ------------- layers -----------------
	SetUIWindowEntry(AlertWindow),
	SetUIWindowEntry(ConnectWindow),
	SetUIWindowEntry(SaveSplash),
	SetUIWindowEntry(SendSplash),

	SetUIPanelEntry(FindPanel),
	SetUIPanelEntry(AutoPanel),
	SetUIPanelEntry(GoPanel),
	SetUIPanelEntry(InfoPanel),
	SetUIPanelEntry(LoginPanel),
	SetUIPanelEntry(OptionsPanel),
	SetUIPanelEntry(RecentPanel),
	SetUIPanelEntry(SavePanel),
	SetUIPanelEntry(SendPanel),
	
	//	extern class ConnectIndicator*	gConnectIndicator;
	//	extern class Keyboard*			gKeyboard;
	//	extern class MenuLayer*			gMenuLayer;
	//	extern class PageViewer* 		gPageViewer;
	//	extern class StatusIndicator*	gStatusIndicator;
	//	extern class SystemLogo* 		gSystemLogo;

	// ------------- borders -----------------
	SetUIBorderEntry(Alert),
	SetUIBorderEntry(Button),
	SetUIBorderEntry(Menu),
	SetUIBorderEntry(Panel),
	SetUIBorderEntry(Rule),
	SetUIBorderEntry(Selection),
	SetUIBorderEntry(TextField),

	// ------------- images -----------------
	SetUIImageEntry(Alert),
	SetUIImageEntry(CheckBox),
	SetUIImageEntry(CheckedBox),
	SetUIImageEntry(Continue),
	SetUIImageEntry(MenuIcon),
	SetUIImageEntry(MenuUpIcon),
	SetUIImageEntry(MenuDownIcon),
	SetUIImageEntry(RadioButtonOff),
	SetUIImageEntry(RadioButtonOn),
	SetUIImageEntry(Save),
	
	SetUICursorEntry(Active),
	SetUICursorEntry(Normal),
	
	// ------------- sounds -----------------
	SetUISoundEntry(EndLink),
	SetUISoundEntry(Error),
	SetUISoundEntry(Limit),
	SetUISoundEntry(Modem),
	SetUISoundEntry(Popup),
	SetUISoundEntry(Save),
	SetUISoundEntry(Scroll1),
	SetUISoundEntry(Scroll2),
	SetUISoundEntry(StartLink),
	SetUISoundEntry(Submit),
	SetUISoundEntry(Type)
};

// ===========================================================================

Panel*
SetUIPanel(Panel* panel, const char* url, ulong UNUSED(param))
{
	if (panel != nil)
		panel->SetURL(url);
	
	return panel;
}

Window*
SetUIWindow(Window* window, const char* url, ulong UNUSED(param))
{
	if (window != nil)
		window->SetURL(url);
	
	return window;
}

BorderImage*
SetUIBorderImage(BorderImage* borderImage, const char* url, ulong UNUSED(param))
{
	if (borderImage != nil)
		delete(borderImage);
	
	borderImage = BorderImage::NewBorderImage(url);
	return borderImage;
}

ImageData*
SetUIImageData(ImageData* imageData, const char* url, ulong UNUSED(param))
{
	if (imageData != nil)
		delete(imageData);
	
	imageData = ImageData::NewImageData(url);
	return imageData;
}

ImageMapCursor*
SetUIImageMapCursor(ImageMapCursor* imageMapCursor, const char* url, ulong hotspot)
{
	if (imageMapCursor != nil)
		delete(imageMapCursor);
	
	int hpos = (hotspot >> 16) && 0xffff;
	int vpos = hotspot && 0xffff;

	int n = 0;
	sscanf(url, "%d,%d,%n", &hpos, &vpos, &n);	// can do "hpos,vpos,http://foo/cursor.img"

	imageMapCursor = ImageMapCursor::NewImageMapCursor(&(url[n]));
	imageMapCursor->SetHotspot(hpos, vpos);

	return imageMapCursor;
}

SoundEffect*
SetUISound(SoundEffect* effect, const char* url, ulong volume)
{
	if (effect != nil)
		delete(effect);

	int n = 0;
	sscanf(url, "%lu,%n", &volume, &n);	// can do "hpos,vpos,http://foo/cursor.img"

	effect = SoundEffect::NewSoundEffect(url, volume, true);
	return effect;
}

// ===========================================================================

void
SetUIElement(const char* name, const char* value)
{
	// find entry for this name...if none exists, warn and continue
	
	long entry=0;
	while (entry < sizeof(gSetUITable)/sizeof(gSetUITable[0])) {
		if (EqualString(name, gSetUITable[entry].name)) {
			break;
		}
		entry++;
	}

	if (IsWarning(entry >= sizeof(gSetUITable)/sizeof(gSetUITable[0]))) {
		#ifdef DEBUG
			LogMessage("SetUIElement didn't understand name=%s", name);
		#endif
		return;		
	}

	if ((value == nil) || EqualString(value, "default"))
		value = gSetUITable[entry].defaultValue;
	
	// set ui element to this value
	*(gSetUITable[entry].object) =
		(gSetUITable[entry].setUI)(*(gSetUITable[entry].object), value, gSetUITable[entry].param);
}

void
ResetUIElement(const char* name)
{
	SetUIElement(name, nil);
}

void
ResetUIElements(void)
{
	for (long entry = 0;
		 entry < sizeof(gSetUITable)/sizeof(gSetUITable[0]);
		 entry++) {
		*(gSetUITable[entry].object) =
			(gSetUITable[entry].setUI)(*(gSetUITable[entry].object),
									   gSetUITable[entry].defaultValue,
									   gSetUITable[entry].param);
	}
}

// ===========================================================================

static void HelpSetUI(void);
static void
HelpSetUI(void)
{
	// spit out stream with options/syntax to "client:SetUI"
}

// ===========================================================================

void
SetUI(const StringDictionary* argv)
{
	long argc = argv->GetCount();
	
	for (int index=0; index<argc; index++) {
		// get ith key
		const char* name = argv->GetKeyAt(index);
		if (EqualString(name, "ResetAll")) {
			ResetUIElements();
		}
		else if (EqualString(name, "Reset")) {
			ResetUIElement(argv->GetValueAt(index));
		}
		else if (EqualString(name, "Help")) {
			HelpSetUI();
		}
		if (name != nil) {
			SetUIElement(name, argv->GetValueAt(index));
		}
	}
}















