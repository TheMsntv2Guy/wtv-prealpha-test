#include "WTVTypes.h"
#include "boxansi.h"
#include "SystemGlobals.h"
#include "HWRegs.h"
#include "IIC.h"

static void Start(void);
static void Stop(void);
static void SendBit(uchar value);
static void SendByte(uchar byte);
static void Ack(void);
static uchar ReadBit(void);
static uchar ReadByte(void);

#undef DEBUGGING

#ifdef DEBUGGING
#define DEBUG_IIC(x) printf x
#else
#define DEBUG_IIC(x)
#endif

#define WAIT() {int i ; for(i=0;i<100;i++)  *(vulong*)kSPOTBase; }

void InitIIC(void)
{
	*(vulong*)kDevNVCntlReg = kNVClock;
}

static void Start(void)
{
	ulong temp;
	
	DEBUG_IIC(("IIC : Start\n"));
	
	/* high to low transition while clock is high */
	
	*(vulong*)kDevNVCntlReg = kNVClock;

	temp = *(vulong*)kDevNVCntlReg;
	temp |= (kNVDataOut | kNVDataOutEn);
	*(vulong*)kDevNVCntlReg = temp;
	
	WAIT();

	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVDataOut;
	*(vulong*)kDevNVCntlReg = temp;
	
	WAIT();
	
	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVClock;
	*(vulong*)kDevNVCntlReg = temp;

	WAIT();
}

static void Stop(void)
{
	ulong temp;

	DEBUG_IIC(("IIC : Stop\n"));
	/* low to high transition while clock is high */
	
	WAIT();
	
	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVDataOut;
	temp |= kNVDataOutEn;
	*(vulong*)kDevNVCntlReg = temp;

	WAIT();

	temp = *(vulong*)kDevNVCntlReg;
	temp |= kNVClock;
	*(vulong*)kDevNVCntlReg = temp;

	WAIT();

	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVDataOutEn;
	*(vulong*)kDevNVCntlReg = temp;

	*(vulong*)kDevNVCntlReg;  /* flush write buffer */
}

static void SendBit(uchar value)
{
	ulong temp;
	
	WAIT();

	/* set bit accordingly */
	temp = *(vulong*)kDevNVCntlReg;
	if(value) {
		temp &= ~kNVDataOutEn;
	} else {
		temp &= ~kNVDataOut;
		temp |= kNVDataOutEn;
	}
	*(vulong*)kDevNVCntlReg = temp;
		
	WAIT();

	/* clock on */
	temp = *(vulong*)kDevNVCntlReg;
	temp |= kNVClock;
	*(vulong*)kDevNVCntlReg = temp;

	WAIT();
	
	/* clock off */
	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVClock;
	*(vulong*)kDevNVCntlReg = temp;
	
	WAIT();

	/* output off */
	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVDataOutEn;
	*(vulong*)kDevNVCntlReg = temp;

	*(vulong*)kDevNVCntlReg;  /* flush write buffer */
}

static uchar ReadBit(void)
{
	ulong temp;
	ulong bit;
	
	WAIT();

	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVDataOutEn;
	*(vulong*)kDevNVCntlReg = temp;

	WAIT();

	temp = *(vulong*)kDevNVCntlReg;
	temp |= kNVClock;
	*(vulong*)kDevNVCntlReg = temp;
	
	WAIT();
	
	bit = *(vulong*)kDevNVCntlReg & kNVDataIn;
	
	WAIT();

	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVClock;
	*(vulong*)kDevNVCntlReg = temp;

	*(vulong*)kDevNVCntlReg;  /* flush write buffer */

	return bit;
}

static void Ack(void)
{
	ulong temp;
	
	WAIT();

	temp = *(vulong*)kDevNVCntlReg;
	temp |= kNVClock;
	*(vulong*)kDevNVCntlReg = temp;
			
	WAIT();

	DEBUG_IIC(("IIC : waiting for ACK\n"));
	while( ( *(vulong*)kDevNVCntlReg & kNVDataIn ) )
		;
	DEBUG_IIC(("IIC : received ACK\n"));
	
	temp = *(vulong*)kDevNVCntlReg;
	temp &= ~kNVClock;
	*(vulong*)kDevNVCntlReg = temp;
	
	WAIT();
}

static void SendByte(uchar byte)
{
	int i;
	
	DEBUG_IIC(("IIC : SendByte %x\n",byte));
	for(i=7;i >= 0;i--)
		SendBit( ( byte >> i ) & 0x1 ) ;
	*(vulong*)kDevNVCntlReg;  /* flush write buffer */
}

static uchar ReadByte(void)
{
	uchar byte = 0;
	
	byte = 	( 	
				( ReadBit() << 7) |
				( ReadBit() << 6) |
				( ReadBit() << 5) |
				( ReadBit() << 4) |
				( ReadBit() << 3) |
				( ReadBit() << 2) |
				( ReadBit() << 1) |
			 	( ReadBit() )
			) ;
	DEBUG_IIC(("IIC : ReadByte %x\n",byte));
	return byte;
}

void IICWrite(uchar addr,uchar subaddr,uchar val)
{
	Start();
	SendByte(addr & ~1);
	Ack();
	SendByte(subaddr);
	Ack();
	SendByte(val);
	Ack();
	Stop();
}

/* read a byte from NVRAM */
uchar IICRead(uchar addr,uchar subaddr)
{
	uchar byte;
	
	Start();
	SendByte(addr & ~1);
	Ack();
	SendByte(subaddr);
	Ack();
	Start();
	SendByte(addr | 1);
	Ack();
	byte = ReadByte();
	SendBit(1);				/* ack read to device */
	Stop();
	return byte;
}