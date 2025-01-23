// ===========================================================================
//	AlertWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#include "AlertWindow.h"
#include "Cache.h"
#include "CacheStream.h"
#include "ImageData.h"
#include "MemoryManager.h"
#include "Network.h"
#include "PageViewer.h"
#include "Status.h"
#include "SongData.h"
#include "System.h"
#if defined FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif

// ===========================================================================
//	consts
// ===========================================================================

const short kAlertSideMargin = 20;
const short kAlertTopMargin = 10;
const short kAlertBottomMargin = 190;

// ===========================================================================
//	implementation
// ===========================================================================


Window::Window()
{
	fShouldDrawShadow = true;
}

Window::~Window()
{
}

void 
Window::Draw(const Rectangle* invalid)
{
	DrawShadow();
	ContentView::Draw(invalid);
	gAlertBorder->Draw(&fBounds, invalid);
}

#ifdef FIDO_INTERCEPT
void 
Window::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	DrawShadow(fidoCompatibility);
	ContentView::Draw(fidoCompatibility);
	gAlertBorder->Draw(&fBounds, 0, fidoCompatibility);
}
#endif

void 
Window::DrawShadow()
{
	// We should create a window subclass so this method can be shared.

	Rectangle bounds = fBounds;
	
	if (fShouldDrawShadow)	 { 				// Hack!  Should not know about this!

		// draw background w/ drop shadow
		Rectangle shadowBounds;
		
		// first draw the right shadow
		shadowBounds.top = bounds.top + 6;
		shadowBounds.left = bounds.right;
		shadowBounds.bottom = bounds.bottom;
		shadowBounds.right = bounds.right + 6;
		PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
		
		// then draw the bottom shadow
		shadowBounds.top = bounds.bottom;
		shadowBounds.left = bounds.left + 6;
		shadowBounds.bottom = bounds.bottom + 6;
		shadowBounds.right = bounds.right + 6;
		PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
		
		fShouldDrawShadow = false;
	}
}

#ifdef FIDO_INTERCEPT
void 
Window::DrawShadow(class FidoCompatibilityState& fidoCompatibility) const
{
	// We should create a window subclass so this method can be shared.

	Rectangle bounds = fBounds;
	{
		// draw background w/ drop shadow
		Rectangle shadowBounds;
		
		// first draw the right shadow
		shadowBounds.top = bounds.top + 6;
		shadowBounds.left = bounds.right;
		shadowBounds.bottom = bounds.bottom;
		shadowBounds.right = bounds.right + 6;
		fidoCompatibility.PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
		
		// then draw the bottom shadow
		shadowBounds.top = bounds.bottom;
		shadowBounds.left = bounds.left + 6;
		shadowBounds.bottom = bounds.bottom + 6;
		shadowBounds.right = bounds.right + 6;
		fidoCompatibility.PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 160, nil);
	}
}
#endif

void 
Window::Hide()
{
	if (!fIsVisible)
		return;
		
	// Invalidate shadow.
	Rectangle bounds = fBounds;
	InsetRectangle(bounds, -6, -6);
	InvalidateBehind(&bounds);

	ContentView::Hide();
	gPageViewer->RefreshSelection();
	
	// Delete the document, it will be regenerated next time.	
	delete(fDocument);
	fDocument = nil;
	fIdleState = kViewIdle;
}

void
Window::SetURL(const char* url)
{
	fPageResource.SetURL(url);
}


// ===========================================================================

GenericAlertWindow::GenericAlertWindow()
{
	fIsVisible = false;
	fShouldAutoScroll = false;
	fAction = kNoAction;
	fError = kNoError;
	fShouldPlaySound = true;
	fShouldMakePage = true;
	fBackDisabled = false;
	
	gScreen->GetBounds(&fBounds);
	fBounds.top += kAlertTopMargin;
	fBounds.left += kAlertSideMargin;
	fBounds.bottom -= kAlertBottomMargin;
	fBounds.right -= kAlertSideMargin;
}

GenericAlertWindow::~GenericAlertWindow()
{
	if (fServerMessage)
		FreeTaggedMemory(fServerMessage, "GenericAlertWindow::fServerMessage");
	if (fTargetFragment)
		FreeTaggedMemory(fTargetFragment, "GenericAlertWindow::fTargetFragment");
}

Boolean 
GenericAlertWindow::BackInput()
{
	if (!fBackDisabled) {
		Hide();
		return ContentView::BackInput();
	}

	gErrorSound->Play();
		
	return true;
}

void 
GenericAlertWindow::ChangeIdleState(ViewIdleState newState, ViewIdleState oldState)
{
	ContentView::ChangeIdleState(newState, oldState);

	// Don't show until it is done laying out.
	if (newState == kDocumentLoadingComplete) {
		ContentView::Show();
		gPageViewer->RefreshSelection();
	}
}

