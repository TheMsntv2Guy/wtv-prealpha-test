// Copyright(c) 1996 Artemis Research, Inc. All rights reserved.

#include "Headers.h"
#include "CacheStream.h"
#include "Cookie.h"
#include "DateTime.h"
#include "MemoryManager.h"
#include "Resource.h"
#include "Stream.h"
#include "URLParser.h"

// =============================================================================
// Constants

#define kCookieAddURL	"wtv-cookie:/add"
#define kCookieGetURL	"wtv-cookie:/get"
#define kCookieListURL	"wtv-cookie:/list"

// =============================================================================
// Global variables

CookieList* gCookieList;

// =============================================================================
// Class CookieList

CookieList::CookieList()
{
}

CookieList::~CookieList()
{
	delete(fStream);
}

Boolean
CookieList::CreateStream()
{
	if (fStream == nil) {
		Resource resource;
		resource.SetURL(kCookieListURL);
		fStream = resource.NewStreamForAppend();
		if (IsWarning(fStream == nil))
			return false;
	}
	
	// May get reset at some point...
	fStream->SetStatus(kComplete);
	return true;
}

void
CookieList::Append(const char* domain, const char* path)
{
	char* pathMinusFile = nil;
	char* slash;

	if (Exists(domain, path))
		return;
	
	if (!CreateStream())
		return;
	
	pathMinusFile = CopyString(path, "NewCookieResource");
	slash = strrchr(pathMinusFile, '/');
	if (slash != nil)
		*(slash + 1) = 0;

	fStream->FastForward();
	fStream->Write(domain, strlen(domain) + 1);
	fStream->Write(pathMinusFile, strlen(pathMinusFile) + 1);
	
	FreeTaggedMemory(pathMinusFile, "NewCookieResource");
}

#ifdef INCLUDE_FINALIZE_CODE
void
CookieList::Finalize()
{
	if (gCookieList != nil) {
		delete(gCookieList);
		gCookieList = nil;
	}
}
#endif /* INCLUDE_FINALIZE_CODE */

static Boolean
DomainMatch(const char* target, const char* domain)
{
	long targetLen = strlen(target);
	long domainLen = strlen(domain);
	
	if (domainLen - targetLen < 0)
		return false;
	
	domain += domainLen - targetLen;
	return EqualString(domain, target);
}

static Boolean
PathMatch(const char* target, const char* path)
{
	return EqualStringN(target, path, strlen(target));
}

Boolean
CookieList::Exists(const char* domain, const char* path)
{
	long dataLength;
	const char* data;
	const char* fence;
	
	if (!CreateStream())
		return false;
	
	if ((dataLength = fStream->GetDataLength()) == nil)
		return false;
	
	fStream->Rewind();
	data = fStream->GetData();
	fence = data + dataLength;
		
	while (data < fence) {
		const char* d = data;
		const char* p = (data += strlen(data) + 1);
		
		if (DomainMatch(d, domain) && PathMatch(p, path))
			return true;
		
		data += strlen(data) + 1;
	}
	
	return false;
}

void
CookieList::Initialize()
{
	if (IsError(gCookieList != nil))
		return;
		
	gCookieList = new(CookieList);
}

void
CookieList::Load()
{
	// Load the cookie list from the server
	
	Resource resource;
	
	resource.SetURL(kCookieListURL);
	resource.SetPriority(kPersistent);
	resource.SetStatus(kNoError);
}

Boolean
CookieList::LoadCookie(const Resource* target)
{
	// Load the cookie from the network for the specified resource. Returns
	// true while pending.
	
	Resource* resource;
	Error status;
	
	if ((resource = NewCookieResource(target)) == nil)
		return false;
	
	status = resource->GetStatus();
	
	// Start loading from the network.
	if (status == kNoError) {
		resource->SetPriority(kImmediate);
		delete(resource);
		return true;
	}
	
	// Check for an error or complete.
	if (TrueError(status) || status == kComplete) {
		delete(resource);
		return false;
	}
	
	// Otherwise, it is still pending.
	delete(resource);
	return true;
}

char*
CookieList::NewCookie(const Resource* target, const char* USED_FOR_MEMORY_TRACKING(tag))
{
	Resource* resource = nil;
	DataStream* stream = nil;
	char* cookie = nil;
	long length;
	
	if ((resource = NewCookieResource(target)) == nil)
		goto bail;
	
	if ((stream = resource->NewStream()) == nil)
		goto bail;
	
	if (stream->GetStatus() != kComplete)
		goto bail;
	
	if ((length = stream->GetDataLength()) == 0)
		goto bail;
	
	cookie = (char*)AllocateTaggedMemory(length + 1, tag);
	CopyMemory(stream->GetData(), cookie, length);
	cookie[length] = 0;

bail:
	delete(resource);
	delete(stream);	
	return cookie;
}

