// ===========================================================================
//	URLParser.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __URLPARSER_H__
#include "URLParser.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#define INTENTIONAL_RFC1808_VIOLATION
#define DOT_DOT_SLASH_VIOLATION

// =============================================================================

const char* kURLProtocolFileString = "file";
const char* kURLProtocolFTPString = "ftp";
const char* kURLProtocolHTTPString = "http";

const ulong kURLProtocolFilePort = 0;
const ulong kURLProtocolFTPPort = 0;
const ulong kURLProtocolHTTPPort = 80;

const char* kURLProtocolAbsentString = "http";
const ulong kURLProtocolAbsentPort = 80;

const ulong kURLProtocolUnknownPort = 80;

// =============================================================================

typedef struct
{
	URLProtocol		protocol;
	const char* 	name;
	ulong			port;
}
URLProtocolTable;

static const URLProtocolTable gURLProtocolTable[kNumURLProtocols] =
{
	{ kURLProtocolFile,		"file",		kURLProtocolFilePort },
	{ kURLProtocolFTP,		"ftp",		kURLProtocolFTPPort },
	{ kURLProtocolHTTP,		"http",		kURLProtocolHTTPPort }
};

// =============================================================================

#define kBaseURLTag				"URLParser::fBaseURL"
#define kBaseURLNoFragmentTag	"URLParser::fBaseURLNoFragment"
#define kURLPartsTag			"URLParser::fURLParts"
#define kURLParserPseudoURL		"<URLParser>"

// =============================================================================

URLParser::URLParser()
{
	fBaseURL = nil;				// these are the only three that can allocate
	fBaseURLNoFragment = nil;	// memory so make sure they are nil or that
	fURLParts = nil;	 		// they point to a valid alloc block
}


URLParser::~URLParser()
{
	// if there is no fragment, fBaseURLNoFragment and fBaseURL point to the
	// same space in memory
	if ((fBaseURLNoFragment != nil) && (fBaseURLNoFragment != fBaseURL))
		FreeTaggedMemory(fBaseURLNoFragment, kBaseURLNoFragmentTag);

	if (fBaseURL != nil)
		FreeTaggedMemory(fBaseURL, kBaseURLTag);
	if (fURLParts != nil)
		FreeTaggedMemory(fURLParts, kURLPartsTag);
}

const char*
URLParser::GetDomain() const
{
	IsWarning(fBaseURL == nil);
	return fDomain;
}

const char*
URLParser::GetFragment() const
{
	IsWarning(fBaseURL == nil);
	return fFragment;
}

const char*
URLParser::GetPath() const
{
	IsWarning(fBaseURL == nil);
	return fPath;
}

const char*
URLParser::GetPathPlus() const
{
	IsWarning(fBaseURL == nil);
	
	if (fBaseURLPathPlus == nil)
		return "/";

	return fBaseURLPathPlus;
}

long
URLParser::GetPort() const
{
	IsWarning(fBaseURL == nil);
	return fPortNumber;
}

#ifdef TEST_SIMULATORONLY
URLProtocol
URLParser::GetProtocol() const
{
	IsWarning(fBaseURL == nil);
	return fURLProtocol;
}
#endif

const char*
URLParser::GetProtocolText() const
{
	IsWarning(fBaseURL == nil);
	return fURLProtocolText;
}

const char*
URLParser::GetURL() const
{
	return fBaseURL;
}

const char*
URLParser::GetURLNoFragment() const
{
	IsWarning(fBaseURL == nil);
	return fBaseURLNoFragment;
}

