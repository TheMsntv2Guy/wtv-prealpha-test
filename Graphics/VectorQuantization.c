

#ifndef FIDO_STANDALONE
	#include	"VectorQuantization.h"
#else
	#include "fidoDebug.h"
	#include "vqAdapt.h"
	
	typedef unsigned long ulong;
	typedef unsigned short ushort;
	typedef unsigned char uchar;
	#define PostulateFinal(a)
	#include <stdio.h>
	#define AllocateTaggedMemory(a, b) new char[a]
	#define FreeTaggedMemory(a, b) delete a
	#define ZeroMemory(a,b) memset(a, 0, b)
	#define TrivialMessage(a) // printf a 
	#define Message(a) printf a
#endif
		
static void
CompactCode(vectype *c,ulong *d);

static void
ExpandCode(ulong a,ulong b,vectype *c);

static inline void CopyVector(const vectype *src,vectype *dst);


static inline void ZeroVector(vectype *v)
{

#if	(kVectorTypeSize == 2) && (kCodeSize == 12)
	ulong *p = (ulong *)v;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;
#else
	ZeroMemory(v,sizeof(vectype) * kCodeSize);
#endif
}


static inline void CopyVector(const vectype *src,vectype *dst)
{
#if	(kVectorTypeSize == 2) && (kCodeSize == 12)
	ulong *d = (ulong *)dst;
	const ulong *s = (ulong *)src;
	*d++ = *s++;
	*d++ = *s++;
	*d++ = *s++;
	*d++ = *s++;
	*d++ = *s++;
	*d++ = *s++;
#else	
	int i;
	for ( i=0; i < kCodeSize; i++)
		*dst++ = *src++;
#endif
}


VectorQuantizer::VectorQuantizer()
{

}


VectorQuantizer::~VectorQuantizer()
{
	Reset(false);
}

void
VectorQuantizer::Reset(Boolean keepBitmap)
{

	if ( fBitMap && !keepBitmap ) {
		DeleteBitMapDevice(fBitMap);
		fBitMap = nil;
	}
	if ( fTempCodeBook )
		FreeTaggedMemory(fTempCodeBook,"VQ Codebook");
	fTempCodeBook = nil;
	if ( fCodeBook )
		FreeTaggedMemory(fCodeBook,"VQ Codebook");
	fCodeBook = nil;
	if ( fSourceVectors )
		FreeTaggedMemory(fSourceVectors,"VQ Vectors");
	fSourceVectors = nil;
	if ( fMatchError )
		FreeTaggedMemory(fMatchError,"VQ Error");
	fMatchError = nil;
	if ( fMatchCount )
		FreeTaggedMemory(fMatchCount,"VQ Match");
	fMatchCount = nil;
	
	fState = kVQIdle;
}

