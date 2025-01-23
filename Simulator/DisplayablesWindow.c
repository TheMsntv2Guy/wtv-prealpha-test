//
// DisplayablesWindow.c
//

#ifdef DEBUG_DISPLAYABLEWINDOW

#include "Headers.h"
#include "MacintoshUtilities.h"
#include "DisplayablesWindow.h"
#include "Document.h"
#include "PageViewer.h"



DisplayablesWindow::DisplayablesWindow()
{
	const kHeaderHeight = 19;
	const kLineScroll = 12;
	
	SetFormat(kHeaderHeight, 0, true, true);	// Header, trailer, vscroll, hscroll
	mVLineScroll = kLineScroll;
}

DisplayablesWindow::~DisplayablesWindow()
{
}

void DisplayablesWindow::DrawBody(Rect *r, short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	Document*		doc;
	Displayable*	root;
	Rectangle		screenR;

	if (gPageViewer == nil)
		return;
	if ((doc = gPageViewer->GetDocument()) == nil)
		return;
	if ((root = doc->GetRootDisplayable()) == nil)
		return;
		
	EraseRect(r);
	SetGray(0);
	BodyCoordinates(r);
	TextFont(monaco);
	TextSize(9);
	MoveTo(4,10);
	gPageViewer->GetBounds(&screenR);
	gPageViewer->ScreenToContent(&screenR);
	root->DebugPrintInfo(doc, &screenR, r, false);
	
	MacPoint pt;
	GetPen(&pt);
	SetBodySize(700, pt.v+12);
	SetOrigin(0,0);
	fLastResource = *doc->GetResource();
}

void DisplayablesWindow::DrawHeader(Rect *r)
{
	EraseRect(r);
	SetGray(0);
	TextFont(monaco);
	TextSize(9);
	TextFace(bold);
	MoveTo(r->left + 4, r->top + 12);
	DrawString("\pClassName");
	MoveTo(r->left + 130, r->top + 12);
	TextFace(0);
	DrawString("\p top,lft,hgt,wid");
}


void DisplayablesWindow::Idle()
{
	Document*	doc;
	Rect		r;

	if (gPageViewer == nil || (doc = gPageViewer->GetDocument()) == nil)
		return;
	if (fLastResource == *doc->GetResource())
		return;
	
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	GetBodyRect(&r);
	InvalRect(&r);
	
	SetPort(savePort);
}

#endif /* DEBUG_DISPLAYABLEWINDOW */