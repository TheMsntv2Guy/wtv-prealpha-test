#include "WTVTypes.h"
#include "HWRegs.h"
#include "BoxUtils.h"
#include "Exceptions.h"
#include "Debug.h"
#include "Fence.h"

#include "Flash.h"


#define	kWSMReady_Both		0x00800080
#define	kEraseError_Both	0x00200020

#define	kBSRQueueFull		0x00080008
#define	kBSRBlockReady		0x00800080

#define	kOperationFail		0x00200020


void WriteEnableFlash(Boolean enable)
{
ulong temp0, temp1;

	if(SPOTVersion() == kSPOT3)
	{
		temp0 = *(volatile ulong *)(kSPOTBase + kROMCntl0);
		temp1 = *(volatile ulong *)(kSPOTBase + kROMCntl1);
		
		if(enable)
		{
			temp0 &= ~kROMProtectMask;		/* 0 = WRITEABLE */
			temp1 &= ~kROMProtectMask;
		}
		else
		{
			temp0 |= kROMProtectMask;		/* 1 = PROTECTED */
			temp1 |= kROMProtectMask;
		}
		
		*(volatile ulong *)(kSPOTBase + kROMCntl0) = temp0;
		*(volatile ulong *)(kSPOTBase + kROMCntl1) = temp1;
	}
}



#if !defined(BOOTROM) && !defined(APPROM)

ulong EraseBlock(volatile ulong *addr)
{
ulong csr;

	*addr = 0x20202020;			/* single block erase cmd, both banks */
	*addr = 0xd0d0d0d0;			/* confirm cmd, both banks */
	*addr = 0xd0d0d0d0;			/* resume cmd, both banks */
	
	while((*addr & kWSMReady_Both) != kWSMReady_Both)
#ifdef SPOT3_REFRESH_HACK
		*(volatile ulong *)kMemCmdReg = 0x48000000;		/* refresh */
#else
		;												/* add timeout here */
#endif		
	
	csr = *addr;
	
	*addr = 0x50505050;			/* clear status reg cmd */
	*addr = 0xffffffff;			/* reset devices to read array mode */
	
	return(csr & kEraseError_Both);
}


/* addr -> base of 64KB block */
/* len  =  length in bytes to erase */
ulong EraseBank(ulong *addr, ulong len)
{
ulong err = 0;

	WriteEnableFlash(true);

	len >>= 17;			/* bytes -> 128KB blocks (do 2 chips at a time, each has a 64KB block) */
	
	while(len)
	{
		err = EraseBlock(addr);
		
		if(!err)
			addr += (65536>>1);		/* ulong *, 64KB block = 32KH block */
		else
			goto error;
		len--;
	}
	
error:
	WriteEnableFlash(false);

	len <<= 24;
	return (err | len);
}

/* src & dst should be block-aligned, len is in bytes */

#if 0

ulong CopyToFlash(ulong *src, ulong *dst, ulong len)
{
volatile ulong *blockbase;
ulong err1, err2;

	len >>= 17;			/* bytes -> 128KB blocks (do 2 chips at a time, each has a 64KB block) */

	while(len--)		/* write all blocks */
	{
		blockbase = dst;
		
		do
		{
			*blockbase = 0x71717171;		/* send read ESR cmd to both chips */
		
			while((*(blockbase+1) & kBSRQueueFull) != 0)
				;
			
			*blockbase = 0x10101010;		/* write word cmd */
		
			*dst = *src;
		
			*dst = 0x71717171;				/* send read ESR cmd to both chips */

			while((*(blockbase+1) & kBSRBlockReady) != kBSRBlockReady)
				;
		
			err1 = *(blockbase+1);		/* BSR */
			err2 = *(blockbase+2);		/* GSR */
			
			if ((err1 & kOperationFail) != 0)
				goto error;
			
			if ((err2 & kOperationFail) != 0)
				goto error;
						
			dst++;
			src++;
						
		}
		while((ulong)dst & 0x0000ffff);
  	}

error:	
	*blockbase = 0x50505050;				/* clear status reg cmd */
	*blockbase = 0xffffffff;				/* read array mode */

	return ( len );
}

