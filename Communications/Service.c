// =============================================================================
// Service.c
//
// Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// =============================================================================

#include "Headers.h"
#include "MemoryManager.h"
#include "Service.h"
#include "Socket.h"
#include "System.h"
#include "URLParser.h"
#include "Network.h"
#include "WTVProtocol.h"

#ifdef DEBUG_BOXPRINT
#include "BoxPrintDebug.h"
#endif

// =============================================================================
// Constants

#define kSuspendTime	(60 * kOneSecond)

// =============================================================================
// Global variables

ServiceList* gServiceList;

// =============================================================================
// Class Service

Service::Service()
{
}

Service::~Service()
{
	if (fHostName != nil)
		FreeTaggedMemory(fHostName, "Service::fHostName");
		
	if (fName != nil)
		FreeTaggedMemory(fName, "Service::fName");
	
	IsError(fUserCount != -1); // to catch bugs -- see EndUse
}

void
Service::Append(Service* service)
{
	if (fNext != nil)
		fNext->Append(service);
	
	fNext.SetService(service);
}

#ifdef DEBUG_BOXPRINT
void
Service::BoxPrintDebug(long whatToPrint) const
{
	if (whatToPrint == 0)
		whatToPrint = kBoxPrintName + kBoxPrintHostInfo;

	if (whatToPrint & kBoxPrintHostInfo) {
		const char* hostName = fHostName;
		if (hostName == nil)
			hostName = "<no host name>";

		BoxPrint("Host:  %d.%d.%d.%d:%hu (%s)",
					(fHostAddress >> 24) & 0x0ff,
					(fHostAddress >> 16) & 0x0ff,
					(fHostAddress >> 8) & 0x0ff,
					fHostAddress & 0x0ff,
					fHostPort,
					hostName);
	}

	if (whatToPrint & kBoxPrintName) {
		const char* name = fName;
		if (name == nil)
			name = "<no name>";
		BoxPrint("Service Name:  %s", name);
	}

	if (whatToPrint & kBoxPrintUserCount)
		BoxPrint("User Count:  %hd", fUserCount);
}
#endif

void
Service::ComputeProtocolClassNumber()
{
	if (fIsProxy)
		fProtocolClassNumber = kClassNumberWTVProtocol;
	else if (EqualStringN(fName, "wtv-", 4))
		fProtocolClassNumber = kClassNumberWTVProtocol;
	else if (EqualStringN(fName, "wtvs-", 5))
		fProtocolClassNumber = kClassNumberWTVProtocol;
	else
		fProtocolClassNumber = kClassNumberHTTPProtocol;
}

void 
Service::EndUse()
{
	if (IsError(fUserCount == 0))
		return;
		
	if (--fUserCount == 0) {
		fUserCount = -1; // to catch bugs -- see destructor
		delete(this);
	}
}

Service*
Service::GetNext() const
{
	return fNext;
}

ClassNumber 
Service::GetProtocolClassNumber() const
{
	if (fProtocolClassNumber == 0)
		((Service*)this)->ComputeProtocolClassNumber();

	return fProtocolClassNumber;
}

Boolean
Service::IsActive() const
{
	if (fReactivateTime == 0)
		return true;
	
	if (fReactivateTime < Now())
		return true;
	
	return false;
}

Boolean 
Service::IsEqual(const Service* service) const
{
	if (service == this)
		return true;

	if (service->fHostPort != fHostPort)
		return false;
		
	if (fHostAddress != 0 && service->fHostAddress != 0)
		return service->fHostAddress == fHostAddress;
		
	return EqualString(service->fHostName, fHostName);
}

Protocol* 
Service::NewProtocol() const
{
	switch (GetProtocolClassNumber()) {
	case kClassNumberHTTPProtocol:	return new(HTTPProtocol);
	case kClassNumberWTVProtocol:	return new(WTVProtocol);
	default:						IsError("NewProtocol: unknown protocol");
	}
	
	return new(HTTPProtocol);
}

