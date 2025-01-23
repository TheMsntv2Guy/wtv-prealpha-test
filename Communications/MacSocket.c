// ===========================================================================
//	MacSocket.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef FOR_MAC

#ifndef __LOCALNET_H__
#include "LocalNet.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif
#ifndef __SOCKET_H__
#include "Socket.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif



// ===========================================================================
//	locals/globals/defines
// ===========================================================================

#define	kBufferSize	0x00001000		// 1must be a power of 2 so mask will work
#define kBufferMask (kBufferSize-1)

static short gMacTCPRefNum = 0;
static MacSocket* gSocketList;
static short gResolverOpen = 0;

// Errors from -23000 to -23017
static char* TCPErrorNames23000to23017[] = 
{
	"ipBadLapErr",
	"ipBadCnfgErr",
	"ipNoCnfgErr",
	"ipLoadErr",
	"ipBadAddr",
	"connectionClosing",
	"invalidLength",
	"connectionExists",
	"connectionDoesntExist",
	"insufficientResources",
	"invalidStreamPtr",
	"streamAlreadyOpen",
	"connectionTerminated",
	"invalidBufPtr",
	"invalidRDS/WDS",
	"openFailed",
	"commandTimeout",
	"duplicateSocket"	
};

// Error codes from internal IP functions
static char* TCPErrorNames23032to23048[] =
{
	"ipDontFragErr",
	"ipDestDeadErr",
	"unused23034",
	"icmpEchoTimeoutErr",
	"ipNoFragMemErr",
	"ipRouteErr",
	"unused23039",
	"unused23039",
	"unused23040",
	"nameSyntaxErr",
	"cacheFault",
	"noResultProc",
	"noNameServer",
	"authNameErr",
	"noAnsErr",
	"dnrErr",
	"outOfMemory"
};

static char* TCPErrorNamesOthers[] = 
{
	"minusOne",
	"noErr",
	"inProgress",
};




// ===========================================================================
//	helper functions
// ===========================================================================

static char* TCPErrorName(short err)
{
	// Return a nicely formatted TCP error
	
	if ((err <= -23000) && (err >= -23017))
		return TCPErrorNames23000to23017[-23000 - err];
	if ((err <= -23032) && (err >= -23048))
		return TCPErrorNames23032to23048[-23032 - err];
	if ((err <= 1) && (err >= -1))
		return TCPErrorNamesOthers[1 + err];
	
	static char gErrStr[1+6+1+1]; /* '[' + %hd + ']' + NULL */
	snprintf(gErrStr, sizeof(gErrStr), "[%hd]", err);
	return gErrStr;
}

pascal void ResolverComplete(returnRec* /*returnedInfo*/, char* userDataPtr)
{
	if (userDataPtr)
	{
		MacSocket* socket = (MacSocket*)userDataPtr;
		socket->Resolved();
	}
}

//============================================================================
//	implementations
// ===========================================================================

Error MacSocket::Init()
	{
	static long gSocketID = 1024;
	
	OSErr		osErr;
	TCPiopb		pb;

#ifdef FOR_MAC	
	Assert(!gLocalNet->GetExclusiveRead());
#endif

	if (!gMacTCPRefNum)
	{
		if ((osErr = OpenDriver("\p.ipp", &gMacTCPRefNum)) != noErr)
			ImportantMessage(("Failed to open MacTCP: %d", osErr));
	}

	fSocketID = gSocketID++;
	fStreamBuffer = (Ptr)AllocateTaggedMemory(kBufferSize, "Stream Buffer");
	fReadBuffer = (Byte*)AllocateTaggedMemory(kBufferSize, "Read Buffer");
	fTimeoutTicks = 20*60;
	
	ZeroStruct(&pb);
	pb.csCode = TCPCreate;
	pb.ioCRefNum = gMacTCPRefNum;
	pb.csParam.create.rcvBuff = fStreamBuffer;
	pb.csParam.create.rcvBuffLen = kBufferSize;
	PBControlSync(ParmBlkPtr(&pb));
	
	if (pb.ioResult)
		Message(("Failed to create Socket <%d>: %s",fSocketID, TCPErrorName(pb.ioResult)));
	else
		fStream = pb.tcpStream;
	fPhase = kIdle;
	
	AddOrSet(gSocketList, this);
	return kNoError;
	}

