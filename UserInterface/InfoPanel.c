// ===========================================================================
//	InfoPanel.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __INFOPANEL_H__
#include "InfoPanel.h"
#endif

#include "Cache.h"
#include "CacheEntry.h"
#include "CacheStream.h"
#include "MemoryManager.h"
#include "PageViewer.h"
#include "Sound.h"
#include "System.h"
#include "URLParser.h"

// =============================================================================

InfoPanel::InfoPanel()
{
	IsError(gInfoPanel != nil);
	gInfoPanel = this;
	fPageResource.SetURL(kInfoPanelURL);
}

InfoPanel::~InfoPanel()
{
	IsError(gInfoPanel != this);
	gInfoPanel = nil;
}

Boolean 
InfoPanel::ExecuteInput()
{
	if (!IsOpen())
		return false;
		
	ContentView::ExecuteInput();
	return true;
}

void 
InfoPanel::ExecuteURL(const char* url, const char* formData)
{
	if (EqualString(url, kInfoPanelURL)) {
		ContentView::ExecuteURL(url, formData);
		return;
	}

	Close();	
	gPageViewer->ExecuteURL(url, formData);
}

void 
InfoPanel::Open()
{
	Panel::Open();
	// Don't show the Info Panel until it is done laying out.
	ExecuteURL(kInfoPanelURL, nil);
}

void 
InfoPanel::WritePage()
{
	Document* document = gPageViewer->GetDocument();

	if (IsError(document == nil))
		return;

	CacheStream* stream = fPageResource.NewStreamForWriting();

	if (IsError(stream == nil))
		return;

	stream->SetDataType(kDataTypeHTML);
	stream->SetStatus(kNoError);
	stream->SetPriority(kImmediate);
	
	stream->WriteString("<TITLE>Information</TITLE>");
	stream->WriteString("<BODY bgcolor=#444444 text=#ffdd33>\n");
	stream->WriteString("<TABLE cellspacing=10><TR><TD>");
	stream->WriteString("<IMG src=\"&thumb\" align=absmiddle width=70 height=52 border=1>");
	
	stream->WriteString("<TD><FONT size=+2><SHADOW><B>Information about &title</B></SHADOW></FONT>");	
	stream->WriteString("</TR></TABLE>");
	stream->WriteString("<HR>");
	stream->WriteString("<TABLE cellspacing=5>");
	
	stream->WriteString("<TR valign=top><TD align=right>");
	stream->WriteString("<FONT color=#ffdd33>");
	stream->WriteString("<SHADOW>Address:</SHADOW>");
	stream->WriteString("</FONT>");
	stream->WriteString("<TD>&url</TR>");

	if (document->GetResource()->GetLastModified() != 0) {	
		stream->WriteString("<TR valign=top><TD align=right>");
		stream->WriteString("<FONT color=#ffdd33>");
		stream->WriteString("<SHADOW>Changed:</SHADOW>");
		stream->WriteString("</FONT>");
		stream->WriteString("<TD>&mod</TR>");
	}
	
	stream->WriteString("</TABLE>");

	const char*	helpURL = document->GetHelpURL();
	const char*	creditsURL = document->GetCreditsURL();
	
	if (helpURL != nil || creditsURL != nil) {
		stream->WriteString("<BR>");
		stream->WriteString("<UL><TABLE cellspacing=10><TR>");
	
		if (helpURL != nil) {
			stream->WriteString("<TD><FORM action=\"");
			stream->WriteString(helpURL);
			stream->WriteString("\">");
			stream->WriteString("<INPUT type=submit value=\"Help\" name=\"\">");
			stream->WriteString("</FORM>");
		}
		
		if (creditsURL != nil) {
			stream->WriteString("<TD><FORM action=\"");
			stream->WriteString(creditsURL);
			stream->WriteString("\">");
			stream->WriteString("<INPUT type=submit value=\"Credits\" name=\"\">");
			stream->WriteString("</FORM>");
		}

		stream->WriteString("</TR></TABLE></UL>");
	}

	stream->SetStatus(kComplete);
	delete(stream);
}

// =============================================================================
