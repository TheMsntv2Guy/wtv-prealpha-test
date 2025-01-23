#include "Headers.h"
#include "BoxAbsoluteGlobals.h"
#include "SiliconSerial.h"


static Boolean SiSerialNumCRC(uchar *data);


void InitSiliconSerialNumber(void)
{
uchar raw[8];
uchar temp[8];
Boolean present;
Boolean CRCOK;
uchar retries;
ulong hi = 0,lo = 0;

	Message(("Looking for Si Serial #..."));
	
	retries = kMaxSiSerialNumRetries;		/* look for part up to this many times */
	present = false;
	
	while( !present && retries-- )
		present = ReadSiliconSerialNumber(READ_AG(agCountsPerMicrosec), raw);
	
	if (present)
	{
		Message(("Found."));
		
		retries = kMaxSiSerialNumRetries;	/* try to get a good read this many times */
		CRCOK = false;
		
		while( !CRCOK && retries-- )
		{
			/* untwist the bytes */
			
			temp[0] = raw[3];
			temp[1] = raw[2];
			temp[2] = raw[1];
			temp[3] = raw[0];
	
			temp[4] = raw[7];
			temp[5] = raw[6];
			temp[6] = raw[5];
			temp[7] = raw[4];
			
			hi = ( (temp[0]<<24) | (temp[1]<<16) | (temp[2]<<8) | (temp[3]) );
			lo = ( (temp[4]<<24) | (temp[5]<<16) | (temp[6]<<8) | (temp[7]) );
			
			CRCOK = SiSerialNumCRC(temp);
			
			if ( !CRCOK )
			{
				Message(("Bad CRC, rereading..."));
				ReadSiliconSerialNumber(READ_AG(agCountsPerMicrosec), raw);
			}
		}
		
		if ( !CRCOK )
		{
			Message(("Bad CRC.  Setting Hi & Lo to -1"));
			WRITE_AG(agSerialNumberHi, 0xffffffff);
			WRITE_AG(agSerialNumberLo, 0xffffffff);
		}
		else
		{
			Message(("Serial Number = %08lx %08lx",hi,lo));
			WRITE_AG(agSerialNumberHi, hi);
			WRITE_AG(agSerialNumberLo, lo);
		}
	}
	else
	{
		Message(("NOT FOUND!  Setting Hi & Lo to 0"));
		WRITE_AG(agSerialNumberHi, 0);
		WRITE_AG(agSerialNumberLo, 0);
	}
}



static Boolean SiSerialNumCRC(uchar *data)
{
uchar crc = 0;
uchar C = 0;
uchar oldC = 0;
uchar acc;
uchar iii;
uchar bbb;

	for (iii=0; iii!=7; iii++)
	{
		acc = data[iii];

		for (bbb=0; bbb!=8; bbb++)
		{			
			acc ^= crc;
			
			if (acc & 1)
				C = 1;
			else
				C = 0;
				
			acc = crc;
			
			if (C)
				acc ^= 0x18;
				
			oldC = C;
			
			if (acc & 1)
				C = 1;
			else
				C = 0;
	
			acc >>= 1;
			
			if (oldC)
				acc |= 0x80;
				
			crc = acc;	
			
			acc = data[iii];
			
			if (acc & 1)
				acc = ((acc >> 1) | 0x80);
			else
				acc = (acc >> 1);
				
			data[iii] = acc;
		}
	}
	
	if ( crc == data[7] )
		return true;
	else
		return false;
}
