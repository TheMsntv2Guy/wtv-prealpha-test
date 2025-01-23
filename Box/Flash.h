
#ifndef	__FLASH__
#define __FLASH__

#ifdef __cplusplus
extern "C" {
#endif

void WriteEnableFlash(Boolean enable);

void SetupFlash(ulong *dst);
void FlashLong(ulong data);
void FinishFlash(void);

ulong EraseBlock(volatile ulong *addr);				/* doesn't write enable */
ulong EraseBank(ulong *addr, ulong len);			/* DOES write enable */
ulong CopyToFlash(ulong *src, volatile ulong *dst, ulong len);

ulong EraseAndFlashBlock(ulong *src, ulong *dst);	/* Smart Flasher - call this to copy from RAM to Flash */
													/*  while executing from Flash */

void DupeFlasher(void);								/* move RAM-based Flasher to 0xa0000000 */
ulong CallRAMFlasher(ulong *src, ulong *dst);		/* call this to do a flash */
ulong FlashBlockRAM(ulong *src, ulong *dst);		/* never actually call this */


#ifdef __cplusplus
}
#endif

#endif /* __FLASH__ */