MacSocket::MacSocket()
	{
	}

MacSocket::~MacSocket()
{
	TCPiopb pb;

	if (fStream) {
		ZeroStruct(&pb);
		pb.csCode = TCPRelease;
		pb.ioCRefNum = gMacTCPRefNum;
		pb.tcpStream = fStream;
		
		PBControlSync(ParmBlkPtr(&pb));
		if (pb.ioResult)
			Message(("Failed to release stream <%d>: %s",fSocketID,TCPErrorName(pb.ioResult)));
	}
	
	if (fStreamBuffer != nil)
		FreeTaggedMemory(fStreamBuffer, "Stream Buffer");

	if (fReadBuffer != nil)
		FreeTaggedMemory(fReadBuffer, "Read Buffer");
	
	if (gSocketList == this)
		gSocketList = (MacSocket*)Next();
}

Error MacSocket::Close()
{
	TCPiopb pb;
	OSErr osErr;
	
	osErr = GetStatus(&pb);
	if (osErr != noErr || pb.csParam.status.connectionState != 8) {
		fPhase = kDead;
		return kNoError;
	}

	ZeroStruct(&fTCPiopb);
	fTCPiopb.csCode = TCPClose;
	fTCPiopb.ioCRefNum = gMacTCPRefNum;
	fTCPiopb.tcpStream = fStream;
	fTCPiopb.csParam.close.ulpTimeoutValue = 10;		// 3 second timeout
	fTCPiopb.csParam.close.ulpTimeoutAction = 1;		// Abort
	fTCPiopb.csParam.close.validityFlags = 0xC0;		// ulpTimeoutValue and ulpTimeoutAction are valid 
	
	if (PBControlAsync(ParmBlkPtr(&pb)) != noErr) {
		Message(("Failed to close connection <%d>: %s", fSocketID, TCPErrorName(pb.ioResult)));
		return kCannotClose;
	}
	
	fPhase = kClosing;
	return kNoError;
}

Error MacSocket::Connect(const char* hostName, short hostPort)
{
	// Connect to a host, requires Addr2Name
	
	static	ResultUPP gResolverDoneUPP = NewResultProc(ResolverComplete);
	OSErr	osErr;

#ifdef FOO_DEBUG
	if (gSystem.GetLoginRequired() && strcmp(hostName, "arcadia") != 0)
		Complain(("Looking up hostname \"%s\" when service is enabled.", hostName));
#endif

	strcpy(fHostName, hostName);
	fHostPort = hostPort;
	
	if (gResolverOpen == 0) {
		if (OpenResolver(nil) != noErr)
			return kCannotOpenResolver;
		gResolverOpen++;
	}
		
	StrToAddr(fHostName, &fHostInfo, gResolverDoneUPP, (Ptr)this);
	osErr = fHostInfo.rtnCode;
	if (osErr == cacheFault) {
		fPhase = kResolvingName;
		return kNoError;
	}
	
	if (osErr == noErr)
		return Connect(fHostInfo.addr[0], hostPort);
	
	return kHostNotFound;
}

Error MacSocket::Connect(long hostAddress, short hostPort)
{
	// Connect to host using hostAddress
	
	OSErr	osErr;
	
	fHostAddress = hostAddress;
	fHostPort = hostPort;

	ZeroStruct(&fTCPiopb);
	fTCPiopb.csCode = TCPActiveOpen;
	fTCPiopb.ioResult = 1;
	fTCPiopb.ioCRefNum = gMacTCPRefNum;
	fTCPiopb.tcpStream = fStream;
	fTCPiopb.csParam.open.ulpTimeoutValue = fTimeoutTicks/60;
	fTCPiopb.csParam.open.ulpTimeoutAction = 1;						// Abort
	fTCPiopb.csParam.open.validityFlags = 0xC0;						// ulpTimeoutValue and ulpTimeoutAction are valid 
	fTCPiopb.csParam.open.commandTimeoutValue = fTimeoutTicks/60;
	fTCPiopb.csParam.open.remoteHost = hostAddress;
	fTCPiopb.csParam.open.remotePort = hostPort;
	
	if ((osErr = PBControlAsync(ParmBlkPtr(&fTCPiopb))) != noErr)
		Message(("Connection failed: %s",TCPErrorName(fTCPiopb.ioResult)));
	else
		fPhase = kConnecting;
		
	return osErr == noErr ? kNoError : kNoConnection;
}

