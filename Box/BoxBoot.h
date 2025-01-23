
#ifndef	__BOX_BOOT__
#define __BOX_BOOT__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct 
{
	unsigned long		branch;				/* don't whack this, it's the reset instruction! */
	unsigned long 		nop;				/* don't whack this, it's the delay nop! */
	unsigned long 		rom_checksum;		/* checksum, NOT including the checksum itself */
	unsigned long 		rom_size;			/* size in WORDS */
	unsigned long 		code_size;			/* size in WORDS */
	unsigned long		version;			/* passed in as cmd line arg when buildrom is invoked */
	unsigned long		data_start;			/* offset into ROM to .data section */
	unsigned long		data_length;
	unsigned long		bss_length;

} ROMHeader;

void	move_reset_stack(ulong newTOS);
void	move_exc_code(void);

void 	CheckSystemIntegrity(void);

extern 	void	JumpToApp();				/* only for Boot ROM & Flash downloader */


#ifdef __cplusplus
}
#endif

#endif