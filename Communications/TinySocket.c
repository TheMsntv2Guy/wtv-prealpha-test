// ===========================================================================
//	TinySocket.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __DEFS_H__
#include "defs.h"
#endif
#ifndef __DNR_H__
#include "dnr.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __SOCKET_H__
#include "Socket.h"
#endif
#ifndef __TINYIP_H__
#include "tinyip.h"
#endif
#ifndef _TINYTCP_
#include "tinytcp.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif


#if defined(SIMULATOR) && defined(DEBUG)
#include "SocketWindow.h"
#endif

// ============================================================================

static TinySocket* gSocketList;
extern ip_State *gIPState;
extern ulong gOurIPAddr;

struct TinySocketArrayElem gTinySocketArray[MAX_TINY_SOCKETS];

#define CLOSE_WAIT_TIME 3600 /* Time to wait for close to happen */
/* 
 * 
 * Be careful if you change these. kBufferSize needs to be bigger than WINDOW_SIZE
 * and should be a power of 2.
 *
 */

#define WINDOW_SIZE (tcp_MSS * 7)
//#define WINDOW_SIZE (tcp_MSS * 15)

#define kBufferSize 	0x1000
//#define kBufferSize 0x2000

#define kBufferMask		(kBufferSize-1)

// ============================================================================
// static functions
static long TCPReceive(tcp_Socket s, ulong self, uchar *buf, long count);

// ============================================================================

long
TinySocket::WriteFIFO(uchar *src, ushort count)
{
	unsigned i = 0;
	
	while (i < count)
	{
		if (fReadHead < fReadTail)
		{
			unsigned chunk = (fReadTail - fReadHead) - 1;
			
			if (chunk == 0)
				break;
			
			if ((chunk + i) >= count)
				chunk = (count - i);
			
			memmove((void *)&fReadBuffer[fReadHead], (const void *)src, chunk);
			i += chunk;
			src += chunk;
			fReadHead += chunk;
		}
		else
		{
			if (fReadHead >= fReadTail)
			{
				unsigned chunk = (kBufferMask - fReadHead);

				if (chunk == 0)
				{
					if (fReadTail != 0)
					{
						fReadBuffer[fReadHead] = *src;
						fReadHead = 0;
						i++;
						src++;
					}
					else
						break;
				}

				if ((chunk + i) >= count)
					chunk = (count - i);

				memmove((void *)&fReadBuffer[fReadHead], (const void *)src, chunk);
				i += chunk;
				src += chunk;
				fReadHead += chunk;
			
				if (((fReadHead + 1) & kBufferMask) == fReadTail)
					break;
			}
		}
	}

	if (i < count)
	{
		Message(("WriteFIFO:  Socket Full! (%d < %d)", i, (int)count));
		Message(("Advertised Window : %d", fSocket->rcv_wnd));		
		Message(("Next Data : %d", fSocket->rcv_nxt));
		Message(("Recieve Space Avail : %d", fSocket->rcv_space));
		Message(("Buffer Size Avail : %d", fSocket->rcv_buf));
	}

	fByteCount += count;

	return i;
}

//============================================================================

static long gNumberOfSocketsActive = 0;
static long gNumberOfSocketsInited = 0;

void 
TinySocket::Initialize()
{
	int i;

	if (IsError(kBufferSize <= WINDOW_SIZE)) {
		Message(("kBufferSize (%lu) must be greater than WINDOW_SIZE (%lu)!", (ulong)kBufferSize, (ulong)WINDOW_SIZE));
	}
	if (IsError((kBufferSize & (kBufferSize-1)) != 0)) {
		Message(("kBufferSize (%lu) must be a power of two!", (ulong)kBufferSize));
	}

	for (i = 0; i < MAX_TINY_SOCKETS; i++)
	{
		gTinySocketArray[i].tinysocket = new(TinySocket);
		gTinySocketArray[i].tinysocket->fReadBuffer = (Byte*)AllocateTaggedMemory(kBufferSize, "TinySocket Read Buffer");
		gTinySocketArray[i].flag = kTinyFree;
	}
	
}

#ifdef INCLUDE_FINALIZE_CODE
void
TinySocket::Finalize()
{
int i;

	for (i = 0; i < MAX_TINY_SOCKETS; i++)
	{
		FreeTaggedMemory(gTinySocketArray[i].tinysocket->fReadBuffer, "TinySocket Read Buffer");
		delete(gTinySocketArray[i].tinysocket);
	}
}
#endif /* INCLUDE_FINALIZE_CODE */

