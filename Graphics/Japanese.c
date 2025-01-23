
/*


	This file is a hack from some JIS conversion code I found, it needs to be re-written MHK


*/



long
ConvertJapaneseText(const char *src,char *dst,long inCount);

#include "boxansi.h"


typedef struct{
	char	*handle;
	long	pos;
	long	size;
}fakefile;

static short sMemErr = 0;

#ifndef	EOF
#define	EOF	-1
#endif


static void
strcpy(char *d,char *s);
static long
strlen(char *s);



static void
ffputc(fakefile *f,unsigned char c);

static long
ffgetc(fakefile *f);

static void 
fungetc(unsigned char c,fakefile *f);

static void
ffprintf2(fakefile *fp, const char *fmt,long a,long b);
static void
ffprintf3(fakefile *fp, const char *fmt,long a,long b,long c);


static inline void 
fungetc(unsigned char c,fakefile *f)
{
	if ( f->pos < 0 )
		return;
	f->pos--;
	f->handle[(f->pos)] = c;
}


static inline long
ffgetc(fakefile *f)
{
	if ( f->pos >= f->size )
		return EOF;
	
	return ( ((unsigned char *)(f->handle))[(f->pos++)])	;
}

static void
ffputc(fakefile *fp,unsigned char a)
{

	if ( fp->pos == fp->size ) {
		sMemErr = -1;
		return;
	}
	*(fp->handle + fp->pos) = a;
	fp->pos++;
}


static inline void
strcpy(char *d,char *s)
{

	while ( (*d++ = *s++) != 0 )
		;
}


static inline long
strlen(char *s)
{
	long c = 0;
	
	while ( *s++ != 0 )
		c++;
	return c;
}




#define INPUT       1
#define OUTPUT      2
#define REPAIR      3
#define NOTSET      0
#define NEW         1
#define OLD         2
#define NEC         3
#define EUC         4
#define SJIS        5
#define EUCORSJIS   6
#define ASCII       7
#define NUL         0
#define NL          10
#define FF          12
#define CR          13
#define ESC         27
#ifndef	TRUE
#define TRUE        1
#endif
#ifndef	FALSE
#define FALSE       0
#endif
#define PERIOD      '.'
#define SJIS1(A)    ((A >= 129 && A <= 159) || (A >= 224 && A <= 239))
#define SJIS2(A)    (A >= 64 && A <= 252)
#define HANKATA(A)  (A >= 161 && A <= 223)
#define ISEUC(A)    (A >= 161 && A <= 254)
#define ISMARU(A)   (A >= 202 && A <= 206)
#define ISNIGORI(A) ((A >= 182 && A <= 196) || (A >= 202 && A <= 206))
#ifndef SEEK_CUR
#define SEEK_CUR    1
#endif
/* The following 8 lines of code are used to establish the default output codes
 * when using the "-o[CODE]" or "-r[CODE]" options. They are self-explanatory,
 * and easy to change.
 */
#define DEFAULT_O   SJIS     /* default output code for "-o[CODE]" option */
#define DEFAULT_OS  ".sjs"   /* default file extension for "-o[CODE]" option */
#define DEFAULT_OKI ""       /* default kanji-in code for "-o[CODE]" option */
#define DEFAULT_OKO ""       /* default kanji-out code for "-o[CODE]" option */
#define DEFAULT_R   NEW      /* default output code for "-r[CODE]" option */
#define DEFAULT_RS  ".new"   /* default file extension for "-r[CODE]" option */
#define DEFAULT_RKI "$B"     /* default kanji-in code for "-r[CODE]" option */
#define DEFAULT_RKO "(J"     /* default kanji-out code for "-r[CODE]" option */

void han2zen(fakefile *in,long *p1,long *p2,long incode);
void sjis2jis(long *p1,long *p2);
void jis2sjis(long *p1,long *p2);
void shift2seven(fakefile *in,fakefile *out,long incode,char ki[],char ko[]);
void shift2euc(fakefile *in,fakefile *out,long incode,long tofullsize);
void euc2seven(fakefile *in,fakefile *out,long incode,char ki[],char ko[]);
void euc2euc(fakefile *in,fakefile *out,long incode,long tofullsize);
void shift2shift(fakefile *in,fakefile *out,long incode,long tofullsize);
void euc2shift(fakefile *in,fakefile *out,long incode,long tofullsize);
void seven2shift(fakefile *in,fakefile *out);
void seven2euc(fakefile *in,fakefile *out);
void seven2seven(fakefile *in,fakefile *out,char ki[],char ko[]);

