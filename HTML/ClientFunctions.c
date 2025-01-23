// ===========================================================================
//	ClientFunctions.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CLIENTFUNCTIONS_H__
#include "ClientFunctions.h"
#endif

#include "AlertWindow.h"
#include "Cache.h"
#include "CacheStream.h"
#include "InfoPanel.h"
#include "Keyboard.h"
#include "LoginPanel.h"
#include "MemoryManager.h"
#include "Network.h"
#include "OptionsPanel.h"
#include "PageViewer.h"
#include "Screen.h"
#include "SetUIElement.h"
#include "SongData.h"
#include "System.h"
#include "TextField.h"
#include "Utilities.h"
#include "FlashStorage.h"
#include "BoxUtils.h"
#ifndef __TELLYIO_H__
#include "TellyIO.h"
#endif
#ifndef __WTVPROTOCOL_H__
#include "WTVProtocol.h"
#endif

#ifdef TEST_CLIENTTEST
	#ifndef __CLIENTTEST_H__
	#include "ClientTest.h"
	#endif
#endif
#ifdef DEBUG_TOURIST
	#ifndef __TOURIST_H__
	#include "Tourist.h"
	#endif
#endif

#include "HWAudio.h"
#include "HWDisplay.h"
#include "Exceptions.h"
#include "BoxHWEquates.h"
#include "CrashLogC.h"

// ===========================================================================
// Function Prototypes

static void CloseKeyboard(const StringDictionary*);
static void ConfirmConnectSetup(const StringDictionary*);
static void ConfirmPhoneSetup(const StringDictionary*);
static void DoNothing(const StringDictionary*);
static void Find(const StringDictionary*);
static void GoBack(const StringDictionary*);
static void GoHome(const StringDictionary*);
static void GoToConnectSetup(const StringDictionary*);
static void GoToFavoritePage(const StringDictionary*);
static void GoToPhoneSetup(const StringDictionary*);
static void RedialPhone(const StringDictionary*);
static void Unplugged(const StringDictionary*);
static void UnpluggedAndMoved(const StringDictionary*);
static void HangupPhone(const StringDictionary*);
static void InsertChar(const StringDictionary*);
static void LogIntoPage(const StringDictionary*);
static void OpenFindPanel(const StringDictionary*);
static void OpenGoPanel(const StringDictionary*);
static void OpenLoginPanel(const StringDictionary*);
static void OpenInfoPanel(const StringDictionary*);
static void OpenSavePanel(const StringDictionary*);
static void OpenSendPanel(const StringDictionary*);
static void OpenAutoPanel(const StringDictionary*);
static void PowerOff(const StringDictionary*);
static void ReloadPage(const StringDictionary*);
static void SaveToFavorite(const StringDictionary*);
static void SendPage(const StringDictionary*);
static void SetDefaultColors(const StringDictionary*);
static void SetFont(const StringDictionary*);
static void ShowLastTypedAddress(const StringDictionary*);
static void TextCommand(const StringDictionary*);

#ifdef DEBUG_TOURIST
static void DoTourist(const StringDictionary*);
#endif
#ifdef DEBUG
static void OpenAlertWindow(const StringDictionary*);
#endif
#ifdef DEBUG_CACHE_VIEWASHTML
static void ViewCacheAsHTML(const StringDictionary*);
#endif
#ifdef DEBUG
static void ViewSource(const StringDictionary*);
#endif

// ===========================================================================
// Client Functions

static void
SetAdvancedOptions(const StringDictionary* args)
{
	const char* value = args->GetValue("value");
	
	if (EqualString(value, "true"))
		gOptionsPanel->SetShowAdvanced(true);
	else
		gOptionsPanel->SetShowAdvanced(false);
}

static void 
CloseKeyboard(const StringDictionary* UNUSED(args))
{
	gKeyboard->Close();
}


static void
SetFontFromString(const char* value)
{
	long		fontSizeSelector = 0;

	if (EqualString(value, "small"))
		fontSizeSelector = -1;
	else if (EqualString(value, "large"))
		fontSizeSelector = 1;
		
	gSystem->SetFontSizes(fontSizeSelector);
	gPageViewer->ReLayout();
}

