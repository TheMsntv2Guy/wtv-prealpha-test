// ===========================================================================
//	MacintoshDialogs.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include <Controls.h>

#ifndef __MACINTOSH_H__
#include "Macintosh.h"
#endif
#ifndef __MACINTOSHDIALOGS_H__
#include "MacintoshDialogs.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif



// ===========================================================================
//	static declarations
// ===========================================================================

enum
{
	iOKButton = 1,
	iCancelButton = 2
};

enum
{
	kButtonActive = 0,
	kButtonInactive = 255
};

static void FillFromItem(DialogPtr dialog, short itemNumber, char *result);
static void FillIntoItem(DialogPtr dialog, short itemNumber, char *result);
static void PutLongIntoItem(DialogPtr dialog, short itemNumber, long number);
static long GetLongFromItem(DialogPtr dialog, short itemNumber);
static void PutNamePortIntoItem(DialogPtr dialog, short itemNumber, const char* name, ushort port);
static Boolean GetNamePortFromItem(DialogPtr dialog, short itemNumber, char* name, ushort* port);


// ===========================================================================
//	implementation of static functions
// ===========================================================================

static void
FillFromItem(DialogPtr dialog, short itemNumber, char *result)
{
	short		itemType;
	Handle		itemHdl;
	Rect		box;
	
	GetDItem(dialog, itemNumber, &itemType, &itemHdl, &box);
	GetIText(itemHdl, (StringPtr)result);
	(void)p2cstr((StringPtr)result);
}

static void
FillIntoItem(DialogPtr dialog, short itemNumber, char *result)
{
	short		itemType;
	Handle		itemHdl;
	Rect		box;
	
	GetDItem(dialog, itemNumber, &itemType, &itemHdl, &box);
	setitext(itemHdl, result);
}

static void
PutLongIntoItem(DialogPtr dialog, short itemNumber, long number)
{
	short itemType;
	Handle itemHandle;
	Rect itemBox;
	char printBuffer[11 + 1]; /* 11 for %ld, 1 for NULL */

	GetDItem(dialog, itemNumber, &itemType, &itemHandle, &itemBox);
	snprintf(printBuffer, sizeof(printBuffer), "%ld", number);
	setitext(itemHandle, printBuffer);
}

static long
GetLongFromItem(DialogPtr dialog, short itemNumber)
{
	short itemType;
	Handle itemHandle;
	Rect itemBox;
	Str255 itemText;
	long itemContents = 0;

	GetDItem(dialog, itemNumber, &itemType, &itemHandle, &itemBox);
	GetIText(itemHandle, itemText);
	p2cstr(itemText);
	sscanf((char*)itemText, "%ld", &itemContents);
	return itemContents;
}

static void
PutNamePortIntoItem(DialogPtr dialog, short itemNumber, const char* name, ushort port)
{
	short itemType;
	Handle itemHandle;
	Rect itemBox;
	char namePlusPort[256];
	
	GetDItem(dialog, itemNumber, &itemType, &itemHandle, &itemBox);
	snprintf(namePlusPort, sizeof(namePlusPort), "%s:%hu", name, port);
	setitext(itemHandle, namePlusPort);
}

static Boolean
GetNamePortFromItem(DialogPtr dialog, short itemNumber, char* name, ushort* port)
{
	short itemType;
	Handle itemHandle;
	Rect itemBox;

	GetDItem(dialog, itemNumber, &itemType, &itemHandle, &itemBox);
	GetIText(itemHandle, (StringPtr)name);
	p2cstr((StringPtr)name);
	char* findColon = strchr(name, ':');
	
	if (findColon == nil)
		return false;
	
	*findColon++ = 0;
	if (sscanf(findColon, "%hu", port) != 1)
		return false;
	
	return true;
}

// ===========================================================================
//	font dialog box functions
// ===========================================================================