void jisrepair(fakefile *in,fakefile *out,long verbose,long outcode,char ki[],char ko[]);
void removeescape(fakefile *in,fakefile *out,long verbose,long forcedelesc);
void printcode(long code);
long toup(long data);
long SkipESCSeq(fakefile *in,long temp,long *intwobyte);
long DetectCodeType(fakefile *in);
void setcode(long code,char ki[],char ko[]);



long
ConvertJapaneseText(const char *src,char *dst,long inCount,long dstSize);

long
ConvertJapaneseText(const char *buf,char *obuf,long	srcSize,long dstSize)
{
	long incode;
	fakefile *in,*out,inR,outR;
	long outcode,verbose = FALSE;
	long tofullsize = FALSE;
	char ki[10],ko[10];
	
	sMemErr = 0;
	
	in = &inR;
	out = &outR;
	
	inR.handle = (char *)buf;
	inR.pos = 0;
	inR.size = srcSize;
	
	outR.handle = obuf;
	outR.pos = 0;
	outR.size = dstSize;
	
	
    strcpy(ki,DEFAULT_OKI);
    strcpy(ko,DEFAULT_OKO);
    outcode = DEFAULT_O;
	
	incode = DetectCodeType(in);
	inR.pos = 0;
	
	switch (incode) {
	case NOTSET :
//	  ffprintff(stderr,"Unknown input code! Exiting...\n");
	  break;
	case EUCORSJIS :
//	  ffprintff(stderr,"Ambiguous (Shift-JIS or EUC) input code!\n");
//	  ffprintff(stderr,"Try using the \"-iCODE\" option to specify either Shift-JIS or EUC.\n");
//	  ffprintff(stderr,"Exiting...\n");
//	  exit(1);
	  break;
	case ASCII :
//	  ffprintff(stderr,"Since detected input code is ASCII, it may be damaged New- or Old-JIS\n");
//	  ffprintff(stderr,"Trying to repair...\n");
	  jisrepair(in,out,verbose,outcode,ki,ko);
	  break;
	case NEW :
	case OLD :
	case NEC :
	  switch (outcode) {
		case NEW :
		case OLD :
		case NEC :
		  seven2seven(in,out,ki,ko);
		  break;
		case EUC :
		  seven2euc(in,out);
		  break;
		case SJIS :
		  seven2shift(in,out);
		  break;
	  }
	  break;
	case EUC :
	  switch (outcode) {
		case NEW :
		case OLD :
		case NEC :
		  euc2seven(in,out,incode,ki,ko);
		  break;
		case EUC :
		  euc2euc(in,out,incode,tofullsize);
		  break;
		case SJIS :
		  euc2shift(in,out,incode,tofullsize);
		  break;
	  }
	  break;
	case SJIS :
	  switch (outcode) {
		case NEW :
		case OLD :
		case NEC :
		  shift2seven(in,out,incode,ki,ko);
		  break;
		case EUC :
		  shift2euc(in,out,incode,tofullsize);
		  break;
		case SJIS :
		  shift2shift(in,out,incode,tofullsize);
		  break;
	  }
	  break;
	}
	
	if  ( sMemErr == 0 ) 
		return out->pos;
	else 
		return sMemErr;
}



long toup(long data)
{
  if (islower(data))
    return (toupper(data));
  else
    return data;
}



long SkipESCSeq(fakefile *in,long temp,long *intwobyte)
{
  long tempdata;

  tempdata = *intwobyte;
  if (temp == '$' || temp == '(')
    ffgetc(in);
  if (temp == 'K' || temp == '$')
    *intwobyte = TRUE;
  else
    *intwobyte = FALSE;
  if (tempdata == *intwobyte)
    return FALSE;
  else
    return TRUE;
}
 
