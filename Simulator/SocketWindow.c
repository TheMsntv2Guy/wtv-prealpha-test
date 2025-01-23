// ---------------------------------------------------------------------------
//	SocketWindow.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ---------------------------------------------------------------------------

#include "Headers.h"

#ifdef DEBUG_SOCKETWINDOW

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
#ifndef __SOCKET_H__
#include "Socket.h"
#endif
#ifndef __SOCKETWINDOW_H__
#include "SocketWindow.h"
#endif

SocketWindow* gSocketWindow = nil;

// ---------------------------------------------------------------------------

static RGBColor kReadInColor		= {0xffff, 0x0000, 0x0000};
static RGBColor kReadOutColor		= {0x0000, 0x0000, 0xffff};
static RGBColor kReadInAndOutColor	= {0xffff, 0x0000, 0xffff};
static RGBColor kInvalidColor		= {0xffff, 0xffff, 0xffff};
static RGBColor kBufferSizeColor	= {0x5555, 0x5555, 0x5555};
static RGBColor kBackgroundColor	= {0xdfdf, 0xdfdf, 0xdfdf};
static RGBColor kMarchingBarColor	= {0x0000, 0x0000, 0x0000};

static RGBColor kPhaseBackgroundColor[] =
	{
		{0xdfdf, 0xdfdf, 0xdfdf}, // kIdle
		{0xffff, 0xbfbf, 0xbfbf}, // kResolverBusy
		{0xefef, 0xbfbf, 0xbfbf}, // kResolvingName
		{0xdfdf, 0xbfbf, 0xbfbf}, // kResolvedName
		{0xbfbf, 0xffff, 0xbfbf}, // kListen
		{0xbfbf, 0xefef, 0xbfbf}, // kConnecting
		{0xbfbf, 0xdfdf, 0xbfbf}, // kConnected
		{0xefef, 0xefef, 0xefef}, // kForcedIdle
		{0xbfbf, 0xbfbf, 0xffff}, // kPeerClosed
		{0xbfbf, 0xbfbf, 0xefef}, // kClosing
		{0xbfbf, 0xbfbf, 0xbfbf} // kDead
	};

static const kUpdateTime = 60;		// ticks between readings

static const kInOutGraphHeight = 30;
static const kBufferSizeGraphHeight = 20;
static const kMinorDividerHeight = 2;
static const kMajorDividerHeight = 2;

static const kOverviewWidth = 225;
static const kGraphWidth = SocketInfo::kNumSocketReadings;

static const kSocketHeight = kInOutGraphHeight + kMinorDividerHeight
							+ kBufferSizeGraphHeight; // + kMajorDividerHeight;

static const kSocketWidth = kOverviewWidth + kGraphWidth;

static const kLineSpacing = 11;

static const kNameHOffset = 4;
static const kNameVOffset = 2 + kLineSpacing;

static const kPhaseHOffset = 4;
static const kPhaseVOffset = kNameVOffset + kLineSpacing;

static const kURLHOffset = 4;
static const kURLVOffset = kPhaseVOffset + kLineSpacing;

static const kRateHOffset = 4;
static const kRateVOffset = kURLVOffset + kLineSpacing;

static const kTotalSocketRateHOffset = 4;
static const kTotalSocketRateVOffset = 2 + kLineSpacing;

static const kTotalSocketReadHOffset = 4;
static const kTotalSocketReadVOffset = kTotalSocketRateVOffset + kLineSpacing;

// ---------------------------------------------------------------------------
//	local functions
// ---------------------------------------------------------------------------

static short InOutValueToCoord(short value);
static short BufferSizeValueToCoord(short value);

static short
InOutValueToCoord(short value)
{
	const short kMaxInOutGraphValue = 4096;
	short result = (value*kInOutGraphHeight)/kMaxInOutGraphValue;
	if (result > kInOutGraphHeight)
		result = kInOutGraphHeight;
	return result;
}

static short
BufferSizeValueToCoord(short value)
{
	const short kMaxBufferSizeGraphValue = 4096;
	short result = (value*kBufferSizeGraphHeight)/kMaxBufferSizeGraphValue;
	if (result > kInOutGraphHeight)
		result = kInOutGraphHeight;
	return result;
}

