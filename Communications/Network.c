// ===========================================================================
// Network.c
//
// Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
//===========================================================================

#include "Headers.h"

#include "AlertWindow.h"
#include "BoxUtils.h"
#include "Cache.h"
#include "CacheStream.h"
#include "Cookie.h"
#include "FlashStorage.h"
#include "HTTP.h"
#include "MemoryManager.h"
#include "Network.h"
#include "ObjectStore.h"
#include "OptionsPanel.h"
#include "PageViewer.h"
#include "ppp.h"
#include "tinyip.h"
#include "tinyudp.h"
#include "Serial.h"
#include "Socket.h"
#include "SongData.h"
#include "Status.h"
#include "System.h"
#include "TellyIO.h"
#include "URLParser.h"
#include "CryptoInterface.h"

#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif

#ifndef __CLIENTFUNCTIONS_H__
#include "ClientFunctions.h"
#endif

#ifdef FOR_MAC
	#include "LocalNet.h"
	#include "MacintoshMenus.h"
	#include "MacSimulator.h"
#endif

#ifdef DEBUG_BOXPRINT
#include "BoxPrintDebug.h"
#endif

// ===========================================================================

#define kNetworkStatusURL			"cache:network-status"
#define kTicketURL					"cache:wtv-ticket"
#define kResponseURL				"cache:wtv-challenge-response"

#define kStatusDialBegin	5

// ===========================================================================
// Global variables

Network* gNetwork;

// ===========================================================================
// Class Network

Network::Network()
{
	fTicketResource.SetURL(kTicketURL);
	fChallengeResponse.SetURL(kResponseURL);
}

Network::~Network()
{
	if (gServiceList) {
		delete(gServiceList);
		gServiceList = nil;
	}
	
	fVisitList.DeleteAll();
}

void
Network::Activate()
{
	Resource resource;
	
	resource.SetURL(gSystem->GetBootURL());
	fVisitList.DeleteAll();
	Visit(&resource);

	fIndicator = gStatusIndicator;
	gModemSound->Play();
	LogModemEvent(kModemEventDialin);
	fState = kNetShowStartup;
}

#ifdef DEBUG_BOXPRINT
void
Network::BoxPrintDebug(long whatToPrint) const
{
	int protocolCount;
	
	if (whatToPrint == 0)
		whatToPrint = kBoxPrintState | kBoxPrintProtocol;

	if (whatToPrint & kBoxPrintState)
#ifdef DEBUG_NAMES
		BoxPrint("NetState = %s (%d)", GetStateAsString(), GetState());
#else
		BoxPrint("NetState = %d", GetState());
#endif

	if (whatToPrint & kBoxPrintProtocol)
		for (protocolCount = 0; protocolCount < kProtocolCount; protocolCount++)
			Protocol::StaticBoxPrintDebug(fProtocol[protocolCount], 0);

	if (whatToPrint & kBoxPrintTicketResource)
		Resource::StaticBoxPrintDebug(&fTicketResource, 0);

	if (whatToPrint & kBoxPrintVisitList)
		BoxPrint("VisitList: not implemented");

	if (whatToPrint & kBoxPrintLoginStream)
		BoxPrint("LoginStream: not implemented");

	if (whatToPrint & kBoxPrintServiceList)
		ServiceList::StaticBoxPrintDebug(gServiceList, 0);
}
#endif

HTTPProtocol*
Network::ChooseProtocol(Resource* resource)
{
	ServicePointer service;
	Priority priority;
	long i;
	
	if (IsError(resource == nil))
		return nil;
	
	if (!(service = gServiceList->FindByResource(resource))) {
		resource->SetStatus(kUnknownService);
		return nil;
	}
	
	// Choose a protocol that is ready to go.
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] != nil)
			if (fProtocol[i]->CanConnectTo(service, resource)) {
				fProtocol[i]->Connect(service, resource);
				return fProtocol[i];
			}
	
	// Delete dead protocols.
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] != nil)
			if (fProtocol[i]->IsDead()) {
				delete(fProtocol[i]);
				fProtocol[i] = nil;
			}
	
	// Create a new protocol.
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] == nil) {
			fProtocol[i] = (HTTPProtocol*)service->NewProtocol();
			fProtocol[i]->Connect(service, resource);
			return fProtocol[i];
		}
	
	// Choose an idle protocol connected to a different service.
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] != nil)
			if (fProtocol[i]->IsReady()) {
				delete(fProtocol[i]);
				fProtocol[i] = (HTTPProtocol*)service->NewProtocol();
				fProtocol[i]->Connect(service, resource);
				return fProtocol[i];
			}
	
	// Choose a protocol of lower priority.
	priority = resource->GetPriority();
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] != nil && fProtocol[i]->IsRunning())
			if (fProtocol[i]->GetPriority() < priority) {
				delete(fProtocol[i]);
				fProtocol[i] = (HTTPProtocol*)service->NewProtocol();
				fProtocol[i]->Connect(service, resource);
				return fProtocol[i];
			}

	return nil;
}

