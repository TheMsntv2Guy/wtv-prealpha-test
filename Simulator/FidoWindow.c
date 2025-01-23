#include "Headers.h"
#include "MacintoshUtilities.h"
#include "FidoWindow.h"
#include "FidoStatsWindow.h"
#include "Document.h"
#include "PageViewer.h"


FidoWindow::FidoWindow()
{
	SetFormat(0, 0, false, false);	// Header, vscroll, hscroll, trailer, constrain
	state = nil;
	stats = newMac(FidoStatsWindow);
	stats->ResizeWindow(105, 400);
	stats->SetTitle((const char*)"Fido Stats");
	stats->ShowWindow();
}

FidoWindow::~FidoWindow()
{
	delete(state);
	delete(stats);
}

void FidoWindow::DrawBody(Rect* , short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	if (state == nil)
		return;
#if 0
	Document*		doc;
	Displayable*	root;
	if (gPageViewer == nil)
		return;
	if ((doc = gPageViewer->GetDocument()) == nil)
		return;
	if ((root = doc->GetRootDisplayable()) == nil)
		return;
	doc->DrawBackground(*state);
	root->Draw(doc, *state);
#else
	Layer* last = (Layer*)gScreen->fLayerList->Last();
	for (Layer* layer = last; layer != nil; layer = (Layer*)layer->Previous())
		layer->DrawLayer(*state);
#endif
}

void FidoWindow::DrawHeader(Rect *)
{
}


void FidoWindow::Idle()
{
	Document*	doc;
	Rect		r;

	if (gPageViewer == nil || (doc = gPageViewer->GetDocument()) == nil)
		return;		
	PushDebugChildURL("<Fido>");
	if (state == nil)
		gFidoCompatibility = state = new(FidoCompatibilityState);
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	GetBodyRect(&r);
	InvalRect(&r);
	state->display.reset();
	Draw();
	state->display.draw();
	PopDebugChildURL();
//	fidoPrint::print(state->display);
//	fidoPrint::close();
	SetPort(savePort);
}
