/* lib/bn/bn_mul.c */
/* Copyright (C) 1995 Eric Young (eay@mincom.oz.au)
 * All rights reserved.
 * 
 * This file is part of an SSL implementation written
 * by Eric Young (eay@mincom.oz.au).
 * The implementation was written so as to conform with Netscapes SSL
 * specification.  This library and applications are
 * FREE FOR COMMERCIAL AND NON-COMMERCIAL USE
 * as long as the following conditions are aheared to.
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.  If this code is used in a product,
 * Eric Young should be given attribution as the author of the parts used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Eric Young (eay@mincom.oz.au)
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include "crypto.h"
#include "bn.h"
#include "bn_lcl.h"

#define			LBITS(a)	((a)&BN_MASK2l)
#define			HBITS(a)	(((a)>>BN_BITS4)&BN_MASK2l)
#define			L2HBITS(a)	(((a)&BN_MASK2l)<<BN_BITS4)

/* r must be different to a and b */
int BN_mul(BIGNUM *r, BIGNUM *a, BIGNUM *b)
	{
	int i;
	int max,al,bl;
	BN_ULONG *ap,*bp,*rp;

	al=a->top;
	bl=b->top;
	if ((al == 0) || (bl == 0))
		{
		r->top=0;
		return(1);
		}

	max=(al+bl+1);
	BN_clear(r);
	if (bn_expand(r,(max+1)*BN_BITS2) == NULL) return(0);
	r->top=max;
	r->neg=a->neg^b->neg;
	ap=a->d;
	bp=b->d;
	rp=r->d;
	for (i=0; i<bl; i++)
		{
		rp[al]=bn_mul_add_word(rp,ap,al,*(bp++));
		rp++;
		}
	bn_fix_top(r);
	return(1);
	}

