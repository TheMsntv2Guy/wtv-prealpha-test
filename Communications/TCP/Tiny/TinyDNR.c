
#include "Headers.h"

#include "Cache.h"
#include "defs.h"
#include "dnr.h"
#include "DNRerrno.h"
#include "in.h"

#include "MemoryManager.h"

#include "nameser.h"
#include "netdb.h"
#include "resolv.h"

#include "Socket.h"
#include "Stream.h"
#include "System.h"
#include "tinyip.h"
#include "tinytcp.h"
#include "tinyudp.h"

#include "Utilities.h"

/*
 *	The DNR routines need to operate in a polled mode. We can't block waiting for the response, so a query
 *	is broken into a number of stages.
 *
 *	LIMITATIONS :
 *		- Only fully qualified host names are currently supported.
 *		- Only host name lookups are supported. Others types can be added,
 *				but there doesn't seem to be a pressing need unless we want the client to do
 *				direct mailing.
 *		
 *	TO BE FIXED :
 *		- Only one name server is currently used
 */

#ifdef FOR_MAC
#pragma options align = mac68k
typedef union {
    HEADER hdr;
    unsigned char buf[1024];
} querybuf;

typedef union {
    unsigned int al;
    char ac;
} align;
#pragma options align = reset
#

#else

typedef union {
    HEADER hdr __attribute__ ((packed)) ;
    unsigned char buf[1024] __attribute__ ((packed)) ; 
} querybuf;

typedef union {
    unsigned int al __attribute__ ((packed));
    char ac __attribute__ ((packed));
} align;

#endif /* FOR_MAC */




/* State of the Resolver */
static DNRState gDNRState;
static DNRConnectState gDNRConnectState;
static unsigned int gDNRRunning;
static int gDNRh_errno;

/* Resolver data */

#define	MAXALIASES	35
#define	MAXADDRS	35
#define MAXPACKET 1024
#define CACHE_TIMEOUT 72000 /* (20 * 60 * 60) 20 minutes */
#define QUERY_TIMEOUT  2700     /* (60 * 45) 45 seconds */
/*
 * Resolver state default settings
 */

static struct __res_state _res = {
	RES_TIMEOUT,               	/* retransmition time interval */
	4,                         	/* number of times to retransmit */
	RES_DEFAULT,			/* options flags */
	1,                         	/* number of name servers */
};


typedef struct dnr_cache_entry {
	char name[101];
	ulong hits;
	ulong address;
} dnr_cache_entry_t;
 
#define IP(a,b,c,d) (((ulong)(a) << 24) | ((ulong)(b) << 16) | ((ulong)(c) << 8) | ((ulong)d))

#define CACHE_SIZE 3

static dnr_cache_entry_t dnr_cache[CACHE_SIZE];
static ulong gDNRCacheFlushTime;
static ulong gDNRQueryTimeoutTime;

static char *h_addr_ptrs[MAXADDRS + 1];

static struct hostent host;
static char *host_aliases[MAXALIASES];
static char hostbuf[100+1];
static char host_name_buf[101];


/* Data for the current request */
static Socket* gDNRSocket;
static MemoryStream* gDNRStream; 
static char *gDNRhostName;
DNRhostinfo *gDNRhostInfoPtr;
static DNRResultProcPtr gDNRResultProc ;
static char *gDNRuserDataPtr;
static querybuf gDNRQueryBuffer;
static ushort gDNRExpectedData;
static int gDNRQueryLength;
static int gDNRRetry; 

#ifdef FOR_MAC
#pragma options align = mac68k4byte
static char gDNRbuf[MAXPACKET];
#pragma options align = reset
#else
static char gDNRbuf[MAXPACKET] __attribute__ ((aligned (4)));
#endif


/* Forward definitons for our implementation */


static int	DNRBuildQuery(void);
static struct hostent * DNRGetAnswer(void);
static int DNRSendQuery(void);

ulong inet_addr(const char *cp);

int inet_aton(const char *cp, struct in_addr *addr);

static int dn_expand(const unsigned char *msg, const unsigned char *eomorig, 
					unsigned char *comp_dn, unsigned char *exp_dn, int length);