static const char*
GetSocketURL(Socket* socket)
{
	if (socket == nil)
		return nil;

	if (gNetwork == nil)
		return nil;
	
	for (int i=0; i<Network::kProtocolCount; i++) {
		HTTPProtocol* protocol = gNetwork->GetProtocol(i);
		if (protocol == nil)
			continue;

		const Socket* protocolSocket = protocol->GetSocket();
		if (protocolSocket != socket)
			continue;
		
		ProtocolCommand* command = protocol->GetCommand();
		if (command == nil)
			return nil;
		
		const Resource* resource = command->GetResource();
		if (resource == nil)
			return nil;
		
		if (!resource->HasURL())
			return nil;
		
		char* url = resource->CopyURL("GetSocketURL");
		const char* result = UniqueName(url);
		FreeTaggedMemory(url, "GetSocketURL");
		return result;
		
	}
	return nil;
}

// ---------------------------------------------------------------------------

void
SocketWindow::DrawSocketOverview(const Rect* socketRect, int socketIndex, int readingIndex)
{
	if (socketIndex < 0) {
		DrawTotalSocketOverview(socketRect, readingIndex);
		return;
	}
	char buf[128];
	SocketInfo& socketInfo = fSocketInfo[socketIndex];
	SocketReading& socketReading = socketInfo.socketReading[readingIndex];
	
	const char* name = socketReading.hostName;
	long address = socketReading.hostAddress;
	ushort port = socketReading.hostPort;
	Phase phase = socketReading.phase;

	Rect r;
	r.left = socketRect->left;
	r.top = socketRect->top;
	r.right = r.left + kOverviewWidth;
	r.bottom = r.top + kSocketHeight;
	
	RgnHandle clipRgn = NewRgn();
	GetClip(clipRgn);
	ClipRect(&r);	
	EraseRect(&r);
	MoveTo(r.left+kNameHOffset, r.top+kNameVOffset);
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "Name: %.*s (%d.%d.%d.%d:%hu)",
												sizeof(buf)-7-24,
												socketReading.hostName,
												(socketReading.hostAddress>>24) & 0x0ff,
												(socketReading.hostAddress>>16) & 0x0ff,
												(socketReading.hostAddress>>8) & 0x0ff,
												socketReading.hostAddress & 0x0ff,
												socketReading.hostPort));

	MoveTo(r.left+kPhaseHOffset, r.top+kPhaseVOffset);
#ifdef DEBUG_NAMES
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "Phase: %.*s",
								sizeof(buf)-8, GetPhaseString(socketReading.phase)));
#else /* of #ifdef DEBUG_NAMES */
	DrawText(buf, 0, snprintf(buf, sizeof(buf), "Phase: %ld", (long)socketReading.phase));
#endif /* of #else of #ifdef DEBUG_NAMES */

	
	if (socketReading.phase == kConnected) {
		long intPart = 0;
		long decPart = 0;
		if (socketReading.connectTime > 0) {
			intPart = (socketReading.connectData*kOneSecond)/socketReading.connectTime;
			decPart = (((socketReading.connectData*kOneSecond) - (intPart*socketReading.connectTime))*100)
								/ socketReading.connectTime;
		}
		MoveTo(r.left+kRateHOffset, r.top+kRateVOffset);
		DrawText(buf, 0, snprintf(buf, sizeof(buf), "Read: %ld.%02ld bps (%ld bytes/%ld ticks)",
									intPart, decPart, socketReading.connectData, socketReading.connectTime));

		if (socketReading.url != nil) {
			MoveTo(r.left+kURLHOffset, r.top+kURLVOffset);
			DrawText(buf, 0, snprintf(buf, sizeof(buf), "URL: %.*s",
									sizeof(buf)-8, socketReading.url));
		}							
	}
	
	SetClip(clipRgn);
	DisposeRgn(clipRgn);
}

