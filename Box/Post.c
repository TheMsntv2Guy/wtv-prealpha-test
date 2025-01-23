/* CheckSum Code in Box:Checksum.c */
/* Flash down load in Box:BoxDebugger.c */
/* Implement RAM tests in Box:crt0.s */

/**************************************/
/**** Includes                    *****/
/**************************************/
#include "HWRegs.h"
#include "BoxHWEquates.h"
#include "BoxAbsoluteGlobals.h"

#include "BoxUtils.h"
#include "Checksum.h"
#include "WTVTypes.h"
#include "Interrupts.h"
#include "HWKeyboard.h"

#include "ErrorNumbers.h"
#include "Debug.h"
#include "SystemGlobals.h"
#include "Interrupts.h"


#include "HWModem.h"
#include "HWAudio.h"
#include "Serial.h"

#include "Post.h"
#include "Debug.h"


/**************************************/
/**** Includes                    *****/
/**************************************/
#define DEBUG 1

#define	kKBCtlTimeout	312000		/* ¥¥¥ÊFIXME: use the microsec timer for the timeout */
#define	kKBErr			0xff

static ulong gReadDataTimeout = kKBCtlTimeout;
static ulong gWriteDataTimeout = kKBCtlTimeout;
static ulong gWriteCmdTimeout = kKBCtlTimeout;

/**************************************/
/**** Routines                    *****/
/**************************************/


Boolean TestSPOTReg (short* count, ulong SPOTReg, ulong mask, ulong testval)
{
	ulong   data;
	ulong*	addr;
	


	Message(("GetSpotRegister: Begin"));	
	
	addr = (ulong*)(SPOTReg);
	data = *addr & mask;
	
	Message(("GetSpotRegister: addr is: %x",addr));	
	Message(("GetSpotRegister: data is: %x",data));	
	Message(("GetSpotRegister: testval is: %x",testval));	

	if (data!=testval)
	{
		*count++;
		return (false);
	}
	else 
		return (true);
}
/***************************************************************************/


