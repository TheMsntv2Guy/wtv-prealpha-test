// ===========================================================================
//	Tourist.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef DEBUG_TOURIST

#ifndef __ALERTWINDOW_H__
#include "AlertWindow.h"	/* for gAlertWindow */
#endif
#ifndef __CACHESTREAM_H__
#include "CacheStream.h"
#endif
#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"		/* for Document */
#endif
#ifndef __INPUT_H__
#include "Input.h"			/* for Input */
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"	/* for memory operations */
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"		/* for gPageViewer */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h" 		/* for Resource */
#endif
#ifndef __SCREEN_H__
#include "Screen.h"			/* for gScreen */
#endif
#ifndef __STREAM_H__
#include "Stream.h"			/* for Stream */
#endif
#ifndef __TOURIST_H__
#include "Tourist.h"
#endif
#ifndef __URLPARSER_H__
#include "URLParser.h"		/* for URLParser */
#endif
#ifndef __INFOPANEL_H__
#include "InfoPanel.h"
#endif
#ifndef __OPTIONSPANEL_H__
#include "OptionsPanel.h"
#endif
#ifndef __PANEL_H__
#include "Panel.h"
#endif
#ifndef __RECENTPANEL_H__
#include "RecentPanel.h"
#endif
#ifndef __SONGDATA_H__
#include "SongData.h"
#endif

Tourist* gTourist = nil;

// ===========================================================================
//	local constants, local variables

#define kHelpString				"Help"
#define kPauseString			"Pause"
#define kResumeString			"Resume"
#define kStopString				"Stop"
#define kShowString				"Show"
#define kSkipString				"Skip"
#define kHereDownString			"HereDown"
#define kMaxRetriesString		"MaxRetries"
#define kLoadTimeString			"LoadTime"
#define kMIDITimeString			"MIDITime"
#define kShowTimeString			"ShowTime"
#define kURLString				"URL"
#define kDepthString			"Depth"
#define kScrollThroughString	"ScrollThrough"
#define kMaxBufferedSitesString	"MaxBufferedSites"
#define kMaxTotalSitesString	"MaxTotalSites"
#define kMaxJournalSitesString	"MaxJournalSites"
#define kMonkeyModeString		"Monkey"

const kTouristDefaultScrollThrough	= true;
const kTouristDefaultMaxRetries		= 0;
const kTouristDefaultLoadTime		= kOneSecond * 90;
const kTouristDefaultMIDITime		= kOneSecond * 60 * 5;
const kTouristDefaultShowTime		= kOneSecond * 2;
const kTouristDefaultDepth			= 0;

const kTouristMonkeyDepth			= 32767;




static Boolean	defaultScrollThrough	= kTouristDefaultScrollThrough;
static long		defaultMaxRetries		= kTouristDefaultMaxRetries;
static long		defaultLoadTime			= kTouristDefaultLoadTime;
static long		defaultMIDITime			= kTouristDefaultMIDITime;
static long		defaultShowTime			= kTouristDefaultShowTime;
static long		defaultDepth			= kTouristDefaultDepth;




// ===========================================================================
//	start of TouristSite

TouristSite::TouristSite(void)
{
	fDepth = 0;
	fRetries = 0;
	fComment = nil;
	fURL = nil;
	fVisited = false;
}

TouristSite::~TouristSite(void)
{
	if (fComment != nil)
	{	FreeTaggedMemory(fComment, "TouristSite::fComment");
		fComment = nil;
	}
	if (fURL != nil)
	{	FreeTaggedMemory(fURL, "TouristSite::fURL");
		fURL = nil;
	}
}

TouristSite*
TouristSite::NewTouristSite(const char* url, long depth)
{
	TouristSite* site = new(TouristSite);
	if (site != nil) {
		if (url != nil) {
			site->SetURL(url);
		}
		site->SetDepth(depth);
	}
	return site;
}

void
TouristSite::DeleteTouristSite(TouristSite*& site)
{
	delete(site);
	site = nil;
}

TouristSite*
TouristSite::Duplicate()
{
	TouristSite* duplicateSite = NewTouristSite(GetURL(), GetDepth());
	if (!IsWarning(duplicateSite == nil)) {
		duplicateSite->SetComment(GetComment());
		duplicateSite->SetVisited(GetVisited());
	}
	return duplicateSite;
}

void
TouristSite::SetComment(const char* comment)
{
	if (fComment != nil) {
		FreeTaggedMemory(fComment, "TouristSite::fComment");
	}
	fComment = CopyString(comment, "TouristSite::fComment");
}

void
TouristSite::SetURL(const char* url)
{
	if (fURL != nil) {
		FreeTaggedMemory(fURL, "TouristSite::fURL");
	}
	fURL = CopyString(url, "TouristSite::fURL");
}

//	end of TouristSite
// ===========================================================================





// ===========================================================================
//	start of TouristSiteList

TouristSiteList::~TouristSiteList()
{
	while (GetCount() > 0) {
		TouristSite* site = (TouristSite*)RemoveAt(0);
		if (site == nil)
			break;
		TouristSite::DeleteTouristSite(site);
	}
}


