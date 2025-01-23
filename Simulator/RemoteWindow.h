// ===========================================================================
//	RemoteWindow.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __REMOTEWINDOW_H__
#define __REMOTEWINDOW_H__

void InitRemoteWindows();
void NewRemoteWindow(Boolean fromInit);
void MouseDownInRemoteWindow(struct GrafPort* window, struct MacPoint pt, ulong when);
//void UpdateRemote(ulong input);
void CloseRemoteWindow(struct GrafPort* window);

#define IsRemoteWindow(w) (GetWindowPic((WindowPtr)w))

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include RemoteWindow.h multiple times"
	#endif
#endif /* __REMOTEWINDOW_H__ */
