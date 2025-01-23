// ===========================================================================
//	Socket.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"
#endif

#ifndef __LINKABLE_H__
#include "Linkable.h"
#endif

#ifndef __DEFS_H__
#include "defs.h"
#endif

#ifndef __TINYIP_H__
#include "tinyip.h"
#endif

#ifndef _TINYTCP_
#include "tinytcp.h"
#endif

#ifndef __STREAM_H__
#include "Stream.h"
#endif

#if defined FOR_MAC
	#ifndef __ADDRESSXLATION_H__
	#include "AddressXLation.h"
	#endif
#endif

#ifdef SIMULATOR
	class SocketWindow;
#endif

#include "dnr.h"

enum Phase
{	
	kIdle = 0,
	kResolverBusy,
	kResolvingName,
	kResolvedName,
	kListen,
	kConnecting,
	kConnected,
	kForcedIdle,
	kPeerClosed,
	kClosing,
	kDead,
	kNumPhases
};
typedef enum Phase Phase;
#ifdef DEBUG
const char* GetPhaseString(Phase phase);
#endif

class Socket : public Linkable {
public:
					Socket();
	virtual			~Socket();
	virtual	Error	Init();
	
	static Socket*	NewSocket();
	static Socket*	NewSocket(Boolean old);

					
	Phase			GetPhase() const { return fPhase; };
	long			GetHostAddress() const { return fHostAddress; };
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	const char*		GetHostName() const { return fHostName; };
	ushort			GetHostPort() const { return fHostPort; };
#endif
	virtual long	FIFOCount() const;

	virtual Error	Connect(const char* hostName, short hostPort);
	virtual Error	Connect(long hostAddr, short hostPort);
	virtual Error	Dispose();
	virtual Error	Idle();
	virtual Error	Listen(short localPort);
	virtual Error	ReadIntoStream(Stream* stream);
	virtual Error   Read(char* buffer, long length, long *count);
	virtual Error	ReadLine(char* buffer, long length);
	virtual Error	Write(const void* data, long length);
	virtual Error	Write(const void* data, long length, long* count);

	static Boolean	SocketIdle();
	
protected:
#ifdef SIMULATOR
	friend SocketWindow;
#endif
	uchar			*fReadBuffer;
	long			fReadHead;
	long			fReadTail;
	Phase			fPhase;
	long			fHostAddress;
	long			fHostPort;
	char			fHostName[256];
};


class TinySocket : public Socket {
public:

#define MAX_TINY_SOCKETS 6

					TinySocket();
	virtual			~TinySocket();
	virtual	Error	Init();
	
	static TinySocket*	NewTinySocket();
	static void ReturnTinySocket(TinySocket *socket);

	virtual long	FIFOCount() const;

	virtual Error	Connect(const char* hostName, short hostPort);
	virtual Error	Connect(long hostAddr, short hostPort);
	virtual Error	Dispose();
	virtual Error	Idle();
	virtual Error	Listen(short localPort);
	virtual Error   Read(char* buffer, long length, long *count);
	virtual Error	ReadIntoStream(Stream* stream);
	virtual Error	ReadLine(char* buffer, long length);
	virtual Error	Write(const void* data, long length);
	virtual	Error   Write(const void* data, long length, long* count);
	
	virtual ulong	GetByteCount();
	virtual void	ResetByteCount();
	
	virtual void	TinySocketClean();

	static Boolean	SocketIdle();
	virtual long	IncomingData(uchar *buf, long count);
	virtual void    DNRResolved();

#ifdef INCLUDE_FINALIZE_CODE
	static void		Finalize();
#endif /* INCLUDE_FINALIZE_CODE */
	static void		Initialize();

	friend void		tcp_Free(tcp_Socket);
	friend void 	DNRResolverComplete(DNRhostinfo *returnedInfo, char *userDataPtr);
private:
#ifdef SIMULATOR
	friend SocketWindow;
#endif
	Error			Close();
	Error			ReadFIFO(uchar *dest, ushort count);
	long			WriteFIFO(uchar *src, ushort count);

