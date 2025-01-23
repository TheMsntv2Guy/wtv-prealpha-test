/* ===========================================================================
 *	str.c
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#include "Headers.h"

/* ------------------------------------------------------------------------ */

static unsigned long _Stoul(const char* s, char **endptr, int base);

/* ------------------------------------------------------------------------ */

#define STOUL_BASE_MAX 36
static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char ndigs[STOUL_BASE_MAX+1] = {
	0, 0, 33, 21, 17, 14, 13, 12, 11, 11,	/* base  0 -  9 */
	10, 10, 9, 9, 9, 9, 9, 8, 8, 8,			/* base 10 - 19 */
	8, 8, 8, 8, 7, 7, 7, 7, 7, 7,			/* base 20 - 29 */
	7, 7, 7, 7, 7, 7, 7};					/* base 30 - 36 */
	
static unsigned long
_Stoul(const char* s, char **endptr, int base)
{
	const char* s1;
	const char* s2;
	const char* sc;
	const char* sd;
	long n;
	long lastdigit;
	unsigned long x;
	unsigned long y;
	char sign;

	if (IsError(!(ValidReadLocation(s) &&
				 ((endptr == NULL) || ValidWriteLocations(endptr, 4))))) {
		return 0;
	}
	
	if (s == NULL) {
		if (endptr != NULL) {
			*endptr = NULL;
		}
		return 0;
	}
	
	sc = s;
	while (isspace(*sc)) {
		sc++;
	}
	
	sign = ((*sc == '-') || (*sc == '+')) ? *sc++ : '+';

	if ((base<0) || (base==1) || (STOUL_BASE_MAX<base)) {
		if (endptr != NULL) {
			*endptr = (char*)s;
		}
		return 0;
	} else if (base != 0) {
		if ((base == 16) && (*sc == '0') && (tolower(sc[1]) == 'x')) {
			sc += 2;
		}
	} else if (*sc != 0) {
		base = 10;
	} else if (sc[1] == 'x' || sc[1] == 'X') {
		base = 16;
		sc += 2;
	} else {
		base = 8;
	}

	s1 = sc;
	while (*sc == '0') {
		sc++;
	}

	lastdigit = 0;
	x = 0;
	y = 0;
	s2 = sc;
	while (((sd = (const char*)memchr(digits, tolower(*sc), base)) != NULL)
			&& (x >= y)) {
		y = x;
		lastdigit = sd - digits;
		x = (x * base) + lastdigit;
		sc++;
	}
	
	if (s1 == sc) {
		if (endptr != NULL) {
			*endptr = (char*)s;
		}
		return (unsigned long)0;
	}
	n = sc - s2 - ndigs[base];
	if (n >= 0) {
		if ((0 < n) || (x < x-lastdigit) || ((x-lastdigit)/base != y)) {
			x = ULONG_MAX;
		}
	}
	if (sign == '-') {
		x = -x;
	}
	if (endptr != NULL) {
		*endptr = (char*)sc;
	}
	return x;
}

/* ------------------------------------------------------------------------ */

int
atoi(const char* s)
{
	return (int)(strtol(s, NULL, 10));
}

long
atol(const char* s)
{
	return strtol(s, NULL, 10);
}

char *
strcat(char *s, const char *append)
{
	char *save = s;

	while(ValidReadLocation(s) && (*s != 0)) {
		s++;
	}
	while (ValidWriteLocation(s) && ValidReadLocation(append)
			&& (*s = *append) != 0) {
		s++;
		append++;
	}
	
	return(save);
}


char *
strchr(const char *s, int c)
{
	const char ch = c;

	if (s == NULL)
		s = kEmptyString;

	while(ValidReadLocation(s) && (*s != '\0')) {
		if (*s == ch) {
			return (char*)s;
		}
		s++;
	}
		
	return NULL;
}


int
strcmp(const char *s1, const char *s2)
{
	if (s1 == NULL)
		s1 = kEmptyString;
	if (s2 == NULL)
		s2 = kEmptyString;
	
	while (ValidReadLocation(s1) && ValidReadLocation(s2)) {
		if (*s1 == *s2) {
			if (*s1 == 0) {
				return 0;
			} else {
				s1++;
				s2++;
			}
		} else if (*s1 > *s2) {
			return(1);
		} else {
			return(-1);
		}
	}
	return 0;
}


char *
strcpy(char *s1, const char *s2)
{
	char* save = s1;

	if (s2 == NULL)
		s2 = kEmptyString;
	
	while (ValidWriteLocation(s1) && ValidReadLocation(s2)
			&& ((*s1++ = *s2++) != '\0')) {
		/* empty */
	}
	
	return(save);
}

size_t
strlen(const char *s)
{
	size_t result = 0;

	if (IsError(s == nil))  /* compiler may have its own builtin strlen */
		return result;		/* so we can't allow people to call this with nil */

	while (ValidReadLocation(s) && (*s++ != '\0'))
		result++;
	
	return result;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
char *
strncat(char *s, const char *append, size_t n)
{
	char *save = s;

	while (ValidReadLocation(s) && (*s != 0)) {
		s++;
	}
	while (ValidWriteLocation(s) && ValidReadLocation(append)
			&& ((*s = *append) != 0) && (n>0)) {
		s++;
		append++;
		n--;
	}
	
	return(save);
}
#endif

int
strncmp(const char *s1, const char *s2, size_t n)
{

	if (s1 == NULL)
		s1 = kEmptyString;
	if (s2 == NULL)
		s2 = kEmptyString;
	
	while (ValidReadLocation(s1) && ValidReadLocation(s2) && (n-- != 0)) {
		if (*s1 == *s2) {
			if (*s1 == 0) {
				return 0;
			} else {
				s1++;
				s2++;
			}
		} else if (*s1 > *s2) {
			return (+1);
		} else {
			return (-1);
		}
	}
	return 0;
}


char *
strncpy(char *s1, const char *s2, size_t n)
{
	char* save = s1;

	if (s2 == NULL)
		s2 = kEmptyString;

	if (ValidWriteLocations(s1, n)) {
		while (ValidReadLocation(s2) && (*s2 != '\0') && (n != 0)) {
			*s1++ = *s2++;
			n--;
		}
	}
	while (n--)
		*s1++ = 0;
	
	return (save);
}


char *
strrchr(const char *s, int c)
{
	const char ch = c;
	const char* sc = NULL;

	if (s == NULL) {
		s = kEmptyString;
	}
	
	while (ValidReadLocation(s) && (*s != '\0')) {
		if (*s == ch) {
			sc = s;
		}
		s++;
	}
	
	return (char*)sc;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
size_t
strspn( const char *cs, const char *ct)
{
	size_t count = 0;
	const char* c = cs;
	const char* t;
	
	while (*c != 0) {
	
	   t = ct;
	   while (*t != 0) {
		   
		   if (*c == *t) {
		       count++;
		       break;
		   }  
		   t++;
	   }
	   c++;
	}
	
	return count;
}
#endif

char *
strstr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while (sc != c);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

unsigned long
strtoul(const char* s, char** endptr, int base)
{
	return _Stoul(s, endptr, base);
}

long
strtol(const char* s, char **endptr, int base)
{
	const char* sc = s;
	unsigned long x;
	
	while (isspace(*sc)) {
		sc++;
	}
	x = _Stoul(s, endptr, base);
	if ((*sc == '-') && (x <= LONG_MAX)) {
		return LONG_MIN;
	} else if ((*sc != '-') && (LONG_MAX < x)) {
		return LONG_MAX;
	} else {
		return ((long)x);
	}
}
