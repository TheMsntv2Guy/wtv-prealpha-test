/* lib/rsa/rsa_enc.c */
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

#ifndef RSAref

#include "crypto.h"
#include "bn.h"
#include "md5.h"
#include "rsa.h"
#include "rand.h"
#include "MemoryManager.h"

int RSA_public_encrypt(int flen, unsigned char *from, unsigned char *to, RSA *rsa)
	{
	BIGNUM *f=NULL,*ret=NULL;
	int i,j,k,num=0,r=-1;
	unsigned char *p;
	unsigned char *buf=NULL;
	BN_CTX *ctx;

	ctx=BN_CTX_new();
	if (ctx == NULL) goto err;

	num=BN_num_bytes(rsa->n);
	if (flen > (num-11))
		{
		goto err;
		}
	
	buf=(unsigned char *)AllocateMemory(num);
	if (buf == NULL)
		{
		goto err;
		}
	p=(unsigned char *)buf;

	*(p++)=0;
	*(p++)=2; /* Public Key BT (Block Type) */

	/* pad out with non-zero random data */
	j=num-3-flen;
	RAND_bytes(p,j);
	for (i=0; i<j; i++)
		{
		if (*p == '\0')
			do	{
				RAND_bytes(p,1);
				} while (*p == '\0');
		p++;
		}
	*(p++)='\0';
	memcpy(p,from,(unsigned int)flen);

	f=BN_new();
	ret=BN_new();
	if ((f == NULL) || (ret == NULL)) goto err;

	if (BN_bin2bn(buf,num,f) == NULL) goto err;
	if (!BN_mod_exp(ret,f,rsa->e,rsa->n,ctx)) goto err;

	/* put in leading 0 bytes if the number is less than the
	 * length of the modulus */
	j=BN_num_bytes(ret);
	i=BN_bn2bin(ret,&(to[num-j]));
	for (k=0; k<(num-i); k++)
		to[k]=0;

	r=num;
err:
	if (ctx != NULL) BN_CTX_free(ctx);
	if (f != NULL) BN_free(f);
	if (ret != NULL) BN_free(ret);
	if (buf != NULL) 
		{
		memset(buf,0,num);
		FreeMemory(buf);
		}
	return(r);
	}

int RSA_private_encrypt(int flen, unsigned char *from, unsigned char *to, RSA *rsa)
	{
	BIGNUM *f=NULL,*ret=NULL;
	int i,j,k,num=0,r=-1;
	unsigned char *p;
	unsigned char *buf=NULL;
	BN_CTX *ctx;

	ctx=BN_CTX_new();
	if (ctx == NULL) goto err;

	num=BN_num_bytes(rsa->n);
	if (flen > (num-11))
		{
		goto err;
		}
	buf=(unsigned char *)AllocateMemory(num);
	if (buf == NULL)
		{
		goto err;
		}
	p=buf;

	*(p++)=0;
	*(p++)=1; /* Private Key BT (Block Type) */

	/* padd out with 0xff data */
	j=num-3-flen;
	for (i=0; i<j; i++)
		*(p++)=0xff;
	*(p++)='\0';
	memcpy(p,from,(unsigned int)flen);
	ret=BN_new();
	f=BN_new();
	if ((ret == NULL) || (f == NULL)) goto err;
	if (BN_bin2bn(buf,num,f) == NULL) goto err;
	if (	(rsa->p != NULL) &&
		(rsa->q != NULL) &&
		(rsa->dmp1 != NULL) &&
		(rsa->dmq1 != NULL) &&
		(rsa->iqmp != NULL))
		{ if (!RSA_mod_exp(ret,f,rsa)) goto err; }
	else
		{ if (!BN_mod_exp(ret,f,rsa->d,rsa->n,ctx)) goto err; }

	p=buf;
	i=BN_bn2bin(ret,p);

	/* put in leading 0 bytes if the number is less than the
	 * length of the modulus */
	j=BN_num_bytes(ret);
	i=BN_bn2bin(ret,&(to[num-j]));
	for (k=0; k<(num-i); k++)
		to[k]=0;

	r=num;
err:
	if (ctx != NULL) BN_CTX_free(ctx);
	if (ret != NULL) BN_free(ret);
	if (f != NULL) BN_free(f);
	if (buf != NULL)
		{
		memset(buf,0,num);
		FreeMemory(buf);
		}
	return(r);
	}

