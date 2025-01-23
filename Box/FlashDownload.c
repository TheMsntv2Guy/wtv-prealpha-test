#include "WTVTypes.h"
#include "ErrorNumbers.h"
#include "boxansi.h"
#include "chario.h"

#include "HWRegs.h"

#include "Exceptions.h"
#include "Interrupts.h"
#include "HWDisplay.h"
#include "HWModem.h"
#include "IR.h"
#include "BoxUtils.h"
#include "Checksum.h"

#include "BoxHWEquates.h"
#include "CrashLogC.h"

#include "BoxBoot.h"

#include "BoxDebugger.h"

#include "Debug.h"
#include "ObjectStore.h"
#include "MemoryManager.h"
#include "Utilities.h"
#include "Input.h"
#include "BoxAbsoluteGlobals.h"

#include "TellyIO.h"
#include "ppp.h"
#include "bip.h"

#include "Flash.h"

#include "Compress.h"
#include "FlashDownload.h"

#include "FlashStorage.h"

#ifdef DEBUG
#include "HWKeyboard.h"
#endif

#define IP(a,b,c,d) (((ulong)(a) << 24) | ((ulong)(b) << 16) | ((ulong)(c) << 8) | ((ulong)d))


/* PROTOS */

static void InitBasicSystemServices(void);

static void CheckForPowerOff(void);
static void FixFlagsAndReboot(ulong otherflags);

static void CallAndConnectStateMachine(void);
static void FlashDownloadStateMachine(void);
static ulong BlockDownloadStateMachine(void);

static void GetServerInfo(ulong reason);

static ulong FlashNeedsUpdate(void);

static void UpdateFlashStatus(ulong state, ulong p1, ulong p2);
static void TellUser(const char *s);

static void UpdateProgress(ulong togo);
static void InitProgress(ulong togo);



static const char *gCmdString = "GET ";
static const char *gSerialNumPrefix = "wtv-client-serial-number: ";

#if 0
/* 	********************
	Test Crap, Don't Use
	******************** */

/* Small Uncompressed */
static const char *gDefaultPath = "wtv-flashrom:/content/artemis-prototype-02/version-00/part000.rom";

/* Small Compressed */
static const char *gDefaultPath = "wtv-flashrom:/content/artemis-prototype-03/version-00/part000.rom";

/* Uncompressed */
static const char *gDefaultPath = "wtv-flashrom:/content/artemis-prototype-01/version-00/part000.rom";
#endif



/* 	********************
	      Defaults
	******************** */

#ifdef EXTERNAL_BUILD

/* Compressed */
static const char *gDefaultPath = "wtv-flashrom:/current";

#define	kDefaultServerIP		(long)IP(10,0,0,1)			/* Tweety */
#define	kDefaultServerPort		1618							


#else
	 
/* Compressed */
static const char *gDefaultPath = "wtv-flashrom:/current";

#define	kDefaultServerIP		(long)IP(10,0,0,2)		/* Testament */
#define	kDefaultServerPort		1618							

#endif



/* 	********************
	      Globals
	******************** */
	 
long gServerIP;					/* may come from NV storage for server-initiated flash */
short gServerPort;				/* or use defaults for user-initiated flash.		   */

uchar *gCompressedBlock;
uchar *gDecompressedBlock;

ulong *gFlashStartAddr;

Boolean gThatWasTheLastBlock;

char gBlockReqStr[kMaxGetStringLen];
char gNextBlockReqStr[kMaxGetStringLen];
char gSerialNumberStr[kMaxSerialNumStringLen];


/*
	Boot code calls this.  It returns right away if everything is cool, otherwise it handles
	setting up basic system services, calling Artemis, connecting to the Flash server, 
	getting the image and flashing it block by block.  Oh, and informing the user of the status 
	of the whole mess.

*/