char*
URLParser::NewURL(const char* partial, const char* USED_FOR_MEMORY_TRACKING(tag))
{
	const char* protocol = nil;
	const char* domain = nil;
	const char* port = nil;
	const char* path1 = nil;
	const char* path2 = nil;
	const char* params = nil;
	const char* query = nil;
	const char* fragment = nil;
	Boolean		alwaysFalse = false;

	URLParser tempParser;
	tempParser.SetURL(partial);

	//   Step 1: The base URL is established according to the rules of
	//           Section 3.  If the base URL is the empty string (unknown),
	//           the embedded URL is interpreted as an absolute URL and
	//           we are done.
	//
	//   Step 2: Both the base and embedded URLs are parsed into their
	//           component parts as described in Section 2.4.
	//
	//           (a) If the embedded URL is entirely empty, it inherits the
	//              entire base URL (i.e., is set equal to the base URL)
	//              and we are done.
	//
	//           (b) If the embedded URL starts with a scheme name, it is
	//              interpreted as an absolute URL and we are done.

	if ((fBaseURL == nil) || (fBaseURL == '\0'))
	{
		goto ResolveWithURL_embedded_protocol;
	}

	if ((tempParser.fBaseURL == nil) || (tempParser.fBaseURL == '\0'))
	{
		goto ResolveWithURL_base_protocol;
	}


	if (tempParser.fURLProtocol != kURLProtocolAbsent)
	{
#if defined(INTENTIONAL_RFC1808_VIOLATION)
		if (EqualString(tempParser.fURLProtocolText, fURLProtocolText) && (tempParser.fDomain == nil))
		{
			//	John and Mick think that despite the spec, from
			//
			//		http://foo.com/this/here/path
			//
			//	the partial URL
			//
			//		http:bar.html
			//
			//	should be parsed as:
			//
			//		http://foo.com/this/here/bar.html
			//
			// i.e., if the partial URL's protocol matches the base protocol
			// **AND** the partial URL has no domain, pretend the partial
			// URL didn't specify the protocol.
		}
		else
#endif
			goto ResolveWithURL_embedded_protocol;
	}

	//           (c) Otherwise, the embedded URL inherits the scheme of
	//              the base URL.

	protocol = fURLProtocolText;

	//   Step 3: If the embedded URL's <net_loc> is non-empty, we skip to
	//           Step 7.  Otherwise, the embedded URL inherits the <net_loc>
	//           (if any) of the base URL.

	if (tempParser.fDomain != nil)
	{
		goto ResolveWithURL_embedded_domain;
	}

	domain = fDomain;
	port = fPortText;

	//   Step 4: If the embedded URL path is preceded by a slash "/", the
	//           path is not relative and we skip to Step 7.

	if ((tempParser.fPath != nil) && (tempParser.fPath[0] == '/'))
	{
		goto ResolveWithURL_embedded_path;
	}

	//   Step 5: If the embedded URL path is empty (and not preceded by a
	//           slash), then the embedded URL inherits the base URL path,
	//           and
	//
	//           (a) if the embedded URL's <params> is non-empty, we skip to
	//              step 7; otherwise, it inherits the <params> of the base
	//              URL (if any) and
	//
	//           (b) if the embedded URL's <query> is non-empty, we skip to
	//              step 7; otherwise, it inherits the <query> of the base
	//              URL (if any) and we skip to step 7.


	path1 = fPath;
	
	if (tempParser.fPath == nil)	// if embedded is nil, take base outright
	{
		if (tempParser.fParams != nil)
		{
			goto ResolveWithURL_embedded_params;
		}
		params = fParams;
		
		if (tempParser.fQuery != nil)
		{
			goto ResolveWithURL_embedded_query;
		}
		query = fQuery;

		if (tempParser.fFragment != nil)
		{
			goto ResolveWithURL_embedded_fragment;
		}
		fragment = fFragment;
	}
	else // otherwise put embedded in path2 and resolve later
	{
		path2 = tempParser.fPath;
		goto ResolveWithURL_embedded_params;
	}

// we might have jumped and landed somewhere in here...
	if (alwaysFalse)
	{
ResolveWithURL_embedded_protocol:
		protocol = tempParser.fURLProtocolText;
ResolveWithURL_embedded_domain:
		domain = tempParser.fDomain;
		port = tempParser.fPortText;
ResolveWithURL_embedded_path:
		path1 = tempParser.fPath;
ResolveWithURL_embedded_params:
		params = tempParser.fParams;
ResolveWithURL_embedded_query:
		query = tempParser.fQuery;
ResolveWithURL_embedded_fragment:
		fragment = tempParser.fFragment;
	}
// or we might have jumped and landed somewhere in here...
	if (alwaysFalse)
	{
ResolveWithURL_base_protocol:
		protocol = fURLProtocolText;
		domain = fDomain;
		port = fPortText;
		path1 = fPath;
		params = fParams;
		query = fQuery;
		fragment = fFragment;
	}
	
	size_t resultSize = 2;	// for null termination and possible path '/'
	
	if (protocol != nil)
						resultSize += strlen(protocol) + 1;		// +1 for ":"
	if (domain != nil)
						resultSize += strlen(domain) + 2;		// +2 for "//"
	if (port != nil)
						resultSize += strlen(port) + 1;			// +1 for ":"
	if (path1 != nil)
						resultSize += strlen(path1);			// +1 for "/"
	if (path2 != nil)
						resultSize += strlen(path2);	// this is slightly more than we 
	if (params != nil)									// need, but only a very little more...
						resultSize += strlen(params) + 1;		// +1 for ";"
	if (query != nil)
						resultSize += strlen(query) + 1;		// +1 for "?"
	if (fragment != nil)
						resultSize += strlen(fragment) + 1;		// +1 for "#"
	

	PushDebugChildURL(kURLParserPseudoURL);
	char* result = (char*)AllocateTaggedMemory(resultSize, tag);
	PopDebugChildURL();

//   Step 6: The last segment of the base URL's path (anything
//           following the rightmost slash "/", or the entire path if no
//           slash is present) is removed and the embedded URL's path is
//           appended in its place.  The following operations are
//           then applied, in order, to the new path:
//
//           (a) All occurrences of "./", where "." is a complete path
//              segment, are removed.
//
//           (b) If the path ends with "." as a complete path segment,
//              that "." is removed.
//
//           (c) All occurrences of "<segment>/../", where <segment> is a
//              complete path segment not equal to "..", are removed.
//              Removal of these path segments is performed iteratively,
//              removing the leftmost matching pattern on each iteration,
//              until no matching pattern remains.
//
//           (d) If the path ends with "<segment>/..", where <segment> is a
//              complete path segment not equal to "..", that
//              "<segment>/.." is removed.
//
//   Step 7: The resulting URL components, including any inherited from
//           the base URL, are recombined to give the absolute form of
//           the embedded URL.

	
	const char* resultSource;
	char* resultDest = result;
	
	if ((resultSource = protocol) != nil)
	{	
		while (*resultSource)
		{	*resultDest++ = *resultSource++;
		}
		*resultDest++ = ':';
	}
	if ((resultSource = domain) != nil)
	{	
		*resultDest++ = '/';
		*resultDest++ = '/';
		while (*resultSource)
		{	*resultDest++ = *resultSource++;
		}
	}
	if ((resultSource = port) != nil)
	{	
		*resultDest++ = ':';
		while (*resultSource)
		{	*resultDest++ = *resultSource++;
		}
	}
	
	// Step 6 nightmare starts here -------------------------------------
	
	if ((path1 == nil) || (*path1 == '\0'))
	{
		path1 = path2;
		path2 = nil;
	}
	
	if ((path1 == nil) || (*path1 == '\0'))
	{
		// no paths!
	}
	else if (((path2 == nil) || (*path2 == '\0')) && (*path1 == '/'))
	{
		// absolute path...copy it "as is"!
		resultSource = path1;
		while (*resultSource)
		{	*resultDest++ = *resultSource++;
		}
	}
	else	// gotta merge at least one relative path
	{	
		size_t pathLength = 0;
		if ((*path1 != '/') && (domain != nil) && (*domain != '\0'))
		{	resultDest[pathLength]='/';
			pathLength++;
		}
		strcpy(resultDest+pathLength, path1);
		pathLength += strlen(path1);
		
		char* temp = &resultDest[pathLength];
		while ((pathLength > 0) && (*--temp != '/'))	// remove everything up to last '/'
		{	pathLength--;
		}
		
		if (path2 != nil)
		{
			strcpy(resultDest+pathLength, path2);
			pathLength += strlen(resultDest+pathLength);
		}
		
		while (EqualStringN(resultDest, "./", 2))			// Step 6a
		{	DeletePrefix(resultDest, 2);					// remove leading "./"
			pathLength -= 2;
		}
		while ((temp = strstr(resultDest, "/./")) != nil)	// More Step 6a
		{	DeletePrefix(temp, 2);							// replace "/./" with "/"
			pathLength -= 2;
		}
		if ((pathLength > 2) && (resultDest[pathLength-2] == '/')
							 && (resultDest[pathLength-1] == '.'))
		{	resultDest[pathLength-1] = '\0';				// Step 6b
			pathLength -= 1;								// remove "." if ends in "/.."
		}

		char* searchStart = resultDest;
		while ((*searchStart!='\0') && ((temp = strstr(searchStart+1, "/..")) != nil))
		{
			// found instance of "<segment>/.." with segment size at least 1 character

			// now check if this is "<segment>/../" or "<segment>/.."<END>
			size_t numDelete = 4;
			if (temp[3] == '\0')
			{
				numDelete--;	// no ending '/'
			}
			else if (temp[3] != '/')
			{
				// if neither, next time search beyond this /.. instance
				searchStart = temp+3;
				continue;
			}
						
			// find beginning of the <segment> of "<segment>/../"
			char* segmentStart = temp-1;
			while ((segmentStart != resultDest) && (*segmentStart != '/'))
			{	segmentStart--;
			}
			if (*segmentStart == '/')
			{	segmentStart++;
			}
			
			// now that <segment> start has been found, check that it's not ".."
			if (!strncmp(segmentStart, "../", 3))
			{
				searchStart = temp+4;
				continue;
			}
			
			// now remove "<segment>/.." or "<segment>/../" from path
			numDelete += (temp - segmentStart);
			DeletePrefix(segmentStart, numDelete);
		}

#if defined(DOT_DOT_SLASH_VIOLATION)
		// This removes any "../" or "/../" from the beginning of a path
		// Evidently, Netscape converts "http://foo/../../../../../bar" to
		// "http://foo/bar" ... this hack lets us match that behavior

		while (EqualStringN(resultDest, "../", 3))
		{	DeletePrefix(resultDest, 3);
			pathLength -= 2;
		}

		while (EqualStringN(resultDest, "/../", 4))
		{	DeletePrefix(resultDest, 3);
			pathLength -= 2;
		}
#endif
				
		// ah...get resultDest back on track, now
		while (*resultDest)
			resultDest++;
	}
	// Step 6 nightmare ends here -------------------------------------
	
	if ((resultSource = params) != nil)
	{	
		*resultDest++ = ';';
		while (*resultSource)
		{	*resultDest++ = *resultSource++;
		}
	}
	if ((resultSource = query) != nil)
	{	
		*resultDest++ = '?';
		while (*resultSource)
		{	*resultDest++ = *resultSource++;
		}
	}
	if ((resultSource = fragment) != nil)
	{	
		*resultDest++ = '#';
		while (*resultSource)
		{	*resultDest++ = *resultSource++;
		}
	}

	*resultDest = '\0';	// finally, null terminate the string

#if defined HARDWARE
	if (FindStringLast(result, ".native-code"))
		strcpy(result + strlen(result) - strlen(".native-code"), ".mips-code");
#elif defined FOR_MAC
	if (FindStringLast(result, ".native-code"))
		strcpy(result + strlen(result) - strlen(".native-code"), ".ppc-code");
#endif
	return result;
}

