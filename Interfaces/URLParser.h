// ===========================================================================
//	URLParser.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __URLPARSER_H__
#define __URLPARSER_H__

typedef enum {
	kURLProtocolAbsent = -2,
	kURLProtocolUnknown = -1,
	kURLProtocolFile,
	kURLProtocolFTP,
	kURLProtocolHTTP,
	kNumURLProtocols,
	kURLProtocolDefault = kURLProtocolHTTP
} URLProtocol;

class URLParser {
public:
							URLParser();
	virtual					~URLParser();

	const char*				GetURL() const;
	const char*				GetURLNoFragment() const;
	const char*				GetDomain() const;
	const char*				GetFragment() const;
	const char*				GetPath() const;
	const char*				GetPathPlus() const;
	long					GetPort() const;
	URLProtocol				GetProtocol() const;
	const char*				GetProtocolText() const;

	void					SetURL(const char* url);
	char*					NewURL(const char* partial, const char* tag = "URL");

#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize(void);
#endif /* INCLUDE_FINALIZE_CODE */
	static void				Initialize(void);

#ifdef TEST_SIMULATORONLY
	static void				Test();
#endif /* TEST_SIMULATORONLY */

protected:
	char*					fBaseURL;
	char*					fBaseURLNoFragment;
	const char*				fBaseURLPathPlus;	// base URL except no "http://netloc:port"
	
	size_t					fURLPartsSize;
	char*					fURLParts;
	
	const char*				fURLProtocolText; // <== same as "Scheme" in RFC1808
	URLProtocol				fURLProtocol;
	const char*				fDomain;	// <== same as "NetLoc" in RFC1808
	const char*				fPortText;		// (when coupled with fPortText)
	long					fPortNumber;
	const char*				fPath;
	const char*				fParams;
	const char*				fQuery;
	const char*				fFragment;
};

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include URLParser.h multiple times"
	#endif
#endif /* __URLPARSER_H__ */
