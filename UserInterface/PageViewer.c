// ===========================================================================
//    PageViewer.c
//
//    Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif

#include "CacheStream.h"
#include "ClientFunctions.h"
#include "Control.h"
#include "DateTime.h"
#include "Document.h"
#include "DocumentBuilder.h"
#include "ImageData.h"
#include "InfoPanel.h"
#include "Keyboard.h"
#include "MemoryManager.h"
#include "OptionsPanel.h"
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#include "RecentPanel.h"
#include "Status.h"
#include "SongData.h"
#include "System.h"
#include "SystemLogo.h"
#include "URLParser.h"

#if defined(HARDWARE) && defined(DEBUG)
#include "BoxUtils.h"
#include "BoxAbsoluteGlobals.h"
#endif

// ===========================================================================
//    consts/global variables
// ===========================================================================

const long			kBackLimit = 32;
const long			kVisitLimit = 512;
const long			kMemoryPreflightSize = 64*1024L;

short				gPageViewerScrollDirection = 0;
TransitionEffect	gPageViewerTransitionType = kTransitionNone;
ulong				gPageViewerTransitionDelay = 0;
static ulong		gPageCount = 0;

static char*		gLastFindString = nil;
static char*		gLastExecuteURL = nil;

static char			buffer[256];


const char*
LastModifiedFunction()
{
	buffer[0] = '\0';
	
	if (gPageViewer->GetResource() != nil) {
		ulong lastModified = gPageViewer->GetResource()->GetLastModified();
		DateTimeParser dateTimeParser;
		dateTimeParser.SetDateTime(lastModified);
		snprintf(buffer, sizeof(buffer), "%s %s",
				 dateTimeParser.GetDateString(), dateTimeParser.GetTimeString());
	}
			
	return buffer;
}

const char*
PageURLFunction()
{
	buffer[0] = '\0';
	
	if (gPageViewer->GetDocument() != nil) {	
		char*	url = gPageViewer->GetDocument()->CopyURL("PageURLFunction");
		
		if (url != nil) {
			/* if there is a URL, check that it doesn't use a "secret" wtv protocol */
			URLParser urlParser;
			urlParser.SetURL(url);
			const char* protocol = urlParser.GetProtocolText();
			if ((protocol != nil) && (strncmp("wtv", protocol, 3) == 0))
				strcpy(buffer, "None");
			else
				strncpy(buffer, url, sizeof(buffer));
			FreeTaggedMemory(url, "PageURLFunction");
		}
	}
			
	return buffer;
}
	
const char*
PageTitleFunction()
{
	if (gPageViewer->GetDocument() != nil && gPageViewer->GetDocument()->GetTitle() != nil)
		return gPageViewer->GetDocument()->GetTitle();

	return "no title";
}
	
const char*
PageThumbnailURLFunction()
{
	buffer[0] = '\0';
	
	if (gPageViewer->GetThumbnail() != nil) {	
		char*	url = gPageViewer->GetThumbnail()->CopyURL("PageThumbnailURLFunction");
		
		if (url != nil) {
			strncpy(buffer, url, sizeof(buffer));
			FreeTaggedMemory(url, "PageThumbnailURLFunction");
		}
	}
			
	return buffer;
}

const char*
LastFindStringFunction()
{
	if (gLastFindString == nil)
		gLastFindString = CopyStringTo(gLastFindString, "", "LastFindStringFunction");
		
	return gLastFindString;
}

void
SetLastFindString(const char* string)
{
	gLastFindString = CopyStringTo(gLastFindString, string, "LastFindStringFunction");
}

const char*
LastExecuteURLFunction()
{
	if (gLastExecuteURL == nil)
		gLastExecuteURL = CopyStringTo(gLastExecuteURL, "", "LastExecuteURLFunction");
		
	return gLastExecuteURL;
}

void
SetLastExecuteURL(const char* string)
{
	gLastExecuteURL = CopyStringTo(gLastExecuteURL, string, "LastExecuteURLFunction");
}

// ===========================================================================
//    implementation
// ===========================================================================

BackURL::BackURL()
{
	fSelection = kInvalidSelection;
}

BackURL::~BackURL()
{
	if (fFormData != nil) {	
		delete(fFormData);
	}
}

// =============================================================================

PageViewer::PageViewer()
{
	IsError(gPageViewer != nil);
	gPageViewer = this;
}