Service*
Service::NewService(const Resource* resource)
{
	Service* service;
	char* url;
	
	if (IsError(resource == nil))
		return nil;
	
	url = resource->CopyURL("Service::NewService");
	URLParser urlParser;
	urlParser.SetURL(url);
	FreeTaggedMemory(url, "Service::NewService");

	service = new(Service);
	service->SetHostName(urlParser.GetDomain());
	service->SetHostPort(urlParser.GetPort());
	service->SetName(urlParser.GetProtocolText());
	return service;
}

Socket*
Service::NewSocket(Error* error)
{
	Socket* socket;

	if ((socket = Socket::NewSocket()) == nil) {
		*error = kTooManyConnections;
		return nil;
	}
	
	Message(("Network: connecting to %s %s:%d",
		fName ? fName : "", fHostName ? fHostName : "", fHostPort));

	if(fHostName == "10.0.0.2") {
		fHostAddress = 0x0a000002;
	}
	
	if (fHostAddress == 0) {
		*error = socket->Connect(fHostName, fHostPort);
		SetHostAddress(socket->GetHostAddress());
	} else
		*error = socket->Connect(fHostAddress, fHostPort);

	if (*error != kNoError) {
		socket->Dispose();
		return nil;
	}

	return socket;	
}

void 
Service::SetHostAddress(ulong value)
{
	fHostAddress = value;
	SetDebugModifiedTime();
}

void
Service::SetHostName(const char* nameOrAddress)
{
	if (nameOrAddress == nil)
		return;
		
	if (isdigit(*nameOrAddress))
		fHostAddress = ParseAddress(nameOrAddress);

	fHostName = CopyStringTo(fHostName, nameOrAddress, "Service::fHostName");
	SetDebugModifiedTime();
	return;
}

void 
Service::SetHostPort(ushort value)
{
	SetDebugModifiedTime();
	fHostPort = value;
}

void 
Service::SetIsProxy(Boolean value)
{
	SetDebugModifiedTime();
	fIsProxy = value;
}

void
Service::SetIsEncrypted(Boolean value)
{
	SetDebugModifiedTime();
	fIsEncrypted = value;
}

void 
Service::SetName(const char* value)
{
	SetDebugModifiedTime();
	fName = CopyStringTo(fName, value, "Service::fName");
	/* Mac : determine a better way to do this */
	PostulateFinal(false);
	if (IsNamed("wtv-1800") || IsNamed("wtv-head-waiter") 
		|| IsNamed("wtv-flashrom") || IsNamed("wtv-log"))
			SetIsEncrypted(false);
	else
		if (gNetwork->IsSecure()) 
			SetIsEncrypted(true);
}