static void
SetSetupValue(const StringDictionary* args)
{
	const char* backgroundColor = args->GetValue("setup-background-color");
	const char* textColor = args->GetValue("setup-text-color");
	const char* linkColor = args->GetValue("setup-link-color");
	const char* visitedColor = args->GetValue("setup-visited-color");
	const char* keyboardType = args->GetValue("setup-keyboard");
	const char* fontSizes = args->GetValue("setup-font-sizes");
	const char* songsMuted = args->GetValue("setup-songs-muted");
	const char* soundEffectsMuted = args->GetValue("setup-sounds-muted");
	const char* inStereo = args->GetValue("setup-in-stereo");
	const char* advancedOptions = args->GetValue("setup-advanced-options");
	const char* fromServer = args->GetValue("from-server");
	Boolean		somethingChanged = false;
	Boolean		needsRedraw = false;
	long		value;

	Message(("Changing Setup values"));

	// don't do these if no document
	Document* document = gPageViewer->GetDocument();
	if (document == nil) {
		backgroundColor = nil;
		textColor = nil;
		linkColor = nil;
		visitedColor = nil;
	}

	if (backgroundColor != nil && sscanf(backgroundColor, "%lx", &value) == 1) {
		if (!fromServer) {
			document->SetBackgroundColor(value);
			needsRedraw = true;
		}
		gSystem->SetDefaultBackgroundColor(value);
		somethingChanged = true;
	}

	if (textColor != nil && sscanf(textColor, "%lx", &value) == 1) {
		if (!fromServer) {
			document->SetTextColor(value);
			needsRedraw = true;
		}
		gSystem->SetDefaultTextColor(value);
		somethingChanged = true;
	}

	if (linkColor != nil && sscanf(linkColor, "%lx", &value) == 1) {
		if (!fromServer) {
			document->SetLinkColor(value);
			needsRedraw = true;
		}
		gSystem->SetDefaultLinkColor(value);
		somethingChanged = true;
	}

	if (visitedColor != nil && sscanf(visitedColor, "%lx", &value) == 1) {
		if (!fromServer) {
			document->SetVisitedLinkColor(value);
			needsRedraw = true;
		}
		gSystem->SetDefaultVisitedLinkColor(value);
		somethingChanged = true;
	}
	
	if (keyboardType != nil) {
		gKeyboard->InstallKeyboard(EqualString(keyboardType, "alphabetical") ? kAlphabeticKeyboard : kStandardKeyboard);
		somethingChanged = true;
	}
	if (fontSizes != nil) {
		SetFontFromString(fontSizes);
		somethingChanged = true;
	}
	if (songsMuted != nil)	 {
		gSystem->SetSongsMuted((Boolean)atoi(songsMuted));
		somethingChanged = true;
	}
	if (soundEffectsMuted != nil) {
		gSystem->SetSoundEffectsMuted((Boolean)atoi(soundEffectsMuted));
		somethingChanged = true;
	}
	if (inStereo != nil) {
		gSystem->SetInStereo((Boolean)atoi(inStereo));
		somethingChanged = true;
	}
	if (advancedOptions != nil) {
		gOptionsPanel->SetShowAdvanced((Boolean)atoi(advancedOptions));
		somethingChanged = true;
	}
	
	if (!somethingChanged || fromServer != nil)
		return;
		
	// Create url.
	MemoryStream*	memory = new(MemoryStream);
	if (backgroundColor != nil)
		memory->WriteQuery("setup-background-color", backgroundColor);
	if (textColor != nil)
		memory->WriteQuery("setup-text-color", textColor);
	if (linkColor != nil)
		memory->WriteQuery("setup-link-color", linkColor);
	if (visitedColor != nil)
		memory->WriteQuery("setup-visited-color", visitedColor);
	if (keyboardType != nil)
		memory->WriteQuery("setup-keyboard", keyboardType);
	if (fontSizes != nil)
		memory->WriteQuery("setup-font-sizes", fontSizes);
	if (songsMuted != nil)
		memory->WriteQuery("setup-songs-muted", songsMuted);
	if (soundEffectsMuted != nil)
		memory->WriteQuery("setup-sounds-muted", soundEffectsMuted);
	if (inStereo != nil)
		memory->WriteQuery("setup-in-stereo", inStereo);
	if (advancedOptions != nil)
		memory->WriteQuery("setup-advanced-options", advancedOptions);

	// Create resource using memory's data
	Resource*		resource = new(Resource);
	resource->SetURL("wtv-setup:/set", memory->GetData(), memory->GetDataLength());
	delete(memory);
	
	// send it
	resource->SetPriority(kImmediate);
	resource->SetStatus(kNoError);

	delete(resource);
	
	if (needsRedraw) {
		gPageViewer->InvalidateBounds();
		gScreen->RedrawNow();
	}
}

