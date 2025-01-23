// ===========================================================================
//	HTTP.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __HTTP_H__
#define __HTTP_H__

#include "ErrorNumbers.h"			/* Error */
#include "Protocol.h"				/* Protocol, ProtocolCommand */
#include "Stream.h"

// =============================================================================
// Class HTTPCommand

typedef enum {
	kHTTPCreateStream = 0,
	kHTTPGetCookieBegin,
	kHTTPGetCookieEnd,
	kHTTPWriteCommandBegin,
	kHTTPWriteCommandEnd,
	kHTTPWriteBody,
	kHTTPReadResponse,
	kHTTPReadHeader,
	kHTTPReadHeaderAndAuthorize,
	kHTTPReadHeaderAndFail,
	kHTTPReadHeaderAndRedirect,
	kHTTPReadBody,
	kHTTPComplete,
	kHTTPError
} HTTPState;

class HTTPCommand : public ProtocolCommand {
public:
							HTTPCommand();
	virtual					~HTTPCommand();
	
	virtual ClassNumber		GetClassNumber() const;
	virtual long			GetState() const;
#ifdef DEBUG_NAMES
	virtual const char*		GetStateAsString() const;
#endif /* DEBUG_NAMES */

	virtual Boolean			Idle();
	virtual Boolean			SetAttribute(const char* name, char* value);
	virtual void			WriteAttributes(Stream*);
	void					WriteCookies(Stream*);

#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = ProtocolCommand::kLastBoxPrint << 1,
		kBoxPrintState			= kFirstBoxPrint << 0,
		kBoxPrintCommandStream	= kFirstBoxPrint << 1,
		kLastBoxPrint = kBoxPrintCommandStream
	};
	virtual void			BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const HTTPCommand* command, long whatToPrint);
#endif

protected:
	Boolean					CheckBodyComplete();
	void					IdleCreateStream();
	void					IdleGetCookieBegin();
	void					IdleGetCookieEnd();
	virtual void			IdleReadBody();
	void					IdleReadHeader();
	virtual void			IdleReadResponse();
	virtual void			IdleWriteBody();
	virtual void			IdleWriteCommandBegin();
	virtual void			IdleWriteCommandEnd();
	void					Redirect(const char* url);

protected:
	MemoryStream			fCommandStream;

	HTTPState				fState;
};

// =============================================================================

class HTTPGetCommand : public HTTPCommand {
public:
							HTTPGetCommand();
	virtual					~HTTPGetCommand();

	virtual ClassNumber		GetClassNumber() const;

protected:
	virtual void			IdleWriteCommandBegin();
};

// =============================================================================

class HTTPPostCommand : public HTTPCommand {
public:
							HTTPPostCommand();
	virtual					~HTTPPostCommand();

	virtual ClassNumber		GetClassNumber() const;

protected:
	virtual void			IdleWriteCommandBegin();
};

// =============================================================================

class HTTPProtocol : public Protocol {
public:
							HTTPProtocol();
	virtual					~HTTPProtocol();
	
	virtual ClassNumber		GetClassNumber() const;

	virtual ProtocolCommand* NewGetCommand(const Resource*);
	virtual ProtocolCommand* NewPostCommand(const Resource*);

	static void				Finalize();
	static void				Initialize();

#ifdef DEBUG_BOXPRINT
	enum
	{
		kFirstBoxPrint = Protocol::kLastBoxPrint << 1,
		kBoxPrintMemoryStream	= kFirstBoxPrint << 0,
		kLastBoxPrint = kBoxPrintMemoryStream
	};
	virtual void			BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const HTTPProtocol* protocol, long whatToPrint);
#endif /* DEBUG_BOXPRINT */

protected:
	static MemoryStream*	gMemoryStream;
};

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include HTTP.h multiple times"
	#endif
#endif /* __HTTP_H__ */