#else

#ifdef GOOBER
ulong CopyToFlash(ulong *src, volatile ulong *dst, ulong len)
{
volatile ulong *blockbase = nil;

	WriteEnableFlash(true);

	len >>= 17;			/* bytes -> 128KB blocks (do 2 chips at a time, each has a 64KB block) */

	while(len--)		/* write all blocks */
	{
		blockbase = dst;
		
		do
		{
			*dst = 0x10101010;				/* write word cmd */

			*dst = *src;					/* write data */

			while((*dst & kWSMReady_Both) != kWSMReady_Both)
				;												/* add timeout here */
		
			dst++;
			src++;
						
		}
		while((ulong)dst & 0x0000ffff);
  	}

	if ( blockbase ) 
	{
		*blockbase = 0x50505050;				/* clear status reg cmd */
		*blockbase = 0xffffffff;				/* read array mode */
	}
	WriteEnableFlash(false);

	return ( len );
}
#endif	/* dead code */

#endif

#endif /* BOOTROM */



/*	*****************************************************
	Persistent Data/Modem Flash Download Utility Routines
	***************************************************** */

/* 	This routine handles copying a complete 128KB block of data from RAM to Flash.
	Appropriate action is taken for the early trial systems that *only* have Flash
	(no Mask ROM).

	It turns off the Fences for the duration of the Flash.
	
	Interrupts are disabled for quite a while, but refreshing for SPOT3 systems is
	handled.
*/

ulong EraseAndFlashBlock(ulong *src, ulong *dst)
{
ulong err;
ulong oldROMTiming;
ulong oldFences;

	Message(("EraseAndFlashBlock : src = %lx, dst = %lx...",(ulong)src,(ulong)dst));

	WriteEnableFlash(true);

	oldFences = DisableFences(kFence1ReadIntMask | kFence2WriteIntMask);
	
	Message(("Copying Flash code to RAM..."));
	DupeFlasher();				/* assumes fences off, flash write-enabled */
				
	Message(("Jumping to Flash code..."));

	disable_cpu_ints(1);	/* clear IEc */

	oldROMTiming = *(volatile ulong*)(kSPOTBase+kROMCntl1);	
	*(volatile ulong*)(kSPOTBase+kROMCntl1) = kROMCntl1_Init;

	err = CallRAMFlasher(src, dst);

	EnableFences(oldFences);

	*(volatile ulong*)(kSPOTBase+kROMCntl1) = oldROMTiming;

	enable_cpu_ints(0);		/* turn IEc back on */
	
	WriteEnableFlash(false);
	
	return err;
}		



#ifdef GOOBER

/*	**************************************** 
	Debugger Flash Download Utility Routines
	**************************************** */

volatile ulong *gBlockBase;
volatile ulong *gDst;

void SetupFlash(ulong *dst)
{
	gBlockBase = dst;
	gDst = dst;

	WriteEnableFlash(true);
}

void FlashLong(ulong data)
{
	if(((ulong)gDst & 0x0000ffff) == 0)		/* crossed a blk boundary? */
		gBlockBase = gDst;					/* new block base */

	*gDst = 0x10101010;				/* write word cmd */
	*gDst = data;					/* write data */

	while((*gDst & kWSMReady_Both) != kWSMReady_Both)
#ifdef SPOT3_REFRESH_HACK
		*(volatile ulong *)kMemCmdReg = 0x48000000;		/* refresh */
#else
		;												/* add timeout here */
#endif		

	gDst++;
}

void FinishFlash(void)
{
	*gBlockBase = 0x50505050;		/* clear status reg cmd */
	*gBlockBase = 0xffffffff;		/* read array mode */

	WriteEnableFlash(false);
}

#endif /* GOOBER */
