/* ===========================================================================
 *	printf.c
 *
 *	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#include "Headers.h"

/* ---------------------------------------------------------------------------
 *	prototypes
 * ------------------------------------------------------------------------ */

static void* _vnullprintf_writen(void* unused, const char* src, size_t num);
static void* _vprintf_writen(void* unused, const char* src, size_t num);
#ifdef INCLUDE_FPRINTF
	static void* _vfprintf_writen(void* void_fp, const char* src, size_t num);
#endif /* #ifdef INCLUDE_FPRINTF */
static void* _vsprintf_writen(void* void_state, const char* src, size_t num);

int vnullprintf(const char* format, va_list params);
int vprintf(const char* format, va_list params);
#ifdef INCLUDE_FPRINTF
	int vfprintf(FILE* fp, const char* format, va_list params);
#endif /* #ifdef INCLUDE_FPRINTF */
int vsprintf(char* dest, const char* format, va_list params);
int vsnprintf(char* dest, size_t destsize, const char* format, va_list params);

int nullprintf(const char* format, ...);
int printf(const char* format, ...);
#ifdef INCLUDE_FPRINTF
	int fprintf(FILE* fp, const char* format, ...);
#endif /* #ifdef INCLUDE_FPRINTF */
int sprintf(char* dest, const char* format, ...);
int snprintf(char* dest, size_t destsize, const char* format, ...);

/* ---------------------------------------------------------------------------
 *	_vprintf_main() does the bulk of the work, parsing the format string and
 *			delegating to functions to print each parameter
 *
 *	static prototypes:
 * ------------------------------------------------------------------------ */

typedef struct _vprintf_conversion_struct {
	
	/****** in params */
	char*		buffer;		/* here's some space to use */
	int			bufsize;	/* buffer size */
	va_list*	params;		/* here's the stack...roll your own parameter */

	Boolean 	space_flag;	/* use ' ' instead of '+' for positive */
	Boolean 	plus_flag;	/* use '+' instead of ' ' for positive */
	Boolean 	minus_flag;	/* left-justify conversion */
	Boolean 	num_flag;	/* %o => add leading '0', %x/X => add leading "0x"/"0X" */
	Boolean 	zero_flag;	/* pad with zeros */
	int			width;		/* desired width */
	int			precision;	/* desired precision */
	int			qualifier;	/* unsigned or long (hl) */
	int			code;		
	
	/****** out params */
	int		prefix_size;	/* chars to get from buffer before zeros */
	int		leading_zeros;	/* number of zeros to write out */
	int		size;		/* chars to get from buffer after zeros */

} _vprintf_conversion_struct;

static void _vprintf_char(_vprintf_conversion_struct* vcs);
static void _vprintf_int(_vprintf_conversion_struct* vcs);
static void _vprintf_string(_vprintf_conversion_struct* vcs);
static int _vprintf_main(void*(*pfn)(void*, const char*, size_t),
					void* arg, const char* format, va_list params);

static const char kUnknownConversion[] = "<%  unknown>";
static const kUnknownConversionLocation = 2; /* i.e., index where to plop the char
												into the kUnknownConversion string */

static void
_vprintf_char(_vprintf_conversion_struct* vcs)
{
	vcs->prefix_size = 0;
	vcs->leading_zeros = 0;

	if (vcs->code == 'c') {
		vcs->buffer[0] = va_arg(*(vcs->params), int);
		vcs->size = 1;
	} else if (vcs->code == '%') {
		vcs->buffer[0] = '%';
		vcs->size = 1;
	} else if (vcs->bufsize >= sizeof(kUnknownConversion) - 1) {
		memcpy(vcs->buffer, kUnknownConversion, sizeof(kUnknownConversion) - 1);
		vcs->buffer[kUnknownConversionLocation] = vcs->code;
		vcs->size = sizeof(kUnknownConversion) - 1;
		if ((vcs->width > 0) && (vcs->size > vcs->width)) {
			vcs->size = vcs->width;
		}
	} else {
		vcs->buffer[0] = vcs->code;
		vcs->size = 1;
	}
}