PageViewer::~PageViewer()
{
	IsError(gPageViewer != this);
	gPageViewer = nil;

	if (fPendingBackURL != nil)	
		delete(fPendingBackURL);
	if (fCancelBackURL != nil)
		delete(fCancelBackURL);
	fBackPages.DeleteAll();
}

void 
PageViewer::AddBack(const Resource* resource)
{
	// Add specified URL.
	
	// If there is pending back data, delete it
	if (fPendingBackURL != nil) {	
		delete(fPendingBackURL);
		fPendingBackURL = nil;
	}
	if (fCancelBackURL != nil) {	
		delete(fCancelBackURL);
		fCancelBackURL = nil;
	}
	
	if (fDocument == nil || fDocument->GetSkipBack()) {
		return;
	}
	
	PushDebugChildURL("<Back list>");
	
	// Need to delete first elements from list when backlist limit is exceeded.	
	if (fBackPages.GetCount() >= kBackLimit)
		fBackPages.DeleteAt(0);
	
	BackURL* backURL = NewBackURL(resource);
	
	if (backURL != nil)			
		fBackPages.Add(backURL);

	PopDebugChildURL();
}

void 
PageViewer::AddRecent(const Resource* resource, const char* title)
{
	// If the page is already in the list, just touch its last visited.
	long pageIndex = gRecentPanel->FindPage(resource);
	if (pageIndex != kInvalidPage && 
		gRecentPanel->RecentURLAt(pageIndex)->GetThumbnail()->GetStatus() == kComplete) {
		gRecentPanel->RecentURLAt(pageIndex)->Touch();
		return;
	}
	
	long oldScroll = fScrollPosition;
	long oldBottom = fBounds.bottom;

	ScrollBy(-oldScroll);
	fBounds.bottom = gScreen->GetHeight();
	
	Draw(&fBounds);
	InvalidateAbove(&fBounds);

	fBounds.bottom = oldBottom;
	ScrollBy(oldScroll);
	
	InvalidateBounds();
	
	Resource*	thumbnail;	
	Image*		logoImage = fDocument->GetLogoImage();
	Boolean		hasLogo = (logoImage != nil && logoImage->GetStatus() == kComplete);
	
	if (hasLogo)
		thumbnail = (Resource*)logoImage->GetResource();
	else
		thumbnail = NewThumbnail();
	gRecentPanel->AddPage(resource, title, thumbnail);
	if (hasLogo) {
		delete(logoImage);
	}
	else {
		delete(thumbnail);
	}
}

long 
PageViewer::AlignScrollPoint(Displayable* lineStart, long newScroll, long oldScroll)
{
	// If there is only one displayable on the line, see if it can give us a better 
	// scroll alignment.

	// Skip leading linebreaks;
	Displayable*	item;
	for (item = lineStart; item != nil; item = (Displayable*)item->Next())
		if (!item->IsLineBreak())
			break;

	// Find end of line.
	Displayable*	lineBegin = item;
	for (; item != nil; item = (Displayable*)item->Next())
		if (item->IsLineBreak())
			break;
	
	if (item != nil && item->Previous() == lineBegin)
		return lineBegin->AlignScrollPoint(newScroll, oldScroll);
	
	return newScroll;
}

Boolean 
PageViewer::BackInput()
{
	// If we already have a back in progress, cancel it.
	if (fCancelBackURL != nil) {
		fBackPages.Add(fPendingBackURL);
		fPendingBackURL = fCancelBackURL;
		fCancelBackURL = nil;
		ContentView::ShowResource(fPendingBackURL->GetResource());
		return true;
	}
		
	long 		count = fBackPages.GetCount();
	
	if (count < 1) {
		gLimitSound->Play();
		return false;
	}
		
	fPendingBackURL = (BackURL*)fBackPages.At(count - 1);
	if (IsError(fPendingBackURL == nil))
		return false;
		
	gEndLinkSound->Play();		
	fBackPages.RemoveAt(count - 1);

	// If we're still waiting on data, hide the status indicator.
	if (fIdleState == kWaitForDocument) {
		gStatusIndicator->Hide();
		ChangeIdleState(fLastIdleState, fIdleState);
		RefreshSelection();
	}
		
	// If for the same URL, just scroll to selection, otherwise, we need to execute the URL.
	if (*fPendingBackURL->GetResource() == fResource && fDocument != nil) {
		ScrollToPosition(fPendingBackURL->GetScrollPosition());
		SetCurrentSelection(fPendingBackURL->GetSelection());
		fSelectionPosition = fPendingBackURL->GetSelectionPosition();
		fInsideImageMap = fPendingBackURL->IsInImageMap();
		fMapCursorPosition = fPendingBackURL->GetMapCursorPosition();
		delete fPendingBackURL;
		fPendingBackURL = nil;
	} else {
		fCancelBackURL = NewBackURL(&fResource);
		// Call ContentView directly to avoid add this to back.	
		ContentView::ShowResource(fPendingBackURL->GetResource());
	}

	return true;
}

