/* lib/rand/md5_rand.c */
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
#include "md5.h"
#include "rand.h"

#include "BoxUtils.h"

#include "Utilities.h"

/* Changed how the state buffer used.  I now attempt to 'wrap' such
 * that I don't run over the same locations the next time  go through
 * the 1023 bytes - many thanks to
 * Robert J. LeBlanc <rjl@renaissoft.com> for his comments
 */

/*#define NORAND	1 */
/*#define PREDICT	1 */

#define STATE_SIZE	1023
static int state_num=0,state_index=0;
static unsigned char state[STATE_SIZE];
static unsigned char md[MD5_DIGEST_LENGTH];
static int count=0;

char *RAND_version="\0RAND part of SSLeay v 0.5.1b 23/02/96";

void RAND_cleanup()
	{
	memset(state,0,STATE_SIZE);
	state_num=0;
	state_index=0;
	memset(md,0,MD5_DIGEST_LENGTH);
	count=0;
	}

void RAND_seed(unsigned char *buf,int num)
	{
	int i,j,k;
	MD5_CTX m;

#ifdef NORAND
	return;
#endif

	for (i=0; i<num; i+=MD5_DIGEST_LENGTH)
		{
		j=(num-i);
		j=(j > MD5_DIGEST_LENGTH)?MD5_DIGEST_LENGTH:j;

		MD5_Init(&m);
		MD5_Update(&m,md,MD5_DIGEST_LENGTH);
		k=(state_index+j)-STATE_SIZE;
		if (k > 0)
			{
			MD5_Update(&m,&(state[state_index]),j-k);
			MD5_Update(&m,&(state[0]),k);
			}
		else
			MD5_Update(&m,&(state[state_index]),j);
			
		MD5_Update(&m,buf,j);
		MD5_Final(md,&m);

		buf+=j;

		for (k=0; k<j; k++)
			{
			state[state_index++]^=md[k];
			if (state_index >= STATE_SIZE)
				{
				state_index=0;
				state_num=STATE_SIZE;
				}
			}
		}
	if (state_index > state_num)
		state_num=state_index;
	memset((char *)&m,0,sizeof(m));
	}

void RAND_bytes(unsigned char *buf,int num)
	{
	int i,j,k;
	MD5_CTX m;
	static int init=1;
	unsigned long l;

#ifdef PREDICT
	{
	static unsigned char val=0;

	for (i=0; i<num; i++)
		buf[i]=val++;
	return;
	}
#endif

	if (init)
		{
		init=0;
		/* put in some default random data, we need more than
		 * just this */
		RAND_seed((unsigned char *)&m,sizeof(m));
		l=GetRandom();
		RAND_seed((unsigned char *)&l,sizeof(l));
		l=Now();
		RAND_seed((unsigned char *)&l,sizeof(l));
		l=GetRandom();
		RAND_seed((unsigned char *)&l,sizeof(l));

#ifdef PURIFY
		memset(state,0,STATE_SIZE);
		memset(md,0,MD5_DIGEST_LENGTH);
#endif
		}

	while (num > 0)
		{
		j=(num >= MD5_DIGEST_LENGTH/2)?MD5_DIGEST_LENGTH/2:num;
		num-=j;
		MD5_Init(&m);
		MD5_Update(&m,&(md[MD5_DIGEST_LENGTH/2]),MD5_DIGEST_LENGTH/2);
#ifndef PURIFY
		MD5_Update(&m,buf,j); /* purify complains */
#endif
		k=(state_index+j)-state_num;
		if (k > 0)
			{
			MD5_Update(&m,&(state[state_index]),j-k);
			MD5_Update(&m,&(state[0]),k);
			}
		else
			MD5_Update(&m,&(state[state_index]),j);
		MD5_Final(md,&m);

		for (i=0; i<j; i++)
			{
			if (state_index >= state_num)
				state_index=0;
			state[state_index++]^=md[i];
			*(buf++)=md[i+MD5_DIGEST_LENGTH/2];
			}
		}

	MD5_Init(&m);
	MD5_Update(&m,(unsigned char *)&count,sizeof(count)); count++;
	MD5_Update(&m,md,MD5_DIGEST_LENGTH);
	MD5_Final(md,&m);
	}