static int __dn_skipname(const unsigned char *comp_dn, const unsigned char *eom);
unsigned short _getshort(unsigned char *msgp);
unsigned int _getlong(unsigned char *msgp);
static void __putshort(unsigned short s, unsigned char *msgp);
/* static void __putlong(unsigned long l,  unsigned char *msgp); */
static int dn_comp(const unsigned char *exp_dn, unsigned char *comp_dn, int length, 
							unsigned char **dnptrs, unsigned char **lastdnptr);
static int dn_find(unsigned char *exp_dn, unsigned char *msg, unsigned char **dnptrs, unsigned char **lastdnptr);

#define ENABLE_CACHE_FLUSH
//#define PRELOAD_CACHE

static void DNRFlushCache (void)
{

//	Message(("DNRFlushCache\n"));

#ifdef ENABLE_CACHE_FLUSH
	int i;

	for (i=0; i < CACHE_SIZE; i++)
		{
		dnr_cache[i].name[0] = 0;
		dnr_cache[i].hits = 0;
		}
#endif
}

static void DNREnterCache(void) 
{
	int i, victim;
	ulong hits;
	
	hits = dnr_cache[0].hits;
	victim = 0;
	
	for (i=1; i < CACHE_SIZE; i++) {
		if (dnr_cache[i].hits < hits)
		{
			hits = dnr_cache[i].hits;
			victim = i;
		}
	}
	IsError(strlen(gDNRhostInfoPtr->cname) > 255);
	
	strcpy(dnr_cache[victim].name, gDNRhostInfoPtr->cname);
	dnr_cache[victim].hits = 1;
	dnr_cache[victim].address = gDNRhostInfoPtr->addr[0];

}

//#define DO_LISTEN_TEST
//#define DO_CONNECT_TEST
//#define DO_UDP_TEST
//#define DO_SECURE_TEST

#ifdef DO_UDP_TEST
extern ip_State *gIPState;
udp_Socket gUDPSocket;

static long
udp_datahandler(udp_Socket s, uplink link, uchar *buffer, long count)
{
#pragma unused(s)
#pragma unused(link)
#pragma unused(buffer)

	Message(("Recieved %d bytes on udp socket", count));

	return 0;

}

#endif

#ifdef DO_LISTEN_TEST
Socket *gListenSocket;
Boolean	gListening = false;
Boolean gConnected = false;
#endif  /* DO_LISTEN_TEST */

#ifdef DO_CONNECT_TEST
Socket *gConnectSocket;
Boolean gConnected = false;
#endif

#ifdef DO_SECURE_TEST
Socket *gSecureSocket;
Boolean gConnected = false;
Boolean gSecureReady = false;
#endif

