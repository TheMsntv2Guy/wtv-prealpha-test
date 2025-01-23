// ===========================================================================
//	Protocol.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __CACHESTREAM_H__
#include "CacheStream.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __NETWORK_H__
#include "Network.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __PROTOCOL_H__
#include "Protocol.h"
#endif
#ifndef __SOCKET_H__
#include "Socket.h"
#endif

#ifdef DEBUG_BOXPRINT
#include "BoxPrintDebug.h"
#endif

// =============================================================================
// Class Protocol

Protocol::Protocol()
{
}

Protocol::~Protocol()
{
	Disconnect();
}

#ifdef DEBUG_BOXPRINT
void
Protocol::BoxPrintDebug(long whatToPrint) const
{
	if (whatToPrint == 0)
		whatToPrint = kBoxPrintCommand + kBoxPrintState;

	if (whatToPrint & kBoxPrintCommand) {
		if (!fCommand)
			BoxPrint("Command: <none>");
		else
			fCommand->BoxPrintDebug(0);
	}

	if (whatToPrint & kBoxPrintResource)
		fResource.BoxPrintDebug(0);

	if (whatToPrint & kBoxPrintService) {
		if (!fService)
			BoxPrint("Service: <none>");
		else
			fService->BoxPrintDebug(0);
	}

	if (whatToPrint & kBoxPrintSocket)
		BoxPrint("Socket: not implemented");

	if (whatToPrint & kBoxPrintState) {
#ifdef DEBUG_NAMES
		BoxPrint("ProtocolState: %s (%d)", GetStateAsString(), GetState());
#else /* of #ifdef DEBUG_NAMES */
		BoxPrint("ProtocolState: %d", GetState());
#endif /* of #else of #ifdef DEBUG_NAMES */
	}
}
#endif

Boolean
Protocol::CanConnectTo(Service* service, const Resource* target) const
{
	if (IsError(service == nil))
		return false;
	
	if (IsError(target == nil))
		return false;
	
	if (GetClassNumber() != service->GetProtocolClassNumber())
		return false;
	
	if (fResource.IsValid() && fResource != *target)
		return false;
	
	if (GetState() == kProtocolDead)
		return true;
	
	if ((GetState() <= kProtocolReady) && fService.IsEqual(service))
		return true;
	
	return false;
}

void
Protocol::Connect(Service* service, const Resource* resource)
{
	if (IsError(!CanConnectTo(service, resource)))
		return;
	
	if (GetState() != kProtocolDead)
		return;
	
	fResource = *resource;
	fService.SetService(service);
	SetState(kProtocolConnectBegin);
}

void
Protocol::Disconnect()
{
	SetState(kProtocolDisconnectBegin);
	IdleDisconnectBegin();
}

ClassNumber
Protocol::GetClassNumber() const
{
	return kClassNumberProtocol;
}

Priority 
Protocol::GetPriority() const
{
	// The command's version may be more up-to-date because of redirection.
	if (fCommand != nil)
		return fCommand->GetResource()->GetPriority();

	return fResource.GetPriority();
}

#ifdef DEBUG_NAMES
const char*
Protocol::GetStateAsString() const
{
	switch (GetState()) {
	case kProtocolDead:				return "dead";
	case kProtocolConnectBegin:		return "connect begin";
	case kProtocolConnectEnd:		return "connect end";
	case kProtocolReady:			return "ready";
	case kProtocolCommandBegin:		return "command begin";
	case kProtocolCommandContinue:	return "command continue";
	case kProtocolCommandEnd:		return "command end";
	case kProtocolDisconnectBegin:	return "disconnect begin";
	case kProtocolDisconnectEnd:	return "disconnect end";
	}
	
	return "unknown protocol";
}
#endif

