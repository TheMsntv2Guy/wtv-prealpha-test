/* ===========================================================================
	TellyIO.h

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __TELLYIO_H__
#define __TELLYIO_H__

#define SCRIPTMAX	16384			/* Memory size for script */
#define kMAX_DIGITS	32

#define k800TellyScriptURL			"file://ROM/Text/artemis_18006138199.tok"
#define kDownloadedTellyScriptURL	"cache:downloaded.tok"
#define kLocalTellyScriptURL		"file://ROM/Text/artemis_14153290511.tok"

typedef enum {
	kTellyIdle = 0,
	kTellyConnected = 1,
	kTellyCarrier = 2,
	kTellyDialing = 3,
	kTellyLogin = 4,
	kTellyNegotiatingPPP = 5,
	kTellyInitializingModem = 6,
	kTellyHandshake = 7
} TellyProgress;

typedef enum {
	kTellyParseError = 0,
	kTellyConnecting = 1,
	kTellyLinkConnected = 2,
	kTellyConfigurationError = 3,
	kTellyDialingError = 4,
	kTellyNoDialtone = 5,
	kTellyNoAnswer = 6,
	kTellyBusy = 7,
	kTellyHandshakeFailure = 8,
	kTellyUnknownError = 9
} TellyResult;

typedef struct ConnectionStats {
	long dcerate;
	long dterate;
	long protocol;
	long compression;
} ConnectionStats;

typedef enum {
	kSpeed21600 = 10,
	kSpeed19200 = 11,
	kSpeed16800 = 12,
	kPowerLoss = 13,
	kCarrierLoss = 14,
	kInactivityTimeout = 15
} ConnectionLogItems;

#define kMaxLogEntries 32

/********

type = 0-9 -- same as TellyResult
		10 -- speed 21600
		11 -- speed 19200
		12 -- speed 16800
		13 -- power loss
		14 -- carrier loss
		15 -- inactivity timeout
		
********/

typedef struct LogStruct {
	long		type;
	ulong		GMT;
} LogStruct;

typedef struct ConnectionLog {
	long		maxEntries;
	long		entries;
	LogStruct	items[kMaxLogEntries];
} ConnectionLog;

/* MAKE SURE YOU CHANGE THE MATCHING STRUCT IN THE TELLYSCRIPTS! */

typedef struct PHONE_SETTINGS {
	char	callWaitingPrefix[kMAX_DIGITS];
	char	dialOutsidePrefix[kMAX_DIGITS];
	char	accessNumber[kMAX_DIGITS];
	Boolean usePulseDialing;
	Boolean audibleDialing;
	Boolean disableCallWaiting;
	Boolean dialOutsideLine;
	Boolean changedCity;
	Boolean waitForTone;
} PHONE_SETTINGS;

TellyResult RunScript(const char* data, long length);
TellyProgress GetScriptProgress(void);
long GetScriptProgressPercentage(long callCount);
const char* GetScriptProgressText(long callCount);
void SetScriptProgress(TellyProgress status);
ConnectionStats *GetConnectionStats(void);
void TellyIdle(void);
void SetScriptAborted(void);
void SetScriptResult(TellyResult status);
TellyResult GetScriptResult(void);
void NewScript(uchar *data, long length);
void SetPhoneSettings(PHONE_SETTINGS *phone);
PHONE_SETTINGS *GetPhoneSettings(void);
ConnectionLog *GetConnectionLog(void);
void AddConnectionLog(long type);
void SendConnectionLog(void);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include TellyIO.h multiple times"
	#endif
#endif /* __TELLYIO_H__ */