static void
ConfirmConnectSetup(const StringDictionary* args)
{
	const char* kDefaultUserPrefix = "16";
	const char* kSignupTypeID = "15";
	const char* kLoginTypeID = "01";
	const char* name = args->GetValue("name");
	const char* service = args->GetValue("service");
	const char* type = args->GetValue("type");
	const char* forceSignup = args->GetValue("ForceSignup");
	const char* machine = args->GetValue("machine");
	const char* userprefix = args->GetValue("userprefix");
	char port[256];
	
	// Both service and type must be set or it is an error.
	// This shouldn't be a problem because they are popup menus.
	if (IsError(service == nil) || IsError(type == nil))
		return;

	// Setup machine and userid variables based on the
	// which service we are connecting to.
	if (EqualString(service, "production")) {
		machine = kDefaultProductionServerName;
		userprefix = kDefaultUserPrefix;

	} else if (EqualString(service, "testing")) {
		machine = kDefaultTestServerName;
		userprefix = kDefaultUserPrefix;

	} else if (EqualString(service, "personal")) {
		if (machine == nil || userprefix == nil) {
			machine = kDefaultProductionServerName;
			userprefix = kDefaultUserPrefix;
		}
	}

	// Setup the type, name, and port based on the type of
	// service we are connecting to.
	if (EqualString(type, "proxy")) {
		Message(("ConfirmConnectSetup: proxy"));
		gNetwork->InitializeForProxy(kDefaultProxyServerName, kDefaultProxyServerPort);

	} else if (EqualString(type, "headwaiter")) {
		Message(("ConfirmConnectSetup: headwaiter"));
		snprintf(port, sizeof(port), "%s%s", userprefix, kLoginTypeID);
		gNetwork->InitializeForLogin(machine, atoi(port));

	} else if (EqualString(type, "signup")) {
		Message(("ConfirmConnectSetup: signup"));
		snprintf(port, sizeof(port), "%s%s", userprefix, kSignupTypeID);
		gNetwork->InitializeForSignup(machine, atoi(port), EqualString(forceSignup, "true"));

	} else if (EqualString(type, "krueger")) {
		Message(("ConfirmConnectSetup: krueger"));
		gNetwork->InitializeForKrueger();

	} else {
		Message(("ConfirmConnectionSetup: none"));
		gNetwork->InitializeForNone();
	}
	
	if (name != nil)
		WTVPCommand::AddWTVAttribute("wtv-user-name-hack", name);

	gNetwork->ShowStartup();
	gNetwork->Reactivate(false);
}

static void
ConfirmPhoneSetup(const StringDictionary* args)
{
	PHONE_SETTINGS	phone;
	
	phone.usePulseDialing = EqualString(args->GetValue("UsePulseDialing"), "true");
	phone.audibleDialing = EqualString(args->GetValue("AudibleDialing"), "true");
	phone.disableCallWaiting = EqualString(args->GetValue("DisableCallWaiting"), "true");
	phone.dialOutsideLine = EqualString(args->GetValue("DialOutsideLine"), "true");
	phone.changedCity = EqualString(args->GetValue("ChangedCity"), "true");
	phone.waitForTone = EqualString(args->GetValue("WaitForDialTone"), "true");
	memcpy(phone.callWaitingPrefix, args->GetValue("CallWaitingPrefix"), kMAX_DIGITS);
	memcpy(phone.dialOutsidePrefix, args->GetValue("DialOutsidePrefix"), kMAX_DIGITS);
	memcpy(phone.accessNumber, args->GetValue("AccessNumber"), kMAX_DIGITS);

	SetPhoneSettings(&phone);

	gNetwork->ShowStartup();
	gNetwork->Reactivate(false);
}