	tcp_Socket  	fSocket;
	DNRhostinfo		fHostInfo;			// Name resolver nonsense
	void*			fTinySocketStruct;
	ulong			fCloseTimer;
	ulong			fByteCount;
};

enum TinyFlag {kTinyFree, kTinyInUse};

struct TinySocketArrayElem {
		TinySocket *tinysocket;
		enum TinyFlag flag;
};

extern struct TinySocketArrayElem gTinySocketArray[];

#ifdef NOT_NEEDED_NOW
class SecureSocket : public Socket {
public:
					SecureSocket();
	virtual			~SecureSocket();
	
	virtual long	FIFOCount() const;
	
	virtual Error	Connect(const char* hostName, short hostPort);
	virtual Error	Connect(long hostAddr, short hostPort);
	virtual Error	Dispose();
	virtual Error	Idle();
	virtual Error	Listen(short localPort);
	virtual Error   Read(char* buffer, long length, long *count);
	virtual Error	ReadIntoStream(Stream* stream);
	virtual Error	ReadLine(char* buffer, long length);
	virtual Error	Write(const void* data, long length);
	virtual	Error   Write(const void* data, long length, long* count);

	void	SetRealSocket(Socket *socket);
	Error	SetSessionKeys(uchar* key1, uchar*key2);
	
	virtual ulong	GetByteCount();
	virtual void	ResetByteCount();
	
	static Boolean	SocketIdle();
	static Error	ProcessChallenge(uchar* keys, uchar** response, uchar** key1, uchar** key2);

private:
	TinySocket*	fRealSocket;
	Boolean		fSocketConnected;
	RC4_KEY		fSendKey;
	RC4_KEY		fReceiveKey;
	
};
#endif


#ifdef FOR_MAC
class MacSocket : public Socket {
public:
					MacSocket();
	virtual			~MacSocket();
	virtual	Error	Init();

	virtual long	FIFOCount() const;

	virtual Error	Connect(const char* hostName, short hostPort);
	virtual Error	Connect(long hostAddr, short hostPort);
	virtual Error	Dispose();
	virtual Error	Idle();
	virtual Error	Listen(short localPort);
	virtual Error	ReadIntoStream(Stream* stream);
	virtual Error	ReadLine(char* buffer, long length);
	virtual Error	Write(const void* data, long count);

	virtual Error   Read(char* buffer, long length, long *count);
	virtual Error   Write(const void *data, long length, long *count);

	static Boolean	SocketIdle();

	friend pascal void ResolverComplete(returnRec *returnedInfo, char *userDataPtr);

private:
	Error			Close();
	Error			ReadFIFO(uchar *dest, ushort count);
	long			WriteFIFO(uchar *src, ushort count);

	void			Resolved();			// Don't call this
	OSErr			GetStatus(TCPiopb* pb);

	Error			Receive(void* data, long space, long* count);
	Error			Send();

	Error			GetUnreadData(long* unread, short* connectionState);
	Error			GetContiguousData(uchar** data, long* contig);

	tcp_Socket  	fSocket;
	long			fSocketID;
	hostInfo		fHostInfo;			// Name resolver nonsense
	
	TCPiopb			fTCPiopb;
	long			fTimeoutTicks;
	StreamPtr		fStream;
	Ptr				fStreamBuffer;
	wdsEntry		fWDS[4+1];			// Up to 4 chunks of 16k plus terminator

	short			fCacheThis;
};
#endif

//============================================================================
//	Useful utils

//  This is probably a bad macro to use...it doesn't guarantee that there's
//  enough room in _str to write an IP address, it's not a safe macro (i.e.,
//  _ip is evaluated multiple times, and it's not correctly parenthesized
//  if _ip is a complex expression.  I'm commenting it while I make sure nobody
//  was depending on it.

// #define IPStr(_ip,_str) sprintf(_str,"%d.%d.%d.%d",(_ip >> 24) & 0xFF, (_ip >> 16) & 0xFF, (_ip >> 8) & 0xFF, _ip & 0xFF);


#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Socket.h multiple times"
	#endif
#endif /* __SOCKET_H__ */
