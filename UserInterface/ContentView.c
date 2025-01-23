// ===========================================================================
//	ContentView.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include "CrashLogC.h"
#undef s1
#include "boxansi.h"

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif

#include "AlertWindow.h"
#include "Cache.h"
#include "CacheStream.h"
#include "ClientFunctions.h"
#include "Code.h"
#include "DocumentBuilder.h"
#include "ImageData.h"
#include "Keyboard.h"
#include "MemoryManager.h"
#include "Menu.h"
#include "Network.h"
#include "ObjectStore.h"
#include "OptionsPanel.h"
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#include "Screen.h"
#include "Song.h"
#include "SongData.h"
#include "Sound.h"
#include "Status.h"
#include "System.h"
#include "SystemLogo.h"
#include "TextField.h"

// ===========================================================================
//	globals/locals
// ===========================================================================

static const ulong kInitialScreenDrawTimeout = kOneSecond * 8;
static const long kSelectionScrollIncrement = 100;
static const long kSelectionMargin = 4;

static const char kTopFragment[] = "top";
static const char kBottomFragment[] = "bottom";

// ===========================================================================
//	implementations
// ===========================================================================

ContentView::ContentView()
{
	fCurrentSelection = kInvalidSelection;
	fTargetSelection = kInvalidSelection;
	fShouldAutoScroll = true;
	fShowSelection = true;
	fShowActiveSelection = true;
	fShouldPaintBackground = true;
	
	SelectionLayer*	selectionLayer = new(SelectionLayer);
	selectionLayer->Initialize(this);
	AddChild(selectionLayer);
	
	SetBounds(0, 0, gScreen->GetWidth(), gScreen->GetHeight());
}

ContentView::~ContentView()
{
	if (fCurrentSong != nil) {
		delete(fCurrentSong);
		fCurrentSong = nil;
	}
	if (fDocument != nil) {	
		delete(fDocument);
		fDocument = nil;
	}
	if (fDocumentBuilder != nil) {	
		delete(fDocumentBuilder);
		fDocumentBuilder = nil;
	}
	if (fLastDocument != nil) {	
		delete(fLastDocument);
		fLastDocument = nil;
	}
	if (fParser != nil) {	
		delete(fParser);
		fParser = nil;
	}
	if (fTargetFragment != nil) {	
		FreeTaggedMemory(fTargetFragment, "ContentView::fTargetFragment");
		fTargetFragment = nil;
	}
	if (fStream != nil) {	
		delete(fStream);
		fStream = nil;
	}
	if (fChild != nil) {
		fChild->DeleteAll();
		fChild = nil;
	}
}

Boolean 
ContentView::CanScrollDown() const
{
	return ((fScrollPosition + GetHeight() - fTopMargin * 2) < (fDocument ? fDocument->GetHeight() : 0) && 
			fIdleState > kWaitForDocument);
}

Boolean 
ContentView::CanScrollUp() const
{
	return (fScrollPosition > 0 && fIdleState > kWaitForDocument);
}

Boolean 
ContentView::Completed()
{
	return fIdleState == kDocumentLoadingComplete && fDrawIdleState == kDrawingComplete;
}

void 
ContentView::BackgroundToScreen(Rectangle* bounds) const
{
	// ContentToScreen expects content coordinates and returns screen coordinates
	
	Rectangle viewBounds;
	GetBounds(&viewBounds);
	OffsetRectangle(*bounds, viewBounds.left, viewBounds.top - fScrollPosition);
}

void 
ContentView::ContentToScreen(Rectangle* bounds) const
{
	// ContentToScreen expects content coordinates and returns screen coordinates

	Rectangle viewBounds;
	GetBounds(&viewBounds);
	OffsetRectangle(*bounds, viewBounds.left, viewBounds.top - fScrollPosition + fTopMargin);
}