void removeescape(fakefile *in,fakefile *out,long verbose,long forcedelesc)
{
  long p1,p2,p3;
  unsigned long count = 0,other = 0;

	if ( verbose )
		;
  while ((p1 = ffgetc(in)) != EOF) {
    if (p1 == ESC) {
      p2 = ffgetc(in);
      if (p2 == '(') {
        p3 = ffgetc(in);
        switch (p3) {
          case 'J' :
          case 'B' :
          case 'H' :
            ffprintf2(out,"%c%c",p2,p3);
            count++;
            break;
          default :
            if (forcedelesc)
              ffprintf2(out,"%c%c",p2,p3);
            else
              ffprintf3(out,"%c%c%c",p1,p2,p3);
            other++;
            break;
        }
      }
      else if (p2 == '$') {
        p3 = ffgetc(in);
        switch (p3) {
          case 'B' :
          case '@' :
            ffprintf2(out,"%c%c",p2,p3);
            count++;
            break;
          default :
            if (forcedelesc)
              ffprintf2(out,"%c%c",p2,p3);
            else
              ffprintf3(out,"%c%c%c",p1,p2,p3);
            other++;
            break;
        }
      }
      else {
        if (forcedelesc)
			ffputc(out,p2);
        else
          ffprintf2(out,"%c%c",p1,p2);
        other++;
      }
    }
    else
	 ffputc(out,p1);
  }
}


/* MHK I changed a couple things here. 1st ignore space chars when in two byte mode
	becuase that was causing it to get out of sync ( maybe the mailer was consolidating
	spaces...) Second treat CRs the same as NL since this is a brain dead Mac after all.
	
*/



void jisrepair(fakefile *in,fakefile *out,long verbose,long outcode,char ki[],char ko[])
{
  long p1,p2,p3,intwobyte = FALSE;
  unsigned long count = 0;
  
  if ( verbose )
  	;

  while ((p1 = ffgetc(in)) != EOF) {
    if (intwobyte) {
	
	  if ( p1 == ' ' )
	  	continue;
      if (p1 == ESC) {
        p2 = ffgetc(in);
        if (p2 == '(') {
          p3 = ffgetc(in);
          switch (p3) {
            case 'J' :
            case 'B' :
            case 'H' :
              intwobyte = FALSE;
              switch (outcode) {
                case NEC :
                case NEW :
                case OLD :
                  ffprintf2(out,"%c%s",ESC,(long)ko);
                  break;
                default :
                  break;
              }
              break;
            default :
              ffprintf3(out,"%c%c%c",p1,p2,p3);
              break;
          }
        }
        else if (p2 == 'H') {
          intwobyte = FALSE;
          switch (outcode) {
            case NEC :
            case NEW :
            case OLD :
              ffprintf2(out,"%c%s",ESC,(long)ko);
              break;
            default :
              break;
          }
        }
        else
          ffprintf2(out,"%c%c",p1,p2);
      }
      else if (p1 == '(') {
        p2 = ffgetc(in);
        switch (p2) {
          case 'J' :
          case 'B' :
          case 'H' :
            intwobyte = FALSE;
            switch (outcode) {
              case NEC :
              case NEW :
              case OLD :
                ffprintf2(out,"%c%s",ESC,(long)ko);
                break;
              default :
                break;
            }
            count++;
            break;
          default :
            switch (outcode) {
              case NEC :
              case NEW :
              case OLD :
                ffprintf2(out,"%c%c",p1,p2);
                break;
              case EUC :
                p1 += 128;
                p2 += 128;
                ffprintf2(out,"%c%c",p1,p2);
                break;
              case SJIS :
                jis2sjis(&p1,&p2);
                ffprintf2(out,"%c%c",p1,p2);
                break;
            }
            break;
        }
      }
      else if (p1 == NL || p1 == CR) {
        switch (outcode) {
          case NEC :
          case NEW :
          case OLD :
            ffprintf3(out,"%c%s%c",ESC,(long)ko,p1);
            break;
          default :
			ffputc(out,p1);
            break;
        }
        count++;
        intwobyte = FALSE;
      }
      else if (p1 == FF)
        ;
      else {
        p2 = ffgetc(in);
        switch (outcode) {
          case NEC :
          case NEW :
          case OLD :
            ffprintf2(out,"%c%c",p1,p2);
            break;
          case EUC :
            p1 += 128;
            p2 += 128;
            ffprintf2(out,"%c%c",p1,p2);
            break;
          case SJIS :
            jis2sjis(&p1,&p2);
            ffprintf2(out,"%c%c",p1,p2);
            break;
        }
      }
    }
    else {
      if (p1 == ESC) {
        p2 = ffgetc(in);
        if (p2 == '$') {
          p3 = ffgetc(in);
          switch (p3) {
            case 'B' :
            case '@' :
              intwobyte = TRUE;
              switch (outcode) {
                case NEC :
                case NEW :
                case OLD :
                  ffprintf2(out,"%c%s",ESC,(long)ki);
                  break;
                default :
                  break;
              }
              break;
            default :
              ffprintf3(out,"%c%c%c",p1,p2,p3);
              break;
          }
        }
        else if (p2 == 'K') {
          intwobyte = TRUE;
          switch (outcode) {
            case NEC :
            case NEW :
            case OLD :
              ffprintf2(out,"%c%s",ESC,(long)ki);
              break;
            default :
              break;
          }
        }
        else
          ffprintf2(out,"%c%c",p1,p2);
      }
      else if (p1 == '$') {
        p2 = ffgetc(in);
        switch (p2) {
          case 'B' :
          case '@' :
            intwobyte = TRUE;
            switch (outcode) {
              case NEC :
              case NEW :
              case OLD :
                ffprintf2(out,"%c%s",ESC,(long)ki);
                break;
              default :
                break;
            }
            count++;
            break;
          default :
            switch (outcode) {
              case NEC :
              case NEW :
              case OLD :
                ffprintf2(out,"%c%c",p1,p2);
                break;
              case EUC :
                ffprintf2(out,"%c%c",p1,p2);
                break;
              case SJIS :
                ffprintf2(out,"%c%c",p1,p2);
                break;
            }
            break;
        }
      }  
      else if (p1 == FF)
        ;
      else
		ffputc(out,p1);
    }
  }
  if (intwobyte) {
    switch (outcode) {
      case NEC :
      case NEW :
      case OLD :
        ffprintf2(out,"%c%s",ESC,(long)ko);
        count++;
        break;
      default :
        break;
    }
  }
}

