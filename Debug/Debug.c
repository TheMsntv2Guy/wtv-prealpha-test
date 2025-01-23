/* ===========================================================================
	Debug.c
	
	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#include "Headers.h"

#include "CrashLogC.h"

#ifndef __CLASSES_H__
#include "Classes.h"
#endif

#ifndef __MemoryManager_H__
#include "MemoryManager.h"
#endif

#ifndef __SYSTEM_H__
#include "System.h"
#endif

/* ===========================================================================
	globals/#defines
=========================================================================== */

// name of the current url, for debugging purposes

#ifdef DEBUG
const char kBootingURLString[] = "<Booting>";
const char kNoActiveURLString[] = "<No Active URL>";

const char* gDebugParentURL = &(kBootingURLString[0]);
const char* gDebugChildURL = nil;

int		compilerStopsHereIfPostulationIsFalse[2];

#endif /* DEBUG */

#ifdef HARDWARE
const char* gLastRequestedURL = (((CrashLogStruct*)kCrashLogBase)->lastReqURL);
#else
char lastURLBuf[kLastReqURLBufSize];
const char *gLastRequestedURL = lastURLBuf;
#endif /* HARDWARE */


// =============================================================================

#ifdef DEBUG_NAMES
const char* GetErrorString(Error error)
{
	static char buf[256];
	char* name;

	switch (error) {
		case kAborted:				name = "Aborted"; break;
		case kAuthorizationError:	name = "Authorization Error"; break;
		case kBadHuffmanErr:		name = "Bad huffman code"; break;
		case kBadMarkerErr:			name = "Other JPEG marker that I don't want to know about"; break;
		case kBadSOFMarkerErr:		name = "Bad Start of Frame marker, bad JPEG data"; break;
		case kBadSOSMarkerErr:		name = "Bad Start of Scan marker, bad JPEG data"; break;
		case kCacheFull:			name = "Cache full"; break;
		case kCannotClose:			name = "Cannot close"; break;
		case kCannotListen:			name = "Cannot listen"; break;
		case kCannotOpenResolver:	name = "Cannot open resolver"; break;
		case kCannotParse:			name = "Cannot parse"; break;
		case kComplete:				name = "Complete"; break;
		case kElementNotFound:		name = "Element not found"; break;
		case kFileNotFound:			name = "File not found"; break;
		case kGenericError:			name = "Generic error"; break;
		case kHostNotFound:			name = "Host not found"; break;
		case kLineBusy:				name = "Line busy"; break;
		case kLostConnection:		name = "Lost connection"; break;
		case kLowMemory:			name = "Low Memory"; break;
		case kNoCarrier:			name = "No carrier"; break;
		case kNoConnection:			name = "No connection"; break;
		case kNoError:				name = "No error"; break;
		case kNoSOFMarkerErr:		name = "No Start of Frame marker, bad JPEG data"; break;
		case kNoSOIMarkerErr:		name = "No Start of Image marker, bad JPEG data"; break;
		case kNoSOSMarkerErr:		name = "No Start of Scan marker, bad JPEG data"; break;
		case kOutOfDataErr:			name = "Read past end of data"; break;
		case kPageNotFound:			name = "Page not found"; break;
		case kPending:				name = "Pending"; break;
		case kResourceNotFound:		name = "Resource not found"; break;
		case kResponseError:		name = "Response error"; break;
		case kStreamReset:			name = "Stream Reset"; break;
		case kTimedOut:				name = "Timed Out"; break;
		case kTooManyConnections:	name = "Too many connections"; break;
		case kTruncated:			name = "Truncated"; break;
		case kUnknownDataType:		name = "Unknown data type"; break;
		case kUnknownService:		name = "Unknown service"; break;
		default:
			snprintf(buf, sizeof(buf), "Unknown #%ld", (long)error);
			name = buf;
	}
	
	return name;
}
#endif /* DEBUG_NAMES */