Boolean 
GenericAlertWindow::ExecuteInput()
{
	Boolean	value = true;
	
	if (IsError(fDocument == nil))
		return true;

	Displayable* current = fDocument->GetSelectable(fCurrentSelection);
	if (current == nil)
		return true;
		
	// handle controls...		
	if (!current->HasURL())
		value = current->ExecuteInput();
	else {
		char* url = current->NewURL(&fMapCursorPosition, "GenericAlertWindow::ExecuteInput");
		if (url != nil) {	
			gPageViewer->ExecuteURL(url, nil);
			FreeTaggedMemory(url, "GenericAlertWindow::ExecuteInput");
		}
	}
	
	Hide();

	// If a target fragment was specified for this alert, go there now.
	if (fTargetFragment != nil) {
		gPageViewer->PostTargetFragment(fTargetFragment);
		FreeTaggedMemory(fTargetFragment, "GenericAlertWindow::fTargetFragment");
		fTargetFragment = nil;
	}
	
	return value;
}

const char* 
GenericAlertWindow::GetActionURL() const
{
	switch (fAction) {
	
		case kNoAction:		break;
		case kStartup:		break;
		case kPreviousPage:		return "client:GoBack";
		case kTargetURL:	break;
		case kPowerOff:			return "client:PowerOff";
		default:			IsWarning(true); break; /* unrecognized fAction */
	}
	
	return "client:DoNothing";
}

// NOTE: we need to be smarter about these error messages. The message that
// should be displayed might be different depending on what kind of page it
// was trying to access. For example, if there is no connection at all, then
// a general message about not being able to get the page is not appropriate.
//
// --chris

const char*
GenericAlertWindow::GetMessage() const
{
	const char* message = nil;

	// If the server sends a message, display it instead of one of our
	// canned ones.
	if (fServerMessage != nil)
		return fServerMessage;
		
	switch (fError) {
		case kLostConnection:			
			message = "The connection to WebTV has been lost.";
			break;

		case kTimedOut:
		case kCannotOpenResolver:
		case kTooManyConnections:
		case kCannotListen:
		case kResourceNotFound:
		case kFileNotFound:
		case kNoConnection:
		case kUnknownService:
		case kHostNotFound:		
		case kPageNotFound:			
		case kResponseError:
			message = "The page requested isn't available right now.";
			break;
		
		case kCacheFull:
		case kLowMemory:
		case kCannotParse:
		case kElementNotFound:
		case kNoSOIMarkerErr:
		case kNoSOFMarkerErr:
		case kNoSOSMarkerErr:
		case kBadSOFMarkerErr:
		case kBadSOSMarkerErr:
		case kBadMarkerErr:
		case kBadHuffmanErr:
		case kOutOfDataErr:
			message = "A problem was encountered in displaying the current page.";
			break;
			
		default:
			message = "A technical problem just came up. If this problem "
					  "occurs often, contact WebTV Customer Service.";		
			break;
	};
	
	return message;
}

void 
GenericAlertWindow::Hide()
{
	if (!fIsVisible)
		return;
		
	Window::Hide();
}

void			
GenericAlertWindow::MakePage()
{
}

Boolean 
GenericAlertWindow::OptionsInput()
{
	// Disable options while alert viewer is up.
	return true;
}

void
GenericAlertWindow::Reset()
{
	SetError(kNoError);
	SetServerMessage(nil);
}

void
GenericAlertWindow::SetAction(AlertAction value)
{
	fAction = value;
}

void
GenericAlertWindow::SetError(Error error)
{
	fError = error;
	
	// When an error is set, we assume that the page
	// will page generated, instead of using a URL.
	fShouldMakePage = true;
}

void
GenericAlertWindow::SetErrorResource(const Resource* resource)
{
	if (!IsError(resource == nil))
		fErrorResource = *resource;
	else
		fErrorResource.Reset();
}

void
GenericAlertWindow::SetServerMessage(const char* message)
{
	fServerMessage = CopyStringTo(fServerMessage, message, "GenericAlertWindow::fServerMessage");
}

void
GenericAlertWindow::SetTargetFragment(const char* fragment)
{
	fTargetFragment = CopyStringTo(fTargetFragment, fragment, "GenericAlertWindow::fTargetFragment");
}

void
GenericAlertWindow::SetURL(const char* url)
{
	Window::SetURL(url);
	
	// When the URL is set, the alert should use that page
	// instead of generating one.
	fShouldMakePage = false;
	
}

