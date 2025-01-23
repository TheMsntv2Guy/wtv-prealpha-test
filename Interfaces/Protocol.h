// ===========================================================================
//	Protocol.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"		/* for Priority */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"		/* for Resource */
#endif
#ifndef __SERVICE_H__
#include "Service.h"		/* for Service, ServicePointer */
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"		/* for HasDebugModifiedTime */
#endif

class DataStream;
class MemoryStream;
class Protocol;
class ProtocolCommand;
class Queue;
class Service;
class Socket;

// =============================================================================

typedef enum {
	kProtocolDead = 0,
	kProtocolConnectBegin,
	kProtocolConnectEnd,
	kProtocolSecureBegin,
	kProtocolSecureEnd,
	kProtocolReady,
	kProtocolCommandBegin,
	kProtocolCommandContinue,
	kProtocolCommandEnd,
	kProtocolDisconnectBegin,
	kProtocolDisconnectEnd
} ProtocolState;

class Protocol : public HasDebugModifiedTime {
public:
							Protocol();
	virtual					~Protocol();

	Boolean					CanConnectTo(Service*, const Resource*) const;
	virtual ClassNumber		GetClassNumber() const;
	ProtocolCommand*		GetCommand() const;
	Priority				GetPriority() const;
	Service*				GetService() const;
	Socket*					GetSocket() const;
	ProtocolState			GetState() const;
#ifdef DEBUG_NAMES
	const char*				GetStateAsString() const;
#endif /* DEBUG_NAMES */
	Boolean					IsConnectedTo(Service*) const;
	Boolean					IsDead() const;
	Boolean					IsReady() const;
	Boolean					IsRunning() const;
	
	void					SetResource(const Resource*);

	void					Connect(Service*, const Resource*);
	void					Disconnect();
	void					Idle();
	void					RunCommand(ProtocolCommand*);

#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintCommand		= kFirstBoxPrint << 0,
		kBoxPrintResource		= kFirstBoxPrint << 1,
		kBoxPrintService 		= kFirstBoxPrint << 2,
		kBoxPrintSocket			= kFirstBoxPrint << 3,
		kBoxPrintState			= kFirstBoxPrint << 4,
		kLastBoxPrint = kBoxPrintState
	};
	virtual void			BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const Protocol* protocol, long whatToPrint);
#endif

protected:
	void					IdleCommandBegin();
	void					IdleCommandContinue();
	virtual void			IdleCommandEnd();
	void					IdleConnectBegin();
	virtual void			IdleConnectEnd();
	void					IdleDisconnectBegin();
	void					IdleDisconnectEnd();
	virtual void			IdleSecureBegin();
	virtual void			IdleSecureEnd();
	void					SetState(ProtocolState state);
	
protected:
	ProtocolCommand*		fCommand;
	Resource				fResource;
	ServicePointer			fService;
	Socket*					fSocket;
	ProtocolState			fState;
};

inline ProtocolCommand*
Protocol::GetCommand() const
{
	return fCommand;
}

inline Service*
Protocol::GetService() const
{
	return fService;
}

inline Socket*
Protocol::GetSocket() const
{
	return fSocket;
}

inline ProtocolState
Protocol::GetState() const
{
	return fState;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline Boolean
Protocol::IsConnectedTo(Service* service) const
{
	return fService.IsEqual(service);
}
#endif

inline Boolean
Protocol::IsDead() const
{
	return fState == kProtocolDead;
}

inline Boolean
Protocol::IsReady() const
{
	return fState == kProtocolReady;
}

inline Boolean
Protocol::IsRunning() const
{
	return fState == kProtocolCommandContinue;
}

inline void
Protocol::SetState(ProtocolState state)
{
	fState = state;
	SetDebugModifiedTime();
}
// =============================================================================

class ProtocolCommand : public Listable, public HasAttributes,
						public HasDebugModifiedTime {
public:
							ProtocolCommand();
	virtual					~ProtocolCommand();
	
	virtual ClassNumber		GetClassNumber() const;
	Protocol*				GetProtocol() const;
	const Resource*			GetResource() const;
	virtual long			GetState() const;
#ifdef DEBUG_NAMES
	virtual const char*		GetStateAsString() const;
#endif /* DEBUG_NAMES */
	Error					GetStatus() const;
	Boolean					IsTimedOut() const;
	
	void					SetProtocol(Protocol*);
	void					SetResource(const Resource*);
	
	virtual Boolean			Idle(); // returns true if it wants more idle time
	void					ResetTimeout();

#ifdef DEBUG_BOXPRINT
public:
	enum
	{
		kFirstBoxPrint = 1,
		kBoxPrintProtocol		= kFirstBoxPrint << 0,
		kBoxPrintResource		= kFirstBoxPrint << 1,
		kBoxPrintStream 		= kFirstBoxPrint << 2,
		kBoxPrintTimeoutStart	= kFirstBoxPrint << 3,
		kLastBoxPrint = kBoxPrintTimeoutStart
	};
	virtual void			BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const ProtocolCommand* command, long whatToPrint);
#endif

protected:
	Boolean					CreateStream();
	Service*				GetService() const;
	Socket*					GetSocket() const;
	void					IdleReadHeader();
	Boolean					IsValid() const;
	void					SetAttributeString(char*);

protected:
#ifdef DEBUG
	enum { kProtocolCommandTimeout = 10 * 60 * kOneSecond };
#else
	enum { kProtocolCommandTimeout = 60 * kOneSecond };
#endif
	
	Protocol*				fProtocol;
	Resource				fResource;
	DataStream*				fStream;
	ulong					fTimeoutStart;
};

inline Protocol*
ProtocolCommand::GetProtocol() const
{
	return fProtocol;
}

inline const Resource*
ProtocolCommand::GetResource() const
{
	return &fResource;
}

inline Service*
ProtocolCommand::GetService() const
{
	return fProtocol->GetService();
}

inline Socket*
ProtocolCommand::GetSocket() const
{
	return fProtocol->GetSocket();
}

inline Error
ProtocolCommand::GetStatus() const
{
	return fResource.GetStatus();
}

inline Boolean
ProtocolCommand::IsTimedOut() const
{
	return Now() - fTimeoutStart > kProtocolCommandTimeout;
}

#ifdef DEBUG_CACHE_VALIDATE
inline Boolean
ProtocolCommand::IsValid() const
{
	return fProtocol != nil && fResource.IsValid();
}
#endif

inline void
ProtocolCommand::ResetTimeout()
{
	fTimeoutStart = Now();
}

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Protocol.h multiple times"
	#endif
#endif /* __PROTOCOL_H__ */