Boolean SPOTRegResetTest (void)
{
	short* count=0;



	Message(("TestSPOTReset:Start"));

	TestSPOTReg(count, (kBusIntEnableSet + kSPOTBase), 0xfffffe00, 0);      /* BUS_INTEN 31:8 to zero. */
	TestSPOTReg(count, (kBusChipCntl + kSPOTBase), 0x3C000000, 0);	  	  /* BUS_CHPCNTL 29:26 AUDCLKDIV[3:0] is 0 */
	TestSPOTReg(count, (kBusChipCntl + kSPOTBase), 0x0000ffff, 0x0000ffff);	  /* BUS_CHPCNTL 15:0 TOCOUNT[15:0] is 0xFFFF */
	TestSPOTReg(count, (kBusIntEnableSet + kSPOTBase), 0x000000fc, 0);	  /* BUS_INTEN 7:2 is 0 */
	TestSPOTReg(count, (kBusErrStatus + kSPOTBase), 0x0000007c, 0);	  	  /* BUS_ERRSTAT 6:2 is 0 */

if (*count)
	Message(("FAILED TestSPOTRegisters after first!!"));

	TestSPOTReg(count, (kROMCntl0 + kSPOTBase), 0x80000000, 0 );	  		  /* ROM_CNTL0 31 is 0 */
	TestSPOTReg(count, (kROMCntl0 + kSPOTBase), 0x1f000000, 0x1f000000);	  /* ROM_CNTL0 28:24 is 0b11111 */
	TestSPOTReg(count, (kROMCntl0 + kSPOTBase), 0x001f0000, 0x001f0000);    /* ROM_CNTL0 20:16 is 0b11111 */
	TestSPOTReg(count, (kROMCntl0 + kSPOTBase), 0x00007000, 0x00007000);    /* ROM_CNTL0 14:12 is 0b111 */
	TestSPOTReg(count, (kROMCntl0 + kSPOTBase), 0x00000200, 0x00000200);    /* ROM_CNTL0 9:8 is 0b11 */
	TestSPOTReg(count, (kROMCntl0 + kSPOTBase), 0x0000000f, 0x0000000f);    /* ROM_CNTL0 3:0 is 0b1111 */

if (*count)
	Message(("FAILED TestSPOTRegisters after second set!!"));
		
	TestSPOTReg(count, (kROMCntl1 + kSPOTBase), 0x80000000, 0 );    		  /* ROM_CNTL1 31 is 0 */
	TestSPOTReg(count, (kROMCntl1 + kSPOTBase), 0x1f000000, 0x1f000000);    /* ROM_CNTL1 28:24 is 0b11111 */
	TestSPOTReg(count, (kROMCntl1 + kSPOTBase), 0x001f0000, 0x001f0000);    /* ROM_CNTL1 20:16 is 0b11111 */
	TestSPOTReg(count, (kROMCntl1 + kSPOTBase), 0x00007000, 0x00007000);    /* ROM_CNTL1 14:12 is 0b111 */
	TestSPOTReg(count, (kROMCntl1 + kSPOTBase), 0x00000200, 0x00000200);    /* ROM_CNTL1 9:8 is 0b11 */
	TestSPOTReg(count, (kROMCntl1 + kSPOTBase), 0x0000000f, 0x0000000f);    /* ROM_CNTL1 3:0 is 0b1111 */

if (*count)
	Message(("FAILED TestSPOTRegisters after 3 set!!"));
	
	TestSPOTReg(count, (kVidFCntl + kSPOTBase), 0x00000040, 0);    		  /* VID_FCNTL 6 is 0 */
	TestSPOTReg(count, (kVidFCntl + kSPOTBase), 0x00000008, 0);             /* VID_FCNTL 3 is 0 */
	TestSPOTReg(count, (kVidFCntl + kSPOTBase), 0x00000004, 0);    	   	  /* VID_FCNTL 2 is 0 */
	TestSPOTReg(count, (kVidFCntl + kSPOTBase), 0x00000001, 0);   		  /* VID_FCNTL 0 is 0 */

if (*count)
	Message(("FAILED TestSPOTRegisters after 4 set!!"));
	
	
	TestSPOTReg(count, (kVidHIntLine + kSPOTBase), 0x000003ff, 0);  		  /* VID_HINTLINE 9:0 is 0 */
	TestSPOTReg(count, (kVidIntEnableSet + kSPOTBase), 0x0000007c, 0);   	  /* VID_INTEN 6:2 is 0 */
	
	TestSPOTReg(count, (kDevLEDs + kSPOTBase), 0x00000007, 0);   	 	      /* DEV_LED 2:0 is 0 */
	TestSPOTReg(count, (kDevIDChip + kSPOTBase), 0x00000002, 0x00000002);   /* DEV_IDCNTL 1 is 1 */
	
	TestSPOTReg(count, (0x0000400c + kSPOTBase), 0x0000000c, 0);    		  /* DEV_NVCNTL 3:2 is 0 */
	
	TestSPOTReg(count, (kDevSmartCard + kSPOTBase), 0x00000030, 0);  	  	  /* DEV_SCCNTL 5:4 is 0 */
	TestSPOTReg(count, (kDevSmartCard + kSPOTBase), 0x00000002, 0);   	  /* DEV_SCCNTL 1 is 0 */

if (*count)
	Message(("FAILED TestSPOTRegisters after 5 set!!"));
	
	
	TestSPOTReg(count, kDevExtTime, 0xf0000000, 0xf0000000);    			  /* DEV_EXTTIME 31:28 is 0xF */
	TestSPOTReg(count, kDevExtTime, 0x0f000000, 0x08000000);   			  /* DEV_EXTTIME 27:24 is 0x8 */
	TestSPOTReg(count, kDevExtTime, 0x00c00000, 0x00800000);   			  /* DEV_EXTTIME 23:22 is 0x2 */
	TestSPOTReg(count, kDevExtTime, 0x0000f000, 0x0000f000);   			  /* DEV_EXTTIME 15:12 is 0xF */
	TestSPOTReg(count, kDevExtTime, 0x00000f00, 0x00000800);   			  /* DEV_EXTTIME 11:8 is 0x8 */
	TestSPOTReg(count, kDevExtTime, 0x000000c0, 0x00000080);   			  /* DEV_EXTTIME 7:6 is 0x2 */
	

if (*count)
	Message(("FAILED TestSPOTRegisters after 6 set!!"));

	TestSPOTReg(count, (kMemCntl + kSPOTBase), 0xc0000000, 0xc0000000);     /* MEM_CNTL 31:30 is 0b11 */
	
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0xe0000000, 0xa0000000);    /* MEM_TIMING 31:29 is 0x5 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x18000000, 0x00800000);    /* MEM_TIMING 28:27 is 0x1 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x0c000000, 0x08000000);    /* MEM_TIMING 26:25 is 0x2 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x01c00000, 0x01800000);    /* MEM_TIMING 24:22 is 0x6 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x00300000, 0x00300000);    /* MEM_TIMING 21:20 is 0x3 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x000c0000, 0x00080000);    /* MEM_TIMING 19:18 is 0x2 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x00038000, 0x00028000);    /* MEM_TIMING 17:15 is 0x5 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x00006000, 0x00004000);    /* MEM_TIMING 14:13 is 0x2 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x00001800, 0x00001800);    /* MEM_TIMING 12:11 is 0x3 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x00000300, 0x00000300);    /* MEM_TIMING 10:9 is 0x3 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x00000180, 0x00000180);    /* MEM_TIMING 8:7 is 0x3 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x00000070, 0x00000070);    /* MEM_TIMING 6:4 is 0x7 */
	TestSPOTReg(count, (kMemTiming + kSPOTBase), 0x0000000f, 0x0000000a);    /* MEM_TIMING 3:0 is 0xa */


	Message(("TestSPOTReset:End Tests"));

	return (true);
}
/***************************************************************************/

