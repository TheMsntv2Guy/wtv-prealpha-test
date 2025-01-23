#include "Headers.h"

#ifndef __DEFS_H__
#include "defs.h"
#endif
#ifndef __TINYIP_H__
#include "tinyip.h"
#endif
#ifndef _TINYTCP_
#include "tinytcp.h"
#endif

#include "bip.h"

extern ip_State *gIPState;
extern ulong gOurIPAddr;

static long bipTCPReceive(tcp_Socket s, ulong self, uchar *buf, long count);

long	fReadHead;
long	fReadTail;
char	fReadBuffer[kBufferSize];
tcp_Socket fSocket;

long
bipWriteFIFO(uchar *src, ushort count)
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
		Message(("WriteFIFO:  Socket Full!", i, count));
		Message(("Advertised Window : %d", fSocket->rcv_wnd));		
		Message(("Next Data : %d", fSocket->rcv_nxt));
		Message(("Recieve Space Avail : %d", fSocket->rcv_space));
		Message(("Buffer Size Avail : %d", fSocket->rcv_buf));
	}

	return i;
}

Error
bipInit()
	{
	static int inited;

	if (!inited)				/* this will get called reentrantly, Tellyscript calls idle */
		{
		inited = true;
		gIPState = ipInit();
		}

	fReadHead = 0;
	fReadTail = 0;

	return kNoError;
	}

long
bipFIFOCount()
{
	return (ushort)((fReadHead - fReadTail) & kBufferMask);
}

static long
bipTCPReceive(tcp_Socket s, ulong self, uchar *buf, long count)
{
#pragma unused(s)

	if (buf) {
		if (count > 0) {
			count = bipWriteFIFO(buf, count);
		}
	} else {
		if (count == 0) {  		/* peer closed */
			Message(("TCPReceive: close"));
		}
		else               		/* TCP reset or failure */
		{
			Message(("TCPReceive: reset"));
		}
	}	
	return 0;
}

Error
bipConnect(long hostAddress, short hostPort)
{
	Message(("Connecting to 0x%lx, port 0x%lx\n", hostAddress, hostPort));

	SetIpAddress(gIPState, gOurIPAddr);
	fReadHead = 0;
	fReadTail = 0;
	fSocket = tcpOpen((tcp_PCB *)gIPState->ip_tcp, 0, hostAddress, hostPort, (upcall)bipTCPReceive, (uplink)0, WINDOW_SIZE + kMSSSlop);

/* need to add a callback to tcp to let us know when it's actually established
   the connection (or if it timed out trying).  Currently reads will fail (return 0)
   until it's connected */
   
	return kNoError;			/* or kNoConnection */
}

Error
bipClose()
{
	Message(("Closing Socket Connection <%d>", fSocket));

	if (fSocket != nil)
	{
		switch (fSocket->state)
			{
				case tcp_CLOSED:
				case tcp_CLOSING:
					Message(("Socket <%d> already closed", fSocket));
					return kNoError;
			}
	
		if (fSocket->state == tcp_SYNSENT)
			Message(("Closing socket in tcp_SYNSENT"));
			
		if (fSocket->state == tcp_SYNREC)
			Message(("Closing socket in tcp_SYNREC"));
		
//		tcpClose(fSocket);
		tcpAbort(fSocket);
	}	
	return kNoError;
}

Error
bipWrite(const void *data, long length, long *count)
{
	uchar *buffer = (uchar *)data;
	long n;

	if (length == 0)
		return kNoError;

	if (data == 0)
		return kGenericError;
		
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

	if (sent < *count)
	{
		*count = sent; 	
		return kPending;
	}
	
	return kNoError;
}

Error
bipRead(char* buffer, long length, long *count )
{
	ushort	waiting = bipFIFOCount();
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

		tcpCredit(fSocket, *count);
		return error;
		
	}

	return kPending;
}