Error ResolverInit(void)
{

	if (gDNRRunning)
		return(kNoError);
 
 	gDNRRunning = 1;

#ifdef APPROM
	_res.nsaddr_list[0] = (ulong)gSystem->GetNameServer();
#else
	_res.nsaddr_list[0] = (ulong)IP(10,0,0,2);					/* HACK - use mandarin for now */
#endif

	strcpy(_res.defdname, ".artemis.com");
	DNRFlushCache();
	
	gDNRCacheFlushTime = Now() + (ulong)CACHE_TIMEOUT;
	
#ifdef PRELOAD_CACHE	
	strcpy(dnr_cache[0].name, "10.0.0.2");
	dnr_cache[0].address = IP(10,0,0,2);
	dnr_cache[0].hits = 1;
#endif /* PRELOAD_CACHE */	
	
	gDNRState = kDNRIdle;
	
#ifdef DO_UDP_TEST
	gUDPSocket = udpOpen((udp_State)gIPState->ip_udp, 9, udp_datahandler, true);
#endif /* DO_UDP_TEST */
	
#ifdef DO_LISTEN_TEST
	gListenSocket = Socket::NewSocket(false);
	
	if(!gListenSocket->Listen(80))
	{
		Message((" Successful Listen()"));
		gListening = true;
		gConnected = false;
	}
	else
		gListenSocket->Dispose();
#endif /* DO_LISTEN_TEST */

#ifdef DO_CONNECT_TEST
	gConnectSocket = Socket::NewSocket(false);
	
	if(!gConnectSocket->Connect("10.0.0.2", 8101))
	{
			Message((" Successful Connect()"));
			gConnected = true;
	}
#endif /* DO_CONNECT_TEST */

#ifdef DO_SECURE_TEST
	gSecureSocket = Socket::NewSocket(true);
	
	if(!gSecureSocket->Connect("10.0.0.2", 8101))
	{
			Message((" Successful Connect()"));
			gConnected = true;
	}
#endif /* DO_SECURE_TEST */

	return(kNoError);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Error ResolverClose(void)
{
	gDNRRunning = 0;
	if (gDNRSocket)
	{
		gDNRSocket->Dispose();
	 	gDNRSocket = nil;
	 }
	 
#ifdef DO_UDP_TEST
	if (gUDPSocket)
	 	udpClose(gUDPSocket);
#endif /* DO_UDP_TEST */
	 
	return (kNoError);
}
#endif

Error HostByName(char *hostName, DNRhostinfo *hostInfoPtr, DNRResultProcPtr ResultProc, char *userDataPtr) 
{
	char *cp;
	int n = 0;

//	Message(("HostByName : Entered\n"));
		
	if (!gDNRRunning || !ResultProc || !userDataPtr || !hostName || !hostInfoPtr) 
	{
//		Message(("HostByName : Returned(kGenericError)\n"));
		return(kGenericError);
	}

	if (gDNRState != kDNRIdle) {
		hostInfoPtr->rtnCode = kDNRBusy;
//	Message(("HostByName : Returned(kNoError), kDNRBusy\n"));
		return(kNoError);
	}
		
	/* Minimal check for "good" host name */
	for (cp = hostName, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;
	
	if (n == 0) {
		if ((strlen(_res.defdname) + strlen(hostName) + 1) > sizeof(host_name_buf))
		{
//			Message(("HostByName : Returned(kGenericError), host_name_buf\n"));
			return (kGenericError);
		}
		strcpy(host_name_buf, hostName);
		strcat(host_name_buf, _res.defdname);
	} else if(n != 2) 
		   {
//				Message(("HostByName : Returned(kGenericError), bad name\n"));
				return (kGenericError);
			} else {
					if (strlen(hostName) > sizeof(host_name_buf))
//					Message(("HostByName : Returned(kGenericError), host_name_buf 2\n"));
					return (kGenericError);
					strcpy(host_name_buf, hostName);
			}
			
	gDNRhostName = host_name_buf;
	gDNRhostInfoPtr = hostInfoPtr;
	gDNRResultProc = ResultProc;
	gDNRuserDataPtr = userDataPtr;

	for (n = 0; n < CACHE_SIZE; n++) {
		if (dnr_cache[n].hits && !strncmp(gDNRhostName, dnr_cache[n].name, strlen(dnr_cache[n].name))) { /* cache hit */
			dnr_cache[n].hits++;
			gDNRhostInfoPtr->rtnCode = kDNRSuccess;
			strcpy(gDNRhostInfoPtr->cname, dnr_cache[n].name);
			gDNRhostInfoPtr->addr[0] = dnr_cache[n].address;
//			Message(("HostByName : Returned(kNoError), cache hit\n"));
			return(kNoError);
		}
	}

//	Message (("Name Query : %s", host_name_buf));
	 
	gDNRhostInfoPtr->rtnCode = kDNRCacheFault;
	 
	gDNRConnectState = kDNRNoConnect;
	gDNRState = kDNRResQueryDomain;
	gDNRRetry = 0;
	
//	Message(("HostByName : Returned(kNoError), lookup started\n"));

	return (kNoError);
}

/* Private implementations */


/* This is the loop that handles the process of making a DNS query and doing the callback to the socket that
 * needed name resolution.
 */
void DNRHandlerLoop (void)
{
	Error error;
	
	if (!gDNRRunning)
		return;
	
	switch (gDNRState) {
	
	case kDNRIdle:
	{
		ulong now = Now();

		if ((int)(now - gDNRCacheFlushTime) > 0)
		{
			DNRFlushCache();
			gDNRCacheFlushTime = now + (ulong)CACHE_TIMEOUT;
		}

#ifdef DO_LISTEN_TEST
		if (gListening)
		{
			Error error;
			
			if ((error = gListenSocket->Idle()) != kPending)
			{
				if (error == kNoError)
					Message(("Someone connected to Listen() socket"));
					
//				gListenSocket->Dispose();
				gListening = false;
				gConnected = true;
			}	
		}
		
		if (gConnected)
		{
			Error error;
			
			if ((error = gListenSocket->Idle()) != kNoError)
			{
				if (error == kNoConnection)
				{
					Message(("Listen() socket closed by peer"));
					gListenSocket->Dispose();
					gConnected = false;
				}
				else
					Message(("Listen() socket returned : %d",error));
			}
		}		
		
#endif  /* DO_LISTEN_TEST */


#ifdef DO_CONNECT_TEST
		if (gConnected)
		{
			Error error;
			
			if ((error = gConnectSocket->Idle()) != kPending)
			{
				if (error == kNoError)
					break;

				if (error == kNoConnection)
				{
					Message(("Connect() socket closed by peer"));
					gConnectSocket->Dispose();
					gConnected = false;
				}
			
				if (error == kHostNotFound)
				{
					Message(("Connect() host not found"));
					gConnectSocket->Dispose();
					gConnected = false;
				}
			}
		
		}
#endif /* DO_CONNECT_TEST */

#ifdef DO_SECURE_TEST
		if (gConnected)
		{
			Error error;
			
			if (gSecureReady)
			{
				char buffer[1024];
				long count;
				error = gSecureSocket->Read(buffer, 1023, &count);
				if (error)
				{
					if (error == kPending)
					break;
					
					Message(("SSL error"));
					gSecureSocket->Dispose();
					gConnected = false;

				}
				
				buffer[count+1] = 0;
				Message(("%s\n", buffer));
			}
			else if ((error = gSecureSocket->Idle()) != kPending)
			{
				if (error == kNoError) {
					gSecureReady = true;
					break;
				}

				if (error == kNoConnection)
				{
					Message(("Connect() socket closed by peer"));
					gSecureSocket->Dispose();
					gConnected = false;
				}
			
				if (error == kHostNotFound)
				{
					Message(("Connect() host not found"));
					gSecureSocket->Dispose();
					gConnected = false;
				}
				
				Message(("Connect() failed"));
				gSecureSocket->Dispose();
				gConnected = false;
			}
		
		}
#endif /* DO_SECURE_TEST */

		return;	
	}
	case kDNRResQueryDomain:						/* Doing a lookup */
		switch (gDNRConnectState) {
			case kDNRNoConnect:						/* Setup the query */
				gDNRh_errno = HOST_NOT_FOUND;		/* default, if we never query */
				DNRBuildQuery();
				if (((gDNRSocket = Socket::NewSocket()) == nil))
				{
					gDNRConnectState = kDNRConnectClosing;
					gDNRh_errno = NO_RECOVERY;
				}	
				else
					gDNRConnectState =  kDNRConnectTry;
				break;
		
			case kDNRConnectTry:					/* The socket needs time to get ready */
			{	
				ulong now = Now();
				
				if(gDNRSocket->GetPhase() == kIdle) {
					gDNRSocket->Connect(_res.nsaddr_list[0], (short)NAMESERVER_PORT);
// Message (("DNR Attempted connect"));
					gDNRQueryTimeoutTime = now + (ulong)QUERY_TIMEOUT;
				} else if (gDNRSocket->GetPhase() == kConnected) 
						{ /* Reached connected state */
							if (DNRSendQuery()) {
								gDNRConnectState = kDNRConnectClosing;
								gDNRh_errno = NO_RECOVERY;
							} else {	
								if ((gDNRStream = new(MemoryStream)) != nil)
								{
									gDNRConnectState = kDNRConnectRequest1;
									gDNRExpectedData = sizeof(short);
								} else
									{
					   	    			gDNRConnectState = kDNRConnectClosing;
										gDNRh_errno = NO_RECOVERY;									
									}
								}
				} else if (gDNRSocket->Idle() == kNoConnection) {
					gDNRConnectState = kDNRConnectClosing;
					gDNRh_errno = NO_RECOVERY;
				} else if ((int)(now - gDNRQueryTimeoutTime) > 0)
					   {
					   	    gDNRConnectState = kDNRConnectClosing;
							gDNRh_errno = NO_RECOVERY;
						}
				break;
			}
			
			case kDNRConnectRequest1:
// Message (("DNR kDNRConnectRequest1")); 

				error = gDNRSocket->ReadIntoStream(gDNRStream);
// Message (("DNR kDNRConnectRequest1 error : %d", error));

				if((error != kPending) && ((gDNRStream->GetPending()) >= 2)) {
					gDNRStream->Read((uchar *)&gDNRQueryBuffer, 2);
					gDNRExpectedData = *(ushort *)&gDNRQueryBuffer;
					if (gDNRExpectedData > sizeof(gDNRQueryBuffer))
						gDNRExpectedData = sizeof(gDNRQueryBuffer);
					gDNRConnectState = kDNRConnectRequest2;
				} else if (gDNRSocket->Idle() == kNoConnection) {
					gDNRConnectState = kDNRConnectClosing;
					gDNRh_errno = NO_RECOVERY;
				}
				break;	
				
			case kDNRConnectRequest2:
// Message (("DNR kDNRConnectRequest2")); 

				gDNRSocket->ReadIntoStream(gDNRStream);

				if (gDNRStream->GetPending() >= gDNRExpectedData) {
						gDNRStream->Read((uchar *)&gDNRQueryBuffer, gDNRExpectedData);
						gDNRConnectState = kDNRConnectResponse;
				} else if (gDNRSocket->Idle() == kNoConnection) {
					gDNRConnectState = kDNRConnectClosing;
					gDNRh_errno = NO_RECOVERY;
				}				
				break;
						
			case kDNRConnectResponse:   /* Process the response from the server */
				if (DNRGetAnswer()) {
					strcpy(gDNRhostInfoPtr->cname, gDNRhostName);
					gDNRhostInfoPtr->addr[0] = ((struct in_addr *)host.h_addr_list[0])->s_addr;
					DNREnterCache();
					gDNRConnectState = kDNRConnectClosing;
					gDNRh_errno = 0;
				} else if ((gDNRh_errno == TRY_AGAIN) && (!gDNRRetry)) 
					   {
							if (gDNRSocket)
							{
								gDNRSocket->Dispose();
								gDNRSocket = nil;
							}
							if ((gDNRStream) && (gDNRStream->GetStatus() == kPending))
							{
								gDNRStream->SetStatus(kStreamReset);
								delete(gDNRStream);
							}
							gDNRConnectState = kDNRNoConnect;
							gDNRRetry = 1;
						} else
							gDNRConnectState = kDNRConnectClosing;
				break;
			
			case  kDNRConnectClosing:	/* Close down the TCP connection */
// Message (("DNR kDNRConnectClosing"));
				if (gDNRSocket)
				{
					gDNRSocket->Dispose();
					gDNRSocket = nil;
				}
				if (gDNRStream)
				{
					delete(gDNRStream);
					gDNRStream = nil;
				}
				
				gDNRConnectState = kDNRConnectComplete;
				break;
								
			case kDNRConnectComplete:  /* return the result through the callback */
				
				if (gDNRh_errno == HOST_NOT_FOUND)
					gDNRhostInfoPtr->rtnCode = kDNRHostNotFound;
				else if (gDNRh_errno > 0)
					gDNRhostInfoPtr->rtnCode = kDNROtherError;
				else
					gDNRhostInfoPtr->rtnCode = kDNRSuccess;
				
				(*gDNRResultProc)(gDNRhostInfoPtr, gDNRuserDataPtr);
				
				gDNRState = kDNRIdle;
				gDNRConnectState = kDNRNoConnect;
				break;
		}
		
	default:
		return;
	}

	return;

}

static int	DNRBuildQuery()
{
	HEADER *hp;
	char *buf;
	char *cp;
	int n,buflen;
	char *dnptrs[10], **dpp, **lastdnptr;

// Message (("DNRBuildQuery()"));

	buf = &gDNRbuf[2];
	buflen = sizeof(gDNRbuf) - 2;
	hp = (HEADER *) buf;
	hp->id = ++_res.id;
	hp->opcode = QUERY;
	hp->pr = (_res.options & RES_PRIMARY) != 0;
	hp->rd = (_res.options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	cp = buf + sizeof(HEADER);
	buflen -= sizeof(HEADER);
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof(dnptrs)/sizeof(dnptrs[0]);
	n = dn_comp((uchar *)gDNRhostName, (uchar *)cp, buflen,
		   (uchar **)dnptrs, (uchar **)lastdnptr);
	cp += n;
	__putshort(T_A, (uchar *)cp);
	cp += sizeof(ushort);
	__putshort(C_IN, (uchar *)cp);
	cp += sizeof(ushort);
	hp->qdcount = 1;
	gDNRQueryLength = (cp - buf);
	return 0;
}

static struct hostent * DNRGetAnswer()
{
	register HEADER *hp;
	register uchar *cp;
	register int n;
	uchar *eom;
	char *bp, **ap;
	int type, hclass, buflen, ancount, qdcount;
	int haveanswer, getclass = C_ANY;
	char **hap;

// Message (("DNR DNRGetAnswer()")); 

	eom = gDNRQueryBuffer.buf + gDNRExpectedData;

	hp = &gDNRQueryBuffer.hdr;
	ancount = hp->ancount;
	qdcount = hp->qdcount;
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = gDNRQueryBuffer.buf + sizeof(HEADER);
	if (qdcount) {
		cp += __dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
		cp += __dn_skipname(cp, eom) + QFIXEDSZ;
	}
	ap = &host_aliases[0];
	*ap = NULL;
	host.h_aliases = host_aliases;
	hap = h_addr_ptrs;
	*hap = NULL;
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((uchar *)&gDNRQueryBuffer.buf, (uchar *)eom,
		    (uchar *)cp, (uchar *)bp, buflen)) < 0)
			break;
		cp += n;
		type = _getshort(cp);
 		cp += sizeof(ushort);
		hclass = _getshort(cp);
 		cp += sizeof(ushort) + sizeof(unsigned int);
		n = _getshort(cp);
		cp += sizeof(ushort);
		if (type == T_CNAME) {
			cp += n;
			if (ap >= &host_aliases[MAXALIASES-1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}

		if (haveanswer) {
			if (n != host.h_length) {
				cp += n;
				continue;
			}
			if (C_IN != getclass) {
				cp += n;
				continue;
			}
		} else {
			host.h_length = n;
			getclass = C_IN;
			host.h_addrtype = 2 /* AF_INET */;
			host.h_name = bp;
			bp += strlen(bp) + 1;
		}

		bp += sizeof(align) - ((unsigned int)bp % sizeof(align));

		if (bp + n >= &hostbuf[sizeof(hostbuf)]) {
			break;
		}
		CopyMemory(cp, *hap++ = bp, n);
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
		host.h_addr_list = h_addr_ptrs;
		return (&host);
	} else {
		gDNRh_errno = TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
}

static int DNRSendQuery(void) 
{
	Error error;
	ushort len;

// Message (("DNRSendQuery()")); 
	
	len = (ushort)gDNRQueryLength;
	
//	memmove(gDNRbuf, (uchar *)&len, 2);

	__putshort(len, (uchar *)&gDNRbuf);
	
	error = gDNRSocket->Write(gDNRbuf, gDNRQueryLength + 2);
	
	return (error);

}



/*
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 */
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong
inet_addr(const char *cp)
{
	struct in_addr val;

	if (inet_aton(cp, &val))
		return (val.s_addr);
	return (INADDR_NONE);
}
#endif

/* 
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
int
inet_aton(const char *cp, struct in_addr *addr)
{
	ulong val = 0;
	int base = 10, n;
	char c;
	unsigned int parts[4];
	unsigned int *pp = parts;

	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, other=decimal.
		 */

		while ((c = *cp) != '\0') {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				cp++;
				continue;
			}
			break;
		}
		if (*cp == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16-bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3 || val > 0xff)
				return (0);
			*pp++ = val, cp++;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && (!isascii(*cp) || !isspace(*cp)))
		return (0);
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = val;
	return (1);
}
#endif