void
Network::Deactivate()
{
	// Deactivate the network.
	LogModemEvent(kModemEventHangup);

	if (IsWarning(fState == kNetInactive))
		return;

	ImportantMessage(("Network::Deactivate"));
	SetTicket(nil);
	HandleNotificationPort(false);
	fState = kNetHangUpBegin;
}

#ifdef INCLUDE_FINALIZE_CODE
void
Network::Finalize()
{
	HTTPProtocol::Finalize();

	if (gServiceList != nil) {
		delete(gServiceList);
		gServiceList = nil;
	}

	if (gNetwork != nil) {
		delete(gNetwork);
		gNetwork = nil;
	}
}
#endif

#ifdef SIMULATOR
HTTPProtocol*
Network::GetProtocol(long index) const
{
	if (IsError(index < 0 || index >= kProtocolCount))
		return nil;
		
	return fProtocol[index];
}
#endif

#ifdef DEBUG_NAMES
const char*
Network::GetStateAsString() const
{
	switch (fState) {
	case kNetInactive:				return "Inactive";
	case kNetShowStartup:			return "ShowStartup";
	case kNetDialBegin:				return "DialBegin";
	case kNetDialEnd:				return "DialEnd";
	case kNetActive:				return "Active";
	case kNetWaiting:				return "Waiting";
	case kNetHangUpThenDialBegin:	return "HangUpThenDialBegin";
	case kNetHangUpThenDialEnd:		return "HangUpThenDialEnd";
	case kNetHangUpThenWait:		return "HangUpThenWait";
	case kNetHangUpBegin:			return "HangUpBegin";
	case kNetHangUpEnd:				return "HangUpEnd";
	default:						IsError("unknown fState");
	}
	
	return "unknown";
}
#endif

static uchar kSessionKeyBuf[16];

uchar*
Network::GetSessionKey1(long incarnation) const
{
	uchar buffer[36];
	union {
		uchar buf[4];
		long val;
	} num;
		
	memcpy(&buffer[0],fSessionKey1, 16);
	num.val = incarnation;
	memcpy(&buffer[16], &num.buf[0], 4);
	memcpy(&buffer[20], fSessionKey1, 16);

	return MD5(buffer, 36, kSessionKeyBuf);

}

uchar*
Network::GetSessionKey2(long incarnation) const
{
	uchar buffer[36];
	union {
		uchar buf[4];
		long	 val;
	} num;

	memcpy(&buffer[0],fSessionKey2, 16);
	num.val = incarnation;
	memcpy(&buffer[16], &num.buf[0], 4);
	memcpy(&buffer[20], fSessionKey2, 16);

	return MD5(buffer, 36, kSessionKeyBuf);
}

void
Network::HangUp()
{
	LogModemEvent(kModemEventHangup);
	fState = kNetHangUpThenWait;
}

void
Network::HangUpAll()
{
	KillProtocols();
	gRAMCache->DisableLoadingAll();
	SerialSetDtr(kMODEM_PORT, false);		/* hangup */
	SetScriptAborted();
	SetScriptResult(kTellyConnecting);
	SetScriptProgress(kTellyIdle);
	SetPPPConnected(false);
	SetBoxLEDs(GetBoxLEDs() & ~kBoxLEDConnect);
	gOptionsPanel->PhoneStatusChanged();
}

void 
Network::Idle()
{
	PerfDump perfdump("Network::Idle");
	PushDebugChildURL("Network::Idle");

	switch (fState) {
	case kNetInactive:				break;
	case kNetShowStartup:			IdleShowStartup(); break;
	case kNetDialBegin:				IdleDialBegin(); break;
	case kNetDialEnd:				IdleDialEnd(); break;
	case kNetActive:				ProfileUnitEnter("Network::IdleActive");
									IdleActive(); 
									ProfileUnitExit("Network::IdleActive");
									break;
	case kNetWaiting:				IdleWaiting(); break;
	case kNetHangUpBegin:			IdleHangUpBegin(); break;
	case kNetHangUpEnd:				IdleHangUpEnd(); break;
	case kNetHangUpThenDialBegin:	IdleHangUpThenDialBegin(); break;
	case kNetHangUpThenDialEnd:		IdleHangUpThenDialEnd(); break;
	case kNetHangUpThenWait:		IdleHangUpThenWait(); break;
	default:						IsError(fState);
	}

	PopDebugChildURL();
}

void
Network::IdleShowStartup()
{
	Message(("Network: IdleShowStartup"));

/* they pressed 'go' button before powering up -- go to phone setup */

	if (gSystem->GetUsingConnectSetup())
		{
		ExecuteClientFunction("client:GoToConnectSetup", nil);
		gSystem->SetUsingConnectSetup(false);		/* only show once */
		fState = kNetWaiting;
		}
	else if (gSystem->GetUsingPhoneSetup())
		{
		ExecuteClientFunction("client:GoToPhoneSetup", nil);
		gSystem->SetUsingPhoneSetup(false);			/* only show once */
		fState = kNetWaiting;
		}
	else
		{
		ShowStartup();
		fState = kNetDialBegin;
		IdleDialBegin();
		}
}

