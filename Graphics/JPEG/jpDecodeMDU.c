// ===========================================================================
//	jpDecodeMDU.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#include "jpIncludes.h"

// ===========================================================================
//	globals

uchar ZZ[kMCUSize] = {	
	 0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};

// ===========================================================================
//	local functions
static long 
DecodeComponentPro(JPEGDecoder *j, int count,CompSpec *cspec, short *blocks);



//====================================================================
//	Read a group of bits from the stream
//	Only used inside a scan, so we can go to town with stripping 0xFF00 stuffing
//	NOTE:	This is the only part of the code that uses VLC

#define	bmask(_bc)	((1<<(_bc))-1)

#define get_bits(_x) (((_x) <= j->bits) ? (j->last32 >> (j->bits -= (_x))) & bmask(_x) :  GetBitsRead(j,(_x)))
#define next_bits(_x) (((_x) <= j->bits) ? (j->last32 >> (j->bits - (_x))) & bmask(_x) : NextBitsRead(j,(_x)))

#define next32bits() ((32 <= j->bits) ? (j->last32 >> (j->bits - 32))  : NextBitsRead(j,32))

#define skip_bits(_x)	if ((j->bits -= _x) < 0) {			\
							j->bits += 32;					\
							j->last32 = j->next32;			\
							j->next32 =  *(long *)j->data;	\
							j->data += 4;					\
						}




//#define	REG_HUFF_DEC
// register based for speed -- need to check compiler output
#ifdef	REG_HUFF_DEC

#define l_get_bits(_x)	\
if ( (_x) <= bits )					\
	bits_result = (last32 >> (bits -= (_x))) & bmask(_x);		\
else 	{	\
	tcount = (_x) - bits;			\
	bits_result = (last32 & bmask(bits)) << tcount;					\
	last32 = next32;  next32 = *data++;	bits = 32 - tcount;					\
	bits_result |= (last32 >> bits) & bmask(tcount);			\
}


#define l_next_bits(_x) 	\
if  ( (_x) <= bits ) 	\
	bits_result = (last32 >> (bits - (_x))) & bmask(_x);	\
else {		\
	tcount = (_x) - bits;	\
	bits_result = ((last32 & bmask(bits)) << tcount)  | (next32 >> (32 - tcount))  & bmask(tcount);	\
}						

#define l_skip_bits(_x)			\
if ((bits -= _x) < 0) {			\
	bits += 32;					\
	last32 = next32;			\
	next32 = *data++;			\
}

#else
	


	
#define l_next_bits(_x) 	bits_result = next_bits(_x)

#define l_get_bits(_x)	bits_result = get_bits(_x)

#define	l_skip_bits	skip_bits

#endif


static inline long
GetBitsRead(register JPEGDecoder *j,register  long count)
{
	register long	a,b;
	register long bits = j->bits;
	count -= bits;
	if ( count == 32 )
		a = 0;
	else
		a = (j->last32 & bmask(bits) ) << count;
	b = j->last32 = j->next32;
	j->next32 = *(long *)j->data;
	j->data += 4;
	j->bits = 32 - count;
	if ( count )
		return a | ( b >> j->bits) & bmask(count);
	else
		return a;
}

static inline long
NextBitsRead(register JPEGDecoder *j,register long count)
{
	count -= j->bits;
	if ( count == 32 )
		return	((j->last32 & bmask(j->bits))) | j->next32;
	else if ( count )
		return	((j->last32 & bmask(j->bits)) << count) | (j->next32 >> (32 - count)) & bmask(count);
	else
		return	(j->last32 & bmask(j->bits));
	
}



static short	
HuffmanDecode(JPEGDecoder *j,register HuffmanTable *h,ushort &run)
{
	register ushort	code;
	register short	l;
	register uchar	s;	
	register FastRec *f;
	register ulong bitbuffer;
	register int bitcount = 32;
	short	v;

	bitbuffer = next32bits();
	code = (bitbuffer >> 24) & bmask(8);
	f = h->fast + code;
	l = f->length;

	if (l > 0) 
	{			// Got a whole extended value
		bitcount -= l;
		run = f->run;
		v = f->value;
	}
	else if (l < 0) 
	{			// Got a symbol
		run = f->run;
		s = f->value;
		bitcount -= s-l;
		l = (bitbuffer >> bitcount) & bmask(s);
		v = Extend(l,s);
	} 
	else 
	{
		bitcount -= 8;
		l = 8;
		while (code > h->maxcode[l]) 
		{
			code <<= 1;
			bitcount--;
			code += (bitbuffer >> bitcount) & bmask(1);
			l++;
		}
		if (l > 16) 
		{		// bad huffman code
			run = 64;
			v = -1;
			Message(("Invalid Huffman code"));
			goto done;
		}
		s = h->huffval[ h->valptr[l] + (code - h->mincode[l] ) ];
		run = s >> 4;
		v = s & 0xf;
		if ( v ) 
		{
			bitcount -= v;
			code = (bitbuffer >> bitcount) & bmask(v);
			v = Extend(code,v);
		}
	}
done:
	skip_bits(32-bitcount);
	return v;
}


