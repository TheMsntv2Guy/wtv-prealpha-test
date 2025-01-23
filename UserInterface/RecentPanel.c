// ===========================================================================
//	RecentPanel.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __RECENTPANEL_H__
#include "RecentPanel.h"
#endif

#include "Cache.h"
#include "CacheEntry.h"
#include "CacheStream.h"
#include "MemoryManager.h"
#include "OptionsPanel.h"
#include "PageViewer.h"
#include "System.h"


// =============================================================================

static const short kMaxTitleWidth = 134;
static const short kRecentLimit = 9;

// =============================================================================

RecentPanel::RecentPanel()
{
	IsError(gRecentPanel != nil);
	gRecentPanel = this;

	fPageResource.SetURL(kRecentPanelURL);
}

RecentPanel::~RecentPanel()
{
	IsError(gRecentPanel != this);
	gRecentPanel = nil;

	fPageList.DeleteAll();
}

void 
RecentPanel::AddPage(const Resource* resource, const char* title, const Resource* thumbnail)
{
	short count = fPageList.GetCount();

	IsError(count > kRecentLimit);
	
	// Avoid putting the recent page in the list.
	if (*resource == fResource)
		return;
		
	// Avoid repeats in recent page.
	RecentURL*	recentURL;
	long		pageIndex = FindPage(resource);

	// If the page wasn't found, make a new one.	
	if (pageIndex == kInvalidPage) {
		PushDebugChildURL("<Recent Panel>");
		recentURL = new(RecentURL);
		
		// Delete oldest item if list is full. Never remove HOME (item 0).
		if (count == kRecentLimit) {
			long	oldest = 1;
			long	oldestTime = Now();
			
			for (long i = 1; i < count; i++)
				if (RecentURLAt(i)->LastVisited() < oldestTime) {
					oldestTime = RecentURLAt(i)->LastVisited();
					oldest = i;
				}		
			fPageList.DeleteAt(oldest); 
		}
		fPageList.Add(recentURL);
		PopDebugChildURL();
	} else {
		recentURL = RecentURLAt(pageIndex);
	}
				
	recentURL->SetResource(resource);
	recentURL->SetTitle(title);

	if (thumbnail != nil)
		recentURL->SetThumbnail(thumbnail);
}

void 
RecentPanel::ClearPages()
{
	fPageList.DeleteAll();
}

void 
RecentPanel::ExecuteURL(const char* url, const char* formData)
{
	if (EqualString(url, kRecentPanelURL)) {
		ContentView::ExecuteURL(url, formData);
		return;
	}

	Close();
	
	RecentURL*	recent = (RecentURL*)fPageList.At(fCurrentSelection.fSelectableIndex);
	if (recent != nil)
		gPageViewer->ShowResource(recent->GetResource());
}