void 
Network::IdleDialBegin()
{
	TellyResult tellyResult;
	DataStream* stream;

	Message(("Network: IdleDialBegin"));
	fIndicator->SetPercentDone(kStatusDialBegin);
	gOptionsPanel->PhoneStatusChanged();

	if (!gSystem->GetUsePhone()) {
		fState = kNetDialEnd;
		return;
	}

#ifdef HARDWARE
	gVBLsElapsed = 0;
	gVBLsPerConnectedFlashes = 15;
#endif

	fState = kNetDialEnd;
	stream = NewTellyScriptStream();
	tellyResult = RunScript(stream->GetData(), stream->GetDataLength());

	if (tellyResult == kTellyLinkConnected)
		{
		ConnectionStats*	stats = GetConnectionStats();

		switch (stats->dcerate)
			{
			case 21600:
				AddConnectionLog(kSpeed21600);
				break;
			case 19200:
				AddConnectionLog(kSpeed19200);
				break;
			case 16800:
				AddConnectionLog(kSpeed16800);
				break;
			default:
				break;
			}
		}
	else
		AddConnectionLog(tellyResult);

	delete(stream);
}

void
Network::IdleDialEnd()
{
	// Hang out waiting for TellyScript to dial the telephone. If successful, the next
	// state is kNetActive. Otherwise, we show an alert window, hang up, and set the
	// next state to kNetWaiting.
	
	TellyResult tresult;

	if (!gSystem->GetUsePhone()) {
		SetBoxLEDs(GetBoxLEDs() | kBoxLEDConnect);
		fState = kNetActive;
		fIndicator->Hide();
		return;
	}
	
	if ((tresult = GetScriptResult()) == kTellyConnecting) {
		static long oldprogress;
		long progress = GetScriptProgress();
		Boolean needToSetIndicator = (progress != oldprogress);

		if (needToSetIndicator) {
			fIndicator->SetMessage(GetScriptProgressText(fCallCount));
			fIndicator->SetPercentDone(GetScriptProgressPercentage(fCallCount));
			oldprogress = progress;
		}
		return;
	}

#ifdef HARDWARE
	gVBLsPerConnectedFlashes = 0;
	SetBoxLEDs(GetBoxLEDs() & ~kBoxLEDConnect);
#endif

	switch (tresult) {
	case kTellyLinkConnected:
		Message(("Network: NET going active\n"));
		SetBoxLEDs(GetBoxLEDs() | kBoxLEDConnect);

		fCallCount--;
		fIndicator->SetMessage(GetScriptProgressText(fCallCount));
		fIndicator->SetPercentDone(GetScriptProgressPercentage(fCallCount));

		if (fCallCount < 1)				/* keep the progress bar up between 800 and POP calls */
			fIndicator->Hide();
		
		fState = kNetActive;
		return;				

	case kTellyBusy:
		gAlertWindow->SetURL("file://rom/phone/busy.html");
		break;

	case kTellyHandshakeFailure:
		gAlertWindow->SetURL("file://rom/phone/badhandshake.html");
		break;

	case kTellyNoAnswer:
		gAlertWindow->SetURL("file://rom/phone/noanswer.html");
		break;

	case kTellyNoDialtone:
		gAlertWindow->SetURL("file://rom/phone/nodialtone.html");
		break;

	case kTellyConfigurationError:
	case kTellyDialingError:
	case kTellyUnknownError:
	case kTellyParseError:
		gAlertWindow->SetURL("file://rom/phone/genericerr.html");
		break;

	default:
		IsError("Tellyscript: unknown tellyscript result");
	    gAlertWindow->SetURL("file://rom/phone/genericerr.html");
	}

	// Handle errors.
	gAlertWindow->Show();
	HangUpAll();
	fState = kNetWaiting;
}	

void 
Network::IdleActive()
{
	Resource* pending;
	HTTPProtocol* protocol;
	ProtocolCommand* command;

	if (gSystem->GetUsePhone())
		if (GetScriptResult() != kTellyLinkConnected) {
			AddConnectionLog(kCarrierLoss);
			gConnectWindow->Reset();
			gConnectWindow->SetError(kLostConnection);
			gConnectWindow->Show();
			HangUpAll();
			fState = kNetWaiting;
			return;
		}
	
	if (fNotificationPending)
		RequestNotifications();

	RunProtocols();

	for (;;) {
		if ((pending = NewPendingResource()) == nil) {
			if (fVisitList.GetCount() != 0 && !IsProtocolRunning()) {
				Resource* visitResource = (Resource*)fVisitList.At(0);
				gPageViewer->ShowResource(visitResource);
				fVisitList.RemoveAt(0);
			}
			return;
		}

#ifdef SIMULATOR
		if ((gLocalNet != nil) && IsWarning(gLocalNet->GetExclusiveRead()))
			return;
#endif

		if ((protocol = ChooseProtocol(pending)) == nil) {
			delete(pending);
			return;
		}
		
		if (!protocol->IsReady()) {
			delete(pending);
			return;
		}
		
		if (pending->GetPostDataLength() == 0)
			command = protocol->NewGetCommand(pending);
		else
			command = protocol->NewPostCommand(pending);
			
		if (IsError(command == nil)) {
			delete(pending);
			return;
		}
		
		protocol->RunCommand(command);
		delete(pending);
	}
}
		
