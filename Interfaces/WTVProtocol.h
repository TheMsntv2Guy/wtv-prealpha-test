// ===========================================================================
//	WTVProtocol.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================
#ifndef __WTVPROTOCOL_H__
#define __WTVPROTOCOL_H__

#ifndef __HTTP_H__
#include "HTTP.h"
#endif

#ifndef __PROTOCOL_H__
#include "Protocol.h"
#endif

#include "rc4.h"

// =============================================================================

class WTVPCommand : public HTTPCommand {
public:
							WTVPCommand();
	virtual					~WTVPCommand();

	virtual ClassNumber		GetClassNumber() const;
	
	static void				AddWTVAttribute(const char* name, const char* value);
	static void				Initialize();
#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize();
#endif /* INCLUDE_FINALIZE_CODE */

protected:
	void					RedirectWithData(const char* url);

protected:
	static Resource*		gWTVAttributeResource;
	
	virtual void			IdleReadBody();
	virtual void			IdleReadResponse();
	virtual Boolean			SetAttribute(const char* name, char* value);
	virtual void			WriteAttributes(Stream*);

protected:
	Boolean					fIsEncrypted;
}; 

// =============================================================================

class WTVPGetCommand : public WTVPCommand {
public:
							WTVPGetCommand();
	virtual					~WTVPGetCommand();

	virtual ClassNumber		GetClassNumber() const;

protected:
	virtual void			IdleWriteCommandBegin();
};

// =============================================================================
class WTVPPostCommand : public WTVPCommand {
public:
							WTVPPostCommand();
	virtual					~WTVPPostCommand();

	virtual ClassNumber		GetClassNumber() const;

protected:
	virtual void			IdleWriteCommandBegin();
};

// =============================================================================
// Protocol for communicating with the WebTV server.

class WTVProtocol : public HTTPProtocol {
public:
	virtual					~WTVProtocol();

	virtual ClassNumber		GetClassNumber() const;
	virtual void			GetSessionKeys(RC4_KEY** SendKey, RC4_KEY** ReceiveKey);
	
	virtual ProtocolCommand* NewGetCommand(const Resource*);
	virtual ProtocolCommand* NewPostCommand(const Resource*);
	
protected:
	virtual void			IdleCommandEnd();
	virtual void			IdleSecureBegin();
	virtual void			IdleSecureEnd();
	Error					SetSessionKeys();

protected:
	MemoryStream			fSecureStream;
	int						fIncarnation;
	RC4_KEY					fSendKey;
	RC4_KEY					fReceiveKey;
	
};

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include WTVProtocol.h multiple times"
	#endif
#endif /* __WTVPROTOCOL_H__ */