static void 
DoNothing(const StringDictionary* UNUSED(args))
{
}

static void 
TextCommand(const StringDictionary* args)
{
	const char* command = args->GetValue("name");
	
	if (command == nil)
		return;
		
	if (EqualString(command, "backspace"))
		gKeyboard->TextCommand(kBackspaceCmd);
		
	else if (EqualString(command, "clear"))
		gKeyboard->TextCommand(kClearCmd);
		
	else if (EqualString(command, "enter"))
		gKeyboard->TextCommand(kEnterCmd);
		
	else if (EqualString(command, "shift"))
		gKeyboard->TextCommand(kShiftCmd);

	else if (EqualString(command, "next"))
		gKeyboard->TextCommand(kNextCmd);

	else if (EqualString(command, "prev"))
		gKeyboard->TextCommand(kPreviousCmd);

	else if (EqualString(command, "down"))
		gKeyboard->MoveCursor(kCursorDown);
		
	else if (EqualString(command, "left"))
		gKeyboard->MoveCursor(kCursorLeft);
		
	else if (EqualString(command, "right"))
		gKeyboard->MoveCursor(kCursorRight);
		
	else if (EqualString(command, "up"))
		gKeyboard->MoveCursor(kCursorUp);

	// unrecognized commands are ignored.
}

static void 
Find(const StringDictionary* args)
{
	const char* text = args->GetValue("text");
	Document* document = gPageViewer->GetDocument();
	
	if (document == nil || text == nil || *text == '\0') {
		gErrorSound->Play();
		return;
	}
	
	SetLastFindString(text);
	
	// 1) Find the text in the pageviewer document.
	// 2) Corrolate with display objects to find scroll position
	Displayable*	found = document->FindString(text);
	
	// 3) Scroll to position with text near top of screen (if necessary)
	// 4) Select text (yellow on black for now)
	if (found != nil ) {
		gFindPanel->Close();
		gPageViewer->SetCurrentSelectableWithScroll(found);
		gPageViewer->InvalidateBounds();
	}
	
	// 5) Notification if can't find that text.
	else 
		gErrorSound->Play();
}

static void 
GoBack(const StringDictionary* UNUSED(args))
{
	gPageViewer->BackInput();
}

static void 
GoHome(const StringDictionary* UNUSED(args))
{
	gScreen->HomeInput();
}

static void
GoToAboutPage(const StringDictionary* UNUSED(args))
{
	//gScroll1Sound->Play();
	gOptionsPanel->Close();
	gPageViewer->ExecuteURL(kAboutURL, nil);
}

static void
GoToConnectSetup(const StringDictionary* UNUSED(args))
{
	gPageViewer->ExecuteURL(kConnectSetupURL, nil);
}

static void 
GoToFavoritePage(const StringDictionary* UNUSED(args))
{
	const char* url = gSystem->GetFavoriteURL();
	if (url == nil)
		return;
	gPageViewer->ExecuteURL(url);
}

static void
GoToPhoneSetup(const StringDictionary* UNUSED(args))
{
	gPageViewer->ExecuteURL(kPhoneSetupURL, nil);
}

static void
RedialPhone(const StringDictionary* UNUSED(args))
{
	gNetwork->Reactivate(false);
}

static void
UnpluggedAndMoved(const StringDictionary* UNUSED(args))
{
	ImportantMessage(("UnpluggedAndMoved: Resetting bootURL to preregistratioin (signup)\n"));
	gRAMCache->Delete(kDownloadedTellyScriptURL);
	gNetwork->InitializeForSignup(gSystem->GetServerName(), gSystem->GetServerPort(), gSystem->GetForceRegistration());
	gNetwork->Activate();
}

