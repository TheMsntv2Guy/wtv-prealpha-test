
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>

#include	"Symbols.h"


int verbose = 0;

int
ReadBELong(unsigned long *lp,FILE * f)
{
	int i;
	unsigned char a,b,c,d;
	i = fread(&a,1,1,f);
	i += fread(&b,1,1,f);
	i += fread(&c,1,1,f);
	i += fread(&d,1,1,f);
	*lp = ((unsigned long)a<<24) + ((unsigned long)b<<16) + ((unsigned long)c<<8) + ((unsigned long)d); 
	return i;
}

int
ReadSymbol(Symbol *s,FILE *infile)
{
	if ( ReadBELong(&s->address,infile) != 4 )
		return -1;
	if ( fread(s->symbol,1,kSymbolNameLen,infile) == kSymbolNameLen) {
		if ( verbose > 1 )
			fprintf(stderr,"0x%08x %s\n",s->address,s->symbol);
		return 0;
	}
	return -1;
	
	
}


int main(int argc,char *argv[])
{
	int	i;
	FILE *fp;
	unsigned long  symCount = 0;
	unsigned long address = 0;
	long	offset;
	unsigned long	sig = 0;
	unsigned mid = 0;
	unsigned low = 0;
	unsigned high = 0;
	Symbol	*gSymbols = 0;
	
	if ( argc != 3 ) {
		fprintf(stderr,"usage: %s symbolfile hex-address\n",argv[0]);
		exit(1);
	}
	if ( sscanf(argv[2],"%lx",&address) != 1 ) {
		fprintf(stderr,"usage: %s symbolfile hex-address\n",argv[0]);
		exit(1);
	}
	fp = fopen(argv[1],"rb");
	if ( fp == 0 ) {
		fprintf(stderr,"Can't open symbol file '%s'\n",argv[1]);
		exit(1);
	}
	ReadBELong(&sig,fp);
	if (sig != kSymbolSignature ) {
		fprintf(stderr,"invalid symbol file '%s'\n",argv[1]);
		exit(1);
	}
	ReadBELong(&symCount,fp);
	
	if( verbose )
		fprintf(stderr,"reading %d symbols  from %s\n",symCount,argv[1]);
	
	gSymbols = (Symbol*)malloc(symCount*sizeof(Symbol));
	if ( gSymbols == 0 ) {
		fprintf(stderr,"Can't malloc symbol table\n");
		exit(1);
	}

	for ( i=0; i < symCount; i++ ) {
		if ( ReadSymbol(&gSymbols[i],fp) ) {
			fprintf(stderr,"error reading symbols\n");
			exit(1);
		}
	}

	if( verbose )
		fprintf(stderr,"looking for address 0x%08x\n",address);

	high = symCount - 1;
	
	while(low <= high)
	{
		mid = (low+high)/2;
		if(gSymbols[mid].address > address)
			high = mid - 1;
		else if(gSymbols[mid].address < address)
			low = mid + 1;
		else
			break;
	}
	
	if((long)(address - gSymbols[mid].address) < 0)
		mid--;
	if ( mid < 0 )
		mid = 0;
	if ( mid >= symCount) 
		mid = symCount;
		
	offset = address - gSymbols[mid].address;
	fprintf(stdout,"0x%08X=%s + 0x%05X\n",address,gSymbols[mid].symbol,offset);
	
	fclose(fp);
	free(gSymbols);
	exit(0);
}

