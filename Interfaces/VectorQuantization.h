#ifndef	__VECTORQUANTIZATION_H__
#define	__VECTORQUANTIZATION_H__

#include	"Headers.h"
#ifndef __DISPLAYABLE_H__
#include "Displayable.h"		/* for Displayable */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"			/* for Resource */
#endif
#include	"Utilities.h"
#ifndef	__ERRORNUMBERS__
#include	"ErrorNumbers.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef	__GRAPHICS__
#include	"Graphics.h"
#endif

#ifdef SIMULATOR
	#ifndef __SIMULATOR_H__
	#include "Simulator.h"
	#endif
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif




typedef  ushort vectype;		
	
#define	kVectorTypeSize	2	

const kCodeSize = 12;					// during VQ :	 y u v - y u v 	    when done:		 y u y v		
										//  			 y u v - y u v						 y u y v



enum VQState {
	kVQIdle,
	kVQBeginIteration,
	kVQIterating,
	kVQEndIteration,
	kVQDone,
	kVQError
};

class VectorQuantizer : public Displayable 
{
public:
							VectorQuantizer();
							~VectorQuantizer();							
	Error					Start(BitMapDevice *src,Rectangle *srcRect = nil,Rectangle *dst = nil,short codebookSize=256,short iterations=4);
	Boolean					Continue(Boolean cancel);
	BitMapDevice*			GetBitMap(Boolean transferOwnership);
	
protected:		
	VQState					fState;
	void					Reset(Boolean keepBitmap);
	Boolean					Iterate();
	short					MatchVectorToCodeBook(const vectype *vector);
	Boolean					InCodeBook(const vectype *vector);
	long					ReduceCodebook();
	vectype*				fSourceVectors;
	vectype*				fCodeBook;
	vectype*				fTempCodeBook;
	ushort*					fMatchError;
	ushort*					fMatchCount;
	long					fVectorCount;
	ushort					fCodebookSize;
	ushort					fWidth;
	ushort					fHeight;
	ushort					fIterations;
	ulong					fLastError;
	ulong					fIterationError;
	BitMapDevice*			fBitMap;
	Rectangle				fDstRect;	
};




#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Audio.h multiple times"
	#endif
#endif
