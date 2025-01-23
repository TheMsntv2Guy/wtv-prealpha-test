
#ifdef	__macintosh
#include <Types.h>
#include <Memory.h>
#include <CursorCtl.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "demangle.h"

#undef TESTING

char buffer[512];

#include	"Symbols.h"

Symbol *gSymbols;


int main(int argc,char *argv[]);
int comparefunc(const void *e1,const void *e2);

int comparefunc(const void *e1,const void *e2)
{
	Symbol* s1 = (Symbol*)e1;
	Symbol* s2 = (Symbol*)e2;
	
	return (s1->address - s2->address);
}

int
WriteBELong(unsigned long s,FILE * f)
{
	int i;
	char a,b,c,d;
	a = s>>24;
	b = s>>16;
	c = s>>8;
	d = s;
	i = fwrite(&a,1,1,f);
	i += fwrite(&b,1,1,f);
	i += fwrite(&c,1,1,f);
	i += fwrite(&d,1,1,f);
	return i;
}

int
WriteSymbol(Symbol *s,FILE *outfile)
{
	if ( WriteBELong(s->address,outfile) != 4 )
		return -1;
	if ( fwrite(s->symbol,1,kSymbolNameLen,outfile) != kSymbolNameLen )
		return -1;
	return 0;
}


int main(int argc,char *argv[])
{
	int	i;
	FILE *fp;
	FILE *outfile;
	Symbol s;
	long nSyms = 0;
	long symNo;
	char dummy1;
	long st;
	long sc;
	long indx;
#ifdef	__macintosh
	Handle h;
	OSErr	err;
#endif
	char *name;
	SymHeader head;
	int ret = 0;
	char toolName[64];
	static char sbuf[512];
	
	
	
	strcpy(toolName, argv[0]);
	
	if(argc != 4)
	{
		fprintf(stderr, "### Usage: %s <symfile> -o <outfile>\n", toolName);
		return 1;
	}
	
#ifdef	__macintosh
	
	InitCursorCtl(nil);
	h = TempNewHandle(kMaxSymbols*sizeof(Symbol),&err);
	if(h == 0)
	{
		fprintf(stderr, "### %s: NewHandle failed\n", toolName);
		return 1;
	}
	
	MoveHHi(h);
	HLock(h);
	
	gSymbols = (Symbol*)*h;
#else
	gSymbols = (Symbol*)malloc(kMaxSymbols*sizeof(Symbol));
	if ( gSymbols == 0 ) {
		fprintf(stderr,"Can't malloc symbol table\n");
		exit(1);
	}
#endif

	fp = fopen(argv[1],"r");
	if(fp == 0)
	{
		fprintf(stderr, "### %s: fopen failed\n", toolName);
		ret = 1;
		goto bail;
	}
	 
	do
	{
		fgets(buffer,255,fp);
	} while(strstr(buffer,START) == 0);
	
	fprintf(stdout, "#  Slurping symbols...\n");
	fflush(stdout);


	do
	{
		fgets(buffer,255,fp);
		if ( buffer == 0 ) 
		{
			fprintf(stderr,"# reached end of input without END");
			ret = 1;
			goto bail;
		}
		if(strstr(buffer,END) != 0)
			break;
		if ( nSyms == kMaxSymbols ) 
		{
			fprintf(stderr,"# too many symbols > %d",kMaxSymbols);
			ret = 1;
			goto bail;
		}
		if ( sscanf(buffer,"[%x] %c %x st %x sc %x indx %x %s\n",&symNo,&dummy1,&s.address,&st,&sc,&indx,sbuf) == 7 ) 
		{	
			name = cplus_demangle(sbuf,0);
			if( name && strcmp(name,"ÿÁ") != 0 )
				strcpy(s.symbol,name);
			else
				strncpy(s.symbol,sbuf,63);
			free(name);
			switch(st)
			{
				case 0x1:
					/* global or v table */
					if(sc == 0xd || sc == 0xf)
						gSymbols[nSyms++] = s;
					break;
				case 0x2:
					gSymbols[nSyms++] = s;
					break;
				case 0x6:
				case 0xe:
					if(dummy1 != 'e')
						gSymbols[nSyms++] = s;
					break;
				default:
					break;
			}
			if(indx != 0xfffff)
				fgets(buffer,255,fp);	/* skip second line */
		}
#ifdef	__macintosh
		if ( (nSyms & 3) == 0 ) 
			SpinCursor(32);
#endif
		
	} while(1);
	fprintf(stdout, "#  slurped %d symbols\n",nSyms);
	fflush(stdout);
	
	fprintf(stdout, "#  sorting symbols...\n");
	fflush(stdout);
	
	qsort(gSymbols,nSyms,sizeof(Symbol),comparefunc);
	
	outfile = fopen(argv[3],"wb");
	if(!outfile)
	{
		fprintf(stderr, "### %s: fopen target failed\n", toolName);
		ret = 1;
		goto bail;
	}
	
	printf("# writing symbols to '%s'...\n",argv[3]);
	fflush(stdout);
	
	head.cookie = kSymbolSignature;
	head.count = nSyms;
	
	WriteBELong(head.cookie,outfile);
	WriteBELong(head.count,outfile);
	
	for ( i=0; i < nSyms; i++ ) {
		if ( WriteSymbol(&gSymbols[i],outfile) ) {
			fprintf(stderr, "### %s: write symbol failed\n", toolName);
			ret = 1;
			goto bail;
		}
	}	
	fclose(outfile);
	fclose(fp);

#ifdef TESTING
	{
		int i;
		outfile = fopen("testsyms","w");
		for(i=0;i<nSyms;i++)
			fprintf(outfile,"%x %s\n",gSymbols[i].address,gSymbols[i].symbol);
		fclose(outfile);
	}
#endif



bail:	
#ifdef	__macintosh
	HUnlock(h);
	DisposeHandle(h);
#else
	free(gSymbols);
#endif
	fprintf(stdout, "#  SnarfSyms done\n");
	
	return ret;
}

