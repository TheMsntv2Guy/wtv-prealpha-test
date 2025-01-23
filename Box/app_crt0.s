#include "iregdef.h"
#include "idtcpu.h"
#include "BoxHWEquates.h"


/* 
	App crt0.s
	
	By this point, the Boot ROM has set up the memory controller, performed a primitive SPOT
	initialization, and sized RAM.  Some of the Absolute Globals (like RAM size) have been set 
	up.

	After deciding that the App ROM is valid, the Boot ROM simply jumps to the base of the App
	ROM.  The App ROM inits its data & bss sections, sets up its gp, and jumps to the App main().
	
	The App main() performs higher-level SPOT/driver initialization, then cranks up the 
	browser (or whatever) app.
*/



	.set noreorder

.ent	start
.globl	start
start:
.frame	sp,0,$31

appentry:
	b		app_start
	nop
	
rom_checksum:	.word	0x00000000			/* checksum, NOT including the checksum itself */
rom_size:		.word	0x00000000			/* size in bytes */
code_size:		.word	0x00000000			/* size in bytes */
version:		.word	0x00000000			/* version number */
data_start:		.word	0x00000000			/* address of .data in ROM image */
data_len:		.word	0x00000000			/* .data len, in WORDS */
bss_len:		.word 	0x00000000			/* .bss len, in WORDS */
romfs_address:	.word 	0x00000000			/* EMAC: romfs_address */
	
app_start:
	
	#
	# Copy data section to RAM, initialize bss
	#

									/* copy data to data area */
	lw		t0,data_start
	lw		t1,data_len
	li		t3,kDataSegmentBase		/* where data goes at runtime */
copyDataArea:
	lw		t2,0(t0)
	addu	t0,t0,4
	subu	t1,t1,1
	sw		t2,0(t3)
	bne		zero,t1,copyDataArea
	addu	t3,t3,4
	
		
								/* zero bss area, bss placed right after data */
	lw		t1,bss_len
zeroBssArea:
	sw		zero,0(t3)
	subu	t1,t1,1
	bne		zero,t1,zeroBssArea
	addu	t3,t3,4

	la		gp,_gp				/* set up the globals pointer */
				
	la		t0,main				/* t0 -> our C entry point */
	j		t0					/* here we go... (we won't be back!) */
	nop
	
.end	start


#
# Move the stack to wherever the caller tells us.
# This should only be called by main() after determining where the system stack should go.
#

.ent	move_reset_stack
.globl	move_reset_stack
move_reset_stack:
.frame	sp,0,$31

	move	sp,a0
	
	j		ra
	nop

.end	move_reset_stack



#
# main() will call this first
#

.ent	__main
.globl	__main
__main:
.frame	sp,0,$31

	j		ra
	nop

.end	__main



