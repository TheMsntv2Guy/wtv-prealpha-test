#ifndef __STUBS_H__
#define __STUBS_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

/* this is the file that holds the stubs for the system services
 * that do not yet exist. 
 */

#define kFileExistsException	0


void RequestURL(char *address);
int getdomainname(char *name, long len);
char *getpass( const char *prompt );


#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Stubs.h multiple times"
	#endif
#endif /* __STUBS_H__ */