void
SocketWindow::DrawTotalSocketOverview(const Rect* socketRect, int readingIndex)
{
	char buf[128];
	SocketReading& socketReading = fTotalSocketInfo.socketReading[readingIndex];
	
	Rect r;
	r.left = socketRect->left;
	r.top = socketRect->top;
	r.right = r.left + kOverviewWidth;
	r.bottom = r.top + kSocketHeight;
	
	RgnHandle clipRgn = NewRgn();
	GetClip(clipRgn);
	ClipRect(&r);	
	EraseRect(&r);

	if (socketReading.valid) {
		if ((socketReading.connectTime > 0) && ((socketReading.connectData > 0) || (socketReading.readIn > 0))) {
			long intPart = (socketReading.connectData*kOneSecond)/socketReading.connectTime;
			long decPart = (((socketReading.connectData*kOneSecond) - (intPart*socketReading.connectTime))*100)
														/ socketReading.connectTime;
			MoveTo(r.left+kTotalSocketRateHOffset, r.top+kTotalSocketRateVOffset);
			DrawText(buf, 0, snprintf(buf, sizeof(buf), "Read: %ld.%02ld bps (%ld bytes/%ld ticks)",
										intPart, decPart, socketReading.connectData, socketReading.connectTime));
		
			int prevIndex = readingIndex - 1;
			if (prevIndex < 0)
				prevIndex += SocketInfo::kNumSocketReadings;
			SocketReading& prevReading = fTotalSocketInfo.socketReading[prevIndex];
		
			long time = socketReading.connectTime - prevReading.connectTime;
			long data = socketReading.readIn;
			
			intPart = (data*kOneSecond) / time;
			decPart = (((data*kOneSecond) - (intPart*time))*100) / time;
			
			MoveTo(r.left+kTotalSocketReadHOffset, r.top+kTotalSocketReadVOffset);
			DrawText(buf, 0, snprintf(buf, sizeof(buf), "This: %ld.%02ld bps (%ld bytes/%ld ticks)",
										intPart, decPart, data, time));
		}
	}
	SetClip(clipRgn);
	DisposeRgn(clipRgn);
}

// ---------------------------------------------------------------------------

void
SocketWindow::DrawSocketReading(const Rect* socketRect, int socketIndex, int readingIndex)
{
	RGBColor saveColor;
	PenState savePenState;
	GetForeColor(&saveColor);
	GetPenState(&savePenState);

	SocketReading& reading = fSocketInfo[socketIndex].socketReading[readingIndex];
	int phaseIndex = (int)reading.phase;

	short hCoord = socketRect->left + kOverviewWidth + readingIndex;
	short vCoord;
	
	// first do in/out graph

	vCoord = socketRect->top;
	MoveTo(hCoord, vCoord);
	
	if (reading.valid) {
		short readInHeight = InOutValueToCoord(reading.readIn);
		short readOutHeight = InOutValueToCoord(reading.readOut);
	
		short shorterHeight = readInHeight;
		short tallerHeight = readOutHeight;
		RGBColor& tallerColor = kReadOutColor;

		if (reading.readIn > reading.readOut) {
			shorterHeight = readOutHeight;
			tallerHeight = readInHeight;
			tallerColor = kReadInColor;
		}
		
		if (kInOutGraphHeight > tallerHeight) {
			RGBForeColor(&(kPhaseBackgroundColor[phaseIndex]));
			LineTo(hCoord, vCoord + kInOutGraphHeight - tallerHeight);
		}
		
		if (tallerHeight > shorterHeight) {
			RGBForeColor(&tallerColor);
			LineTo(hCoord, vCoord + kInOutGraphHeight - shorterHeight);
		}			
		
		if (shorterHeight > 0) {
			RGBForeColor(&kReadInAndOutColor);
			LineTo(hCoord, vCoord + kInOutGraphHeight);
		}
	} else {
		RGBForeColor(&kInvalidColor);
		LineTo(hCoord, vCoord + kInOutGraphHeight);
	}
	
	RGBForeColor(&kMarchingBarColor);
	MoveTo(hCoord+1, vCoord);
	LineTo(hCoord+1, vCoord + kInOutGraphHeight);
	
	// then do size graph
	
	short sizeHeight = BufferSizeValueToCoord(reading.start);
	
	vCoord = socketRect->top + kInOutGraphHeight + kMinorDividerHeight;

	MoveTo(hCoord, vCoord);

	if (reading.valid) {
		short height = BufferSizeValueToCoord(reading.start);
		
		if (kBufferSizeGraphHeight > height) {
			RGBForeColor(&(kPhaseBackgroundColor[phaseIndex]));
			LineTo(hCoord, vCoord + kBufferSizeGraphHeight - height);
		}
		if (height > 0) {
			RGBForeColor(&kBufferSizeColor);
			LineTo(hCoord, vCoord + kBufferSizeGraphHeight);
		}
	} else {
		RGBForeColor(&kInvalidColor);
		LineTo(hCoord, vCoord + kBufferSizeGraphHeight);
	}

	RGBForeColor(&kMarchingBarColor);
	MoveTo(hCoord+1, vCoord);
	LineTo(hCoord+1, vCoord + kBufferSizeGraphHeight);

	SetPenState(&savePenState);
	RGBForeColor(&saveColor);
}