static void
_vprintf_int(_vprintf_conversion_struct* vcs)
{
	static const char lower_digits[] = "x0123456789abcdef";
	static const char upper_digits[] = "X0123456789ABCDEF";
	const char* digits = &(lower_digits[1]);
	Boolean useSigned = true;
	int base = 10;
	
	vcs->prefix_size = 0;
	vcs->leading_zeros = 0;
	vcs->size = 0;

	switch (vcs->code) {
		case 'd':
			/* digits = &(lower_digits[1]); */
			/* useSigned = true; */
			/* base = 10; */
			break;
		case 'i':
			/* digits = &(lower_digits[1]); */
			/* useSigned = true; */
			/* base = 10; */
			break;
		case 'o':
			/* digits = &(lower_digits[1]); */
			useSigned = false;
			base = 8;
			break;
		case 'u':
			/* digits = &(lower_digits[1]); */
			useSigned = false;
			/* base = 10; */
			break;
		case 'X':
			digits = &(upper_digits[1]);
			useSigned = false;
			base = 16;
			break;
		case 'x':
			/* digits = &(lower_digits[1]); */
			useSigned = false;
			base = 16;
			break;
		default:
			if (IsError(true))/* Unrecognized printf character */
			    return;
	}
	
	Boolean	positive = true;
	
	unsigned long ulval;
	
	if (useSigned) {
		if (vcs->qualifier == 'h') {
			ulval = (unsigned long)(long)va_arg(*(vcs->params), short);
		} else if (vcs->qualifier != 'l') {
			ulval = (unsigned long)(long)va_arg(*(vcs->params), int);
		} else {
			ulval = (unsigned long)va_arg(*(vcs->params), long);
		}
	} else {
		if (vcs->qualifier == 'h') {
			ulval = (unsigned long)va_arg(*(vcs->params), unsigned short);
		} else if (vcs->qualifier != 'l') {
			ulval = (unsigned long)va_arg(*(vcs->params), unsigned int);
		} else {
			ulval = va_arg(*(vcs->params), unsigned long);
		}
	}
	
	if (useSigned && (((long)ulval) < 0)) {
		ulval = -ulval;
		positive = false;
	}

	char* buffer = vcs->buffer;
	int bufsize = vcs->bufsize;
	if (IsError(bufsize <= 0)) /* where are we supposed to store the result?! */
		return;

	if ((vcs->precision != 0) || (ulval != 0)) {
		do {
			buffer[--bufsize] = digits[ulval % base];
			ulval = ulval/base;
		} while ((ulval != 0) && (bufsize>0));
	}
	
	vcs->size = vcs->bufsize - bufsize;
	if (vcs->size < vcs->precision)
		vcs->leading_zeros = vcs->precision - vcs->size;
	
	if (vcs->num_flag) {
		if ((base == 8) && !IsWarning(bufsize < 1)) {
				buffer[--bufsize] = '0';
				vcs->prefix_size++;
		} else if ((base == 16) && !IsWarning(bufsize < 2)) {
				buffer[--bufsize] = digits[-1];
				buffer[--bufsize] = '0';
				vcs->prefix_size+=2;
		}	
	}

	if (useSigned) {
		if (positive) {
			if ((vcs->plus_flag) && !IsWarning(bufsize < 1)) {
				buffer[--bufsize] = '+';
				vcs->prefix_size++;
			} else if ((vcs->space_flag) && !IsWarning(bufsize < 1)) {
				buffer[--bufsize] = ' ';
				vcs->prefix_size++;
			}
		} else {
			if (!IsWarning(bufsize < 1)) {
				buffer[--bufsize] = '-';
				vcs->prefix_size++;
			}
		}
	}

	vcs->buffer += bufsize;
}

