#include "iregdef.h"
#include "idtcpu.h"
#include "frame.h"
		
/*
** FlushDataCache()
** flush entire data cache
*/
FRAME(FlushDataCache,sp,0,ra)
	.set	noreorder

	li	a0,0x9fc00000		/* a0 -> base of cacheable seg */
	li	t0,255			/* 8KB, 32bytes/line -> 256 lines */
	
DFlushLoop:
	cache	0x1,0(a0)		/* Index WBack Invalidate - Set 0 (bit 13 selects set) */
	cache	0x1,0x2000(a0)		/* Index WBack Invalidate - Set 1 (bit 13 selects set) */
	
	lw	zero,0(a0)		/* load good data to clear dirty bits & set good parity */
	lw	zero,0x2000(a0)
	
	cache	0x1,0(a0)		/* write block (set 0) and mark as invalid */
	cache	0x1,0x2000(a0)		/* write block (set 1) and mark as invalid */
	
	addu	a0,32			/* next line... */
	bgtz	t0,DFlushLoop
	addi	t0,-1
	
	jr	ra
	nop
	
ENDFRAME(FlushDataCache)
	
/*
** FlushDataCacheAddr()
** flush address range data cache
*/
#ifdef _INCLUDE_DEAD_CODE_
FRAME(FlushDataCacheAddr,sp,0,ra)
	.set	noreorder

1:
	cache	0x19,0(a0)		/* Hit WriteBack */
	addu	a0,32			/* next line... */
	bgtz	a1,1b
	addi	a1,-1
	
	jr	ra
	nop
	
ENDFRAME(FlushDataCacheAddr)
#endif
	
/*
** FlushCache()
** flush entire cache
*/
#ifdef _INCLUDE_DEAD_CODE_
FRAME(FlushCache,sp,0,ra)
	.set	noreorder

	j	ClearCache		/* clears everything, for now */
	nop
	
ENDFRAME(FlushCache)
#endif

/*
** ClearCache(void* baseAddress, ulong byteCount)
** flush portion of cache
*/
FRAME(ClearCache,sp,0,ra)		/* еее HACK HACK HACK - FIXME! This is way overkill */
	.set	noreorder

	# Initialize CPU caches

	# DATA
	
	mtc0	zero,C0_TAGLO		/* Cache state == 00 --> Invalid */
	li	a0,0x9fc00000		/* a0 -> base of cacheable seg */
	li	t0,255			/* 8KB, 32bytes/line -> 256 lines */
	
DInitLoop:
	cache	0x9,0(a0)		/* Index Store Tag - Set 0 (bit 13 selects set) */
	cache	0x9,0x2000(a0)		/* Index Store Tag - Set 1 (bit 13 selects set) */
	
	lw	zero,0(a0)		/* load good data to clear dirty bits & set good parity */
	lw	zero,0x2000(a0)
	
	cache	0x9,0(a0)		/* re-invalidate the tags */
	cache	0x9,0x2000(a0)
	
	addu	a0,32			/* next line... */
	bgtz	t0,DInitLoop
	addi	t0,-1
	
	# INSTRUCTION
	
	mtc0	zero,C0_TAGLO
	li	a0,0x9fc00000
	li	t0,255
	
IInitLoop:
	cache	0x8,0(a0)		/* Index Store Tag - Set 0 (bit 13 selects set) */		
	cache	0x8,0x2000(a0)		/* Index Store Tag - Set 1 (bit 13 selects set) */
	
	cache	0x14,0(a0)		/* fill I cache from memory */
	cache	0x14,0(a0)
	
	cache	0x8,0(a0)		/* re-invalidate the tags */		
	cache	0x8,0x2000(a0)		

	addu	a0,32			/* next line... */
	bgtz	t0,IInitLoop
	addi	t0,-1

	j	ra
	nop
	
ENDFRAME(ClearCache)