void 
Network::IdleHangUpBegin()
{
	Message(("Network: IdleHangUpBegin"));
	HangUpAll();
	fState = kNetHangUpEnd;
}

void 
Network::IdleHangUpEnd()
{
	fState = kNetInactive;
}

void
Network::IdleHangUpThenDialBegin()
{
	Message(("Network: IdleHangUpThenDialBegin"));
	HangUpAll();
	fState = kNetHangUpThenDialEnd;
	IdleHangUpThenDialEnd();
}

void
Network::IdleHangUpThenDialEnd()
{
	fState = kNetDialBegin;
	IdleDialBegin();
}

void
Network::IdleHangUpThenWait()
{
	Message(("Network::IdleHangUpThenWait"));
	HangUpAll();
	fState = kNetWaiting;
}

void
Network::IdleWaiting()
{
	Resource* pending;
	const char* url;
	
	// Check if there is anything to do.
	if ((pending = NewPendingResource()) == nil)
		return;

	// Inform the headwaiter that we have reconnected.
	if ((url = gSystem->GetReconnectURL()) != nil)
		Visit(url);

	// Reconnect.
	delete(pending);
	fState = kNetDialBegin;
	ShowConnectIndicator(true);
	IdleDialBegin();
}

void 
Network::Initialize()
{
	const char* host;
	long port;
	
	ImportantMessage(("Network::Initialize\n"));
	gNetwork = new(Network);
	host = gSystem->GetServerName();
	port = gSystem->GetServerPort();
	
	switch (gSystem->GetServerType()) {
	case kServerTypePreregistration:
		gNetwork->InitializeForSignup(host, port, gSystem->GetForceRegistration());
		break;
	
	case kServerTypeLogin:
		gNetwork->InitializeForLogin(host, port);
		break;
	
	case kServerTypeProxy:
		gNetwork->InitializeForProxy(host, port);
		break;
		
	default:
		gNetwork->InitializeForNone();
		break;
	}

	/* make the phone be in a good state.  later gets overridden by users restore NVRAM settings */
	{
	PHONE_SETTINGS	phone;
	
	phone.usePulseDialing = false;
	phone.audibleDialing = true;
	phone.disableCallWaiting = false;
	phone.dialOutsideLine = false;
	phone.changedCity = false;
	phone.waitForTone = true;
	phone.callWaitingPrefix[0] = 0;
	phone.dialOutsidePrefix[0] = 0;
	phone.accessNumber[0] = 0;

	SetPhoneSettings(&phone);

	/* init the struct size for the remote */

	GetConnectionLog()->maxEntries = kMaxLogEntries;
	}
}

void
Network::InitializeForKrueger()
{
	Resource resource;
	
	delete(gServiceList);
	gServiceList = new(ServiceList);
	gSystem->SetBootURL("http://www.artemis.com/");
	gSystem->SetHomeURL("http://www.artemis.com/");
	gSystem->SetNameServer(QuadChar(192,244,1,1));
	gSystem->SetUseJapanese(true);

	fVisitList.DeleteAll();
	resource.SetURL(gSystem->GetBootURL());
	Visit(&resource);
}

void
Network::InitializeForLogin(const char* host, long port)
{
	Resource resource;
	
	delete(gServiceList);
	gServiceList = new(ServiceList);
	gServiceList->Add("wtv-head-waiter", host, port);
	gSystem->SetBootURL("wtv-head-waiter:/login");
	gSystem->SetHomeURL("wtv-home:/home");
	gServiceList->Add("wtv-register", host, port);
	
	fVisitList.DeleteAll();
	resource.SetURL(gSystem->GetBootURL());
	Visit(&resource);
}

void
Network::InitializeForNone()
{
	Resource resource;
	
	delete(gServiceList);
	gServiceList = new(ServiceList);
	gSystem->SetBootURL("http://arcadia/webtv/client/splash-debug.html");
	gSystem->SetHomeURL("http://arcadia/webtv/client/home.html");

	fVisitList.DeleteAll();
	resource.SetURL(gSystem->GetBootURL());
	Visit(&resource);
}