static long 
DecodeComponentPro(JPEGDecoder *j, int count,CompSpec *cspec, short *blocks)
{

	int				k,b;
	short			v;
	ushort			run;
	uchar			s;
	long			l;
	ushort			code;
	HuffmanTable 	*h = cspec->ACH;
	short	dcSAVal		 = 1<<cspec->dcBitPos;
	short	acSAVal		 = 1<<cspec->acBitPos;

	if ( blocks == nil )
		blocks = j->blocks;

	for (b = 0; b < count; b++ ) 
	{
		// Progressive JPEG DC 
		if ( cspec->specStart == 0 ) 
		{
			// first or only scan 
			if ( cspec->dcPrevBitPos == 0 ) 
			{
				v = HuffmanDecode(j,cspec->DCH,run);
				cspec->DC += v;
				blocks[0] = cspec->DC << cspec->dcBitPos;
			} 
			else 
			{
				// successive approximation refinement scan
				if ( get_bits(1) ) 
					cspec->DC += dcSAVal;
			}
		}
		
		// Progressive JPEG AC 
		
		else 
		{ 

			// first or only scan for this band
			
			if ( cspec->acPrevBitPos == 0 ) 
			{
				if ( j->bandSkip > 0 ) 
					j->bandSkip--;
				else 
				{
					for (k = cspec->specStart; k <= cspec->specEnd; k++) 
					{
						// decode the next huffman code
						
						v = HuffmanDecode(j,cspec->ACH,run);

						// process the decoded value
						
						if (v) {
							// run of zeros followed by a value
							k += run;
							if ( k > cspec->specEnd ) 
							{
								Message(("Coeff past end of band"));
								j->error = kBadHuffmanErr;
								return -1;
							}
							blocks[ZZ[k]] = v << cspec->acBitPos;
						} else  {
							if ( run == 15 ) {
								// run of zeros followed by zero
								k += run;
								if ( k > cspec->specEnd ) 
								{
									Message(("Coeff run past end of band"));
									j->error = kBadHuffmanErr;
									return -1;
								}
							}
							else 
							{	
								// is not a run but a end of band code
								j->bandSkip = (1<<run);
								if ( run ) 
									j->bandSkip += get_bits(run);
								j->bandSkip--;
								break;
							}
						}
					}
				}
			} else 
			{

				register short *zcoeffp;
				register short zcoef;
			
				// successive approximation refinement scan for this band
				
				k = cspec->specStart;
				if ( j->bandSkip == 0 ) 
				{
					for (k = cspec->specStart; k <= cspec->specEnd; k++) 
					{
					
						long bitcount = 16;
						ulong bitbuffer = next_bits(16);
						code = (bitbuffer >> 8) & bmask(8);
						FastRec *f = h->fast + code;
						l = f->length;
						if (l > 0) 
						{			// Got a whole extended value
							bitcount -= l;
							run = f->run;
							v = f->value;
							if ( v != 0 ) {
								if ( v == Extend(1,1) )
									v = acSAVal;
								else 
									v = -acSAVal;
							}
						}
						else if (l < 0) 
						{			// Got a symbol
							run = f->run;
							s = f->value;
							bitcount -= s-l;
							if ( s == 1 )  {
								if ( (bitbuffer >> bitcount) & bmask(1) )
									v = acSAVal;
								else
									v = -acSAVal;
							}
							else 
								v = 0;
						}
						else 
						{
							l = 8;
							bitcount -= l;
							while (code > h->maxcode[l]) 
							{
								code <<= 1;
								bitcount--;
								code += (bitbuffer >> bitcount) & bmask(1);
								l++;
							}
							if (l > 16) 
							{
								Message(("Bad AC Successive Approximation Huffman code"));
								j->error = kBadHuffmanErr;
								return -1;
							}
							s = h->huffval[ h->valptr[l] + (code - h->mincode[l] ) ];
							run = s >> 4;				
							s &= 0xf;
							if ( s != 0  ) 	
							{
								if ( s != 1 ) 
								{
									j->error = kBadHuffmanErr;
									Message(("Bad AC Successive Approximation Correction Huffman code"));
									return -1;
								}
								bitcount--;
								if ( (bitbuffer >> bitcount) & bmask(1) )
									v = acSAVal;
								else
									v = -acSAVal;
							} else
								v = 0;
						}
						skip_bits(16-bitcount);
						if ( v == 0 )  
						{
							if ( run != 15 ) {
								j->bandSkip = (1<<run);
								if ( run ) 
									j->bandSkip += get_bits(run);
								break;
							}
						}
					
						// look for the next zero-history coefficient
						//
						// if run is non zero, skip that many zero-history coef
						//      before stopping
						// for all nonzero history coefficients passed over
						//		read correction bits from the stream
						
						if ( run > (cspec->specEnd-k)+1 ) 
						{
							j->error = kBadHuffmanErr;
							Message(("Bad AC Successive Approximation run length code %d > %d-%d+1",run,cspec->specEnd,k));
							return -1;
						}
						
						while ( k <= cspec->specEnd ) 
						{
							zcoeffp = blocks+ZZ[k];
							zcoef = *zcoeffp;
	
							if ( zcoef  ) 
							{
								// has history, read correction bit from stream
								if ( get_bits(1) ) 
								{		// needs correction 
									if ( zcoef & acSAVal ) 
										*zcoeffp = zcoef + ( zcoef >= 0 ? acSAVal : -acSAVal);
								}
							} else 
							{
								// end after we get to the first non-zero history
								// coefficient after the run is exhausted
								if ( run-- == 0 )
									break;
							}
							k++;
						}
						if ( v ) 
							blocks[ZZ[k]] = v;
					}
				}
			
				// continue processing correction bits till end of band
			
				if ( j->bandSkip > 0 ) 
				{
					for (; k <= cspec->specEnd; k++) 
					{
						zcoeffp = blocks + ZZ[k];

						if ( (zcoef = *zcoeffp) != 0  ) 
						{
							if ( get_bits(1) )
							{
								if ( zcoef & acSAVal ) 
									*zcoeffp = zcoef + ( zcoef >= 0 ? acSAVal : -acSAVal);
							}
						}
					}
					j->bandSkip--;
				}
			}
		}
		if (j->error)
			return -1;
		if ( blocks )
			blocks += kMCUSize;
	}
	return 0;
}
		