static void
Unplugged(const StringDictionary* UNUSED(args))
{
	gNetwork->Activate();
}

static void 
HangupPhone(const StringDictionary* UNUSED(args))
{
	Boolean preventHangup = false;
#ifdef DEBUG_MONKEY
	preventHangup = gMonkeyEnable;
#endif
	if (!preventHangup) {
		gNetwork->HangUp();
	}
	gScreen->RedrawNow();
}

static void 
InsertChar(const StringDictionary* args)
{
	const char* c = args->GetValue("value");
	
	if (c == nil)
		return;
		
	gKeyboard->InsertChar(*c);
}

static void
LogIntoPage(const StringDictionary* args)
{
	const char* name = args->GetValue("name");
	const char* password = args->GetValue("password");

	gLoginPanel->Login(name, password);
}

static void 
OpenFindPanel(const StringDictionary* UNUSED(args))
{
	gFindPanel->Open();
}

static void 
OpenGoPanel(const StringDictionary* UNUSED(args))
{
	gGoPanel->Open();
}

static void 
OpenInfoPanel(const StringDictionary* UNUSED(args))
{
	gInfoPanel->Open();
}

static void 
OpenLoginPanel(const StringDictionary* UNUSED(args))
{
	gLoginPanel->Open();
}

static void
OpenSavePanel(const StringDictionary* UNUSED(args))
{
	gSavePanel->Open();
}

static void
OpenSendPanel(const StringDictionary* UNUSED(args))
{
	gSendPanel->Open();
}

static void
OpenAutoPanel(const StringDictionary* UNUSED(args))
{
	gAutoPanel->Open();
}

static void
PowerOff(const StringDictionary* UNUSED(args))
{
	gSystem->PowerOff();
}

static void
ReloadPage(const StringDictionary* UNUSED(args))
{
	gScreen->ReloadPage();
}

static void 
SaveToFavorite(const StringDictionary* UNUSED(args))
{
	const Document* document;
	Resource* resource;
	MemoryStream* memory;
	CacheEntry* entry;
	char* documentURL;
	char* postData = nil;
	long postDataLength = 0;
	
	gSavePanel->Close();

	if ((document = gPageViewer->GetDocument()) == nil) {
		ImportantMessage(("SaveToFavorite: no current document"));
		return;
	}
	
	documentURL = document->CopyURL("SaveToFavorite");
	
	if (EqualString(documentURL, gSystem->GetFavoriteURL())) {
		FreeTaggedMemory(documentURL, "SaveToFavorite");		
		return;
	}
		
	const Resource* thumbnail;
	const char*		dataTypeString = "image/wtv-bitmap";
	
	if ((thumbnail = gPageViewer->GetThumbnail()) != nil && thumbnail->HasURL())
		dataTypeString = thumbnail->GetDataTypeString();
		
	// Create url.
	memory = new(MemoryStream);
	memory->WriteString("wtv-favorite:/add?");
	memory->WriteQuery("favorite-title", document->GetTitle());
	memory->WriteQuery("favorite-thumbnail-type", dataTypeString);
	memory->WriteString("&favorite-url=");
	memory->WriteString(documentURL);

	// Create post data.
	DataStream* thumbnailStream;
	if (thumbnail != nil && thumbnail->HasURL() &&
		(thumbnailStream = thumbnail->NewStream()) != nil) {
			postDataLength = thumbnailStream->GetDataLength();
			postData = (char*)AllocateBuffer(postDataLength);
			CopyMemory(thumbnailStream->GetData(), postData, postDataLength);
			delete(thumbnailStream);
		}

	// Create resource.
	resource = new(Resource);
	resource->SetURL(memory->GetDataAsString(), postData, postDataLength);
	resource->SetPriority(kImmediate);
	resource->SetStatus(kNoError);
	
	gSaveSplash->SetSendResource(resource);
	gSaveSplash->Show();
	gSaveSplash->Idle();	// WARNING - Idle once so first draw is clean.

	// Purge current favorite page.
	if ((entry = gRAMCache->Find(gSystem->GetFavoriteURL())) != nil)
		gRAMCache->Purge(entry);

	// Clean up.
	FreeTaggedMemory(documentURL, "SaveToFavorite");
	delete(memory);
	delete(resource);
	if (postData != nil)
		FreeBuffer(postData, postDataLength);
}