void
Network::InitializeForProxy(const char* host, long port)
{
	Resource resource;
	Service* service;
	
	service = new(Service);
	service->SetName("http");
	service->SetHostName(host);
	service->SetHostPort(port);
	service->SetIsProxy(true);
	service->SetProtocolClassNumber(kClassNumberHTTPProtocol);

	delete(gServiceList);
	gServiceList = new(ServiceList);
	gServiceList->Add(service);
	gSystem->SetBootURL("http://arcadia/webtv/client/splash-debug.html");
	gSystem->SetHomeURL("http://arcadia/webtv/client/home.html");
	
	fVisitList.DeleteAll();
	resource.SetURL(gSystem->GetBootURL());
	Visit(&resource);
}

void
Network::InitializeForSignup(const char* host, long port, Boolean forceSignup)
{
	Resource resource;
	
	delete(gServiceList);
	gServiceList = new(ServiceList);
	gServiceList->Add("wtv-1800", host, port);
	gSystem->SetBootURL("wtv-1800:/preregister");
	gServiceList->Add("wtv-register", host, port);

	if (forceSignup)
		gSystem->SetBootURL("wtv-1800:preregister?ForceRegistration=1");
	
	fVisitList.DeleteAll();
	resource.SetURL(gSystem->GetBootURL());
	Visit(&resource);
}

Boolean
Network::IsProtocolRunning() const
{
	long i;
	
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] != nil)
			if (fProtocol[i]->IsRunning())
				return true;
	
	return false;
}
	
void 
Network::KillProtocols()
{
	long i;
	
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] != nil) {
			delete(fProtocol[i]);
			fProtocol[i] = nil;
		}
		
#ifdef FOR_MAC
	while (MacSocket::SocketIdle())
		;
#endif
}

Resource* 
Network::NewPendingResource()
{
	CacheEntry* entry;
	Resource* resource;
	
	if ((entry = gRAMCache->NextPendingEntry()) != nil) {
		resource = new(Resource);
		resource->SetURL(entry);
		return resource;
	}
	
	return nil;
}

DataStream*
Network::NewTellyScriptStream()
{
	// Choose a TellyScript and return a new stream for reading. An empty
	// MemoryStream is created if no TellyScript can be found so that the
	// caller does not have to handle nil.
	
	Resource resource;
	DataStream* stream;
	
	// Try downloaded TellyScript.
	resource.SetURL(kDownloadedTellyScriptURL);
	if ((stream = resource.NewStream()) != nil) {
		Message(("Network: executing downloaded TellyScript"));
		return stream;
	}
	
#ifdef DEBUG
	// Try local TellyScript.
	if (gSystem->GetServerType() != kServerTypePreregistration) {
		resource.SetURL(kLocalTellyScriptURL);
		if ((stream = resource.NewStream()) != nil) {
			Message(("Network: executing 415 number TellyScript"));
			return stream;
		}
	}
#endif

	// Try ROM-based TellyScript.
	resource.SetURL(k800TellyScriptURL);
	if ((stream = resource.NewStream()) != nil) {
		Message(("Network: executing 800 number TellyScript"));
		return stream;
	}
	
	// Return null TellyScript.
	Message(("Network: executing null TellyScript"));
	return new(MemoryStream);
}

static long
udp_datahandler(udp_Socket UNUSED(s), uplink UNUSED(link), uchar* UNUSED(buffer), long UNUSED(count))
{
	Message(("Network: Notification over udp."));
	gNetwork->SetNotificationPending(true);
	return 0;
}

Error
Network::HandleNotificationPort(Boolean open)
{
	extern ip_State* gIPState;
	static udp_Socket gUDPSocket = nil;
	static Boolean inited = false;

	if (open) {
		if (inited)
			return kGenericError;

		if (!gSystem->GetUsePhone())
			return kGenericError;
		
		if (!gIPState)
			return kGenericError;
	
		gUDPSocket = udpOpen((udp_State)gIPState->ip_udp, 6666, udp_datahandler, true);
		
		if (gUDPSocket == nil)
			return kGenericError;
		
		inited = true;
	} else {
	
		if(!inited)
			return kGenericError;
		
		if (gUDPSocket != nil) {
			udpClose(gUDPSocket);
			gUDPSocket = nil;
		}
	}
	
	return kNoError;
}

void
Network::Reactivate(Boolean reconnect)
{
	if (IsWarning(fState != kNetWaiting))
		return;
		
	fState = kNetDialBegin;
	ShowConnectIndicator(reconnect);
}

void
Network::RequestNotifications()
{
	Resource resource;
	const char* url;
	
	url = gSystem->GetNotificationsURL();
	if (IsWarning(url == nil))
		return;
	
	resource.SetURL(url);
	resource.Purge();
	resource.SetPriority(kBackground);
	resource.SetStatus(kNoError);
	fNotificationPending = false;
}