Error
VectorQuantizer::Start(BitMapDevice *src,Rectangle *srcRect,Rectangle *dstRect,short codebookSize,short iterations)
{

	long	x,y;
	uchar	*sp = src->baseAddress;
	vectype *v;

	if ( fState != kVQIdle )
		Reset(false);

	if ( srcRect == nil )
		srcRect = &src->bounds;
		
	fBitMap = NewBitMapDevice(*srcRect,vqFormat,nil,kNoTransparentColor);
	if ( fBitMap == nil ) {
		fState = kVQError;
		return kLowMemory;
	}
		
	if ( dstRect == nil )
		fDstRect = fBitMap->bounds;
		
	fWidth = srcRect->right - srcRect->left;
	fHeight = srcRect->bottom - srcRect->top;
	if  ( fWidth != fDstRect.right - fDstRect.left ) {
		fState = kVQError;
		return kGenericError;
	}
	if  ( fHeight != fDstRect.bottom - fDstRect.top ) {
		fState = kVQError;
		return kGenericError;
	}
	
	{
		// еее this is alot of stuff to be allocating, it would be nice if we could read the vectors
		// from the source image as we go along compressing it еее

		Error result = kNoError;
		
		fVectorCount = ((fHeight+1)>>1) * ((fWidth+1)>>1);
		fSourceVectors = (vectype *)AllocateTaggedMemory(fVectorCount* kCodeSize * sizeof(vectype),"VQ Vectors");
		if ( fSourceVectors == nil ) {
			result = kLowMemory;
			goto done;
		}

		v = fSourceVectors;
		switch ( src->format ) {
		case yuv422Format:
			const ulong 	*spla,*splb;
			for ( y=0; y < fHeight>>1; y++ ) {
				spla = (const ulong *)sp;
				splb = (const ulong *)(sp+src->rowBytes);
				for ( x=0; x < fWidth>>1; x++ ) {
					ExpandCode(*spla++,*splb++,v);
					v += kCodeSize;
				}
				if ( fWidth & 1 ) {
					ulong a = *(ushort *)spla;
					ulong b = *(ushort *)splb;
					a |= (a<<16);
					b |= (a<<16);
					ExpandCode(a,b,v);
					v += kCodeSize;
				}
				sp += src->rowBytes*2;
			}
			if ( fHeight & 1 ) {
				spla = (ulong *)sp;
				for ( x=0; x < fWidth>>1; x++ ) {
					ulong pix = *spla++; 
					ExpandCode(pix,pix,v);
					v += kCodeSize;
				}
				if ( fWidth & 1 ) {
					ulong a = *(ushort *)spla;
					a |= (a<<16);
					ExpandCode(a,a,v);
					v += kCodeSize;
				}
			}
			break;
		case index8Format: {
				const uchar *ctp,*ctab = GetCLUTYUV(src);
				const uchar	*spa,*spb;
				vectype *vp = v;
				
				if ( src->transparentColor & kTransparent ) {			// cant deal with this yet
					result = kGenericError;
					goto done;
				}
				for ( y=0; y < fHeight>>1; y++ ) {
					spa = sp;
					spb = sp+src->rowBytes;
					for ( x=0; x < fWidth>>1; x++ ) {
						ctp = ctab + *spa++ * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						ctp = ctab + *spa++ * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						ctp = ctab + *spb++ * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						ctp = ctab + *spb++ * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
					}
					if ( fWidth & 1 ) {
						ctp = ctab + *spa * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						ctp = ctab + *spa * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						ctp = ctab + *spb * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						ctp = ctab + *spb * 3;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
						*vp++ = *ctp++;
					}
					sp += src->rowBytes*2;
				}
				if ( fHeight & 1  ) {
					const uchar *cta,*ctb;
					spa = sp;
					for ( x=0; x < fWidth>>1; x++ ) {
						cta = ctab + *spa++ * 3;
						*vp++ = cta[0];
						*vp++ = cta[1];
						*vp++ = cta[2];
						ctb = ctab + *spa++ * 3;
						*vp++ = ctb[0];
						*vp++ = ctb[1];
						*vp++ = ctb[2];
						*vp++ = *cta++;
						*vp++ = *cta++;
						*vp++ = *cta++;
						*vp++ = *ctb++;
						*vp++ = *ctb++;
						*vp++ = *ctb++;
					}
					if ( fWidth & 1 ) {
						cta = ctab + *spa++ * 3;
						*vp++ = cta[0];
						*vp++ = cta[1];
						*vp++ = cta[2];
						*vp++ = cta[0];
						*vp++ = cta[1];
						*vp++ = cta[2];
						*vp++ = cta[0];
						*vp++ = cta[1];
						*vp++ = cta[2];
						*vp++ = cta[0];
						*vp++ = cta[1];
						*vp++ = cta[2];
					}
				}
			}
			break;
		default:
			Message(("Can't compress format %X to vq",src->format));
			result = kGenericError;
			goto done;
		}
		fCodebookSize = codebookSize;
		fCodeBook = (vectype *)AllocateTaggedMemory( fCodebookSize * kCodeSize*sizeof(vectype),"VQ Codebook");
		fMatchError = (ushort *)AllocateTaggedMemory(fCodebookSize*sizeof(ushort),"VQ Error");
		fMatchCount = (ushort *)AllocateTaggedMemory(fCodebookSize*sizeof(ushort),"VQ Match");
		fTempCodeBook = (vectype *)AllocateTaggedMemory(fCodebookSize *kCodeSize*sizeof(vectype),"VQ Codebook");
		
		if ( fCodeBook == nil || fMatchError == nil || fMatchCount == nil || fTempCodeBook == nil ) {
			result = kLowMemory;
			goto done;
		}
		fIterations = iterations;
		fLastError = 0xFFFFFFFF;
		fState = kVQBeginIteration;
		// calucate initial codebook
		
		if (0) {
			ZeroMemory(fCodeBook,fCodebookSize*kCodeSize*sizeof(vectype));
			TrivialMessage(("Initializing codebook to zero"));
		} else {
			vectype *cc = fCodeBook;
			long	j;
			for (j = 0; j < fCodebookSize; j++)	 {
				v = fSourceVectors + (((j*fVectorCount)/fCodebookSize)*kCodeSize);
				CopyVector(v,cc);
				cc += kCodeSize;
			}
			TrivialMessage(("Initializing codebook from image"));
		}
done:
		if ( result ) {
			Reset(true);
			fState = kVQError;
		}
		return result;
	}
	fState = kVQError;
	return kGenericError;

}