void 
ContentView::ContentToScreen(Region* region) const
{
	// ContentToScreen expects content coordinates and returns screen coordinates

	Rectangle viewBounds;
	GetBounds(&viewBounds);
	region->Offset(viewBounds.left, viewBounds.top - fScrollPosition + fTopMargin);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
ContentView::ScreenToBackground(Rectangle* bounds) const
{
	// ScreenToBackground expects screen coordinates and returns background coordinates
	
	Rectangle viewBounds;
	GetBounds(&viewBounds);
	OffsetRectangle(*bounds, -viewBounds.left, fScrollPosition - viewBounds.top);
}
#endif

void 
ContentView::ScreenToContent(Rectangle* bounds) const
{
	// ScreenToContent expects screen coordinates and returns content coordinates
	
	Rectangle viewBounds;
	GetBounds(&viewBounds);
	OffsetRectangle(*bounds, -viewBounds.left, fScrollPosition - viewBounds.top - fTopMargin);
}

void 
ContentView::DeleteDocument()
{
	DeleteStream();
	
	if (fDocument != nil) {
		delete(fDocument);
		fDocument = nil;
	}
}

void 
ContentView::DeleteStream()
{
	if (fDocumentBuilder != nil) {	
		delete(fDocumentBuilder);
		fDocumentBuilder = nil;
	}
	if (fParser != nil) {	
		delete(fParser);
		fParser = nil;
	}
	if (fStream != nil) {	
		delete(fStream);
		fStream = nil;
	}
}

void 
ContentView::DocumentHeightChanged(long, long)
{	
	// If we have a target fragment or selection we should scroll as soon
	// as possible so the layout/image loading can concentrate on 
	// the visible items.

	if (fTargetFragment != nil && ScrollToFragment(fTargetFragment)) {
		FreeTaggedMemory(fTargetFragment, "ContentView::fTargetFragment");
		fTargetFragment = nil; 
	}
	
	if (!SelectionsEqual(fTargetSelection, kInvalidSelection) && ScrollToSelection(fTargetSelection))
		fTargetSelection = kInvalidSelection;
}

Boolean 
ContentView::DownInput(Input* input)
{
	if (!fIsVisible || fDocument == nil)
		return false;
	
	Displayable* current = fDocument->GetSelectable(fCurrentSelection);

	// handle scrolling lists.
	if (current != nil && current->DownInput())
		return true;

	SelectNextAnchor(false, input->modifiers & kAutoRepeat);
	
	return true;
}

void 
ContentView::Draw(const Rectangle* invalid)
{
	if (fDocument == nil && fLastDocument == nil)
		return;
		
	// Guarantee non-nil invalid for children.
	Rectangle	bounds = (invalid == nil ? fBounds : *invalid);

	// Clear any pending selection background restore.
	GetSelectionLayer()->DeleteBackBuffer();
			
	if (fLastDocument != nil) {
		fLastDocument->DrawBackground(&bounds);
		fLastDocument->Draw(&bounds);
	}	
	else if (fDocument != nil) {
		// If the current selection is a text field, and the invalid rect is 
		// completely contained in its bounds, we don't need to draw the background.
		Boolean	drawBackground = true;
		Displayable*	current = fDocument->GetSelectable(fCurrentSelection);

		if (current != nil && current->IsTextField()) {
			Rectangle	currentBounds;
			current->GetBounds(&currentBounds);
			ContentToScreen(&currentBounds);
			if (RectangleContainedInRectangle(&bounds, &currentBounds))
				drawBackground = false;
		}
		
		if (drawBackground)
			fDocument->DrawBackground(&bounds);
		fDocument->Draw(&bounds);	
			
#ifdef REGION_DEBUG
		{
			ulong			count = fDocument->GetSelectableCount();
			Rectangle		bounds;
			Displayable*	current;
			short			i;
			
			for (i=0; i<count; i++) {
				current = fDocument->GetSelectable(i);
				current->GetBounds(&bounds);
				if (bounds.right <= bounds.left || bounds.bottom <= bounds.top)
					continue;
	
				FrameRectangle(gScreenDevice, bounds, 1, 0xff0000, 0);
	
				current->GetSelectionRegion(gSelectionRegion);
				gSelectionRegion->Draw(1, 0xff00);
				
				InvalidateBounds();
			}
		}
#endif
	}
}

#ifdef FIDO_INTERCEPT
void 
ContentView::Draw(FidoCompatibilityState& fidoCompatibility) const
{
	if (fDocument == nil && fLastDocument == nil)
		return;
		
	if (fLastDocument != nil) {
		fLastDocument->DrawBackground(fidoCompatibility);
		fLastDocument->Draw(fidoCompatibility);
	}	
	else if (fDocument != nil) {
		fDocument->DrawBackground(fidoCompatibility);	
		fDocument->Draw(fidoCompatibility);
	}
}
#endif

static char* 
ConvertURLToNewsBrowser(char* url, const char* USED_FOR_DEBUG(tag))
{
	char*	newsName;
	
	if (!EqualStringN("news:", url, 5))
		return url;
		
	// get past protocol prefix and spaces
	newsName = url + 5;
	while (isspace(*newsName))
		newsName++;

	static const char format[] = "http://arcadia/goldman/cgi-bin/wwwnntp?%s";
	long allocSize = sizeof(format) + strlen(newsName) - 2; // -2 for %s, sizeof() includes NULL
	url = (char*)ReallocateTaggedMemory(url, allocSize, tag);	
	snprintf(url, allocSize, format, newsName);
	return url;
}

Boolean 
ContentView::DispatchInput(Input* input)
{
	switch (input->data) {
		case kBackKey:	
		case kExecuteKey:
		case kForwardKey:
		case kHomeKey:
		case kOptionsKey:
		case kRecentKey:
				// Hide logo for all screen transitions.
				gSystemLogo->Hide();
				break;

		case kDownKey:
		case kLeftKey:
		case kRightKey:
		case kScrollDownKey:
		case kScrollUpKey:
		case kUpKey:
				// No navigation when waiting for requested document.
				if (fIdleState == kWaitForDocument)
					return true;
				break;
	}
	
	fCurrentInputTime = input->time;
	
	return Layer::DispatchInput(input);
}

Boolean 
ContentView::ExecuteInput()
{
	if (!fIsVisible || fDocument == nil || fIdleState <= kWaitForDocument)
		return false;

	Displayable* current = fDocument->GetSelectable(fCurrentSelection);
	if (current == nil)
		return false;

	// handle controls...		
	if (!current->HasURL() && (current->GetImageMap() == nil || fInsideImageMap)) {
		Boolean handled = current->ExecuteInput();
		if (handled && fShowActiveSelection && fDocument != nil && current->IsSubmit())
			GetSelectionLayer()->DrawActiveNow();
		return handled;
	}
		
	// handle imagemaps (should be handled by the imagemap in the future)
	if (!fInsideImageMap && current->GetImageMap() != nil) {
		Image*	image = current->GetMappedImage();
		
		if (IsError(image == nil))
			return true;
			
		fInsideImageMap = true;

		// Center the selection position in the visible portion of the image.
		Rectangle	imageBounds;
		Rectangle	visibleImageBounds;
		Rectangle	visibleBounds;
		image->GetBoundsTight(&imageBounds);
		VisibleContentBounds(visibleBounds);
		visibleImageBounds = imageBounds;

		if (visibleImageBounds.top < visibleBounds.top)
			visibleImageBounds.top = visibleBounds.top;
		if (visibleImageBounds.bottom > visibleBounds.bottom)
			visibleImageBounds.bottom = visibleBounds.bottom;
			
		// Offset visible bounds to be relative to image bounds
		OffsetRectangle(visibleImageBounds, -imageBounds.left, -imageBounds.top);
			
		fMapCursorPosition.x = (visibleImageBounds.right + visibleImageBounds.left) / 2;
		fMapCursorPosition.y = (visibleImageBounds.bottom + visibleImageBounds.top) / 2;

		RefreshSelection();

		return true;
	}

	char*	linkURL = current->NewURL(&fMapCursorPosition, "URL");	
	if (linkURL == nil)
		return false;
	
	linkURL = ConvertURLToNewsBrowser(linkURL, "URL");

	// No link sound when typing.
	if (!gKeyboard->ContainsTypingCommand(linkURL))
		gStartLinkSound->Play();
	
	// Force the selection to draw active immediately.
	if (fShowActiveSelection)
		GetSelectionLayer()->DrawActiveNow();
		
	// Update visited for local fragments
	if (current->IsAnchor())
		((Anchor*)current)->SetVisited();
		
	ExecuteURL(linkURL, nil);
		
	FreeTaggedMemory(linkURL, "URL");		
	return true;
}

void 
ContentView::ExecuteURL(const char* url, const char* formData) 
{
	Resource* resource;
	long formDataLength;
	
	if (IsError(url == nil || *url == 0))
		return;
	
	// Hide system logo in case this was a programatic execute.
	if (gSystemLogo != nil && gSystemLogo->IsVisible())
		gSystemLogo->Hide();
		
	// Hide menu layer.
	if (gMenuLayer != nil && gMenuLayer->IsVisible())
		gMenuLayer->Hide();

	// Check for client function.
	if (ExecuteClientFunction(url, formData)) {
		RefreshSelection();
		return;
	}
	
	resource = new(Resource);
	
	if (formData != nil && *formData != 0)
		formDataLength = strlen(formData);
	else
		formDataLength = 0;

	// Check for fragments
	const char* fragment = strchr(url, '#');
	if (fragment != nil && *(++fragment) != '\0') {			
		// If url is a fragment only, its in the same document so just scroll.
		if ((fragment-1) == url) {
			ShowFragment(fragment);
			delete(resource);
			return; 
		} else {
			char* resourceName = CopyString(url, "ContentView::ExecuteURL");
			*strchr(resourceName, '#') = '\0';	// Strip fragment.
		
			resource->SetURL(resourceName, GetBaseResource(), formData, formDataLength);
			FreeTaggedMemory(resourceName, "ContentView::ExecuteURL");
			
			if (*resource == *GetResource()) {
				ShowFragment(fragment);
				delete(resource);
				return;
			}
			PostTargetFragment(fragment);			
		}
	} else {
		resource->SetURL(url, GetBaseResource(), formData, formDataLength);
	}
	
	ShowResource(resource);
	delete(resource);
}

const Resource* 
ContentView::GetBaseResource() const
{
	return fDocument != nil ? fDocument->GetBaseResource() : &fResource;
}

Displayable* 
ContentView::GetCurrentSelectable() const
{
	if (fDocument == nil)
		return nil;
		
	return fDocument->GetSelectable(fCurrentSelection);
}

Document* 
ContentView::GetDocument() const
{
	return fDocument;
}

uchar 
ContentView::GetPercentComplete() const
{
	// This is the ÒmainÓ percentage counter. It values the
	// html as a quarter of the percentage, and then the images
	// as the rest.

	uchar htmlPercent;
	uchar documentPercent = 0;

	// haven't started yet...	
	if (fStream == nil && fDocument == nil)
		return 0;

	if (fStream != nil)
		htmlPercent = fStream->GetPercentComplete(kDefaultHTMLExpectedSize);
	else
		// resource is complete already...		
		htmlPercent = 100;
		
	if (fDocument != nil && fIdleState >= kDocumentContentComplete)
		documentPercent = fDocument->GetPercentComplete();
	TrivialMessage(("Percent: HTML=%d%%, Image=%d%%", htmlPercent, documentPercent));
	
	// 1/4 for html and 3/4 for images
	if (IsError(htmlPercent > 100))
		htmlPercent = 100;
	if (IsError(documentPercent > 100))
		documentPercent = 100;
	return (htmlPercent + (3 * documentPercent) + 3) / 4;
}

const Resource* 
ContentView::GetResource() const
{
	return &fResource;
}

void 
ContentView::GetScrollBounds(Rectangle* bounds) const
{
	*bounds = fBounds;
	if (fDocument != nil)
		bounds->left += fDocument->GetNoScrollWidth();
}

ViewIdleState 
ContentView::HandleNonHTMLResource(DataType type)
{
	switch (type)
	{
		case kDataTypeGIF:
		case kDataTypeJPEG:
		case kDataTypeBitmap:
		case kDataTypeAnimation:
		case kDataTypeXBitMap:
		case kDataTypeFidoImage:
		{
			// All of these cases are handled as an embedded image in HTML.
			Resource* newResource = NewHTMLResourceForImage(&fResource);
			fResource = *newResource;
			gStatusIndicator->SetTarget(&fResource);
			gStatusIndicator->SetPercentDone(5);
			delete(newResource);
			return kWaitForDocument;
		}

		case kDataTypeMIDI:
			if (fDocument != nil) {
				char*	url = fResource.CopyURL("ContentView::HandleNonHTMLResource");
				fDocument->AddSong(url, 1);
				PlaySongSoon();
				FreeTaggedMemory(url, "ContentView::HandleNonHTMLResource");
			}
			return HandleNonShowableResource(type);
			break;
			
		case kDataTypeMPEGAudio:
		case kDataTypeRealAudioMetafile:
		case kDataTypeRealAudioProtocol:
			if (fDocument != nil) {
				char*	url = fResource.CopyURL("ContentView::HandleNonHTMLResource");
				fDocument->AddAudio(url, type);
				PlaySongSoon();
				FreeTaggedMemory(url, "ContentView::HandleNonHTMLResource");
			}
			return HandleNonShowableResource(type);
			break;

		case kDataTypeTellyScript:
			if (fResource.GetStatus() != kComplete)
				return kWaitForDocument;
			gNetwork->SetTellyScript(&fResource);
			break;
			
		case kDataTypeMIPSCode:
#ifdef FOR_MAC
			(void)IsWarning(true);
#else
			Code::NewCode(&fResource);
#endif
			return HandleNonShowableResource(type);
			break;
			
		case kDataTypePPCCode:
#ifdef __MIPSEB__
			(void)IsWarning(true);
#else
			Code::NewCode(&fResource);
#endif
			return HandleNonShowableResource(type);
			break;
			
		case kDataTypeURL:
			{
				// Redirected URL. Disable back and execute the new url.
				if (fDocument != nil)
					fDocument->SetSkipBack(true);
				DataStream*	stream = fResource.NewStream();
				if (stream != nil) {
					char*	url = CopyString(stream->GetDataAsString(), "ContentView::HandleNonHTMLResource");
					delete(stream);
					if (url != nil) {
						fResource.SetPriority((Priority)0);
						ExecuteURL(url);
						FreeTaggedMemory(url, "ContentView::HandleNonHTMLResource");
						return kWaitForDocument;
					}
				}
			}
			// Fall through to error message if we couldn't handle the redirect.
			
		default:
			ImportantMessage(("Unknown Resource type: %d", type));
			gAlertWindow->Reset();
			gAlertWindow->SetError(kCannotParse);
			gAlertWindow->SetAction(kPreviousPage);
			gAlertWindow->Show();
			DeleteStream();
	}
	
	return kViewIdle;
}

ViewIdleState 
ContentView::HandleNonShowableResource(DataType)
{
	if (fDocument != nil) {
		fResource = *fDocument->GetResource();
		fResource.SetPriority(kImmediate);
		fDocument->SetPriority(kImmediate);	
		RefreshSelection();
	}
	
	// Force back to layout complete to show phone progress until complete.
	return MIN(fLastIdleState, kDocumentLayoutComplete);
}

void 
ContentView::Idle()
{
	PerfDump perfdump("ContentView::Idle");

	PushDebugChildURL(fResource.HasURL() ? fResource.GetURL() : "<bogus resource>");
	
	// Capture the old document height to be used in drawing state changes.
	long 	oldHeight = (fDocument != nil && fIdleState > kWaitForDocument ?
						 fDocument->GetHeight() : 0);

	// Check resource status here, so that individual idlers can assume its good.
	if (fIdleState > kViewIdle && fIdleState < kDocumentContentComplete) {
		// Check for a reset stream, probably due to a redirection.
		if (fResource.GetStatus() == kStreamReset) {
			DeleteStream();
			ChangeIdleState(kWaitForDocument, fIdleState);
		}		
		else if (TrueError(fResource.GetStatus()) && fResource.GetStatus() != kTruncated) {
			gAlertWindow->SetError(fResource.GetStatus());
			gAlertWindow->SetErrorResource(&fResource);
			gAlertWindow->SetAction(kPreviousPage);
			gAlertWindow->Show();
			DeleteStream();
			ChangeIdleState(kViewIdle, fIdleState);
		}
	}
	
	ViewIdleState oldState, newState;	
	Boolean documentChanged;
	
	// If we held on to the last document, idle it.
	if (fLastDocument != nil)
		fLastDocument->Idle(this);
		
	// Idle the current song;
	if (fCurrentSong != nil)
		fCurrentSong->Idle();
	
	// Handle Document Idle
	// Continue idling the document as long as state is changing.	
	do {
		oldState = newState = fIdleState;
		documentChanged = false;
		
		// Idle the document
		if (fDocument != nil) {
			documentChanged = fDocument->Idle(this);
		}
	
		// Handle current state
		switch (fIdleState) {
			case kViewIdle:
				break;
			
			case kWaitForDocument:
				newState = IdleWaitForDocument();
				break;
			
			case kBeginDocument:
				newState = IdleBeginDocument();
				break;
			
			case kDocumentTitleComplete:
				newState = IdleDocumentTitleComplete();
				break;
			
			case kDocumentContentComplete:
				newState = IdleDocumentContentComplete(documentChanged);
				break;
				
			case kDocumentLayoutComplete:
				newState = IdleDocumentLayoutComplete();
				break;
			
			case kDocumentLoadingComplete:
				break;
		}
		
		if (oldState != newState)
			ChangeIdleState(newState, oldState);
									
	} while (oldState != newState || documentChanged);
	
	
	// Handle drawing idle if we're visible.
	if (IsVisible() && fDocument != nil) {
		long 		newHeight = (fIdleState > kWaitForDocument ? 
								 fDocument->GetHeight() : 0);
		Rectangle	visibleBounds;
		Rectangle	firstScreenBounds;
		
		gSelectionBorder->Idle();
	
		VisibleContentBounds(firstScreenBounds);
		OffsetRectangle(firstScreenBounds, -firstScreenBounds.top, 0);

		if (oldHeight != newHeight)
			DocumentHeightChanged(newHeight, oldHeight);
		
		VisibleContentBounds(visibleBounds);

		switch (fDrawIdleState) {
			case kDrawIdle:
				break;
				
			case kWaitForInitialScreen:
				// If the current page and background is complete, or we have something on the screen
				// and the allotted wait time has passed, draw the first screen.
				if ((((newHeight >= visibleBounds.bottom || fIdleState >= kDocumentLayoutComplete) && 
					  fDocument->GetVisibleImageStatus(&visibleBounds) == kComplete) ||
				     (newHeight > visibleBounds.top && Now() > (fDocumentBeginTime + kInitialScreenDrawTimeout) &&
				      fLastDocument == nil)) &&
				     fTargetFragment == nil) {
					InitialScreenLayoutCompleted();
					InvalidateBounds();
					if (SelectionsEqual(fCurrentSelection, kInvalidSelection))
						SelectFirstAnchor();
					fDrawIdleState = kWaitForFirstScreenDrawn;	
				}
				break;
				
			case kWaitForFirstScreenDrawn:				
				// If the first screen is complete, let subclassers know, so they can perform operations
				// like capturing a thumbnail.
				if (((newHeight >= firstScreenBounds.bottom || fIdleState >= kDocumentLayoutComplete) && 
					 fDocument->GetVisibleImageStatus(&firstScreenBounds) == kComplete)) {
					FirstScreenDrawingCompleted();
					fDrawIdleState = kWaitForCurrentScreen;
				}
				// Fall through to handle current screen.
							
			case kWaitForCurrentScreen: 
				if (newHeight > oldHeight && oldHeight <= visibleBounds.bottom) {
					if (newHeight > visibleBounds.bottom || fIdleState >= kDocumentLayoutComplete)
						ScreenLayoutCompleted();
					InvalidateBounds();	
					if (SelectionsEqual(fCurrentSelection, kInvalidSelection))
						SelectFirstAnchor();
				}

				if (fIdleState >= kDocumentLoadingComplete && fDrawIdleState == kWaitForCurrentScreen)
					fDrawIdleState = kDrawingComplete;
				break;
				
			case kDrawingComplete:
				// Check for request for to re-layout.
				if (fRequestedLayout != nil) {
					fDocument->Relayout();
					SetCurrentSelectableWithScroll(fRequestedLayout);
					fRequestedLayout = nil;
					InvalidateBounds();
				}
				break;
		}		
	}

	PopDebugChildURL();
}

ViewIdleState 
ContentView::IdleWaitForDocument()
{
	DataType type = fResource.GetDataType();
	DataStream*	stream = nil;

	if (type == 0)
		return kWaitForDocument;
		
	if (type != kDataTypeHTML && type != kDataTypeTEXT)
		return HandleNonHTMLResource(type);
	
	if ((stream = fResource.NewStream()) == nil)
		return kWaitForDocument;
		
	if (stream->GetDataLength() == 0) {
		delete(stream);
		return kWaitForDocument;
	}
	
	// Cleanup the old stream, builder, etc. and save the new one.		
	DeleteStream();
	fStream = stream;
	
	// If the current document has status disabled, continue to idle show this document
	// until the next document is complete.
	
	delete(fLastDocument);
	fLastDocument = nil;
	if (fDocument != nil && !fDocument->GetDisplayStatus())
		fLastDocument = fDocument;
	else {
		delete(fDocument);
		// Clear any invalidates posted by old document.
		fInvalidRegion.Reset();
	}
	fDocument = nil;
	
	// We have a resource and data, so now parse it. Create and setup the parser/builder.		

	if (type == kDataTypeHTML) {
		fParser = new(HTMLParser);
#if defined(FOR_MAC)
		((HTMLParser*)fParser)->SetResource(&fResource);
#endif
	} else
		fParser = new(PlainTextParser);
		
	fDocumentBuilder = new(DocumentBuilder);
	fParser->SetBuilder(fDocumentBuilder);
	fDocumentBuilder->SetResource(&fResource);	

	fDocument = fDocumentBuilder->GetDocument();
	fDocument->SetView(this);
	fDocument->SetPriority(kImmediate);	

	fScrollPosition = 0;
	
	fCurrentSelection = kInvalidSelection;
	fInsideImageMap = false;
	fMapCursorPosition.x = 0;
	fMapCursorPosition.y = 0;
		
	fDrawIdleState = kWaitForInitialScreen;
	fDocumentBeginTime = Now();
	
	return kBeginDocument;
}

ViewIdleState 
ContentView::IdleBeginDocument()
{
	if (IsError(fStream == nil))
		return kViewIdle;

	ParseDocument();
		
	// If we now have a title or the resource is complete, go to the  next state.
	if (fDocument->GetTitle() != nil || 
		(fStream->GetPending()  == 0 && fStream->GetStatus() == kComplete))
		return kDocumentTitleComplete;
		
	return kBeginDocument;
}


ViewIdleState 
ContentView::IdleDocumentTitleComplete()
{
	if (IsError(fStream == nil))
		return kViewIdle;

	ParseDocument();
	
	// Check if we finished content.	
	if (fStream->GetPending() == 0 && 
		(fStream->GetStatus() == kComplete || fStream->GetStatus() == kTruncated)) {
		if (fStream->GetStatus() == kTruncated)
			fDocumentBuilder->AbortAndFinalize();
		else
			fDocumentBuilder->Finalize();

		fDocument->Layout(); // Layout any pending line after finalize.
		DeleteStream();
		
		// If we're waiting on a fragment and we haven't found it yet, we
		// never will. Clear it.
		if (fTargetFragment != nil && fDocument->FindFragment(fTargetFragment) == nil) {
			FreeTaggedMemory(fTargetFragment, "ContentView::fTargetFragment");
			fTargetFragment = nil;
		}
		
		return kDocumentContentComplete;
	}
	
	return kDocumentTitleComplete;
}

ViewIdleState 
ContentView::IdleDocumentContentComplete(Boolean documentChanged)
{
	if (documentChanged)
		fDocument->Layout();
	
	return fDocument->IsLayoutCurrent() ? kDocumentLayoutComplete : kDocumentContentComplete;
}

ViewIdleState 
ContentView::IdleDocumentLayoutComplete()
{
	Error status = fDocument->GetStatus();
	
	// Wait for current song complete also.
	if (status == kComplete && fCurrentSong != nil)
		status = fCurrentSong->GetStatus();
	
	// Start reload timing when the document is completed.
	if (status == kComplete) {
		fDocument->StartReloadTiming();
		delete(fLastDocument);
		fLastDocument = nil;
		// Clear any invalidates posted by old document.
		fInvalidRegion.Reset();
		return kDocumentLoadingComplete;
	}
		
	return kDocumentLayoutComplete;
}

void 
ContentView::ChangeIdleState(ViewIdleState newState, ViewIdleState)
{
	fIdleState = newState;
}

Boolean 
ContentView::IsContentView() const
{
	return true;
}

Boolean 
ContentView::KeyboardInput(Input* input)
{
	Displayable* current;
	
	if (!fIsVisible || fDocument == nil)
		return false;
	
	current = fDocument->GetSelectable(fCurrentSelection);
	
	if (current == nil)
		return false;

	// handle text input...		
	if (current->KeyboardInput(input))
		return true;

	// default action for return is to handle as execute.
	// NOTE: we should define characters like return in a central place.
	if (input->data == 0x0d)
		return ExecuteInput();
	
	return true;
}

Boolean 
ContentView::LeftInput(Input* input)
{
	if (!fIsVisible || fDocument == nil)
		return false;
	
	SelectPreviousAnchor(true, input->modifiers & kAutoRepeat);
	
	return true;
}

Boolean
ContentView::KeyboardDownInput(Input* input)
{
	if (fDocument == nil)
		return false;
		
	Displayable* current = fDocument->GetSelectable(fCurrentSelection);

	// handle textfields.
	if (current != nil && current->MoveCursorDown())
		return true;

	return DownInput(input);
}

Boolean
ContentView::KeyboardLeftInput(Input* input)
{
	if (fDocument == nil)
		return false;

	Displayable* current = fDocument->GetSelectable(fCurrentSelection);

	// handle textfields.
	if (current != nil && current->MoveCursorLeft())
		return true;
		
	return LeftInput(input);
}

Boolean
ContentView::KeyboardRightInput(Input* input)
{
	if (fDocument == nil)
		return false;

	Displayable* current = fDocument->GetSelectable(fCurrentSelection);

	// handle textfields.
	if (current != nil && current->MoveCursorRight())
		return true;
		
	return RightInput(input);
}

Boolean
ContentView::KeyboardUpInput(Input* input)
{
	if (fDocument == nil)
		return false;
		
	Displayable* current = fDocument->GetSelectable(fCurrentSelection);

	// handle textfields.
	if (current != nil && current->MoveCursorUp())
		return true;
		
	return UpInput(input);
}

Resource* 
ContentView::NewHTMLResourceForImage(const Resource* oldResource)
{
	// Make a new HTML resource, prepending "image:" to the resource name for its name.
	char*	oldURL = oldResource->CopyURL("ContentView::NewHTMLResourceForImageOld");
	char*	newURL = NewLocalURL(oldResource, "image:", nil, "ContentView::NewHTMLResourceForImage");
	
	Resource* newResource = new(Resource);
	newResource->SetURL(newURL);
	newResource->SetDataType(kDataTypeHTML);
	newResource->SetStatus(kNoError);
	newResource->SetPriority(kImmediate);
		
	// Use the image url leaf name for title.	
	const char*	imageTitle;
	for (imageTitle = oldURL + strlen(oldURL); *(imageTitle-1) != '/' && imageTitle > oldURL; imageTitle--)
	;

	// Write the HTML that will include the image, centered.
	DataStream* stream = newResource->NewStreamForWriting();
	
	if (!IsError(stream == nil))
	{
		// Write the title
		stream->WriteString("<TITLE>");
		stream->WriteString(imageTitle);
		stream->WriteString("</TITLE>");
		
		// Open the body and center everything
		stream->WriteString("<BODY BGCOLOR=#000000 VALIGN=CENTER><CENTER>");
		
		// Write the image
		stream->WriteString("<IMG SRC=\"");
		stream->WriteString(oldURL);
		stream->WriteString("\">");
	
		// Close the body.
		stream->WriteString("<BR><BR></CENTER></BODY>\n");
	
		stream->SetStatus(kComplete);
		delete(stream);
	}
	
	FreeTaggedMemory(oldURL, "ContentView::NewHTMLResourceForImageOld");
	FreeTaggedMemory(newURL, "ContentView::NewHTMLResourceForImage");
	
	return newResource;
}

void 
ContentView::ParseDocument()
{
	if (fStream->GetPending() == 0)
		return;
		
	Rectangle	visibleBounds;
	
	VisibleContentBounds(visibleBounds);	
		
	// Parse and layout a chunk at a time as determined by the parser.
	do {
		fParser->Parse(fStream);
		fDocument->Layout();
	} while (fDocument->GetHeight() < visibleBounds.bottom && fDocument->IsLayoutCurrent() &&
		     fStream->GetPending() > 0);
}

void 
ContentView::PlaySongSoon()
{
	Song*	song = nil;

	if (fDocument != nil)
		song = fDocument->RemoveSong();
		
	if (song != nil) {
		song->AttachPreviousSong(fCurrentSong);
		fCurrentSong = song;
		fCurrentSong->PlaySoon();
	}
	else if (fCurrentSong != nil) {
		// not new one, so might as well kill old one now
		delete(fCurrentSong);
		fCurrentSong = nil;
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
ContentView::StopSong()
{
	if (fCurrentSong != nil)
		fCurrentSong->StopPlaying();
}
#endif

void 
ContentView::PostTargetFragment(const char* name)
{
	fTargetFragment = CopyStringTo(fTargetFragment, name, "ContentView::fTargetFragment");
}
	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
ContentView::PostTargetSelection(Selection selection)
{
	fTargetSelection = selection;
}
#endif

void 
ContentView::FirstScreenDrawingCompleted()
{
}

void 
ContentView::InitialScreenLayoutCompleted()
{
	ScreenLayoutCompleted();
	fDocument->StartAnimations();
}

void 
ContentView::ScreenLayoutCompleted()
{
}

void 
ContentView::SetBounds(const Rectangle* bounds)
{
	Layer::SetBounds(bounds);
	fChild->SetBounds(bounds);
}

void ContentView::SetBounds(Ordinate left, Ordinate top, Ordinate right, Ordinate bottom)
{
	Layer::SetBounds(left, top, right, bottom);
}

void 
ContentView::SetTopMargin(short margin)
{
	fTopMargin = margin;
}

Boolean 
ContentView::ShouldPaintBackground() const
{
	return fShouldPaintBackground;
}

void 
ContentView::ShowFragment(const char* fragment)
{
	ScrollToFragment(fragment);
}

void 
ContentView::ShowResource(const Resource* resource)
{
	if (IsError(resource == nil))
		return;
	
#ifdef HARDWARE
	/* last req url is updated until we crash */
	if( ((CrashLogStruct*)kCrashLogBase)->crashSig != kValidCrashLogSig )
	{
		strncpy( ((CrashLogStruct*)kCrashLogBase)->lastReqURL, resource->GetURL(), kLastReqURLBufSize-1 );
		((CrashLogStruct*)kCrashLogBase)->lastReqURL[kLastReqURLBufSize-1] = 0;
	}
#else
	#if defined(DEBUG)	
		strncpy( (char *)gLastRequestedURL, resource->GetURL(), kLastReqURLBufSize-1 );
		((char *)gLastRequestedURL)[kLastReqURLBufSize-1] = 0;
	#endif 
#endif

#if defined(DEBUG)	
	SetDebugParentURL(gLastRequestedURL);
#endif 

	FlushInput();
	
	// Deactivate current resource.
	if (fDocument != nil) {
		if (fDocument->GetPriority() > kBackground)
			fDocument->SetPriority(kBackground);
		fDocument->SetReloadTime(-1);
	} else {
		if (fResource.GetPriority() > kBackground)
			fResource.SetPriority(kBackground);
	}
	
	// Activate new resource.
	fResource = *resource;
	fResource.SetPriority(kImmediate);
	
	// Reset eny error status on this resource
	if (TrueError(fResource.GetStatus()) && fResource.GetStatus() != kTruncated)
		fResource.SetStatus(kNoError);

	fLastIdleState = fIdleState;	// Save so we can restore this if resource is not showable.	
	ChangeIdleState(kWaitForDocument, fIdleState);
	fTargetSelection = kInvalidSelection;
	fRequestedLayout = nil;
}

void 
ContentView::ReLayout()
{
	DeleteDocument();
	gOptionsPanel->ExecuteURL(kOptionsPanelURL, nil);
	ShowResource(&fResource);
}

void 
ContentView::Reload()
{
	if (fDocument != nil) {
		fDocument->ResetResources();
		DeleteDocument();
		ShowResource(&fResource);
	}
}

#ifdef SIMULATOR
void 
ContentView::ReloadAll()
{
	gRAMCache->PurgeAll();
	ReLayout();
}
#endif

void 
ContentView::RequestLayout(Displayable* requestor)
{
	if (fIdleState < kDocumentLayoutComplete || fRequestedLayout != nil)
		return;
		
	fRequestedLayout = requestor;
}

Boolean 
ContentView::RightInput(Input* input)
{
	if (!fIsVisible || fDocument == nil)
		return false;

	SelectNextAnchor(true, input->modifiers & kAutoRepeat);

	return true;
}

Boolean 
ContentView::UpInput(Input* input)
{
	if (!fIsVisible || fDocument == nil)
		return false;
	
	Displayable* current = fDocument->GetSelectable(fCurrentSelection);

	// handle scrolling lists.
	if (current != nil && current->UpInput())
		return true;

	SelectPreviousAnchor(false, input->modifiers & kAutoRepeat);
	return true;
}

void 
ContentView::VisibleBackgroundBounds(Rectangle& bounds) const
{
	// VisibleContentBounds returns content coordinates
	
	Rectangle viewBounds;
	
	GetBounds(&viewBounds);
	bounds.top = fScrollPosition;
	bounds.bottom = bounds.top + (viewBounds.bottom - viewBounds.top);	// + view height
	bounds.left = 0;
	bounds.right = (viewBounds.right - viewBounds.left);	// + view width
}

void 
ContentView::VisibleContentBounds(Rectangle& bounds) const
{
	// VisibleContentBounds returns content coordinates
	
	Rectangle viewBounds;
	
	GetBounds(&viewBounds);
	bounds.top = fScrollPosition;
	bounds.bottom = bounds.top + (viewBounds.bottom - viewBounds.top - fTopMargin);	// + view height - margin
	bounds.left = 0;
	bounds.right = (viewBounds.right - viewBounds.left);	// + view width
}

// ===========================================================================
// ContentView selection code
//

Boolean 
ContentView::IsSelected(Displayable* displayable) const
{
	Displayable* current;
	
	if (SelectionsEqual(fCurrentSelection, kInvalidSelection) || fDocument == nil)
		return false;
		
	current = (Displayable*)fDocument->GetSelectable(fCurrentSelection);
	
	return current == displayable;
}

Selection 
ContentView::NextAnchorWithScroll(Selection current, Boolean goHorizontal, Boolean autoRepeat)
{
	Rectangle	visibleBounds;
	Rectangle	selectableBounds;
	Selection	original = current;

	if (IsError(fDocument == nil))
		return kInvalidSelection;

	VisibleContentBounds(visibleBounds);

#ifdef SPATIAL_NAVIGATION
	current = goHorizontal ? NextVisibleAnchorRight(original) : NextVisibleAnchorBelow(original);
#else
	current = goHorizontal ? NextVisibleAnchor(original) : NextVisibleAnchorBelow(original);
#endif	
	if (!SelectionsEqual(current, kInvalidSelection)) {
		fDocument->GetSelectable(current)->GetBounds(&selectableBounds);
		
		// If this selection was already clipped off the top, ignore it.
		if (SelectionsEqual(original, kInvalidSelection) && selectableBounds.top < visibleBounds.top)
			current = kInvalidSelection;
			
		// Check for selectable intersecting with bottom of view.
		else {
			if (selectableBounds.bottom > (visibleBounds.bottom - kSelectionMargin)) {
				short delta = MAX(selectableBounds.bottom - visibleBounds.bottom + kSelectionMargin, kSelectionScrollIncrement);
				delta = MIN(delta, selectableBounds.top - visibleBounds.top);
				
				if (delta > 0)
					ScrollDown(delta, delta/2);
			}					
#ifdef SPATIAL_NAVIGATION
			return (goHorizontal ? NextVisibleAnchorRight(original) : NextVisibleAnchorBelow(original));
#else
			return (goHorizontal ? NextVisibleAnchor(original) : NextVisibleAnchorBelow(original));
#endif
		}
	}

#ifdef SPATIAL_NAVIGATION
	if (goHorizontal)
		return original;
#endif
			
	// Otherwise we should scroll down and then select the next visible selectable if one exists...
	long delta = (autoRepeat ? GetHeight() - fTopMargin - kPageOverlap : kSelectionScrollIncrement);
	if (ScrollDown(delta, delta/2)) {
		VisibleContentBounds(visibleBounds);	// We've scrolled, so capture the bounds again.

#ifdef SPATIAL_NAVIGATION
		current = goHorizontal ? NextVisibleAnchorRight(original) : NextVisibleAnchorBelow(original);
#else
		current = goHorizontal ? NextVisibleAnchor(original) : NextVisibleAnchorBelow(original);
#endif	
		// Check for clipped selection after scroll
		if (!SelectionsEqual(current, kInvalidSelection)) {
			fDocument->GetSelectable(current)->GetBounds(&selectableBounds);
			if (selectableBounds.bottom <= (visibleBounds.bottom - kSelectionMargin))
				return current;
		}
		// Couldn't select next, check to see if we can still select the old one.
		if (!goHorizontal && !SelectionsEqual(original, kInvalidSelection)) {
			fDocument->GetSelectable(original)->GetBounds(&selectableBounds);
			if (selectableBounds.top >= visibleBounds.top ||
				(selectableBounds.bottom - visibleBounds.top) > kSelectionScrollIncrement)
				return original;
		}
	}

	// Couldn't scroll or select original anymore, check to see if we can select one that
	// doesn't require a common parent.
	if (!goHorizontal) {
		current = NextVisibleAnchorBelow(original, false, false);

		// Check for clipped selection after scroll
		if (!SelectionsEqual(current, kInvalidSelection)) {
			fDocument->GetSelectable(current)->GetBounds(&selectableBounds);
			if (selectableBounds.bottom <= (visibleBounds.bottom - kSelectionMargin))
				return current;
		}
	}
		
	// Couldn't select next, check to see if we can still select the old one.
	if (!SelectionsEqual(original, kInvalidSelection)) {
		fDocument->GetSelectable(original)->GetBounds(&selectableBounds);
		if (selectableBounds.top >= visibleBounds.top ||
			(selectableBounds.bottom - visibleBounds.top) > kSelectionScrollIncrement)
			return original;
	}					
			
	return kInvalidSelection; // If we get here, there is nothing to select.
}

Selection 
ContentView::NextVisibleAnchor(Selection current, Boolean wrap)
{
	if (IsError(fDocument == nil))
		return kInvalidSelection;
		
	ulong count = fDocument->GetSelectableCount(current.fPageIndex);
	Selection s;

	if (count == 0)
		return kInvalidSelection;
		
	Rectangle visibleBounds;
	Rectangle selectableBounds;
	Rectangle currentBounds;
	
	VisibleContentBounds(visibleBounds);
	
	// Force search to start at zero...	
	if (current.fSelectableIndex == kInvalidSelectable) {
		current.fSelectableIndex = -1;
		SetRectangle(currentBounds, 0,0,0,0);
	}
	else
		fDocument->GetSelectable(current)->GetBounds(&currentBounds);
			
	// Look forward from the current anchor.
	s = current;
	for (s.fSelectableIndex++; s.fSelectableIndex < count; s.fSelectableIndex++) {
		fDocument->GetSelectable(s)->GetBounds(&selectableBounds);
		
		// Early exit for optimization.
		if (selectableBounds.top >= visibleBounds.bottom)
			break;
			
		if (!EmptyRectangle(&selectableBounds) && RectanglesIntersect(&selectableBounds, &visibleBounds)) {
		
			// If the new selectable is below the current one, continue to look on screen for one that is to
			// the right. This case can occur with row-spans in tables.
			long	minHeight = MIN(RectangleHeight(currentBounds),
									RectangleHeight(selectableBounds)) / 2;
		
			if ((currentBounds.bottom - selectableBounds.top) < minHeight) {
				Rectangle	selectable1Bounds;
				Selection 	s1 = s;
				for (s1.fSelectableIndex++; s1.fSelectableIndex < count; s1.fSelectableIndex++) {
					fDocument->GetSelectable(s1)->GetBounds(&selectable1Bounds);
					
					// Early exit for optimization.
					if (selectable1Bounds.top >= visibleBounds.bottom)
						break;

					minHeight = MIN(RectangleHeight(currentBounds),
									RectangleHeight(selectable1Bounds)) / 2;
									
					// If selectable is adjacent, use it instead.
					if ((currentBounds.bottom - selectable1Bounds.top) >= minHeight) {
						s = s1;
						selectableBounds = selectable1Bounds;
						break;
					}	
				}
			}
		
			if (fNoHWrapSelection && selectableBounds.left < currentBounds.left)
				return kInvalidSelection;
			return s;
		}
	}

	// Look from the beginning to the current anchor.
	if (wrap)
		for (s.fSelectableIndex = 0; s.fSelectableIndex <= current.fSelectableIndex; s.fSelectableIndex++)
		{
			fDocument->GetSelectable(s)->GetBounds(&selectableBounds);
			if (!EmptyRectangle(&selectableBounds) && RectanglesIntersect(&selectableBounds, &visibleBounds))
				return s;
		}

	return kInvalidSelection;	// no anchor onscreen
}

Selection 
ContentView::NextVisibleAnchorBelow(Selection current, Boolean wrap, Boolean requireSibling)
{
	if (IsError(fDocument == nil))
		return kInvalidSelection;

	ulong			count = fDocument->GetSelectableCount(current.fPageIndex);
	Selection		s;		
	Selection		original = current;
	Selection		bestGuess = kInvalidSelection;
	Rectangle		visibleBounds;
	Rectangle		originalBounds;
	Rectangle		originalParentBounds;
	Rectangle		guessBounds;

	if (count == 0)
		return kInvalidSelection;
		
	VisibleContentBounds(visibleBounds);
	GetBounds(&originalParentBounds);
		
	do {
		if (current.fSelectableIndex == kInvalidSelectable) {
			current.fSelectableIndex = -1;
			SetRectangle(originalBounds, 0, 0, 0, 0);
		}
		else {
			Displayable*	item = fDocument->GetSelectable(current);
			item->GetFirstRegionBounds(&originalBounds);
			Displayable*	originalParent = item->GetParent();
			if (originalParent != nil)
				originalParent->GetBounds(&originalParentBounds);
		}
		
		s = current;		
		for (s.fSelectableIndex++; s.fSelectableIndex < count; s.fSelectableIndex++) {
			Displayable* selectable = fDocument->GetSelectable(s);
			Rectangle selectableBounds;
			Rectangle selectableFullBounds;
			Rectangle selectableParentBounds;
	
			selectable->GetFirstRegionBounds(&selectableBounds);
			selectable->GetBounds(&selectableFullBounds);
	
			// Early exit for optimization.
			if (selectableFullBounds.top >= (visibleBounds.bottom + RectangleHeight(visibleBounds)))
				break;
	
			if (EmptyRectangle(&selectableFullBounds) || !RectanglesIntersect(&selectableFullBounds, &visibleBounds))
				continue;
	
			// Look for selectables that are below current and include horizontal overlap.		
			// Test for below require that the vertical overlap be less than 1/2
			// the height of the smaller item.
			long minHeight = MIN(RectangleHeight(originalBounds),
								 RectangleHeight(selectableBounds)) / 2;
			if (selectable->GetParent() != nil)
				selectable->GetParent()->GetBounds(&selectableParentBounds);
			else
				GetBounds(&selectableParentBounds);
			long parentOverlap = MIN(selectableParentBounds.right - originalParentBounds.left, 
					 				 originalParentBounds.right - selectableParentBounds.left);

			if ((originalBounds.bottom - selectableBounds.top) < minHeight &&
				( parentOverlap > 0 || !requireSibling)) {
				// If we don't have a guess yet, any selectable that
				// matches the above condition can be the bestGuess.			
				if (SelectionsEqual(bestGuess, kInvalidSelection)) {
					bestGuess = s;
					guessBounds = selectableBounds;
				}
	
				// If this selectable is at the same position vertically as our
				// guess and it has greater overlap with the horizontal position, than
				// make this the new guess.
				else {
					minHeight = MIN(RectangleHeight(guessBounds),
									RectangleHeight(selectableBounds)) / 2;
					long guessOverlap = MIN(guessBounds.right - fSelectionPosition.left,
										 	fSelectionPosition.right - guessBounds.left);
					long selectableOverlap =  MIN(selectableBounds.right - fSelectionPosition.left,
										 		  fSelectionPosition.right - selectableBounds.left);
									
					if (selectableOverlap > guessOverlap && 
					    (guessBounds.bottom - selectableBounds.top) >= minHeight) {
						bestGuess = s;
						guessBounds = selectableBounds;
					}
				}
			}
		}
		
		current.fSelectableIndex = kInvalidSelectable;

	} while (SelectionsEqual(bestGuess, kInvalidSelection) && wrap--);
	
	return bestGuess;
}

#ifdef SPATIAL_NAVIGATION
Selection 
ContentView::NextVisibleAnchorRight(Selection current, Boolean wrap)
{
	if (IsError(fDocument == nil))
		return kInvalidSelection;

	ulong			count = fDocument->GetSelectableCount(current.fPageIndex);
	Selection		s;		
	Selection		start = current;
	Selection		bestGuess = kInvalidSelection;
	Displayable*	selectable;
	Rectangle		visibleBounds;
	Rectangle		originalBounds;
	Rectangle 		selectableBounds;
	Rectangle		guessBounds;

	if (count == 0)
		return kInvalidSelection;
		
	VisibleContentBounds(visibleBounds);
	
	// We want to consider all selectables on the screen. So back up to the
	// first visible selectable.
	while (start.fSelectableIndex >= 0) {
		selectable = fDocument->GetSelectable(start);
		selectable->GetBounds(&selectableBounds);
		if (selectableBounds.bottom <= visibleBounds.top)
			break;
		start.fSelectableIndex--;
	};
		
		
	do {
		if (current.fSelectableIndex == kInvalidSelectable) {
			SetRectangle(originalBounds, 0, 0, 0, 0);
		}
		else {
			selectable = fDocument->GetSelectable(current);
			selectable->GetFirstRegionBounds(&originalBounds);
		}
		
		s = start;		
		for (s.fSelectableIndex++; s.fSelectableIndex < count; s.fSelectableIndex++) {
			selectable = fDocument->GetSelectable(s);
	
			selectable->GetFirstRegionBounds(&selectableBounds);
	
			// Early exit for optimization.
			if (selectableBounds.top >= (visibleBounds.bottom + RectangleHeight(visibleBounds)))
				break;
	
			if (EmptyRectangle(&selectableBounds) || !RectanglesIntersect(&selectableBounds, &visibleBounds))
				continue;
	
			// Look for selectables that are right of current and include vertical overlap.		

			if (selectableBounds.left >= originalBounds.right) {
				// If we don't have a guess yet, any selectable that
				// matches the above condition can be the bestGuess.			
				if (SelectionsEqual(bestGuess, kInvalidSelection)) {
					bestGuess = s;
					guessBounds = selectableBounds;
				}
	
				// If this selectable is at the same position horizontally as our
				// guess and it has greater overlap with the vertical position, than
				// make this the new guess.
				else {
					long guessOverlap = MIN(guessBounds.bottom - fSelectionPosition.top,
										 	fSelectionPosition.bottom - guessBounds.top);
					long selectableOverlap =  MIN(selectableBounds.bottom - fSelectionPosition.top,
										 		  fSelectionPosition.bottom - selectableBounds.top);
									
					if (selectableOverlap > guessOverlap && selectableBounds.left < guessBounds.right) {
						bestGuess = s;
						guessBounds = selectableBounds;
					}
				}
			}
		}
		
		current.fSelectableIndex = kInvalidSelectable;

	} while (SelectionsEqual(bestGuess, kInvalidSelection) && wrap--);
	
	return bestGuess;
}
#endif

Boolean 
ContentView::ScrollDown(long, long)
{
	return false;
}

Boolean 
ContentView::ScrollUp(long, long)
{
	return false;
}

Selection 
ContentView::PreviousAnchorWithScroll(Selection current, Boolean goHorizontal, Boolean autoRepeat)
{
	Rectangle	visibleBounds;
	Rectangle	selectableBounds;
	Selection	original = current;

	if (IsError(fDocument == nil))
		return kInvalidSelection;

	VisibleContentBounds(visibleBounds);

#ifdef SPATIAL_NAVIGATION	
	current = goHorizontal ? PreviousVisibleAnchorLeft(original) : PreviousVisibleAnchorAbove(original);
#else
	current = goHorizontal ? PreviousVisibleAnchor(original) : PreviousVisibleAnchorAbove(original);
#endif
		
	if (!SelectionsEqual(current, kInvalidSelection)) {
		fDocument->GetSelectable(current)->GetBounds(&selectableBounds);
		
		// If this selection was already clipped off the bottom, ignore it.
		if (SelectionsEqual(original, kInvalidSelection) && 
			selectableBounds.bottom > (visibleBounds.bottom - kSelectionMargin))
			current = kInvalidSelection;
			
		// Check for selectable intersecting with top of view.
		else {
			if (selectableBounds.top < visibleBounds.top) {
				long delta = MAX(visibleBounds.top - selectableBounds.top, kSelectionScrollIncrement);
				delta = MIN(delta, visibleBounds.bottom - kSelectionMargin - selectableBounds.bottom);
				
				if (delta > 0)
					ScrollUp(delta, delta/2);
			}
				
#ifdef SPATIAL_NAVIGATION	
			return (goHorizontal ? PreviousVisibleAnchorLeft(original) : PreviousVisibleAnchorAbove(original));
#else
			return (goHorizontal ? PreviousVisibleAnchor(original) : PreviousVisibleAnchorAbove(original));
#endif
		}
	}
	
#ifdef SPATIAL_NAVIGATION
	if (goHorizontal)
		return original;
#endif

	// No previous visible, try scroll up.
	long delta = (autoRepeat ? GetHeight() - fTopMargin - kPageOverlap : kSelectionScrollIncrement);
	if (ScrollUp(delta, delta/2)) {
		VisibleContentBounds(visibleBounds);	// Recapture after scroll.
			
#ifdef SPATIAL_NAVIGATION	
		current = goHorizontal ? PreviousVisibleAnchorLeft(original) : PreviousVisibleAnchorAbove(original);
#else
		current = goHorizontal ? PreviousVisibleAnchor(original) : PreviousVisibleAnchorAbove(original);
#endif				
		// Check for clipped selection after scroll
		if (!SelectionsEqual(current, kInvalidSelection)) {
			fDocument->GetSelectable(current)->GetBounds(&selectableBounds);
			if (selectableBounds.top >= visibleBounds.top)
				return current;
		}

		// Couldn't select previous, check to see if we can still select the original.
		if (!goHorizontal && !SelectionsEqual(original, kInvalidSelection)) {
			fDocument->GetSelectable(original)->GetBounds(&selectableBounds);
			if (selectableBounds.bottom <= visibleBounds.bottom ||
				(visibleBounds.bottom - selectableBounds.top) > kSelectionScrollIncrement)
				return original;
		}				
	}
	
	// Couldn't scroll or select original anymore, check to see if we can select
	// one that doesn't require a common parent.
	if (!goHorizontal) {
		current = PreviousVisibleAnchorAbove(original, false, false);
				
		// Check for clipped selection after scroll
		if (!SelectionsEqual(current, kInvalidSelection)) {
			fDocument->GetSelectable(current)->GetBounds(&selectableBounds);
			if (selectableBounds.top >= visibleBounds.top)
				return current;
		}
	}
	
	// Couldn't select previous, check to see if we can still select the original.
	if (!SelectionsEqual(original, kInvalidSelection)) {
		fDocument->GetSelectable(original)->GetBounds(&selectableBounds);
		if (selectableBounds.bottom <= visibleBounds.bottom ||
			(visibleBounds.bottom - selectableBounds.top) > kSelectionScrollIncrement)
			return original;
	}
	
	return kInvalidSelection; // If we get here, there is nothing to select.
}


Selection 
ContentView::PreviousVisibleAnchor(Selection current, Boolean wrap)
{
	if (IsError(fDocument == nil))
		return kInvalidSelection;

	ulong			count = fDocument->GetSelectableCount(current.fPageIndex);
	Selection		s; 
	Rectangle		visibleBounds;
	Rectangle		selectableBounds;
	Rectangle		currentBounds;

	if (count == 0)
		return kInvalidSelection;
		
	VisibleContentBounds(visibleBounds);

	// Force search to start at end...
	if (current.fSelectableIndex == kInvalidSelectable) {
		current.fSelectableIndex = count;
		SetRectangle(currentBounds, visibleBounds.right, visibleBounds.bottom,
									visibleBounds.right, visibleBounds.bottom);
	}
	else
		fDocument->GetSelectable(current)->GetBounds(&currentBounds);
		
	// Look backward...
	s = current;
	for (s.fSelectableIndex--; s.fSelectableIndex >= 0; s.fSelectableIndex--) {
		fDocument->GetSelectable(s)->GetBounds(&selectableBounds);

		// Early exit for optimization.
		if (selectableBounds.bottom <= visibleBounds.top)
			break;
			
		if (!EmptyRectangle(&selectableBounds) && RectanglesIntersect(&selectableBounds, &visibleBounds)) {
			// If the new selectable is above the current one, continue to look on screen for one that is to
			// the left. This case can occur with row-spans in tables.
			long	minHeight = MIN(RectangleHeight(currentBounds),
									RectangleHeight(selectableBounds)) / 2;
		
			if ((selectableBounds.bottom - currentBounds.top) < minHeight) {
				Rectangle	selectable1Bounds;
				Selection 	s1 = s;
				for (s1.fSelectableIndex--; s1.fSelectableIndex >= 0; s1.fSelectableIndex--) {
					fDocument->GetSelectable(s1)->GetBounds(&selectable1Bounds);
					
					// Early exit for optimization.
					if (selectableBounds.bottom <= visibleBounds.top)
						break;

					minHeight = MIN(RectangleHeight(currentBounds),
									RectangleHeight(selectable1Bounds)) / 2;
									
					// If selectable is adjacent, use it instead.
					if ((selectable1Bounds.bottom - currentBounds.top) >= minHeight) {
						s = s1;
						selectableBounds = selectable1Bounds;
						break;
					}	
				}
			}
		
			if (fNoHWrapSelection && selectableBounds.left > currentBounds.left)
				return kInvalidSelection;
			return s;
		}
	}

	// Wrap around...
	if (wrap)
		for (s.fSelectableIndex = count - 1; s.fSelectableIndex >= current.fSelectableIndex; s.fSelectableIndex--) {
			fDocument->GetSelectable(s)->GetBounds(&selectableBounds);
			if (!EmptyRectangle(&selectableBounds) && RectanglesIntersect(&selectableBounds, &visibleBounds))
				return s;
		}

	return kInvalidSelection;	// no anchor onscreen
}

Selection 
ContentView::PreviousVisibleAnchorAbove(Selection current, Boolean wrap, Boolean requireSibling)
{
	if (IsError(fDocument == nil))
		return kInvalidSelection;

	ulong			count = fDocument->GetSelectableCount(current.fPageIndex);
	Selection		s;
	Selection		original = current;
	Selection		bestGuess = kInvalidSelection;
	Rectangle		visibleBounds;
	Rectangle		originalBounds;
	Rectangle		originalParentBounds;
	Rectangle		guessBounds;

	if (count == 0)
		return kInvalidSelection;
		
	VisibleContentBounds(visibleBounds);
	GetBounds(&originalParentBounds);		

	do {
		if (current.fSelectableIndex == kInvalidSelectable) {
			originalBounds.top = originalBounds.bottom = visibleBounds.bottom;
			originalBounds.left = originalBounds.right = visibleBounds.left;
			current.fSelectableIndex = count;
		}
		else {
			Displayable*	item = fDocument->GetSelectable(current);
			item->GetFirstRegionBounds(&originalBounds);
			Displayable*	originalParent = item->GetParent();
			if (originalParent != nil)
				originalParent->GetBounds(&originalParentBounds);
		}
		
		// Look backward...
		s = current;
		for (s.fSelectableIndex--; s.fSelectableIndex >= 0; s.fSelectableIndex--) {
			Displayable* selectable = fDocument->GetSelectable(s);
			Rectangle selectableBounds;
			Rectangle selectableFullBounds;
			Rectangle selectableParentBounds;
	
			selectable->GetFirstRegionBounds(&selectableBounds);
			selectable->GetBounds(&selectableFullBounds);
	
			// Early exit for optimization.
			if (selectableFullBounds.bottom <= (visibleBounds.top - RectangleHeight(visibleBounds)))
				break;
	
			if (EmptyRectangle(&selectableFullBounds) || !RectanglesIntersect(&selectableFullBounds, &visibleBounds))
				continue;
				
			// Look for selectables that are above current and include horizontal overlap.
			// Test for above require that the vertical overlap be less than 1/2
			// the height of the smaller item.
			long minHeight = MIN(RectangleHeight(originalBounds),
								 RectangleHeight(selectableBounds)) / 2;
			if (selectable->GetParent() != nil)
				selectable->GetParent()->GetBounds(&selectableParentBounds);
			else
				GetBounds(&selectableParentBounds);
			long parentOverlap = MIN(selectableParentBounds.right - originalParentBounds.left, 
					 				 originalParentBounds.right - selectableParentBounds.left);

			if ((selectableBounds.bottom - originalBounds.top) < minHeight &&
			    (parentOverlap > 0 || !requireSibling)) {
				// If we don't have a guess yet, any selectable that
				// matches the above condition can be the bestGuess.			
				if (SelectionsEqual(bestGuess, kInvalidSelection)) {
					bestGuess = s;
					guessBounds = selectableBounds;
				}
	
				// If this selectable is at the same position vertically as our
				// guess and it has greater overlap with the horizontal position, than
				// make this the new guess.
				else {
					minHeight = MIN(RectangleHeight(guessBounds),
									RectangleHeight(selectableBounds)) / 2;
					long guessOverlap = MIN(guessBounds.right - fSelectionPosition.left,
										 	fSelectionPosition.right - guessBounds.left);									
					long selectableOverlap =  MIN(selectableBounds.right - fSelectionPosition.left,
										 		  fSelectionPosition.right - selectableBounds.left);
		
					if (selectableOverlap > guessOverlap && 
					    (selectableBounds.bottom - guessBounds.top) >= minHeight) {
						bestGuess = s;
						guessBounds = selectableBounds;
					}
				}
			}
		}	

		current.fSelectableIndex = kInvalidSelectable;		

	} while (SelectionsEqual(bestGuess, kInvalidSelection) && wrap--);
	
	return bestGuess;
}

#ifdef SPATIAL_NAVIGATION
Selection 
ContentView::PreviousVisibleAnchorLeft(Selection current, Boolean wrap)
{
	if (IsError(fDocument == nil))
		return kInvalidSelection;

	ulong			count = fDocument->GetSelectableCount(current.fPageIndex);
	Selection		s;		
	Selection		start = current;
	Selection		bestGuess = kInvalidSelection;
	Displayable*	selectable;
	Rectangle		visibleBounds;
	Rectangle		originalBounds;
	Rectangle 		selectableBounds;
	Rectangle		guessBounds;

	if (count == 0)
		return kInvalidSelection;
		
	VisibleContentBounds(visibleBounds);
	
	// We want to consider all selectables on the screen. So move to the
	// last visible selectable.
	while (start.fSelectableIndex < count) {
		selectable = fDocument->GetSelectable(start);
		selectable->GetBounds(&selectableBounds);
		if (selectableBounds.top >= visibleBounds.bottom)
			break;
		start.fSelectableIndex++;
	};
				
	do {
		if (current.fSelectableIndex == kInvalidSelectable) {
			SetRectangle(originalBounds, visibleBounds.right, visibleBounds.bottom,
										 visibleBounds.right, visibleBounds.bottom);
		}
		else {
			selectable = fDocument->GetSelectable(current);
			selectable->GetFirstRegionBounds(&originalBounds);
		}
		
		s = start;		
		for (s.fSelectableIndex--; s.fSelectableIndex >= 0; s.fSelectableIndex--) {
			selectable = fDocument->GetSelectable(s);
	
			selectable->GetFirstRegionBounds(&selectableBounds);
	
			// Early exit for optimization.
			if (selectableBounds.top >= (visibleBounds.bottom + RectangleHeight(visibleBounds)))
				break;
	
			if (EmptyRectangle(&selectableBounds) || !RectanglesIntersect(&selectableBounds, &visibleBounds))
				continue;
	
			// Look for selectables that are right of current and include vertical overlap.		

			if (selectableBounds.right <= originalBounds.left) {
				// If we don't have a guess yet, any selectable that
				// matches the above condition can be the bestGuess.			
				if (SelectionsEqual(bestGuess, kInvalidSelection)) {
					bestGuess = s;
					guessBounds = selectableBounds;
				}
	
				// If this selectable is at the same position horizontally as our
				// guess and it has greater overlap with the vertical position, than
				// make this the new guess.
				else {
					long guessOverlap = MIN(guessBounds.bottom - fSelectionPosition.top,
										 	fSelectionPosition.bottom - guessBounds.top);
					long selectableOverlap =  MIN(selectableBounds.bottom - fSelectionPosition.top,
										 		  fSelectionPosition.bottom - selectableBounds.top);
									
					if (selectableOverlap > guessOverlap && selectableBounds.right > guessBounds.left) {
						bestGuess = s;
						guessBounds = selectableBounds;
					}
				}
			}
		}
		
		current.fSelectableIndex = kInvalidSelectable;

	} while (SelectionsEqual(bestGuess, kInvalidSelection) && wrap--);
	
	return bestGuess;
}
#endif

void 
ContentView::RefreshSelection(Selection selection)
{
	if (fDocument == nil ||
		selection.fSelectableIndex < 0 || 
	    selection.fSelectableIndex >= fDocument->GetSelectableCount(selection.fPageIndex) ||
	    fDrawIdleState <= kWaitForInitialScreen)
		return;

	Displayable* selectable = fDocument->GetSelectable(selection);
	Rectangle contentBounds;
	Rectangle selectableBounds;

	VisibleContentBounds(contentBounds);
	selectable->GetBounds(&selectableBounds);
	if (!RectanglesIntersect(&selectableBounds, &contentBounds))
		return;
	
	gSelectionBorder->GetOuterBounds(&selectableBounds);
	ContentToScreen(&selectableBounds);
	InvalidateBounds(&selectableBounds);
	
	if (fInsideImageMap) {
		Rectangle cursorBounds = ((ImageMapSelectable*)selectable)->GetCursorBounds(&fMapCursorPosition);
		ContentToScreen(&cursorBounds);
		InvalidateBounds(&cursorBounds);
	}
}

void 
ContentView::RefreshSelection()
{
	RefreshSelection(fCurrentSelection);
}

void 
ContentView::SaveSelectionPosition()
{
	if (!SelectionsEqual(fCurrentSelection, kInvalidSelection) && !IsError(fDocument == nil)) {
		Displayable*	selected = fDocument->GetSelectable(fCurrentSelection);
		Rectangle		bounds;
		
		selected->GetFirstRegionBounds(&bounds);
		fSelectionPosition.left = bounds.left;
		fSelectionPosition.right = bounds.right;
		fSelectionPosition.top = bounds.top;
		fSelectionPosition.bottom = bounds.bottom;
	} else
		SetRectangle(fSelectionPosition, 0,0,0,0);
}

void 
ContentView::SaveSelectionHPosition()
{
	if (!SelectionsEqual(fCurrentSelection, kInvalidSelection) && !IsError(fDocument == nil)) {
		Displayable*	selected = fDocument->GetSelectable(fCurrentSelection);
		Rectangle		bounds;
		
		selected->GetFirstRegionBounds(&bounds);
		fSelectionPosition.left = bounds.left;
		fSelectionPosition.right = bounds.right;
	} else
		fSelectionPosition.left = fSelectionPosition.right = 0;
}

void 
ContentView::SaveSelectionVPosition()
{
	if (!SelectionsEqual(fCurrentSelection, kInvalidSelection) && !IsError(fDocument == nil)) {
		Displayable*	selected = fDocument->GetSelectable(fCurrentSelection);
		Rectangle		bounds;
		
		selected->GetFirstRegionBounds(&bounds);
		fSelectionPosition.top = bounds.top;
		fSelectionPosition.bottom = bounds.bottom;
	} else
		fSelectionPosition.top = fSelectionPosition.bottom = 0;
}

Boolean 
ContentView::ScrollToFragment(const char* fragmentName)
{
	if (IsError(fDocument == nil))
		return false;
	if (IsError(fragmentName == nil))
		return false;
		
	Displayable*	fragment = fDocument->FindFragment(fragmentName);
	
	if (fragment != nil && fragment->IsLayoutComplete()) {
		Rectangle		visibleBounds;

		VisibleContentBounds(visibleBounds);
		
		if (fragment->IsSelectable())
			SetCurrentSelectableWithScroll(fragment);
			
		else if (fragment->GetTop() < visibleBounds.top || 
				 fragment->GetTop() > (visibleBounds.top + visibleBounds.bottom) / 2)
			ScrollToPosition(fragment->GetTop());
		
		return true;
	}
	
	// Try well known fragments
	if (EqualString(fragmentName, kTopFragment))
		ScrollToPosition(0);
	else if (EqualString(fragmentName, kBottomFragment) && fIdleState >= kDocumentLayoutComplete)
		ScrollToPosition(MAX(0, fDocument->GetHeight() - GetHeight() - fTopMargin*2));
		
	return false;
}

void 
ContentView::ScrollToPosition(long position)
{
	if (IsError(fDocument == nil))
		return;
	if (IsWarning(position > fDocument->GetHeight()))
		return;
	
	if (fScrollPosition == position)
		return;
			
	fScrollPosition = position;
	fDocument->GetRootDisplayable()->SetTopDisplayable(nil);

	if (fDrawIdleState > kWaitForInitialScreen)
		InvalidateBounds();

	SetCurrentSelection(NextVisibleAnchor(kInvalidSelection));
	SaveSelectionPosition();
}
	
Boolean 
ContentView::ScrollToSelection(Selection selection)
{
	if (IsError(fDocument == nil))
		return false;
	if (IsError(selection.fPageIndex > 1 || selection.fSelectableIndex < 0))
		return false;
	
	if (selection.fSelectableIndex >= fDocument->GetSelectableCount(selection.fPageIndex))
		return false;
		
	Rectangle		bounds;
	Rectangle		visibleBounds;
	Displayable*	current = fDocument->GetSelectable(selection);
		
	if (current != nil)
	{
		current->GetBounds(&bounds);
		
		if (!EmptyRectangle(&bounds)) {
			VisibleContentBounds(visibleBounds);
			
			if (!RectanglesIntersect(&bounds, &visibleBounds))
				ScrollToPosition(bounds.top);
			
			SetCurrentSelection(selection);
			SaveSelectionPosition();
			return true;
		}
	}
	
	return false;
}

void 
ContentView::SelectFirstAnchor()
{
	if (IsError(fDocument == nil))
		return;

	if (!SelectionsEqual(fTargetSelection, kInvalidSelection))
		return;
	
	// First look for something explicitly selected
	Selection	s;
	for (s.fPageIndex = 0; s.fPageIndex < 2; s.fPageIndex++) {			
		long count = fDocument->GetSelectableCount(s.fPageIndex);
	
		for (s.fSelectableIndex = 0; s.fSelectableIndex < count; s.fSelectableIndex++)
			if (fDocument->GetSelectable(s)->IsInitiallySelected()) {
				SetCurrentSelection(s);
				SaveSelectionPosition();
			}
	}
		
	// If no explicit selection, try the first selection in the body
	if (SelectionsEqual(fCurrentSelection, kInvalidSelection))  {
		s.fPageIndex = 0;
		s.fSelectableIndex = kInvalidSelectable;
		SelectTopAnchor(s);
	}
		
	// If still no selection try first selection is sidebar
	if (SelectionsEqual(fCurrentSelection, kInvalidSelection)) {
		s.fPageIndex = 1;
		SelectTopAnchor(s);
	}
	
	if (fCurrentSelection.fSelectableIndex != kInvalidSelectable) {
		Displayable*	selected = fDocument->GetSelectable(fCurrentSelection);
		long			count = fDocument->GetSelectableCount(fCurrentSelection.fPageIndex);
		
		selected->SelectFirst();	
		
		// If the selection is explicity selected, or there is only one selectable, add
		// a prefetcher for the link.
		if ((count == 1 || selected->IsInitiallySelected()) && selected->HasURL() && selected->GetImageMap() == nil) {
			char* href = selected->NewURL(nil, "ContentView::SelectFirstAnchor");
			if (!EqualStringN(href, "client:", 7)) {
				DocumentPrefetcher* prefetcher = new(DocumentPrefetcher);
				prefetcher->Load(href, fDocument->GetBaseResource());
				fDocument->AddPrefetcher(prefetcher);
			}
			FreeTaggedMemory(href, "ContentView::SelectFirstAnchor");
		}
	}				
}

Selection 
ContentView::VisibleAnchorInNewPage(Selection current, long pageIndex)
{
	if (IsError(fDocument == nil))
		return kInvalidSelection;

	ulong			count = fDocument->GetSelectableCount(pageIndex);
	Selection		bestGuess = {pageIndex, kInvalidSelectable};
	Selection		s = {pageIndex, 0};
	Rectangle		visibleBounds;
	Rectangle		originalBounds;
	Rectangle		guessBounds;

	if (count == 0)
		return bestGuess;
		
	VisibleContentBounds(visibleBounds);

	if (current.fSelectableIndex == kInvalidSelectable) {
		SetRectangle(originalBounds, 0, 0, 0, 0);
		current.fSelectableIndex = -1;
	}
	else
		fDocument->GetSelectable(current)->GetFirstRegionBounds(&originalBounds);
	
	for (s.fSelectableIndex = 0; s.fSelectableIndex < count; s.fSelectableIndex++) {	
		Displayable* selectable = fDocument->GetSelectable(s);
		Rectangle selectableBounds;

		selectable->GetFirstRegionBounds(&selectableBounds);

		// Early exit for optimization.
		if (selectableBounds.top >= visibleBounds.bottom)
			break;

		if (EmptyRectangle(&selectableBounds) || !RectanglesIntersect(&selectableBounds, &visibleBounds))
			continue;

		// If we don't have a guess yet, any selectable that
		// matches the above conditions can be the bestGuess.			
		if (bestGuess.fSelectableIndex ==  kInvalidSelectable) {
			bestGuess = s;
			guessBounds = selectableBounds;
		}

		else {
			long guessOverlap = MIN(guessBounds.bottom - originalBounds.top,
								 	 originalBounds.bottom - guessBounds.top);
			long selectableOverlap =  MIN(selectableBounds.bottom - originalBounds.top,
								 	 	   originalBounds.bottom - selectableBounds.top);
							
			if (selectableOverlap > guessOverlap ||
				(selectableOverlap == guessOverlap &&
				 ABS(originalBounds.left - selectableBounds.left) < 
				 ABS(originalBounds.left - guessBounds.left))) {
				bestGuess = s;
				guessBounds = selectableBounds;
			}
		}
	}
	
	
	return bestGuess;
}

void 
ContentView::SelectNextAnchor(Boolean goHorizontal, Boolean autoRepeat)
{
	if (IsError(fDocument == nil))
		return;
		
	// Handle imagemaps.
	if (fInsideImageMap && !SelectionsEqual(fCurrentSelection, kInvalidSelection)) {
		Coordinate delta;
		delta.x = goHorizontal;
	  	delta.y = !goHorizontal;
		Displayable* current = fDocument->GetSelectable(fCurrentSelection);

		if (IsError(current == nil))
			return;
		
		if (current->BumpSelectionPosition(&fMapCursorPosition, &delta, this)) {
			return;
		} else {
			RefreshSelection();
			fInsideImageMap = false;
		}
	}
		
	Selection	newSelection;
	long		oldScroll = fScrollPosition;

	// If we're in the main content, handle normally.
	if (fCurrentSelection.fPageIndex == 0) {
		if (fShouldAutoScroll)
			newSelection = NextAnchorWithScroll(fCurrentSelection, goHorizontal, autoRepeat);
		else if (goHorizontal)
			newSelection = NextVisibleAnchor(fCurrentSelection, true);
		else
			newSelection = NextVisibleAnchorBelow(fCurrentSelection, true);
	}
	
	// Otherwise, we're in the side-bar. Check for need to leave side-bar.
	else {	
		if (goHorizontal) {	
			if (ShouldLeaveSideBar(fCurrentSelection)) {
				newSelection = VisibleAnchorInNewPage(fCurrentSelection, 0);
				if (SelectionsEqual(newSelection, kInvalidSelection))
					newSelection = fCurrentSelection;
				}
			else
				newSelection = NextVisibleAnchor(fCurrentSelection, false);
		}
		else {
			newSelection = NextVisibleAnchorBelow(fCurrentSelection, false);
			if (SelectionsEqual(newSelection, kInvalidSelection)) {
				newSelection = fCurrentSelection;
				ScrollDown(kSelectionScrollIncrement, kSelectionScrollIncrement/2);
			}
		}	
	}
	
	if (!SelectionsEqual(newSelection, fCurrentSelection))
		SetCurrentSelection(newSelection);
	else if (oldScroll == fScrollPosition)
		gLimitSound->Play();
	
	if (goHorizontal)
		SaveSelectionHPosition();
	else
		SaveSelectionVPosition();
}

void 
ContentView::SelectPreviousAnchor(Boolean goHorizontal, Boolean autoRepeat)
{
	if (IsError(fDocument == nil))
		return;
	
	// Handle imagemaps.
	if (fInsideImageMap && !SelectionsEqual(fCurrentSelection, kInvalidSelection)) {
		Coordinate	delta;
		Displayable*		current = fDocument->GetSelectable(fCurrentSelection);

		delta.x = -goHorizontal;
		delta.y = -!goHorizontal;
		
		if (current->BumpSelectionPosition(&fMapCursorPosition, &delta, this)) {
			return;
		}
		else {
			RefreshSelection();
			fInsideImageMap = false;
		}
	}
		
	Selection	newSelection;
	long oldScroll = fScrollPosition;

	// If we are in the main content
	if (fCurrentSelection.fPageIndex == 0) {
		// If there is a side-bar, see if we should go there.		
		if (fDocument->GetSelectableCount(1) > 0 && goHorizontal && ShouldEnterSideBar(fCurrentSelection))
			newSelection = VisibleAnchorInNewPage(fCurrentSelection, 1);
		else if (fShouldAutoScroll)
			newSelection = PreviousAnchorWithScroll(fCurrentSelection, goHorizontal, autoRepeat);
		else if (goHorizontal)
			newSelection = PreviousVisibleAnchor(fCurrentSelection, true);
		else
			newSelection = PreviousVisibleAnchorAbove(fCurrentSelection, true);
	}
	
	// Otherwise we're in the sidebar.
	else { 
		if (goHorizontal)
			newSelection = PreviousVisibleAnchor(fCurrentSelection, false);
		else
			newSelection = PreviousVisibleAnchorAbove(fCurrentSelection, false);
			
		if (SelectionsEqual(newSelection, kInvalidSelection)) {
			newSelection = fCurrentSelection;
			ScrollUp(kSelectionScrollIncrement, kSelectionScrollIncrement/2);
		}
	}
	
	if (!SelectionsEqual(newSelection, fCurrentSelection))	
		SetCurrentSelection(newSelection);
	else if (oldScroll == fScrollPosition)
		gLimitSound->Play();
	
	if (goHorizontal)
		SaveSelectionHPosition();
	else
		SaveSelectionVPosition();
}

void 
ContentView::SelectTopAnchor(Selection startHint)
{
	if (IsError(fDocument == nil))
		return;
	
	Rectangle	visibleBounds;	
	VisibleContentBounds(visibleBounds);

	fInsideImageMap = false;		
	
	// Find the first visible anchor whose top and bottom are not clipped.
	Selection	newSelection = startHint;
	while(!SelectionsEqual((newSelection = NextVisibleAnchor(newSelection)), kInvalidSelection)) {
		Rectangle	selectableBounds;
		fDocument->GetSelectable(newSelection)->GetBounds(&selectableBounds);
		
		if (((selectableBounds.bottom - visibleBounds.top) > kSelectionScrollIncrement ||
			 selectableBounds.top >= visibleBounds.top) &&
			((visibleBounds.bottom - selectableBounds.top) > kSelectionScrollIncrement ||
			 selectableBounds.bottom <= visibleBounds.bottom))
			break;
	}
	
	if (!SelectionsEqual(newSelection, fCurrentSelection))	
		SetCurrentSelection(newSelection);
	SaveSelectionPosition();
}

void 
ContentView::SetCurrentSelectableWithScroll(Displayable* selectable)
{
	ulong count;
	Selection s = {0, 0};
	
	if (IsError(fDocument == nil))
		return;
		
	count = fDocument->GetSelectableCount(s.fPageIndex);

	// Find the selectable. If we don't find it we still show it, but don't select it.
	
	for (s.fSelectableIndex = 0; s.fSelectableIndex < count; s.fSelectableIndex++)
		if (fDocument->GetSelectable(s) == selectable)
			break;
	
	Rectangle	selectableBounds;
	Rectangle	visibleBounds;
	short 		delta;
	
	VisibleContentBounds(visibleBounds);
	selectable->GetBounds(&selectableBounds);
	
	// For text fields, use the text bounds, rather than the field bounds
	if (selectable->IsTextField())
		((TextField*)selectable)->GetTextBounds(&selectableBounds);
		
	// If the selectable is below the bottom of the view, scroll down to it.
	if (selectableBounds.bottom > (visibleBounds.bottom - kSelectionMargin)) {

		// Try to show the entire selectable, but don't scroll past top
		delta = selectableBounds.bottom - visibleBounds.bottom + kSelectionMargin;
		delta = MIN(delta, selectableBounds.top - visibleBounds.top);
		
		if (delta > 0)
			ScrollDown(delta, delta);
	}
	
	// If the field is above the top of the view, scroll up to it.			
	else if (selectableBounds.top < visibleBounds.top &&
			 selectableBounds.bottom < (visibleBounds.bottom - kSelectionMargin)) {
		
		// Try to show the entire selectable, but don't scroll past bottom.
		delta = visibleBounds.top - selectableBounds.top;
		delta = MIN(delta, visibleBounds.bottom - selectableBounds.bottom - kSelectionMargin);
		
		if (delta > 0)
			ScrollUp(delta, delta);
	}
	
	if (s.fSelectableIndex < count) {
		SetCurrentSelection(s);
		SaveSelectionPosition();
	}
	else
		SelectTopAnchor();
}

void
ContentView::SetCurrentSelection(Selection selection)
{
	if (IsError(fDocument == nil))
		return;

	// Exit early if no change.
	if (selection.fPageIndex == fCurrentSelection.fPageIndex &&
		selection.fSelectableIndex == fCurrentSelection.fSelectableIndex)
		return;
		
	Displayable*	selectable;
				
	RefreshSelection(fCurrentSelection);
	selectable = fDocument->GetSelectable(fCurrentSelection);
	if (selectable != nil)
		selectable->Deselect();
	
	fCurrentSelection = selection;

	RefreshSelection(fCurrentSelection);	
	selectable = fDocument->GetSelectable(fCurrentSelection);
	if (selectable != nil)
		selectable->Select();
}

Boolean
ContentView::ShouldEnterSideBar(Selection current)
{
	// If we are at the first selectable, or the next selectabe is
	// above the current, we should move to the side-bar.
	
	if (IsError(fDocument == nil))
		return true;

	if (current.fSelectableIndex <= 0)
		return true;
		
	Selection		previous = current;
	Rectangle		currentBounds;
	Rectangle		previousBounds;
	
	previous.fSelectableIndex--;
	
	fDocument->GetSelectable(current)->GetBounds(&currentBounds);
	fDocument->GetSelectable(previous)->GetBounds(&previousBounds);
	
	// Look for selectables that are below current.			
	// Test for aboive require that the vertical overlap be less than 1/2
	// the height of the smaller item.
	long	minHeight = MIN(RectangleHeight(currentBounds),
							RectangleHeight(previousBounds)) / 2;

	if ((previousBounds.bottom - currentBounds.top) < minHeight)
		return true;
	
	return false;
}

Boolean
ContentView::ShouldLeaveSideBar(Selection current)
{
	// If we are on the last selectable, or the next selectable is
	// below the current, we should move to the body.
	
	if (IsError(fDocument == nil))
		return true;

	Selection		next = current;
	Rectangle		currentBounds;
	Rectangle		nextBounds;

	next.fSelectableIndex++;
	
	if (next.fSelectableIndex >= fDocument->GetSelectableCount(current.fPageIndex))
		return true;

	fDocument->GetSelectable(current)->GetBounds(&currentBounds);
	fDocument->GetSelectable(next)->GetBounds(&nextBounds);
	
	// Look for selectables that are below current.			
	// Test for below require that the vertical overlap be less than 1/2
	// the height of the smaller item.
	long	minHeight = MIN(RectangleHeight(currentBounds),
							RectangleHeight(nextBounds)) / 2;

	if ((currentBounds.bottom - nextBounds.top) < minHeight)
		return true;
	
	return false;
}

// ===========================================================================

SelectionLayer::SelectionLayer()
{
}

SelectionLayer::~SelectionLayer()
{
	if (fBackBuffer != nil)
		DeleteBitMapDevice(fBackBuffer);
	gSelectionBorder->SetVisible(false);
}

void
SelectionLayer::DeleteBackBuffer()
{
	if (fBackBuffer != nil) {
		DeleteBitMapDevice(fBackBuffer);
		fBackBuffer = nil;
	}
}

void
SelectionLayer::Initialize(ContentView* view)
{
	fView = view;
}

void
SelectionLayer::Draw(const Rectangle* invalid)
{
	if (fView->GetDocument() == nil)
		return;
		
	// Guarantee non-nil invalid for children.
	Rectangle	bounds = (invalid == nil ? fBounds : *invalid);

	if (fView->fLastDocument == nil)
		DrawSelection(fView->fCurrentSelection, fView->fShowSelection, &bounds);
}

#ifdef FIDO_INTERCEPT
void
SelectionLayer::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	if (fView->GetDocument() == nil)
		return;

	if (fView->fLastDocument == nil)
		DrawSelection(fView->fCurrentSelection, fView->fShowSelection, fidoCompatibility);
}
#endif

void 
SelectionLayer::DrawActiveNow()
{
	fActive = true;
	fView->RefreshSelection();
	gScreen->RedrawNow();
	fActive = false;
}

void 
SelectionLayer::DrawSelection(Selection selection, Boolean turnOn, const Rectangle* invalid)
{
	if (IsError(fView->fDocument == nil)) {
		return;
	}
	if ( gScreen->GetTopLayer() != fView )
		return;

	gSelectionBorder->SetVisible(false);
	
	if (selection.fSelectableIndex == kInvalidSelectable || 
		selection.fSelectableIndex >= fView->fDocument->GetSelectableCount(selection.fPageIndex)) {
		return;
	}

	if (!turnOn ) 
	{
		return;
	}
	
	Displayable* current = fView->fDocument->GetSelectable(selection);
	
	if (IsError(current == nil) || !current->IsHighlightable()) {
		return;
	}
	
	if (fView->fInsideImageMap) {
		// Draw a cursor at the center of the selectionBounds:
		Rectangle cursorBounds = ((ImageMapSelectable*)current)->GetCursorBounds(&fView->fMapCursorPosition);
		fView->ContentToScreen(&cursorBounds);
		RestoreBehind(invalid);
		SaveBehind(&cursorBounds, invalid);
		if (fActive && fView->fShowActiveSelection)
			gImageMapCursorActive->Draw(&cursorBounds, invalid);
		else	
			gImageMapCursorNormal->Draw(&cursorBounds, invalid);			
	} 
	else {
		current->GetSelectionRegion(gSelectionRegion);
		if (!gSelectionRegion->IsEmpty()) {
			fView->ContentToScreen(gSelectionRegion);

//  ¥¥¥ Change the lines below for Mark's animated selection
		
			if (fActive && fView->fShowActiveSelection)
//				gSelectionBorder->SetAnimationRange(4,7);
				gSelectionBorder->SetFrame(4);
			else
//				gSelectionBorder->SetAnimationRange(0,3);
				gSelectionBorder->SetFrame(0);
			gSelectionBorder->Draw(gSelectionRegion, invalid);
			gSelectionBorder->SetVisible(true);
#ifdef FIDO_INTERCEPT
			delete(gFidoSelectionRegion);
			gFidoSelectionRegion = gSelectionRegion->NewCopy();
#endif
				
			gSelectionRegion->Reset();
		} else {
			gSelectionBorder->SetVisible(false);
		}

		DeleteBackBuffer();
	}
}

#ifdef FIDO_INTERCEPT
void 
SelectionLayer::DrawSelection(Selection selection, Boolean turnOn, class FidoCompatibilityState& fidoCompatibility) const
{
	if (IsError(fView->fDocument == nil))
		return;

	if (selection.fSelectableIndex == kInvalidSelectable || 
		selection.fSelectableIndex >= fView->fDocument->GetSelectableCount(selection.fPageIndex))
		return;

	if (!turnOn || gScreen->GetTopLayer() != fView)
		return;
	
	Displayable* current = fView->fDocument->GetSelectable(selection);
	
	if (IsError(current == nil) || !current->IsHighlightable())
		return;
		
	if (fView->fInsideImageMap) {
//		current->DrawSelectionCursor(&fView->fMapCursorPosition, fView, fidoCompatibility);
	} else {
		if (!gFidoSelectionRegion->IsEmpty()) {
			if (fActive && fView->fShowActiveSelection)
				gSelectionBorder->SetFrame(4);
//				gSelectionBorder->SetAnimationRange(4,7);
			else
				gSelectionBorder->SetFrame(0);
//				gSelectionBorder->SetAnimationRange(0,3);
			gSelectionBorder->Draw(gFidoSelectionRegion, 0, fidoCompatibility);
		}
			
		DeleteBackBuffer();
	}
}
#endif

void
SelectionLayer::RestoreBehind(const Rectangle* invalid)
{
	if (fBackBuffer != nil)
		DrawImage(gScreenDevice, *fBackBuffer, fBackBuffer->bounds, 0, invalid);
}

void
SelectionLayer::SaveBehind(const Rectangle* bounds, const Rectangle* invalid)
{
	Rectangle	deviceBounds = *bounds;

	// Force even pixel alignment because blit routine doesn't seem to like odd alignment
	// on device.
	if (deviceBounds.left & 1)
		deviceBounds.left--;

	if (fBackBuffer != nil && !EqualRectangles(&fBackBuffer->bounds, &deviceBounds))
		DeleteBackBuffer();
			
	if (fBackBuffer == nil) {
			
		fBackBuffer = NewBitMapDevice(deviceBounds, gScreenDevice.format, 
									  gScreenDevice.colorTable, gScreenDevice.transparentColor);
		
		if (fBackBuffer == nil)
			return; 
		
		CopyImage(gScreenDevice, *fBackBuffer, fBackBuffer->bounds, fBackBuffer->bounds);
	}
	else
		CopyImage(gScreenDevice, *fBackBuffer, 
				  fBackBuffer->bounds, fBackBuffer->bounds, 0, invalid);
}

void
SelectionLayer::UpdateSavedBits(const Rectangle* drawn)
{
	if (!IsError(drawn == nil) && fBackBuffer != nil)		
		CopyImage(gScreenDevice, *fBackBuffer, 
				  fBackBuffer->bounds, fBackBuffer->bounds, 0, drawn);
}

// ===========================================================================