/*
 * Expand compressed domain name 'comp_dn' to full domain name.
 * 'msg' is a pointer to the begining of the message,
 * 'eomorig' points to the first location after the message,
 * 'exp_dn' is a pointer to a buffer of size 'length' for the result.
 * Return size of compressed name or -1 if there was an error.
 */
static int dn_expand(const unsigned char *msg, const unsigned char *eomorig, unsigned char *comp_dn, 
								unsigned char *exp_dn, int length)
{
	register unsigned char *cp, *dn;
	register int n, c;
	unsigned char *eom;
	int len = -1, checked = 0;

	dn = exp_dn;
	cp = (unsigned char *)comp_dn;
	eom = exp_dn + length;
	/*
	 * fetch next label in domain name
	 */
	while ((n = *cp++) != 0) {
		/*
		 * Check for indirection
		 */
		switch (n & INDIR_MASK) {
		case 0:
			if (dn != exp_dn) {
				if (dn >= eom)
					return (-1);
				*dn++ = '.';
			}
			if (dn+n >= eom)
				return (-1);
			checked += n + 1;
			while (--n >= 0) {
				if ((c = *cp++) == '.') {
					if (dn + n + 2 >= eom)
						return (-1);
					*dn++ = '\\';
				}
				*dn++ = c;
				if (cp >= eomorig)	/* out of range */
					return(-1);
			}
			break;

		case INDIR_MASK:
			if (len < 0)
				len = cp - comp_dn + 1;
			cp = (unsigned char *)msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			if (cp < msg || cp >= eomorig)	/* out of range */
				return(-1);
			checked += 2;
			/*
			 * Check for loops in the compressed name;
			 * if we've looked at the whole message,
			 * there must be a loop.
			 */
			if (checked >= eomorig - msg)
				return (-1);
			break;

		default:
			return (-1);			/* flag error */
		}
	}
	*dn = '\0';
	if (len < 0)
		len = cp - comp_dn;
	return (len);
}