void 
PageViewer::ChangeIdleState(ViewIdleState newState, ViewIdleState oldState)
{
	#if defined(HARDWARE) && defined(DEBUG)
	ulong time;
	#endif
	
	ContentView::ChangeIdleState(newState, oldState);
	
	switch (newState) {
		case kViewIdle:
			gStatusIndicator->Hide();
			gOptionsPanel->StopPhoneProgress();
			break;
			
		case kWaitForDocument:
			// Show status message.
			if (fResource.GetStatus() != kComplete) {
				gStatusIndicator->SetMessage("Asking for page");
				gStatusIndicator->SetTarget(&fResource);
				gStatusIndicator->Show();
				gStatusIndicator->SetPercentDone(5); 	// 5% done for sending request.
			}
			gOptionsPanel->ScrollStatusChanged();				
			break;
				
		case kBeginDocument:
			// Reset input queue.
		#if defined(HARDWARE) && defined(DEBUG)
			SetCounter(0);
		#endif
			FlushInput();
			
			if (fPendingBackURL != nil && fTargetFragment == nil) {
				fTargetSelection = fPendingBackURL->GetSelection();
				fSelectionPosition = fPendingBackURL->GetSelectionPosition();
				fInsideImageMap = fPendingBackURL->IsInImageMap();
				fMapCursorPosition = fPendingBackURL->GetMapCursorPosition();
				fScrollPosition = fPendingBackURL->GetScrollPosition();
			}
			gOptionsPanel->StartPhoneProgress();
			break;
		
		case kDocumentTitleComplete:
			if (fDocument->GetTitle() != nil) {
				gStatusIndicator->SetMessage(fDocument->GetTitle(), true);
				
				if (!fDocument->GetSkipBack()) {
					long pageIndex = gRecentPanel->FindPage(&fResource);
					if (pageIndex != kInvalidPage) {
						gRecentPanel->RecentURLAt(pageIndex)->Touch();
					} else {
						gRecentPanel->AddPage(&fResource, fDocument->GetTitle(), nil);
					}
				}
			}
			gInfoPanel->WritePage();
			break;
		
		case kDocumentContentComplete:
		{
			// the rest of this is for images
			uchar	percentDone = GetPercentComplete();
			if (!gStatusIndicator->IsVisible() && percentDone < 100 &&
				fDrawIdleState <= kWaitForInitialScreen) {
				gStatusIndicator->SetTarget(&fResource);
				gStatusIndicator->Show();
				gStatusIndicator->SetPercentDone(MIN(percentDone, 5)); 	// 5% done for sending request.
			}
			gStatusIndicator->SetMessage("Getting pictures");
			RestoreFormData();
			break;
		}	
		case kDocumentLoadingComplete:
		#if defined(HARDWARE) && defined(DEBUG)
			time = ((FetchCounter())*2/(READ_AG(agCPUSpeed)/1000)); /* convert to milliseconds */
			Message(("Page took %d msecs to complete\n",time));
		#endif
			gOptionsPanel->StopPhoneProgress();

			{
			Indicator *indicator;
			
/* hack to show the dial status indicator only after startup page is loaded */

			if ((indicator = GetQueuedShow()) != 0)
				{
				SetQueuedShow(0);
				indicator->Show();
				}
			}
			break;
		default:
			break;
	}
}

void
PageViewer::ClearBackList()
{
	fBackPages.DeleteAll();
	fVisitedPages.DeleteAll();
}

const Resource* 
PageViewer::GetThumbnail()
{
	ulong i;

	if (fDocument == nil)
		return nil;
		
	if ((i = gRecentPanel->FindPage(fDocument->GetResource())) == kInvalidPage)
		return nil;	
	
	return gRecentPanel->RecentURLAt(i)->GetThumbnail();
}