enum
{
	//iOKButton = 1,
	//iCancelButton = 2,
	iFontDefaultButton = 3,
	iFontSize1 = 12,
	iFontSize2 = 13,
	iFontSize3 = 14,
	iFontSize4 = 15,
	iFontSize5 = 16,
	iFontSize6 = 17,
	iFontSize7 = 18,
	iFontSize8 = 19,
	iFontDisplay = 20
};

Boolean SetFontSizeRecord(FontSizeRecord* result, ulong UNUSED(font), const FontSizeRecord* defaultValues)
{
	Assert(result != nil);
	DialogPtr	dialog = GetNewDialog(rFontSizeDialog, nil, (WindowPtr)-1);
	short itemHit;
	Boolean accepted = false;
	Boolean canHitDefault = true;
	
	Assert(dialog != nil);
	
	// еее allocate offscreen buffer the size of the iFontDisplay item
	
	SetDialogDefaultItem(dialog, iOKButton);
	SetDialogCancelItem(dialog, iCancelButton);
	
	PutLongIntoItem(dialog, iFontSize1, result->size[0]);
	PutLongIntoItem(dialog, iFontSize2, result->size[1]);
	PutLongIntoItem(dialog, iFontSize3, result->size[2]);
	PutLongIntoItem(dialog, iFontSize4, result->size[3]);
	PutLongIntoItem(dialog, iFontSize5, result->size[4]);
	PutLongIntoItem(dialog, iFontSize6, result->size[5]);
	PutLongIntoItem(dialog, iFontSize7, result->size[6]);
	PutLongIntoItem(dialog, iFontSize8, result->size[7]);
	
	do
	{
		// еее draw strings into the iFontDisplay offscreen buffer, copy
		// еее them to the screen
		Handle itemHandle;
		short itemType;
		Rect itemBox;
		GetDialogItem(dialog, iFontDefaultButton, &itemType, &itemHandle, &itemBox);
		if ((itemHandle != nil) && (itemType == ctrlItem + btnCtrl))	// yeah, this is it
		{
			// if we're ALREADY at default values, make button inactive
			if ((GetLongFromItem(dialog, iFontSize1) == defaultValues->size[0])
				&& (GetLongFromItem(dialog, iFontSize2) == defaultValues->size[1])
				&& (GetLongFromItem(dialog, iFontSize3) == defaultValues->size[2])
				&& (GetLongFromItem(dialog, iFontSize4) == defaultValues->size[3])
				&& (GetLongFromItem(dialog, iFontSize5) == defaultValues->size[4])
				&& (GetLongFromItem(dialog, iFontSize6) == defaultValues->size[5])
				&& (GetLongFromItem(dialog, iFontSize7) == defaultValues->size[6])
				&& (GetLongFromItem(dialog, iFontSize8) == defaultValues->size[7]))
			{	HiliteControl((ControlHandle)itemHandle, kButtonInactive);
			}
			else
			{	HiliteControl((ControlHandle)itemHandle, kButtonActive);
			}
		}

		ModalDialog(nil, &itemHit);

		if (itemHit == iFontDefaultButton)
		{	PutLongIntoItem(dialog, iFontSize1, defaultValues->size[0]);
			PutLongIntoItem(dialog, iFontSize2, defaultValues->size[1]);
			PutLongIntoItem(dialog, iFontSize3, defaultValues->size[2]);
			PutLongIntoItem(dialog, iFontSize4, defaultValues->size[3]);
			PutLongIntoItem(dialog, iFontSize5, defaultValues->size[4]);
			PutLongIntoItem(dialog, iFontSize6, defaultValues->size[5]);
			PutLongIntoItem(dialog, iFontSize7, defaultValues->size[6]);
			PutLongIntoItem(dialog, iFontSize8, defaultValues->size[7]);
		}
			
		
	}
	while ((itemHit != iOKButton) && (itemHit != iCancelButton));

	if (itemHit==iOKButton)
	{
		result->size[0] = GetLongFromItem(dialog, iFontSize1);
		result->size[1] = GetLongFromItem(dialog, iFontSize2);
		result->size[2] = GetLongFromItem(dialog, iFontSize3);
		result->size[3] = GetLongFromItem(dialog, iFontSize4);
		result->size[4] = GetLongFromItem(dialog, iFontSize5);
		result->size[5] = GetLongFromItem(dialog, iFontSize6);
		result->size[6] = GetLongFromItem(dialog, iFontSize7);
		result->size[7] = GetLongFromItem(dialog, iFontSize8);
		accepted = true;
	}
	
	// еее dispose of offscreen buffer

	DisposeDialog(dialog);

	return accepted;
}