void
Protocol::Idle()
{
	PerfDump perfdump("Protocol::Idle");

	switch (GetState()) {
	case kProtocolDead:				break;
	case kProtocolConnectBegin:		IdleConnectBegin(); break;
	case kProtocolConnectEnd:		IdleConnectEnd(); break;
	case kProtocolSecureBegin:		IdleSecureBegin(); break;
	case kProtocolSecureEnd:		IdleSecureEnd(); break;
	case kProtocolReady:			break;
	case kProtocolCommandBegin:		IdleCommandBegin(); break;
	case kProtocolCommandContinue:	IdleCommandContinue(); break;
	case kProtocolCommandEnd:		IdleCommandEnd(); break;
	case kProtocolDisconnectBegin:	IdleDisconnectBegin(); break;
	case kProtocolDisconnectEnd:	IdleDisconnectEnd(); break;
	}
}

void
Protocol::IdleCommandBegin()
{
	if (IsError(fCommand == nil)) {
		fResource.Reset();
		SetState(kProtocolReady);
		return;
	}

	SetState(kProtocolCommandContinue);
	IdleCommandContinue();
}

void
Protocol::IdleCommandContinue()
{
	if (IsError(fCommand == nil)) {
		fResource.Reset();
		SetState(kProtocolReady);
		return;
	}
	
	if (fCommand->Idle()) {
		SetDebugModifiedTime();
		return;
	}
	
	// Disconnect if command did not complete.
	if (fCommand->GetStatus() != kComplete) {
		SetState(kProtocolDisconnectBegin);
		IdleDisconnectBegin();
		return;
	}

	// Command completed normally.	
	SetState(kProtocolCommandEnd);
	IdleCommandEnd();
}

void
Protocol::IdleCommandEnd()
{
	delete(fCommand);
	fCommand = nil;
	fResource.Reset();
	SetState(kProtocolDisconnectBegin);
	
	// Don't automatically call IdleDisconnectBegin: a subclass may override this
	// to implement persistent connections.
}

void
Protocol::IdleConnectBegin()
{
	Error error;
	
	if (IsError(fSocket != nil || !fService)) {
		SetState(kProtocolDisconnectBegin);
		return;
	}
	
	fSocket = fService->NewSocket(&error);
	
	switch (error) {
	case kNoError:
		SetState(kProtocolConnectEnd);
		IdleConnectEnd();
		break;
	
	case kTooManyConnections:
		SetState(kProtocolDisconnectBegin);
		IdleDisconnectBegin();
		break;
	
	default:
		fResource.SetStatus(error);
		SetState(kProtocolDisconnectBegin);
		IdleDisconnectBegin();
	}
}

void
Protocol::IdleConnectEnd()
{
	Error error;
	
	if (IsError(fSocket == nil)) {
		SetState(kProtocolDisconnectBegin);
		return;
	}
	
	if ((error = fSocket->Idle()) == kPending)
		return;
	
	if (error != kNoError) {
		fService->Suspend();
		SetState(kProtocolDisconnectBegin);
		IdleDisconnectBegin();
		return;
	}
	
	SetState(kProtocolSecureBegin);
	IdleSecureBegin();
}

void
Protocol::IdleDisconnectBegin()
{
	if (fCommand != nil) {
		delete(fCommand);
		fCommand = nil;
	}
	
	if (fSocket != nil) {
		fSocket->Dispose();
		fSocket = nil;
	}
	
	fResource.Reset();
	fService.Reset();
	SetState(kProtocolDisconnectEnd);
	IdleDisconnectEnd();
}

void
Protocol::IdleDisconnectEnd()
{
	SetState(kProtocolDead);
}

void Protocol::IdleSecureBegin()
{
	SetState(kProtocolSecureEnd);
	IdleSecureEnd();
}

void Protocol::IdleSecureEnd()
{
	SetState(kProtocolReady);
}

void
Protocol::RunCommand(ProtocolCommand* command)
{
	if (IsError(command == nil))
		return;
	
	if (IsError(GetState() != kProtocolReady)) {
		delete(command);
		return;
	}
	
	if (IsError(fCommand != nil)) {
		delete(fCommand);
		fCommand = nil;
	}
	
	fCommand = command;
	fCommand->SetProtocol(this);
	fResource = *fCommand->GetResource();
	SetState(kProtocolCommandBegin);
	IdleCommandBegin();
}

