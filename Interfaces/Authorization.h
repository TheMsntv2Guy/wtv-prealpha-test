// Copyright(c) 1995-1996 Artemis Research, Inc. All rights reserved.

#ifndef __AUTHORIZATION_H__
#define __AUTHORIZATION_H__

class DataStream;
class Resource;

// =============================================================================

class AuthorizationList {
public:
							AuthorizationList();
	virtual					~AuthorizationList();
	
	void					Add(const Resource* target, const char* realm, const char* name, const char* password);
	char*					NewAuthorization(const Resource* target, const char* realm, const char* tag);

#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize();
#endif
	static void				Initialize();
	static void				Test();

protected:
	Boolean					CreateStream();
	Boolean					Exists(const char* domain, const char* realm);

protected:
	DataStream*				fStream;
};

extern AuthorizationList* gAuthorizationList;

// =============================================================================

#endif