extern void 	VerifyMemory();

Boolean	
VectorQuantizer::Continue(Boolean cancel)
{
	long	i;
	
	if ( cancel ) {
		Reset(false);
		return true;
	}
	switch (fState )
	{ 
	case kVQIdle:
		return true;
	case kVQDone:
		return true;
	case kVQError:
		return true;
	case kVQBeginIteration:
		fIterationError = 0;
		ZeroMemory(fMatchCount,fCodebookSize*sizeof(ushort));
		ZeroMemory(fMatchError,fCodebookSize*sizeof(ushort));
		ZeroMemory(fTempCodeBook,fCodebookSize*kCodeSize*sizeof(vectype));
		fState = kVQIterating;
	case kVQIterating:
		if ( !Iterate()  ) {			// partial
			break;
		}
		fIterations--;
		fState = kVQEndIteration;
	case kVQEndIteration:
		if ( fIterationError >= fLastError || fIterations == 0  )  {
			ReduceCodebook();
			if ( fSourceVectors )
				FreeTaggedMemory(fSourceVectors,"VQ Vectors");
			fSourceVectors = nil;
			if ( fTempCodeBook )
				FreeTaggedMemory(fTempCodeBook,"VQ Codebook");
			fTempCodeBook = nil;
			if ( fMatchError )
				FreeTaggedMemory(fMatchError,"VQ Error");
			fMatchError = nil;
			if ( fMatchCount )
				FreeTaggedMemory(fMatchCount,"VQ Match");
			fMatchCount = nil;
			
			if ( fBitMap->colorTable )
				FreeTaggedMemory(fBitMap->colorTable,"Color Table");
			fBitMap->colorTable = (CLUT *)AllocateTaggedMemory(4 + fCodebookSize*sizeof(ulong)*2,"Color Table");
			if ( fBitMap->colorTable == nil )
			{
				Reset(false);
				fState = kVQError;
				break;
			}
			fBitMap->colorTable->version = kYUV64;
			fBitMap->colorTable->size = fCodebookSize*sizeof(ulong)*2;
			fBitMap->format = vqFormat;
			ulong *cb = (ulong *)((char *)fBitMap->colorTable + 4);
			vectype  *vp = fCodeBook;
			for ( i=0; i < fCodebookSize; i++ ) {
				CompactCode(vp,cb);
				cb += 2;
				vp += kCodeSize;
			}
			Reset(true);
			fState = kVQDone;
			return true;
		} else
			fState = kVQBeginIteration;
		fLastError = fIterationError;
		return false;
	}
	return false;
}



BitMapDevice *
VectorQuantizer::GetBitMap(Boolean transferOwnership)
{
	if ( fState != kVQDone )
		return nil;
	BitMapDevice *bm = fBitMap;
	if ( transferOwnership )
		fBitMap = nil;
	return bm;
}


#if	0			// should just use XCopyRect instead

/***********************************************************************


	Decodes a VQFormat bitmap into a normal bitmap ( currently output must be YUV422 format )

*/

Error
ConvertFromVQ(BitMapDevice *src,Rectangle *srcRect,BitMapDevice *dst,Rectangle *dstRect)
{
	long	w,h,x,y;
	long	i;
	ulong 	*dpa,*dpb;
	uchar	*dp = dst->baseAddress;
	uchar	*sp = src->baseAddress;
	ulong	*codeBook,*cbp;
	
	if ( src->format != vqFormat || src->colorTable->version  != kYUV64)
		return kGenericError;

	if ( srcRect == nil )
		srcRect = &src->bounds;
	if ( dstRect == nil )
		dstRect = &dst->bounds;
		
	w = srcRect->right - srcRect->left;
	h = srcRect->bottom - srcRect->top;
	if  ( w != dstRect->right - dstRect->left )
		return kGenericError;
	if  ( h != dstRect->bottom - dstRect->top )
		return kGenericError;
	
	if (  dst->format == yuv422Format )
	{
		long bump =  src->rowBytes - (w>>1);
		codeBook = (ulong *)&src->colorTable->data[0];
		i = 0;
		sp += srcRect->left;
		sp += srcRect->top * src->rowBytes;
		dp += dstRect->left*2;
		dp += dstRect->top * dst->rowBytes;
		for ( y=h>>1; y-- > 0 ; ) {
			dpa = (ulong *)dp;
			dpb = (ulong *)(dp+dst->rowBytes);
			for ( x=w>>1; x-- > 0 ; ) {
				register short code = *sp++;
				cbp = codeBook + (code<<1);
				*dpa++ = *cbp++;
				*dpb++ = *cbp++;
			}
			sp += bump;
			dp += dst->rowBytes<<1;
		}
		return kNoError;
	}
	return kGenericError;
}