#ifdef FIDO_INTERCEPT
static void
_vprintf_float(_vprintf_conversion_struct* vcs)
{
	static const char digits[] = "0123456789";
	static const long powersOf10[] = {0, 10, 100, 1000, 10000, 100000, 1000000,
		10000000 ,100000000, 1000000000};
	vcs->prefix_size = 0;
	vcs->size = 0;
	switch (vcs->code) {
		case 'f':
			break;
		default:
			IsError(true);	/* Unrecognized printf character */
			return;
	}
	Boolean	positive = true;
	float ulval = (float)va_arg(*(vcs->params), double);
	
	if (ulval < 0) {
		ulval = -ulval;
		positive = false;
	}

	char* buffer = vcs->buffer;
	int bufsize = vcs->bufsize;
	if (IsError(bufsize <= 0)) /* where are we supposed to store the result?! */
		return;
	long integer = ulval;
	long fraction = (ulval - integer) * powersOf10[vcs->precision];
	
	for (int counter = vcs->precision; counter > 0; --counter) {
		buffer[--bufsize] = digits[fraction % 10];
		fraction = fraction / 10;
		if (IsError(bufsize <= 0))
			return;
	}
	if (vcs->precision > 0 && bufsize > 0)
		buffer[--bufsize] = '.';
	do {
		buffer[--bufsize] = digits[integer % 10];
		integer = integer / 10;
	} while (integer != 0 && bufsize > 0);
	
	vcs->size = vcs->bufsize - bufsize;
	if (vcs->size < vcs->precision)
		vcs->leading_zeros = vcs->precision - vcs->size;

	if (positive) {
		if ((vcs->plus_flag) && !IsWarning(bufsize < 1)) {
			buffer[--bufsize] = '+';
			vcs->prefix_size++;
		} else if ((vcs->space_flag) && !IsWarning(bufsize < 1)) {
			buffer[--bufsize] = ' ';
			vcs->prefix_size++;
		}
	} else {
		if (!IsWarning(bufsize < 1)) {
			buffer[--bufsize] = '-';
			vcs->prefix_size++;
		}
	}
	vcs->buffer += bufsize;
}
#endif

static void
_vprintf_string(_vprintf_conversion_struct* vcs)
{
	vcs->prefix_size = 0;
	vcs->leading_zeros = 0;
	vcs->buffer = va_arg(*(vcs->params), char*);
	vcs->size = (vcs->buffer != nil) ? strlen(vcs->buffer) : 0;
	if ((vcs->size > vcs->precision) && (vcs->precision >= 0))
		vcs->size = vcs->precision;
}