// ===========================================================================
//	Connect-to dialog box functions
// ===========================================================================

const char kPrefsDevelopmentServerName[] = "Development Server Name";
const char kPrefsDevelopmentUserID[] = "Development Server UserID";
	
enum
{
	//iOKButton					= 1,
	//iCancelButton				= 2,
	
	iSignupRadioButton			= 3,
	iLoginRadioButton			= 4,
	iProxyRadioButton			= 5,
	iNoneRadioButton			= 6,
	
	iSignupEditText				= 7,
	iLoginEditText				= 8,
	iProxyEditText				= 9,
	
	iSignupPopupMenu			= 10,
	iLoginPopupMenu				= 11,
	iProxyDefaultButton			= 12,
	
	iConnectTitleStaticText		= 13,
	iForceRegistrationBox		= 14,
	iSetDevelopmentServerButton	= 15
};

enum {
	kProductionServer = 0,
	kTestServer = 1,
	kDevelopmentServer = 2,
	kNumServers = 3
};

const char* kDefaultSignupNames[] = {
	kDefaultProductionServerName,		// production
	kDefaultTestServerName,				// test
	kDefaultDevelopmentServerName		// development
};

short kDefaultSignupPorts[] = {
	kDefaultPreregistrationServerPort,	// production
	kDefaultPreregistrationServerPort	// test
};

const char* kDefaultLoginNames[] = {
	kDefaultProductionServerName,		// production
	kDefaultTestServerName				// test
};

short kDefaultLoginPorts[] = {
	kDefaultLoginServerPort,			// production
	kDefaultLoginServerPort				// test
};

//	kDefaultProxyServerName,			// production
//	kDefaultProxyServerPort,			// production

// ===========================================================================

