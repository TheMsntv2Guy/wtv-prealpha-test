#ifndef __BOOT_H__
#define __BOOT_H__


void		LateBoot(void);
void		LateShutdown(void);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Boot.h multiple times"
	#endif
#endif /* __BOOT_H__ */