#if defined(DEBUG_CLASSNUMBER) || defined(MEMORY_TRACKING)
const char* DebugClassNumberToName(ClassNumber classNum)
{
	static char buf[256];
	char* name;

	switch (classNum)
	{
		case kClassNumberAlertWindow:				name = "AlertWindow"; break;
		case kClassNumberAlignment:					name = "Alignment"; break;
		case kClassNumberAnchor:					name = "Anchor"; break;
		case kClassNumberAnchorEnd:					name = "AnchorEnd"; break;
		case kClassNumberAnchorRegion:				name = "AnchorRegion"; break;
		case kClassNumberAnimation:					name = "Animation"; break;
		case kClassNumberAnimationImage:			name = "AnimationImage"; break;
		case kClassNumberAnimationResource:			name = "AnimationResource"; break;
		case kClassNumberArea:						name = "Area"; break;
		case kClassNumberAuthorizationList:			name = "AuthorizationList"; break;
		case kClassNumberBackURL:					name = "BackURL"; break;
		case kClassNumberBitMapImage:				name = "BitMapImage"; break;
		case kClassNumberBitMapResource:			name = "BitMapResource"; break;
		case kClassNumberBorderImage:				name = "BorderImage"; break;
		case kClassNumberBorderImageResource:		name = "BorderImageResource"; break;
		case kClassNumberBuilder:					name = "Builder"; break;
		case kClassNumberBullet:					name = "Bullet"; break;
		case kClassNumberButton:					name = "Button"; break;
		case kClassNumberCRaSession:				name = "CRaSession"; break;
		case kClassNumberCache:						name = "Cache"; break;
		case kClassNumberCacheEntry:				name = "CacheEntry"; break;
		case kClassNumberCacheReader:				name = "CacheReader"; break;
		case kClassNumberCacheStream:				name = "CacheStream"; break;
		case kClassNumberCacheWriter:				name = "CacheWriter"; break;
		case kClassNumberCell:						name = "Cell"; break;
		case kClassNumberCheckBox:					name = "CheckBox"; break;
		case kClassNumberCheckbox:					name = "Checkbox"; break;
		case kClassNumberClock:						name = "Clock"; break;
		case kClassNumberClockField:				name = "ClockField"; break;
		case kClassNumberCode:						name = "Code"; break;
		case kClassNumberColorBullet:				name = "ColorBullet"; break;
		case kClassNumberColorText:					name = "ColorText"; break;
		case kClassNumberCompositeDisplayable:		name = "CompositeDisplayable"; break;
		case kClassNumberConnectIndicator:			name = "ConnectIndicator"; break;
		case kClassNumberConnectWindow:				name = "ConnectWindow"; break;
		case kClassNumberContentView:				name = "ContentView"; break;
		case kClassNumberControl:					name = "Control"; break;
		case kClassNumberCookieList:				name = "CookieList"; break;
		case kClassNumberDataIterator:				name = "DataIterator"; break;
		case kClassNumberDataList:					name = "DataList"; break;
		case kClassNumberDataStream:				name = "DataStream"; break;
		case kClassNumberDateTimeParser:			name = "DateTimeParser"; break;
		case kClassNumberDefinitionMargin:			name = "DefinitionMargin"; break;
		case kClassNumberDisplayable:				name = "Displayable"; break;
		case kClassNumberDivider:					name = "Divider"; break;
		case kClassNumberDocument:					name = "Document"; break;
		case kClassNumberDocumentBuilder:			name = "DocumentBuilder"; break;
		case kClassNumberDocumentPrefetcher:		name = "DocumentPrefetcher"; break;
		case kClassNumberEmbedded:					name = "Embedded"; break;
		case kClassNumberFidoCelArrayPool:			name = "FidoCelArrayPool"; break;
		case kClassNumberFidoCelPoolLink:			name = "FidoCelPoolLink"; break;
		case kClassNumberFidoCelRecord:				name = "FidoCelRecord"; break;
		case kClassNumberFidoClutBlockList:			name = "FidoClutBlockList"; break;
		case kClassNumberFidoCompatibilityState:	name = "FidoCompatibilityState"; break;
		case kClassNumberFidoFillBlockList:			name = "FidoFillBlockList"; break;
		case kClassNumberFidoImage:					name = "FidoImage"; break;
		case kClassNumberFidoTexture:				name = "FidoTexture"; break;
		case kClassNumberFidoYEntry:				name = "FidoYEntry"; break;
		case kClassNumberFidoYEntryContainer:		name = "FidoYEntryContainer"; break;
		case kClassNumberFidoYPoolLink:				name = "FidoYPoolLink"; break;
		case kClassNumberFileProtocol:				name = "FileProtocol"; break;
		case kClassNumberFileStream:				name = "FileStream"; break;
		case kClassNumberForegroundView:			name = "ForegroundView"; break;
		case kClassNumberForm:						name = "Form"; break;
		case kClassNumberGenericAlertWindow:		name = "GenericAlertWindow"; break;
		case kClassNumberGIF:						name = "GIF"; break;
		case kClassNumberGIFImage:					name = "GIFImage"; break;
		case kClassNumberGIFResource:				name = "GIFResource"; break;
		case kClassNumberHTMLParser:				name = "HTMLParser"; break;
		case kClassNumberHTTPCommand:				name = "HTTPCommand"; break;
		case kClassNumberHTTPGetCommand:			name = "HTTPGetCommand"; break;
		case kClassNumberHTTPPostCommand:			name = "HTTPPostCommand"; break;
		case kClassNumberHTTPProtocol:				name = "HTTPProtocol"; break;
		case kClassNumberHandlesInput:				name = "HandlesInput"; break;
		case kClassNumberHasIterator:				name = "HasIterator"; break;
		case kClassNumberImage:						name = "Image"; break;
		case kClassNumberImageData:					name = "ImageData"; break;
		case kClassNumberImageMap:					name = "ImageMap"; break;
		case kClassNumberImageMapBuilder:			name = "ImageMapBuilder"; break;
		case kClassNumberImageMapCursor:			name = "ImageMapCursor"; break;
		case kClassNumberImageMapParser:			name = "ImageMapParser"; break;
		case kClassNumberImageMapSelectable:		name = "ImageMapSelectable"; break;
		case kClassNumberInfoPanel:					name = "InfoPanel"; break;
		case kClassNumberInputHidden:				name = "InputHidden"; break;
		case kClassNumberInputImage:				name = "InputImage"; break;
		case kClassNumberInputPassword:				name = "InputPassword"; break;
		case kClassNumberIterator:					name = "Iterator"; break;
		case kClassNumberJPEGImage:					name = "JPEGImage"; break;
		case kClassNumberJPEGResource:				name = "JPEGResource"; break;
		case kClassNumberKeyboard:					name = "Keyboard"; break;
		case kClassNumberLayer:						name = "Layer"; break;
		case kClassNumberLineBreak:					name = "LineBreak"; break;
		case kClassNumberLinkable:					name = "Linkable"; break;
		case kClassNumberLoginPanel:				name = "LoginPanel"; break;
		case kClassNumberLoginProtocol:				name = "LoginProtocol"; break;
		case kClassNumberMIDI:						name = "MIDI"; break;
		case kClassNumberMPEGAudioStream:			name = "MPEGAudioStream"; break;
		case kClassNumberMacSimulator:				name = "MacSimulator"; break;
		case kClassNumberMacSocket:					name = "MacSocket"; break;
		case kClassNumberMargin:					name = "Margin"; break;
		case kClassNumberMemoryStream:				name = "MemoryStream"; break;
		case kClassNumberMenu:						name = "Menu"; break;
		case kClassNumberMenuItem:					name = "MenuItem"; break;
		case kClassNumberMenuLayer:					name = "MenuLayer"; break;
		case kClassNumberNetwork:					name = "Network"; break;
		case kClassNumberNewCelImage:				name = "NewCelImage"; break;
		case kClassNumberObjectIterator:			name = "ObjectIterator"; break;
		case kClassNumberObjectList:				name = "ObjectList"; break;
		case kClassNumberOldCelImage:				name = "OldCelImage"; break;
		case kClassNumberOptionsPanel:				name = "OptionsPanel"; break;
		case kClassNumberPage:						name = "Page"; break;
		case kClassNumberPageViewer:				name = "PageViewer"; break;
		case kClassNumberPanel:						name = "Panel"; break;
		case kClassNumberParser:					name = "Parser"; break;
		case kClassNumberPlainTextParser:			name = "PlainTextParser"; break;
		case kClassNumberPopUpMenu:					name = "PopUpMenu"; break;
		case kClassNumberPrefetchBuilder:			name = "PrefetchBuilder"; break;
		case kClassNumberProtocol:					name = "Protocol"; break;
		case kClassNumberProtocolCommand:			name = "ProtocolCommand"; break;
		case kClassNumberQueue:						name = "Queue"; break;
		case kClassNumberRadioButton:				name = "RadioButton"; break;
		case kClassNumberRC4Stream:					name = "RC4Stream"; break;
		case kClassNumberRealAudioStream:			name = "RealAudioStream"; break;
		case kClassNumberRecentPanel:				name = "RecentPanel"; break;
		case kClassNumberRecentURL:					name = "RecentURL"; break;
		case kClassNumberRegion:					name = "Region"; break;
		case kClassNumberResetButton:				name = "ResetButton"; break;
		case kClassNumberResource:					name = "Resource"; break;
		case kClassNumberResourceDataCache:			name = "ResourceDataCache"; break;
		case kClassNumberScreen:					name = "Screen"; break;
		case kClassNumberScrollingList:				name = "ScrollingList"; break;
		case kClassNumberSelectable:				name = "Selectable"; break;
		case kClassNumberSelectionLayer:			name = "SelectionLayer"; break;
		case kClassNumberSelector:					name = "Selector"; break;
		case kClassNumberService:					name = "Service"; break;
		case kClassNumberServiceList:				name = "ServiceList"; break;
		case kClassNumberServicePointer:			name = "ServicePointer"; break;
		case kClassNumberSideBar:					name = "SideBar"; break;
		case kClassNumberSocket:					name = "Socket"; break;
		case kClassNumberSong:						name = "Song"; break;
		case kClassNumberSongData:					name = "SongData"; break;
		case kClassNumberSpatialDisplayable:		name = "SpatialDisplayable"; break;
		case kClassNumberSplashWindow:				name = "SplashWindow"; break;
		case kClassNumberStatusIndicator:			name = "StatusIndicator"; break;
		case kClassNumberStoreProtocol:				name = "StoreProtocol"; break;
		case kClassNumberStoreStream:				name = "StoreStream"; break;
		case kClassNumberStream:					name = "Stream"; break;
		case kClassNumberStringDictionary:			name = "StringDictionary"; break;
		case kClassNumberStringList:				name = "StringList"; break;
		case kClassNumberSubmitButton:				name = "SubmitButton"; break;
		case kClassNumberSynthesisFilter:			name = "SynthesisFilter"; break;
		case kClassNumberSystem:					name = "System"; break;
		case kClassNumberSystemLogo:				name = "SystemLogo"; break;
		case kClassNumberTCPStream:					name = "TCPStream"; break;
		case kClassNumberTable:						name = "Table"; break;
		case kClassNumberTag:						name = "Tag"; break;
		case kClassNumberTagList:					name = "TagList"; break;
		case kClassNumberText:						name = "Text"; break;
		case kClassNumberTextField:					name = "TextField"; break;
		case kClassNumberTinySocket:				name = "TinySocket"; break;
		case kClassNumberTourist:					name = "Tourist"; break;
		case kClassNumberTouristItinerary:			name = "TouristItinerary"; break;
		case kClassNumberTouristItineraryIterator:	name = "TouristItineraryIterator"; break;
		case kClassNumberTouristJournal:			name = "TouristJournal"; break;
		case kClassNumberTouristJournalIterator:	name = "TouristJournalIterator"; break;
		case kClassNumberTouristSite:				name = "TouristSite"; break;
		case kClassNumberTouristSiteListIterator:	name = "TouristSiteListIterator"; break;
		case kClassNumberURLParser:					name = "URLParser"; break;
		case kClassNumberVectorQuantizer:			name = "VectorQuantizer"; break;
		case kClassNumberVisitedURL:				name = "VisitedURL"; break;
		case kClassNumberVisitedList:				name = "VisitedList"; break;
		case kClassNumberWTVPCommand:				name = "WTVPCommand"; break;
		case kClassNumberWTVPGetCommand:			name = "WTVPGetCommand"; break;
		case kClassNumberWTVPPostCommand:			name = "WTVPPostCommand"; break;
		case kClassNumberWTVProtocol:				name = "WTVProtocol"; break;
		case kClassNumberXBitMapImage:				name = "XBitMapImage"; break;
		case kClassNumberXPoint:					name = "XPoint"; break;
		case kClassNumberAudioScope:				name = "AudioScope"; break;
		default:
			snprintf(buf, sizeof(buf), "Unknown Class #%ld", (long)classNum);
			name = buf;
	}
	
	return name;
}
#endif /* defined(DEBUG_CLASSNUMBER) || defined(MEMORY_TRACKING) */