void 
GenericAlertWindow::Show()
{
	gStatusIndicator->Hide();
	gConnectIndicator->Hide();
	gScreen->RedrawNow();
	gScreen->CloseAllPanels();
	
	if (fShouldPlaySound)
		gErrorSound->Play();
		
	fShouldDrawShadow = true;
	
	if (fShouldMakePage)
		MakePage();
		
	char* url = fPageResource.CopyURL("GenericAlertWindow::GetURL");
	ExecuteURL(url, nil);
	FreeTaggedMemory(url, "GenericAlertWindow::GetURL");

	// reset after using
	fError = kNoError;	
	SetServerMessage(nil);
	fErrorResource.Reset();
}

// =============================================================================

SplashWindow::SplashWindow()
{
}

SplashWindow::~SplashWindow()
{
}

Boolean
SplashWindow::DispatchInput(Input* input)
{
	if (fSendResource.GetStatus() == kComplete || input->data == kBackKey)
		Hide();
	
	return true;
}

void 
SplashWindow::Idle()
{
	Window::Idle();
	
	if (!IsVisible())
		return;
	
	Error	status = fSendResource.GetStatus();
	
	if (TrueError(status)) {
		gAlertWindow->SetError(status);
		gAlertWindow->SetErrorResource(&fSendResource);
		gAlertWindow->SetAction(kTargetURL);
		gAlertWindow->Show();
		Hide();
	}
	else if (status == kComplete && Now() >= fHideTime)
		Hide();
}

void 
SplashWindow::SetSendResource(const Resource* resource)
{
	if (!IsError(resource == nil))
		fSendResource = *resource;
}

void 
SplashWindow::Show()
{
	char* windowURL = fPageResource.CopyURL("SplashWindow::ExecuteURL");

	fShouldDrawShadow = true;

	ExecuteURL(windowURL, nil);
	FreeTaggedMemory(windowURL, "SplashWindow::ExecuteURL");
	
	ContentView::Show();
	gSaveSound->Play();
	
	fHideTime = Now() + kOneSecond*2;
}

// =============================================================================

AlertWindow::AlertWindow()
{
	IsError(gAlertWindow != nil);
	gAlertWindow = this;

	fPageResource.SetURL(kAlertWindowURL);
	fShouldPlaySound = true;
}

AlertWindow::~AlertWindow()
{
	IsError(gAlertWindow != this);
	gAlertWindow = this;
}

void 
AlertWindow::MakePage()
{
	CacheStream* stream = fPageResource.NewStreamForWriting();

	if (IsError(stream == nil))
		return;

	stream->SetDataType(kDataTypeHTML);
	stream->SetStatus(kNoError);
	stream->SetPriority(kImmediate);

	stream->WriteString("<body bgcolor=#444444 text=#ffdd33 link=#ffdd33>\n");
	stream->WriteString("<br>");
	stream->WriteString("<img src=\"");
	stream->WriteString(kAlertImageURL);
	stream->WriteString("\" vspace=10 hspace=10 align=left>");
	stream->WriteString("<p>");
	stream->WriteString("<table width=480><tr>");
	stream->WriteString("<td height=115 valign=top><br><font size=+1><shadow>\n");
	
	stream->WriteString(GetMessage());

	// If the resource is valid, show the url.
	if (fErrorResource.HasURL()) {
		char*	url = fErrorResource.CopyURL("AlertWindow::MakePage");
		if (!EqualStringN(url, "wtv-", 4)) {
			stream->WriteString("<br><br>");
			stream->WriteString(url);
		}
		FreeTaggedMemory(url, "AlertWindow::MakePage");
	}

	stream->WriteString("</shadow></font></tr></table>");
	stream->WriteString("<br clear=left>");
	stream->WriteString("<hr>");
	stream->WriteString("<table><tr>");
	stream->WriteString("<td width=350>");

#ifdef DEBUG
	stream->WriteNumeric(fError);
#ifdef DEBUG_NAMES
	stream->WriteString(": ");
	stream->WriteString(GetErrorString(fError));
#endif
#endif

	stream->WriteString("<TD width=20>");
	stream->WriteString("<TD><A");
	
	{	// Only has action if target url set.	
		const char* url = GetActionURL();
		if (url != nil) {
			stream->WriteString(" HREF=\"");
			stream->WriteString(url);
			stream->WriteString("\"");
		}
	}
	
	stream->WriteString(">");
	stream->WriteString("<IMG vspace=5 ");
	stream->WriteString("SRC=");
	stream->WriteString(kContinueImageURL);
	stream->WriteString(">");
	stream->WriteString("</A>");
	stream->WriteString("</TR></TABLE>");
	stream->WriteString("</BODY>\n");

	stream->SetStatus(kComplete);
	delete(stream);
}

// =============================================================================

ConnectWindow::ConnectWindow()
{
	IsError(gConnectWindow != nil);
	gConnectWindow = this;

	fPageResource.SetURL(kConnectWindowURL);
	fShouldPlaySound = false;
}

