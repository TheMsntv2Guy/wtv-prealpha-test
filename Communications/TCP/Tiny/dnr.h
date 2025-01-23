/* dnr.h */

/* DNR Support */

#ifndef __DNR_H__
#define __DNR_H__

#define DNR_NUM_ALT_ADDRS 1

enum DNRState {	
	kDNRIdle,
	kDNRResQueryDomain
};

enum DNRConnectState {
	kDNRNoConnect,
	kDNRConnectTry,
	kDNRConnectRequest1,
	kDNRConnectRequest2,
	kDNRConnectResponse,
	kDNRConnectClosing,
	kDNRConnectComplete
};

 
enum {
	kDNRSuccess,
	kDNRCacheFault,
	kDNRBusy,
	kDNRHostNotFound,
	kDNROtherError
};

struct DNRhostinfo {
	long							rtnCode;
	char							cname[101];
	unsigned short					filler;		/* Filler for proper byte alignment	 */
	unsigned long					addr[DNR_NUM_ALT_ADDRS];
};

typedef DNRhostinfo DNRhostinfo;

typedef void (*DNRResultProcPtr)(DNRhostinfo *hostInfoPtr, char *userDataPtr);

extern Error ResolverInit(void);
extern Error ResolverClose(void);
//extern Error HostByName(char *hostName, DNRhostinfo *hostInfoPtr, DNRResultProcPtr ResultProc, char *userDataPtr);
extern Error HostByName(char *hostName, DNRhostinfo *hostInfoPtr, DNRResultProcPtr ResultProc, char *userDataPtr);

extern void DNRHandlerLoop (void);


#endif /* __DNR_H__ */