// =============================================================================

#ifdef DEBUG
#ifdef EXTERNAL_BUILD
Boolean	gPreventDebuggerBreaks = true; // valid even for DEBUG version
#else
Boolean	gPreventDebuggerBreaks = false; // valid even for DEBUG version
#endif
#endif

#if defined(DEBUG) && defined(SIMULATOR)
#include <stdio.h>

// ==============
static char gComplainString[256]; // use this if logging is broken
#define SafeComplain(message) 										\
	(gPreventDebuggerBreaks ? 0 : (									\
		snprintf(gComplainString, sizeof(gComplainString), 			\
					"%.*s",	sizeof(gComplainString)-1, message),	\
		c2pstr(gComplainString),									\
	 	DebugBreakStr((StringPtr)gComplainString),					\
	 	1))
// ==============

static Boolean gLogFileCreated = false;
static FILE* gLogFile = nil;
const char* kLogFileName = "WebTV Log";

FILE*
OpenLogFile(void)
{
	if (gLogFile == nil) {
		
		if (!gLogFileCreated) {
#ifdef FOR_MAC
			create(kLogFileName, 0, 'MPS ', 'TEXT');
#endif
			gLogFileCreated = true;
		}
		
		gLogFile = fopen(kLogFileName, "a");
		if (gLogFile == nil) {
			SafeComplain("Cannot open log file");
		}
	}
	return gLogFile;
}