ConnectWindow::~ConnectWindow()
{
	IsError(gConnectWindow != this);
	gConnectWindow = this;
}

Boolean 
ConnectWindow::ExecuteInput()
{
	if (!GenericAlertWindow::ExecuteInput())
		return false;
	
	gNetwork->Reactivate(true);
	return true;
}

void 
ConnectWindow::MakePage()
{
	CacheStream* stream = fPageResource.NewStreamForWriting();

	if (IsError(stream == nil))
		return;

	stream->SetDataType(kDataTypeHTML);
	stream->SetStatus(kNoError);
	stream->SetPriority(kImmediate);

	stream->WriteString("<BODY bgcolor=#444444 text=#ffdd33 link=#ffdd33>\n");
	stream->WriteString("<BR>");
	stream->WriteString("<IMG SRC=\"");
	stream->WriteString(kAlertImageURL);
	stream->WriteString("\" vspace=10 hspace=10 align=left>");
	stream->WriteString("<P>");
	stream->WriteString("<TABLE width=480><TR>");
	stream->WriteString("<TD height=115 valign=top><FONT SIZE=+1>\n");
	stream->WriteString("<P>");
	stream->WriteString(GetMessage());
	stream->WriteString("</FONT></TR></TABLE>");
	stream->WriteString("<BR clear=left>");
	stream->WriteString("<HR>");
	stream->WriteString("<TABLE><TR>");
	stream->WriteString("<TD width=350>");

	stream->WriteString("<TD width=20>");
	stream->WriteString("<TD><A");
	
	{	// Only has action if target url set.
		const char* url = GetActionURL();
		if (url != nil) {
			stream->WriteString(" HREF=\"");
			stream->WriteString(url);
			stream->WriteString("\"");
		}
	}
	
	stream->WriteString(">");
	stream->WriteString("<IMG vspace=5 ");
	stream->WriteString("SRC=");
	stream->WriteString(kContinueImageURL);
	stream->WriteString(">");
	stream->WriteString("</A>");
	stream->WriteString("</TR></TABLE>");
	stream->WriteString("</BODY>\n");

	stream->SetStatus(kComplete);
	delete(stream);
}

// =============================================================================

PowerOffAlert::PowerOffAlert()
{
	SetAction(kPowerOff);
	fShouldPlaySound = false;
	fPageResource.SetURL(kConnectWindowURL);
}

PowerOffAlert::~PowerOffAlert()
{
}

void
PowerOffAlert::MakePage()
{
	CacheStream* stream = fPageResource.NewStreamForWriting();

	if (IsError(stream == nil))
		return;

	stream->SetDataType(kDataTypeHTML);
	stream->SetStatus(kNoError);
	stream->SetPriority(kImmediate);

	stream->WriteString("<BODY bgcolor=#444444 text=#ffdd33 link=#ffdd33>\n");
	stream->WriteString("<BR>");
	stream->WriteString("<IMG SRC=\"");
	stream->WriteString(kAlertImageURL);
	stream->WriteString("\" vspace=10 hspace=10 align=left>");
	stream->WriteString("<P>");
	stream->WriteString("<TABLE width=480><TR>");
	stream->WriteString("<TD height=115 valign=top><FONT SIZE=+1>\n");
	stream->WriteString("<P>");
	stream->WriteString("Press the power off button to turn off the internet terminal.");
	stream->WriteString("</FONT></TR></TABLE>");
	stream->WriteString("<BR clear=left>");
	stream->WriteString("<HR>");
	stream->WriteString("<TABLE><TR>");
	stream->WriteString("<TD width=270>");

	stream->WriteString("<TD width=20>");
	stream->WriteString("<TD><A href=\"client:donothing\">");
	stream->WriteString("<IMG vspace=5 ");
	stream->WriteString("SRC=");
	stream->WriteString(kContinueImageURL);
	stream->WriteString(">");
	stream->WriteString("</A>");

	stream->WriteString("<TD width=20>");
	stream->WriteString("<TD><A");

	{	// Only has action if target url set.
		const char* url = GetActionURL();
		if (url != nil) {
			stream->WriteString(" HREF=\"");
			stream->WriteString(url);
			stream->WriteString("\"");
		}
	}
	
	stream->WriteString(">");
	stream->WriteString("<IMG vspace=5 ");
	stream->WriteString("SRC=");
	stream->WriteString(kPowerOffImageURL);
	stream->WriteString(">");
	stream->WriteString("</A>");
	stream->WriteString("</TR></TABLE>");
	stream->WriteString("</BODY>\n");

	stream->SetStatus(kComplete);
	delete(stream);
}

// =============================================================================