ViewIdleState 
PageViewer::HandleNonShowableResource(DataType type)
{
	// We're not going to a new page, so delete the back entry we just acquired.
	if (fDocument != nil && fBackPages.GetCount() > 0)
		fBackPages.DeleteAt(fBackPages.GetCount() - 1);
	
	// The document will now be show.  Remove the status indicator.		
	if (gStatusIndicator->IsVisible()) {
		Rectangle	bounds;
		gStatusIndicator->SetPercentDone(100);
		gStatusIndicator->GetBounds(&bounds);
		gStatusIndicator->Draw(&bounds);
		UpdateScreenBits();
		gStatusIndicator->Hide();
	}
	
	// Update scroll and phone status.
	gOptionsPanel->ScrollStatusChanged();
	gOptionsPanel->StartPhoneProgress();

	return ContentView::HandleNonShowableResource(type);
}

void 
PageViewer::Idle()
{
	PerfDump perfdump("PageViewer::Idle");
	// If we are building the document, preflight for available memory.
	if (fIdleState > kWaitForDocument && fIdleState < kDocumentContentComplete &&
		fDocument->GetDisplayStatus() /* Hack! Connecting + Splash screens don't fit w/ MACtcp*/) {
		void*	preflight = AllocateTaggedMemoryNilAllowed(kMemoryPreflightSize, "PageViewer::Idle");
	
		// If we cannot allocate the preflight memory, finalize what
		// we've done so far and abort.	
		if (preflight == nil) {
			fDocumentBuilder->AbortAndFinalize();
			fDocument->Layout();
			DeleteStream();
			fResource.SetStatus(kTruncated);
			fTargetFragment = nil;
			ChangeIdleState(kDocumentContentComplete, fIdleState);
		}
		else
			FreeTaggedMemory(preflight, "PageViewer::Idle");
	}
	
	ContentView::Idle();	
	
	if (fDocument != nil) {
		Image*		logo = fDocument->GetLogoImage();
		Error		status;
		
		if (logo != nil && (status = logo->GetStatus()) != kNoError && status != kPending) {
			AddRecent(&fResource, fDocument->GetTitle());
			fDocument->SetLogoImage(nil);
		}
	}
}

BackURL* 
PageViewer::NewBackURL(const Resource* resource)
{
	if (fDocument == nil)
		return nil;
		
	BackURL* backURL = new(BackURL);
	backURL->SetResource(resource);
	backURL->SetSelection(fCurrentSelection);
	backURL->SetSelectionPosition(fSelectionPosition);
	backURL->SetInImageMap(fInsideImageMap);
	backURL->SetMapCursorPosition(fMapCursorPosition);
	backURL->SetScrollPosition(fScrollPosition);
	
	// If the document has forms, add the form data;
	if (fDocument->GetFormCount() != 0) {
		StringList*	formList = new(StringList);
		
		for (long i = 0; i < fDocument->GetFormCount(); i++) {
			Form*	form = fDocument->GetForm(i);
			
			for (long j = 0; j < form->GetControlCount(); j++) {
				Control*	control = form->GetControl(j);
				formList->Add(control->SaveFillinData());
			}
		}
		backURL->SetFormData(formList);
	}
	
	return backURL;			
}

Resource* 
PageViewer::NewThumbnail()
{
	if (!fResource.HasURL())
		return nil;
	
	PushDebugChildURL("<Thumbnail>");

	// Create thumbnail URL.

	char* thumbnailURL = NewLocalURL(&fResource, "cache:", ".thumbnail", "Thumbnail");
	Resource* newResource = new(Resource);
	newResource->SetURL(thumbnailURL);
	newResource->SetDataType(kDataTypeBitmap);
	newResource->SetPriority(kImmediate);	
	newResource->SetStatus(kNoError);
	FreeTaggedMemory(thumbnailURL, "Thumbnail");
	
	// Write bitmap image to cache.
	CacheStream* stream = newResource->NewStreamForWriting();
	if (!IsError(stream == nil)) {
		BitMapImage* image = new(BitMapImage);
		BitMapDevice* thumbnail = ::NewThumbnail(&gScreenDevice, 8);
	
		image->Write(stream, thumbnail);
		stream->SetStatus(kComplete);

		DeleteBitMapDevice(thumbnail);
		delete(image);
		delete(stream);
	}
	
	PopDebugChildURL();

	newResource->SetPriority(kBackground);

	return newResource;
}

