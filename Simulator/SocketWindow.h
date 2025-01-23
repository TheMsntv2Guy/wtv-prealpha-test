// ---------------------------------------------------------------------------
//	SocketWindow.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ---------------------------------------------------------------------------

#ifndef __SOCKETWINDOW_H__
#define __SOCKETWINDOW_H__

// ---------------------------------------------------------------------------

#ifdef DEBUG_SOCKETWINDOW

#ifndef __SOCKET_H__
#include "Socket.h"			/* for Phase */
#endif
#ifndef __STDWINDOW_H__
#include "StdWindow.h"		/* for StdWindow */
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"		/* for HasDebugModifiedTime */
#endif

struct SocketReading {
	const char*	hostName;
	ulong		hostAddress;
	ushort		hostPort;
	const char* url;
	ushort		start;
	ushort		readIn;
	ushort		readOut;
	ushort		writeIn;
	ushort		writeOut;
	ulong		connectTime;
	ulong		connectData;
	Boolean		valid;
	Phase		phase;
};

class SocketInfo {
public:
	enum { kNumSocketReadings = 300 };	// how much history to keep
	SocketReading	socketReading[kNumSocketReadings];
};

class SocketWindow : public StdWindow
{
public:
						SocketWindow(void);
	virtual				~SocketWindow(void);

	virtual void		DoAdjustMenus(ushort modifiers);
	virtual Boolean		DoMenuChoice(long menuChoice, ushort modifiers);
	virtual void		Click(struct MacPoint* where, ushort modifiers);
	virtual void		Close();

	virtual void		DrawBody(Rect* r, short hScroll, short vScroll, Boolean scrolling);
	virtual void		Idle();

	// intercepting sockets...
	static void			ReadIn(const TinySocket* self, long count);
	static void			ReadOut(const TinySocket* self, long count);
	static void			WriteIn(const TinySocket* self, long count);
	static void			WriteOut(const TinySocket* self, long count);
	static void			Connecting(const TinySocket* self);
	static void			Closing(const TinySocket* self);

protected:
	void				CheckForAdvance(void);
	void				CheckForOverview(void);
	void				DoAdvance(void);
	
	void				DrawSocketOverview(const Rect* r, int socketIndex, int readingIndex);
	void				DrawTotalSocketOverview(const Rect* r, int readingIndex);
	void				DrawSocketReading(const Rect* r, int socketIndex, int readingIndex);
	void				DrawSocket(int socketIndex);
	void				GetSocketRect(Rect* socketRect, int socketIndex);

protected:
	ulong				fLastAdvanceTime;
	ulong				fLastModificationTime;
	int					fCurrentReading;
	SocketInfo			fTotalSocketInfo;
	SocketInfo			fSocketInfo[MAX_TINY_SOCKETS];
};

extern SocketWindow* gSocketWindow;

#endif /* DEBUG_SOCKETWINDOW */

// ---------------------------------------------------------------------------

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include SocketWindow.h multiple times"
	#endif
#endif /* __SOCKETWINDOW_H__ */