/*
 * Compress domain name 'exp_dn' into 'comp_dn'.
 * Return the size of the compressed name or -1.
 * 'length' is the size of the array pointed to by 'comp_dn'.
 * 'dnptrs' is a list of pointers to previous compressed names. dnptrs[0]
 * is a pointer to the beginning of the message. The list ends with NULL.
 * 'lastdnptr' is a pointer to the end of the arrary pointed to
 * by 'dnptrs'. Side effect is to update the list of pointers for
 * labels inserted into the message as we compress the name.
 * If 'dnptr' is NULL, we don't try to compress names. If 'lastdnptr'
 * is NULL, we don't update the list.
 */
static int dn_comp(const unsigned char *exp_dn, unsigned char *comp_dn, int length, 
							unsigned char **dnptrs, unsigned char **lastdnptr)
{
	register unsigned char *cp, *dn;
	register int c, l;
	unsigned char **cpp = NULL, **lpp = NULL, *sp, *eob;
	unsigned char *msg;

	dn = (unsigned char *)exp_dn;
	cp = comp_dn;
	eob = cp + length;
	if (dnptrs != NULL) {
		if ((msg = *dnptrs++) != NULL) {
			for (cpp = dnptrs; *cpp != NULL; cpp++)
				;
			lpp = cpp;	/* end of list to search */
		}
	} else
		msg = NULL;
	for (c = *dn++; c != '\0'; ) {
		/* look to see if we can use pointers */
		if (msg != NULL) {
			if ((l = dn_find(dn-1, msg, dnptrs, lpp)) >= 0) {
				if (cp+1 >= eob)
					return (-1);
				*cp++ = (l >> 8) | INDIR_MASK;
				*cp++ = l % 256;
				return (cp - comp_dn);
			}
			/* not found, save it */
			if (lastdnptr != NULL && cpp < lastdnptr-1) {
				*cpp++ = cp;
				*cpp = NULL;
			}
		}
		sp = cp++;	/* save ptr to length byte */
		do {
			if (c == '.') {
				c = *dn++;
				break;
			}
			if (c == '\\') {
				if ((c = *dn++) == '\0')
					break;
			}
			if (cp >= eob) {
				if (msg != NULL)
					*lpp = NULL;
				return (-1);
			}
			*cp++ = c;
		} while ((c = *dn++) != '\0');
		/* catch trailing '.'s but not '..' */
		if ((l = cp - sp - 1) == 0 && c == '\0') {
			cp--;
			break;
		}
		if (l <= 0 || l > MAXLABEL) {
			if (msg != NULL)
				*lpp = NULL;
			return (-1);
		}
		*sp = l;
	}
	if (cp >= eob) {
		if (msg != NULL)
			*lpp = NULL;
		return (-1);
	}
	*cp++ = '\0';
	return (cp - comp_dn);
}

