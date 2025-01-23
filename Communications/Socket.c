// ===========================================================================
//	Socket.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include "defs.h"
#include "tinyip.h"
#include "tinytcp.h"
#include "MemoryManager.h"
#include "Socket.h"
#include "Utilities.h"

#ifdef FOR_MAC
#include "MacSimulator.h"
#endif

// ===========================================================================
// Class Socket

Socket::Socket()
{
}

Socket::~Socket()
{
}

Error 
Socket::Connect(const char* UNUSED(hostName), short UNUSED(hostPort))
{
	Trespass();
	return kGenericError;
}

Error 
Socket::Connect(long UNUSED(hostAddress), short UNUSED(hostPort))
{
	Trespass();
	return kGenericError;
}

Error 
Socket::Dispose()
{
	Trespass();
	return kGenericError;
}

long
Socket::FIFOCount() const
{
	Trespass();
	return 0;
}

Error 
Socket::Idle() 
{
	Trespass();
	return kGenericError;
}

Error 
Socket::Init()
{
	return kNoError;
}

Error 
Socket::Listen(short UNUSED(localPort))
{
	Trespass();
	return kGenericError;
}

Socket* 
Socket::NewSocket()
{
	Socket *socket;
	
#ifdef FOR_MAC
	if (!gSystem->GetUsePhone()) {
		socket = new(MacSocket);
		socket->Init();
		
		return socket;
	}
#endif

	if ((socket = TinySocket::NewTinySocket()) == nil)
		return nil;
		
	if (socket->Init() != kNoError) {
		return nil;
	}

	return socket;	
}

Socket* 
Socket::NewSocket(Boolean UNUSED(old))
{
	Socket *socket;
	
#ifdef FOR_MAC
	if (!gSystem->GetUsePhone()) {
		socket = new(MacSocket);
		socket->Init();
		
		return socket;
	}
#endif

	if ((socket = TinySocket::NewTinySocket()) == nil)
		return nil;
		
	if (socket->Init() != kNoError) {
		return nil;
	}

	return socket;	
}

Error 
Socket::ReadIntoStream(Stream* UNUSED(stream))
{
	Trespass();
	return kGenericError;
}

Error Socket::Read(char* UNUSED(buffer), long UNUSED(length), long* UNUSED(count))
{
	Trespass();
	return kGenericError;
}

Error 
Socket::ReadLine(char* UNUSED(buffer), long UNUSED(length))
{
	Trespass();
	return kGenericError;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Boolean 
Socket::SocketIdle()
{
	Trespass();
	return false;
}
#endif

Error 
Socket::Write(const void* UNUSED(data), long UNUSED(length))
{
	Trespass();
	return kGenericError;
}

Error 
Socket::Write(const void* UNUSED(data), long UNUSED(length), long* UNUSED(count))
{
	Trespass();
	return kGenericError;
}

// ===========================================================================

#ifdef DEBUG_NAMES
const char*
GetPhaseString(Phase phase)
{
	static char buffer[50];
	const char* result;

	switch (phase) {
		case kIdle:				result = "Idle"; break;
		case kResolverBusy:		result = "Resolver Busy"; break;
		case kResolvingName:	result = "Resolving Name"; break;
		case kResolvedName:		result = "Resolved Name"; break;
		case kListen:			result = "Listen"; break;
		case kConnecting:		result = "Connecting"; break;
		case kConnected:		result = "Connected"; break;
		case kForcedIdle:		result = "ForcedIdle"; break;
		case kPeerClosed:		result = "PeerClosed"; break;
		case kClosing:			result = "Closing"; break;
		case kDead:				result = "Dead"; break;
		default:
			snprintf(buffer, sizeof(buffer), "Unknown phase (%lu)", (ulong)phase);
			result = &(buffer[0]);
			break;
	}
	return result;
}
#endif /* DEBUG_NAMES */