void
Protocol::SetResource(const Resource* resource)
{
	fResource = *resource;
}

#ifdef DEBUG_BOXPRINT
void
Protocol::StaticBoxPrintDebug(const Protocol* protocol, long whatToPrint)
{
	if (protocol == nil) {
		BoxPrint("Protocol: <nil>");
	} else {
		BoxPrint("Protocol: <%#06x>", protocol);
		AdjustBoxIndent(1);
		protocol->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif

// =============================================================================
// Class ProtocolCommand

ProtocolCommand::ProtocolCommand()
{
	ResetTimeout();
}

ProtocolCommand::~ProtocolCommand()
{
	if (fResource.GetStatus() == kPending)
		fResource.SetStatus(kStreamReset);
	
	delete(fStream);
}

#ifdef DEBUG_BOXPRINT
void
ProtocolCommand::BoxPrintDebug(long whatToPrint) const
{
	if (whatToPrint == 0)
		return; // whatToPrint = kBoxPrintTimeoutStart;


	if (whatToPrint & kBoxPrintProtocol)
		Protocol::StaticBoxPrintDebug(fProtocol, 0);

	if (whatToPrint & kBoxPrintResource)
		Resource::StaticBoxPrintDebug(&fResource, 0);

	if (whatToPrint & kBoxPrintStream)
		BoxPrint("Stream: not implemented");

	if (whatToPrint & kBoxPrintTimeoutStart)
		BoxPrint("TimeoutStart = %d (Elapsed = %d, time out at %d)",
				 fTimeoutStart, Now() - fTimeoutStart, kProtocolCommandTimeout);
}
#endif

Boolean
ProtocolCommand::CreateStream()
{
	delete(fStream);
	
	if ((fStream = fResource.NewStreamForWriting()) == nil)
		return false;
	
	fStream->SetDataExpected(CacheEntry::kDataExpectedUnknown);
	fStream->SetStatus(kPending);
	SetDebugModifiedTime();
	return true;
}

ClassNumber
ProtocolCommand::GetClassNumber() const
{
	return kClassNumberProtocolCommand;
}

long
ProtocolCommand::GetState() const
{
	return (fProtocol == nil) ? 0 : (long)(fProtocol->GetState());
}

#ifdef DEBUG_NAMES
const char*
ProtocolCommand::GetStateAsString() const
{
	return "unknown command";
}
#endif /* DEBUG_NAMES */

Boolean
ProtocolCommand::Idle()
{
	Trespass();
	return false;
}

void
ProtocolCommand::SetAttributeString(char* string)
{
	char* colon;
	char* value;
	
	if (IsWarning(string == nil || *string == 0))
		return;
	
	colon = FindString(string, ":");
	if (IsWarning(colon == nil))
		return;
	
	value = SkipCharacters(colon + 1, " ");
	if (IsWarning(value == nil || *value == 0))
		return;
	
	*colon = 0;
	if (!SetAttribute(string, value))
		Message(("ProtocolCommand: ignoring %s: %s", string, value));

	SetDebugModifiedTime();
}

void
ProtocolCommand::SetProtocol(Protocol* protocol)
{
	fProtocol = protocol;
	SetDebugModifiedTime();
}

void
ProtocolCommand::SetResource(const Resource* resource)
{
	if (IsError(resource == nil))
		return;

	if (fStream != nil) {
		delete(fStream);
		fStream = nil;
	}
	
	fResource = *resource;
	SetDebugModifiedTime();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
ProtocolCommand::StaticBoxPrintDebug(const ProtocolCommand* protocolCommand, long whatToPrint)
{
	if (protocolCommand == nil) {
		BoxPrint("ProtocolCommand: <nil>");
	} else {
		BoxPrint("ProtocolCommand: <%#06x>", protocolCommand);
		AdjustBoxIndent(1);
		protocolCommand->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif

