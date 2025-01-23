/* ===========================================================================
 *	mem.c
 *
 *	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#include "Headers.h"


/*
	These can be optimized significantly.
 */


void *memchr(const void *s, int c, size_t n)
{
	const unsigned char uc = (const unsigned char)c;
	const unsigned char* sc = (const unsigned char*)s;

	if (ValidReadLocations(s, n)) {
		while (n-- > 0) {
			if (uc == *sc) {
				return (void*)sc;
			}
			sc++;
		}
	}
	return NULL;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char* s1c = (const unsigned char*)s1;
	const unsigned char* s2c = (const unsigned char*)s2;

#ifdef SIMULATOR
	if (!(ValidReadLocations(s1, n) && ValidReadLocations(s2, n)))
		return (s1==s2) ? 0 : (((long)s1 < (long)s2) ? -1 : 1);
#endif
	{
		while (n--) {
			if (*s1c == *s2c) {
				s1c++;
				s2c++;
			} else {
				return (*s1c > *s2c) ? 1 : -1;
			}
		}
	}
	return 0;
}

void *memcpy(void *s1, const void *s2, size_t n)
{
	unsigned char* s1c = (unsigned char*)s1;
	const unsigned char* s2c = (const unsigned char*)s2;

#ifdef DEBUG
	if (IsError((s2c < s1c) && (s1c < s2c + n))) {
		return memmove(s1, s2, n);	/* don't use memcpy for overlapping blocks!! */
	}
#endif
#ifdef SIMULATOR
	if (ValidWriteLocations(s1, n) && ValidReadLocations(s2, n))
#endif
	{
		return memmove(s1, s2, n);
	}
	return s1;
}

/*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef	int word;		/* "word" used for optimal copy speed */

#define	wsize	sizeof(word)
#define	wmask	(wsize - 1)
#define quadsize (sizeof(word) * 4)
#define quadmask (quadsize - 1)

void *memmove(void *s1, const void *s2, size_t n)
{
#ifdef SIMULATOR
	if (ValidWriteLocations(s1, n) && ValidReadLocations(s2, n))
#endif
	{
		register char *dst = (char *) s1;
		register const char *src = (const char *) s2;
		register size_t t;

		if (n == 0 || dst == src)		/* nothing to do */
			goto done;

		if ((ulong)dst < (ulong)src) {
			/*
			 * Copy forward.
		 	 */
			t = (int)src;	/* only need low bits */
			if ((t | (int)dst) & wmask) {
				/*
				 * Try to align operands.  This cannot be done
				 * unless the low bits match.
				 */
				if ((t ^ (int)dst) & wmask || n < wsize)
					t = n;
				else
					t = wsize - (t & wmask);
				n -= t;
				do { *dst++ = *src++; } while (--t);
			}
			/* Check for quad aligned case */
			t = n / quadsize;
			if (t) {
				if (!(((int) src | (int)dst) & quadmask)) {
					register ulong t1,t2,t3,t4;

					n -= t * quadsize;
					do {
						t1 = *(ulong *)src;
						t2 = *(ulong *)(src+4);
						t3 = *(ulong *)(src+8);
						t4 = *(ulong *)(src+12);
						*(ulong *)dst 	  = t1;
						*(ulong *)(dst+4)  = t2;
						*(ulong *)(dst+8)  = t3;
						*(ulong *)(dst+12) = t4;
						src += quadsize;
						dst += quadsize;
					} while (--t);
				}
			}
			t = n / wsize;
			if (t) do {
				*(word *)dst = *(word *)src; src += wsize; dst += wsize;
			} while (--t);
			t = n & wmask;
			if (t) do {
				*dst++ = *src++;
			} while (--t);
		} else {
			/*
			 * Copy backwards.  Otherwise essentially the same.
			 * Alignment works as before, except that it takes
			 * (t&wmask) bytes to align, not wsize-(t&wmask).
			 */
			src += n;
			dst += n;
			t = (int)src;
			if ((t | (int)dst) & wmask) {
				if ((t ^ (int)dst) & wmask || n <= wsize)
					t = n;
				else
					t &= wmask;
				n -= t;
				do { *--dst = *--src; } while (--t);
			}
			/* Check for quad aligned case */
			t = n / quadsize;
			if (t) {
				if (!(((int) src | (int)dst) & quadmask)) {
					register ulong t1,t2,t3,t4;
			
					n -= t * quadsize;
					do {
						src -= quadsize;
						dst -= quadsize;
						t1 = *(ulong *)src;
						t2 = *(ulong *)(src+4);
						t3 = *(ulong *)(src+8);
						t4 = *(ulong *)(src+12);
						*(ulong *)dst 	  = t1;
						*(ulong *)(dst+4)  = t2;
						*(ulong *)(dst+8)  = t3;
						*(ulong *)(dst+12) = t4;
					} while (--t);
				}
			}
			t = n / wsize;
			if (t) do {
				src -= wsize; dst -= wsize; *(word *)dst = *(word *)src;
			} while (--t);
			t = n & wmask;
			if (t) do {
				*--dst = *--src;
			} while (--t);		
		}
	}
done:
	return (s1);
	
}

void *memset(void *dst0, register int c0, register size_t length)
{
	register size_t t;
	register u_int c;
	register uchar *dst;

#ifdef SIMULATOR
	if (ValidWriteLocations(dst0, length))
#endif
	{
		dst = (uchar *)dst0;

		if (length < 3 * wsize) {
			while (length != 0) {
				*dst++ = c0;
				--length;
			}
			return (dst0);
		}

		if ((c = (uchar)c0) != 0) {	/* Fill the word. */
			c = (c << 8) | c;
			c = (c << 16) | c;
		}
		/* Align destination by filling in bytes. */
		if ((t = (int)dst & wmask) != 0) {
			t = wsize - t;
			length -= t;
			do {
				*dst++ = c0;
			} while (--t != 0);
		}

		/* Fill words.  Length was >= 2*words so we know t >= 1 here. */
		t = length / wsize;
		do {
			*(uint *)dst = c;
			dst += wsize;
		} while (--t != 0);

		/* Mop up trailing bytes, if any. */
		t = length & wmask;
		if (t != 0)
			do {
				*dst++ = c0;
			} while (--t != 0);
	}
	return (dst0);
}