void DealWithFlashDownload(void)
{
ulong reason;

	reason = FlashNeedsUpdate();
	
	if (reason)
	{
		Message(("\n>> Flash needs updating! <<\n"));

		SetBoxLEDs(kBoxLEDPower);							/* power led ON */
				
		InitBasicSystemServices();			/* Set up ints we need, turn on display, init filesys */
	
		gCompressedBlock = (uchar *)AllocateMemory(131072 + sizeof(FlashBlockHeader));
		gDecompressedBlock = (uchar *)AllocateMemory(131072);		/* These BETTER NOT fail! */
		Message(("gCompressedBlock = %08lx, gDecompressedBlock = %08lx", 
											(ulong)gCompressedBlock, (ulong)gDecompressedBlock));
	
		if(NVGetFlags() & kFlashDLFailed)					/* did a previous flash dl fail? */
			UpdateFlashStatus(kPreviousFlashFailed, 0, 0);	
		
		UpdateFlashStatus(reason, 0, 0);					/* Let the user know why we're reflashing */
		
		GetServerInfo(reason);								/* fill in flash server globals */

		bipInit();											/* init tcp */

		CallAndConnectStateMachine();						/* hook us up, handles errs internally */

		SetBoxLEDs(kBoxLEDPower | kBoxLEDConnect);			/* now we're connected */

		FlashDownloadStateMachine();						/* do the work, handles errs internally */
		
		FixFlagsAndReboot(0);								/* we got it, let's go use it */
	}
	else
	{
		Message(("Flash is OK."));
		return;								/* Yay, we're OK */
	}
}



void FlashDownloadIdle(void)
{
 	CheckForPowerOff();
}





/*	Handles errors internally.
	Some errors clear dl flags and reboot, some retry whatever failed a few times.
*/

void CallAndConnectStateMachine(void)
{
TellyResult tellyResult;
Error biperr;
FSNode* node;
	
	UpdateFlashStatus(kCalling800Number, 0, 0);
			
	if ((node = Resolve("/ROM/Text/artemis_18006138199.tok", false)) == nil) 
	{
		Message(("CallAndConnectStateMachine: cannot find tellyscript"));
		UpdateFlashStatus(kTellyScriptError, kTellyParseError, 0);
		FixFlagsAndReboot(kFlashDLFailed);
		return;
	}

	/* make the phone be in a good state.  later gets overrided by users restore NVRAM settings */
	{
		PHONE_SETTINGS	phone;
		
		phone.usePulseDialing = false;
		phone.audibleDialing = true;
		phone.disableCallWaiting = false;
		phone.dialOutsideLine = false;
		phone.changedCity = false;
		phone.waitForTone = true;
		phone.callWaitingPrefix[0] = 0;
		phone.dialOutsidePrefix[0] = 0;
		phone.accessNumber[0] = 0;
	
		SetPhoneSettings(&phone);
	}
	
	tellyResult = RunScript(node->data, node->dataLength);		/* call server */
	
	/* ### FIXME - we need to dl the local pop script & call it! */
	
	printf("tellyResult = %d\n",tellyResult);
	
	if (tellyResult == kTellyLinkConnected)
	{		
		UpdateFlashStatus(kConnectingToFlashServer, 0, 0);
		
		biperr = bipConnect(gServerIP, gServerPort);		
		Message(("bipConnect err = %d",biperr));				

		if(biperr == kNoError)
			return;
		else
		{
			UpdateFlashStatus(kFlashServerNotResponding, 0, 0);			/* where the hell is fadden? */			
			FixFlagsAndReboot(kFlashDLFailed);
		}
	}
	else
	{
		UpdateFlashStatus(kTellyScriptError, tellyResult, 0);
		FixFlagsAndReboot(kFlashDLFailed);
	}
}







/* 	Handles errors internally. 
	All errors clear dl flags and reboot.
*/