void infiniteLoop (void)
{
	for (;;) ;
}
/***************************************************************************/

static Boolean KBSelfTestOK(void)
{
uchar data;
ulong ints;

	ints = DisableInts( kKeyboardIntMask );
	
	KBWriteCommand( kTestController );
	data = KBReadData();
	
	EnableInts( ints );

	if ( data == 0x55 )
		return true;
	else
		return false;
}

/***************************************************************************/


static void KBWriteCommand(uchar data)
{
ulong timeout = kKBCtlTimeout;

	while (timeout--)
	{
		if (!(*(volatile ulong *)kKybdControlReg & kKStat_InputFull))
		{
			*(volatile ulong *)kKybdControlReg = data;
			goto Done;
		}
	}
	
Done:
	if (timeout < gWriteCmdTimeout)
		gWriteCmdTimeout = timeout;
}

/***************************************************************************/

static uchar KBReadData(void)
{
	ulong timeout = kKBCtlTimeout;
	ulong result = kKBErr;

	while (timeout--)
	{
		if (*(volatile ulong *)kKybdControlReg & kKStat_OutputFull)
		{	
			result = *(volatile ulong *)kKybdDataReg;
			goto Done;
		}
	}
	
Done:
	if (timeout < gReadDataTimeout)	
		gReadDataTimeout = timeout;
	return (uchar)result;	
}


/***************************************************************************/