Error MacSocket::Dispose()
{
	// Close the connection, any outstanding reads and writes ...
	
	return Close();
}

long
MacSocket::FIFOCount() const
{
	return (ushort)((fReadHead - fReadTail) & kBufferMask);
}

Error MacSocket::GetContiguousData(Byte **data, long *contig)
{
	// GetContiguousData returns amount of read read in one contig piece
	
	short	connectionState;
	long	unread;
	Error	error;
	
	if ((error = GetUnreadData(&unread,&connectionState)) != kNoError)
		return error;
	if (unread == 0)											// None available
		return (connectionState == 8) ? kPending :kNoConnection;// If connection is not ok, not going to get any more

	if ((fReadTail & kBufferMask) >= (fReadHead & kBufferMask))	// Only return contiguous space
		unread = kBufferSize - (fReadTail & kBufferMask);
	if (unread == 0)
		return kPending;
	
	*contig = unread;
	*data = (Byte *)fReadBuffer + (fReadTail & kBufferMask);
	return kNoError;
}

OSErr MacSocket::GetStatus(TCPiopb *pb)
{
	// Get the status of the current connection
	
	OSErr		osErr;

	ZeroStruct(pb);
	pb->csCode = TCPStatus;
	pb->ioCRefNum = gMacTCPRefNum;
	pb->tcpStream = fStream;
	if ((osErr = PBControlSync(ParmBlkPtr(pb))) != noErr)
	{
		char*	s = TCPErrorName(osErr);
		Message(("Status failed: %s",s));
	}
	return osErr;
}

Error MacSocket::GetUnreadData(long *unread, short *connectionState)
{
	// Fill the read buffer with as much data as will fit
	
	long	space;
	long	count;
	OSErr	osErr;
	TCPiopb pb;
	
	osErr = GetStatus(&pb);
	*unread = 0;
	*connectionState = pb.csParam.status.connectionState;
	if (osErr != noErr)
		return kNoConnection;
	
	// If there is unread data, read it into buffer
	if (pb.csParam.status.amtUnreadData != 0)
	{							
		// Only return contiguous space
		if ((fReadHead & kBufferMask) >= (fReadTail & kBufferMask))
			space = kBufferSize - (fReadHead & kBufferMask);
		else
			space = kBufferSize - (fReadHead - fReadTail);
			
		Assert(fReadTail <= fReadHead);
		Assert(space <= kBufferSize);
		
		if (space != 0)
		{
			long limitNetSpeed = gSimulator->GetUsePhone() ? 0 : gSimulator->GetLimitNetSpeed();

			if (limitNetSpeed != 0)
			{
				static UnsignedWide lastTime = {0, 0};
				UnsignedWide currentTime, difference;
				
				Microseconds(&currentTime);
				difference.hi = currentTime.hi - lastTime.hi - (currentTime.lo < lastTime.lo);
				difference.lo = currentTime.lo - lastTime.lo;
				
				if (difference.hi == 0)
				{
					unsigned long maxRead = difference.lo / (1000000 / (limitNetSpeed / 9));
					
					if (maxRead < space)
						space = maxRead;
					if (space == 0)
						space = 1;
				}
				lastTime = currentTime;
			}
			osErr = Receive(fReadBuffer + (fReadHead & kBufferMask), space, &count);
			fReadHead += count;
		}
	}
	
	*unread = fReadHead - fReadTail;
	return osErr == noErr ? kNoError : kNoConnection;
}