#endif



static inline void AccumulateVector(vectype *accum,const vectype *addend)
{
#if	(kVectorTypeSize == 2) && (kCodeSize == 12)

	// assumes unsigned and no overflows
	
	ulong *d = (ulong *)accum;
	ulong *s = (ulong *)addend;
	*d++ += *s++;
	*d++ += *s++;
	*d++ += *s++;
	*d++ += *s++;
	*d++ += *s++;
	*d++ += *s++;
#else	
	int i;
	for ( i=0; i < kCodeSize; i++ ) 
		*accum++ += *addend++;			// Add the vector into the codebook
#endif
}



static  inline ulong
ClipByte(short x);

static  inline ulong
ClipByte(short x)
{
	if ( x < 0 )
		return 0;
	if ( x > 255 )
		return 255;
	return x;
}

/***********************************************************************


	Compress vector to 8 byte FIDO format
	
*/

static void
CompactCode(vectype *c,ulong *d)
{
	ulong p;
	short y1,y2,u,v;
	
	y1 = *c++;
	u = *c++;
	v = *c++;
	y2 = *c++;
	u += *c++;
	v += *c++;
	u /= 2;
	v /= 2;
	
	p =  ClipByte(y1)<<24;
	p |= ClipByte(u)<<16;
	p |= ClipByte(y2)<<8;
	p |= ClipByte(v);
	*d++ = p;
	
	y1 = *c++;
	u = *c++;
	v = *c++;
	y2 = *c++;
	u += *c++;
	v += *c++;
	u /= 2;
	v /= 2;

	p =  ClipByte(y1)<<24;
	p |= ClipByte(u)<<16;
	p |= ClipByte(y2)<<8;
	p |= ClipByte(v);
	*d++ = p;
}


/***********************************************************************


	Expand two YUYV pairs into transient vector format
	
*/

static void
ExpandCode(ulong a,ulong b,vectype *c)
{
	
//#define		VQ_GRAY
#ifdef	VQ_GRAY
	*c++ = a>>24;
	*c++ = 0;
	*c++ = 0;
	*c++ = (a>>8) & 0xff;
	*c++ = 0;
	*c++ = 0;
	*c++ = b>>24;
	*c++ = 0;
	*c++ = 0;
	*c++ = (b>>8) & 0xff;
	*c++ = 0;
	*c++ = 0;
#else
	short u,v;
	u = (vectype)((a>>16) & 0xff);
	v = (vectype)((a) & 0xff);
	*c++ = a>>24;
	*c++ = u;
	*c++ = v;
	*c++ = (a>>8) & 0xff;
	*c++ = u;
	*c++ = v;
	
	u = (vectype)((b>>16) & 0xff);
	v = (vectype)((b) & 0xff);
	*c++ = b>>24;
	*c++ = u;
	*c++ = v;
	*c++ = (b>>8) & 0xff;
	*c++ = u;
	*c++ = v;
#endif
}




/***********************************************************************


	Find bestmatching vector in codebook
	
*/

const	kMaxMatch	= 200;			// needs to be such that (kMaxMatch * 255) < maximum vectype

inline short	
VectorQuantizer::MatchVectorToCodeBook(const vectype *vector)
{
	register short	e,i;
	register const vectype	*v,*c,*cb;
	register ulong	err,minError;
	short			j,minCode;
	
	minError = 0x7FFFFFFF;
	minCode = 0;
	for (j = 0, cb = fCodeBook; j < fCodebookSize; j++, cb += kCodeSize) {
		v = vector;
		c = cb;
		err = 0;
		for ( i=kCodeSize ; i--;  ) {
			e = *v++ - *c++;
			err += e * e;
			if ( err > minError )
				break;
		}
		if (err <= minError) {
			minError = err;
			minCode = j;
			if ( err == 0 )
				break;
		}
	}
	fIterationError += minError;
	if ( fMatchCount[minCode] < kMaxMatch ) 
	{
		AccumulateVector(fTempCodeBook + (minCode*kCodeSize),vector);
		fMatchCount[minCode]++;
		if ( fMatchError[minCode] + (minError>>2) > 0xffff )
			fMatchError[minCode] = 0xffff;
		else
			fMatchError[minCode] += minError>>2;
	}
	return minCode;
}




/***********************************************************************


	Checks if vector is in codebook
	
*/