Boolean
SetServerConfiguration(void)
{
	// names of edit text stuff
	char signupName[256];
	snprintf(signupName, sizeof(signupName), "%s", gMacSimulator->GetPreregistrationName());
	ushort signupPort = gMacSimulator->GetPreregistrationPort();

	char proxyName[256];
	snprintf(proxyName, sizeof(proxyName), "%s", gMacSimulator->GetProxyName());
	ushort proxyPort = gMacSimulator->GetProxyPort();

	char loginName[256];
	snprintf(loginName, sizeof(loginName), "%s", gMacSimulator->GetLoginName());
	ushort loginPort = gMacSimulator->GetLoginPort();
	
	ServerType serverType = gMacSimulator->GetServerType();

	// development server stuff
	char developmentServerName[256];
	ushort developmentUserID = 1005;

	ushort developmentSignupServerPort;
	ushort developmentLoginServerPort;
	
	{	const char* prefsDevelopmentServerName;
		gMacSimulator->OpenPreferences();
		if (gMacSimulator->GetPreferenceString(kPrefsDevelopmentServerName, &prefsDevelopmentServerName)) {
			strncpy(developmentServerName, prefsDevelopmentServerName, 256);
		} else {
			strncpy(developmentServerName, kDefaultSignupNames[kDevelopmentServer], 256);
		}
		gMacSimulator->ClosePreferences();
	}
	gMacSimulator->GetPreferenceUShort(kPrefsDevelopmentUserID, &developmentUserID);

	//
	Boolean forceRegistration = gMacSimulator->GetForceRegistration();
	
	DialogPtr dialog = GetNewDialog(rConnectToDialog, nil, (WindowPtr)-1);
	Handle itemHandle;
	short itemType;
	Rect itemBox;
	short itemHit;
	Boolean accepted = false;

	Assert(dialog != nil);
	
	SetDialogDefaultItem(dialog, iOKButton);
	SetDialogCancelItem(dialog, iCancelButton);
	
	PutNamePortIntoItem(dialog, iSignupEditText, signupName, signupPort);
	PutNamePortIntoItem(dialog, iLoginEditText,  loginName,  loginPort);
	PutNamePortIntoItem(dialog, iProxyEditText,  proxyName,  proxyPort);
	
	short lastSignupPopupValue = -1;
	short lastLoginPopupValue = -1;
	
	do
	{
		switch (itemHit)
		{
			case iSignupRadioButton:
				serverType = kServerTypePreregistration;
				break;
			
			case iLoginRadioButton:
				serverType = kServerTypeLogin;
				break;
			
			case iProxyRadioButton:
				serverType = kServerTypeProxy;
				break;
			
			case iNoneRadioButton:
				serverType = kServerTypeNone;
				break;
			
			case iSignupPopupMenu:
				GetDialogItem(dialog, iSignupPopupMenu, &itemType, &itemHandle, &itemBox);
				if (lastSignupPopupValue != GetControlValue((ControlHandle)itemHandle)) {
					lastSignupPopupValue = 0;
				}
				//serverType = kServerTypePreregistration;
				break;
				
			case iLoginPopupMenu:
				GetDialogItem(dialog, iLoginPopupMenu, &itemType, &itemHandle, &itemBox);
				if (lastLoginPopupValue != GetControlValue((ControlHandle)itemHandle)) {
					lastLoginPopupValue = 0;
				}
				//serverType = kServerTypeLogin;
				break;
				
			case iProxyDefaultButton:
				PutNamePortIntoItem(dialog, iProxyEditText,  kDefaultProxyServerName,  kDefaultProxyServerPort);
				//serverType = kServerTypeProxy;
				break;

			case iForceRegistrationBox:
				forceRegistration = !forceRegistration;
				break;
				
			case iSetDevelopmentServerButton:
			{
				char developmentUserIDString[256];
				snprintf(developmentUserIDString, sizeof(developmentUserIDString), "%hu", developmentUserID);
							
				paramtext("Enter your development server and your user ID "
						  "(your user id is the third field of 'grep myusername /etc/passwd')",
						  "Server", "User ID #", "");
				if (QueryUser2(rGenericQuery2Dialog, developmentServerName, developmentUserIDString)) {
					sscanf(developmentUserIDString, "%hu", &developmentUserID);
				}
				
			}
				break;
				
			default:
				break;
		}

		developmentSignupServerPort = ((developmentUserID - 989)*100) + 15;
		developmentLoginServerPort = ((developmentUserID - 989)*100) + 1;

		GetNamePortFromItem(dialog, iSignupEditText, signupName, &signupPort);
		GetNamePortFromItem(dialog, iLoginEditText,  loginName,  &loginPort);
		GetNamePortFromItem(dialog, iProxyEditText,  proxyName,  &proxyPort);

		// only one of the radio buttons can be on
		GetDialogItem(dialog, iSignupRadioButton, 		&itemType, &itemHandle, &itemBox);
		SetControlValue((ControlHandle)itemHandle, 		(serverType == kServerTypePreregistration) ? 1 : 0);
		GetDialogItem(dialog, iLoginRadioButton, 		&itemType, &itemHandle, &itemBox);
		SetControlValue((ControlHandle)itemHandle, 		(serverType == kServerTypeLogin) ? 1 : 0);
		GetDialogItem(dialog, iProxyRadioButton, 		&itemType, &itemHandle, &itemBox);
		SetControlValue((ControlHandle)itemHandle, 		(serverType == kServerTypeProxy) ? 1 : 0);
		GetDialogItem(dialog, iNoneRadioButton, 		&itemType, &itemHandle, &itemBox);
		SetControlValue((ControlHandle)itemHandle, 		(serverType == kServerTypeNone) ? 1 : 0);

		// if the preregistration button is on, allow user to FORCE reregistration
		GetDialogItem(dialog, iForceRegistrationBox, 	&itemType, &itemHandle, &itemBox);
		SetControlValue((ControlHandle)itemHandle, 		forceRegistration ? 1 : 0);
		HiliteControl((ControlHandle)itemHandle,		(serverType == kServerTypePreregistration)
														? kButtonActive : kButtonInactive);

		// if text items contain specific known values, set popup menus/default buttons
		
		// Signup menu
		GetDialogItem(dialog, iSignupPopupMenu, &itemType, &itemHandle, &itemBox);
		if (lastSignupPopupValue == 0) {
			lastSignupPopupValue = GetControlValue((ControlHandle)itemHandle);
			if (lastSignupPopupValue < kNumServers) {
				PutNamePortIntoItem(dialog, iSignupEditText,
									kDefaultSignupNames[lastSignupPopupValue-1],
									kDefaultSignupPorts[lastSignupPopupValue-1]);
			} else {
				PutNamePortIntoItem(dialog, iSignupEditText,
									developmentServerName,
									developmentSignupServerPort);
			}
		} else {
			lastSignupPopupValue = 0;
			while (lastSignupPopupValue<kNumServers-1) {
				if ((kDefaultSignupPorts[lastSignupPopupValue] == signupPort)
						&& EqualString(signupName, kDefaultSignupNames[lastSignupPopupValue])) {
					break;
				}
				lastSignupPopupValue++;
			}
			if (lastSignupPopupValue == kNumServers-1) {
				if (!((developmentSignupServerPort == signupPort)
							&& EqualString(signupName, developmentServerName))) {
					lastSignupPopupValue++;
				}
			}
			SetControlValue((ControlHandle)itemHandle, ++lastSignupPopupValue);
		}

		// login menu
		GetDialogItem(dialog, iLoginPopupMenu, &itemType, &itemHandle, &itemBox);
		if (lastLoginPopupValue == 0) {
			lastLoginPopupValue = GetControlValue((ControlHandle)itemHandle);
			if (lastLoginPopupValue < kNumServers) {
				PutNamePortIntoItem(dialog, iLoginEditText,
									kDefaultLoginNames[lastLoginPopupValue-1],
									kDefaultLoginPorts[lastLoginPopupValue-1]);
			} else {
				PutNamePortIntoItem(dialog, iLoginEditText,
									developmentServerName,
									developmentLoginServerPort);
			}
		} else {
			lastLoginPopupValue = 0;
			while (lastLoginPopupValue<kNumServers-1) {
				if ((kDefaultLoginPorts[lastLoginPopupValue] == loginPort)
						&& EqualString(loginName, kDefaultLoginNames[lastLoginPopupValue])) {
					break;
				}
				lastLoginPopupValue++;
			}
			if (lastLoginPopupValue == kNumServers-1) {
				if (!((developmentLoginServerPort == loginPort)
							&& EqualString(loginName, developmentServerName))) {
					lastLoginPopupValue++;
				}
			}
			SetControlValue((ControlHandle)itemHandle, ++lastLoginPopupValue);
		}

		// proxy default button
		{
			Boolean proxyIsDefault = EqualString(proxyName, kDefaultProxyServerName) &&
										(proxyPort == kDefaultProxyServerPort);
			GetDialogItem(dialog, iProxyDefaultButton, &itemType, &itemHandle, &itemBox);
			HiliteControl((ControlHandle)itemHandle, proxyIsDefault ? kButtonInactive : kButtonActive);
		}

		//PutNamePortIntoItem(dialog, iProxyEditText,
		//					kDefaultProxyServerName,
		//					kDefaultProxyServerPort);
		//break;

		Boolean canDoOK = true;

		canDoOK = canDoOK && GetNamePortFromItem(dialog, iSignupEditText, signupName, &signupPort);
		canDoOK = canDoOK && GetNamePortFromItem(dialog, iLoginEditText,  loginName,  &loginPort);
		canDoOK = canDoOK && GetNamePortFromItem(dialog, iProxyEditText,  proxyName,  &proxyPort);

		GetDialogItem(dialog, iOKButton, &itemType, &itemHandle, &itemBox);
		HiliteControl((ControlHandle)itemHandle, canDoOK ? kButtonActive : kButtonInactive);

		ModalDialog(nil, &itemHit);
	}
	while ((itemHit != iOKButton) && (itemHit != iCancelButton));

	if (itemHit == iOKButton)
	{
		gMacSimulator->SetServerType(serverType);
			
		gMacSimulator->SetPreregistrationName(signupName);
		gMacSimulator->SetPreregistrationPort(signupPort);

		gMacSimulator->SetLoginName(loginName);
		gMacSimulator->SetLoginPort(loginPort);

		gMacSimulator->SetProxyName(proxyName);
		gMacSimulator->SetProxyPort(proxyPort);

		gMacSimulator->SetForceRegistration(forceRegistration);

		gMacSimulator->SetPreferenceString(kPrefsDevelopmentServerName, developmentServerName);
		gMacSimulator->SetPreferenceUShort(kPrefsDevelopmentUserID, developmentUserID);

		accepted = true;
	}
	
	DisposeDialog(dialog);

	return accepted;
}

