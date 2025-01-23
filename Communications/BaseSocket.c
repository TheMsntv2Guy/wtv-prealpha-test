// ===========================================================================
//	BaseSocket.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __DEFS_H__
#include "defs.h"
#endif
#ifndef TINYIP_H
#include "tinyip.h"
#endif
#ifndef _TINYTCP_
#include "tinytcp.h"
#endif
#ifndef __SOCKET_H__
#include "Socket.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif


Error Socket::Init()
	{
	return kNoError;
	}

Socket::Socket()
	{
	}

Socket::~Socket()
{
}

Error Socket::Connect(const char* hostName, short hostPort)
{
#pragma unused (hostName, hostPort)
	return kNoError;
}

Error Socket::Connect(long hostAddress, short hostPort)
{
#pragma unused(hostAddress, hostPort)
	return kNoError;
}

Error Socket::Dispose()
{
	return kNoError;
}

long Socket::GetHostAddress()
{
	return 0;
}

Phase Socket::GetPhase()
{
	return kIdle;
}

Error Socket::Idle() 
{
	return kNoError;
}

#ifdef NEEDS_LISTEN
OSErr Socket::Listen(short localPort)
{
#pragma unused (localPort)
	return kNoError;
}
#endif

Error Socket::ReadIntoStream(Stream *stream)
{
#pragma unused (stream)
	return kNoError;
}

Error Socket::ReadLine(char* buffer, long /*length*/)
{
#pragma unused(buffer)
	return kPending;					// No line yet, but not an error
}

Boolean Socket::SocketIdle()
{
	return false;
}

Error Socket::Write(const void *data, long count)
{
#pragma unused(data, count)
	return kNoError;
}

Error Socket::WriteLine(const char *line)
{
#pragma unused(line)
	return kNoError;
}
