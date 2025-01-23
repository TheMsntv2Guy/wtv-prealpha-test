// Copyright(c) 1996 Artemis Research, Inc. All rights reserved.

#include "Headers.h"
#include "CacheStream.h"
#include "Authorization.h"
#include "DateTime.h"
#include "MemoryManager.h"
#include "Resource.h"
#include "Stream.h"
#include "URLParser.h"

// =============================================================================
// Global variables

AuthorizationList* gAuthorizationList;

// =============================================================================
// Class AuthorizationList

AuthorizationList::AuthorizationList()
{
}

AuthorizationList::~AuthorizationList()
{
	delete(fStream);
}

void
AuthorizationList::Add(const Resource* resource, const char* realm, const char* name, const char* password)
{
	URLParser parser;
	char* url;
	const char* domain;
	MemoryStream* stream;
	char* authorization;
	
	if (IsWarning(resource == nil || realm == nil || name == nil || password == nil))
		return;

	if (!CreateStream())
		return;
	
	// Get domain from resource.
	url = resource->CopyURL("URL");
	parser.SetURL(url);
	FreeTaggedMemory(url, "URL");
	domain = parser.GetDomain();
	if (IsWarning(domain == nil))
		return;
	
	if (Exists(domain, realm))
		return;
	
	// Create Base64 authorization.
	stream = new(MemoryStream);
	stream->WriteString(name);
	stream->WriteString(":");
	stream->WriteString(password);
	authorization = NewBase64String(stream->GetDataAsString(), "AuthorizationList::Add");
	delete(stream);

	// Add domain, realm, and authorization to the list.
	fStream->FastForward();
	fStream->Write(domain, strlen(domain) + 1);
	fStream->Write(realm, strlen(realm) + 1);
	fStream->Write(authorization, strlen(authorization) + 1);
	FreeTaggedMemory(authorization, "AuthorizationList::Add");
}

Boolean
AuthorizationList::CreateStream()
{
	if (fStream == nil) {
		Resource resource;
		resource.SetURL("cache:authorization-list");
		fStream = resource.NewStreamForAppend();
		if (IsWarning(fStream == nil))
			return false;
	}
	
	// May get reset at some point...
	fStream->SetStatus(kComplete);
	return true;
}

#ifdef INCLUDE_FINALIZE_CODE
void
AuthorizationList::Finalize()
{
	if (gAuthorizationList != nil) {
		delete(gAuthorizationList);
		gAuthorizationList = nil;
	}
}
#endif /* INCLUDE_FINALIZE_CODE */

Boolean
AuthorizationList::Exists(const char* targetDomain, const char* targetRealm)
{
	long dataLength;
	const char* data;
	const char* fence;

	if (IsWarning(targetDomain == nil))
		return false;
	
	if (IsWarning(targetRealm == nil))
		return false;
	
	if (!CreateStream())
		return false;
	
	if ((dataLength = fStream->GetDataLength()) == 0)
		return false;
	
	fStream->Rewind();
	data = fStream->GetData();
	fence = data + dataLength;
		
	while (data < fence) {
		const char* domain = data;
		const char* realm = (data += strlen(data) + 1);
		
		if (EqualString(targetDomain, domain) && EqualString(targetRealm, realm))
			return true;
		
		data += strlen(data) + 1;
	}
	
	return false;
}

char*
AuthorizationList::NewAuthorization(const Resource* resource, const char* targetRealm, const char* USED_FOR_MEMORY_TRACKING(tag))
{
	URLParser parser;
	long dataLength;
	const char* data;
	const char* fence;
	const char* targetDomain;
	char* url;

	if (IsWarning(resource == nil))
		return nil;
	
	if (IsWarning(targetRealm == nil))
		return nil;
	
	if (!CreateStream())
		return nil;
	
	if ((dataLength = fStream->GetDataLength()) == 0)
		return nil;
	
	// Get domain from resource.
	url = resource->CopyURL("URL");
	parser.SetURL(url);
	FreeTaggedMemory(url, "URL");
	targetDomain = parser.GetDomain();
	if (IsWarning(targetDomain == nil))
		return nil;
	
	fStream->Rewind();
	data = fStream->GetData();
	fence = data + dataLength;
		
	while (data < fence) {
		const char* domain = data;
		const char* realm = (data += strlen(data) + 1);
		const char* authorization = (data += strlen(data) + 1);
		
		if (EqualString(targetDomain, domain) && EqualString(targetRealm, realm))
			return CopyString(authorization, tag);
		
		data += strlen(data) + 1;
	}
	
	return nil;
}

void
AuthorizationList::Initialize()
{
	if (IsError(gAuthorizationList != nil))
		return;
		
	gAuthorizationList = new(AuthorizationList);
}

#ifdef TEST_SIMULATORONLY
#ifdef DEBUG
void
AuthorizationList::Test()
{
	Resource resource;
	char* authorization;
	
	resource.SetURL("http://domainA/foo.html");
	gAuthorizationList->Add(&resource, "realmA", "nameA", "passwordA");
	Assert(gAuthorizationList->Exists("domainA", "realmA"));
	authorization = gAuthorizationList->NewAuthorization(&resource, "realmA", "Authorization");
	Assert(authorization != nil);
	if (authorization != nil)
		FreeTaggedMemory(authorization, "Authorization");
	Assert(!gAuthorizationList->Exists("domainA", "realmB"));
	Assert(!gAuthorizationList->Exists("domainB", "realmA"));}
#endif
#endif

// =============================================================================