// ===========================================================================
//	Alert, Query user
// ===========================================================================

Boolean
AlertUser(short dialogID, Boolean allowCancel)
{
	DialogPtr	dialog = GetNewDialog(dialogID, nil, (WindowPtr)-1);

	SetDialogDefaultItem(dialog, ok);
	SetDialogCancelItem(dialog, allowCancel ? cancel : ok);

	if (!allowCancel) {
		Handle	itemHandle;
		short	itemType;
		Rect	itemBox;
		GetDialogItem(dialog, cancel, &itemType, &itemHandle, &itemBox);
		HideControl((ControlHandle)itemHandle);
	}

	Boolean	accepted = false;
	while (true) {
		short itemHit;
		ModalDialog(nil, &itemHit);
		if (itemHit == ok) {
			accepted = true;
			break;
		}
		if ((itemHit == cancel) && allowCancel) {
			accepted = false;
			break;
		}
	}
	DisposeDialog(dialog);
	return false;
}

Boolean
QueryUser(short dialogID, char *result,Boolean isDefault)
{
	Assert(result != nil);
	DialogPtr	dialog = GetNewDialog(dialogID, nil, (WindowPtr)-1);
	short		itemHit;
	Boolean		accepted = false;

	Assert(dialog != nil);
	FillIntoItem(dialog, 3, result);
	if ( !isDefault )
		SelIText(dialog, 3, 0, 327677);	
	else
		SelIText(dialog, 3, strlen(result), strlen(result));	
	SetDialogDefaultItem(dialog, ok);
	SetDialogCancelItem(dialog, cancel);
	do
		ModalDialog(nil, &itemHit);
	while ((itemHit != ok) && (itemHit != cancel));
	
	if (itemHit == ok) {
		FillFromItem(dialog, 3, result);
		accepted = true;
	}
		
	DisposeDialog(dialog);
	return accepted;
}

Boolean
QueryUser2(short dialogID, char *result1, char *result2)
{
	Assert(result1 != nil);
	Assert(result2 != nil);
	DialogPtr	dialog = GetNewDialog(dialogID, nil, (WindowPtr)-1);
	short		itemHit;
	Boolean		accepted = false;
	
	Assert(dialog != nil);
	FillIntoItem(dialog, 3, result1);
	FillIntoItem(dialog, 4, result2);
	SetDialogDefaultItem(dialog, ok);
	SetDialogCancelItem(dialog, cancel);
	do
		ModalDialog(nil, &itemHit);
	while (itemHit != ok && itemHit != cancel);
	
	if (itemHit == ok) {
		FillFromItem(dialog, 3, result1);
		FillFromItem(dialog, 4, result2);
		accepted = true;
	}
		
	DisposeDialog(dialog);
	return accepted;
}