static int
_vprintf_main(void*(*pfn)(void*, const char*, size_t),
		 void* arg, const char* format, va_list params)
{
	int numchars = 0;
	int copychars;
	char ch;
	const _VPRINTF_CONVERSION_STRUCT_BUFSIZE = 32;
	char buffer[_VPRINTF_CONVERSION_STRUCT_BUFSIZE];
	_vprintf_conversion_struct vcs;
	
	const char* s = format;
	
	while (true) {
		
		/****** first copy non-conversion stuff */
		
		ch = *s;
		while ((ch != '\0') && (ch != '%')) {
			ch = *++s;
		}
		copychars = s - format;
		if (copychars != 0) {
			if ((*pfn)(arg, format, copychars) == NULL) {
				return EOF;
			} else {
				numchars += copychars;
			}
		}

		if (ch == '\0') {
			return numchars;
		}
		
		/****** ch=='%', so parse conversion flags */
		vcs.buffer = &(buffer[0]);
		vcs.bufsize = sizeof(buffer);
		vcs.params = &params;
		
		vcs.space_flag = false;
		vcs.plus_flag = false;
		vcs.minus_flag = false;
		vcs.num_flag = false;
		vcs.zero_flag = false;
		
		vcs.prefix_size	= 0;
		vcs.leading_zeros = 0;
		vcs.size = 0;
		
		while ((ch = *++s) != '\0') {
			const char* conversion_flag = strchr(" +-#0", ch);
			if (conversion_flag == NULL)		{ break; } 
			else if (*conversion_flag == '\0')	{ break; } 
			else if (*conversion_flag == ' ')	{ vcs.space_flag = true; }
			else if (*conversion_flag == '+')	{ vcs.plus_flag = true; }
			else if (*conversion_flag == '-')	{ vcs.minus_flag = true; }
			else if (*conversion_flag == '#')	{ vcs.num_flag = true; }
			else if (*conversion_flag == '0') 	{ vcs.zero_flag = true; }
			else 								{if(IsError(true)) return EOF;}
		}
		
		/****** parse field width */
		
		if (ch == '*') {	/* get it out of the va_list? */
			vcs.width = va_arg(params, int);
			if (vcs.width < 0) {
				vcs.width = -vcs.width;
				vcs.minus_flag = true;
			}
			ch = *++s;
		} else {
			vcs.width = 0;	/* default */
			while (isdigit(ch)) {
				vcs.width = (vcs.width*10) + (ch - '0');
				ch = *++s;
			}
		}
		
		/****** parse precision */
		if (ch == '.') {
			ch = *++s;
			if (ch=='*') {	/* get it out of the va_list? */
				vcs.precision = va_arg(params, int);
				ch = *++s;
			} else {
				vcs.precision = 0;
				while (isdigit(ch)) {
					vcs.precision = (vcs.precision*10) + (ch - '0');
					ch = *++s;
				}
			}
		} else {
			vcs.precision = -1;	/* default */
		}
		
		/****** check for qualifier */
		if ((ch=='h') || (ch == 'l')) {
			vcs.qualifier = ch;
			ch = *++s;
		} else {
			vcs.qualifier = '\0';	/* default */
		}
		
		/****** switch on the specific conversion */
		vcs.code = ch;		
		switch (ch)
		{
			case 'c':
				_vprintf_char(&vcs);	/* responsible for checking the '%' case... */
				break;
			case 'd':
			case 'i':
			case 'o':
			case 'u':
			case 'x':
			case 'X':
				_vprintf_int(&vcs);
				break;
			case 'p':
				vcs.code = 'x';	/* to get pointers to print as hex unsigned */
				_vprintf_int(&vcs);
				break;				
			case 'n':
				if (vcs.qualifier == 'h') {
					short* sp = va_arg(params, short*);
					if (ValidWriteLocations(sp, sizeof(short)))
					    *sp = numchars;
				} else if (vcs.qualifier != 'l') {
					long* lp = va_arg(params, long*);
					if (ValidWriteLocations(lp, sizeof(long)))
					    *lp = numchars;
				} else {
					int* ip = va_arg(params, int*);
					if (ValidWriteLocations(ip, sizeof(long)))
					    *ip = numchars;
				}
				break;
 			case 's':
 				_vprintf_string(&vcs);
 				break;
			case '%':
				_vprintf_char(&vcs);
				break;				
	#ifdef FIDO_INTERCEPT
			case 'f':
				_vprintf_float(&vcs);
				break;
	#endif
			default:
				if (IsError(true))     /* we didn't recognize this character */
				    _vprintf_char(&vcs); /* print out what we didn't recognize */
				break;
		
		}
		
		{	/* now actually print the damn thing */
		
			int spaces = vcs.width - (vcs.prefix_size + vcs.leading_zeros + vcs.size);
		
			if (vcs.zero_flag && (!vcs.minus_flag) && (spaces > 0))
			{
				vcs.leading_zeros += spaces;
				spaces = 0;
			}
			
			/****** five parts: leading spaces, prefix, leading zeros, size, trailing spaces */
			/****** for example:  "    0x000ab5cd" is: generated from "0xab5cd", pre=2, post=5 */
			/******                11112233344444 */
		
			if (!vcs.minus_flag) {	
				while (spaces-- > 0) {
					if ((*pfn)(arg, " ", 1) == NULL) {
						return EOF;
					} else {
						numchars++;
					}
				}
			}
			if (vcs.prefix_size > 0) {
				if ((*pfn)(arg, vcs.buffer, vcs.prefix_size) == NULL) {
					return EOF;
				} else {
					numchars += vcs.prefix_size;
				}
			}
			while (vcs.leading_zeros-- > 0) {
				if ((*pfn)(arg, "0", 1) == NULL) {
					return EOF;
				} else {
					numchars++;
				}
			}
			if (vcs.size > 0) {
				if ((*pfn)(arg, vcs.buffer + vcs.prefix_size, vcs.size) == NULL) {
					return EOF;
				} else {
					numchars += vcs.size;
				}
			}
			if (vcs.minus_flag) {	
				while (spaces-- > 0) {
					if ((*pfn)(arg, " ", 1) == NULL) {
						return EOF;
					} else {
						numchars++;
					}
				}
			}
		
			format = ++s;
		}	
	}
}



/* ---------------------------------------------------------------------------
 *	_vprintf_writen:   helper to vfprintf
 *	_vfprintf_writen:  helper to vfprintf
 *	_vsprintf_writen:  helper to vsprintf and vsnprintf
 * ------------------------------------------------------------------------ */

typedef struct {
	char*	dest;
	size_t	size;		/* how many we have space for */
	size_t	count;		/* how many we tried to write */
} _vsprintf_writen_state;

#ifdef TEST_CLIENTTEST
static void*
_vnullprintf_writen(void* /*unused*/, const char* src, size_t num)
{
	return (void*)(ValidReadLocations(src, num));
}
#endif


static void*
_vprintf_writen(void* /*unused*/, const char* src, size_t num)
{
	int count = 0;

	if (!ValidReadLocations(src, num))
		return NULL;
	
	while (count<num) {
		if (putchar(src[count++]) == EOF)
			return NULL;
	}
	return (void*)true;
}