void sjis2jis(long *p1,long *p2)
{
    register unsigned char c1 = *p1;
    register unsigned char c2 = *p2;
    register long adjust = c2 < 159;
    register long rowOffset = c1 < 160 ? 112 : 176;
    register long cellOffset = adjust ? (31 + (c2 > 127)) : 126;

    *p1 = ((c1 - rowOffset) << 1) - adjust;
    *p2 -= cellOffset;
}

void jis2sjis(long *p1,long *p2)
{
    register unsigned char c1 = *p1;
    register unsigned char c2 = *p2;
    register long rowOffset = c1 < 95 ? 112 : 176;
    register long cellOffset = c1 % 2 ? 31 + (c2 > 95) : 126;

    *p1 = ((c1 + 1) >> 1) + rowOffset;
    *p2 = c2 + cellOffset;
}

void shift2seven(fakefile *in,fakefile *out,long incode,char ki[],char ko[])
{
  long p1,p2,intwobyte = FALSE;

  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case NUL :
      case FF :
        break;
      case CR :
      case NL :
        if (intwobyte) {
          intwobyte = FALSE;
          ffprintf2(out,"%c%s",ESC,(long)ko);
        }
		ffputc(out,CR);
        break;
      default :
        if SJIS1(p1) {
          p2 = ffgetc(in);
          if SJIS2(p2) {
            sjis2jis(&p1,&p2);
            if (!intwobyte) {
              intwobyte = TRUE;
              ffprintf2(out,"%c%s",ESC,(long)ki);
            }
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else if HANKATA(p1) {
          han2zen(in,&p1,&p2,incode);
          sjis2jis(&p1,&p2);
          if (!intwobyte) {
            intwobyte = TRUE;
            ffprintf2(out,"%c%s",ESC,(long)ki);
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else {
          if (intwobyte) {
            intwobyte = FALSE;
            ffprintf2(out,"%c%s",ESC,(long)ko);
          }
		  ffputc(out,p1);
        }
        break;
    }
  }
  if (intwobyte)
    ffprintf2(out,"%c%s",ESC,(long)ko);
}

void shift2euc(fakefile *in,fakefile *out,long incode,long tofullsize)
{
  long p1,p2;
  
  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case CR :
      case NL :
			ffputc(out,CR);
        break;
      case NUL :
      case FF :
        break;
      default :
        if SJIS1(p1) {
          p2 = ffgetc(in);
          if SJIS2(p2) {
            sjis2jis(&p1,&p2);
            p1 += 128;
            p2 += 128;
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else if HANKATA(p1) {
          if (tofullsize) {
            han2zen(in,&p1,&p2,incode);
            sjis2jis(&p1,&p2);
            p1 += 128;
            p2 += 128;
          }
          else {
            p2 = p1;
            p1 = 142;
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else
			ffputc(out,p1);
        break;
    }
  }
}
 
void euc2seven(fakefile *in,fakefile *out,long incode,char ki[],char ko[])
{
  long p1,p2,intwobyte = FALSE;

  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case NL :
      case CR :
        if (intwobyte) {
          intwobyte = FALSE;
          ffprintf2(out,"%c%s",ESC,(long)ko);
        }
			ffputc(out,p1);
        break;
      case FF :
        break;
      default :
        if ISEUC(p1) {
          p2 = ffgetc(in);
          if ISEUC(p2) {
            p1 -= 128;
            p2 -= 128;
            if (!intwobyte) {
              intwobyte = TRUE;
              ffprintf2(out,"%c%s",ESC,(long)ki);
            }
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else if (p1 == 142) {
          p2 = ffgetc(in);
          if HANKATA(p2) {
            p1 = p2;
            han2zen(in,&p1,&p2,incode);
            sjis2jis(&p1,&p2);
            if (!intwobyte) {
              intwobyte = TRUE;
              ffprintf2(out,"%c%s",ESC,(long)ki);
            }
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else {
          if (intwobyte) {
            intwobyte = FALSE;
            ffprintf2(out,"%c%s",ESC,(long)ko);
          }
			ffputc(out,p1);
        }
        break;
    }
  }
  if (intwobyte)
    ffprintf2(out,"%c%s",ESC,(long)ko);
}
 
void euc2shift(fakefile *in,fakefile *out,long incode,long tofullsize)
{
  long p1,p2;

  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case FF :
        break;
      default :
        if ISEUC(p1) {
          p2 = ffgetc(in);
          if ISEUC(p2) {
            p1 -= 128;
            p2 -= 128;
            jis2sjis(&p1,&p2);
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else if (p1 == 142) {
          p2 = ffgetc(in);
          if HANKATA(p2) {
            if (tofullsize) {
              p1 = p2;
              han2zen(in,&p1,&p2,incode);
              ffprintf2(out,"%c%c",p1,p2);
            }
            else {
              p1 = p2;
			ffputc(out,p1);
            }
          }
          else
            ffprintf2(out,"%c%c",p1,p2);
        }
        else
			ffputc(out,p1);
        break;
    }
  }
}

void euc2euc(fakefile *in,fakefile *out,long incode,long tofullsize)
{
  long p1,p2;

  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case FF :
        break;
      default :
        if ISEUC(p1) {
          p2 = ffgetc(in);
          if ISEUC(p2)
            ffprintf2(out,"%c%c",p1,p2);
        }
        else if (p1 == 142) {
          p2 = ffgetc(in);
          if (HANKATA(p2) && tofullsize) {
            p1 = p2;
            han2zen(in,&p1,&p2,incode);
            sjis2jis(&p1,&p2);
            p1 += 128;
            p2 += 128;
          }
          ffprintf2(out,"%c%c",p1,p2);
        }
        else
			ffputc(out,p1);
        break;
    }
  }
}

void shift2shift(fakefile *in,fakefile *out,long incode,long tofullsize)
{
  long p1,p2;
  
  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case CR :
      case NL :
			ffputc(out,CR);
        break;
      case NUL :
      case FF :
        break;
      default :
        if SJIS1(p1) {
          p2 = ffgetc(in);
          if SJIS2(p2)
            ffprintf2(out,"%c%c",p1,p2);
        }
        else if (HANKATA(p1) && tofullsize) {
          han2zen(in,&p1,&p2,incode);
          ffprintf2(out,"%c%c",p1,p2);
        }
        else
			ffputc(out,p1);
        break;
    }
  }
}

void seven2shift(fakefile *in,fakefile *out)
{
  long temp,p1,p2,intwobyte = FALSE;

  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case ESC :
        temp = ffgetc(in);
        SkipESCSeq(in,temp,&intwobyte);
        break;
      case NL :
      case CR :
        if (intwobyte)
          intwobyte = FALSE;
			ffputc(out,p1);
        break;
      case FF :
        break;
      default :
        if (intwobyte) {
          p2 = ffgetc(in);
          jis2sjis(&p1,&p2);
          ffprintf2(out,"%c%c",p1,p2);
        }
        else
			ffputc(out,p1);
        break;
    }
  }
}
  