void
SocketWindow::DrawSocket(int socketIndex)
{
	Rect socketRect;
	GetSocketRect(&socketRect, socketIndex);
	DrawSocketOverview(&socketRect, socketIndex, fCurrentReading);
	for (int readingIndex=0; readingIndex<SocketInfo::kNumSocketReadings; readingIndex++) {
		DrawSocketReading(&socketRect, socketIndex, readingIndex);
	}
}

void
SocketWindow::GetSocketRect(Rect* socketRect, int socketIndex)
{
	socketRect->top = w->portRect.top + ((socketIndex+1)*(kSocketHeight+kMajorDividerHeight));
	socketRect->left = w->portRect.left;

	//GlobalToLocal((struct MacPoint*)socketRect);

	socketRect->bottom = socketRect->top + kSocketHeight;
	socketRect->right = socketRect->left + kSocketWidth;
}

// ---------------------------------------------------------------------------

SocketWindow::SocketWindow()
{
	if (gSocketWindow != nil)
	{	delete gSocketWindow;
	}

	SetFormat(0, 0, false, false);	// Header, trailer, vscroll, hscroll
	SetDefaultFontID(geneva);
	SetDefaultFontSize(9);
	SetDefaultFontStyle(normal);
	ResizeWindow(kSocketWidth, kSocketHeight*(MAX_TINY_SOCKETS+1));
	SetTitle("TinySockets");
	
	gSocketWindow = this;
}

SocketWindow::~SocketWindow()
{
	gSocketWindow = nil;
}

void
SocketWindow::Click(struct MacPoint* where, ushort UNUSED(modifiers))
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);

	short leftCoord = w->portRect.left + kOverviewWidth;
	short rightCoord = leftCoord + kGraphWidth;

	if (StillDown() && (where->h >= leftCoord) && (where->h < rightCoord)) {
		// let user choose which sample point to display

		PenState oldPS, newPS;
		GetPenState(&oldPS);
	
		PenMode(patXor);
		PenPat(&(qd.gray));
		GetPenState(&newPS);
	
			Rect headerRect;
			GetHeaderRect(&headerRect);
			
			struct MacPoint readingPoint = *where;
			MoveTo(readingPoint.h, w->portRect.top);
			LineTo(readingPoint.h, w->portRect.bottom);

			while (StillDown())
			{
				struct MacPoint newPoint;
				GetMouse(&newPoint);
				if (newPoint.h < leftCoord)
				{	newPoint.h = leftCoord;
				}
				if (newPoint.h >= rightCoord)
				{	newPoint.h = rightCoord - 1;
				}
				
				if (newPoint.h != readingPoint.h)
				{	MoveTo(readingPoint.h, w->portRect.top);
					LineTo(readingPoint.h, w->portRect.bottom);
					MoveTo(newPoint.h, w->portRect.top);
					LineTo(newPoint.h, w->portRect.bottom);
					readingPoint = newPoint;
					int displayReading = newPoint.h - leftCoord;
					SetPenState(&oldPS);
					for (int socketIndex = -1; socketIndex < MAX_TINY_SOCKETS; socketIndex++) {
						Rect socketRect;
						GetSocketRect(&socketRect, socketIndex);
						DrawSocketOverview(&socketRect, socketIndex, displayReading);
					}
					SetPenState(&newPS);
				}
				readingPoint = newPoint;
			}
			MoveTo(readingPoint.h, w->portRect.top);
			LineTo(readingPoint.h, w->portRect.bottom);
		
		SetPenState(&oldPS);
	}
	SetPort(savePort);
}

void
SocketWindow::Close()
{
	HideWindow();
}

void
SocketWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);
	MenuHandle menu = GetMenu(mNetwork);
	EnableItem(menu, iSocketWindow);

	SetMenuItemText(menu, iSocketWindow,
					GetVisible() ? "\pHide SocketWindow" : "\pShow SocketWindow");
}

Boolean
SocketWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);
	Boolean handled = false;

	if ((theMenu == mNetwork) && (theItem == iSocketWindow))
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
SocketWindow::DrawBody(Rect* UNUSED(r), short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	for (int i=-1; i<MAX_TINY_SOCKETS; i++) {
		DrawSocket(i);
	}
}