void FlashDownloadStateMachine(void)
{
ulong dlstate = kFetchBlock;
ulong fetchErrCnt = 0;
ulong flashErrCnt = 0;
ulong err;
		
	gThatWasTheLastBlock = false;				/* Block downloader sets when we get the last block */
	
	while(1)
	{
		TCPIdle(true);
		CheckForPowerOff();
		
		switch(dlstate)
		{
			case kFetchBlock:
					if(BlockDownloadStateMachine() == kNoError)
					{	
						Message(("going to decompress"));
						fetchErrCnt = 0;
						dlstate = kDecompressBlock;
					}
					else
						dlstate = kFetchError;
					break;
					
			case kDecompressBlock:
					Message(("flashAddress = %08x",((FlashBlockHeader *)gCompressedBlock)->flashAddress));
					Message(("blockNum = %08x",((FlashBlockHeader *)gCompressedBlock)->blockNum));
					
					if( (((FlashBlockHeader *)gCompressedBlock)->compressHeader).type != kCompressNone )
					{
						UpdateFlashStatus(kDecompressing, 0, 0);

						ExpandLzss( (gCompressedBlock + sizeof(FlashBlockHeader)), gDecompressedBlock, 
									(((FlashBlockHeader *)gCompressedBlock)->compressHeader).compressedSize, 
									(((FlashBlockHeader *)gCompressedBlock)->compressHeader).expandedSize );									
					}
					else
						memcpy( gDecompressedBlock ,(gCompressedBlock + sizeof(FlashBlockHeader)),
											(((FlashBlockHeader *)gCompressedBlock)->compressHeader).expandedSize );
					
					dlstate = kFlashBlock;
					break;
					
			case kFlashBlock:
					UpdateFlashStatus(kFlashingBlock, 0, 0);

					Message(("Flashing block..."));
								
					Message(("magic # = %lx", ((FlashBlockHeader *)gCompressedBlock)->magic));
					Message(("flashAddress = %lx", ((FlashBlockHeader *)gCompressedBlock)->flashAddress));
					Message(("blockNum = %x", ((FlashBlockHeader *)gCompressedBlock)->blockNum));

					err = EraseAndFlashBlock( (ulong *)gDecompressedBlock, 
											  (ulong *)(((FlashBlockHeader *)gCompressedBlock)->flashAddress) );

					Message(("Block Flashed to addr %08lx, err = %08lx", 
										(((FlashBlockHeader *)gCompressedBlock)->flashAddress), err));

					if(err)
						dlstate = kFlashError;
					else
					{				
						if(gThatWasTheLastBlock)
							dlstate = kSuccess;
						else
							dlstate = kFetchBlock;
					}
					break;
			
			case kFetchError:
					UpdateFlashStatus(kErrorFetching, 0, 0);
					fetchErrCnt++;
					if(fetchErrCnt == kMaxBlockFetchErrs)
					{
						UpdateFlashStatus(kTooManyFetchErrors, 0, 0);
						FixFlagsAndReboot(0);
					}
					else
					{
						UpdateFlashStatus(kRetrying, 0, 0);
						dlstate = kFetchBlock;		/* try again */
					}
					break;
					
			case kFlashError:
					UpdateFlashStatus(kErrorFlashing, 0, 0);
					flashErrCnt++;
					if(flashErrCnt == kMaxBlockFlashErrs)
					{
						UpdateFlashStatus(kTooManyFlashErrors, 0, 0);
						FixFlagsAndReboot(0);
					}
					else
					{
						UpdateFlashStatus(kRetrying, 0, 0);
						dlstate = kFlashBlock;		/* try again */
					}
					break;
					
			case kSuccess:
					Message(("Hurray, Flash successfully updated!"));
					return;
					break;
		}
	}	
}







/*
	1. Ask for the block described in gBlockReqStr.
	2. Handhold bip to get the response header for the request.
	3. Parse the response.  Extract the wtv-visit field into temp local array.
	                        Extract length from Content-Length field of response.
	4. Handhold bip to receive "length" bytes into gCompressedBlock buffer.
	5. If receive is successful:  Copy temp wtv-visit string into gBlockReqStr.
								  Decompress block from gCompressedBlock into gDecompressedBlock.
	6. If gBlockReqStr[0] == 0 (no wtv-visit field), set gThatWasTheLastBlock.
*/