Error MacSocket::Idle() 
{
	// Called while IO is incomplete
	
	Error		error;
	
	switch (fPhase)
	{
		case kResolvedName:
			Assert(gResolverOpen != 0);
			if (fHostInfo.rtnCode < 0 || fHostInfo.addr == 0) {
				Message(("MacSocket: connect DNS error: %d", fHostInfo.rtnCode));
				fPhase = kIdle;
				return kHostNotFound;
			}
			
			if (fHostName[strlen(fHostName)-1] == '.')		// Trim trailing period from host name
				fHostName[strlen(fHostName)-1] = 0;
			Message(("MacSocket: resolved host name '%s'", fHostName));
			if ((error = Connect(fHostAddress,fHostPort)) != kNoError)
				return error;
			return kPending;								// Still in progress .....
			break;
		case kResolvingName:
			return kPending;
			break;
		default:
			if (fTCPiopb.ioResult < 0) {
				Message(("MacSocket: completed with an error: %s", TCPErrorName(fTCPiopb.ioResult)));
				return kNoConnection;
			}
			if (fTCPiopb.ioResult == noErr && fPhase == kConnecting)
				fPhase = kConnected;
			return fTCPiopb.ioResult == 1 ? kPending : kNoError;
	}
}

Error MacSocket::Listen(short localPort)
{
	// Listen on a port for a connection
	
	OSErr		osErr;

	ZeroStruct(&fTCPiopb);
	fTCPiopb.csCode = TCPPassiveOpen;
	fTCPiopb.ioResult = 1;
	fTCPiopb.ioCRefNum = gMacTCPRefNum;
	fTCPiopb.tcpStream = fStream;
	fTCPiopb.csParam.open.ulpTimeoutValue = fTimeoutTicks/60;
	fTCPiopb.csParam.open.ulpTimeoutAction = 1;						// Abort
	fTCPiopb.csParam.open.validityFlags = 0xC0;						// ulpTimeoutValue and ulpTimeoutAction are valid 
	fTCPiopb.csParam.open.commandTimeoutValue = fTimeoutTicks/60;
	fTCPiopb.csParam.open.localPort = localPort;
	
	if ((osErr = PBControlAsync(ParmBlkPtr(&fTCPiopb))) != noErr)
		Message(("Listen failed: %s",TCPErrorName(fTCPiopb.ioResult)));
	return osErr == noErr ? kNoError : kCannotListen;
}

Error MacSocket::ReadIntoStream(Stream *stream)
{
	// Read any available data into a stream
	
	Byte	*data;
	long	unread;
	Error	error;
	
	if ((error = GetContiguousData(&data,&unread)) != kNoError)		// Get a chunk of data that is in one piece
		return error;
	stream->Write(data,unread);
	fReadTail += unread;
	return kNoError;
}

Error MacSocket::ReadLine(char* buffer, long length)
{
	// Read a line of data, line end is defined by CRLF, 
	// CR or LF (but not LFCR)
	
	short	connectionState;
	long	unread;
	short	c,other;
	Error	error;
	long	p;
	
	if ((error = GetUnreadData(&unread,&connectionState)) != kNoError)
		return error;
	if (unread == 0)												// None available
		return (connectionState == 8) ? kPending : kNoConnection;	// If connection is not ok, not going to get any more
	
	p = fReadTail;
	while (p < fReadHead)
	{
		c = (*buffer++ = fReadBuffer[p++ & kBufferMask]);
		
		length--;
		if ( IsError(length == 0) )
			return kGenericError;
			
		if ((c == 0x0D) || (c == 0x0A))
		{
			buffer[-1] = 0;											// Got a complete line, strip the 0x0D
			if (p == fReadHead && c == 0x0D)
				break;												// Bounary case, can't have readTail > readHead
			
			other = fReadBuffer[p & kBufferMask];
			if (c == 0x0D && other == 0x0A)
				p++;												// And the 0x0A in in 0x0D,0x0A
			fReadTail = p;		
			return kNoError;
		}
	}
	return kPending;					// No line yet, but not an error
}


