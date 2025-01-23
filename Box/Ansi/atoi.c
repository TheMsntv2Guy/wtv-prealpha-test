/* еееее this file is now obsolete еееее */

#if 0

// ===========================================================================
//	atoi.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#include "boxansi.h"

/* еее NEED TO FIX еее doesn't handle negative numbers */

long atol(const char *s)
{
	long retval = 0;
	char c;

	if (s == NULL)
		s = kEmptyString;

	if (ValidReadLocation(s))
	{
		while ((c = *s++) >= '0' && c <= '9') {
			retval *= 10;
			retval += c - '0';
		}
	}
	
	return retval;
}

int atoi(const char *s)
{
	return atol(s);
}

#endif