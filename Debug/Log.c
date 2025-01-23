#include "Headers.h"

#ifdef HARDWARE
#include "CrashLogC.h"
#endif

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif

#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

#ifndef __STREAM_H__
#include "Stream.h"
#endif

#ifndef __SYSTEM_H__
#include "System.h"
#endif

// =============================================================================

void
LogError(const char* message, const char* file, long line)
{
	Complain(("%s (%s %ld)", message, file, line));
	SendLog(kLogError, message, strlen(message), file, line);
}

void
LogWarning(const char* USED_FOR_DEBUG(message), const char* USED_FOR_DEBUG(file), long USED_FOR_DEBUG(line))
{
	ImportantMessage(("%s (%s %ld)", message, file, line));
#if defined(DEBUG)
	SendLog(kLogWarning, message, strlen(message), file, line);
#endif
}

void
SendLog(const char* type, const char* message, long length, const char* file, long line)
{
	static sendingLog = false;
	const char* logURL;
	MemoryStream* stream;
	Resource resource;
	
	if (sendingLog || (logURL = gSystem->GetLogURL()) == nil)
		return;

	sendingLog = true;

	stream = new(MemoryStream);
	stream->WriteString(logURL);
	stream->WriteString("?");
	stream->WriteQuery("type", type);

#ifdef DEBUG
	if (gLastRequestedURL != nil)
		stream->WriteQuery("url", gLastRequestedURL);
#endif

	if (file != nil) {
		stream->WriteQuery("file", file);
		stream->WriteQuery("line", line);
	}
	
	resource.SetURL(stream->GetDataAsString(), message, length);
	resource.SetPriority(kBackground);
	resource.SetStatus(kNoError);
	
	delete(stream);
	sendingLog = false;
}

// =============================================================================

struct ModemEventNameLookup
{
	ModemEvent 	modemEvent;
	const char*	modemEventName;
};

static ModemEventNameLookup gModemEventNameLookup[] =
{
	{ kModemEventDialin, "Dialin" },
	{ kModemEventHangup, "Hangup" }
};

void
LogModemEvent(ModemEvent modemEvent)
{
	const char* modemEventName = "UnknownModemEvent";
	
	for (int i=0; i<sizeof(gModemEventNameLookup)/sizeof(gModemEventNameLookup[0]); i++) {
		if (gModemEventNameLookup[i].modemEvent == modemEvent) {
			modemEventName = gModemEventNameLookup[i].modemEventName;
			break;
		}
	}

	char message[128];
	long messageLength = snprintf(message, sizeof(message), "ModemEvent:%s", modemEventName);	
	SendLog("ModemEvent", message, messageLength, nil, 0);
}

// =============================================================================

void
LogCrash(void)
{
#ifdef HARDWARE
	if( ((CrashLogStruct*)kCrashLogBase)->crashSig == kValidCrashLogSig )
	{
		Message(("Sending up crash log"));
		SendLog("CrashEvent", (const char *)kCrashLogBase, sizeof(CrashLogStruct), nil, 0);
		
		((CrashLogStruct*)kCrashLogBase)->crashSig = 0;
	}
#endif
}

// =============================================================================

#ifdef DEBUG_TOURIST

struct TouristEventNameLookup
{
	TouristEvent	touristEvent;
	const char*		touristEventName;
};

static TouristEventNameLookup gTouristEventNameLookup[] =
{
	{ kTouristEventStartTour, "StartTour" },
	{ kTouristEventGoToSite, "GoToSite" },
	{ kTouristEventArriveAtSite, "ArriveAtSite" },
	{ kTouristEventEndTour, "EndTour" }
};

void
LogTouristEvent(TouristEvent touristEvent, const char* url)
{
	const char* touristEventName = "UnknownTouristEvent";
	
	for (int i=0; i<sizeof(gTouristEventNameLookup)/sizeof(gTouristEventNameLookup[0]); i++) {
		if (gTouristEventNameLookup[i].touristEvent == touristEvent) {
			touristEventName = gTouristEventNameLookup[i].touristEventName;
			break;
		}
	}
	
	if (url == nil)
		url = "<nil>";
	
	const char kFormatString[] = "Tourist:%s,%s";
	long messageSize = strlen(touristEventName) + sizeof(kFormatString) + strlen(url);
	
	char* message = (char*)AllocateTaggedMemory(messageSize, "LogTouristEvent");
	char tinyBuffer[128];	// in case allocate fails, here's SOMETHING
	if (message == nil) {
		message = &(tinyBuffer[0]);
		messageSize = sizeof(tinyBuffer);
	}
	messageSize = snprintf(message, messageSize, kFormatString, touristEventName, url);
	
	SendLog("TouristEvent", message, messageSize, nil, 0);
	if (message != &(tinyBuffer[0])) {
		FreeTaggedMemory(message, "LogTouristEvent");
	}
}

#endif /* DEBUG_TOURIST */
// =============================================================================