void 
PageViewer::RestoreFormData()
{
	if (fPendingBackURL != nil && fPendingBackURL->GetFormData() != nil) {
		PostulateFinal(false);			// Dave - need additional validation checks.
										// Expires and maybe CRC.
		
		// Verify that the control count for the document matches that of 
		// the string list.
		long	i, j;
		long	controlCount = 0;
		for (i = 0; i < fDocument->GetFormCount(); i++)
			controlCount += fDocument->GetForm(i)->GetControlCount();
			
		if (controlCount == fPendingBackURL->GetFormData()->GetCount() &&
			!fPendingBackURL->GetResource()->HasExpired()) {
			long	index = 0;
			for (i = 0; i < fDocument->GetFormCount(); i++) {
				Form*	form = fDocument->GetForm(i);
				
				for (j = 0; j < form->GetControlCount(); j++) {
					char*	string = (char*)fPendingBackURL->GetFormData()->At(index++);
					if (string != nil)
						form->GetControl(j)->RestoreFillinData(string);
				}
			}
		}
		if (fDrawIdleState > kWaitForInitialScreen) {
			delete(fPendingBackURL);
			fPendingBackURL = nil;
			InvalidateBounds();
		}
	}
}

long 
PageViewer::ScrollBy(long offset)
{
	if (fDocument == nil)
		return 0;

	long	newScroll = MIN(MAX(fScrollPosition + offset, 0), fDocument->GetHeight());
	
	// Calculate actual offset for return;
	offset = newScroll - fScrollPosition;
	
	if (offset != 0) {
		fScrollPosition = newScroll;
		fDocument->GetRootDisplayable()->SetTopDisplayable(nil);
		InvalidateBounds();
	}
		
	return offset;
}
	
Boolean 
PageViewer::ScrollDown(long delta, long minDelta)
{
	PerfDump perfdump("PageViewer::ScrollDown");

	if (!CanScrollDown())
		return false;

	long 	pageHeight = GetHeight() - fTopMargin;
	long 	docHeight = fDocument->GetHeight();
	long 	newScroll = fScrollPosition + delta;
	long 	oldScroll = fScrollPosition;
	Boolean oldCanScrollUp = CanScrollUp();
	
	RootPage* rootPage = fDocument->GetRootDisplayable();
	Displayable*	topDisplayable = rootPage->GetTopDisplayable();
		
	if (fDrawIdleState > kWaitForInitialScreen && gSystemLogo->IsVisible()) {
		gSystemLogo->Hide();
		gScreen->RedrawNow();
	}
					
	if (topDisplayable == nil)
		topDisplayable = rootPage->GetFirstChild(); 
	
	// Now find the top of the last visible line to use as the new scroll position. 
	// If current line spans the page, we just move down a full page.
	while (topDisplayable != nil) {
		Rectangle	lineBounds = rootPage->GetLineBounds(topDisplayable);
		
		if (lineBounds.bottom <= newScroll && lineBounds.top < (docHeight - pageHeight + fTopMargin)) {
			do {
				topDisplayable = (Displayable*)topDisplayable->Next();
			} while (topDisplayable != nil && !topDisplayable->IsLineBreak());
		} else {
			// If the top line is less than the minimum movement, move full delta.
			if ((lineBounds.top - fScrollPosition) < minDelta &&
				 lineBounds.top < (docHeight - pageHeight + fTopMargin)) {
				// See if the displayable can provide us with a better alignment.
				long alignedScroll = AlignScrollPoint(topDisplayable, newScroll, fScrollPosition);
				
				if ((alignedScroll - fScrollPosition) < minDelta)
					fScrollPosition = newScroll;
				else
					fScrollPosition = alignedScroll;
					
				if (fScrollPosition == newScroll)
					fScrollPosition = MIN(fScrollPosition, docHeight - pageHeight + fTopMargin);
			}
			else if (lineBounds.top <= newScroll) {
				fScrollPosition = lineBounds.top;
				
				// Don't leave more than min delta at bottom.
				if ((fScrollPosition + pageHeight - (docHeight + fTopMargin)) > minDelta) {
					topDisplayable = rootPage->GetTopDisplayable();
					fScrollPosition = docHeight + fTopMargin - pageHeight;
				}
			} 
			else { 	// Next line is below delta, only scroll full delta.
				fScrollPosition += delta;
				if (fScrollPosition > (docHeight - pageHeight + fTopMargin)) {
					fScrollPosition = docHeight - pageHeight + fTopMargin;
					topDisplayable = rootPage->GetTopDisplayable();
				}
			}
			break;
		}
	}
	
	rootPage->SetTopDisplayable(topDisplayable);		
	
	if (fDrawIdleState > kWaitForInitialScreen) {
		 // Record direction and distance for scroll
		gPageViewerScrollDirection += fScrollPosition - oldScroll;
	
		if (!CanScrollDown() || !oldCanScrollUp)	
			gOptionsPanel->ScrollStatusChanged();
		
		gKeyboard->ClearTargetViewOffset();
		InvalidateBounds();
	}		
	
	return true;
}

