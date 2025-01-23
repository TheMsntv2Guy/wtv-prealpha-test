

#include "idtcpu.h"

extern unsigned long icache_size;
extern unsigned long dcache_size;

/*
** Config_cache() -- determine sizes of i and d caches
** Sizes stored in globals dcache_size and icache_size
*/
#define CONFIGFRM ((4*4)+4+4)	


asm void config_cache(void)
{
	.set	noreorder
	subu		sp,CONFIGFRM
	sw		ra,CONFIGFRM-4(sp)	// save return address
	sw		s0,4*4(sp)		// save s0 in first regsave slot
	mfc0		s0,C0_SR			// save SR
	mtc0		zero,C0_SR		// disable interrupts
	.set	reorder
	jal		_size_cache
	sw		v0,dcache_size
	li		v0,SR_SWC		// swap caches
	.set	noreorder
	mtc0		v0,C0_SR
	jal		_size_cache
	nop
	sw		v0,icache_size
	mtc0		zero,C0_SR		// swap back caches
	and		s0,~SR_PE		// do not inadvertantly clear PE
	mtc0		s0,C0_SR			// restore SR
	.set	reorder
	lw		s0,4*4(sp)		// restore s0
	lw		ra,CONFIGFRM-4(sp)	// restore ra
	addu		sp,CONFIGFRM		// pop stack
	j		ra
}	
	

/*
** _size_cache()
** return size of current data cache
*/
FRAME(_size_cache,sp,0,ra)
	.set	noreorder
	mfc0	t0,C0_SR		# save current sr
	and	t0,~SR_PE		# do not inadvertently clear PE
	or	v0,t0,SR_ISC		# isolate cache
	mtc0	v0,C0_SR
	/*
	 * First check if there is a cache there at all
	 */
	move	v0,zero
	li	v1,0xa5a5a5a5		# distinctive pattern
	sw	v1,K0BASE		# try to write into cache
	lw	t1,K0BASE		# try to read from cache
	nop
	mfc0	t2,C0_SR
	nop
	.set	reorder
	and	t2,SR_CM
	bne	t2,zero,3f		# cache miss, must be no cache
	bne	v1,t1,3f		# data not equal -> no cache
	/*
	 * Clear cache size boundries to known state.
	 */
	li	v0,MINCACHE
1:
	sw	zero,K0BASE(v0)
	sll	v0,1
	ble	v0,MAXCACHE,1b

	li	v0,-1
	sw	v0,K0BASE(zero)		# store marker in cache
	li	v0,MINCACHE		# MIN cache size

2:	lw	v1,K0BASE(v0)		# Look for marker
	bne	v1,zero,3f		# found marker
	sll	v0,1			# cache size * 2
	ble	v0,MAXCACHE,2b		# keep looking
	move	v0,zero			# must be no cache
	.set	noreorder
3:	mtc0	t0,C0_SR		# restore sr
	j	ra
	nop
	ENDFRAME(_size_cache)
	.set	reorder
	
	
/*
** FlushCache()
** flush entire cache
*/
FRAME(FlushCache,sp,0,ra)
	lw	t1,icache_size	
	lw	t2,dcache_size
	.set	noreorder
	mfc0	t3,C0_SR		# save SR
	nop
	and	t3,~SR_PE		# dont inadvertently clear PE
	beq	t1,zero,_check_dcache	# if no i-cache check d-cache
	nop
	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	li	t0,K0BASE
	.set	reorder
	or	t1,t0,t1

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b
	/*
	 * flush data cache
	 */
_check_dcache:
	li	v0,SR_ISC		# isolate and swap back caches
	.set	noreorder
	mtc0	v0,C0_SR
	nop
	beq	t2,zero,_flush_done
	.set	reorder
	li	t0,K0BASE
	or	t1,t0,t2

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b

	.set	noreorder
_flush_done:
	mtc0	t3,C0_SR		# un-isolate, enable interrupts
	.set	reorder
	j	ra
	ENDFRAME(FlushCache)


/*
** ClearCache(void* baseAddress, ulong byteCount)
** flush portion of cache
*/
FRAME(ClearCache,sp,0,ra)

	/*
	 * flush instruction cache
	 */
	lw	t1,icache_size
	lw	t2,dcache_size
	.set	noreorder
	mfc0	t3,C0_SR		# save SR
	and	t3,~SR_PE		# dont inadvertently clear PE
	nop
	nop
	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	.set	reorder
	bltu	t1,a1,1f		# cache is smaller than region
	move	t1,a1
1:	addu	t1,a0			# ending address + 1
	move	t0,a0

	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t1,1b

	/*
	 * flush data cache
	 */

	.set	noreorder
	nop
	li	v0,SR_ISC		# isolate and swap back caches
	mtc0	v0,C0_SR
	nop
	.set	reorder
	bltu	t2,a1,1f		# cache is smaller than region
	move	t2,a1
1:	addu	t2,a0			# ending address + 1
	move	t0,a0

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t2,1b

	.set	noreorder
	mtc0	t3,C0_SR		# un-isolate, enable interrupts
	.set	reorder
	j	ra
	ENDFRAME(ClearCache)


