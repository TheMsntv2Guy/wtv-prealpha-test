// Copyright (c) 1996 Artemis Research, Inc. All rights reserved.

#ifndef __LOG_H__
#define __LOG_H__

// =============================================================================

#define kLogCrash		"crash"
#define kLogError		"error"
#define kLogTest		"test"
#define kLogWarning		"warning"

// =============================================================================

void LogError(const char* message, const char* file, long line);
void LogWarning(const char* message, const char* file, long line);
void SendLog(const char* type, const char* message, long length, const char* file = nil, long line = 0);

// =============================================================================

enum ModemEvent {
	kModemEventDialin,
	kModemEventHangup
};

void LogModemEvent(ModemEvent modemEvent);

// =============================================================================

void LogCrash(void);

// =============================================================================

#ifdef DEBUG_TOURIST
	enum TouristEvent {
		kTouristEventStartTour,
		kTouristEventGoToSite,
		kTouristEventArriveAtSite,
		kTouristEventEndTour
	};
	void LogTouristEvent(TouristEvent touristEvent, const char* url);
#endif /* DEBUG_TOURIST */

// =============================================================================

#if defined(GOOBER) || defined(BOOTROM)

#define IsError(condition)		(condition)
#define IsWarning(condition)	(condition)

#else

#define IsError(condition)		((condition) ? (LogError("Error: " ## #condition, __FILE__, __LINE__), 1) : 0)
#define IsWarning(condition)	((condition) ? (LogWarning("Warning: " ## #condition, __FILE__, __LINE__), 1) : 0)

#endif

// =============================================================================

#endif
