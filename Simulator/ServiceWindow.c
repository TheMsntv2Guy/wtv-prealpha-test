// ---------------------------------------------------------------------------
//	ServiceWindow.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ---------------------------------------------------------------------------

#include "Headers.h"

#ifdef DEBUG_SERVICEWINDOW

#ifndef __HTTP_H__
#include "HTTP.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __NETWORK_H__
#include "Network.h"
#endif
#ifndef __PROTOCOL_H__
#include "Protocol.h"
#endif
#ifndef __SERVICE_H__
#include "Service.h"
#endif
#ifndef __SERVICEWINDOW_H__
#include "ServiceWindow.h"
#endif

// ---------------------------------------------------------------------------
//	local constants/variables
// ---------------------------------------------------------------------------
static const kHorizontalOffset = 8;
static const kVerticalOffset = 12;
static const kLineHeight = 12;
static const kNameOffset = 0;
static const kHostOffset = 350;

static const kStateOffset = 100;
static const kURLOffset = 200;

static const kResourceOffset = 375;

ServiceWindow* gServiceWindow = nil;

// ---------------------------------------------------------------------------
//	local functions
// ---------------------------------------------------------------------------
static void DrawCommand(const ProtocolCommand* command, struct MacPoint& pt);
static void DrawProtocol(const Protocol* protocol, struct MacPoint& pt);
static void DrawService(const Service* service, struct MacPoint& pt);
static void DrawResource(const Resource* resource, struct MacPoint& pt);

static void
DrawCommand(const ProtocolCommand* command, struct MacPoint& pt)
{
	if (command != nil) {
		const char* name;
		const Resource* resource;
		const char* state;
		char* url;

#ifdef DEBUG_CLASSNUMBER		
		name = DebugClassNumberToName(command->GetClassNumber());
#else
		name = "<unknown>";
#endif

		MoveTo(pt.h + kNameOffset, pt.v);
		DrawText(name, 0, strlen(name));
	
#ifdef DEBUG_NAMES
		state = command->GetStateAsString();
#else /* of #ifdef DEBUG_NAMES */
		char buf[12];
		snprintf(buf, sizeof(buf), "%ld", (long)(command->GetState()));
		state = &(buf[0]);
#endif /* of #else of #ifdef DEBUG_NAMES */

		MoveTo(pt.h + kStateOffset, pt.v);
		DrawText(state, 0, strlen(state));
	
		if ((resource = command->GetResource()) != nil) {
			if (resource->HasURL()) {
				url = resource->CopyURL("DrawCommand");
				MoveTo(pt.h + kURLOffset, pt.v);
				DrawText(url, 0, strlen(url));
				FreeTaggedMemory(url, "DrawCommand");
			}
		}
	}
	pt.v += kLineHeight;
}

static void
DrawProtocol(const Protocol* protocol, struct MacPoint& pt)
{
	ProtocolCommand* command = nil;
	Service* service = nil;
	const Resource* resource = nil;
	
	if (protocol != nil)
	{
#ifdef DEBUG_CLASSNUMBER
		const char* name = DebugClassNumberToName(protocol->GetClassNumber());
#else
		const char* name = "<unknown>";
#endif

		MoveTo(pt.h + kNameOffset, pt.v);
		DrawText(name, 0, strlen(name));

		const char* state;		
#ifdef DEBUG_NAMES
		state = protocol->GetStateAsString();
#else /* of #ifdef DEBUG_NAMES */
		char buf[12];
		snprintf(buf, sizeof(buf), "%ld", (long)(protocol->GetState()));
		state = &(buf[0]);
#endif /* of #else of #ifdef DEBUG_NAMES */

		MoveTo(pt.h + kStateOffset, pt.v);
		DrawText(state, 0, strlen(state));
		
		command = protocol->GetCommand();
		if (command != nil)
		{
			resource = command->GetResource();
		}
		service = protocol->GetService();
	}
	
	pt.v += kLineHeight;

	DrawService(service, pt);
	DrawCommand(command, pt);
	DrawResource(resource, pt);
}

