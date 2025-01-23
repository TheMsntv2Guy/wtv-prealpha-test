#ifndef __TESTING_H__
#define __TESTING_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

void		ExecuteTest(ulong testNumber);
void		TestObjectStore(void);
void		benchmarks(void);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Testing.h multiple times"
	#endif
#endif /* __TESTING_H__ */