/*
 * Skip over a compressed domain name. Return the size or -1.
 */
static int __dn_skipname(const unsigned char *comp_dn, const unsigned char *eom)
{
	register unsigned char *cp;
	register int n;

	cp = (unsigned char *)comp_dn;
	while (cp < eom && ((n = *cp++) != 0)) {
		/*
		 * check for indirection
		 */
		switch (n & INDIR_MASK) {
		case 0:		/* normal case, n == len */
			cp += n;
			continue;
		default:	/* illegal type */
			return (-1);
		case INDIR_MASK:	/* indirection */
			cp++;
		}
		break;
	}
	return ((uchar*)cp - (uchar*)comp_dn);
}

/*
 * Search for expanded name from a list of previously compressed names.
 * Return the offset from msg if found or -1.
 * dnptrs is the pointer to the first name on the list,
 * not the pointer to the start of the message.
 */
static int dn_find(unsigned char *exp_dn, unsigned char *msg, unsigned char **dnptrs, unsigned char **lastdnptr)
{
	register unsigned char *dn, *cp, **cpp;
	register int n;
	unsigned char *sp;

	for (cpp = dnptrs; cpp < lastdnptr; cpp++) {
		dn = exp_dn;
		sp = cp = *cpp;
		while ((n = *cp++) != 0)  {
			/*
			 * check for indirection
			 */
			switch (n & INDIR_MASK) {
			case 0:		/* normal case, n == len */
				while (--n >= 0) {
					if (*dn == '.')
						goto next;
					if (*dn == '\\')
						dn++;
					if (*dn++ != *cp++)
						goto next;
				}
				if ((n = *dn++) == '\0' && *cp == '\0')
					return (sp - msg);
				if (n == '.')
					continue;
				goto next;

			default:	/* illegal type */
				return (-1);

			case INDIR_MASK:	/* indirection */
				cp = msg + (((n & 0x3f) << 8) | *cp);
			}
		}
		if (*dn == '\0')
			return (sp - msg);
	next:	;
	}
	return (-1);
}

/*
 * Routines to insert/extract short/long's. Must account for byte
 * order and non-alignment problems. This code at least has the
 * advantage of being portable.
 *
 * used by sendmail.
 */

unsigned short _getshort(unsigned char *msgp)
{
	unsigned short u;

	GETSHORT(u, msgp);
	return (u);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
unsigned int _getlong(unsigned char *msgp)
{
	unsigned short u;

	GETLONG(u, msgp);
	return (u);
}
#endif

static void __putshort(unsigned short s, unsigned char *msgp)
{
	PUTSHORT(s, msgp);
}

/* static void __putlong(unsigned long l,  unsigned char *msgp)
{
	PUTLONG(l, msgp);
} */