int RSA_private_decrypt(int flen, unsigned char *from, unsigned char *to, RSA *rsa)
	{
	BIGNUM *f=NULL,*ret=NULL;
	int i,j,num=0,r=-1;
	unsigned char *p;
	unsigned char *buf=NULL;
	BN_CTX *ctx;

	ctx=BN_CTX_new();
	if (ctx == NULL) goto err;

	num=BN_num_bytes(rsa->n);

	buf=(unsigned char *)AllocateMemory(num);
	if (buf == NULL)
		{
		goto err;
		}

	if (flen != num)
		{
		goto err;
		}

	/* make data into a big number */
	ret=BN_new();
	f=BN_new();
	if ((ret == NULL) || (f == NULL)) goto err;
	if (BN_bin2bn(from,(int)flen,f) == NULL) goto err;
	/* do the decrypt */
	if (	(rsa->p != NULL) &&
		(rsa->q != NULL) &&
		(rsa->dmp1 != NULL) &&
		(rsa->dmq1 != NULL) &&
		(rsa->iqmp != NULL))
		{ if (!RSA_mod_exp(ret,f,rsa)) goto err; }
	else
		{ if (!BN_mod_exp(ret,f,rsa->d,rsa->n,ctx)) goto err; }

	p=buf;
	i=BN_bn2bin(ret,p);

	/* BT must be 02 */
	if (*(p++) != 02)
		{
		goto err;
		}

	/* scan over padding data */
	j=num-2; /* one for type and one for the prepended 0. */
	for (i=0; i<j; i++)
		if (*(p++) == 0) break;
	if (i == j)
		{
		goto err;
		}
	if (i < 8)
		{
		goto err;
		}

	/* skip over the '\0' */
	i++;	
	j-=i;

	/* output data */
	memcpy(to,p,(unsigned int)j);
	r=j;
err:
	if (ctx != NULL) BN_CTX_free(ctx);
	if (f != NULL) BN_free(f);
	if (ret != NULL) BN_free(ret);
	if (buf != NULL)
		{
		memset(buf,0,num);
		FreeMemory(buf);
		}
	return(r);
	}

int RSA_public_decrypt(int flen, unsigned char *from, unsigned char *to, RSA *rsa)
	{
	BIGNUM *f=NULL,*ret=NULL;
	int i,j,num=0,r=-1;
	unsigned char *p;
	unsigned char *buf=NULL;
	BN_CTX *ctx;

	ctx=BN_CTX_new();
	if (ctx == NULL) goto err;

	num=BN_num_bytes(rsa->n);
	buf=(unsigned char *)AllocateMemory(num);
	if (buf == NULL)
		{
		goto err;
		}

	if (flen != num)
		{
		goto err;
		}

	/* make data into a big number */
	f=BN_new();
	ret=BN_new();
	if ((f == NULL) || (ret == NULL)) goto err;

	if (BN_bin2bn(from,flen,f) == NULL) goto err;
	/* do the decrypt */
	if (!BN_mod_exp(ret,f,rsa->e,rsa->n,ctx)) goto err;

	p=buf;
	i=BN_bn2bin(ret,p);

	/* BT must be 01 */
	if (*(p++) != 01)
		{
		goto err;
		}

	/* scan over padding data */
	j=num-2; /* one for type and one for the prepended 0. */
	for (i=0; i<j; i++)
		{
		if (*p != 0xff) /* should decrypt to 0xff */
			{
			if (*p == 0)
				{ p++; break; }
			else	{
				goto err;
				}
			}
		p++;
		}
	if (i == j)
		{
		goto err;
		}
	if (i < 8)
		{
		goto err;
		}

	/* skip over the '\0' */
	i++;	
	j-=i;

	/* output data */
	memcpy(to,p,(unsigned int)j);
	r=j;
err:
	if (ctx != NULL) BN_CTX_free(ctx);
	if (f != NULL) BN_free(f);
	if (ret != NULL) BN_free(ret);
	if (buf != NULL)
		{
		memset(buf,0,num);
		FreeMemory(buf);
		}
	return(r);
	}

int RSA_mod_exp(BIGNUM *r0, BIGNUM *I, RSA *rsa)
	{
	BIGNUM *r1=NULL,*m1=NULL;
	int ret=0;
	BN_CTX *ctx;

	if ((ctx=BN_CTX_new()) == NULL) goto err;
	m1=BN_new();
	r1=BN_new();
	if ((m1 == NULL) || (r1 == NULL)) goto err;

	if (!BN_mod(r1,I,rsa->q,ctx)) goto err;
	if (!BN_mod_exp(m1,r1,rsa->dmq1,rsa->q,ctx)) goto err;

	if (!BN_mod(r1,I,rsa->p,ctx)) goto err;
	if (!BN_mod_exp(r0,r1,rsa->dmp1,rsa->p,ctx)) goto err;

	if (!BN_add(r1,r0,rsa->p)) goto err;
	if (!BN_sub(r0,r1,m1)) goto err;

	if (!BN_mul(r1,r0,rsa->iqmp)) goto err;
	if (!BN_mod(r0,r1,rsa->p,ctx)) goto err;
	if (!BN_mul(r1,r0,rsa->q)) goto err;
	if (!BN_add(r0,r1,m1)) goto err;
	
	ret=1;
err:
	if (m1 != NULL) BN_free(m1);
	if (r1 != NULL) BN_free(r1);
	BN_CTX_free(ctx);
	return(ret);
	}

#else
#include "../../rsaref/rsaref.c"
#endif