Resource*
CookieList::NewCookieResource(const char* domain, const char* path)
{
	Resource* resource = nil;
	MemoryStream* memory = nil;
	char* slash;
	char* pathMinusFile = nil;
	
	if (domain == nil)
		return nil;
	
	if (path == nil)
		path = "/";
	
	pathMinusFile = CopyString(path, "NewCookieResource");
	slash = strrchr(pathMinusFile, '/');
	*(slash + 1) = 0;
	
	if (!Exists(domain, pathMinusFile))
		goto bail;
	
	memory = new(MemoryStream);
	memory->WriteQuery("domain", domain);
	memory->WriteQuery("path", pathMinusFile);

	resource = new(Resource);
	resource->SetURL(kCookieGetURL, memory->GetData(), memory->GetDataLength());

bail:
	if (pathMinusFile)
		FreeTaggedMemory(pathMinusFile, "NewCookieResource");
	delete(memory);
	return resource;
}

Resource*
CookieList::NewCookieResource(const Resource* target)
{
	URLParser urlParser;
	char* url;
	
	url = target->CopyURL("URL");
	urlParser.SetURL(url);
	FreeTaggedMemory(url, "URL");
	return NewCookieResource(urlParser.GetDomain(), urlParser.GetPath());
}

void
CookieList::SendToServer(const char* cookie, const char* expires, const char* path, const char* domain)
{
	Resource resource;
	MemoryStream* memory;
	
	if (IsError(cookie == nil || path == nil || domain == nil))
		return;
	
	if (expires == nil)
		expires = "";
	
	memory = new(MemoryStream);
	memory->WriteQuery("cookie", cookie);
	memory->WriteQuery("expires", expires);
	memory->WriteQuery("path", path);
	memory->WriteQuery("domain", domain);

	resource.SetURL(kCookieAddURL, memory->GetData(), memory->GetDataLength());
	resource.SetPriority(kImmediate);
	resource.SetStatus(kNoError);

	Message(("CookieList::SendToServer: %s", memory->GetDataAsString()));	
	delete(memory);
}

static void
ParseSetCookie(char* input, const char** name, const char** expires, const char** path, const char** domain)
{
	// NAME=VALUE; expires=DATE; path=PATH; domain=DOMAIN_NAME; secure
	
	char* p;
	
	// Initialize return values.
	*name = *expires = *path = *domain = nil;
	
	// Set *cookie.
	*name = input;
	if ((input = FindCharacter(input, ";")) == nil)
		return;
	*input = 0;
	
	// Set remaining return values.
	for (;;) {
		input = SkipCharacters(input + 1, " ");

		if ((p = SkipString(input, "expires")) != nil)
			*expires = SkipCharacters(p, " =");
		else if ((p = SkipString(input, "path")) != nil)
			*path = SkipCharacters(p, " =");
		else if ((p = SkipString(input, "domain")) != nil)
			*domain = SkipCharacters(p, " =");

		if ((input = FindCharacter(input, ";")) == nil)
			return;
		*input = 0;
	}
}	

Boolean
CookieList::SetAttribute(const char* name, char* value, const Resource* base)
{
	Boolean result = false;
	Resource* resource = nil;
	DataStream* stream = nil;
	char* url = nil;
	const char* cookie;
	const char* expires;
	const char* path;
	const char* domain;
	
	if (IsError(name == nil || value == nil || base == nil))
		return false;

	if (!EqualString(name, "set-cookie"))
		return false;
	
	// Setup URL parser.
	URLParser urlParser;
	url = base->CopyURL("URL");
	urlParser.SetURL(url);

	// Determine cookie components.
	ParseSetCookie(value, &cookie, &expires, &path, &domain);
	if (IsWarning(cookie == nil))
		return false;
	if (path == nil)
		path = urlParser.GetPath();
	if (domain == nil)
		domain = urlParser.GetDomain();
	
	// Create cookie resource.	
	Append(domain, path);
	resource = NewCookieResource(domain, path);
	if (IsError(resource == nil))
		goto bail;
	stream = resource->NewStreamForWriting();
	if (IsError(stream == nil))
		goto bail;
	stream->SetExpires(DateTimeParser::Parse(expires));
	stream->WriteString(cookie);
	
	// Tell the world.
	SendToServer(cookie, expires, path, domain);
	result = true;
	
bail:
	if (url != nil)
		FreeTaggedMemory(url, "URL");
	delete(resource);
	delete(stream);
	return result;
}