TinySocket* 
TinySocket::NewTinySocket()
{
	int i;

	for (i = 0; i < MAX_TINY_SOCKETS; i++)
	{
		if (gTinySocketArray[i].flag == kTinyFree)
		{
			gTinySocketArray[i].flag = kTinyInUse;
			return gTinySocketArray[i].tinysocket;
		}
	}	

	return nil;
}

void TinySocket::ReturnTinySocket(TinySocket *socket)
{
	int i;
	
	for (i = 0; i < MAX_TINY_SOCKETS; i++)
	{
		if (gTinySocketArray[i].tinysocket == socket)
		{
			if (IsError(gTinySocketArray[i].flag != kTinyInUse)) {
					Message(("Returning free socket"));
			}
			
			gTinySocketArray[i].flag = kTinyFree;
		}
	}	

	socket->TinySocketClean();

}

void
TinySocket::TinySocketClean()
{
	if (gSocketList == this)
		gSocketList = (TinySocket *)Next();

	Remove();

	if(IsError(fSocket != nil))
		fSocket = nil;
}

Error
TinySocket::Init()
	{
	static int inited;

	if (!inited)				/* this will get called reentrantly, Tellyscript calls idle */
		{
		inited = true;
		gIPState = ipInit();
		}

	if (gNumberOfSocketsActive > MAX_TINY_SOCKETS) 
	{
		Message(("Tried to Create more than 6 sockets"));
		return kGenericError; 
	}
	gNumberOfSocketsInited++;

	fReadHead = 0;
	fReadTail = 0;
	fByteCount = 0;
	fPhase = kIdle;
	fSocket = nil;

	AddOrSet(gSocketList, this);
	return kNoError;
	}


TinySocket::TinySocket()
{
	gNumberOfSocketsActive++;
}

TinySocket::~TinySocket()
{
	gNumberOfSocketsActive--;
}

long
TinySocket::FIFOCount() const
{
	return (ushort)((fReadHead - fReadTail) & kBufferMask);
}

ulong TinySocket::GetByteCount()
{
	return fByteCount;
}

void TinySocket::ResetByteCount()
{
	fByteCount = 0;
}

void
TinySocket::DNRResolved()
{
	// Called from DNRResolverComplete when DNS nonsense is complete
	
	fHostAddress = fHostInfo.addr[0];
	fPhase = kResolvedName;
}

void
DNRResolverComplete(DNRhostinfo *hostInfoPtr, char *userDataPtr)
{

#pragma unused(hostInfoPtr)
	if (userDataPtr)
	{
		TinySocket* socket = (TinySocket*)userDataPtr;
		socket->DNRResolved();
	}
}

Error
TinySocket::Connect(const char* hostName, short hostPort)
{

	if(hostName == "10.0.0.2") {
		Error result = Connect(0x0a000002, hostPort);
		return result;
	}

	// Connect to a host, requires Addr2Name
	
	Error	error;
	
	Message(("Connecting to '%s'...", hostName));
	strcpy(fHostName, hostName);
	fHostPort = hostPort;
	
	ResolverInit();

    Message(("HostByName from Connect!"));

	error = HostByName(fHostName, &fHostInfo, DNRResolverComplete, (char *)this);
	if (error == kGenericError)
		return kHostNotFound;

	error = (Error)fHostInfo.rtnCode;
	if (error == kDNRCacheFault) {
		fPhase = kResolvingName;
		return kNoError;
	}
	
	if(error == kDNRBusy) {
		fPhase = kResolverBusy;
		return kNoError;
	}
	
	if (error == kDNRSuccess)
	{
		fHostAddress = fHostInfo.addr[0];
		Error result = Connect(fHostInfo.addr[0], hostPort);
		strcpy(fHostName, hostName);
		return result;
	}

	return kHostNotFound;
}