ulong BlockDownloadStateMachine(void)
{
ulong blkstate = kAskForBlock;
long jjj,bytesRemaining,bipcnt;
char response[(kBufferSize>>1)];		/* grab up to half a buffer (should be enough to get response text) */
ulong xxx,CRCount;
char *nextBlkStr;
char *lenStr;
ulong len = 0;
long bytesReceived;
char *data = nil;
ulong timeout;

	CRCount = 0;
	timeout = 0;
	
	jjj= 0;
	bytesRemaining = strlen(gBlockReqStr);

	Message(("req: %s",gBlockReqStr));

	while(1)
	{
		TCPIdle(true);
		CheckForPowerOff();
		
		switch(blkstate)
		{
			case kAskForBlock:
					if(bipWrite(&gBlockReqStr[jjj], bytesRemaining, &bipcnt) == kNoError)
					{
						if(bipcnt != bytesRemaining)
						{
							jjj += bipcnt;
							bytesRemaining -= bipcnt;
						}
						else
						{
							/* Message(("Sent block request: %s", gBlockReqStr)); */
							
							jjj = 0;
							bytesRemaining = (kBufferSize>>1);
							blkstate = kGetResponse;
						}

						if(bipcnt)
							timeout = 0;
						else
							if(!timeout)
								timeout = Now() + (kGetDataTimeout * 60);
								
						if(timeout)
							if(Now() > timeout)
								blkstate = kDownloadFailed;
					}
					else
					{
						Message(("Failed asking for block %s", gBlockReqStr));
						blkstate = kDownloadFailed;
					}
					break;
					
			case kGetResponse:
					bipRead(&response[jjj], bytesRemaining, &bipcnt);

					jjj += bipcnt;
					bytesRemaining -= bipcnt;

					for(xxx=0;xxx!=jjj;xxx++)
					{
						if(response[xxx] == '\n')
							CRCount++;
						else
							CRCount = 0;
							
						if(CRCount == 2)
						{
							blkstate = kParseResponse;
							continue;
						}
					}

					if(bipcnt)
						timeout = 0;
					else
						if(!timeout)
							timeout = Now() + (kGetDataTimeout * 60);
							
					if(timeout)
						if(Now() > timeout)
							blkstate = kDownloadFailed;
					break;
					
			/*
				Response looks like:
					200 OK
					wtv-visit: <next block request string>		<-- not present for last block!
					Content-type: binary/flashblock
					Content-Length: <length in bytes>
					\n
			*/
			case kParseResponse:		/* entry: jjj = # chars rec'd, response[] holds response strings */
						
					if( !strstr(response, "200 OK\n") )
					{
						Message(("Didn't get 200 OK!"));
						blkstate = kDownloadFailed;
						break;
					}
										
					lenStr = strstr(response, "Content-Length: ");	/* better have this */
					lenStr += 16;									/* skip past "Content-Length: " */

					data = strstr(response, "\n\n");				/* gotta have this */
					data += 2;										/* skip past cr's to first byte of binary data */
					
					nextBlkStr = strstr(response, "wtv-visit: ");	/* won't have this for last blk */
					if(nextBlkStr)
					{
						nextBlkStr += 11;							/* skip past "wtv-visit: " */														
						*strstr(nextBlkStr, "\n") = 0;				/* terminate nextBlkStr */
						strcpy(gNextBlockReqStr, nextBlkStr);		/* we'll be trashing response buffer */
					}
					else
						gNextBlockReqStr[0] = 0;					/* no next, we're done */
					
					*strstr(lenStr, "\n") = 0;						/* terminate lenStr */					
										
					/* Message(("next: %s",gNextBlockReqStr)); */
					
					len = strtoul(lenStr, 0, 10);
					Message(("%ld bytes to recv",len));
					
					bytesReceived = (((ulong)response+jjj) - (ulong)data);
					jjj = 0;
					
					InitProgress(len - bytesReceived);
					
					blkstate = kGetCompressedData;	
					timeout = 0;
					break;
					
			case kGetCompressedData:
			
					len -= bytesReceived;							/* go until we get them all */
					
					if(bytesReceived)
						UpdateProgress(len);
										
					while(bytesReceived--)							/* copy from rcv buf to decomp buf */
					{
						*(gCompressedBlock + jjj) = *data;
						jjj++;
						data++;
					}
										
					if(len <= 0)
						blkstate = kDownloadSuccessful;
					else	
					{
						bipRead(response, (kBufferSize>>1), &bytesReceived);
						data = response;
					}
					
					if(bytesReceived)
						timeout = 0;
					else
						if(!timeout)
							timeout = Now() + (kGetDataTimeout * 60);
							
					if(timeout)
						if(Now() > timeout)
							blkstate = kDownloadFailed;
					break;
					
			case kDownloadFailed:
					return kGenericError;
					break;
			
			case kDownloadSuccessful:
					Message(("Received complete block!"));

					UpdateFlashStatus(kReceivedBlock, ((FlashBlockHeader *)gCompressedBlock)->blockNum, 0);

					if(gNextBlockReqStr[0])
					{
						strcpy((gBlockReqStr+4), gNextBlockReqStr);	/* copy past GET */
						strcat(gBlockReqStr, "\n");
						strcat(gBlockReqStr, gSerialNumberStr);		/* glue on serial num w/2 CR's */
					}
					else
						gThatWasTheLastBlock = true;				/* we're finished */
					
					return kNoError;
					break;
		}
	}
}