Boolean 
PageViewer::ScrollDownInput(Input*)
{
	if (!CanScrollDown()) {
		gLimitSound->Play();
		return false;
	}

	// This will remove the selection before scrolling...
	if (!SelectionsEqual(fCurrentSelection, kInvalidSelection) &&
	    fCurrentSelection.fPageIndex == 0) {
		Boolean oldShowSelection = fShowSelection;
		fShowSelection = false;		
		RefreshSelection(fCurrentSelection);
		gScreen->RedrawNow();
		fShowSelection = oldShowSelection;
	}

	long delta = GetHeight() - fTopMargin - kPageOverlap;
	if (!ScrollDown(delta, delta/2))
		return false;

	if (fCurrentSelection.fPageIndex == 0) {	
		Selection	s = fCurrentSelection;	
		if (s.fSelectableIndex > kInvalidSelectable)
			s.fSelectableIndex--; 
						
		SelectTopAnchor(s);
	}	
	return true;
}

Boolean 
PageViewer::ForwardInput()
{
	return false;
}

Boolean 
PageViewer::ScrollUp(long delta, long minDelta)
{
	PerfDump perfdump("PageViewer::ScrollUp");
	
	if (!CanScrollUp())
		return false;
		
	RootPage*	rootPage = fDocument->GetRootDisplayable();
	long 		newScroll = fScrollPosition - delta;
	long		oldScroll = fScrollPosition;
	Boolean		oldCanScrollDown = CanScrollDown();

	if (fDrawIdleState > kWaitForInitialScreen && gSystemLogo->IsVisible()) {
		gSystemLogo->Hide();
		gScreen->RedrawNow();
	}
					
	if (newScroll <= 0) {
		rootPage->SetTopDisplayable(nil);
		fScrollPosition = 0;
	} else {
		// Find the current line at or intersecting the top of the new page.
		Displayable*	topDisplayable = rootPage->GetTopDisplayable();
		Displayable*	child = topDisplayable;
		
		// Now find the top of the first fully visible line to use as the new scroll position. 
		// If current line spans the page, we just move up a full page.
		
		while (child != nil) {
			Rectangle	lineBounds = rootPage->GetLineBounds(child);
			
			if (lineBounds.top >= newScroll || EmptyRectangle(&lineBounds)) {
				if (lineBounds.top >= newScroll)
					topDisplayable = child;
				do {
					child = (Displayable*)child->Previous();
				} while (child != nil && child->Previous() != nil && !child->IsLineBreak());
				
				if (child == nil) {
					fScrollPosition = rootPage->GetLineBounds(topDisplayable).top;
				}
			} else {
				Rectangle	topBounds = rootPage->GetLineBounds(topDisplayable);
				
				// If the top line is less the minimum movement, move full delta.
				if ((fScrollPosition - topBounds.top) < minDelta || topBounds.top < newScroll) {
					topDisplayable = child;
					
					// See if the displayable can provide us with a better alignment.
					long alignedScroll = AlignScrollPoint(topDisplayable, newScroll, fScrollPosition);
					
					if ((fScrollPosition - alignedScroll) < delta/2)
						fScrollPosition = newScroll;
					else
						fScrollPosition = alignedScroll;
				}
				// Otherwise use the topmost line that fit on the page.
				else {
					fScrollPosition = rootPage->GetLineBounds(topDisplayable).top;
				}
				break;
			}
		}
		
		rootPage->SetTopDisplayable(topDisplayable);
	}
		
	if (fDrawIdleState > kWaitForInitialScreen) {
		// Record direction and distance for scroll
		gPageViewerScrollDirection += fScrollPosition - oldScroll; 
		
		if (!CanScrollUp() || !oldCanScrollDown)	
			gOptionsPanel->ScrollStatusChanged();
	
		gKeyboard->ClearTargetViewOffset();
		InvalidateBounds();
	}

	return true;
}

