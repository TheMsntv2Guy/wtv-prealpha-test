// ===========================================================================
//	Network.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __NETWORK_H__
#define __NETWORK_H__

#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"			/* Error */
#endif

#ifndef __RESOURCE_H__
#include "Resource.h"				/* Resource */
#endif

#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

class Protocol;
class HTTPProtocol;
class Indicator;
class Network;
class Service;
class ServiceList;
class ServicePointer;
class ServiceWindow;
class Socket;
class SocketWindow;

// =============================================================================
// Class Network

typedef enum {
	kNetInactive = 0,
	kNetShowStartup,
	kNetDialBegin,
	kNetDialEnd,
	kNetActive,
	kNetWaiting,
	kNetHangUpThenDialBegin,
	kNetHangUpThenDialEnd,
	kNetHangUpThenWait,
	kNetHangUpBegin,
	kNetHangUpEnd
} NetState;

class Network : public HasAttributes {
public:
							Network();
	virtual					~Network();
	
	NetState				GetState() const;
	HTTPProtocol*			GetProtocol(long index) const;
#ifdef DEBUG_NAMES
	const char*				GetStateAsString() const;
#endif
	const char*				GetStateString() const;
	uchar*					GetSessionKey1(long incarnation) const;
	uchar*					GetSessionKey2(long incarnation) const;
	Boolean					IsSecure() const;

	Boolean					IsActive() const;
	
	virtual Boolean			SetAttribute(const char* name, char* value);
	void					SetNotificationPending(Boolean);
	void					SetTellyScript(const Resource*);
	void					SetTicket(const char* ticket);
	void 					SetResponse(const char *response);
	Boolean					SetSpecialAttribute(const char* name, char* value);
	
	void					Activate();
	void					Deactivate();
	void					HangUp();
	void					Idle();
	void					InitializeForLogin(const char* host, long port);
	void					InitializeForKrueger();
	void					InitializeForNone();
	void					InitializeForProxy(const char* host, long port);
	void					InitializeForSignup(const char* host, long port, Boolean forceSignup);
	void					KillProtocols();
	void					OpenConnection(const char* url);
	void					Reactivate(Boolean reconnect);
	void					RestoreState(void);
	void					SaveState(void);
	void					ShowStartup();
	void					Visit(const Resource*);
	void					Visit(const char* url);
	void					WriteAttributes(Stream* stream);

#ifdef INCLUDE_FINALIZE_CODE
	static void				Finalize();
#endif
	static void				Initialize();

	enum { kProtocolCount = 4 };

#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintTicketResource	= kFirstBoxPrint << 0,
		kBoxPrintVisitList 		= kFirstBoxPrint << 1,
		kBoxPrintLoginStream	= kFirstBoxPrint << 2,
		kBoxPrintProtocol		= kFirstBoxPrint << 3,
		kBoxPrintServiceList	= kFirstBoxPrint << 4,
		kBoxPrintState			= kFirstBoxPrint << 5,
		kLastBoxPrint = kBoxPrintState
	};
	void					BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const Network* network, long whatToPrint);
#endif

protected:
	HTTPProtocol*			ChooseProtocol(Resource* pending);
	void					HandleNotificationsPending();
	void					HangUpAll();
	void					IdleActive();
	void					IdleDialBegin();
	void					IdleDialEnd();
	void					IdleHangUpBegin();
	void					IdleHangUpEnd();
	void					IdleHangUpThenDialBegin();
	void					IdleHangUpThenDialEnd();
	void					IdleHangUpThenWait();
	void					IdleShowStartup();
	void					IdleWaiting();
	Boolean					IsProtocolRunning() const;
	Resource*				NewPendingResource();
	DataStream*				NewTellyScriptStream();
	Error					HandleNotificationPort(Boolean open);
	Error					ProcessChallenge(uchar* keys);
	Error					ProcessSecret(uchar* key);
	void					RequestNotifications();
	void					RunProtocols();
	void					SetSecure(Boolean);
	void					ShowConnectIndicator(Boolean reconnecting);

protected:
	long					fCallCount;
	Resource				fChallengeResponse;
	Indicator*				fIndicator;

	DataStream*				fLoginStream;
	HTTPProtocol*			fProtocol[kProtocolCount];
	uchar					fSessionKey1[16];
	uchar					fSessionKey2[16];
	uchar					fSecret[8];
	Resource				fTicketResource;
	ObjectList				fVisitList; // list of Resource objects

	NetState				fState;

	Boolean					fHaveTicket;
	Boolean					fSecureNetwork;
	Boolean					fNotificationPending;
	Boolean					fSecretValid;
};

extern Network* gNetwork;

inline NetState
Network::GetState() const
{
	return fState;
}

inline Boolean 
Network::IsActive() const
{
	return fState == kNetActive;
}

inline Boolean
Network::IsSecure() const
{
	return fSecureNetwork && fHaveTicket;
}

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Network.h multiple times"
	#endif
#endif /* __NETWORK_H__ */