void
Network::RestoreState(void)
{
	uchar *data;
	char *name = 0;
	ushort port = 0;
	long len;
	Boolean pieceMissing = false;
	Resource resource;
	DataStream* stream;
	
	Message(("Network: restoring state..."));
	
	/* get tellyscript */
	data = NVRead(kTellyTag, &len);
	if (data != nil) {
		gRAMCache->Delete(kDownloadedTellyScriptURL);
		resource.SetURL(kDownloadedTellyScriptURL);
		if ((stream = resource.NewStreamForWriting()) != nil) {
			stream->Write(data, len);
			delete(stream);
		} else
			Message(("Network: cannot create stream for %s", kDownloadedTellyScriptURL));
	} else
		Message(("No Tellyscript in NV storage!"));

	/* get connection log */
	data = NVRead(kConnectionLogTag, &len);
	if (data != nil)
		memcpy(GetConnectionLog(), data, len);

	/* get host name */
	data = NVRead(kHNameTag, &len);
	if (data != nil)
		name = (char*)data;
	else
	{
		pieceMissing = true;
		Message(("No Hostname in NV storage!"));
	}

	/* get host port */
	data = NVRead(kHPortTag, &len);
	if (data != nil)
		port = *(ushort*)data;
	else {
		pieceMissing = true;
		Message(("No Host Port in NV storage!"));
	}
		
	if ( !pieceMissing ) {
	/* add the service */
		Message(("Network::RestoreState : name = %s, port = %d", name, port));
		gServiceList->Add("wtv-head-waiter", name, port);
	}

	/* get boot url */
	data = NVRead(kBootTag, &len);
	if (data != nil)
	gSystem->SetBootURL((char*)data);
	else
		Message(("No Boot URL in NV storage!"));

	/* get users phone settings */
	data = NVRead(kPhoneTag, &len);
	if (data != nil)
	SetPhoneSettings((PHONE_SETTINGS*)data);
	else
		Message(("No Phone Settings in NV storage!"));
		
	/* get shared secret key*/
	data = NVRead(kSecretTag, &len);
	if (data != nil) {
		memcpy(&fSecret, data, sizeof(fSecret));
		fSecretValid = true;
	}
	else
		Message(("No Shared Secret in NV storage!"));
	
}

void
Network::RunProtocols()
{
	long i;
	
	PushDebugChildURL("Network::RunProtocols");
	for (i = 0; i < kProtocolCount; i++)
		if (fProtocol[i] != nil)
			fProtocol[i]->Idle();
	PopDebugChildURL();
			
#ifdef FOR_MAC
	MacSocket::SocketIdle();
#endif
	TinySocket::SocketIdle();
}

void
Network::SaveState(void)
{
	Service *service;
	char* name;
	ushort port;
	Resource resource;
	DataStream* stream;
	
	Message(("Network: saving state..."));
	
	/* save downloaded tellyscript */
	resource.SetURL(kDownloadedTellyScriptURL);
	if ((stream = resource.NewStream()) != nil) {
		NVWrite((uchar*)stream->GetData(), stream->GetDataLength(), kTellyTag);
		delete(stream);
	}
	
	/* save connection log */
	
	NVWrite((uchar *)GetConnectionLog(), sizeof(ConnectionLog), kConnectionLogTag);

	/* save wtv-head-waiter service */
	service = gServiceList->FindByName("wtv-head-waiter");
	if (service != nil) {
		name = (char*)service->GetHostName();
		port = service->GetHostPort();
		NVWrite((uchar*)name,strlen(name)+1, kHNameTag);
		NVWrite((uchar*)&port,sizeof(port), kHPortTag);
	}
	
	/* save BootURL */
	name = (char*)gSystem->GetBootURL();
	NVWrite((uchar*)name,strlen(name)+1, kBootTag);

	/* users phone settings */
	NVWrite((uchar*)GetPhoneSettings(), sizeof(PHONE_SETTINGS), kPhoneTag);
	
	/* shared secret key */
	if (fSecretValid)
		NVWrite((uchar*)&fSecret, sizeof(fSecret), kSecretTag);
	
}

Boolean
Network::SetAttribute(const char* name, char* value)
{
	if (EqualString(name, "wtv-visit")) {
		Visit(value);
		return true;
	}
	
	// the ticket is only asked for at the beginning of the session
	if (EqualString(name, "wtv-ticket")) {
		SetTicket(value);
		gCookieList->Load();
		gSystem->LoadSettings();
		LogCrash();					/* if we have a valid crashlog, send it up */
		SendConnectionLog();
		return true;
	}

	if (EqualString(name, "wtv-challenge")) {
		Error err;
		
		err = ProcessChallenge((uchar*)value);
		
		/* Need to determine what to do on an error here. */
		
		return true;
	}
	
	return false;
}

Boolean
Network::SetSpecialAttribute(const char* name, char* value)
{
	
	/* the initial secret processing only happens when talking to wtv-1800 */
	if (EqualString(name, "wtv-initial-key")) {
		ProcessSecret((uchar*)value);
		return true;
	}
	
	return false;
}