static void 
SendPage(const StringDictionary* args)
{
	Document* document = gPageViewer->GetDocument();
	const char* address;
	const char* subject;
	Resource* resource;
	MemoryStream* memory;
	char* documentURL;
	
	if (IsError(document == nil))
		return;

	if ((address = args->GetValue("address")) == nil) {
		PostulateFinal(false); // notify user
		return;
	}

	if ((subject = args->GetValue("subject")) == nil) {
		PostulateFinal(false); // notify user
		return;
	}
	documentURL = document->CopyURL("SendPage");

	// Create post data.
	memory = new(MemoryStream);
	memory->WriteQuery("message_body", "");		// no message body
	memory->WriteQuery("message_to", address);
	memory->WriteQuery("message_url", documentURL);
	memory->WriteQuery("message_title", document->GetTitle());
	memory->WriteQuery("message_subject", subject);
	memory->WriteQuery("sendoff", "true");
	
	// Create resource.
	resource = new(Resource);
	resource->SetURL("wtv-mail:sendmail", memory->GetData(), memory->GetDataLength());
	
	resource->SetPriority(kImmediate);
	resource->SetStatus(kNoError);
	
	gSendPanel->Close();
	
	gSendSplash->SetSendResource(resource);	
	gSendSplash->Show();
	gSendSplash->Idle();	// WARNING - Idle once so first draw is clean.

	// Clean up.
	FreeTaggedMemory(documentURL, "SendPage");
	delete(memory);
	delete(resource);	
}

static void 
ServiceInfo(const StringDictionary* UNUSED(args))
{
	gPageViewer->ExecuteURL("http://www.artemis.com/tmp/test.html", nil);
}

static void 
SetDefaultColors(const StringDictionary* args)
{
	const char* backgroundColor = args->GetValue("BackgroundColor");
	const char* textColor = args->GetValue("TextColor");
	const char* linkColor = args->GetValue("LinkColor");
	const char* visitedColor = args->GetValue("VisitedColor");
	
	Document* document = gPageViewer->GetDocument();
	long value;
	
	if (document == nil)
		return;
	
	if (backgroundColor && sscanf(backgroundColor, "%lx", &value) == 1) {
		document->SetBackgroundColor(value);
		gSystem->SetDefaultBackgroundColor(value);
	}

	if (textColor && sscanf(textColor, "%lx", &value) == 1) {
		document->SetTextColor(value);
		gSystem->SetDefaultTextColor(value);
	}

	if (linkColor && sscanf(linkColor, "%lx", &value) == 1) {
		document->SetLinkColor(value);
		gSystem->SetDefaultLinkColor(value);
	}

	if (visitedColor && sscanf(visitedColor, "%lx", &value) == 1) {
		document->SetVisitedLinkColor(value);
		gSystem->SetDefaultVisitedLinkColor(value);
	}

	gPageViewer->InvalidateBounds();
	gScreen->RedrawNow();
}

static void 
SetFont(const StringDictionary* args)
{
	SetFontFromString(args->GetValue("Font Size"));
}

static void 
ShowLastTypedAddress(const StringDictionary* UNUSED(args))
{
	Document* document = gGoPanel->GetDocument();
	
	Selection firstSelection = {0,0};
	Selection secondSelection = {0,1};
	
	Displayable* targetTextField;
	Displayable* showLastButton;

	if (IsWarning(document == nil))
		return;
			
	targetTextField = document->GetSelectable(firstSelection);
	showLastButton = document->GetSelectable(secondSelection);
	
	if (IsWarning(targetTextField == nil || showLastButton == nil))
		return;

	if (targetTextField->IsTextField()) {
		TextField* textfield = (TextField*)targetTextField;
		
		if (textfield == nil)
			return;
			
		textfield->ClearText();
		
		// NOTE: remember to put in default value for empty case
		const char* lastAddress = LastExecuteURLFunction();
		
		if (EqualString(lastAddress, ""))
			textfield->Reset();
			
		textfield->AddText(LastExecuteURLFunction());
		textfield->InvalidateBounds(true);
	}
}

