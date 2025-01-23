#include "Headers.h"
#include "MacintoshUtilities.h"
#include "FidoStatsWindow.h"
#include "Document.h"
#include "PageViewer.h"
#include "fidoSim.h"
#include "statWindow.h"

FidoStatsWindow::FidoStatsWindow()
{
	SetFormat(0, 0, false, false);	// Header, vscroll, hscroll, trailer, constrain
	stats = new statWindow(0, &fido::stats);
	stats->window = (struct WindowRecord*) w;
}

FidoStatsWindow::~FidoStatsWindow()
{
	delete stats;
}

void FidoStatsWindow::DrawBody(Rect* , short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	stats->draw();
}

void FidoStatsWindow::DrawHeader(Rect *)
{
}


void FidoStatsWindow::Idle()
{
	Rect		r;
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	GetBodyRect(&r);
	InvalRect(&r);
	Draw();
	SetPort(savePort);
}
