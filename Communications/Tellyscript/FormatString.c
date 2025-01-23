/*

	In the spirit of printf() but using pascal strings.

	Format specifiers begin with the character % and include zero or more
	of the following conversion specification elements:

	% [justify flag] [0 pad character] [field size] [type of variable]

	Justify flag (optional):
		-	Left justify output in field, pad on right (default is to
			right justify).
	Pad character (optional):
		0	Use zero rather than space (default) for the pad character.
	Type of variable:
		i	int
		l	long
		c	single character
		s	pascal string
		a	character array
		%	print a %, no argument used
		
*/
#include "Headers.h"
#include "TellyDefs.h"

Byte *FormatString(Byte *string, Byte *tmplate);
Byte *SFormatString(Byte *string, Byte *tmplate, long *param);


static void MoveBytes(
	register Byte *source,
	register Byte **destination,
	register Byte *limit,
	register short count)
{
	while (*destination <= limit && count--)
		*(*destination)++ = *source++;
}

static void MoveBlanks(
	register Byte **destination,
	register Byte *limit,
	register short count,
	register Byte pad)
{
	if (count <= 0)
		return;
	while (*destination <= limit && count--)
		*(*destination)++ = pad;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Byte *FormatString(
	Byte *result,			/* Result string */
	Byte *tmplate)			/* Tmplate string */
{
	va_list ap;
	Byte *p = result + 1;
	Byte *max = tmplate + *tmplate;
	Byte num[30];
	short fieldWidth;
	Byte pad;
	Boolean leftJustify;

	va_start(ap, tmplate);

	while (tmplate < max) {
		tmplate++;
		if (*tmplate == '%') {
			tmplate++;
			fieldWidth = 0;
			leftJustify = false;
			pad = ' ';
			if (*tmplate == '-') {
				tmplate++;
				leftJustify = true;
			}
			if (*tmplate >= '0' && *tmplate <= '9') {
				if (*tmplate == '0')
					pad = '0';
				do
					fieldWidth = 10*fieldWidth + (*tmplate++ & 0x0F);
				while (*tmplate >= '0' && *tmplate <= '9');
			}
			switch (*tmplate) {
				case 'a':			/* Character array */
					MoveBytes(va_arg(ap, Byte *), &p, result + 255, fieldWidth);
					break;
				case 'c':			/* Character */
					{
						Byte c;

						if (!leftJustify)
							MoveBlanks(&p, result + 255, fieldWidth - 1, pad);
#ifndef THINK_C
						/* In MPW C char is passed in 4 bytes on the stack! */
						c = va_arg(ap, long);
#else
						/* In THINK C char is passed in 2 bytes on the stack! */
						c = va_arg(ap, short);
#endif
						MoveBytes(&c, &p, result + 255, 1);
						if (leftJustify)
							MoveBlanks(&p, result + 255, fieldWidth - 1, pad);
						}
					break;
				case 's':			/* String */
					{
						register Byte *sp;

						sp = va_arg(ap, Byte *);
						if (!leftJustify)
							MoveBlanks(&p, result + 255, fieldWidth - strlen((char *) sp),
								pad);
						MoveBytes(sp + 1, &p, result + 255, strlen((char *) sp));
						if (leftJustify)
							MoveBlanks(&p, result + 255, fieldWidth - strlen((char *) sp),
								pad);
					}
					break;
				case 'i':			/* int */
#ifndef THINK_C
					/* In MPW C short is passed in 4 bytes on the stack! */
#else
					sprintf((char *) num,"%d",va_arg(ap, short));
					goto integer;
#endif
				case 'l':			/* long */
					sprintf((char *) num,"%ld",va_arg(ap, long));
				integer:
					if (!leftJustify)
						MoveBlanks(&p, result + 255, fieldWidth - strlen((char *) num),
							pad);
					MoveBytes(num + 1, &p, result + 255, strlen((char *) num));
					if (leftJustify)
						MoveBlanks(&p, result + 255, fieldWidth - strlen((char *) num),
							pad);
					break;
				default:
					*p++ = *tmplate;
			}
		} else						/* Copy character */
			*p++ = *tmplate;
	}
	va_end(pa);
	*result = p - result - 1;
	return result;
}
#endif

Byte *SFormatString(			/* Special version for scripts */
	Byte *result,				/* Result string (C string) */
	register Byte *tmplate,	/* Tmplate string (C-string) */
	long params[])				/* Array with parameters (char, long or C-string) */
{
	Byte *p = result;
	register long *s = params;
	Byte num[30];
	short fieldWidth;
	Byte pad;
	Boolean leftJustify;

	while (*tmplate) {
		if (*tmplate == '%') {
			tmplate++;
			if (!*tmplate)
				break;
			fieldWidth = 0;
			leftJustify = false;
			pad = ' ';
			if (*tmplate == '-') {
				tmplate++;
				if (!*tmplate)
					break;
				leftJustify = true;
			}
			if (*tmplate >= '0' && *tmplate <= '9') {
				if (*tmplate == '0')
					pad = '0';
				do
					fieldWidth = 10*fieldWidth + (*tmplate++ & 0x0F);
				while (*tmplate >= '0' && *tmplate <= '9');
				if (!*tmplate)
					break;
			}
			switch (*tmplate) {
				case 'c':			/* Character */
					if (!leftJustify)
						MoveBlanks(&p, result + 255, fieldWidth - 1, pad);
					MoveBytes((Byte *)s + 3, &p, result + 255, 1);
					if (leftJustify)
						MoveBlanks(&p, result + 255, fieldWidth - 1, pad);
					s++;
					break;
				case 's':			/* C-string */
					{
						register Byte *sp;

						sp = (Byte *)*s;
						if (!leftJustify)
							MoveBlanks(&p, result + 255, fieldWidth - strlen((char *)sp),
								pad);
						MoveBytes(sp, &p, result + 255, strlen((char *)sp));
						if (leftJustify)
							MoveBlanks(&p, result + 255, fieldWidth - strlen((char *)sp),
								pad);
					}
					s++;
					break;
				case 'd':
				case 'i':			/* int */
					sprintf((char *) num, "%ld", *s);
					s++;
					if (!leftJustify)
						MoveBlanks(&p, result + 255, fieldWidth - strlen((char *) num),
							pad);
					MoveBytes(num, &p, result + 255, strlen((char *) num));
					if (leftJustify)
						MoveBlanks(&p, result + 255, fieldWidth - strlen((char *) num),
							pad);
					break;
				default:
					*p++ = *tmplate;
			}
		} else						/* Copy character */
			*p++ = *tmplate;
		tmplate++;
	}
done:
	*p='\0';
	return result;
}