#ifdef INCLUDE_FPRINTF
static void*
_vfprintf_writen(void* void_fp, const char* src, size_t num)
{
	int count = 0;

	if ((void_fp != nil) && (ValidReadLocations(src, num))) {	
		while (count<num) {
			if (fputc(src[count++], (FILE*)void_fp) == EOF)
				return NULL;
		}
	}
	return (FILE*)void_fp;
}
#endif /* #ifdef INCLUDE_FPRINTF */

static void*
_vsprintf_writen(void* void_state, const char* src, size_t num)
{
	_vsprintf_writen_state* state = (_vsprintf_writen_state*)void_state;
	size_t copycount = 0;
	char* dest = nil;

	/* maybe we could check to see that (state, sizeof(state)) are readable, writeable */

	if (state == nil) {
		return dest;
	}
	
	copycount = state->size - state->count;	/* space we have left */
	if (copycount > num) {
		copycount = num;	/* space we're going to use */
	}
	
	dest = state->dest + state->count;
	if (ValidReadLocations(src, copycount) && ValidWriteLocations(dest, copycount)) {
		if (IsError(copycount != num)) {
			ImportantMessage(("snprintf tried to overwrite buffer!"));
		}
		memcpy(dest, src, copycount);
		state->count += copycount;
	}

	return dest + copycount;
}

/* ---------------------------------------------------------------------------
 *	vprintf, vfprintf, vsprintf, vsnprintf - va_list versions
 * ------------------------------------------------------------------------ */

#ifdef TEST_CLIENTTEST
int
vnullprintf(const char* format, va_list params)
{
	return _vprintf_main(&_vnullprintf_writen, (void*)0 /*unused*/, format, params);
}
#endif

int
vprintf(const char* format, va_list params)
{
	return _vprintf_main(&_vprintf_writen, (void*)0 /*unused*/, format, params);
}

#ifdef INCLUDE_FPRINTF
int
vfprintf(FILE* fp, const char* format, va_list params)
{
	return _vprintf_main(&_vfprintf_writen, (void*)fp, format, params);
}
#endif /* #ifdef INCLUDE_FPRINTF */

int
vsprintf(char* dest, const char* format, va_list params)
{
	_vsprintf_writen_state state;
	int result;
	state.dest = dest;
	state.size = LONG_MAX - 1; 
	state.count = 0;
	
	result = _vprintf_main(&_vsprintf_writen, (void*)&state, format, params);
	if (result >= 0) {
		dest[result] = '\0';
	}
	return result;
}

int
vsnprintf(char* dest, size_t destsize, const char* format, va_list params)
{
	_vsprintf_writen_state state;
	int result;
	state.dest = dest;
	state.size = destsize - 1; /* -1 for zero termination */
	state.count = 0;
	
	result = _vprintf_main(&_vsprintf_writen, (void*)&state, format, params);
	if (!IsError(result >= destsize) && (result >= 0)) {
		dest[result] = '\0';
	}
	return result;
}

/* ---------------------------------------------------------------------------
 *	nullprintf, printf, fprintf, sprintf, snprintf - variable parameter versions
 * ------------------------------------------------------------------------ */

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
int
nullprintf(const char* format, ...)
{
	int result;
	va_list params;
	
	va_start(params, format);
	result = vnullprintf(format, params);
	va_end(params);
	
	return result;
}
#endif

int
printf(const char* format, ...)
{
	int result;
	va_list	params;
	
	va_start(params, format);
	result = vprintf(format, params);
	va_end(params);
	
	return result;
}

#ifdef INCLUDE_FPRINTF
int
fprintf(FILE* fp, const char* format, ...)
{
	int result;
	va_list	params;
	
	va_start(params, format);
	result = vfprintf(fp, format, params);
	va_end(params);
	
	return result;
}
#endif /* #ifdef INCLUDE_FPRINTF */

int sprintf(char* dest, const char* format, ...)
{
	int result;
	va_list params;
	
	va_start(params, format);
	result = vsprintf(dest, format, params);
	va_end(params);
	if (result >= 0)
		dest[result] = '\0';
	return result;
}

int snprintf(char* dest, size_t destsize, const char* format, ...)
{
	int result;
	va_list params;
	
	va_start(params, format);
	result = vsnprintf(dest, destsize, format, params);
	va_end(params);
	if ((result >= 0) && (result < destsize))
		dest[result] = '\0';
	
	return result;
}