static void 
SubmitForm(const StringDictionary* args)
{
	const char* formName = args->GetValue("Name");
	const char* submitName = args->GetValue("SubmitName");
	const char* submitValue = args->GetValue("SubmitValue");
	Document*	document = gPageViewer->GetDocument();
	
	if (document != nil) {
		Form*	form = document->FindForm(formName);
		if (form != nil) {
			form->CreateSubmission(submitName, submitValue);
			form->Submit();
		}
	}
}

#ifdef DEBUG_TOURIST
static void
DoTourist(const StringDictionary* args)
{
	Tourist::Execute(args);
}
#endif /* DEBUG_TOURIST */

#ifdef DEBUG
static void
OpenAlertWindow(const StringDictionary*)
{
	gAlertWindow->Reset();
	gAlertWindow->SetError(kPageNotFound);
	gAlertWindow->Show();
}

static void
OpenConnectWindow(const StringDictionary* UNUSED(args))
{
	gConnectWindow->Reset();
	gConnectWindow->SetError(kLostConnection);
	gConnectWindow->Show();
}
#endif

#ifdef DEBUG_CACHE_VIEWASHTML
static void 
ViewCacheAsHTML(const StringDictionary* UNUSED(args))
{
	gRAMCache->ViewCacheAsHTML();
}
#endif

#ifdef DEBUG
static void
ViewSource(const StringDictionary* UNUSED(args))
{
	gPageViewer->ViewSource();
}
#endif

static void
UpdateFlash(const StringDictionary* args)
{
	const char* ipaddr = args->GetValue("ipaddr");
	const char* hostport = args->GetValue("port");
	const char* path = args->GetValue("path");
	ulong ip = ParseAddress(ipaddr);
	ushort port = strtoul(hostport, 0, 10);

	Message(("Flash download triggered..."));

	Message(("ipaddr = %x",ip));
	Message(("port = %hd",port));
	Message(("path = %s",path));

	/* this is a hack, clean it up, Joe! */
	#ifdef HARDWARE
	KillDisplay();						/* black screen */
	#endif

	NVInit(kNVPhaseSaveFB);

	NVWrite((uchar*)&ip,sizeof(ulong),(ulong)kFlashIPTag);
	NVWrite((uchar*)&port,sizeof(ushort),(ulong)kFlashPortTag);
	NVWrite((uchar *)path,strlen(path)+1,(ulong)kFlashPathTag);

	NVCommit();

	NVSetFlags(NVGetFlags() | kServerReqFlashUpdate);
		
#ifdef HARDWARE
	Reboot(kColdBoot);		/* here we go! */
#endif
}


// ===========================================================================
// Client Function Table
//

typedef void (*ClientFunction)(const StringDictionary*);

typedef struct ClientFunctionEntry {
	const char* name;
	ClientFunction function;
} ClientFunctionEntry;