long 
TinySocket::IncomingData(uchar *buf, long count)
{
	if (buf) {
		if (count > 0) {
			count = WriteFIFO(buf, count);
#ifdef DEBUG_SOCKETWINDOW
			SocketWindow::ReadIn(this, count);
#endif
		}
	} else {
		if (count == 0) {  							/* peer closed */
			if (fSocket != nil) { 
				if (fSocket->state == tcp_CLOSEWT) {
					fPhase = kPeerClosed;
				}
				if (fSocket->state == tcp_CLOSED) {
					fPhase = kClosing;	
#ifdef DEBUG_SOCKETWINDOW
					SocketWindow::Closing(this);
#endif
					fSocket = nil;
				}
			}	
			Message(("TCPReceive: close"));
		}
		else                 							/* TCP reset or failure */
		{
			Message(("TCPReceive: reset"));
			if ((fSocket != nil) && (fSocket->state == tcp_CLOSED))
			{
				fPhase = kClosing;	
#ifdef DEBUG_SOCKETWINDOW
				SocketWindow::Closing(this);
#endif
				fSocket = nil;
			}
		}
	}	
	return (0);
}

static long
TCPReceive(tcp_Socket s, ulong self, uchar *buf, long count)
{
#pragma unused(s)
	if (self == 0)
		return 0;
		
	return (((TinySocket *)self)->IncomingData(buf, count));
}

Error
TinySocket::Connect(long hostAddress, short hostPort)
{
	Message(("Connecting to 0x%lx, port 0x%lx\n", hostAddress, hostPort));

	SetIpAddress(gIPState, gOurIPAddr);
	fReadHead = 0;
	fReadTail = 0;
	fSocket = tcpOpen((tcp_PCB *)gIPState->ip_tcp, 0, hostAddress, hostPort, (upcall)TCPReceive, (uplink)this, WINDOW_SIZE + kMSSSlop);

/* need to add a callback to tcp to let us know when it's actually established
   the connection (or if it timed out trying).  Currently reads will fail (return 0)
   until it's connected */
   
	fHostAddress = hostAddress;
	fHostPort = hostPort;
	snprintf(fHostName, sizeof(fHostName), "%d.%d.%d.%d:%hu",
				(fHostAddress >> 24) & 0x0ff,
				(fHostAddress >> 16) & 0x0ff,
				(fHostAddress >> 8) & 0x0ff,
				fHostAddress & 0x0ff,
				fHostPort);

	if (fSocket == nil)	{
   		fPhase = kForcedIdle;
		return kNoConnection;
	}
   
	fPhase = kConnecting;	
#ifdef DEBUG_SOCKETWINDOW
	SocketWindow::Connecting(this);
#endif
	return kNoError;			/* or kNoConnection */
}

Error
TinySocket::Listen(short ourPort)
{
	SetIpAddress(gIPState, gOurIPAddr);
	fSocket = tcpListen((tcp_PCB *)gIPState->ip_tcp, ourPort, (upcall)TCPReceive, (uplink)this, WINDOW_SIZE + kMSSSlop, 0);

   if(!fSocket)
   {
   		fPhase = kForcedIdle;
		return kNoConnection;
	}
   
	fPhase = kListen;	
	return kNoError;			/* or kNoConnection */
}


Error
TinySocket::Close()
{
	Message(("Closing Socket Connection <%d>", fSocket));

	if (fSocket != nil)
	{
		switch (fSocket->state)
			{
				case tcp_CLOSED:
				case tcp_CLOSING:
					Message(("Socket <%ld> already closed", (ulong)fSocket));
					fPhase = kDead;
					return kNoError;
			}
	
		if (fSocket->state == tcp_SYNSENT)
			Message(("Closing socket in tcp_SYNSENT"));
			
		if (fSocket->state == tcp_SYNREC)
			Message(("Closing socket in tcp_SYNREC"));
		
//		tcpClose(fSocket);
		tcpAbort(fSocket);
	}	
	fPhase = kClosing;
#ifdef DEBUG_SOCKETWINDOW
	SocketWindow::Closing(this);
#endif
	fCloseTimer = Now() + CLOSE_WAIT_TIME;
	return kNoError;
}

Error
TinySocket::Dispose()
{
	// Close the connection, any outstanding reads and writes ...

	return Close();
}