void seven2euc(fakefile *in,fakefile *out)
{
  long temp,p1,p2,intwobyte = FALSE;

  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case ESC :
        temp = ffgetc(in);
        SkipESCSeq(in,temp,&intwobyte);
        break;
      case NL :
      case CR :
        if (intwobyte)
          intwobyte = FALSE;
			ffputc(out,p1);
        break;
      case FF :
        break;
      default :
        if (intwobyte) {
          p2 = ffgetc(in);
          p1 += 128;
          p2 += 128;
          ffprintf2(out,"%c%c",p1,p2);
        }
        else
			ffputc(out,p1);
        break;
    }
  }
}

void seven2seven(fakefile *in,fakefile *out,char ki[],char ko[])
{
  long temp,p1,p2,change,intwobyte = FALSE;

  while ((p1 = ffgetc(in)) != EOF) {
    switch (p1) {
      case ESC :
        temp = ffgetc(in);
        change = SkipESCSeq(in,temp,&intwobyte);
        if ((intwobyte) && (change))
          ffprintf2(out,"%c%s",ESC,(long)ki);
        else if (change)
          ffprintf2(out,"%c%s",ESC,(long)ko);
        break;
      case NL :
      case CR :
        if (intwobyte) {
          intwobyte = FALSE;
          ffprintf2(out,"%c%s",ESC,(long)ko);
        }
			ffputc(out,p1);
        break;
      case FF :
        break;
      default :
        if (intwobyte) {
          p2 = ffgetc(in);
          ffprintf2(out,"%c%c",p1,p2);
        }
        else
			ffputc(out,p1);
        break;
    }
  }
  if (intwobyte)
    ffprintf2(out,"%c%s",ESC,(long)ko);
}