static const  ClientFunctionEntry gClientFunctionTable[] =  {
	{"UpdateFlash", 			UpdateFlash},
	{"CloseKeyboard", 			CloseKeyboard},
	{"ConfirmConnectSetup",		ConfirmConnectSetup},
	{"ConfirmPhoneSetup",		ConfirmPhoneSetup},
	{"DoNothing", 				DoNothing},
	{"Find", 					Find},
	{"GoBack", 					GoBack},
	{"GoHome", 					GoHome},
	{"GoToAboutPage",			GoToAboutPage},
	{"GoToConnectSetup",		GoToConnectSetup},
	{"GoToFavoritePage",		GoToFavoritePage},
	{"GoToPhoneSetup",			GoToPhoneSetup},
	{"RedialPhone",				RedialPhone},
	{"InsertChar",				InsertChar},
	{"HangupPhone",				HangupPhone},			
	{"LogIntoPage",				LogIntoPage},
	{"OpenAutoPanel",			OpenAutoPanel},
	{"OpenFindPanel", 			OpenFindPanel},
	{"OpenGoPanel", 			OpenGoPanel},
	{"OpenLoginPanel", 			OpenLoginPanel},
	{"OpenInfoPanel", 			OpenInfoPanel},
	{"OpenSavePanel", 			OpenSavePanel},
	{"OpenSendPanel", 			OpenSendPanel},
	{"PowerOff", 				PowerOff},
	{"ReloadPage",				ReloadPage},
	{"SaveToFavorite",			SaveToFavorite},
	{"SendPage",				SendPage},
	{"ServiceInfo",				ServiceInfo},
	{"SetAdvancedOptions",		SetAdvancedOptions},
	{"SetDefaultColors",		SetDefaultColors},
	{"SetFont",					SetFont},
	{"SetSetupValue",			SetSetupValue},
	{"SetUI",					SetUI},
	{"ShowLastTypedAddress",	ShowLastTypedAddress},
	{"SubmitForm",				SubmitForm},
	{"TextCommand", 			TextCommand},
	{"Unplugged",				Unplugged},
	{"UnpluggedAndMoved",		UnpluggedAndMoved},

#ifdef DEBUG
	{"OpenAlertWindow", 		OpenAlertWindow},
	{"OpenConnectWindow", 		OpenConnectWindow},
#endif
#ifdef TEST_CLIENTTEST
	{"Test",					DoClientTest},
#endif
#ifdef DEBUG_TOURIST
	{"Tourist",					DoTourist},
#endif
#ifdef DEBUG_CACHE_VIEWASHTML
	{"ViewCacheAsHTML",			ViewCacheAsHTML},
#endif
#ifdef DEBUG
	{"ViewSource",				ViewSource},
#endif
	{ 0, 						0}
};

// ===========================================================================
// ExecuteClientFunction
//

Boolean 
ExecuteClientFunction(const char* URL, const char* formData)
{
	const char kPrefix[] = "client:";
	const char kPrefixCount = sizeof(kPrefix) - 1;

	if (!EqualStringN(URL, kPrefix, kPrefixCount))
		return false;
	
	Message(("Executing client function %s", URL));
	
	// Create modifiable URL string.
	long urlCharacters = strlen(URL) - kPrefixCount;
	char* url = (char*)AllocateBuffer(urlCharacters + 1);
	strcpy(url, URL + kPrefixCount);
	
	// Find the start of the URL parameters.
	char* urlParameters = FindCharacter(url, "?");
	if (urlParameters != nil)
		*urlParameters++ = 0;
	
	// Find client function.
	ClientFunction function = nil;
	long i;
	for (i = 0; gClientFunctionTable[i].name != 0; i++)
		if (EqualString(gClientFunctionTable[i].name, url)) {
			function = gClientFunctionTable[i].function;
			break;
		}
	
	// Return if no client function.
	if (function == nil) {
		ImportantMessage(("ExecuteClientFunction: ignoring %s", URL));
		FreeBuffer(url, urlCharacters + 1);
		return false;
	}
		
	// Create string args of parameters.
	StringDictionary* args = new(StringDictionary);
#ifdef DEBUG
	extern Boolean	gDebugDictionary;
	Boolean			saveDebug = gDebugDictionary;
	gDebugDictionary = true;
#endif
	if (urlParameters && *urlParameters)
		args->Add(urlParameters);
	if (formData && *formData) {
		Message(("Adding form data %s to client function dictionary", formData));
		args->Add(formData);
#ifdef DEBUG
	gDebugDictionary = saveDebug;
#endif
	}
	
	// Call client function.
	Message(("ExecuteClientFunction: %s", URL));
	(*function)(args);
	delete(args);
	FreeBuffer(url, urlCharacters + 1);
	return true;
}

// ===========================================================================