Error
Network::ProcessSecret(uchar* key)
{
	Error err = kNoError;
	uchar* decode_buf = nil;
	int	decode_len;

	if (IsError(key == nil))
		return kGenericError;
		
	if (fSecretValid)
		return kGenericError;
	
	decode_len = strlen((const char*)key);
	decode_buf = (uchar*)NewFromBase64((const char*)key, &decode_len);
	if(!decode_buf) goto bail;
	
	/* Set the secret here */
	memcpy(&fSecret, decode_buf, decode_len);
	fSecretValid = true;
	
	goto nobail;
	
bail:
	err = kGenericError;
	
nobail:

	if (decode_buf)
		FreeTaggedMemory(decode_buf, kStringTag);
	return err;
}

static uchar kDefaultClientKey[] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};

Error
Network::ProcessChallenge(uchar* keys)
{
	Error err = kNoError;
	uchar* decode_buf = nil;
	uchar* decrypt_buf = nil;
	uchar* decrypt_bufp;
	uchar* hash;
	uchar* response_buf = nil;
	uchar* response_bufp;
	uchar* eresponse = nil;
	int	decode_len;
	int	decrypt_len;
	int encrypt_len;
	int count;	
	Challenge_t* challenge;
	Response_t* Response_tp;
	EVP_CIPHER_CTX encrypt_ctx;

	if (IsError(keys == nil))
		return kGenericError;

	/* Mac : for demos only, remove later */
	PostulateFinal(false);
	if (!fSecretValid) {
		memcpy(&fSecret, kDefaultClientKey, 8);
		fSecretValid = true;
	}
		
	if(IsError(!fSecretValid))
		return kAuthorizationError;

	memset((void*)&encrypt_ctx, NULL, sizeof(encrypt_ctx));
	
	decode_len = strlen((const char*)keys);

	decode_buf = (uchar*)NewFromBase64((const char*)keys, &decode_len);
	if(!decode_buf) goto bail;

	decrypt_buf = (uchar*)AllocateMemory(decode_len);
	if(!decrypt_buf) goto bail;
	decrypt_bufp = decrypt_buf;
	challenge = (Challenge_t *)(decrypt_buf);
	
	memcpy(challenge, &decode_buf[0], 8);	/* copy vector */
	decrypt_bufp += 8;
		
	EVP_DecryptInit(&encrypt_ctx, EVP_des_ecb, (unsigned char *)&fSecret, NULL);
	EVP_DecryptUpdate(&encrypt_ctx, decrypt_bufp, &count, (decode_buf + 8), 
							decode_len - sizeof(challenge->vector));
	decrypt_bufp += count;
	decrypt_len = count;
	EVP_DecryptFinal(&encrypt_ctx, decrypt_bufp, &count);
	decrypt_len += count;
		
	if (decrypt_len != (sizeof(Challenge_t) - sizeof(challenge->vector)))
		goto bail;
	
	hash = MD5((uchar*)&challenge->c_text, (sizeof(Challenge_t) - sizeof(challenge->vector)
												- sizeof(challenge->hash)), NULL);
	if (memcmp((const void*)hash, (const void*)(challenge->hash), MD5_DIGEST_LENGTH)) {
		err = kAuthorizationError;
		goto nobail;
	}
		
	memcpy(fSessionKey1, challenge->key1, 16);
		
	memcpy(fSessionKey2, challenge->key2, 16);
	
	memcpy(&fSecret, challenge->new_exchange_key, 8);
	fSecretValid = true;
	
	if (!(response_buf = (uchar*)AllocateMemory(sizeof(Response_t) + 8)))
		goto bail;
	Response_tp = (Response_t*)(response_buf);
	memcpy(Response_tp->vector, challenge->vector, sizeof(Response_tp->vector));
	memcpy(Response_tp->c_text, challenge->c_text, sizeof(Response_tp->c_text));
	MD5(challenge->c_text, sizeof(challenge->c_text), Response_tp->hash);
	
	/* need to use new exchange key here */

	memset((void*)&encrypt_ctx, NULL, sizeof(encrypt_ctx));

	EVP_EncryptInit(&encrypt_ctx, EVP_des_ecb, (unsigned char*)&fSecret, NULL);
	response_bufp = (unsigned char *)&(Response_tp->hash);
	encrypt_len = 8;
	EVP_EncryptUpdate(&encrypt_ctx, response_bufp, &count, response_bufp, 
							sizeof(Response_t) - sizeof(Response_tp->vector));
	response_bufp += count;
	encrypt_len += count;
	EVP_EncryptFinal(&encrypt_ctx, response_bufp, &count);
	encrypt_len += count;
	
	eresponse = (uchar*)NewBase64StringFromBuf((const uchar*)response_buf, encrypt_len);
	if(!eresponse) goto bail;
	
	SetResponse((const char*)eresponse);
	
	goto nobail;
		
bail:
	err = kGenericError;
	Message(("ProcessChallenge Error"));

nobail:

	if (response_buf)
		FreeMemory(response_buf);
	if (decode_buf)
		FreeTaggedMemory(decode_buf, kStringTag);
	if (decrypt_buf)
		FreeMemory(decrypt_buf);
	if (eresponse)
		FreeTaggedMemory(eresponse, kStringTag);	

	return err;

}