long 
RecentPanel::FindPage(const Resource* resource) const
{
	short count = fPageList.GetCount();

	for (long i = 0; i < count; i++) {
		RecentURL* recentURL = (RecentURL*)fPageList.At(i);
		
		if (*recentURL->GetResource() == *resource)
			return i;
	}
	
	return kInvalidPage;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long 
RecentPanel::GetCount() const
{
	return fPageList.GetCount();
}
#endif

void 
RecentPanel::Open()
{
	WriteHTML();
	ExecuteURL(kRecentPanelURL, nil);
}

void 
RecentPanel::WriteHTML()
{
#ifdef DEBUG
	static ulong		gRecentPageWriteCount = 0;
#endif
	CacheStream* stream = fPageResource.NewStreamForWriting();
	if (IsError(stream == nil))
		return;
	
	stream->SetDataType(kDataTypeHTML);
	stream->SetStatus(kNoError);
	stream->SetPriority(kImmediate);

#ifdef DEBUG
	gRecentPageWriteCount = 0;	// start anew
#endif
	stream->WriteString("<TITLE>Recent Places</TITLE>");
	stream->WriteString("<BODY bgcolor=#444444 text=#ffdd33 link=#ffdd33 vlink=#ffdd33>\n");
	stream->WriteString("<center><table><tr>\n");
	stream->WriteString("<td valign=top align=center>");
	stream->WriteString("<SHADOW><B><FONT SIZE=+2>Recent <img src=\"file://ROM/Images/rearview_icon.gif\" align=absmiddle> Places</FONT></B></SHADOW>\n\n");
	stream->WriteString("</tr></table>\n");
	stream->WriteString("<TABLE width=480 cellspacing=0 cellpadding=3 border=0>\n");
	
	for (int i = 0; i < fPageList.GetCount(); i++) {
		RecentURL* url = RecentURLAt(i);
		char* urlString = url->CopyURL("RecentPanel::WriteHTML");
		
		if (i % 3 == 0) {
			stream->WriteString("<TR><td width=6><img src=\"file://ROM/Images/spacer_alpha.gif\" width=6 height=2>");
		}
				
		stream->WriteString("<TD align=center width=\"33%%\" >");
		stream->WriteString("<A HREF=");
		stream->WriteHTMLEscapedString(urlString);
		
		// Make current selected.
		if (*url->GetResource() == *gPageViewer->GetResource())
			stream->WriteString(" SELECTED");
			
		stream->WriteString(">");
		
		Error	status = url->GetThumbnail()->GetStatus();
		if (status == kComplete || status == kTimedOut) {
			char*	thumbnailURL = url->GetThumbnail()->CopyURL("RecentPanel::WriteHTML");
			stream->WriteString("<IMG hspace=15 vspace=4 border=1 src=\"");
			stream->WriteHTMLEscapedString(thumbnailURL);
			stream->WriteString("\">");
			FreeTaggedMemory(thumbnailURL, "RecentPanel::WriteHTML");
		}
		else
			stream->WriteString("<IMG border=1 hspace=15 width=70 height=52>");
		
		stream->WriteString("<BR>");
		stream->WriteString("<FONT size=-1>");

		// Truncate title so that table cells don't become too long...
		const char* title = url->GetTitle();

		PackedStyle style = gDefaultStyle;
		style.fontSize--;				
			
		char* shortTitle = NewTruncatedStringWithEllipsis(title,gPageViewer->GetDocument()->GetFont(style),gPageViewer->GetDocument()->GetCharacterEncoding(),
													 	  kMaxTitleWidth, "RecentPanel");

		if (shortTitle != nil) {													 	  
			stream->WriteString(shortTitle);
			FreeTaggedMemory(shortTitle, "RecentPanel");
		}
				
		stream->WriteString("</FONT>");
		stream->WriteString("</A>");
		
		if (i % 3 == 2 || (i == fPageList.GetCount()-1)) {
			stream->WriteString("<td width=6><img src=\"file://ROM/Images/spacer_alpha.gif\" width=6 height=2></TR>");
		}
		
		FreeTaggedMemory(urlString, "RecentPanel::WriteHTML");
	}

	stream->WriteString("</TABLE></center></BODY>\n");
	stream->SetStatus(kComplete);
	delete(stream);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
const char* 
RecentPanel::TitleAt(long index) const
{
	RecentURL* url = (RecentURL*)fPageList.At(index);
	return url->GetTitle();
}
#endif

RecentURL* 
RecentPanel::RecentURLAt(long index) const
{
	return (RecentURL*)fPageList.At(index);
}

// =============================================================================

RecentURL::RecentURL()
{
	fLastVisited = Now();
}

RecentURL::~RecentURL()
{
	if (fTitle != nil)
		FreeTaggedMemory(fTitle, "RecentURL::fTitle");

	fThumbnail.SetPriority((Priority)0);
}

const char* 
RecentURL::GetTitle() const
{
	return fTitle;
}

const Resource* 
RecentURL::GetThumbnail() const
{
	return &fThumbnail;
}

char* 
RecentURL::CopyURL(const char* tagName) const
{
	return fResource.CopyURL(tagName);
}

void 
RecentURL::SetTitle(const char* value)
{
	PushDebugChildURL("<Recent Panel>");
	fTitle = CopyStringTo(fTitle, value, "RecentURL::fTitle");
	PopDebugChildURL();
}

void 
RecentURL::SetThumbnail(const Resource* thumbnail)
{
	fThumbnail = *thumbnail;
	fThumbnail.SetPriority(kPersistent);
}

void 
RecentURL::SetResource(const Resource* resource)
{
	fResource = *resource;
}

void 
RecentURL::Touch()
{
	fLastVisited = Now();
}

// =============================================================================