Boolean 
PageViewer::ScrollUpInput(Input*)
{
	if (!CanScrollUp()) {
		gLimitSound->Play();
		return false;
	}
	
	// This will remove the selection before scrolling...
	if (!SelectionsEqual(fCurrentSelection, kInvalidSelection) &&
		fCurrentSelection.fPageIndex == 0) {
		Boolean oldShowSelection = fShowSelection;
		fShowSelection = false;		
		RefreshSelection(fCurrentSelection);
		gScreen->RedrawNow();
		fShowSelection = oldShowSelection;
	}

	long delta = GetHeight() - fTopMargin - kPageOverlap;
	if (!ScrollUp(delta, delta/2))
		return false;

	if (fCurrentSelection.fPageIndex == 0)	
		SelectTopAnchor();
		
	return true;
}

void 
PageViewer::FirstScreenDrawingCompleted()
{
	ContentView::FirstScreenDrawingCompleted();
	
	// Remember the new page.
	if (fDocument->GetLogoImage() == nil)
		if (fDocument != nil && !fDocument->GetSkipBack())
			AddRecent(&fResource, fDocument->GetTitle());
	
	gInfoPanel->WritePage();
}

ulong 
PageViewer::GetPageCount()
{
	return gPageCount;
}

void 
PageViewer::InitialScreenLayoutCompleted()
{
	TransitionEffect	transitionEffect = fDocument->GetTransitionEffect();
	
	ContentView::InitialScreenLayoutCompleted();

	// No longer can cancel back operation.	
	if (fCancelBackURL != nil) {
		delete(fCancelBackURL);
		fCancelBackURL = nil;
	}
	if (fPendingBackURL != nil && 
		(fPendingBackURL->GetFormData() == nil || fIdleState >= kDocumentContentComplete)) {
		delete(fPendingBackURL);
		fPendingBackURL = nil;
	}
	
	gOptionsPanel->TitleChanged();

	// Check to see if status should be turned off (or on).	
	if (fDocument->GetDisplayStatus() != !gStatusIndicator->IsDisabled())
		gStatusIndicator->SetDisabled(!fDocument->GetDisplayStatus());

	// The document will now be show.  Remove the status indicator.		
	if (gStatusIndicator->IsVisible()) {
		Rectangle	bounds;
		gStatusIndicator->SetPercentDone(100);
		gStatusIndicator->GetBounds(&bounds);
		gStatusIndicator->Draw(&bounds);
		UpdateScreenBits();
		gStatusIndicator->Hide();
		gSystemLogo->StartFading();
	}

	// Check to see if the option panel should be turned off (or on).	
	if (fDocument->GetEnableOptions() != gOptionsPanel->IsEnabled()) {
		if (fDocument->GetEnableOptions())
			gScreen->EnableOptionsPanel();
		else 
			gScreen->DisableOptionsPanel();
	}
	
	// Check to see if the option panel should be hidden (or shown.	
	if (fDocument->GetDisplayOptions() != gOptionsPanel->IsShown()) {
		if (fDocument->GetDisplayOptions())
			gScreen->ShowOptionsPanel();
		else 
			gScreen->HideOptionsPanel();
	}
	
	PlaySongSoon();
	gPageViewerTransitionType = transitionEffect;			// can put other effect types here
	gPageViewerTransitionDelay = 2;
	gPageCount++;
}

void 
PageViewer::ScreenLayoutCompleted()
{
	ContentView::ScreenLayoutCompleted();

	gOptionsPanel->ScrollStatusChanged();
}

void 
PageViewer::ShowFragment(const char* fragment)
{
	AddBack(&fResource);
	fVisitedPages.Add(GetResource(), fragment);
	ContentView::ShowFragment(fragment);
}

void 
PageViewer::ShowResource(const Resource* resource)
{
	AddBack(&fResource);
	fVisitedPages.Add(resource, fTargetFragment);
	ContentView::ShowResource(resource);
}

