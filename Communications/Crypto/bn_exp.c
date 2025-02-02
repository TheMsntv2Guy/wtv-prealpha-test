/* lib/bn/bn_exp8.c */
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

int BN_mod_mul(BIGNUM *ret, BIGNUM *a, BIGNUM *b, BIGNUM *m, BN_CTX *ctx)
	{
	BIGNUM *t;
	int r=0;

	t=ctx->bn[ctx->tos++];
	if (!BN_mul(t,a,b)) goto err;
	if (!BN_mod(ret,t,m,ctx)) goto err;
	r=1;
err:
	ctx->tos--;
	return(r);
	}

#ifndef RECP_MUL_MOD
/* this one works */
int BN_mod_exp(BIGNUM *r, BIGNUM *a, BIGNUM *p, BIGNUM *m, BN_CTX *ctx)
	{
	int i,bits,ret=0;
	BIGNUM *v,*tmp;

	v=ctx->bn[ctx->tos++];
	tmp=ctx->bn[ctx->tos++];

	if (BN_copy(v,a) == NULL) goto err;
	bits=BN_num_bits(p);

	if (p->d[0]&1)
		{ if (BN_copy(r,a) == NULL) goto err; }
	else	{ if (BN_one(r) == NULL) goto err; }

	for (i=1; i<bits; i++)
		{
		if (!BN_sqr(tmp,v,ctx)) goto err;
		if (!BN_mod(v,tmp,m,ctx)) goto err;
		if (BN_is_bit_set(p,i))
			{
			if (!BN_mul(tmp,r,v)) goto err;
			if (!BN_mod(r,tmp,m,ctx)) goto err;
			}
		}
	ret=1;
err:
	ctx->tos-=2;
	return(ret);
	}

#else
int BN_mod_exp(BIGNUM *r, BIGNUM *a, BIGNUM *p, BIGNUM *m, BN_CTX *ctx)
	{
	int nb,i,j=0,bits,ret=0;
	BIGNUM *v,*tmp,*d,*pc[8];
	BN_CTX *c2=NULL;

	v=ctx->bn[ctx->tos];
	tmp=ctx->bn[ctx->tos+1];
	d=ctx->bn[ctx->tos+2];
	ctx->tos+=3;

	if (!BN_mod(v,a,m,ctx)) goto err;
	bits=BN_num_bits(p);

	if (!BN_one(r)) goto err;

	nb=BN_reciprocal(d,m,ctx);
	if (nb == -1)
		goto err;

	if ((c2=BN_CTX_new()) == NULL) goto err;
	for (i=0; i<8; i++)
		pc[i]=c2->bn[i];
	c2->tos+=7;
	if (!BN_copy(pc[1],v)) goto err;
	if (!BN_mod_mul_reciprocal(pc[2],pc[1],pc[1],m,d,nb,ctx)) goto err;
	if (!BN_mod_mul_reciprocal(pc[3],pc[2],pc[1],m,d,nb,ctx)) goto err;
	if (!BN_mod_mul_reciprocal(pc[4],pc[2],pc[2],m,d,nb,ctx)) goto err;
	if (!BN_mod_mul_reciprocal(pc[5],pc[4],pc[1],m,d,nb,ctx)) goto err;
	if (!BN_mod_mul_reciprocal(pc[6],pc[3],pc[3],m,d,nb,ctx)) goto err;
	if (!BN_mod_mul_reciprocal(pc[7],pc[6],pc[1],m,d,nb,ctx)) goto err;

	bits=((bits-1)/3)*3; /* for 4 bits, start at 2 */
	for (i=bits; i>=0; i-=3)
		{
		if (!BN_mod_mul_reciprocal(r,r,r,m,d,nb,ctx)) goto err;
		if (!BN_mod_mul_reciprocal(r,r,r,m,d,nb,ctx)) goto err;
		if (!BN_mod_mul_reciprocal(r,r,r,m,d,nb,ctx)) goto err;
		j=	 BN_is_bit_set(p,i)+
			(BN_is_bit_set(p,i+1)<<1)+
			(BN_is_bit_set(p,i+2)<<2);
		if (j != 0)
			{
			if (!BN_mod_mul_reciprocal(r,r,pc[j],m,d,nb,ctx))
				goto err;
			}
#ifdef BN_FIXED_TIME
		else
			{
			if (!BN_mod_mul_reciprocal(pc[0],r,pc[5],m,d,nb,ctx))
				goto err;
			}
#endif
		}
	ret=1;
err:
	if (c2 != NULL) BN_CTX_free(c2);
	ctx->tos-=3;
	return(ret);
	}
#endif

int BN_mod_mul_reciprocal(BIGNUM *r, BIGNUM *x, BIGNUM *y, BIGNUM *m, BIGNUM *i, int nb, BN_CTX *ctx)
	{
	int ret=0,j;
	BIGNUM *a,*b,*c,*d;

	a=ctx->bn[ctx->tos++];
	b=ctx->bn[ctx->tos++];
	c=ctx->bn[ctx->tos++];
	d=ctx->bn[ctx->tos++];

	if (x == y)
		{ if (!BN_sqr(a,x,ctx)) goto err; }
	else
		{ if (!BN_mul(a,x,y)) goto err; }
	if (!BN_rshift(d,a,nb-1)) goto err;
	if (!BN_mul(b,d,i)) goto err;
	if (!BN_rshift(c,b,nb-1)) goto err;
	if (!BN_mul(b,m,c)) goto err;
	if (!BN_sub(r,a,b)) goto err;
	j=0;
	while (BN_cmp(r,m) >= 0)
		{
		if (j++ > 2)
			{
			goto err;
			}
		if (!BN_sub(r,r,m)) goto err;
		}

	ret=1;
err:
	ctx->tos-=4;
	return(ret);
	}

int BN_reciprocal(BIGNUM *r, BIGNUM *m, BN_CTX *ctx)
	{
	int nm,ret=-1;
	BIGNUM *t;

	t=ctx->bn[ctx->tos++];

	nm=BN_num_bits(m);
	if (!BN_lshift(t,BN_value_one,nm*2)) goto err;

	if (!BN_div(r,NULL,t,m,ctx)) goto err;
	ret=(nm+1);
err:
	ctx->tos--;
	return(ret);
	}

