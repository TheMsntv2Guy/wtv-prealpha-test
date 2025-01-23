#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "parallelcrap.h"
#include "ProgressBar.h"

#undef DEBUGGING

#define kTimeout 30*60
#define BUFFSIZE 0x10000
#define DEBOUNCE 1
#define kStrobes 1

#ifdef DEBUGGING
#define DEBUG(x)	printf x
#else
#define DEBUG(x)
#endif

extern int MainEvent(void);
extern Boolean ShouldAbort(void);
extern void displaybuff(char*, long);

ulong   gBridge;
ulong	gSlot;
char 	gBuf[255];
long 	gLeftOver = 0;;
Boolean gTimeout = false;

void SquirtIt(char* s)
{
	char			cmd[10];
	char 			file[255];
	long 			count = 0;
	ulong 			address;
	FILE*			fp;
	long			i;
	register		uchar	byte, *buf;
	uchar*			buffer;
	register ulong	checksum = 0;
	long			start, stop, bytesLeft, size, delay;
	
	gTimeout = false;
	
	sscanf(s, "%s %s %x", cmd, file, &address);
	
	fp = fopen(file, "rb");
	if (fp == 0) {
		sprintf(gBuf, "¥ ERROR: couldn't open file '%s'\r", file);
		displaybuff(gBuf, strlen(gBuf));
		return;
	}
	
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	buffer = buf = (uchar*)malloc(BUFFSIZE);
	if (buf == 0) {
		sprintf(gBuf, "¥ ERROR: out of memory\r");
		displaybuff(gBuf, strlen(gBuf));
		return;
	}
	
	start = TickCount();
	
	SquirtByte(kSquirtCmd);					// squirt command byte
	
	Squirt4Bytes(size);
	Squirt4Bytes(address);
	
	/* let goober spit out some info before we continue */
	for(i=0;i<5;i++)
		MainEvent();
		
	sprintf(gBuf, "Erasing flash...\r");
	InitProgress(gBuf, size/2300);
	for(i=0; i<20; i++)
	{
		if (ShouldAbort()) {
			sprintf(gBuf, "¥ ERROR: erase aborted by user\r");
			displaybuff(gBuf, strlen(gBuf));
			goto bail;
		}
		UpdateProgress(size/2300/20);
		Delay(size/2300/20, &delay);
	}
	EndProgress();
	
	/* let goober spit out some info before we continue */
	for(i=0; i<5; i++)
		MainEvent();

	bytesLeft = size;
	
	sprintf(gBuf, "Squirting '%s' (%#x bytes) to %#x...\r", file, size, address);
	InitProgress(gBuf, size);
		
	while (bytesLeft > 0)
	{
		if (ShouldAbort()) {
			sprintf(gBuf, "¥ ERROR: erase aborted by user\r");
			displaybuff(gBuf, strlen(gBuf));
			goto bail;
		}

		count = bytesLeft > BUFFSIZE ? BUFFSIZE : bytesLeft;
		if (fread(buffer, count, sizeof(uchar), fp) == 0) {
			sprintf(gBuf, "¥ ERROR: reading file '%s'\r", file);
			displaybuff(gBuf, strlen(gBuf));
			goto bail;
		}
		buf = buffer;
		gLeftOver = bytesLeft;
		for(i=0;i<count;i++)
		{
			byte = *buf++;
			SquirtByte(byte);
			checksum += byte;
			gLeftOver = bytesLeft - i;
			if (gTimeout)
				goto bail;
		}
		bytesLeft -= count;
		UpdateProgress(count);
	}

	stop = TickCount();
	sprintf(gBuf, "¥ squirted %#x bytes in %f secs (%f bytes/sec)\r",
		size, (float)(stop-start)/60.0, (float)size/(float)(stop-start)*60.0);
	displaybuff(gBuf, strlen(gBuf));
	sprintf(gBuf, "\r¥ checksum = %#x\r", checksum);
	displaybuff(gBuf, strlen(gBuf));

bail:
	free((char*)buffer);
	fclose(fp);
	gTimeout = false;
	EndProgress();
}

void Dummy(void)
{
	SetMode(kWriteMode);					// set direction to out
	SquirtByte(kDummyCmd);					// squirt command byte
}

ulong Squirt4Bytes(ulong val)
{
	ulong checksum = 0;
	uchar byte;
	
	byte = (val>>24) & 0xff;
	SquirtByte(byte);
	checksum += byte;
	
	if (gTimeout)
		return 0;
	
	byte = (val>>16) & 0xff;
	SquirtByte(byte);
	checksum += byte;
	
	if (gTimeout)
		return 0;

	byte = (val>>8) & 0xff;
	SquirtByte(byte);
	checksum += byte;
	
	if (gTimeout)
		return 0;

	byte = (val>>0) & 0xff;
	SquirtByte(byte);
	checksum += byte;
	
	return checksum;
}