void
TouristSiteList::Add(const Listable* item)
{
	ObjectList::Add(item);
	fTotalSites++;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean
TouristSiteList::Contains(TouristSite* site)
{
	if (site == nil)
		return false;

	return Contains(site->GetURL());
}
#endif

Boolean
TouristSiteList::Contains(const char* url)
{
	Boolean foundMatch = false;

	TouristSiteListIterator* iterator = (TouristSiteListIterator*)NewIterator();
	TouristSite* site = (TouristSite*)iterator->GetFirst();
	while (site != nil) {
		if (strcmp(site->GetURL(), url) == 0) {
			foundMatch = true;
			break;
		}
		site = (TouristSite*)iterator->GetNext();
	}
	delete(iterator);

	return foundMatch;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
TouristSiteList::DeleteAll(void)
{
	TouristSite* site;
	while ((site == (TouristSite*)RemoveAt(0)) != nil) {
		TouristSite::DeleteTouristSite(site);
	}
}
#endif

void
TouristSiteList::WriteToStream(Stream* stream)
{
	if (stream == nil)
		return;

	stream->WriteString("<TABLE>\n");
	stream->WriteString("<FONT SIZE=\"1\">\n");
	
	stream->WriteString("<CAPTION>Total sites = ");
	stream->WriteNumeric(fTotalSites);
	stream->WriteString("</CAPTION>\n");
	
	// print header
	stream->WriteString("<TH>URL</TH>");
	stream->WriteString("<TH>Depth</TH>");
	stream->WriteString("<TH>Retries</TH>");
	stream->WriteString("<TH>Comment</TH>");
	stream->WriteString("<TR>\n");
	

	TouristSiteListIterator* iterator = (TouristSiteListIterator*)NewIterator();
	if (iterator != nil) {
	
		TouristSite* site = (TouristSite*)iterator->GetFirst();
		while (site != nil) {
			
			// print URL
			stream->WriteString("<TD>");
			const char* url = site->GetURL();
			if (url != nil) {
				stream->WriteString("<A HREF=\"");
				stream->WriteString(url);
				stream->WriteString("\">");
				stream->WriteHTMLEscapedString(url);
				stream->WriteString("</A>");
			}
			
			// print depth
			stream->WriteString("<TD>");
			stream->WriteNumeric(site->GetDepth());
			
			// print retries
			stream->WriteString("<TD>");
			stream->WriteNumeric(site->GetRetries());
			
			// print comment
			stream->WriteString("<TD>");
			const char* comment = site->GetComment();
			if (comment != nil) {
				stream->WriteString(comment);
			}
			stream->WriteString("<TR>");
		
			site = (TouristSite*)iterator->GetNext();
		}
	}

	stream->WriteString("</FONT>\n");
	stream->WriteString("</TABLE>\n");
}

//	end of TouristSiteList
// ===========================================================================




// ===========================================================================
//	start of TouristJournal

void
TouristJournal::Add(const Listable* item)
{
	TouristSiteList::Add(item);

	while ((fMaxSites >= 0) && (fMaxSites < GetCount())) {
		TouristSite* deleteMe = (TouristSite*)RemoveAt(0);
		TouristSite::DeleteTouristSite(deleteMe);
	}
	
}

//	end of TouristJournal
// ===========================================================================




// ===========================================================================
//	start of TouristItinerary

void
TouristItinerary::Add(const Listable* item)
{
	if (((fMaxBufferedSites > 0) && (GetCount() >= fMaxBufferedSites))
		|| ((fMaxTotalSites > 0) && (fTotalSites >= fMaxTotalSites))) {
		TouristSite* site = (TouristSite*)item;
		TouristSite::DeleteTouristSite(site);
		return;
	}
	
	TouristSiteList::Add(item);
}

void
TouristItinerary::AddDocument(Document* document, long depth,
							  Selection selection)
{
	char* url = document->GetBaseResource()->CopyURL("TouristItinerary::AddDocument()");
	URLParser urlParser;
	urlParser.SetURL(url);
	FreeTaggedMemory(url, "TouristItinerary::AddDocument()");

	short numPages = 2;	// еее how do we know how many there are?
	while (selection.fPageIndex < numPages) {
		short numSelectables = document->GetSelectableCount(selection.fPageIndex);
		while (selection.fSelectableIndex < numSelectables) {
			const Displayable* selectable = document->GetSelectable(selection);
			if (selectable->HasURL()) {
				// Selection position centered for image maps.
				Coordinate	selectionPosition = {selectable->GetWidth()/2, selectable->GetHeight()/2};

				char* HREF = selectable->NewURL(&selectionPosition, "TouristItinerary::AddDocument()");
				if (HREF != nil) {
					char* URL = urlParser.NewURL(HREF, "TouristItinerary::AddDocument()");	
					if ((URL != nil) && (!EqualStringN(URL, "mailto:", 7))
							&& (!EqualStringN(URL, "client:Tourist", 14))) {
						if (!Contains(URL)) {
							TouristSite* site = TouristSite::NewTouristSite(URL, depth);
							if (site != nil) {
								Add(site);
							}
						}
						FreeTaggedMemory(URL, "TouristItinerary::AddDocument()");
					}
					FreeTaggedMemory(HREF, "TouristItinerary::AddDocument()");
				}
			}		
			selection.fSelectableIndex++;
		}
		selection.fPageIndex++;
	}
}

//	end of TouristItinerary
// ===========================================================================




// ===========================================================================
//	start of TouristState

#ifdef DEBUG_NAMES
const char* GetTouristStateString(TouristState touristState)
{
	const char* result = nil;
	switch (touristState)
	{
		case kTouristStateNone:				result = "None"; break;
		case kTouristStateInitialized:		result = "Initialized"; break;
		case kTouristStatePaused:			result = "Paused"; break;
		case kTouristStateWaitCompleted:	result = "WaitCompleted"; break;
		case kTouristStateScrollDown:		result = "ScrollDown"; break;
		case kTouristStateScrollUp:			result = "ScrollUp"; break;
		case kTouristStateWaitShow:			result = "WaitShow"; break;
		case kTouristStateWaitMIDI:			result = "WaitMIDI"; break;
		case kTouristStateGiveUpSite:		result = "GiveUpSite"; break;
		case kTouristStateFinishSite:		result = "FinishSite"; break;
		case kTouristStateLoad:				result = "Load"; break;
		case kTouristStateAlmostDone:		result = "Almost Done"; break;
		case kTouristStateDone:				result = "Done"; break;
		default:
		{	
			static char buffer[50];
			snprintf(buffer, sizeof(buffer), "Unrecognized TouristState %d", (int)touristState);
			result = &(buffer[0]);
		}
	}
	return result;
}
#endif /* DEBUG_NAMES */

//	end of TouristState
// ===========================================================================




// ===========================================================================
//	start of Tourist

Tourist::Tourist(void)
{
	fLoadTime = defaultLoadTime;
	fMIDITime = defaultMIDITime;
	fShowTime = defaultShowTime;

	fTouristJournal = new(TouristJournal);
	fTouristItinerary = new(TouristItinerary);
	fTouristSite = nil;
	fTouristState = kTouristStateNone;
	
	fMaxRetries = 1;
	fOldURL = nil;
	fScrollThrough = true;
}

Tourist::~Tourist(void)
{
	fTouristState = kTouristStateNone;
	
	if (fTouristJournal != nil) {
		delete(fTouristJournal);
		fTouristJournal = nil;
	}
	if (fTouristItinerary != nil) {
		delete(fTouristItinerary);
		fTouristItinerary = nil;
	}
	if (fTouristSite != nil) {
		TouristSite::DeleteTouristSite(fTouristSite);
	}
	if (fOldURL != nil) {
		FreeTaggedMemory(fOldURL, "Tourist::fOldURL");
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong Tourist::GetPageCount(void) const
{
	Assert(GetIsActive());
	return fTouristItinerary->GetCount();
}
#endif

void
Tourist::Initialize(void)
{
	if (gTourist != nil) {
		Finalize();
	}

	gTourist = new(Tourist);
}

void
Tourist::Execute(const StringDictionary* args)
{
	// if "Help" is set, print out help
	if (args->GetValue(kHelpString) != nil) {
		if ((gTourist != nil) && !gTourist->GetPaused()) {
			gTourist->Pause();
		}
		ShowHelp();
		return;
	}
	
	// if "Pause" or "Resume" or "Stop" or "Show" or "Skip" is an argument, just do it.
	if (args->GetValue(kPauseString) != nil) {
		if (!IsWarning(gTourist == nil)) {
			gTourist->Pause();
		}
		return;
	}
	
	if (args->GetValue(kResumeString) != nil) {
		if (!IsWarning(gTourist == nil)) {
			gTourist->Resume();
		}
		return;
	}
	
	if (args->GetValue(kStopString) != nil) {
		if (!IsWarning(gTourist == nil)) {
			gTourist->Stop();
		}
		return;
	}
	
	if (args->GetValue(kShowString) != nil) {
		if (!IsWarning(gTourist == nil)) {
			gTourist->Show();
			if (!gTourist->GetPaused()) {
				gTourist->Pause();
			}
		}
		return;
	}
	
	if (args->GetValue(kSkipString) != nil) {
		if (!IsWarning(gTourist == nil)) {
			gTourist->Skip();
		}
		return;
	}
	
	// set max num retries on failed sites by setting "MaxRetries" to a number
	long maxRetries = (gTourist != nil) ? gTourist->GetMaxRetries() : defaultMaxRetries;
	{	const char* maxRetriesString = args->GetValue(kMaxRetriesString);
		if (maxRetriesString != nil) {
			sscanf(maxRetriesString, "%d", &maxRetries);
		}
	}
	
	// set fScrollThrough sites by setting "ScrollThrough" to something that's doesn't start
	// with 'n' or isn't "false"
	Boolean scrollThrough = (gTourist != nil) ? gTourist->GetScrollThrough() : defaultScrollThrough;
	{	const char* scrollThroughString = args->GetValue(kScrollThroughString);
		if (scrollThroughString != nil) {
			scrollThrough = (!EqualString(scrollThroughString, "false"))
							&& (tolower(*scrollThroughString) != 'n');
		}
	}

	// set fLoadTime by setting "LoadTime=<number-of-ticks>"
	long loadTime = (gTourist != nil) ? gTourist->GetLoadTime() : defaultLoadTime;
	{	const char* loadTimeString = args->GetValue(kLoadTimeString);
		if (loadTimeString != nil) {
			sscanf(loadTimeString, "%d", &loadTime);
		}
	}
	
	// set fMIDITime by setting "MIDITime=<number-of-ticks>"
	long MIDITime = (gTourist != nil) ? gTourist->GetMIDITime() : defaultMIDITime;
	{	const char* MIDITimeString = args->GetValue(kMIDITimeString);
		if (MIDITimeString != nil) {
			sscanf(MIDITimeString, "%d", &MIDITime);
		}
	}
	
	// set fShowTime by setting "ShowTime=<number-of-ticks>"
	long showTime			= (gTourist != nil) ? gTourist->GetShowTime() : defaultShowTime;
	{	const char* showTimeString = args->GetValue(kShowTimeString);
		if (showTimeString != nil) {
			sscanf(showTimeString, "%d", &showTime);
		}
	}
	if (gTourist != nil) {
		gTourist->SetMaxRetries(maxRetries);
		gTourist->SetScrollThrough(scrollThrough);
		gTourist->SetLoadTime(loadTime);
		gTourist->SetMIDITime(MIDITime);
		gTourist->SetShowTime(showTime);
		// if we got this far and we've already got a Tourist running, exit!
		return;
	}
	
	// Assume that they're starting a new Tourist

	Tourist::Initialize();
	
	if (IsWarning(gTourist == nil)) {
		return;
	}
	
	// set maxJournalSites by setting "MaxJournalSites" to something that's doesn't start
	// with 'n' or isn't "false"
	long maxJournalSites = gTourist->GetTouristJournal()->GetMaxSites();
	{	const char* maxSitesString = args->GetValue(kMaxJournalSitesString);
		if (maxSitesString != nil) {
			sscanf(maxSitesString, "%ld", &maxJournalSites);
		}
	}

	// set max buffered sites in itinerary by setting "MaxBufferedSites" to a number
	long maxBufferedSites = gTourist->GetTouristItinerary()->GetMaxBufferedSites();
	{	const char* maxBufferedSitesString = args->GetValue(kMaxBufferedSitesString);
		if (maxBufferedSitesString != nil) {
			sscanf(maxBufferedSitesString, "%ld", &maxBufferedSites);
		}
	}

	// set max total sites in itinerary by setting "MaxTotalSites" to a number
	long maxTotalSites = (gTourist->GetTouristItinerary() != nil)
							? gTourist->GetTouristItinerary()->GetMaxTotalSites()
							: TouristItinerary::kDefaultMaxTotalSites;
	{	const char* maxTotalSitesString = args->GetValue(kMaxTotalSitesString);
		if (maxTotalSitesString != nil) {
			sscanf(maxTotalSitesString, "%ld", &maxTotalSites);
		}
	}

	// if "Monkey" is set, value of the monkey is the maximum number of sites to buffer
	// default number of buffered sites is TouristItinerary::kDefaultMaxBufferedSites 
	Boolean monkeyMode = (args->GetValue(kMonkeyModeString) != nil);
	if (monkeyMode != nil) {
		maxBufferedSites = TouristItinerary::kMonkeyMaxBufferedSites;
		maxTotalSites    = TouristItinerary::kMonkeyMaxTotalSites;
		maxJournalSites  = TouristJournal::kMonkeyMaxSites;	
	}

	gTourist->GetTouristJournal()->SetMaxSites(maxJournalSites);
	gTourist->GetTouristItinerary()->SetMaxBufferedSites(maxBufferedSites);
	gTourist->GetTouristItinerary()->SetMaxTotalSites(maxTotalSites);

	// add sites as "URL1=http://foo/&Depth1=1&URL2=http://bar/&Depth2=2" etc.
	
	TouristItinerary* itinerary = gTourist->RemoveTouristItinerary();
	int siteCount = 0;
	char buffer[256];
	const char* url;
	const char* depthString;
	long depth;
	while (true) {
		siteCount++;

		snprintf(buffer, sizeof(buffer), kURLString "%d", siteCount);
		url = args->GetValue(buffer);
		if (url == nil) {
			break;
		}
		
		snprintf(buffer, sizeof(buffer), kDepthString "%d", siteCount);
		depthString = args->GetValue(buffer);
		depth = monkeyMode ? kTouristMonkeyDepth : defaultDepth;
		if (depthString != nil) {
			sscanf(depthString, "%ld", &depth);
		}
		
		TouristSite* site = TouristSite::NewTouristSite(url, depth);
		if (site != nil) {
			itinerary->Add(site);
		}
	}
	siteCount--;
	
	url = args->GetValue(kURLString);
	depthString = args->GetValue(kDepthString);
	
	if ((url != nil) || (depthString != nil) || (siteCount == 0)) {
		
		// set up depth correctly
		depth = monkeyMode ? kTouristMonkeyDepth : defaultDepth;
		if (depthString != nil) {
			sscanf(depthString, "%ld", &depth);
		}
		
		// 
		if (url == nil) {
			Selection currSelection = {0, 0};
		
			if (gPageViewer != nil) {
				Document *document = gPageViewer->GetDocument();
				if (document != nil) {
				
					if (args->GetValue(kHereDownString)) {
						currSelection = gPageViewer->GetCurrentSelection();
					}				
					itinerary->AddDocument(document, depth, currSelection);
				}
			}
		}
		else
		{
			TouristSite* site = TouristSite::NewTouristSite(url, depth);
			if (site != nil) {
				itinerary->Add(site);
			}
		}
	}
	
	gTourist->SetLoadTime(loadTime);
	gTourist->SetMIDITime(MIDITime);
	gTourist->SetShowTime(showTime);
	gTourist->SetMaxRetries(maxRetries);
	gTourist->SetScrollThrough(scrollThrough);
	
	gTourist->SetTouristItinerary(itinerary);
	gTourist->SetTouristState(kTouristStateInitialized);
	gTourist->Start();
}

void
Tourist::Finalize()
{
	if (gTourist != nil)
		delete(gTourist);
	gTourist = nil;
}

void
Tourist::Idle()
{
	if ((gGoPanel != nil) && gGoPanel->IsOpen())
		return;
	if ((gInfoPanel != nil) && gInfoPanel->IsOpen())
		return;
	if ((gOptionsPanel != nil) && gOptionsPanel->IsOpen())
		return;
	if ((gRecentPanel != nil) && gRecentPanel->IsOpen())
		return;
	if ((gSendPanel != nil) && gSendPanel->IsOpen())
		return;

	switch (GetTouristState())
	{
		case kTouristStateNone:
			break;
		
		case kTouristStateInitialized:
			break;
		
		case kTouristStatePaused:
			//IsError(true);	// should be taken off Idle list
			break;
		
		case kTouristStateWaitCompleted:
		{
			TouristSite* site = GetTouristSite();

			if (gAlertWindow->IsVisible()) {
				gAlertWindow->Hide();
				site->SetComment(gAlertWindow->GetMessage());
				SetTouristState(kTouristStateGiveUpSite);
				break;
			}
			
			if (IsError(gPageViewer == nil))
				break;
			
			Document* document = gPageViewer->GetDocument();
			char* currURL = document->CopyURL("Tourist::Idle");
			Boolean stillOnOldPage = EqualString(currURL, fOldURL) && !EqualString(GetTouristSite()->GetURL(), fOldURL);
			if (currURL != nil)
				FreeTaggedMemory(currURL, "Tourist::Idle");
			
			if (gPageViewer->Completed()) {
				if (stillOnOldPage) {
					if (!MIDI::IsPlayListEmpty()) {
						/* must be MIDI file playing */
						SetTouristState(kTouristStateWaitMIDI);
					}
				} else {
					site->SetComment(gClock->GetTimeString());
					site->SetVisited(true);
					
					LogTouristEvent(kTouristEventArriveAtSite, site->GetURL());
	
					TouristItinerary* itinerary = RemoveTouristItinerary();
					long depth = (site == nil) ? 0 : site->GetDepth();
					
					if ((itinerary != nil) && (document != nil) && (depth > 0)) {
						Selection selection = {0,0};
						itinerary->AddDocument(document, depth-1, selection);
					}
					SetTouristItinerary(itinerary);
					if (fScrollThrough) {
						SetTouristState(kTouristStateScrollDown);
					} else {
						SetTouristState(kTouristStateWaitShow);
					}
					break;
				}
			}			
			
			
			if (Now() > fCheckpoint) {
				site->SetComment("<B>Timed out</B>");
				SetTouristState(kTouristStateGiveUpSite);
			}
			break;
		}
		case kTouristStateScrollDown:
		{
			Input input;
			input.device = kWebTVIRRemote;
			input.modifiers = 0;
			input.data = kScrollDownKey;
			
			if (!gScreen->DispatchInput(&input)) {
				SetTouristState(kTouristStateScrollUp);
			}
			break;
		}
		case kTouristStateScrollUp:
		{
			Input input;
			input.device = kWebTVIRRemote;
			input.modifiers = 0;
			input.data = kScrollUpKey;
			
			if (!gScreen->DispatchInput(&input)) {
				SetTouristState(kTouristStateWaitShow);
			}
			break;
		}
		case kTouristStateWaitShow:
		{
			if (Now() > fCheckpoint) {
				SetTouristState(kTouristStateFinishSite);
			}
			break;
		}
		case kTouristStateWaitMIDI:
		{
			if (MIDI::IsPlayListEmpty() || (Now() > fCheckpoint)) {
				SetTouristState(kTouristStateFinishSite);
			} else {
				MIDI::NoLoopingInPlayList();	// make sure that midi isn't looping on us!
			}
		}
		case kTouristStateGiveUpSite:
		{
			TouristSite* site = GetTouristSite();
			if (site != nil) {
				long numRetries = site->GetRetries();
				if (numRetries < fMaxRetries) {
					TouristItinerary* itinerary = RemoveTouristItinerary();
					if (itinerary != nil) {
						TouristSite* journalSite = site->Duplicate();
						if (journalSite != nil) {
							journalSite->SetRetries(journalSite->GetRetries() + 1);
							itinerary->Add(journalSite);
						}
						SetTouristItinerary(itinerary);
					}
				}
			}
			
			SetTouristState(kTouristStateFinishSite);
			break;
		}
		case kTouristStateFinishSite: 
			{		
				// move current site to journal
				TouristSite* site = RemoveTouristSite();
				if (site != nil) {
					TouristJournal* journal = RemoveTouristJournal();
					if (journal != nil) {
						journal->Add(site);
						SetTouristJournal(journal);
					}
				}
				SetTouristState(kTouristStateLoad);
			}
			break;
			
		case kTouristStateLoad:
			{
				TouristSite* site = GetTouristSite();
				
				if (site == nil) { // if no tour site, try to get one
					TouristItinerary* itinerary = RemoveTouristItinerary();
					if (itinerary == nil) {
						SetTouristState(kTouristStateAlmostDone);
						break;
					}
					if (itinerary->IsEmpty()) {
						SetTouristState(kTouristStateAlmostDone);
						SetTouristItinerary(itinerary);
						break;
					}
					
					site = (TouristSite*)itinerary->RemoveAt(0);
					SetTouristItinerary(itinerary);
					SetTouristSite(site);
					
					if (IsWarning(site == nil)) {
						SetTouristState(kTouristStateAlmostDone);
						break;
					}
				}
				LogTouristEvent(kTouristEventGoToSite, site->GetURL());
				
				if (fOldURL != nil) {
					FreeTaggedMemory(fOldURL, "Tourist::fOldURL");
					fOldURL = nil;
				}
				
				if (IsError(gPageViewer==nil))
					break;
				
				Document* document = gPageViewer->GetDocument();
				if (!IsError(document == nil)) {
					fOldURL = document->CopyURL("Tourist::fOldURL");
				}
				
				gPageViewer->ExecuteURL(site->GetURL(), nil);
				SetTouristState(kTouristStateWaitCompleted);
			}
			break;
		
		case kTouristStateAlmostDone:
			SetTouristState(kTouristStateDone);
			Show();
			{	// log Tourist ending tour
				if (gPageViewer == nil)
					return;
				Document* document = gPageViewer->GetDocument();
				if (document == nil)
					return;
				const Resource* resource = document->GetBaseResource();
				if (resource == nil)
					return;
				char* url = resource->CopyURL("Tourist::Finalize");
				LogTouristEvent(kTouristEventEndTour, url);
				FreeTaggedMemory(url, "Tourist::Finalize");
			}
			break;
		
		case kTouristStateDone:
			Tourist::Finalize();
			break;
	}
}

void
Tourist::Start()
{
	LogTouristEvent(kTouristEventEndTour, nil);

	if (IsWarning(GetTouristState() != kTouristStateInitialized))
		return;

	SetTouristState(kTouristStateLoad);

	{	// log Tourist starting tour
		if (gPageViewer == nil)
			return;
		Document* document = gPageViewer->GetDocument();
		if (document == nil)
			return;
		const Resource* resource = document->GetBaseResource();
		if (resource == nil)
			return;
		char* url = resource->CopyURL("Tourist::Start");
		LogTouristEvent(kTouristEventStartTour, url);
		FreeTaggedMemory(url, "Tourist::Start");
	}
}

void
Tourist::Stop()
{
	SetTouristState(kTouristStateAlmostDone);
}

void
Tourist::Pause()
{
	SetTouristState(kTouristStatePaused);
}

void
Tourist::Resume()
{
	SetTouristState(kTouristStateLoad);
}

void
Tourist::Skip()
{
	TouristState touristState = GetTouristState();
	
	if ((touristState == kTouristStateScrollDown)
	   || (touristState == kTouristStateScrollUp)) {
		SetTouristState(kTouristStateWaitShow);
	}

	if ((touristState == kTouristStateWaitCompleted)
		|| (touristState == kTouristStateWaitShow)) {
		fCheckpoint = Now();
	}
}

void
Tourist::Show()
{
	const char kTouristResultsURL[] = "cache:/Tourist?Show";

	Resource* resource = new(Resource);
	if (IsError(resource == nil))
		return;

	resource->SetURL(kTouristResultsURL);

	CacheStream* stream = resource->NewStreamForWriting();
	if (!IsError(stream == nil))
	{
		stream->SetDataType(kDataTypeHTML);
		
		stream->WriteString("<HTML>\n");
		stream->WriteString("<HEAD><TITLE>Client:Tourist?Show</TITLE></HEAD>\n");
		stream->WriteString("<BODY>\n");
		stream->WriteString("<CAPTION>Client:Tourist</CAPTION>\n");
		stream->WriteString("\n<BR>\n\n");

		TouristState touristState = gTourist->GetTouristState();

		stream->WriteString("Tourist state:  ");
#ifdef DEBUG_NAMES
		stream->WriteString(GetTouristStateString(touristState));
#else
		stream->WriteNumeric((long)touristState);
#endif
		stream->WriteString("\n<BR>\n\n");

		if (touristState != kTouristStateDone) {
			stream->WriteString("<A HREF=\"client:Tourist?" kResumeString "\">");
			stream->WriteString("&ltResume Tourist&gt");
			stream->WriteString("</A>\n");
			stream->WriteString("\n<BR>\n\n");
		}

		WriteToStream(stream);		
		
		stream->WriteString("</BODY>\n");
		stream->WriteString("</HTML>\n");

		stream->SetStatus(kComplete);
		
		gPageViewer->ExecuteURL(kTouristResultsURL);
		
		delete(stream);
	}
	delete(resource);
}

void
Tourist::ShowHelp()
{
	const char kTouristHelpURL[] = "cache:/Tourist?Help";

	Resource* resource = new(Resource);
	if (IsError(resource == nil))
		return;

	resource->SetURL(kTouristHelpURL);

	CacheStream* stream = resource->NewStreamForWriting();
	if (!IsError(stream == nil))
	{
		stream->SetDataType(kDataTypeHTML);
		
		stream->WriteString("<HTML>\n");
		stream->WriteString("<HEAD><TITLE>Client:Tourist?Help</TITLE></HEAD>\n");
		stream->WriteString("<BODY>\n");

		stream->WriteString("<B>client:Tourist</B>\n");
		stream->WriteString("(help|Help|HELP)\n");
		stream->WriteString("(" kPauseString ")\n");
		stream->WriteString("(" kResumeString ")\n");
		stream->WriteString("(" kStopString ")\n");
		stream->WriteString("(" kShowString ")\n");
		stream->WriteString("(" kSkipString ")\n");
		stream->WriteString("(" kHereDownString ")\n");
		stream->WriteString("(" kURLString "n=<I>URL</I>)\n");
		stream->WriteString("(" kDepthString "=<I>num</I>)\n");
		stream->WriteString("(" kURLString "n=<I>URL</I> [" kDepthString "n=<I>num</I>])\n");
		stream->WriteString("(" kMaxRetriesString "=<I>num</I>)\n");
		stream->WriteString("(" kScrollThroughString "={true/false})\n");
		stream->WriteString("(" kLoadTimeString "=<I>ticks</I>)\n");
		stream->WriteString("(" kMIDITimeString "=<I>ticks</I>)\n");
		stream->WriteString("(" kShowTimeString "=<I>ticks</I>)\n");
		stream->WriteString("<BR>");
		stream->WriteString("<UL>");
		stream->WriteString("<LI><B>" kHelpString "</B> - Show this page\n");
		stream->WriteString("<LI><B>" kPauseString "</B> - Pause the Tourist\n");
		stream->WriteString("<LI><B>" kResumeString "</B> - Resume the Tourist\n");
		stream->WriteString("<LI><B>" kStopString "</B> - Stop the Tourist\n");
		stream->WriteString("<LI><B>" kSkipString "</B> - Go on to the next page in the itinerary.");
		stream->WriteString("<LI><B>" kHereDownString "</B> - start from the current selection and "
							"only do those selections beyond this point on the page.");
		stream->WriteString("<LI><B>" kShowString "</B> - Show the current state of the Tourist. "
							"(Note: implicitly pauses the Tourist)\n");

		stream->WriteString("<LI><B>" kURLString "=<I>URL</I></B> - root URL to search.  If omitted,"
							"defaults to current page");

		stream->WriteString("<LI><B>" kDepthString "=<I>num</I></B> - depth to search root URL");

		stream->WriteString("<LI><B>" kMaxRetriesString "=<I>num</I></B> - max num times to retry "
							"document that failed to load (default is ");
		stream->WriteNumeric(defaultMaxRetries);
		stream->WriteString(")\n");

		stream->WriteString("<LI><B>" kScrollThroughString "=<I>true/false</I></B> - scroll down "
							"and back up through document once it comes in (default is ");
		stream->WriteString(defaultScrollThrough ? "true" : "false");
		stream->WriteString(")\n");
		
		stream->WriteString("<LI><B>" kLoadTimeString "=<I>num</I></B> - amount of time (in ticks) "
							"to wait for the document to load before timing out (default is ");
		stream->WriteNumeric(defaultLoadTime);
		stream->WriteString(")\n");
		
		stream->WriteString("<LI><B>" kMIDITimeString "=<I>num</I></B> - amount of time (in ticks) "
							"to wait for MIDI songs to play before timing out (default is ");
		stream->WriteNumeric(defaultMIDITime);
		stream->WriteString(")\n");
		
		stream->WriteString("<LI><B>" kShowTimeString "=<I>num</I></B> - amount of time (in ticks) "
							"to idle and just look at the document after it comes in (default is ");
		stream->WriteNumeric(defaultShowTime);
		stream->WriteString(")\n");
		
		stream->WriteString("</UL>");
		stream->WriteString("</BODY>\n");
		stream->WriteString("</HTML>\n");

		stream->SetStatus(kComplete);
		
		gPageViewer->ExecuteURL(kTouristHelpURL);
		
		delete(stream);
	}
	delete(resource);
}

void
Tourist::WriteToStream(Stream* stream)
{
	TouristItinerary* itinerary = GetTouristItinerary();
	if ((itinerary != nil) && (itinerary->GetCount() > 0)) {
		stream->WriteString("<BR>Itinerary of sites yet to visit<BR>");
		itinerary->WriteToStream(stream);
	}

	TouristJournal* journal = GetTouristJournal();
	if (journal != nil) {
		stream->WriteString("<BR>The most recent sites Tourist has visited:<BR>");
		journal->WriteToStream(stream);
	}
}

// ============= Getters and Setters (and a few Removes for good luck)

#ifdef DEBUG_TOURISTWINDOW
long
Tourist::GetDefaultDepth()
{
	return defaultDepth;
}
#endif

#ifdef DEBUG_TOURISTWINDOW
long
Tourist::GetDefaultLoadTime()
{
	return defaultLoadTime;
}
#endif

#ifdef DEBUG_TOURISTWINDOW
long
Tourist::GetDefaultMaxRetries()
{
	return defaultMaxRetries;
}
#endif

#ifdef DEBUG_TOURISTWINDOW
long
Tourist::GetDefaultMIDITime()
{
	return defaultMIDITime;
}
#endif

#ifdef DEBUG_TOURISTWINDOW
Boolean
Tourist::GetDefaultScrollThrough()
{
	return defaultScrollThrough;
}
#endif

#ifdef DEBUG_TOURISTWINDOW
long
Tourist::GetDefaultShowTime()
{
	return defaultShowTime;
}
#endif

TouristItinerary*
Tourist::RemoveTouristItinerary()
{
	SetDebugModifiedTime();
	TouristItinerary* itinerary = fTouristItinerary;
	fTouristItinerary = nil;
	return itinerary;
}

TouristJournal*
Tourist::RemoveTouristJournal()
{
	SetDebugModifiedTime();
	TouristJournal* journal = fTouristJournal;
	fTouristJournal = nil;
	return journal;
}

TouristSite*
Tourist::RemoveTouristSite()
{
	SetDebugModifiedTime();
	TouristSite* site = fTouristSite;
	fTouristSite = nil;
	return site;
}

#ifdef DEBUG_TOURISTWINDOW
void
Tourist::SetDefaultDepth(long depth)
{
	defaultDepth = depth;
}

void
Tourist::SetDefaultLoadTime(long loadTime)
{
	defaultLoadTime = loadTime;
}

void
Tourist::SetDefaultMaxRetries(long maxRetries)
{
	defaultMaxRetries = maxRetries;
}

void
Tourist::SetDefaultMIDITime(long MIDITime)
{
	defaultMIDITime = MIDITime;
}

void
Tourist::SetDefaultScrollThrough(Boolean scrollThrough)
{
	defaultScrollThrough = scrollThrough;
}

void
Tourist::SetDefaultShowTime(long showTime)
{
	defaultShowTime = showTime;
}
#endif

void
Tourist::SetLoadTime(long loadTime)
{
	SetDebugModifiedTime();
	fLoadTime = loadTime;
}

void
Tourist::SetMaxRetries(long retries)
{
	SetDebugModifiedTime();
	fMaxRetries = retries;
}

void
Tourist::SetMIDITime(long MIDITime)
{
	SetDebugModifiedTime();
	fMIDITime = MIDITime;
}

void
Tourist::SetScrollThrough(Boolean flag)
{
	SetDebugModifiedTime();
	fScrollThrough = flag;
}

void
Tourist::SetShowTime(long showTime)
{
	SetDebugModifiedTime();
	fShowTime = showTime;
}

void
Tourist::SetTouristItinerary(TouristItinerary* itinerary)
{
	SetDebugModifiedTime();
	fTouristItinerary = itinerary;
}

void
Tourist::SetTouristJournal(TouristJournal* journal)
{
	SetDebugModifiedTime();
	fTouristJournal = journal;
}

void
Tourist::SetTouristSite(TouristSite* site)
{
	SetDebugModifiedTime();
	fTouristSite = site;
}

void
Tourist::SetTouristState(TouristState state)
{
	SetDebugModifiedTime();
	fTouristState = state;
	switch (fTouristState)
	{
		case kTouristStateWaitCompleted:
			fCheckpoint = Now() + fLoadTime;
			break;
		case kTouristStateWaitMIDI:
			fCheckpoint = Now() + fMIDITime;
			break;
		case kTouristStateWaitShow:
			fCheckpoint = Now() + fShowTime;
			break;
		default:
			break;
	}
}

//	end of Tourist
// ===========================================================================

#endif /* DEBUG_TOURIST */
