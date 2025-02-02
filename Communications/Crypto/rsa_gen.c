/* lib/rsa/rsa_gen.c */
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
#include "rsa.h"

RSA *RSA_generate_key(int bits, unsigned long e_value, void (*callback)(int, int))
	{
	RSA *rsa=NULL;
	BIGNUM *r0=NULL,*r1=NULL,*r2=NULL,*r3=NULL,*tmp;
	int bitsp,bitsq,ok=-1,n=0;
	BN_CTX *ctx=NULL,*ctx2=NULL;

	ctx=BN_CTX_new();
	if (ctx == NULL) goto err;
	ctx2=BN_CTX_new();
	if (ctx2 == NULL) goto err;
	r0=ctx->bn[0];
	r1=ctx->bn[1];
	r2=ctx->bn[2];
	r3=ctx->bn[3];
	ctx->tos+=4;

	bitsp=(bits+1)/2;
	bitsq=bits-bitsp;
	rsa=RSA_new();
	if (rsa == NULL) goto err;

	/* set e */ 
	rsa->e=BN_new();
	if (rsa->e == NULL) goto err;
	if (!BN_set_word(rsa->e,e_value)) goto err;

	/* generate p and q */
	for (;;)
		{
		rsa->p=BN_generate_prime(bitsp,0,NULL,NULL,callback);
		if (rsa->p == NULL) goto err;
		if (!BN_sub(r2,rsa->p,BN_value_one)) goto err;
		if (!BN_gcd(r1,r2,rsa->e,ctx)) goto err;
		if (BN_is_one(r1)) break;
		if (callback != NULL) callback(2,n++);
		BN_free(rsa->p);
		}
	if (callback != NULL) callback(3,0);
	for (;;)
		{
		rsa->q=BN_generate_prime(bitsq,0,NULL,NULL,callback);
		if (rsa->q == NULL) goto err;
		if (!BN_sub(r2,rsa->q,BN_value_one)) goto err;
		if (!BN_gcd(r1,r2,rsa->e,ctx)) goto err;
		if (BN_is_one(r1) && (BN_cmp(rsa->p,rsa->q) != 0))
			break;
		if (callback != NULL) callback(2,n++);
		BN_free(rsa->q);
		}
	if (callback != NULL) callback(3,1);
	if (BN_cmp(rsa->p,rsa->q) < 0)
		{
		tmp=rsa->p;
		rsa->p=rsa->q;
		rsa->q=tmp;
		}

	/* calculate n */
	rsa->n=BN_new();
	if (rsa->n == NULL) goto err;
	if (!BN_mul(rsa->n,rsa->p,rsa->q)) goto err;

	/* calculate d */
	if (!BN_sub(r1,rsa->p,BN_value_one)) goto err;	/* p-1 */
	if (!BN_sub(r2,rsa->q,BN_value_one)) goto err;	/* q-1 */
	if (!BN_mul(r0,r1,r2)) goto err;	/* (p-1)(q-1) */

/* should not be needed, since gcd(p-1,e) == 1 and gcd(q-1,e) == 1 */
/*	for (;;)
		{
		if (!BN_gcd(r3,r0,rsa->e,ctx)) goto err;
		if (BN_is_one(r3)) break;

		if (1)
			{
			if (!BN_add_word(rsa->e,2L)) goto err;
			continue;
			}
		RSAerr(RSA_F_RSA_GENERATE_KEY,RSA_R_BAD_E_VALUE);
		goto err;
		}
*/
	rsa->d=(BIGNUM *)BN_mod_inverse(rsa->e,r0,ctx2);	/* d */
	if (rsa->d == NULL) goto err;

	/* calculate d mod (p-1) */
	rsa->dmp1=BN_new();
	if (rsa->dmp1 == NULL) goto err;
	if (!BN_mod(rsa->dmp1,rsa->d,r1,ctx)) goto err;

	/* calculate d mod (q-1) */
	rsa->dmq1=BN_new();
	if (rsa->dmq1 == NULL) goto err;
	if (!BN_mod(rsa->dmq1,rsa->d,r2,ctx)) goto err;

	/* calculate inverse of q mod p */
	rsa->iqmp=BN_mod_inverse(rsa->q,rsa->p,ctx2);
	if (rsa->iqmp == NULL) goto err;

	ok=1;
err:
	if (ok == -1)
		{
		ok=0;
		}
	BN_CTX_free(ctx);
	BN_CTX_free(ctx2);
	
	if (!ok)
		{
		if (rsa != NULL) RSA_free(rsa);
		return(NULL);
		}
	else
		return(rsa);
	}