long DetectCodeType(fakefile *in)
{
  long c = 0,whatcode = ASCII;

  while ((whatcode == EUCORSJIS || whatcode == ASCII) && c != EOF) {
    if ((c = ffgetc(in)) != EOF) {
      if (c == ESC) {
        c = ffgetc(in);
        if (c == '$') {
          c = ffgetc(in);
          if (c == 'B')
            whatcode = NEW;
          else if (c == '@')
            whatcode = OLD;
        }
        else if (c == 'K')
          whatcode = NEC;
      }
      else if ((c >= 129 && c <= 141) || (c >= 143 && c <= 159))
        whatcode = SJIS;
      else if (c == 142) {
        c = ffgetc(in);
        if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160) || (c >= 224 && c <= 252))
          whatcode = SJIS;
        else if (c >= 161 && c <= 223)
          whatcode = EUCORSJIS;
      }
      else if (c >= 161 && c <= 223) {
        c = ffgetc(in);
        if (c >= 240 && c <= 254)
          whatcode = EUC;
        else if (c >= 161 && c <= 223)
          whatcode = EUCORSJIS;
        else if (c >= 224 && c <= 239) {
          whatcode = EUCORSJIS;
          while (c >= 64 && c != EOF && whatcode == EUCORSJIS) {
            if (c >= 129) {
              if (c <= 141 || (c >= 143 && c <= 159))
                whatcode = SJIS;
              else if (c >= 253 && c <= 254)
                whatcode = EUC;
            }
            c = ffgetc(in);
          }
        }
        else if (c <= 159)
          whatcode = SJIS;
      }
      else if (c >= 240 && c <= 254)
        whatcode = EUC;
      else if (c >= 224 && c <= 239) {
        c = ffgetc(in);
        if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160))
          whatcode = SJIS;
        else if (c >= 253 && c <= 254)
          whatcode = EUC;
        else if (c >= 161 && c <= 252)
          whatcode = EUCORSJIS;
      }
    }
  }
  return whatcode;
}