Error
TinySocket::Idle() 
{
	PerfDump perfdump("TinySocket::Idle");

	// Called while IO is incomplete

	Error		error;

	switch (fPhase)
	{
		case kResolvedName:
			if (fHostInfo.rtnCode < 0)
			{
				Message(("Connect DNS failed...(%d)",fHostInfo.rtnCode));
				fPhase = kForcedIdle;
				return kHostNotFound;
			}
			Message(("Resolved name as '%s', connecting...",fHostName));
			if ((error = Connect(fHostAddress,fHostPort)) != kNoError)
				return error;
			return kPending;								// Still in progress .....
			break;
		case kResolvingName:
		{
			DNRHandlerLoop();
			return kPending;
			break;
		}
		case kResolverBusy:
			Message(("HostByName from Idle!"));
			error = HostByName(fHostName, &fHostInfo, DNRResolverComplete, (char *)this);
			if (error == kGenericError) 
			{
				fPhase = kForcedIdle;
				return kHostNotFound;
			}
			
			if (fHostInfo.rtnCode == kDNRSuccess)
			{
			if ((error = Connect(fHostAddress,fHostPort)) != kNoError)
				return error;
			return kPending;								// Still in progress .....			
			}
			
			if (fHostInfo.rtnCode == kDNRCacheFault) 
			{
				fPhase = kResolvingName;
				return kPending;
			}
			
			if(fHostInfo.rtnCode == kDNRBusy) 
				return kPending;
			break;
			
		case kForcedIdle:
			Message(("Called Socket->Idle() on kForcedIdle socket"));
			return kHostNotFound;
			break;
	
		case kPeerClosed:
			Message(("Called Socket->Idle() on kClosing socket"));
			return kNoConnection;
	
		case kClosing:
			Message(("Called Socket->Idle() on kClosing socket"));
			return kNoConnection;
			
		case kDead:
			Message(("Called Socket->Idle() on kDead socket"));
			return kNoConnection;
			
		default:
			if ((fSocket) && (fSocket->state == tcp_ESTAB))
			{
				fPhase = kConnected;
				return kNoError;
			}
	}
/* return:

		kNoConnection when you want to give up
		kPending when you're still working on it
		kNoError when you're connected (set fPhase = kConnected) */

			return kPending;
}

Error 
TinySocket::ReadIntoStream(Stream *stream)
{
	ushort	waiting = FIFOCount();
	long	count = 0;

	if (waiting)
	{
		while (waiting > 0)
		{
			if (fReadTail > fReadHead)
			{
				int	chunk = (kBufferSize - fReadTail);
										
				stream->Write((const void *)&fReadBuffer[fReadTail], chunk);
				fReadTail = 0;
				count += chunk;
				waiting -= chunk;
			}
			else
			{
				int chunk = (fReadHead - fReadTail);

				stream->Write((const void *)&fReadBuffer[fReadTail], chunk);
				fReadTail = fReadHead;
				count += chunk;
				waiting -= chunk;
			}

		}
	
		if ((fSocket != nil) && (fPhase == kConnected)) {
			tcpCredit(fSocket, count);
#ifdef DEBUG_SOCKETWINDOW
			SocketWindow::ReadOut(this, count);
#endif
		}							
		return kNoError;
	}

	if (fPhase != kConnected)
	{
		if (fPhase == kClosing)
			return kLostConnection;
		
		if ((fPhase == kDead) || (fPhase == kForcedIdle) || (fPhase == kPeerClosed))
			return kNoConnection;
			
		if ((fSocket != nil) && (fSocket->state == tcp_ESTAB))
				fPhase = kConnected;
	}
	
	return kPending;
}

Error
TinySocket::ReadLine(char* buffer, long length )
{
	ushort	waiting = FIFOCount();
	uchar	c, other;
	long	count = 0;
	long	savedReadTail = fReadTail;
	
	if (waiting > 0)
	{
		while (waiting--)
		{
			 c = (*buffer++ = fReadBuffer[fReadTail++]);		 
			length--;
			count++;
			if ( IsError(length == 0) )
				return kGenericError;
			fReadTail &= kBufferMask; 
			if ((c == 0x0D) || (c == 0x0A))
			{
				buffer[-1] = 0;
				other = fReadBuffer[fReadTail];
				if (c == 0x0D && other == 0x0A)
				{
					fReadTail++;
					fReadTail &= kBufferMask; 
				}
				if ((fSocket != nil) && (fPhase == kConnected)) {
					tcpCredit(fSocket, count);
#ifdef DEBUG_SOCKETWINDOW
					SocketWindow::ReadOut(this, count);
#endif
				}
				return kNoError;
			}
		}
	}

	if (fPhase != kConnected)
	{
		if (fPhase == kClosing)
			return kLostConnection;
		
		if ((fPhase == kDead) || (fPhase == kForcedIdle) || (fPhase == kPeerClosed))
			return kNoConnection;
	}
	
	fReadTail = savedReadTail;
	return kPending;		/* No whole line yet */

}