long 
DecodeComponent(JPEGDecoder *j, short count,CompSpec *cspec, short *blocks)
{
	// progressive JPEG huffman decoding

	if ( j->isProgressive ) 
	{
		return DecodeComponentPro(j,count,cspec,blocks);
	} else 
	{
		register int	k,b;
		register long	v;
		ushort	run;

		if ( blocks == nil ) 
		{
			
			for (b = 0; b < count; b++, blocks += kMCUSize ) 
			{
			
				// F.2.2.1 Huffman decoding of DC coefficients

				v = HuffmanDecode(j,cspec->DCH,run);
				cspec->DC += v;
							  
				//	F.2.2.1 Huffman decoding of AC coefficients
			  
				for (k = 1; k < kMCUSize; k++) 
				{
					v = HuffmanDecode(j,cspec->ACH,run);
						k += run;
					if ( (v|run) == 0 ) 
						break;
				}
				if ( k > kMCUSize ) 
					goto fail;
			}
		} else 
		{
			// sequential JPEG huffman decode

			register ushort *QTable = cspec->QTable;
		
			for (b = 0; b < count; b++, blocks += kMCUSize ) 
			{
			
		//		F.2.2.1 Huffman decoding of DC coefficients
			
				v = HuffmanDecode(j,cspec->DCH,run);
				if ( run ) 
					goto fail;
				cspec->DC += v;
				blocks[0] = cspec->DC * QTable[0];
			  
		//		F.2.2.1 Huffman decoding of AC coefficients
			  
				for (k = 1; k < kMCUSize; k++) 
				{
					v = HuffmanDecode(j,cspec->ACH,run);
						k += run;
					if (v) 
					{
						if ( k > kMCUSize )		// dont trash memory if we get bad data
							break;
						blocks[ZZ[k]] = v * QTable[k];
					} 
					else 
					{
						if ( run == 0 ) 		// zero = end of block
							break;
					}
				}
				if ( k > kMCUSize ) 
					goto fail;
			}

#ifdef	UNDITHER_JPEG
			j->ditherEnergy += UnDither(blocks - (count << 6),count);
#endif
		}
		return 0;

fail:
		j->error = kBadHuffmanErr;
		Message(("Bad Huffman code"));
		return -1;
	}
}




#ifdef	UNDITHER_JPEG


//	Simple Spectral selection to remove dither, should use a median cut on the spatial image

char ud[] = {
	0,1,2,3,4,5,6,7,
	1,1,2,3,4,5,6,7,
	2,2,3,4,5,6,7,8,
	3,3,4,4,5,6,7,8,
	4,4,5,5,6,7,8,9,
	5,5,6,6,7,7,8,9,
	6,6,7,7,8,8,9,10,
	7,7,8,8,9,9,10,10
};

long UnDither(short *blocks, short count)
{
	short	*c = blocks,e,k,b;
	long	result=0;
	
	for (b = e = 0; b < count; b++) {
		e = c[63] < 0 ? -c[63] : c[63];
		result += e;
		if (e > 4)
			for (k = 1; k < kMCUSize; k++)
				if (ud[k] > 5)
					c[k] = 0;
		c += kMCUSize;
	}
	return result;
}

#endif


//====================================================================