#ifdef TEST_SIMULATORONLY
#ifdef DEBUG
void
CookieList::Test()
{
	char buffer[256];
	const char* cookie;
	const char* expires;
	const char* path;
	const char* domain;
	Resource* resource;
	char* url;
	Resource base;
	
	base.SetURL("http://www.artemis.com/index.html");
	
	strcpy(buffer, "type=chocolatechip");
	ParseSetCookie(buffer, &cookie, &expires, &path, &domain);
	Assert(EqualString(cookie, "type=chocolatechip"));
	Assert(expires == nil);
	Assert(path == nil);
	Assert(domain == nil);
	
	strcpy(buffer, "type=chocolatechip; expires=Wednesday, 09-Nov-99 23:12:40 GMT");
	ParseSetCookie(buffer, &cookie, &expires, &path, &domain);
	Assert(EqualString(cookie, "type=chocolatechip"));
	Assert(EqualString(expires, "Wednesday, 09-Nov-99 23:12:40 GMT"));
	Assert(path == nil);
	Assert(domain == nil);
	
	strcpy(buffer, "type=chocolatechip; path=/foo/; expires=Wednesday, 09-Nov-99 23:12:40 GMT");
	ParseSetCookie(buffer, &cookie, &expires, &path, &domain);
	Assert(EqualString(cookie, "type=chocolatechip"));
	Assert(EqualString(expires, "Wednesday, 09-Nov-99 23:12:40 GMT"));
	Assert(EqualString(path, "/foo/"));
	Assert(domain == nil);
	
	strcpy(buffer, "type=chocolatechip; expires=Wednesday, 09-Nov-99 23:12:40 GMT; path=/foo/; domain=artemis.com");
	ParseSetCookie(buffer, &cookie, &expires, &path, &domain);
	Assert(EqualString(cookie, "type=chocolatechip"));
	Assert(EqualString(expires, "Wednesday, 09-Nov-99 23:12:40 GMT"));
	Assert(EqualString(path, "/foo/"));
	Assert(EqualString(domain, "artemis.com"));
	
	strcpy(buffer, "type=chocolatechip; expires=Wednesday, 09-Nov-99 23:12:40 GMT; domain=artemis.com; path=/foo/; secure");
	ParseSetCookie(buffer, &cookie, &expires, &path, &domain);
	Assert(EqualString(cookie, "type=chocolatechip"));
	Assert(EqualString(expires, "Wednesday, 09-Nov-99 23:12:40 GMT"));
	Assert(EqualString(path, "/foo/"));
	Assert(EqualString(domain, "artemis.com"));
	
	resource = gCookieList->NewCookieResource("artemis.com", "/");
	Assert(resource == nil);
	
	Assert(DomainMatch("artemis.com", "artemis.com"));
	Assert(DomainMatch("artemis.com", "foo.artemis.com"));
	Assert(!DomainMatch("foo.artemis.com", "artemis.com"));
	Assert(PathMatch("/", "/"));
	Assert(PathMatch("/", "/foo"));
	Assert(PathMatch("/foo", "/foo/bar.html"));
	Assert(!PathMatch("/foo", "/"));
	
	strcpy(buffer, "artemisname=artemisvalue; path=/; domain=artemis.com");
	gCookieList->SetAttribute("Set-Cookie", buffer, &base);
	Assert(gCookieList->Exists("artemis.com", "/"));
	Assert(gCookieList->Exists("foo.artemis.com", "/foo/"));

	strcpy(buffer, "applename=applevalue; path=/pub/; domain=apple.com");
	gCookieList->SetAttribute("Set-Cookie", buffer, &base);
	Assert(gCookieList->Exists("apple.com", "/pub/"));
	Assert(gCookieList->Exists("apple.com", "/pub/foo/"));
	Assert(gCookieList->Exists("foo.apple.com", "/pub/"));
	Assert(gCookieList->Exists("foo.apple.com", "/pub/foo"));
	Assert(!gCookieList->Exists("foo.apple.com", "/"));
	Assert(!gCookieList->Exists("foo.com", "/"));

	resource = gCookieList->NewCookieResource("artemis.com", "/");
	if (!IsError(resource == nil)) {
		url = resource->CopyURL("url");
		Assert(EqualString(url, kCookieGetURL));
		Assert(EqualStringN(resource->GetPostData(), "domain=artemis.com&path=/", strlen("domain=artemis.com&path=/")));
		FreeTaggedMemory(url, "url");
		delete(resource);
	}
	
	resource = gCookieList->NewCookieResource("artemis.com", "/foo/");
	if (!IsError(resource == nil)) {
		url = resource->CopyURL("url");
		Assert(EqualString(url, kCookieGetURL));
		Assert(EqualStringN(resource->GetPostData(), "domain=artemis.com&path=/foo/", strlen("domain=artemis.com&path=/foo/")));
		FreeTaggedMemory(url, "url");
		delete(resource);
	}
	
	resource = gCookieList->NewCookieResource("artemis.com", "/foo/bar.html");
	if (!IsError(resource == nil)) {
		url = resource->CopyURL("url");
		Assert(EqualString(url, kCookieGetURL));
		Assert(EqualStringN(resource->GetPostData(), "domain=artemis.com&path=/foo/", strlen("domain=artemis.com&path=/foo/")));;
		FreeTaggedMemory(url, "url");
		delete(resource);
	}
}
#endif
#endif

// =============================================================================