/*	*********
	Utilities
	********* */

/*
	Need to update if:
	
			1) Current app rom is hosed.
			2) Server told us to update (flag in NV storage, verify NV storage first).
			3) User requested update (secret key before power key sets NV flag).
			4) Earlier download was aborted.
*/

ulong FlashNeedsUpdate(void)
{
	PostulateFinal(false);		/* FIXME - support bad app rom FS */

	if( !ROMCodeChecksumOK(0x9FE00000, 0x00100000) )		
		return kUpdatingTrashedFlash;

	if( !ROMFSChecksumOK(0x9FC00000, 0x00200000) )		
		return kUpdatingTrashedFlash;

	if( NVGetFlags() & kUserReqFlashUpdate )
		return kUpdatingFlashForUser;
		
	if( NVGetFlags() & kServerReqFlashUpdate )
		return kUpdatingFlashForServer;
		
	return 0;
}





/*	In the case of a Server-initiated flash update, if any one piece of the info the server should
	have sent is missing, the download is aborted.
	
	The user is informed of this, and the box reboots.
*/

void GetServerInfo(ulong reason)
{
long *ipData;
short *portData;
char *pathData;
long len;

	gServerIP = kDefaultServerIP;					/* assume non-server req update */
	gServerPort = kDefaultServerPort;
	strcpy(gBlockReqStr, gCmdString);
	strcat(gBlockReqStr, gDefaultPath);
	strcat(gBlockReqStr, "\n");

	strcpy(gSerialNumberStr, gSerialNumPrefix);		/* set up the serial number */
	snprintf(gSerialNumberStr + strlen(gSerialNumPrefix), 17,
				"%08x%08x", READ_AG(agSerialNumberHi), READ_AG(agSerialNumberLo));
	strcat(gSerialNumberStr, "\n\n");
	
	NVInit(kNVPhaseRestore);				/* boot code will have checked sanity */
											/*  (and reset, if needed)            */
	switch(reason)
	{
		case kUpdatingFlashForServer:
				ipData = (long *)NVRead(kFlashIPTag, &len);
				portData = (short *)NVRead(kFlashPortTag, &len);
				pathData = (char *)NVRead(kFlashPathTag, &len);
				
				if(!ipData)
					Message(("missing ip addr!"));
					
				if(!portData)
					Message(("missing port!"));
				
				if(!pathData)
					Message(("missing path!"));
				
				
				if( ipData && portData && pathData )
				{
					Message(("Got valid data from NV storage"));
					
					gServerIP = *ipData;
					gServerPort = *portData;
					strcpy(gBlockReqStr, gCmdString);
					strcat(gBlockReqStr, pathData);
					strcat(gBlockReqStr, "\n\n");

					Message(("ip = %lx, port = %hd", gServerIP, gServerPort));
				}
				else
				{
					NVSetFlags( NVGetFlags() & ~(kUserReqFlashUpdate | kServerReqFlashUpdate) );		
					UpdateFlashStatus(kBadServerInfo, 0, 0);
					while(1);
				}
				break;
				
		case kUpdatingTrashedFlash:
		case kUpdatingFlashForUser:
		default:
				break;
	}

	strcat(gBlockReqStr, gSerialNumberStr);			/* glue get & serial num together together */
}