Boolean
TinySocket::SocketIdle()
{
	TinySocket* socket;
	TinySocket* next;
	
	for (socket = gSocketList; socket; socket = next) {
		next = (TinySocket*)socket->Next();
		
		if (socket->fPhase == kClosing) {
			if (socket->fSocket == nil) {
				socket->fPhase = kDead;
			} else {
				ulong now = Now();
				if ((int)(now - socket->fCloseTimer) > 0) {
					tcpAbort(socket->fSocket);
					Message(("Aborted socket due to timeout"));
				}
			}
		}
		
		if (socket->fPhase == kDead) {
				ReturnTinySocket(socket);
		}		
	}

	return gSocketList != nil;
}


Error
TinySocket::Write(const void *data, long count)
{
	uchar *buffer = (uchar *)data;
	long n;
	long write_count;

	if (count == 0)
		return kNoError;

	if (data == 0)
		return kGenericError;
		
	if (fPhase != kConnected)
	{
		if (fPhase == kClosing)

			return kLostConnection;
		else
			return kNoConnection;
	}
		
	if (fSocket == nil)
		return kNoConnection;
		
	write_count = count;

	long sent = 0;		
	while(true)
	{
		n = tcpSend(fSocket, buffer, count);
		count -= n;
		sent += n;
		buffer += n;
		if ((count == 0) || (n == 0))
			break;
	}
#ifdef DEBUG_SOCKETWINDOW
	SocketWindow::WriteIn(this, sent);
	SocketWindow::WriteOut(this, sent);
#endif

	if (sent < write_count)
		return kPending;
	
	return kNoError;
}


Error
TinySocket::Write(const void *data, long length, long *count)
{
	uchar *buffer = (uchar *)data;
	long n;
	
	*count = 0;

	if (length == 0)
		return kNoError;

	if (data == 0)
		return kGenericError;
		
	if (fPhase != kConnected)
	{
		if (fPhase == kClosing)
			return kLostConnection;
		
		if ((fPhase == kDead) || (fPhase == kForcedIdle)  || (fPhase == kPeerClosed))
			return kNoConnection;
	
		if ((fSocket != nil) && (fSocket->state == tcp_ESTAB))
				fPhase = kConnected;
		else
			{	
				*count = 0; 
				return kPending;
			}
	}
		
	*count = length;
		
	long sent = 0;
	while(true) {
		n = tcpSend(fSocket, buffer, length);
		length -= n;
		sent += n;
		buffer += n;
		if ((length == 0) || (n == 0))
			break;
	}
#ifdef DEBUG_SOCKETWINDOW
		SocketWindow::WriteIn(this, sent);
		SocketWindow::WriteOut(this, sent);
#endif

	if (sent < *count)
	{
		*count = sent; 	
		return kPending;
	}
	
	return kNoError;
}

Error
TinySocket::Read(char* buffer, long length, long *count )
{
	ushort	waiting = FIFOCount();
	Error   error;

	*count = 0;

	if (waiting > 0)
	{
		while (waiting--)
		{
			(*count)++;
			*buffer++ = fReadBuffer[fReadTail++];
			fReadTail &= kBufferMask; 
			length--;
			if (length == 0)
				break;
		}
		
		if (length == 0)
			error = kNoError;
		else
			error = kPending;

		if ((fSocket != nil) && (fPhase == kConnected)) {
#ifdef DEBUG_SOCKETWINDOW
			SocketWindow::ReadOut(this, *count);
#endif
			tcpCredit(fSocket, *count);
		}
		return error;
		
	}

	if (fPhase != kConnected)
	{
		if (fPhase == kClosing)
			return kLostConnection;
		
		if ((fPhase == kDead) || (fPhase == kForcedIdle)  || (fPhase == kPeerClosed))
			return kNoConnection;
	
		if ((fSocket != nil) && (fSocket->state == tcp_ESTAB))
				fPhase = kConnected;
	}

	return kPending;
}