inline Boolean 
VectorQuantizer::InCodeBook(const vectype *vector)
{
	register int i;
	register const vectype *cc,*vv;
	register const vectype *codeBook = fCodeBook;
	register int count = fCodebookSize;
	
	while (count--) {
		vv = vector;
		cc = codeBook;
		for ( i=0; i < kCodeSize; i++) {
			if ( *cc++ != *vv++ )
				break;
		}
		if ( i == kCodeSize )
			return true;
		codeBook += kCodeSize;
	}
	return false;
}



/***********************************************************************

	Do one ( or less ) iterations to refine VQ codebook.
	Returns true if iteration completed.

*/


Boolean 
VectorQuantizer::Iterate()
{

	register const vectype	*v;
	register vectype	*cc,*c;	
	short	j,holes;
	ulong	n;
	long	x,y;
	long	result = -1;
	uchar 	*dst = fBitMap->baseAddress;
	long 	rowBytes = fBitMap->rowBytes;
	result = 0;
	
	//	Match the all the vectors to the current codebook
	//	Acculmulate the vector into the new codebook

	v = fSourceVectors;
	n = 0;
	for ( y= (fHeight+1)>>1; y-- > 0 ;  ) {
		for ( x=(fWidth+1)>>1; x-- > 0 ;) {
			dst[n++] = MatchVectorToCodeBook(v);
			v += kCodeSize;
		}
		n += rowBytes-((fWidth+1)>>1);
	}
	
	//	Normalize new code book, pack it back into codeBook

	cc = fCodeBook;
	c = fTempCodeBook;
	holes = 0;
	for (j = 0; j < fCodebookSize; j++) {
		n = fMatchCount[j];
		if ( n == 0 ) {
			ZeroVector(cc);
			cc += kCodeSize;
			c += kCodeSize;
			holes++;
		} else {	
			if ( n == 1 ) {
				CopyVector(c,cc);
				cc += kCodeSize;
				c += kCodeSize;
			}
			else {
				int i;
				vectype rounding = (n >> 1);
				for ( i = kCodeSize; i-- ;) {
					vectype cv = *c++;
					*cc++ = (cv + rounding)/n;
				}
			}
		}
	}
	
	TrivialMessage(("MSE of %d in round %2d (%d holes)", fIterationError, fIterations, holes));
	
	//	Fill 1/4 of the available holes
	//	Select a vector that matched to the worst code and copy it into the codebook
	
	holes = (holes + 3) >> 2;
	while (holes) {
		uchar theWorst = 0;
		ushort worstErr = 0;
		for (j = 0; j < fCodebookSize; j++)	 {	// Get code with higest error in worst
			if (fMatchError[j] > worstErr) {
				worstErr = fMatchError[j];
				theWorst = j;
			}
		}
		if ( worstErr == 0 )
			break;
		fMatchError[theWorst] >>= 2;
		v = fSourceVectors;
		n = 0;
		j = 0;
		for ( y=(fHeight+1)>>1; y-- > 0;   ) {
			for ( x=(fWidth+1)>>1; x-- > 0; ) {
				if (dst[n++] == theWorst ) {
					if (!InCodeBook(v)) {
						j = 1;
						break;
					}
				}
				v += kCodeSize;
			}
			n += rowBytes - ((fWidth+1)>>1);
		}
		if (j == 0)
			 break;			// Can't find anything to fill hole with!		

		for (j = 0; j < fCodebookSize; j++)		
			if ( fMatchCount[j] == 0) {
				fMatchCount[j] = 1;
				CopyVector(v,fCodeBook + (j*kCodeSize));
				break;
			}
		holes--;
	}
	return true;
}



long
VectorQuantizer::ReduceCodebook()
{	
	long x,y;
	long j,k,n;
	long 	rowBytes = fBitMap->rowBytes;
	vectype *v;
	long os = fCodebookSize;
	for (j = 0; j < os; j++)	{	
		if ( fMatchCount[j] == 0) {
			fCodebookSize--;
			for ( k=j; k < fCodebookSize; k++ ) 
				CopyVector(fCodeBook + (k+1)*kCodeSize,fCodeBook + k*kCodeSize);
		}
	}
	if ( fCodebookSize < os ) {
		n = 0;
		v = fSourceVectors;
		uchar 	*dst = fBitMap->baseAddress;
		for ( y= (fHeight+1)>>1; y-- > 0 ;  ) {
			for ( x=(fWidth+1)>>1; x-- > 0 ;) {
				dst[n++] = MatchVectorToCodeBook(v);
				v += kCodeSize;
			}
			n += rowBytes-((fWidth+1)>>1);
		}
	}
	return os-fCodebookSize;
}

