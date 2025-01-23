// ===========================================================================
//	ViewSource.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================
#include "Headers.h"

#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif
#ifndef __TEWINDOW_H__
#include "TEWindow.h"
#endif
#ifndef __VIEWSOURCE_H__
#include "ViewSource.h"
#endif




// ===========================================================================

void ViewSource(void)
{
	const Resource* resource;
	
	Assert(gPageViewer != nil);
	Assert(gRAMCache != nil);
	
	if ((resource = gPageViewer->GetResource()) == nil) {
		SysBeep(20);
		return;
	}
	
	if (!resource->HasURL()) {
		SysBeep(20);
		return;
	}
	
	DataStream* stream = resource->NewStream();
	
	if (stream == nil) {
		SysBeep(20);
		return;
	}
	
	TEWindow* tw = newMac(TEWindow);	
	tw->ResizeWindow(300, 200);
	tw->SetBodyText(stream->GetData(), stream->GetDataLength());
	tw->SetBodyFont(monaco, 9, normal);

	char* url = resource->CopyURL("ViewSource");
	tw->SetTitle(url);
	FreeTaggedMemory(url, "ViewSource");

	tw->ShowWindow();
	delete(stream);
}