/* 	After this, we'll only come back to the flash code if the App ROM's 
	checksum is hosed. 
	
	This takes care of turning off any LEDs we left on.
*/

void FixFlagsAndReboot(ulong otherflags)
{
	ModemSetDTR(false);					/* drop the phone line */
	
	NVSetFlags( (NVGetFlags() & ~(kUserReqFlashUpdate | kServerReqFlashUpdate)) | otherflags );		

	if(otherflags | kFlashDLFailed)		/* for failures, delay 10sec */
	{
	ulong target = Now() + (kErrMsgRebootDelay*60);	
	
		while(Now() < target)
			CheckForPowerOff();
	}

	Reboot(kColdBoot);					/* here we go... */
}





/*	Don't really want to power off.  If we see the power key, drop the phone line and fall into
 	a loop waiting for another power key.
	
	When we see a second power key, Reboot.  The DL flags will still be as they were, and a pending
	DL interrupted by the user will be restarted.
*/

void CheckForPowerOff(void)
{
Input nextInput;

	IRIdle();
	NextInput(&nextInput);

	if(nextInput.data == kPowerKey)
	{
		SetScriptAborted();					/* stop telly */
		ModemSetDTR(false);					/* drop the phone line */
		KillDisplay();						/* screen off */
		SetBoxLEDs(0);						/* power led off */
		
#if 0				
		FlushInput();

		while(1)
		{
			IRIdle();
			NextInput(&nextInput);
			if(nextInput.data == kPowerKey)
			{
				Message(("got it!"));
				FixFlagsAndReboot(kPowerOnReboot);
			}
		}
#else
		FixFlagsAndReboot(kPowerOnReboot);
#endif

	}
}




/*	Crank up interrupts, turn on the display, init the FS.
*/

void InitBasicSystemServices(void)
{
	Message(("Initializing basic system services..."));

	Message(("Initializing interrupts..."));

	move_exc_code();
		
	SystemIntHook = (ulong)InterruptDecode;			/* install main int hook */	
		
	DisableAllInts();
	ClearAllInts();
	
	SetDisplayInterrupts(kVidHSyncIntMask);		/* enable hline ints at vid unit */

#ifdef DEBUG
	EnableInts(kVideoIntMask + kKeyboardIntMask + kIRIntMask);
#else
	EnableInts(kVideoIntMask + kIRIntMask);			
#endif

#ifdef SPOT3_REFRESH_HACK
	SetSR(0x10010000);		/* turn off BEV, counter interrupts */
#endif

	enable_cpu_ints(kCPUIntMask+kGooberIntMask);	/* enable ints to the cpu */

#ifdef DEBUG
	InitHWKeyboard();					/* unlike other drivers, call kb init AFTER ints are enabled */
	DisableInts(kKeyboardIntMask);
#endif

	/* InitializeMemoryManagement(); */		/* BoxBoot already did this */
	InitializeFilesystems();

	EnableDisplay();
}







/*	**************
	User Interface 	
	************** */