void WaitForACK(char assert)
{
	uchar x;
	long timeout = 0;
	long start;
	int n = 0;
	
	start = TickCount();
	if (assert) {
		// wait for BUSY bit to assert (low) which means
		// that Goober has seen the data
		DEBUG(("Waiting for ACK\r"));
		while (1) 
		{
			x = ReadIOByte(kReadStatReg);
#ifdef DEBOUNCE
			if ((x & kACKBit) != 0)
				n++;
			else
				n = 0;
			if (n == kStrobes)
				return;
#else
			if ((x & kACKBit) != 0)
				return;
#endif
			timeout = TickCount() - start;
			if (timeout == kTimeout)
			{
				sprintf(gBuf, "¥ ERROR: Waiting for ACK timed out (%#08x bytes to go)\r", gLeftOver);
				displaybuff(gBuf, strlen(gBuf));
				gTimeout = true;
				return;
			}
		}
	} else {
		// wait for BUSY bit to de-assert (high) which means
		// that Goober is ready for more data
		DEBUG(("Waiting for not ACK\r"));
		while (1) 
		{
			x = ReadIOByte(kReadStatReg);
#ifdef DEBOUNCE
			if ((x & kACKBit) == 0)
				n++;
			else
				n = 0;
			if (n == kStrobes)
				return;
#else
			if ((x & kACKBit) == 0)
				return;
#endif				
			timeout = TickCount() - start;
			if (timeout == kTimeout)
			{
				sprintf(gBuf, "¥ ERROR: Waiting for not ACK timed out (%#08x bytes to go)\r", gLeftOver);
				displaybuff(gBuf, strlen(gBuf));
				gTimeout = true;
				return;
			}
		}
	}
}

// assumes direction bit set properly
void SquirtByte(uchar byte)
{
	WriteIOByte(kRWPortReg, byte);		// put value into port
	Strobe(true);						// assert STROBE to say we're ready
	WaitForACK(true);					// Goober asserts ACK when data available
	Strobe(false);						// de-assert STROBE to say we're done
	if (!gTimeout)
	{
		WaitForACK(false);				// Goober de-asserts ACK when ready for data
	}
	DEBUG(("Squirted %#x   (#%d)\r\r", byte, byte));
}

void Strobe(char assert)
{
	uchar x;
	
	if (assert) {
		DEBUG(("asserting STROBE\r"));
		x = ReadIOByte(kRWControlReg);
		x &= ~kStrobeBit;
		WriteIOByte(kRWControlReg, x);
	} else {
		DEBUG(("de-asserting STROBE\r"));
		x = ReadIOByte(kRWControlReg);
		x |= kStrobeBit;
		WriteIOByte(kRWControlReg, x);
	}
}

void SetMode(uchar mode)
{
	uchar x;
	
	switch(mode) {
		case kReadMode:
			DEBUG(("Setting direction to read\r"));
			x = ReadIOByte(kRWControlReg);
			x |= kDIRBit;
			WriteIOByte(kRWControlReg, x);
			break;
		case kWriteMode:
			DEBUG(("Setting direction to write\r"));
			x = ReadIOByte(kRWControlReg);
			x &= ~kDIRBit;
			WriteIOByte(kRWControlReg, x);
			break;
		default:
			printf("bad mode %d\r", mode);
	}
}

int GetVal(char* s, uchar* val) 
{
	int x;
	
  	if (isdigit(s[0])) {
  		if (s[1] == 'x')
	 	 	sscanf(s, "%x", &x);
	 	 else
	  		sscanf(s, "%d", &x);	
	  	*val = (uchar)(x & 0xff);
		return(0);
	} else
		return(1);
}

int FindCard(void)
{
    long   bridge, slot; 
    ulong   val;
    
    for (bridge = 1; bridge < 3; bridge++)
    {
        for (slot = 0; slot < 8; slot++)
        {
            val = ReadConfig(bridge, slot, 0x00);
            if (val == swap(kCardSig))
            {
                gBridge = bridge;
                gBridge *= 0x02000000;
                gSlot = slot;
                
                // turn on I/O Space Enable
                
                val = ReadConfig(bridge, slot, 0x04);
                WriteConfig(bridge, slot, 0x04, val|(1<<24));
                
                sprintf(gBuf, "¥ Parallel card found\r");
                displaybuff(gBuf, strlen(gBuf));

				SetMode(kWriteMode);					// set direction to out
				Strobe(false);
				
                return 1;
            }
        }
    }
    
    sprintf(gBuf, "¥ Parallel card NOT found\r");
    displaybuff(gBuf, strlen(gBuf));

    return 0;
}

unsigned long ReadConfig(long bridge, long slot, long reg)
{
    ulong   portAddress, portData;
    ulong   val;

    portAddress = 0xF0800000 + bridge*0x02000000;
    *(volatile ulong*)(portAddress) = (reg<<24) + (0x100000<<slot);
    
    if (**(volatile ulong **)(&portAddress) != (reg<<24) + (0x100000<<slot))
           DebugStr("\p goddammit, I just wrote the fucking portAddress and now it is wrong!");
    
    portData = 0xF0C00000 + bridge*0x02000000;
    val = *(volatile ulong*)(portData);
    
    return (val);
}


void WriteConfig(long bridge, long slot, long reg, ulong data)
{
    ulong   portAddress, portData;
    
    portAddress = 0xF0800000 + bridge*0x02000000;
    *(volatile ulong*)(portAddress) = (reg<<24) + (0x100000<<slot);

    if (**(ulong **)(&portAddress) != (reg<<24) + (0x100000<<slot))
            DebugStr("\p goddammit, I just wrote the fucking portAddress and now it is wrong!");

    portData = 0xF0C00000 + bridge*0x02000000;
    *(volatile ulong*)(portData) = data;
}

uchar ReadIOByte(long reg)
{
   ulong   portData;
    ulong   val;

    portData   = 0xF0000000 + gBridge + reg;
    val = *((volatile uchar*)(portData));
    
    return (val);
}


void WriteIOByte(long reg, uchar data)
{
   ulong   portData;
    
    portData = 0xF0000000 + gBridge + reg;
    *((volatile uchar*)(portData)) = data;
}