void
Service::SetProtocolClassNumber(ClassNumber classNumber)
{
	SetDebugModifiedTime();
	fProtocolClassNumber = classNumber;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
Service::StaticBoxPrintDebug(const Service* service, long whatToPrint)
{
	if (service == nil) {
		BoxPrint("Service: <nil>");
	} else {
		BoxPrint("Service: <%#06x>", service);
		AdjustBoxIndent(1);
		service->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif

void
Service::Suspend()
{
	fReactivateTime = Now() + kSuspendTime;
}

// =============================================================================
// Class ServiceList

ServiceList::ServiceList()
{
}

ServiceList::~ServiceList()
{
	fServiceList.DeleteAll();
}

void 
ServiceList::Add(Service* service)
{
	ServicePointer* pointer;
	Service* parent;
	
	if (IsError(service == nil))
		return;

	// Add an alternate.	
	if ((parent = FindByName(service->GetName())) != nil) {
		parent->Append(service);
		return;
	}
	
	// Add a service.
	pointer = new(ServicePointer);
	pointer->SetService(service);
	fServiceList.Add(pointer);
	SetDebugModifiedTime();
}		

void 
ServiceList::Add(const char* serviceName, const char* nameOrAddress, ushort port)
{
	Service* service = new(Service);
	if (!(IsError(service == nil))) {
		service->SetName(serviceName);
		service->SetHostName(nameOrAddress);
		service->SetHostPort(port);
		Add(service);
	}
}

#ifdef DEBUG_BOXPRINT
void
ServiceList::BoxPrintDebug(long whatToPrint) const
{
	int count = GetCount();
	for (int index=0; index<count; index++) {
		Service* service = At(index);
		if (service != nil) {
			service->BoxPrintDebug(whatToPrint);
		}
	}
}
#endif

Service* 
ServiceList::FindByName(const char* name) const
{
	long count;
	long i;
	
	if (IsWarning(name == nil || *name == 0))
		return nil;
	
	count = GetCount();
	for (i = 0; i < count; i++) {
		Service* service = At(i);
#ifdef DEBUG
		if (((long*)service)[-1] > 0)
			Complain(("***Freed block 0x%08x in service list", (int)&(((long*)service)[-1]) ));
#endif
		if (service->IsNamed(name))
			return service;
	}

	return nil;
}

ServicePointer 
ServiceList::FindByResource(const Resource* resource) const
{
	ServicePointer service;
	char* url;

	if (IsWarning(resource == nil))
		return nil;

	// Get service name.	
	url = resource->CopyURL("ServiceList::Find");
	service = FindByURL(url);
	FreeTaggedMemory(url, "ServiceList::Find");
	return service;
}

ServicePointer
ServiceList::FindByURL(const char* url) const
{
	URLParser urlParser;
	Service* service;
	Service* serviceList;
	const char* name;
	
	urlParser.SetURL(url);
	name = urlParser.GetProtocolText();
	
	// Find an existing, active service.
	serviceList = FindByName(name);
	for (service = serviceList; service != nil; service = service->GetNext())
		if (service->IsActive())
			return service;
	
	// Try to go directly to the http server.
	if (EqualString(name, "http")) {
		Resource resource;
		resource.SetURL(url);
		return Service::NewService(&resource);
	}
	
	// Try to go to the wildcard service.
	if ((service = FindByName("wtv-*")) != nil)
		return service;

	// Give up.
	return nil;
}

Boolean
ServiceList::HasService(const Resource* base, const char* partial) const
{
	Boolean isDataTrusted;
		
		if (base == nil)
		return false;
	
	isDataTrusted = base->IsDataTrusted();
			
	if (partial != nil) {
		char* url;
		Boolean result;
		URLParser urlParser;

		urlParser.SetURL(base->GetURL());
		url = urlParser.NewURL(partial, "ServiceList::HasService");
		result = HasService(url, isDataTrusted);
		FreeTaggedMemory(url, "ServiceList::HasService");
		return result;
	}
		
	return HasService(base->GetURL(), isDataTrusted);
}

Boolean
ServiceList::HasService(const char* url, Boolean USED_FOR_NONDEBUG(isDataTrusted)) const
{
	long count;
	long i;
	
	if (IsWarning(url == nil))
		return false;
	
	if (EqualStringN(url, "http:", 5))
		return true;

	count = GetCount();
	for (i = 0; i < count; i++) {
		Service* service = At(i);
		const char* name = service->GetName();
		long len = strlen(name);
		
		if (EqualStringN(url, name, len) && url[len] == ':') {
#ifndef DEBUG
			if (EqualStringN(name, "wtv-", 4))
				return isDataTrusted;
#endif
			return true;
		}
	}
	
#ifndef DEBUG
	if (!isDataTrusted)
		return false;
#endif

	if (EqualStringN(url, "client:", 7))
		return true;

	if (EqualStringN(url, "file://rom/", 11))
		return true;

	return false;
}

char*
ServiceList::NewSendURL(const char* url, const char* USED_FOR_MEMORY_TRACKING(tag)) const
{
	if (EqualStringN(url, "wtv-", 4))
		return nil;
	
	if (EqualStringN(url, "wtvs-", 5))
		return nil;
	
	return CopyString(url, tag);
}

char*
ServiceList::NewSendURL(const Resource* base, const char* partial, const char* tag) const
{
	URLParser urlParser;
	char* result;
	char* url;
	
	if (base == nil)
		return NewSendURL(partial, tag);
	
	urlParser.SetURL(base->GetURL());
	url = urlParser.NewURL(partial, "ServiceList::NewSendURL");
	result = NewSendURL(url, tag);
	FreeTaggedMemory(url, "ServiceList::NewSendURL");
	return result;
}

#ifdef DEBUG_BOXPRINT
void
ServiceList::StaticBoxPrintDebug(const ServiceList* serviceList, long whatToPrint)
{
	if (serviceList == nil) {
		BoxPrint("ServiceList: <nil>");
	} else {
		BoxPrint("ServiceList: <%#06x>", serviceList);
		AdjustBoxIndent(1);
		serviceList->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif

Boolean
ServiceList::SetAttribute(const char* name, char* value)
{
	Service* service;
	const char* p;
	char* space;
	
	if (!EqualString(name, "wtv-service"))
		return false;
	
	if (EqualString(value, "reset")) {
		fServiceList.DeleteAll();
		return true;
	}
		
	SetDebugModifiedTime();
	service = new(Service);

	if (IsError(service == nil))
		return false;

	service->SetIsProxy(true);
	service->SetProtocolClassNumber(kClassNumberWTVProtocol);
	
	p = SkipString(value, "name");
	p = SkipCharacters(p, ":= ");
	*FindCharacter(p, " ") = 0;
	service->SetName(p);
	p += strlen(p) + 1;
	
	p = SkipString(p, "host");
	p = SkipCharacters(p, ":= ");
	*FindCharacter(p, " ") = 0;
	service->SetHostName(p);
	p += strlen(p) + 1;
	
	p = SkipString(p, "port");
	p = SkipCharacters(p, ":= ");
	if ((space = FindCharacter(p, " ")) != nil) // allow for extensibility
		*space = 0;
	service->SetHostPort((ushort)atoi(p));
	p += strlen(p) + 1;

	Add(service);
	return true;
}

#if defined(TEST_SIMULATORONLY) && defined(DEBUG)
void
ServiceList::Test()
{
	char* url;
	
	url = NewSendURL(nil, "http://artemis/foo.html", "ServiceList::Test");
	Assert(EqualString(url, "http://artemis/foo.html"));
	FreeTaggedMemory(url, "ServiceList::Test");
	
	url = NewSendURL(nil, "wtv-home:/home", "ServiceList::Test");
	Assert(url == nil);
}
#endif

// =============================================================================
// Class ServicePointer

ServicePointer::ServicePointer()
{
	// Enable stack allocation
	fService = nil;
}

ServicePointer::ServicePointer(const ServicePointer& other)
{
	fService = other.fService;

	if (fService != nil)
		fService->BeginUse();
}

ServicePointer::ServicePointer(Service* service)
{
	fService = service;
	
	if (fService != nil)
		fService->BeginUse();
}

ServicePointer::~ServicePointer()
{
	if (fService != nil)
		fService->EndUse();
}

ServicePointer& 
ServicePointer::operator=(const ServicePointer& other)
{
	if (this == &other)
		return *this;
	
	if (fService != nil)
		fService->EndUse();
	
	fService = other.fService;

	if (fService != nil)
		fService->BeginUse();
	
	return *this;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline Service&
ServicePointer::operator*() const
{
	if (IsError(fService == nil)) {
		ServicePointer* This = (ServicePointer*)this;
		This->SetService(new(Service));
	}
	
	return *fService;
}
#endif

Boolean
ServicePointer::IsEqual(const Service* service) const
{
	if (IsWarning(service == nil))
		return false;
		
	if (fService == nil)
		return false;
	
	return fService->IsEqual(service);
}

void 
ServicePointer::Reset()
{
	if (fService == nil)
		return;

	fService->EndUse();
	fService = nil;
}

void
ServicePointer::SetService(Service* service)
{
	if (fService != nil)
		fService->EndUse();
	
	if ((fService = service) != nil)
		fService->BeginUse();
}

// =============================================================================