void URLParser::SetURL(const char* value)
{
	if (value == nil)
		value = kEmptyString;
	
	while (isspace(*value))
		value++;

	PushDebugChildURL(kURLParserPseudoURL);
	
	if (fBaseURLNoFragment != nil && fBaseURLNoFragment != fBaseURL)
		FreeTaggedMemory(fBaseURLNoFragment, kBaseURLNoFragmentTag);
	fBaseURLNoFragment = nil;
	
	fBaseURLPathPlus = nil;	// this guy USED to point inside of fBaseURL
	
	fBaseURL = CopyStringTo(fBaseURL, value, kBaseURLTag);
	// we will demolish fBaseURL and then recopy it later
	
	if (fBaseURL == nil) {
		Complain(("Failed to Copy url string \"%s\"", value));
		PopDebugChildURL();
		return;
	}
	
	fURLPartsSize = strlen(fBaseURL) + 1 + 1 + 12;	// one for '/' in path, one for null termination
													// and twelve for longest portNumber
	if (fURLParts == nil)
		fURLParts = (char*)AllocateTaggedMemory(fURLPartsSize, kURLPartsTag);
	else
		fURLParts = (char*)ReallocateTaggedMemory(fURLParts,fURLPartsSize, kURLPartsTag);

	if (fURLParts == nil) {	
		Complain(("Failed to Copy url string \"%s\"", value));
		PopDebugChildURL();
		return;
	}
	
	char* tempBaseURL = fBaseURL;
	char* tempURLParts = fURLParts;
	char* temp;

	// parse fragment
	temp = strchr(tempBaseURL, '#');
	fFragment = temp;

	if (temp != nil) {	
		*temp++ = '\0';
		
		strcpy(tempURLParts, temp);
		fFragment = tempURLParts;
		tempURLParts += strlen(fFragment) + 1;
	
		fBaseURLNoFragment = CopyStringTo(fBaseURLNoFragment, fBaseURL, kBaseURLNoFragmentTag);
	} else
		fBaseURLNoFragment = fBaseURL;

	PopDebugChildURL();	// no more memory allocations after this point...

	// parse protocol (fProtocolText, no actual matching)
	while (true) {
		char ch = *tempBaseURL;
		if (ch == ':') {
			// then we just got the protocol
			*tempBaseURL++ = 0;	// null-terminate protocol
			strcpy(tempURLParts, fBaseURL);	// and add it on here
			fURLProtocolText = tempURLParts;
			tempURLParts += strlen(fURLProtocolText) + 1;
			break;
		} else if ((!isalnum(ch)) && (ch != '+') && (ch != '.') && (ch != '-')) {
			// bad character...no protocol
			tempBaseURL = fBaseURL;
			fURLProtocolText = nil;
			fURLProtocol = kURLProtocolAbsent;
			break;
		}
		tempBaseURL++;
	}
	
	// parse netloc (domain+port...we'll split them up later on)
	if ((tempBaseURL[0]=='/') && (tempBaseURL[1]=='/')) {
		tempBaseURL += 2;
		temp = strchr(tempBaseURL, '/');
		if (temp != nil)
			*temp = '\0';		// temporarily take slash away (to null-terminate)
		
		strcpy(tempURLParts, tempBaseURL);
		fDomain = tempURLParts;
		tempURLParts += strlen(fDomain) + 1;

		if (temp != nil) {
			*temp = '/';		// put the slash back!
			tempBaseURL = temp;
		} else
			*tempBaseURL = 0;	// empty string
	} else
		fDomain = nil;
	
	// now that netloc is gone, tempBaseURL is path plus all the goodies
	if (*tempBaseURL)
		fBaseURLPathPlus = tempBaseURL;
	
	// parse query
	
	temp = strchr(tempBaseURL, '?');
	if (temp != nil) {
		*temp++ = '\0';
		strcpy(tempURLParts, temp);
		fQuery = tempURLParts;
		tempURLParts += strlen(fQuery) + 1;
	} else
		fQuery = nil;
	
	// parse params
	temp = strchr(tempBaseURL, ';');
	if (temp != nil) {
		*temp++ = '\0';
		strcpy(tempURLParts, temp);
		fParams = tempURLParts;
		tempURLParts += strlen(fParams) + 1;
	} else
		fParams = nil;
	
	// rest is just path
	if ((tempBaseURL != nil) && (*tempBaseURL != '\0')) {
		strcpy(tempURLParts, tempBaseURL);
		fPath = tempURLParts;
		tempURLParts += strlen(fPath) + 1;
	} else
		fPath = nil;

	// now that we messed up fBaseURL, get the original URL back again
	strcpy(fBaseURL, value);

	// convert "procotol://domain/" part of url to lowercase
	{
		char* toLowerBase = fBaseURL;
		char* toLowerNoFragment = fBaseURLNoFragment;
		const char* toLowerBaseEnd = fBaseURLPathPlus;
		if (toLowerBaseEnd == nil)
			toLowerBaseEnd = fBaseURL + strlen(fBaseURL);
		while (toLowerBase < toLowerBaseEnd) {
			*toLowerBase = tolower(*toLowerBase);
			*toLowerNoFragment = tolower(*toLowerNoFragment);
			toLowerBase++;
			toLowerNoFragment++;
		}
		
		if (fURLProtocolText != nil)
			LowerCase((char*)fURLProtocolText);

		if (fDomain != nil)
			LowerCase((char*)fDomain);
	}

	// set fURLProtocol (and supply a default fPortNumber, while we can)
	if (fURLProtocolText == nil) {
		fURLProtocol = kURLProtocolAbsent;
		fPortNumber = kURLProtocolAbsentPort;
	} else { 
		fURLProtocol = kURLProtocolUnknown;
		fPortNumber = kURLProtocolUnknownPort;
		
		for (int index = 0; index<kNumURLProtocols; index++) {
			if (EqualString(gURLProtocolTable[index].name, fURLProtocolText)) {
				fURLProtocol = gURLProtocolTable[index].protocol;
				fPortNumber = gURLProtocolTable[index].port;
				break;
			}
		}
	}
	
	// if they explicitly gave a port number, use it and set fPortText to point to it
	if ((fDomain != nil) && ((temp = strchr(fDomain, ':')) != nil)) {
		*temp++ = '\0';
		fPortText = temp;
		fPortNumber = strtol(fPortText, (char **) 0, 10);
	} else
		fPortText = nil;
}