Boolean MemTest(void)
{
	ulong 	loop;
	ulong 	*RAMaddr;
	ulong 	pattern;
	Boolean status=true;

	Message (("Writting walking fives to Low RAM"));
	
	loop 	= (kCrashLogBase)&(0x0fffffff);
	RAMaddr = (ulong *)0xa0000000;
	pattern = (ulong)0x55555555;
	while (loop)
	{
		*RAMaddr++ = pattern;
		loop -= 4;
	}
	
	Message (("Writting walking fives to Hi RAM"));

	loop = 0x00200000 - ((kCrashLogTop)&(0x0fffffff));
	RAMaddr = (ulong *)kCrashLogTop;
	while (loop--)
	{
		*RAMaddr++ = pattern;
		loop -= 4;
	}	

	Message (("Reading walking fives from Low RAM"));

	loop 	= kCrashLogBase&0x0fffffff;
	RAMaddr = (ulong *)0xa0000000;
	while (loop--)
	{
		if (*RAMaddr++ != pattern)
		{
			Message (("Error reading 5's from addr: %x", RAMaddr));
			status = false;
		}
		loop -= 4;
	}

	Message (("Reading walking fives from Hi RAM"));

	loop = 0x00200000 - (kCrashLogTop&0x0fffffff);
	RAMaddr = (ulong *)kCrashLogTop;
	while (loop--)
	{
		if (*RAMaddr++ != pattern)
		{
			Message (("Error reading 5's from addr: %x", RAMaddr));
			status = false;
		}	
		loop -= 4;
	}
		
	Message (("Writting walking a's to Low RAM"));
	
	loop 	= kCrashLogBase&0x0fffffff;
	RAMaddr = (ulong *)0xa0000000;
	pattern = (ulong)0xaaaaaaaa;
	while (loop--)
	{
		*RAMaddr++ = pattern;
		loop -= 4;
	}
	
	Message (("Writting walking a's to Hi RAM"));

	loop = 0x00200000 - (kCrashLogTop&0x0fffffff);
	RAMaddr = (ulong *)kCrashLogTop;
	while (loop--)
	{
		*RAMaddr++ = pattern;
		loop -= 4;
	}	

	Message (("Reading walking a's from Low RAM"));

	loop 	= kCrashLogBase&0x0fffffff;
	RAMaddr = (ulong *)0xa0000000;
	while (loop--)
	{
		if (*RAMaddr++ != pattern)
		{
			Message (("Error reading a's from addr: %x", RAMaddr));
			status = false;
		}
		loop -= 4;
	}

	Message (("Reading walking a's from Hi RAM"));

	loop = 0x00200000 - (kCrashLogTop&0x0fffffff);
	RAMaddr = (ulong *)kCrashLogTop;
	while (loop--)
	{
		if (*RAMaddr++ != pattern)
		{
			Message (("Error reading a's from addr: %x", RAMaddr));
			status = false;
		}
		loop -= 4;
	}
	return(status);
}




/***************************************************************************/

/***************************************************************************/



/**************************************/
/**** Main Routine                *****/
/**************************************/

void Post(void)
{
	ulong	RomBase = kROMBase;
	ulong	RomSize = kROMSize;
	
Message(("Post:Begin"));

	SetBoxLEDs( kMessageLED );						/* Test 001 */
	
//	SPOTRegResetTest();

	
	SetBoxLEDs( kConnectedLED );					/* Test 010 */

Message(("Post:Before ROMCodeChecksumOK()"));
	
	if (!(ROMCodeChecksumOK(RomBase, RomSize)))
		Message(("ROMCodeChecksumOK() FAILED!!"));

		
	SetBoxLEDs( kConnectedLED | kMessageLED );		/* Test 011 */	

Message(("Post:Before KBSelfTestOK()"));

	if (!(KBSelfTestOK()))
		Message(("KBSelfTestOK() FAILED!!"));

	SetBoxLEDs( kPowerLED );						/* Test 100 */	
		
//	if (!(ModemTest()))
//		infiniteLoop();

//	SetBoxLEDs( kPowerLED | kMessageLED );			/* Test 101 */	

//	AudioTest();

//	SetBoxLEDs( kPowerLED | kConnectedLED);			/* Test 110 */	

Message(("Post:End"));


}