void han2zen(fakefile *in,long *p1,long *p2,long incode)
{
  long junk,maru,nigori;

  maru = nigori = FALSE;
  if (incode == SJIS) {
    *p2 = ffgetc(in);
    if (*p2 == 222) {
      if (ISNIGORI(*p1) || *p1 == 179)
        nigori = TRUE;
      else
        fungetc(*p2,in);
    }
    else if (*p2 == 223) {
      if ISMARU(*p1)
        maru = TRUE;
      else
        fungetc(*p2,in);
    }
    else
      fungetc(*p2,in);
  }
  else if (incode == EUC) {
    junk = ffgetc(in);
    if (junk == 142) {
      *p2 = ffgetc(in);
      if (*p2 == 222) {
        if (ISNIGORI(*p1) || *p1 == 179)
          nigori = TRUE;
        else {
          fungetc(*p2,in);
          fungetc(junk,in);
        }
      }
      else if (*p2 == 223) {
        if ISMARU(*p1)
          maru = TRUE;
        else {
          fungetc(*p2,in);
          fungetc(junk,in);
        }
      }
      else {
        fungetc(*p2,in);
        fungetc(junk,in);
      }
    }
    else
      fungetc(junk,in);
  }
  switch (*p1) {
    case 161 :
      *p1 = 129;
      *p2 = 66;
      break;
    case 162 :
      *p1 = 129;
      *p2 = 117;
      break;
    case 163 :
      *p1 = 129;
      *p2 = 118;
      break;
    case 164 :
      *p1 = 129;
      *p2 = 65;
      break;
    case 165 :
      *p1 = 129;
      *p2 = 69;
      break;
    case 166 :
      *p1 = 131;
      *p2 = 146;
      break;
    case 167 :
      *p1 = 131;
      *p2 = 64;
      break;
    case 168 :
      *p1 = 131;
      *p2 = 66;
      break;
    case 169 :
      *p1 = 131;
      *p2 = 68;
      break;
    case 170 :
      *p1 = 131;
      *p2 = 70;
      break;
    case 171 :
      *p1 = 131;
      *p2 = 72;
      break;
    case 172 :
      *p1 = 131;
      *p2 = 131;
      break;
    case 173 :
      *p1 = 131;
      *p2 = 133;
      break;
    case 174 :
      *p1 = 131;
      *p2 = 135;
      break;
    case 175 :
      *p1 = 131;
      *p2 = 98;
      break;
    case 176 :
      *p1 = 129;
      *p2 = 91;
      break;
    case 177 :
      *p1 = 131;
      *p2 = 65;
      break;
    case 178 :
      *p1 = 131;
      *p2 = 67;
      break;
    case 179 :
      *p1 = 131;
      *p2 = 69;
      break;
    case 180 :
      *p1 = 131;
      *p2 = 71;
      break;
    case 181 :
      *p1 = 131;
      *p2 = 73;
      break;
    case 182 :
      *p1 = 131;
      *p2 = 74;
      break;
    case 183 :
      *p1 = 131;
      *p2 = 76;
      break;
    case 184 :
      *p1 = 131;
      *p2 = 78;
      break;
    case 185 :
      *p1 = 131;
      *p2 = 80;
      break;
    case 186 :
      *p1 = 131;
      *p2 = 82;
      break;
    case 187 :
      *p1 = 131;
      *p2 = 84;
      break;
    case 188 :
      *p1 = 131;
      *p2 = 86;
      break;
    case 189 :
      *p1 = 131;
      *p2 = 88;
      break;
    case 190 :
      *p1 = 131;
      *p2 = 90;
      break;
    case 191 :
      *p1 = 131;
      *p2 = 92;
      break;
    case 192 :
      *p1 = 131;
      *p2 = 94;
      break;
    case 193 :
      *p1 = 131;
      *p2 = 96;
      break;
    case 194 :
      *p1 = 131;
      *p2 = 99;
      break;
    case 195 :
      *p1 = 131;
      *p2 = 101;
      break;
    case 196 :
      *p1 = 131;
      *p2 = 103;
      break;
    case 197 :
      *p1 = 131;
      *p2 = 105;
      break;
    case 198 :
      *p1 = 131;
      *p2 = 106;
      break;
    case 199 :
      *p1 = 131;
      *p2 = 107;
      break;
    case 200 :
      *p1 = 131;
      *p2 = 108;
      break;
    case 201 :
      *p1 = 131;
      *p2 = 109;
      break;
    case 202 :
      *p1 = 131;
      *p2 = 110;
      break;
    case 203 :
      *p1 = 131;
      *p2 = 113;
      break;
    case 204 :
      *p1 = 131;
      *p2 = 116;
      break;
    case 205 :
      *p1 = 131;
      *p2 = 119;
      break;
    case 206 :
      *p1 = 131;
      *p2 = 122;
      break;
    case 207 :
      *p1 = 131;
      *p2 = 125;
      break;
    case 208 :
      *p1 = 131;
      *p2 = 126;
      break;
    case 209 :
      *p1 = 131;
      *p2 = 128;
      break;
    case 210 :
      *p1 = 131;
      *p2 = 129;
      break;
    case 211 :
      *p1 = 131;
      *p2 = 130;
      break;
    case 212 :
      *p1 = 131;
      *p2 = 132;
      break;
    case 213 :
      *p1 = 131;
      *p2 = 134;
      break;
    case 214 :
      *p1 = 131;
      *p2 = 136;
      break;
    case 215 :
      *p1 = 131;
      *p2 = 137;
      break;
    case 216 :
      *p1 = 131;
      *p2 = 138;
      break;
    case 217 :
      *p1 = 131;
      *p2 = 139;
      break;
    case 218 :
      *p1 = 131;
      *p2 = 140;
      break;
    case 219 :
      *p1 = 131;
      *p2 = 141;
      break;
    case 220 :
      *p1 = 131;
      *p2 = 143;
      break;
    case 221 :
      *p1 = 131;
      *p2 = 147;
      break;
    case 222 :
      *p1 = 129;
      *p2 = 74;
      break;
    case 223 :
      *p1 = 129;
      *p2 = 75;
      break;
  }
  if (nigori) {
    if ((*p2 >= 74 && *p2 <= 103) || (*p2 >= 110 && *p2 <= 122))
      (*p2)++;
    else if (*p1 == 131 && *p2 == 69)
      *p2 = 148;
  }
  else if (maru && *p2 >= 110 && *p2 <= 122)
    *p2 += 2;
}




static void
ffprintf3(fakefile *fp,const char *fmt,long a,long b,long c)
{


	if ( fmt[1] == 'c' ) {
		ffputc(fp,a);
	} else {
		char *s = (char *)a;
		while ( *s ) 
			ffputc(fp,*s++);
	}
	if ( fmt[3] == 'c' ) {
		ffputc(fp,b);
	} else {
		char *s = (char *)b;
		while ( *s ) 
			ffputc(fp,*s++);
	}
	if ( fmt[5] == 'c' ) {
		ffputc(fp,c);
	} else {
		char *s = (char *)c;
		while ( *s ) 
			ffputc(fp,*s++);
	}
}



static void
ffprintf2(fakefile *fp,const char *fmt,long a,long b)
{
	
	
	if ( fmt[1] == 'c' ) {
		ffputc(fp,a);
	} else {
		char *s = (char *)a;
		while ( *s ) 
			ffputc(fp,*s++);
	}
	if ( fmt[3] == 'c' ) {
		ffputc(fp,b);
	} else {
		char *s = (char *)b;
		while ( *s ) 
			ffputc(fp,*s++);
	}
}