Error MacSocket::Read(char* buffer, long length, long *count)
{
	// Read a line of data, line end is defined by CRLF, 
	// CR or LF (but not LFCR)
	
	short	connectionState;
	long	unread;
	Error	error;
	long	p;
	
	if ((error = GetUnreadData(&unread,&connectionState)) != kNoError)
		return error;
	if (unread == 0)												// None available
		return (connectionState == 8) ? kPending : kNoConnection;	// If connection is not ok, not going to get any more
	
	count = 0;
	p = fReadTail;
	while (p < fReadHead)
	{
		*buffer++ = fReadBuffer[p++ & kBufferMask];
		
		length--;
		*count++;
		if (length == 0)
			return kNoError;
			
	}
	return kPending;					// No line yet, but not an error
}

Error MacSocket::Receive(void *data, long space, long *count)
{
	// Will pull as much out of the incoming buffer as it can
	
	TCPiopb		pb;
	OSErr		osErr;
	
	*count = 0;
	ZeroStruct(&pb);
	pb.csCode = TCPRcv;
	pb.ioCRefNum = gMacTCPRefNum;
	pb.tcpStream = fStream;
	pb.csParam.receive.commandTimeoutValue = fTimeoutTicks/60;
	pb.csParam.receive.rcvBuff = (Ptr)data;
	pb.csParam.receive.rcvBuffLen = space;
	
	if ((osErr = PBControlSync(ParmBlkPtr(&pb))) != noErr)
		Message(("MacSocket::Receive failed: %s", TCPErrorName(osErr)));
	else
		*count = pb.csParam.receive.rcvBuffLen;

	return osErr == noErr ? kNoError : kNoConnection;
}

void MacSocket::Resolved()
{
	// Called from ResolverDone when DNS nonsense is complete
	
	fHostAddress = fHostInfo.addr[0];
	fPhase = kResolvedName;
}

Error MacSocket::Send()
{	
	// Send data described by the current fWDS
	
	OSErr		osErr;
	
	ZeroStruct(&fTCPiopb);
	fTCPiopb.csCode = TCPSend;
	fTCPiopb.ioCRefNum = gMacTCPRefNum;
	fTCPiopb.tcpStream = fStream;
	fTCPiopb.csParam.send.ulpTimeoutValue = fTimeoutTicks/60;
	fTCPiopb.csParam.send.ulpTimeoutAction = 1;
	fTCPiopb.csParam.send.validityFlags = 0xC0;
	fTCPiopb.csParam.send.pushFlag = 1;
	fTCPiopb.csParam.send.wdsPtr = (Ptr)fWDS;

	if ((osErr = PBControlAsync(ParmBlkPtr(&fTCPiopb))) != noErr)
		Message(("MacSocket::Send failed: %s",TCPErrorName(fTCPiopb.ioResult)));
	return osErr == noErr ? kNoError : kNoConnection;
}

Boolean MacSocket::SocketIdle()
{
	// Delete closed sockets, return true if any are still closing.
	
	MacSocket* socket;
	MacSocket* next;
	
	for (socket = gSocketList; socket; socket = next) {
		next = (MacSocket*)socket->Next();
	
		if (socket->fPhase == kClosing)
			if (socket->fTCPiopb.ioResult <= noErr)
				socket->fPhase = kDead;
		
		if (socket->fPhase == kDead)
			delete(socket);
	}

	return gSocketList != nil;
}

Error MacSocket::Write(const void *data, long count)
{
	// Write data (note .. asynch write requires data to hang around until write is complete)
	
	fWDS[0].ptr = (Ptr)data;
	fWDS[0].length = count;
	fWDS[1].ptr = 0;
	fWDS[1].length = 0;
	return Send();
}

Error MacSocket::Write(const void *data, long length, long *count)
{
	// Write data (note .. asynch write requires data to hang around until write is complete)
	
	fWDS[0].ptr = (Ptr)data;
	fWDS[0].length = length;
	fWDS[1].ptr = 0;
	fWDS[1].length = 0;
	*count = length;
	return Send();
}

#endif /* FOR_MAC */