void UpdateFlashStatus(ulong state, ulong p1, ulong p2)
{
	switch(state)
	{
		/* --- Initial messages to user --- */
		
		case kPreviousFlashFailed:
				TellUser(">> A download started earlier was aborted.  Retrying. <<\n\n");
				break;
				
		case kUpdatingTrashedFlash:
				TellUser("The software in your WebTV box has been corrupted.\n");
				TellUser("Now downloading a fresh copy.  Please don't interrupt.\n\n");
				break;
				
		case kUpdatingFlashForUser:
				TellUser("As you requested, the software in your WebTV box will now be updated.\n");
				TellUser("This may take a while, please don't interrupt!\n\n");
				break;
				
		case kUpdatingFlashForServer:
				TellUser("You need a new version of the WebTV software.\n");
				TellUser("It will now be downloaded to your box automatically.\n");
				TellUser("This may take a while, please don't interrupt!\n\n");
				break;
		
		/* --- Status --- */
				
		case kCalling800Number:
				TellUser("Calling the WebTV Network 800 Number...\n");
				break;

		case kGettingLocalNumber:
				TellUser("Retrieving your Local WebTV Network Number...\n");
				break;
				
		case kCallingLocalNumber:
				TellUser("Calling your Local WebTV Network Number...\n");
				break;
				
		case kConnectingToFlashServer:
				TellUser("Connecting to the Software Update Server...\n");
				break;
				
		case kReceivedBlock:
				gPutCharTV = true;
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");	/* back over progress */
				printf("Received block %ld.  ", p1);
				gPutCharTV = false;
				break;
				
		case kDecompressing:
				TellUser("Decompressing...  ");		
				break;
				
		case kFlashingBlock:
				TellUser("Flashing block...\n");		
				break;
				
		/* --- Errors --- */
		
		/* Tellyscript */
		
		case kTellyScriptError:
				switch(p1)
				{
					case kTellyNoAnswer:
							TellUser("\n");
							TellUser("The WebTV Network is not answering.\n");
							TellUser("I will try calling again in 1 minute.\n\n");
							break;
							
					case kTellyNoDialtone:
							TellUser("\n");
							TellUser("I can't hear a dialtone.  Make sure a phone line is connected\n");
							TellUser("to your WebTV box.\n");
							TellUser("I will try calling again in 1 minute.\n\n");
							break;
					
					case kTellyBusy:
							TellUser("\n");
							TellUser("The WebTV Network phone line is busy.\n");
							TellUser("I will try calling again in 1 minute.\n\n");
							break;
							
					default:
							TellUser("\n");
							TellUser("A Tellyscript error that I cannot handle has occurred.\n");
							TellUser("I can't do anything else, so I'll reboot your box.\n");
							break;
				}
		
		/* TCP */
		
		case kFlashServerNotResponding:
				TellUser("\n");
				TellUser("The Software Update Server is not answering.\n");
				TellUser("Please try again in a few minutes.\n\n");
				break;
				
		/* Other */
		
		case kErrorFetching:
				TellUser(("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"));	/* back over progress */
				TellUser((">>> Error receiving block!"));
				break;
				
		case kErrorFlashing:
				TellUser(("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"));	/* back over progress */
				TellUser((">>> Error flashing block!"));
				break;
				
		case kRetrying:
				TellUser(("  Retrying...\n"));
				break;

		case kDeepShit:
				TellUser("\n");
				TellUser("Uh-oh, your Flash ROM is hosed!\n");
				TellUser("I can't erase/write your Flash ROM.\n\n");
				TellUser("Call customer service, please!\n\n");
				TellUser("You will not be able to use your WebTV box until this problem is\n");
				TellUser("resolved.\n");
				break;
				
		/* --- Misc --- */
		
		case kYouCanStillUseYourBox:
				TellUser("You can still use your WebTV box with the old software.\n");
				break;
				
		case kYourBoxIsFucked:
				TellUser("You cannot use your WebTV box until the download is complete.\n");
				break;
				
		case kFlashDownloadFailed:
				TellUser("The download *FAILED*.  Reset the box manually.\n");
				TellUser("Checksum-related downloads will retry automatically.\n");
				TellUser("User and Server initiated downloads must be re-requested.\n");
				break;
				
		case kBadServerInfo:
				TellUser("Some data needed for a Server-initiated download is missing.\n");
				TellUser("Leaving your ROM untouched.  Please reset the box manually.\n");
				break;
	}
}



static void TellUser(const char *s)
{
	gPutCharTV = true;
	printf("%s",s);
	gPutCharTV = false;
}


static const char spin[4] = { '|', '/', '-', 0x5c };
uchar index = 0;

void UpdateProgress(ulong togo)
{
	gPutCharTV = true;
	printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	printf("%6ld bytes to go...%c",togo,spin[index]);
	index++;  index &= 3;
	gPutCharTV = false;
}

void InitProgress(ulong togo)
{
	gPutCharTV = true;
	printf("\n%6ld bytes to go...%c",togo,spin[index]);
	index++;  index &= 3;
	gPutCharTV = false;
}