void
SocketWindow::Idle()
{
	gSocketWindow->CheckForAdvance();
	gSocketWindow->CheckForOverview();
}

// ---------------------------------------------------------------------------

void
SocketWindow::CheckForAdvance(void)
{
	if (Now() >= fLastAdvanceTime + kUpdateTime)
		DoAdvance();
}

void
SocketWindow::DoAdvance(void)
{
	ulong now = Now();
	// needs update

	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);

	int oldReadingIndex = fCurrentReading;
	int newReadingIndex = (oldReadingIndex+1) % SocketInfo::kNumSocketReadings;
	int invalidReadingIndex = -1;
	const char* invalidReadingName;
		
	//if (fLastModificationTime > fLastAdvanceTime + (2*kUpdateTime))
	if (now > fLastAdvanceTime + (2*kUpdateTime)) {
		invalidReadingIndex = newReadingIndex;
		newReadingIndex = (invalidReadingIndex+1) % SocketInfo::kNumSocketReadings;
		char buf[64];
		snprintf(buf, sizeof(buf), "Delay of %d ticks", now - fLastAdvanceTime);
		invalidReadingName = UniqueName(buf);
	}

	if (fLastModificationTime < fLastAdvanceTime) {
		fLastModificationTime = fLastAdvanceTime;
	}

	if (fTotalSocketInfo.socketReading[oldReadingIndex].readIn == 0) {
		fTotalSocketInfo.socketReading[oldReadingIndex].connectData = 0;
		fTotalSocketInfo.socketReading[oldReadingIndex].connectTime = 0;
	}

	TinySocket* tinySocket;
	
	for (int socketIndex=-1; socketIndex<MAX_TINY_SOCKETS; socketIndex++) {
	
		if (GetVisible()) {
			Rect socketRect;
			GetSocketRect(&socketRect, socketIndex);
			DrawSocketOverview(&socketRect, socketIndex, fCurrentReading);
			DrawSocketReading(&socketRect, socketIndex, oldReadingIndex);
		}
		
		SocketReading& oldReading = fSocketInfo[socketIndex].socketReading[oldReadingIndex];
		SocketReading& newReading = fSocketInfo[socketIndex].socketReading[newReadingIndex];
		tinySocket = gTinySocketArray[socketIndex].tinysocket;
		
		oldReading.connectData += oldReading.readIn;
		newReading = oldReading;

		if ((oldReading.phase < kConnecting) || (oldReading.phase >= kClosing)) {
			newReading.start = 0;
		} else {
			newReading.start += (oldReading.readIn - oldReading.readOut)
								+ (oldReading.writeIn - oldReading.writeOut);
		}

		newReading.readIn	= 0;
		newReading.readOut	= 0;
		newReading.writeIn	= 0;
		newReading.writeOut	= 0;
		newReading.valid	= true;
		newReading.url		= (tinySocket == nil) ? nil : GetSocketURL(tinySocket);
		if (oldReading.connectTime >= 0) {
			newReading.connectTime += now - fLastAdvanceTime;
		}
		
		if (invalidReadingIndex >= 0) {
			SocketReading& invalidReading = fSocketInfo[socketIndex].socketReading[invalidReadingIndex];
			invalidReading.hostName = invalidReadingName;
			invalidReading.valid = false;
			if (GetVisible()) {
				Rect socketRect;
				GetSocketRect(&socketRect, socketIndex);
				DrawSocketOverview(&socketRect, socketIndex, fCurrentReading);
				DrawSocketReading(&socketRect, socketIndex, invalidReadingIndex);
			}
		}
	}

	fLastAdvanceTime = now;
	fCurrentReading = newReadingIndex;

	SetPort(savePort);
}


