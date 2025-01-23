// Copyright(c) 1995-1996 Artemis Research, Inc. All rights reserved.

#ifndef __COOKIE_H__
#define __COOKIE_H__

class DataStream;
class Resource;

// =============================================================================

class CookieList {
public:
							CookieList();
	virtual					~CookieList();
	
	Boolean					Exists(const char* domain, const char* path);
	void					Load();
	Boolean					LoadCookie(const Resource* target);
	char*					NewCookie(const Resource* target, const char* tag);
	Boolean					SetAttribute(const char* name, char* value, const Resource* base);

#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize();
#endif
	static void				Initialize();
	static void				Test();

protected:
	void					Append(const char* domain, const char* path);
	Boolean					CreateStream();
	Resource*				NewCookieResource(const char* domain, const char* path);
	Resource*				NewCookieResource(const Resource* target);
	void					SendToServer(const char* cookie, const char* expires, const char* path, const char* domain);

protected:
	DataStream*				fStream;
};

extern CookieList* gCookieList;

// =============================================================================

#endif