FILE*
GetLogFile(void)
{
	return (gLogFile == nil) ? OpenLogFile() : gLogFile;	
}

void
FlushLogFile(void)
{
	if (gLogFile != nil) {
		fflush(gLogFile);
	}
}

void
CloseLogFile(void)
{
	if (gLogFile != nil) {
		fclose(gLogFile);
		gLogFile = nil;
	}
}


void
VLogMessage(const char *format, va_list list)
{
	if (!gSystem->GetLoggingEnabled())
		return;
	
	vprintf(format, list);
	FILE* fp = GetLogFile();
	if (fp != nil) {
		vfprintf(fp, format, list);
		fprintf(fp, "\n");
		FlushLogFile();
	}
}

void
VRawLogMessage(const char* format, va_list list)
{
	if (!gSystem->GetLoggingEnabled())
		return;

	FILE* fp = GetLogFile();
	if (fp != nil) {
		(void)vfprintf(fp, format, list);
		FlushLogFile();
	}
}

void
PrologMessage(const char *file, int lineNumber)
{
	if (!gSystem->GetLoggingEnabled())
		return;
	
	FILE* fp = GetLogFile();
	if (fp != nil) {
		fprintf(fp, "File %s; Line %d #\t", file, lineNumber);
		FlushLogFile();
	}
}

void
LogMessage(const char *format, ...)
{
	va_list list;
    va_start(list, format);
	VLogMessage(format, list);
	va_end(list);	
}

void
RawLogMessage(const char *format, ...)
{
	va_list list;
    va_start(list, format);
    VRawLogMessage(format, list);
	va_end(list);
}

void
PrologComplaint(const char *file, int lineNumber)
{
	gComplainString[0] =
		snprintf(&(gComplainString[1]), sizeof(gComplainString) - 1,
				 "File %s, Line %d #\t", file, lineNumber);
}


StringPtr
VPrepareComplaint(const char *format, va_list list)
{
	gComplainString[0] +=
		vsnprintf(gComplainString + gComplainString[0],
				  sizeof(gComplainString) - (gComplainString[0]+1),
				  format, 
				  list);
	return (StringPtr)gComplainString;
}

StringPtr
PrepareComplaint(const char *format, ...)
{
    va_list     list;
	StringPtr	result;

    va_start(list, format);
    result = VPrepareComplaint(format, list);
	va_end(list);
	
	return result;
}

#endif /* defined(DEBUG) && defined(SIMULATOR) */