static void DrawResource(const Resource* resource, struct MacPoint& pt)
{
	if (resource != nil)
	{
		char buffer[1024];
		DataType type;
		char typeChars[4];
		type = resource->GetDataType();
		typeChars[0] = type >> 24;
		if (isalnum(typeChars[0]))
		{	typeChars[1] = type >> 16;
			typeChars[2] = type >> 8;
			typeChars[3] = type;
		}
		else
		{	typeChars[0] = typeChars[1] = typeChars[2] = typeChars[3] = '?';
		}

		MoveTo(pt.h, pt.v);

#ifdef DEBUG_NAMES
		DrawText(buffer, 0, 
				 snprintf(buffer, sizeof(buffer), "%s (Priority=%d)",
						GetErrorString(resource->GetStatus()),
						resource->GetPriority()));
#else
		DrawText(buffer, 0, 
				 snprintf(buffer, sizeof(buffer), "%d (Priority=%d)",
						resource->GetStatus(),
						resource->GetPriority()));
#endif

		MoveTo(pt.h + kResourceOffset, pt.v);
		DrawText(buffer, 0, 
				 snprintf(buffer, sizeof(buffer), "Received=%d of %d ('%c%c%c%c')",
						resource->GetDataLength(),
						resource->GetDataExpected(),
						typeChars[0], typeChars[1], typeChars[2], typeChars[3]));
	}
	pt.v += kLineHeight;
}

static void
DrawService(const Service* service, struct MacPoint& pt)
{
	
	if (service != nil) {
		
		const char* name = service->GetName();
		if (name != nil) {
			MoveTo(pt.h + kNameOffset, pt.v);
			DrawText(name, 0, strlen(name));
		}
		
		ulong hostAddress = service->GetHostAddress();
		const char* hostName = service->GetHostName();
		ushort hostPort = service->GetHostPort();
		char buffer[1024];
		MoveTo(pt.h + kHostOffset, pt.v);
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
									"%.*s:%hu (%x)",
									sizeof(buffer) - 1 - 5 - 1,	/* for ':', %hu, NULL */
									(hostName == nil) ? kEmptyString : hostName,
									hostPort,
									hostAddress));
	}
	pt.v += kLineHeight;
}


// ---------------------------------------------------------------------------
//	ServiceWindow
// ---------------------------------------------------------------------------
ServiceWindow::ServiceWindow()
{
	if (gServiceWindow != nil)
	{	delete gServiceWindow;
	}

	const kServiceWindowHeaderHeight = kLineHeight*2;
	SetFormat(kServiceWindowHeaderHeight, 0, false, false);	// Header, trailer, vscroll, hscroll
	fLastNetState = kNetInactive;
	fLastUpdate = 0;
	SetDefaultFontID(geneva);
	SetDefaultFontSize(9);
	SetDefaultFontStyle(normal);
	SetTitle("Services");
	
	gServiceWindow = this;
}

ServiceWindow::~ServiceWindow()
{
	gServiceWindow = nil;
}

void
ServiceWindow::Close()
{
	HideWindow();
}

void
ServiceWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);
	MenuHandle menu = GetMenu(mNetwork);
	EnableItem(menu, iServiceWindow);

	SetMenuItemText(menu, iServiceWindow,
					GetVisible() ? "\pHide ServiceWindow" : "\pShow ServiceWindow");
}

Boolean
ServiceWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);
	Boolean handled = false;

	if ((theMenu == mNetwork) && (theItem == iServiceWindow))
	{
		if (GetVisible())
			HideWindow();
		else
			ShowWindow();
		handled = true;
	}
	return handled || StdWindow::DoMenuChoice(menuChoice, modifiers);
}