#ifdef TEST_SIMULATORONLY
void URLParser::Test()
{
	URLParser* parser = new(URLParser);
	char* s;

	parser->SetURL("http://arcadia:42/index.html");
	Assert(strcmp(parser->GetURL(), "http://arcadia:42/index.html") == 0);
	Assert(parser->GetProtocol() == kURLProtocolHTTP);
	Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	Assert(parser->GetPort() == 42);
	Assert(strcmp(parser->GetPath(), "/index.html") == 0);
	
	parser->SetURL("http://arcadia/index.html");
	Assert(parser->GetProtocol() == kURLProtocolHTTP);
	Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	Assert(parser->GetPort() == 80);
	Assert(strcmp(parser->GetPath(), "/index.html") == 0);
	
	parser->SetURL("http://arcadia:42/images/");
	Assert(parser->GetProtocol() == kURLProtocolHTTP);
	Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	Assert(parser->GetPort() == 42);
	Assert(strcmp(parser->GetPath(), "/images/") == 0);
	
	parser->SetURL("http://arcadia:42");
	Assert(parser->GetProtocol() == kURLProtocolHTTP);
	Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	Assert(parser->GetPort() == 42);
	Assert(parser->GetPath() == nil);
	
	parser->SetURL("ftp://arcadia");
	Assert(parser->GetProtocol() == kURLProtocolFTP);
	Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	Assert(parser->GetPort() == 0);
	Assert(parser->GetPath() == nil);
	
	parser->SetURL("//arcadia/index.html");
	//Assert(parser->GetProtocol() == kURLProtocolHTTP);
	Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	Assert(parser->GetPort() == 80);
	Assert(strcmp(parser->GetPath(), "/index.html") == 0);
	
	// According to RFC1808, this is incorrect...should be => scheme="arcadia", path="42"
	// parser->SetURL("arcadia:42");
	// Assert(parser->GetProtocol() == kURLProtocolHTTP);
	// Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	// Assert(parser->GetPort() == 42);
	// Assert(parser->GetPath() == nil);
	
	// According to RFC1808, this is incorrect...should be => path="arcadia"
	// parser->SetURL("arcadia");
	// Assert(parser->GetProtocol() == kURLProtocolHTTP);
	// Assert(strcmp(parser->GetDomain(), "arcadia") == nil);
	// Assert(parser->GetPort() == 80);
	// Assert(parser->GetPath() == nil);

	static const char* testCases[] =
		{
	// The following examples are John's
			"http://arcadia/bar.html",
			"cache://foo.img",
			"cache://foo.img",
			
			"http://www.stuff.edu/main/docs/file.html"
			,"stuff.html"
			,"http://www.stuff.edu/main/docs/stuff.html"
			
			,"http://www.stuff.edu/main/docs/file.html"
			,"/main/docs/stuff.html"
			,"http://www.stuff.edu/main/docs/stuff.html"
			
			,"http://www.stuff.edu/main/docs/file.html"
			,"//www.stuff.edu/main/docs/stuff.html"
			,"http://www.stuff.edu/main/docs/stuff.html"
			
			,"http://www.stuff.edu/main/docs/"
			,"stuff.html"
			,"http://www.stuff.edu/main/docs/stuff.html"
			
			,"http://www.stuff.edu/main/docs/stuff.html"
			,"//artemis.com"
			,"http://artemis.com"
			
			,"http://www.stuff.edu/main/docs/file.html"
			,"../../main.html"
			,"http://www.stuff.edu/main.html"
			
			,"http://www.stuff.edu/file.html"
			,"main.html"
			,"http://www.stuff.edu/main.html"
			
			,"http://www.stuff.edu/file.html"
			,"/main.html"
			,"http://www.stuff.edu/main.html"
			
			,"http://www.stuff.edu/file.html/"
			,"main.html"
			,"http://www.stuff.edu/file.html/main.html"
			
			,"http://www.stuff.edu/file.html/"
			,"/main.html"
			,"http://www.stuff.edu/main.html"
			
			,"wtv-home:home"
			,"wtv-favorite:favorite"
			,"wtv-favorite:favorite"
			
	// The following examples are thanks to observation by Bruce and Dave
			,"http://www.bankamerica.com"
			,"gifs/homepage-i.gif"
			,"http://www.bankamerica.com/gifs/homepage-i.gif"
			
			,"http://www.foo"
			,"./bar"
			,"http://www.foo/bar"

	// The following examples are from RFC1808 "Relative Uniform Resource Locators" June 1995
			,"http://a/b/c/d;p?q#f"
			,"g:h"
			,"g:h"
			
			,"http://a/b/c/d;p?q#f"
			,"g"
			,"http://a/b/c/g"
			
			,"http://a/b/c/d;p?q#f"
			,"./g"
			,"http://a/b/c/g"
			
			,"http://a/b/c/d;p?q#f"
			,"g/"
			,"http://a/b/c/g/"
			
			,"http://a/b/c/d;p?q#f"
			,"/g"
			,"http://a/g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"//g"
			,"http://g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"?y"
			,"http://a/b/c/d;p?y"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g?y"
			,"http://a/b/c/g?y"	
			
			,"http://a/b/c/d;p?q#f" 	
			,"g?y/./x"
			,"http://a/b/c/g?y/./x"
			
			,"http://a/b/c/d;p?q#f" 	
			,"#s"
			,"http://a/b/c/d;p?q#s"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g#s"
			,"http://a/b/c/g#s"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g#s/./x"
			,"http://a/b/c/g#s/./x"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g?y#s"
			,"http://a/b/c/g?y#s"
			
			,"http://a/b/c/d;p?q#f" 	
			,";x"
			,"http://a/b/c/d;x"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g;x"
			,"http://a/b/c/g;x"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g;x?y#s"
			,"http://a/b/c/g;x?y#s"
			
			,"http://a/b/c/d;p?q#f" 	
			,"."
			,"http://a/b/c/"
			
			,"http://a/b/c/d;p?q#f" 	
			,"./"
			,"http://a/b/c/"
			
			,"http://a/b/c/d;p?q#f" 	
			,".."
			,"http://a/b/"
			
			,"http://a/b/c/d;p?q#f" 	
			,"../"
			,"http://a/b/"
			
			,"http://a/b/c/d;p?q#f" 	
			,"../g"
			,"http://a/b/g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"../.."
			,"http://a/"
			
			,"http://a/b/c/d;p?q#f" 	
			,"../../"
			,"http://a/"
			
			,"http://a/b/c/d;p?q#f" 	
			,"../../g"
			,"http://a/g"
			
			,"http://a/b/c/d;p?q#f" 	
			,kEmptyString
			,"http://a/b/c/d;p?q#f"
			
			,"http://a/b/c/d;p?q#f" 	
			,"../../../g"
#if defined(DOT_DOT_SLASH_VIOLATION)
			,"http://a/g"
#else
			,"http://a/../g"
#endif

			,"http://a/b/c/d;p?q#f" 	
			,"../../../../g"
#if defined(DOT_DOT_SLASH_VIOLATION)
			,"http://a/g"
#else
			,"http://a/../../g"
#endif
			
			,"http://a/b/c/d;p?q#f" 	
			,"/./g"
			,"http://a/./g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"/../g"
			,"http://a/../g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g."
			,"http://a/b/c/g."
			
			,"http://a/b/c/d;p?q#f" 	
			,".g"
			,"http://a/b/c/.g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"g.."
			,"http://a/b/c/g.."
			
			,"http://a/b/c/d;p?q#f" 	
			,"..g"
			,"http://a/b/c/..g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"./../g"
			,"http://a/b/g"
			
			,"http://a/b/c/d;p?q#f" 	
			,"./g/."
			,"http://a/b/c/g/"
			
			,"http://a/b/c/d;p?q#f"
			,"g/./h"
			,"http://a/b/c/g/h"
			
			,"http://a/b/c/d;p?q#f"
			,"g/../h"
			,"http://a/b/c/h"

			,"http://a/b/c/d;p?q#f" 	
			,"http:g"			
#if defined(INTENTIONAL_RFC1808_VIOLATION)
			,"http://a/b/c/g"
#else
			,"http:g"
#endif			
			
			,"http://a/b/c/d;p?q#f" 	
			,"http:"
#if defined(INTENTIONAL_RFC1808_VIOLATION)
			,"http://a/b/c/d;p?q#f"
#else
			,"http:"
#endif

	// The following examples are the same examples but with longer than one-letter segments
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg:hh"			
			,"gg:hh"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg"				
			,"http://aa/bb/cc/gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"./gg"				
			,"http://aa/bb/cc/gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg/"				
			,"http://aa/bb/cc/gg/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"/gg"				
			,"http://aa/gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"//gg"				
			,"http://gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"?yy"				
			,"http://aa/bb/cc/dd;pp?yy"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg?yy"			
			,"http://aa/bb/cc/gg?yy"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg?yy/./xx"		
			,"http://aa/bb/cc/gg?yy/./xx"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"#ss"				
			,"http://aa/bb/cc/dd;pp?qq#ss"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg#ss"			
			,"http://aa/bb/cc/gg#ss"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg#ss/./xx"		
			,"http://aa/bb/cc/gg#ss/./xx"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg?yy#ss"			
			,"http://aa/bb/cc/gg?yy#ss"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,";xx"				
			,"http://aa/bb/cc/dd;xx"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg;xx"			
			,"http://aa/bb/cc/gg;xx"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg;xx?yy#ss"		
			,"http://aa/bb/cc/gg;xx?yy#ss"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"."				
			,"http://aa/bb/cc/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"./"				
			,"http://aa/bb/cc/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,".."				
			,"http://aa/bb/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"../"				
			,"http://aa/bb/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"../gg"			
			,"http://aa/bb/gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"../.."			
			,"http://aa/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"../../"			
			,"http://aa/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"../../gg"			
			,"http://aa/gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,kEmptyString					
			,"http://aa/bb/cc/dd;pp?qq#ff"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"../../../gg"		
#if defined(DOT_DOT_SLASH_VIOLATION)
			,"http://aa/gg"
#else
			,"http://aa/../gg"
#endif
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"../../../../gg"	
#if defined(DOT_DOT_SLASH_VIOLATION)
			,"http://aa/gg"
#else
			,"http://aa/../../gg"
#endif
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"/./gg"			
			,"http://aa/./gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"/../gg"			
			,"http://aa/../gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg."				
			,"http://aa/bb/cc/gg."
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,".gg"				
			,"http://aa/bb/cc/.gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg.."				
			,"http://aa/bb/cc/gg.."
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"..gg"				
			,"http://aa/bb/cc/..gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"./../gg"			
			,"http://aa/bb/gg"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"./gg/."			
			,"http://aa/bb/cc/gg/"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg/./hh"			
			,"http://aa/bb/cc/gg/hh"
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"gg/../hh"			
			,"http://aa/bb/cc/hh"

			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"http:gg"			
#if defined(INTENTIONAL_RFC1808_VIOLATION)
			,"http://aa/bb/cc/gg"
#else
			,"http:gg"
#endif
			
			,"http://aa/bb/cc/dd;pp?qq#ff" 	
			,"http:"			
#if defined(INTENTIONAL_RFC1808_VIOLATION)
			,"http://aa/bb/cc/dd;pp?qq#ff"
#else
			,"http:"
#endif

	// Once again, with all different sized elements:
	//	a(2), b(3), c(4), d(5), p(6), q(7), f(8), g(9), h(10), y(11), x(12), s(13)
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg:hhhhhhhhhh"								
			,"ggggggggg:hhhhhhhhhh"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg"										
			,"http://aa/bbb/cccc/ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"./ggggggggg"										
			,"http://aa/bbb/cccc/ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg/"										
			,"http://aa/bbb/cccc/ggggggggg/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"/ggggggggg"										
			,"http://aa/ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"//ggggggggg"										
			,"http://ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"?yyyyyyyyyyy"										
			,"http://aa/bbb/cccc/ddddd;pppppp?yyyyyyyyyyy"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg?yyyyyyyyyyy"							
			,"http://aa/bbb/cccc/ggggggggg?yyyyyyyyyyy"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg?yyyyyyyyyyy/./xxxxxxxxxxxx"				
			,"http://aa/bbb/cccc/ggggggggg?yyyyyyyyyyy/./xxxxxxxxxxxx"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"#sssssssssssss"									
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#sssssssssssss"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg#sssssssssssss"							
			,"http://aa/bbb/cccc/ggggggggg#sssssssssssss"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg#sssssssssssss/./xxxxxxxxxxxx"			
			,"http://aa/bbb/cccc/ggggggggg#sssssssssssss/./xxxxxxxxxxxx"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg?yyyyyyyyyyy#sssssssssssss"				
			,"http://aa/bbb/cccc/ggggggggg?yyyyyyyyyyy#sssssssssssss"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,";xxxxxxxxxxxx"									
			,"http://aa/bbb/cccc/ddddd;xxxxxxxxxxxx"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg;xxxxxxxxxxxx"							
			,"http://aa/bbb/cccc/ggggggggg;xxxxxxxxxxxx"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg;xxxxxxxxxxxx?yyyyyyyyyyy#sssssssssssss"	
			,"http://aa/bbb/cccc/ggggggggg;xxxxxxxxxxxx?yyyyyyyyyyy#sssssssssssss"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"."												
			,"http://aa/bbb/cccc/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"./"												
			,"http://aa/bbb/cccc/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,".."												
			,"http://aa/bbb/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"../"												
			,"http://aa/bbb/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"../ggggggggg"										
			,"http://aa/bbb/ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"../.."											
			,"http://aa/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"../../"											
			,"http://aa/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"../../ggggggggg"									
			,"http://aa/ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,kEmptyString													
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"../../../ggggggggg"
#if defined(DOT_DOT_SLASH_VIOLATION)
			,"http://aa/ggggggggg"
#else
			,"http://aa/../ggggggggg"
#endif
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"../../../../ggggggggg"							
#if defined(DOT_DOT_SLASH_VIOLATION)
			,"http://aa/ggggggggg"
#else
			,"http://aa/../../ggggggggg"
#endif
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"/./ggggggggg"										
			,"http://aa/./ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"/../ggggggggg"									
			,"http://aa/../ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg."										
			,"http://aa/bbb/cccc/ggggggggg."
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,".ggggggggg"										
			,"http://aa/bbb/cccc/.ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg.."										
			,"http://aa/bbb/cccc/ggggggggg.."
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"..ggggggggg"										
			,"http://aa/bbb/cccc/..ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"./../ggggggggg"									
			,"http://aa/bbb/ggggggggg"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"./ggggggggg/."									
			,"http://aa/bbb/cccc/ggggggggg/"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg/./hhhhhhhhhh"							
			,"http://aa/bbb/cccc/ggggggggg/hhhhhhhhhh"
			
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"ggggggggg/../hhhhhhhhhh"							
			,"http://aa/bbb/cccc/hhhhhhhhhh"

			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"http:ggggggggg"									
#if defined(INTENTIONAL_RFC1808_VIOLATION)
			,"http://aa/bbb/cccc/ggggggggg"
#else
			,"http:ggggggggg"
#endif

			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff" 	
			,"http:"											
#if defined(INTENTIONAL_RFC1808_VIOLATION)
			,"http://aa/bbb/cccc/ddddd;pppppp?qqqqqqq#ffffffff"
#else
			,"http:"
#endif

			,"HtTp://MiXeD.cAsE/GeTs/CoNvErTeD/iN;pRoToCoL?aNd=DoMaIn#OnLy"
			,"LiKe#ThIs"
			,"http://mixed.case/GeTs/CoNvErTeD/LiKe#ThIs"
			
			,"HtTp:/SoMe.ThInGs/WiTh/No/DoMaIn"
			,"../../StAy/MiXeD"
			,"http:/SoMe.ThInGs/StAy/MiXeD"
			
			,"HtTp://MiXeD.cAsE"
			,""
			,"http://mixed.case"
		};
	
	const char** currTestCase = testCases;
	int numCases = sizeof(testCases)/(3*sizeof(testCases[0]));
	
	Assert(((sizeof(testCases)/(sizeof(testCases[0]))) % 3) == 0); // else the testCases array isn't a mulitple of three!
	while (numCases--)
	{
		const char* baseURL = *currTestCase++;
		const char* partialURL = *currTestCase++;
		const char* resultURL = *currTestCase++;
		
		Assert((baseURL != nil) || (partialURL != nil) && (resultURL != nil));
		parser->SetURL(baseURL);
		s = parser->NewURL(partialURL, "URL");
		Assert(strcmp(s, resultURL) == 0);
		FreeTaggedMemory(s, "URL");
	}

}
#endif // defined(TEST_SIMULATORONLY)