void
SocketWindow::CheckForOverview()
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	TinySocket* tinySocket;
	for (int socketIndex=0; socketIndex<MAX_TINY_SOCKETS; socketIndex++) {
		SocketInfo& socketInfo = fSocketInfo[socketIndex];
		SocketReading& socketReading = socketInfo.socketReading[fCurrentReading];
		Boolean needsRedraw = false;
		tinySocket = gTinySocketArray[socketIndex].tinysocket;
		
		if (tinySocket == nil) {
			socketReading.hostName = "<nil>";
			socketReading.hostAddress = 0;
			socketReading.hostPort = 0;
			socketReading.phase = kDead;
		} else {
			if (!EqualString(tinySocket->fHostName, socketReading.hostName)) {
				socketReading.hostName = UniqueName(tinySocket->fHostName);
				needsRedraw = true;
			}
			if (socketReading.hostAddress != tinySocket->fHostAddress) {
				socketReading.hostAddress = tinySocket->fHostAddress;
				needsRedraw = true;
			}
			if (socketReading.hostPort != tinySocket->fHostPort) {
				socketReading.hostPort = tinySocket->fHostPort;
				needsRedraw = true;
			}
			if (socketReading.phase != tinySocket->fPhase) {
				socketReading.phase = tinySocket->fPhase;
				needsRedraw = true;
			}
			if ((tinySocket->fPhase >= kConnecting) || (tinySocket->fPhase < kClosing)) {
				const char* tinySocketURL = GetSocketURL(tinySocket);
				if (!EqualString(tinySocketURL, socketReading.url)) {
					socketReading.url = tinySocketURL;
					needsRedraw = true;
				}
			}
		}
		if (needsRedraw && GetVisible()) {
			Rect r;
			GetSocketRect(&r, socketIndex);
			DrawSocketOverview(&r, socketIndex, fCurrentReading);
		}
	}
	
	SetPort(savePort);
}

void
SocketWindow::ReadIn(const TinySocket* self, long count)
{
	if (gSocketWindow == nil)
		return;
	
	gSocketWindow->CheckForAdvance();
	
	gSocketWindow->fTotalSocketInfo.socketReading[gSocketWindow->fCurrentReading].readIn += count;
	for (int i=0; i<MAX_TINY_SOCKETS; i++) {
		if (self == gTinySocketArray[i].tinysocket) {
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].readIn += count;
			//gSocketWindow->fLastModificationTime = Now();
			return;
		}
	}
}

void
SocketWindow::ReadOut(const TinySocket* self, long count)
{
	if (gSocketWindow == nil)
		return;
	
	gSocketWindow->CheckForAdvance();
	
	gSocketWindow->fTotalSocketInfo.socketReading[gSocketWindow->fCurrentReading].readOut += count;

	for (int i=0; i<MAX_TINY_SOCKETS; i++) {
		if (self == gTinySocketArray[i].tinysocket) {
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].readOut += count;
			//gSocketWindow->fLastModificationTime = Now();
			return;
		}
	}
}

void
SocketWindow::WriteIn(const TinySocket* self, long count)
{
	if (gSocketWindow == nil)
		return;
	
	gSocketWindow->CheckForAdvance();
	
	gSocketWindow->fTotalSocketInfo.socketReading[gSocketWindow->fCurrentReading].writeIn += count;

	for (int i=0; i<MAX_TINY_SOCKETS; i++) {
		if (self == gTinySocketArray[i].tinysocket) {
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].writeIn += count;
			//gSocketWindow->fLastModificationTime = Now();
			return;
		}
	}
}

void
SocketWindow::WriteOut(const TinySocket* self, long count)
{
	if (gSocketWindow == nil)
		return;
	
	gSocketWindow->CheckForAdvance();
	
	gSocketWindow->fTotalSocketInfo.socketReading[gSocketWindow->fCurrentReading].writeOut += count;

	for (int i=0; i<MAX_TINY_SOCKETS; i++) {
		if (self == gTinySocketArray[i].tinysocket) {
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].writeOut += count;
			//gSocketWindow->fLastModificationTime = Now();
			return;
		}
	}
}

void
SocketWindow::Connecting(const TinySocket* self)
{
	if (gSocketWindow == nil)
		return;
	
	gSocketWindow->DoAdvance();

	for (int i=0; i<MAX_TINY_SOCKETS; i++) {
		if (self == gTinySocketArray[i].tinysocket) {
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].connectTime = 0;
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].connectData = 0;
			return;
		}
	}
}

void
SocketWindow::Closing(const TinySocket* self)
{
	if (gSocketWindow == nil)
		return;
	
	gSocketWindow->DoAdvance();

	for (int i=0; i<MAX_TINY_SOCKETS; i++) {
		if (self == gTinySocketArray[i].tinysocket) {
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].connectTime = -1;
			gSocketWindow->fSocketInfo[i].socketReading[gSocketWindow->fCurrentReading].connectData = 0;
			return;
		}
	}
}

#endif /* DEBUG_SOCKETWINDOW */