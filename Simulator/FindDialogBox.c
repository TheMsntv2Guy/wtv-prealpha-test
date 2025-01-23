// ===========================================================================
//	FindDialogBox.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __FINDDIALOGBOX_H__
#include "FindDialogBox.h"
#endif

#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif

#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif




// ===========================================================================
//	globals/locals/defines
// ===========================================================================

enum
{
	kOKButton = 1,
	kCancelButton = 2,
	kStaticCaption = 3,
	kFindStringEditText = 4,
	kCaseSensitiveCheckBox = 5,
	kEntireWordCheckBox = 6,
	kWrapSearchCheckBox = 7
};

typedef struct
{
	//MacPoint	location;	// maybe we'll make it movable modal some day?
	Str255		lastSearchString;
	MacBoolean	caseSensitive;
	MacBoolean	entireWord;
	MacBoolean	wrapSearch;	
}
FindDialogBoxPreferences;

const char kFindDialogBoxPreferences[] = "Find Dialog Box Preferences";
Str255 kDefaultSearchString = "\p";
static char gLastSearchCString[256];
static FindDialogBoxPreferences gDefaultFindDialogPrefs = {"\p", false, false, false};




// ===========================================================================
//	implementations
// ===========================================================================

MacBoolean
GetFindDialogCaseSensitive(void)
{
	return gDefaultFindDialogPrefs.caseSensitive;
}
MacBoolean
GetFindDialogEntireWord(void)
{
	return gDefaultFindDialogPrefs.entireWord;
}
MacBoolean
GetFindDialogWrapSearch(void)
{
	return gDefaultFindDialogPrefs.wrapSearch;
}
const char*
GetFindDialogSearchString(void)
{
	char* src = (char*)gDefaultFindDialogPrefs.lastSearchString;
	char* dst = (char*)gLastSearchCString;
	int length = (unsigned char)(*src++);

	memcpy(dst, src, length);
	dst[length] = 0;

	return gLastSearchCString;
}

void SetFindDialogCaseSensitive(MacBoolean caseSensitive)
{
	gDefaultFindDialogPrefs.caseSensitive = caseSensitive;
}
void SetFindDialogEntireWord(MacBoolean entireWord)
{
	gDefaultFindDialogPrefs.entireWord = entireWord;
}
void SetFindDialogWrapSearch(MacBoolean wrapSearch)
{
	gDefaultFindDialogPrefs.wrapSearch = wrapSearch;
}
void SetFindDialogSearchString(const char* src)
{
	SetFindDialogSearchString(src, strlen(src));
}
void SetFindDialogSearchString(const char* src, ulong length)
{
	if (length > 255)
		length = 255;
	char* dst = (char*)gDefaultFindDialogPrefs.lastSearchString;
	*dst++ = length;
	memcpy(dst, src, length);
}

MacBoolean
DoFindDialog(void) // (char** findString, Boolean* caseSensitive, Boolean* entireWord, Boolean* wrap)
{
	MacBoolean result = false;
	// if preferences exist, get them:
	FindDialogBoxPreferences *prefPtr = &gDefaultFindDialogPrefs;

	ulong prefLength;
	if (gMacSimulator->GetPreference(kFindDialogBoxPreferences, &prefPtr, &prefLength))
	{
		Assert(prefLength == sizeof(FindDialogBoxPreferences));
		memcpy(&gDefaultFindDialogPrefs, prefPtr, sizeof(FindDialogBoxPreferences));
	}
	
	// init dialog box
	Rect itemRect;
	short itemType;
	Handle itemHandle;
	
	DialogPtr dialogPtr = GetNewDialog(rFindDialog, nil, (WindowPtr)-1);
	Assert(dialogPtr != nil);
	if (dialogPtr == nil)
		return false;
	
	// init items inside dialog box
	SetDialogDefaultItem(dialogPtr, kOKButton);
	SetDialogCancelItem(dialogPtr, kCancelButton);
	SetDialogTracksCursor(dialogPtr, true);
	
	GetDialogItem(dialogPtr, kFindStringEditText, &itemType, &itemHandle, &itemRect);
	Assert(itemType == editText);
	if (itemType == editText)
		SetDialogItemText(itemHandle, gDefaultFindDialogPrefs.lastSearchString);
	
	GetDialogItem(dialogPtr, kCaseSensitiveCheckBox, &itemType, &itemHandle, &itemRect);
	Assert(itemType == ctrlItem + chkCtrl);
	if (itemType == ctrlItem + chkCtrl)
		SetCtlValue((ControlHandle)itemHandle, gDefaultFindDialogPrefs.caseSensitive ? 1 : 0);
	
	GetDialogItem(dialogPtr, kEntireWordCheckBox, &itemType, &itemHandle, &itemRect);
	Assert(itemType == ctrlItem + chkCtrl);
	if (itemType == ctrlItem + chkCtrl)
		SetCtlValue((ControlHandle)itemHandle, gDefaultFindDialogPrefs.entireWord ? 1 : 0);
	
	GetDialogItem(dialogPtr, kWrapSearchCheckBox, &itemType, &itemHandle, &itemRect);
	Assert(itemType == ctrlItem + chkCtrl);
	if (itemType == ctrlItem + chkCtrl)
		SetCtlValue((ControlHandle)itemHandle, gDefaultFindDialogPrefs.wrapSearch ? 1 : 0);
	
	ShowWindow(dialogPtr);
	SetPort(dialogPtr);
	
	// do dialog box
	MacBoolean dialogDone = false;
	while (!dialogDone)
	{
		short itemHit;
		
		ModalDialog(nil, &itemHit);
		
		switch(itemHit)
		{
			case kOKButton:
				result = true;
				dialogDone = true;
				break;
			case kCancelButton:
				result = false;
				dialogDone = true;
				break;
			case kCaseSensitiveCheckBox:
			case kEntireWordCheckBox:
			case kWrapSearchCheckBox:
				GetDItem(dialogPtr, itemHit, &itemType, &itemHandle, &itemRect);
				SetCtlValue((ControlHandle)itemHandle, 
							GetCtlValue((ControlHandle)itemHandle) ? 0 : 1);
				break;
			
			case kStaticCaption:
			case kFindStringEditText:
				// who cares
				break;
				
		}
	}
	GetDialogItem(dialogPtr, kFindStringEditText, &itemType, &itemHandle, &itemRect);
	GetDialogItemText(itemHandle, gDefaultFindDialogPrefs.lastSearchString);
	
	GetDialogItem(dialogPtr, kCaseSensitiveCheckBox, &itemType, &itemHandle, &itemRect);
	gDefaultFindDialogPrefs.caseSensitive = (GetCtlValue((ControlHandle)itemHandle) != 0);
	
	GetDialogItem(dialogPtr, kEntireWordCheckBox, &itemType, &itemHandle, &itemRect); 
	gDefaultFindDialogPrefs.entireWord = (GetCtlValue((ControlHandle)itemHandle) != 0);
	
	GetDialogItem(dialogPtr, kWrapSearchCheckBox, &itemType, &itemHandle, &itemRect);
	gDefaultFindDialogPrefs.wrapSearch = (GetCtlValue((ControlHandle)itemHandle) != 0);
	
	// close dialog box
	DisposDialog(dialogPtr);
	
	// save preferences if not cancelled
	if (result)
		gMacSimulator->SetPreference(kFindDialogBoxPreferences, &gDefaultFindDialogPrefs, sizeof(FindDialogBoxPreferences));
	return result;
}