void
ServiceWindow::DrawHeader(Rect* r)
{
	EraseRect(r);
	struct MacPoint pt;
	pt.h = r->left + kHorizontalOffset;
	pt.v = r->top + kVerticalOffset;

	char buffer[256];
	const char title[] = "Network state: ";
	
	snprintf(buffer, sizeof(buffer), "%s", title);

	if (gNetwork == nil) {
		snprintf(&(buffer[sizeof(title) - 1]), sizeof(buffer) - sizeof(title),
				 "%s", "uninitialized");
	} else {
#ifdef DEBUG_NAMES
		snprintf(&(buffer[sizeof(title) - 1]), sizeof(buffer) - sizeof(title),
				 "%s", gNetwork->GetStateAsString());
#else
		snprintf(&(buffer[sizeof(title) - 1]), sizeof(buffer) - sizeof(title),
				 "%d", gNetwork->GetState());
#endif
	}
	MoveTo(pt.h, pt.v);
	DrawText(buffer, 0, strlen(buffer));
}

void
ServiceWindow::DrawBody(Rect *r, short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	EraseRect(r);
	fLastUpdate = Now();

	struct MacPoint pt;
	pt.h = r->left + kHorizontalOffset;
	pt.v = r->top + kVerticalOffset;

	if (gNetwork != nil) {
		for (int i = 0; i < Network::kProtocolCount; i++) {
			HTTPProtocol* protocol = gNetwork->GetProtocol(i);
			DrawProtocol(protocol, pt);
			pt.v += kLineHeight/2;
			MoveTo(r->left, pt.v - kLineHeight);
			LineTo(r->right, pt.v - kLineHeight);
		}
	}
	
	if (gServiceList != nil) {
		pt.v += 2;
		MoveTo(r->left, pt.v - kLineHeight);
		LineTo(r->right, pt.v - kLineHeight);

		const char kNameColumnHeader[] = "Service Name";
		MoveTo(pt.h + kNameOffset, pt.v);
		DrawText(kNameColumnHeader, 0, sizeof(kNameColumnHeader)-1);

		const char kHostColumnHeader[] = "Service Host";
		MoveTo(pt.h + kHostOffset, pt.v);
		DrawText(kHostColumnHeader, 0, sizeof(kHostColumnHeader) - 1);

		pt.v += 3*kLineHeight/2;

		int count = gServiceList->GetCount();

		for (int i = 0; i < count; i++) {
			Service* servicePtr = gServiceList->At(i);
			DrawService(servicePtr, pt);
		}
	}
}

void
ServiceWindow::Idle()
{
	Boolean needsUpdate = false;
	
	if (gNetwork != nil) {
		needsUpdate = gNetwork->GetState() != fLastNetState;
		fLastNetState = gNetwork->GetState();
		
		if (!needsUpdate) {
			int count = Network::kProtocolCount;
			for (int i=0; i < count; i++) {
				const HTTPProtocol* protocol = gNetwork->GetProtocol(i);
				if (protocol == nil)
					continue;
				if (protocol->GetDebugModifiedTime() >= fLastUpdate) {
					needsUpdate = true;
					break;
				}
				
				const ProtocolCommand* command = protocol->GetCommand();
				if (command == nil)
					continue;
				if (command->GetDebugModifiedTime() >= fLastUpdate) {
					needsUpdate = true;
					break;
				}
				
				const Resource* resource = command->GetResource();
				if (resource == nil)
					continue;
				const CacheEntry* cacheEntry = resource->GetCacheEntry();
				if (cacheEntry->GetDebugModifiedTime() >= fLastUpdate) {
					needsUpdate = true;
					break;
				}
			}
		}
	}
	
	if ((!needsUpdate) && (gServiceList != nil)) {
		int count = gServiceList->GetCount();
		for (int i = 0; i<count && !needsUpdate; i++) {
			Service* service = gServiceList->At(i);
			if (service->GetDebugModifiedTime() >= fLastUpdate) {
				needsUpdate = true;
				break;
			}
		}
	}
	
	if (needsUpdate) {
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		InvalRect(&(w->portRect));	// needs to be updated...
		SetPort(savePort);
	}
}

#endif /* DEBUG_SERVICEWINDOW */