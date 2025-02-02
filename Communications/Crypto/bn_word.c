/* lib/bn/bn_word.c */
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

BN_ULONG BN_mod_word(BIGNUM *a, unsigned long w)
	{
	BN_ULONG ret;
	int i;

	ret=0;
	for (i=a->top-1; i>=0; i--)
		{
#ifndef BN_LLONG
		ret=((ret<<BN_BITS4)|((a->d[i]>>BN_BITS4)&BN_MASK2l))%w;
		ret=((ret<<BN_BITS4)|(a->d[i]&BN_MASK2l))%w;
#else
		ret=(((BN_ULLONG)ret<<BN_BITS2)|a->d[i])%w;
#endif
		}
	return(ret);
	}

BN_ULONG BN_div_word(BIGNUM *a, unsigned long w)
	{
	BN_ULONG ret;
	int i;

	if (a->top == 0) return(0);
	ret=0;
	for (i=a->top-1; i>=0; i--)
		{
#ifndef BN_LLONG
		ret=((ret<<BN_BITS4)|((a->d[i]>>BN_BITS4)&BN_MASK2l))%w;
		ret=((ret<<BN_BITS4)|(a->d[i]&BN_MASK2l))%w;
#else
		BN_ULLONG ll;

		ll=((BN_ULLONG)ret<<BN_BITS2)|a->d[i];
		a->d[i]=(BN_ULONG)(ll/w);
		ret=(BN_ULONG)(ll%w);
#endif
		}
	if (a->d[a->top-1] == 0)
		a->top--;
	return(ret);
	}

int BN_add_word(BIGNUM *a, unsigned long w)
	{
	BN_ULONG l;
	int i;

	if (bn_expand(a,a->top*BN_BITS2+1) == NULL) return(0);
	i=0;
	for (;;)
		{
		l=(a->d[i]+w)&BN_MASK2;
		a->d[i]=l;
		if (w > l)
			w=1;
		else
			break;
		i++;
		}
	if (i >= a->top)
		a->top++;
	return(1);
	}

#ifdef undef
BN_ULONG *BN_mod_inverse_word(BN_ULONG a)
	{
	BN_ULONG A,B,X,Y,M,D,R,RET,T;
	int sign,hight=1;

	X=0;
	Y=1;
	A=0;
	B=a;
	sign=1;

	while (B != 0)
		{

#endif