void
Network::SetTellyScript(const Resource* resource)
{
	CacheEntry* cacheEntry;
	
	if (IsError(resource == nil))
		return;
	
	cacheEntry = resource->GetCacheEntry();
	if (IsError(cacheEntry == nil))
		return;
	
	Message(("Network: installing TellyScript from %s", cacheEntry->GetName()));
	gRAMCache->Delete(kDownloadedTellyScriptURL);
	cacheEntry->SetName(kDownloadedTellyScriptURL, gRAMCache);
	fState = kNetHangUpThenDialBegin;
}

void
Network::SetTicket(const char* ticket)
{
	CacheStream* stream = fTicketResource.NewStreamForWriting();

	if (IsError(stream == nil))
		return;
	
	stream->SetDataType((DataType)0);
	stream->SetPriority(kPersistent);
	stream->WriteString(ticket);	// nil is ok
	stream->SetStatus(kComplete);
	delete(stream);
	if (ticket)
		fHaveTicket = true;
	else {
		fHaveTicket = false;
		fSecureNetwork = false;
	}
	
	HandleNotificationPort(true);
}

void
Network::SetResponse(const char *response)
{
	CacheStream* stream = fChallengeResponse.NewStreamForWriting();

	if (IsError(stream == nil))
		return;
		
	stream->SetDataType((DataType)0);
	stream->WriteString(response);
	stream->SetPriority(kPersistent);
	stream->SetStatus(kComplete);
	delete(stream);
}

void Network::SetSecure(Boolean value)
{
	fSecureNetwork = value;
}

void
Network::ShowConnectIndicator(Boolean reconnecting)
{
	Resource resource;

	if (reconnecting) {
		fIndicator = gStatusIndicator;
		gConnectIndicator->Hide();
	} else {
		fIndicator = gConnectIndicator;
		gStatusIndicator->Hide();
		gPageViewer->SetQueuedShow(fIndicator);
	}

	resource.SetURL(kNetworkStatusURL);
	fIndicator->SetMessage(GetScriptProgressText(fCallCount));
	fIndicator->SetPercentDone(0);
	fIndicator->SetTarget(&resource);

	if (reconnecting)
		fIndicator->Show();			/* show it for everything but the startup screen */

	if (strstr(gSystem->GetBootURL(), "wtv-1800:preregister") != NULL)
		fCallCount = 2;
	else
		fCallCount = 0;
}

void
Network::ShowStartup()
{
	gPageViewer->ExecuteURL(gSystem->GetStartupURL());
	gStatusIndicator->SetDisabled(true);
	if (gSystem->GetUsePhone())
		ShowConnectIndicator(false);
}

#ifdef DEBUG_BOXPRINT
void
Network::StaticBoxPrintDebug(const Network* network, long whatToPrint)
{
	if (network == nil) {
		BoxPrint("Network: <nil>");
		return;
	}
	
	BoxPrint("Network: <%#06x>", network);
	AdjustBoxIndent(1);
	network->BoxPrintDebug(whatToPrint);
	AdjustBoxIndent(-1);
}
#endif

void 
Network::Visit(const char* url)
{
	Resource* resource;

	Message(("Visiting %s",url));
	resource = new(Resource);
	resource->SetURL(url);
	fVisitList.Add(resource);
}

void
Network::Visit(const Resource* resource)
{
	Resource* newResource;
	
	if (IsError(resource == nil))
		return;

	newResource = new(Resource);
	*newResource = *resource;
	fVisitList.Add(newResource);
}

void 
Network::WriteAttributes(Stream* stream)
{
	if (IsError(stream == nil))
		return;

	DataStream*	dataStream;

	if ((dataStream = fTicketResource.NewStream()) != nil) {
		long length = dataStream->GetDataLength();
		char* buffer = (char*)AllocateBuffer(length+1);

		CopyMemory(dataStream->GetData(), buffer, length);
		buffer[length] = 0;
		stream->WriteAttribute("wtv-ticket", buffer);
		FreeBuffer(buffer, length+1);
		delete(dataStream);
	}
	
	if ((dataStream = fChallengeResponse.NewStream()) != nil) {
		long length = dataStream->GetDataLength();
		char* buffer = (char*)AllocateBuffer(length+1);

		CopyMemory(dataStream->GetData(), buffer, length);
		buffer[length] = 0;
		stream->WriteAttribute("wtv-challenge-response", buffer);
		FreeBuffer(buffer, length+1);
		delete(dataStream);
		SetResponse(nil);
		SetSecure(true);
	}
}

void
Network::SetNotificationPending(Boolean value)
{
	fNotificationPending = value;
}

// ============================================================================