#ifdef DEBUG
void
PageViewer::ViewSource(void)
{
/* еее
	Resource destResource;
	const Resource* sourceResource = GetResource();
	if (sourceResource == nil)
		goto ViewSource_fail;
	
	destResource.SetURL("cache:/ViewSource");
	
	DataStream* destStream;
	DataStream* sourceStream = resource->NewStream();
	
*/	
}
#endif /* DEBUG */

Boolean
PageViewer::WasVisited(const Resource* base, const char* partial) const
{
	return fVisitedPages.Find(base, partial) != nil;
}

// =============================================================================

VisitedList::VisitedList()
{
	// Set up data list and add one bogus entry to force allocation in low memory
	fList.SetDataSize(sizeof(VisitedURL));
	fList.SetListIncrement(kVisitLimit);
	VisitedURL	visited;
	fList.Add(&visited);
	fList.RemoveAll();
}

VisitedList::~VisitedList()
{
}

void 
VisitedList::Add(const Resource* base, const char* fragment)
{
	if (IsError(base == nil || base->GetURL() == nil))
		return;
	
	long	count = fList.GetCount();
	long	oldest = 0;
	long	oldestTime = Now();
	ulong	hash = InitCrc32();
	long	i;
	
	hash = UpdateCrc32(hash, base->GetURL(), strlen(base->GetURL()));
	if (fragment != nil)
		hash = UpdateCrc32(hash, fragment, strlen(fragment));
		
	for (i = 0; i < count; i++) {
		VisitedURL*	current = (VisitedURL*)fList.At(i);
		if (!IsError(current == nil)) {
			if (current->GetHash() == hash) {
				current->Touch();
				return;
			}
			if (current->LastVisited() < oldestTime) { 
				oldest = i;
				oldestTime = current->LastVisited();
			}
		}
	}
		
	PushDebugChildURL("<Visited list>");
	
	// If the list if full, remove the oldest
	if (count >= kVisitLimit)
		fList.RemoveAt(oldest);

	// Add to list, sorted on hash
	VisitedURL	visited;
	visited.SetURL(base, fragment);
	
	count = fList.GetCount();
	for (i = 0; i < count; i++) {
		VisitedURL*	current = (VisitedURL*)fList.At(i);
		if (!IsError(current == nil) && visited.GetHash() < current->GetHash())
			break;
	}
		
	fList.AddAt(&visited, i);
	
	PopDebugChildURL();
}

void
VisitedList::DeleteAll()
{
	fList.RemoveAll();
}

VisitedURL* 
VisitedList::Find(const Resource* base, const char* partial) const 
{
	if (IsError(base == nil || base->GetURL() == nil))
		return nil;
		
	VisitedURL*	found = nil;
	
	URLParser urlParser;
	
	urlParser.SetURL(base->GetURL());
	char*	temp = urlParser.NewURL(partial, "VisitedList::Find");
	urlParser.SetURL(temp);
	FreeTaggedMemory(temp, "VisitedList::Find");
	
	const char*	url = urlParser.GetURLNoFragment();
	const char*	fragment = urlParser.GetFragment();
	ulong 		hash = InitCrc32();
	
	// Compute hash for url.
	hash = UpdateCrc32(hash, url, strlen(url));
	if (fragment != nil)
		hash = UpdateCrc32(hash, fragment, strlen(fragment));
	
	// Perform binary search on hash.
	long lower = 0;
	long upper = fList.GetCount() - 1;
	while (upper >= lower && found == nil) {
		long i = (lower + upper) / 2;
		VisitedURL*	current = (VisitedURL*)fList.At(i);
		
		if (IsError(current == nil))
			break;
		
		if (hash == current->GetHash())
				found = current;
		else if (hash < current->GetHash())
			upper = i - 1;
		else
			lower = i + 1;
	}
	
	return found;
}

// ============================================================================

VisitedURL::VisitedURL()
{
	fLastVisited = Now();
}

VisitedURL::~VisitedURL()
{
}

void 
VisitedURL::SetURL(const Resource* resource, const char* fragment)
{
	// Compute hash.
	const char*	url = resource->GetURL();
	if (IsError(url == nil))
		return;

	fHash = InitCrc32();
	fHash = UpdateCrc32(fHash, url, strlen(url));
	if (fragment != nil)
		fHash = UpdateCrc32(fHash, fragment, strlen(fragment));
}

void
VisitedURL::Touch()
{
	fLastVisited = Now();
}

// =============================================================================

