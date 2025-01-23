
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef	__macintosh
#include "Memory.h"

#define	LONG(v)		(v)
#define	SHORT(v)	(v)
#endif



typedef struct
{
	unsigned short		f_magic;
	unsigned short		f_nscns;
	long				f_timdat;
	long				f_symptr;
	long				f_nsyms;
	unsigned short		f_opthdr;
	unsigned short 		f_flags;
} filhdr;

typedef struct
{
	short		magic;
	short		vstamp;
	long		tsize;
	long		dsize;
	long		bsize;
	long		entry;
	long		text_start;
	long		data_start;
	long		bss_start;
	long		gprmask;
	long		cprmask[4];
	long		gpvalue;
} aouthdr;


typedef struct
{
	char			name[8];
	unsigned long	longs[8];
} secnhdr;

typedef struct
{
	char			name[8];
	unsigned long	start;
	unsigned long	start2;
	unsigned long 	length;
	unsigned long	offset;
	unsigned long 	unk[4];
} secn;


typedef struct
{
	filhdr			file_h;
	aouthdr			aout_h;
	secnhdr			text_h;			
	secnhdr			data_h;			
	unsigned long	pad;
} fakeECOFFHeader;



#define SCNHSZ 40
#define	TXT_OFFSET(f)	((sizeof(filhdr) + f.f_opthdr + f.f_nscns * SCNHSZ + 7) & ~7)


typedef struct
{
	unsigned long		romfs_length;
	unsigned long		romfs_checksum;
	
} ROMFSHeader;

typedef struct
{
	unsigned long		emac_cool;
	
} ROMFSNode;


typedef struct 
{
	unsigned long		branch;				/* don't whack this, it's the reset instruction! */
	unsigned long 		nop;				/* don't whack this, it's the delay nop! */
	unsigned long 		rom_checksum;		/* checksum, NOT including the checksum itself */
	unsigned long 		rom_size;			/* size in bytes */
	unsigned long 		code_size;			/* size in bytes */
	unsigned long		version;			/* passed in as cmd line arg when buildrom is invoked */
	unsigned long		data_start;			/* offset into ROM to .data section */
	unsigned long		data_length;
	unsigned long		bss_length;
	unsigned long		romfs_address;
} ROMHeader;


unsigned long
LONG(unsigned long v)
{
	unsigned long l;
	unsigned char *p = (unsigned char *)&l;
	*p++ = v>>24;
	*p++ = v>>16;
	*p++ = v>>8;
	*p++ = v;
	return l;
}

unsigned short
SHORT(unsigned short v)
{
	unsigned short s;
	unsigned char *p = (unsigned char *)&s;
	*p++ = v>>8;
	*p++ = v;
	return s;
}


FILE *infp;
FILE *romfsfp;
FILE *outfp;
int size = 0;
filhdr fh;
aouthdr ah;
unsigned char *workingPtr;				/* used as we fill in the rom, increments. */
unsigned char *romMem;					/* always points at allocated block */
unsigned long romSize		= 65536;
unsigned long availROMSize	= 0;		/* how much of the total rom we can put stuff in */
unsigned long romOffset;
unsigned long romFileSize;
unsigned long romCodeSize;
unsigned long romFSSize;
unsigned long buildVersion	= 0x01020304;
unsigned long fakeECOFFSize = 0;
unsigned long romBaseAddr	= 0x80100000;
unsigned long romfsNext	= 0x00000000;
char *romFileName 			= "BOXROM";
char *romFSName 			= "ROMFS";
char *outputFileName		= "OUTROM";
char toolName[32];  /* name of the executable */


int	gPad 				= 0;	/* should we pad a ROM w/o a ROMFS? */


#ifdef	__macintosh
Handle gROMMem;
#else
char *gROMMem;
#endif

secn sections[16];			/* more than we'll ever need */


/* memory versions ( not neccessarily big-endian ) */

secn msections[16];			
filhdr mfh;
aouthdr mah;


unsigned long AddFakeECOFFHeader(unsigned char *mem);
void DumpStats(void);
void ClearROMBuffer(void);
unsigned long getVal(char *s, unsigned long *val);
unsigned long parseCmdLine(unsigned long argc, char *argv[]);

unsigned long ChecksumROM(unsigned long *start, unsigned long *end);


unsigned long AddFakeECOFFHeader(unsigned char *mem)
{
fakeECOFFHeader *ecoff;

	ecoff = (fakeECOFFHeader *)mem;
	
	ecoff->file_h.f_magic  		= SHORT(0x0160);
	ecoff->file_h.f_nscns  		= SHORT(0x0002);
	ecoff->file_h.f_timdat		= 0x00000000;
	ecoff->file_h.f_symptr 		= 0x00000000;
	ecoff->file_h.f_nsyms  		= 0x00000000;
	ecoff->file_h.f_opthdr 		= SHORT(sizeof(aouthdr));
	ecoff->file_h.f_flags  		= SHORT(0x0207);
	
	ecoff->aout_h.magic			= SHORT(0x0107);
	ecoff->aout_h.vstamp		= SHORT(0x020b);
	if(romFSSize)
		ecoff->aout_h.tsize		= LONG(romSize);
	else
		ecoff->aout_h.tsize		= LONG(romCodeSize);		/* code-only builds are built to just the right size */
	ecoff->aout_h.dsize			= 0;
	ecoff->aout_h.bsize			= 0;
	ecoff->aout_h.entry			= LONG(0x80200000);
	ecoff->aout_h.text_start	= LONG(0x80200000);
	ecoff->aout_h.data_start	= LONG(0x80300000);
	ecoff->aout_h.bss_start		= 0;
	ecoff->aout_h.gprmask		= LONG(0xF407FFFE);
	ecoff->aout_h.cprmask[0]	= 0;
	ecoff->aout_h.cprmask[1]	= 0;
	ecoff->aout_h.cprmask[2]	= 0;
	ecoff->aout_h.cprmask[3]	= 0;
	ecoff->aout_h.gpvalue		= 0;			/* we'll fix this up in the ROM loader */
	
	strcpy(ecoff->text_h.name, ".text");
	ecoff->text_h.longs[0] = LONG(0x80200000);
	ecoff->text_h.longs[1] = LONG(0x80200000);
	ecoff->text_h.longs[2] = LONG(romSize);
	ecoff->text_h.longs[3] = LONG(sizeof(fakeECOFFHeader));
	ecoff->text_h.longs[4] = LONG(0x00000000);
	ecoff->text_h.longs[5] = LONG(0x00000000);
	ecoff->text_h.longs[6] = LONG(0x00000000);
	ecoff->text_h.longs[7] = LONG(0x00000020);
			
	strcpy(ecoff->text_h.name, ".data");
	ecoff->text_h.longs[0] = LONG(0x80300000);
	ecoff->text_h.longs[1] = LONG(0x80300000);
	ecoff->text_h.longs[2] = LONG(0);
	ecoff->text_h.longs[3] = LONG(sizeof(fakeECOFFHeader));
	ecoff->text_h.longs[4] = LONG(0x00000000);
	ecoff->text_h.longs[5] = LONG(0x00000000);
	ecoff->text_h.longs[6] = LONG(0x00000000);
	ecoff->text_h.longs[7] = LONG(0x00000020);
			
	ecoff->pad = LONG(0);
	
	return(sizeof(fakeECOFFHeader));
}


void DumpStats(void)
{
unsigned long iii;

	fprintf(stdout, "--- ECOFF File is 0x%x (%d) bytes long.\n",romFileSize,romFileSize);
	fprintf(stdout, "--- There are %d sections.\n",mfh.f_nscns);
	
	for (iii = 0; iii != mfh.f_nscns; iii++)
	{
		fprintf(stdout, "--- %s section starts at file offset 0x%x (%d), is 0x%x (%d) bytes starting at %x\n",	msections[iii].name,
																										msections[iii].offset,
																										msections[iii].offset,
																										msections[iii].length,
																										msections[iii].length,
																										msections[iii].start);
	}

	fprintf(stdout, "--- ROM Base is at 0x%x\n",romBaseAddr);
}


void ClearROMBuffer(void)
{
unsigned long offset;

	for (offset = 0; offset != romSize+fakeECOFFSize; offset += 4)		/* init the ROM space */
		*(unsigned long *)((unsigned long)romMem + offset) = LONG(0x6a6f6562);
};



unsigned long getVal(char *s, unsigned long *val) 
{
	if (isdigit(s[0]))
  	{
 		if (s[1] == 'x')
			sscanf(s, "%x", val);
	  	else
	  		sscanf(s, "%d", val);	
	
		return(0);
	}
	else
		return(1);
}

unsigned long parseCmdLine(unsigned long argc, char *argv[])
{
char *s;
unsigned long cnt;
unsigned long err;
strcpy(toolName, argv[0]);

  err = 0;										/* assume no err */

	if (argc != 1)								
	{
		for (cnt = 1; cnt < argc; cnt++)
		{
			s = argv[cnt];
			switch (s[1])
			{
			case 's':
			  			cnt++;
						err = getVal(argv[cnt], &romSize);		
						break;
			case 'a':
			  			cnt++;
						err = getVal(argv[cnt], &availROMSize);		
						break;
			case 'v':
			  			cnt++;
						err = getVal(argv[cnt], &buildVersion);	
						break;
			case 'b':
			  			cnt++;
						err = getVal(argv[cnt], &romBaseAddr);	
						break;
			case 'n':
			  			cnt++;
						err = getVal(argv[cnt], &romfsNext);	
						break;
			case 'c':
			  			cnt++;
						romFileName = argv[cnt];
						break;
			case 'f':
			  			cnt++;
						romFSName = argv[cnt];
						break;
			case 'o':
			  			cnt++;
						outputFileName = argv[cnt];
						break;
			case 'e':
						fakeECOFFSize = sizeof(fakeECOFFHeader);	
						break;
			case 'p':
						gPad = 1;	
						break;
			case 'h':
			default:
			  			err = 1;						
						cnt = argc;
						break;
			}
			  
			if (err != 0)
				cnt = argc;	
		}
	}
	
	
	if (err == 1)			
	{
		fprintf(stderr, "%s [options]\n", toolName);
		fprintf(stderr, "            -s <romsize>	ROM size in bytes\n");
		fprintf(stderr, "            -a <availrom>	(optional) # bytes in <romsize> we can use\n");
		fprintf(stderr, "            -v <version>	ROM version\n");
		fprintf(stderr, "            -b <rombase>	ROM base address\n");
		fprintf(stderr, "            -c <codefile>	ROM code filename\n");
		fprintf(stderr, "            -f <romfsfile>	ROM filesystem filename\n");
		fprintf(stderr, "            -o <outfile>	Output filename\n");
		fprintf(stderr, "			-e				If present, generate fake ecoff wrapper\n");
		fprintf(stderr, "			-p				If present and NO_ROM_FS, pad ROM out to <romsize>\n");
		fprintf(stderr, "			-h				Display this message\n");
	}
	
	
	return(err);
}


unsigned long SectionSize(char *name);
unsigned long SectionOffset(char *name);

unsigned long SectionSize(char *name)
{
unsigned long iii;

	for(iii=0;iii!=mfh.f_nscns;iii++)
	{
		if(!strcmp(msections[iii].name,name))
			return(msections[iii].length);		/* Sections MUST be word-aligned in ecoff file */
	}
	
	return 0;
}

unsigned long SectionOffset(char *name)
{
unsigned long iii;

	for(iii=0;iii!=mfh.f_nscns;iii++)
	{
		if(!strcmp(msections[iii].name,name))
			return(msections[iii].offset);
	}
	
	return 0;
}


unsigned long ChecksumROM(unsigned long *start, unsigned long *end)
{
unsigned long sum;
unsigned long *mem;

	sum = 0;
	
	for(mem = start; mem != end; mem++)
		sum += LONG(*mem);
	
	return sum;
}


main(int argc,char *argv[])
{
unsigned long dataStart;
unsigned long rDataStart;
unsigned long sDataStart;
unsigned long lit8Start;

unsigned long textSize;
unsigned long dataSize;
unsigned long rdataSize;
unsigned long sdataSize;
unsigned long lit8Size;		
long returnval = 0;		
int	i;

long	checksum;
unsigned char *romFSStart;
ROMFSHeader   *romFSHdr;
ROMFSNode   *romFSNode;

#ifdef	__macintosh
#ifdef applec
	if (parseCmdLine(argc, argv) != 0)			/* try to parse cmd line */
		return;
#else
	romFileName = "WebTV-Goober-MPW";
	romFSName = "ROMFS";
	outputFileName = "OUTROM";
	fakeECOFFSize = 0;	
	romSize = 0x00040000;
	buildVersion = 0x12345678;
	romBaseAddr = 0x9fc00000;
#endif
#else
	if (parseCmdLine(argc, argv) != 0)			/* try to parse cmd line */
		return;
#endif

	fprintf(stdout, "BuildROM tool start.  Target ROM size is %d bytes.\n",romSize);

	if(availROMSize == 0)						/* can we use it all? */
		availROMSize = romSize;

#ifdef	__macintosh
	gROMMem = TempNewHandle(romSize+fakeECOFFSize,&err);
	if (!gROMMem)
	{
		fprintf(stderr, "### %s: Couldn't malloc ROM buffer!\n", toolName);
		return(-1);
	}
	HLock(gROMMem);
	romMem = (unsigned char *)*gROMMem;			/* alloc the buf we'll fill in */
#else
	gROMMem = malloc(romSize+fakeECOFFSize);
	if ( gROMMem == 0 ) {
		fprintf(stderr, "### %s: Couldn't malloc ROM buffer!\n", toolName);
		return(-1);
	}
	romMem = (unsigned char *)gROMMem;
#endif

	ClearROMBuffer();										
	
	infp = fopen(romFileName,"rb");
	if (infp)
	{

		if(strcmp(romFSName,"NO_ROM_FS"))
		{
			romfsfp = fopen(romFSName,"rb");
			if (romfsfp)
			{
				fseek(romfsfp,0,SEEK_END);
				romFSSize = ftell(romfsfp);
				fseek(romfsfp,0,SEEK_SET);
			}
			else
			{
				fprintf(stderr, "### %s: Couldn't open ROM Filesystem file: %s\n", toolName, romFSName);
				returnval = -1;
				goto bail;
			}
		}
		else
			romFSSize = 0;
			

		fseek(infp,0,SEEK_END);
		romFileSize = ftell(infp);
		fseek(infp,0,SEEK_SET);

		fread(&fh, sizeof(fh), 1, infp);

		mfh = fh;
		mfh.f_magic = SHORT(fh.f_magic);
		mfh.f_nscns = SHORT(fh.f_nscns);
		mfh.f_timdat = LONG(fh.f_timdat);
		mfh.f_symptr = LONG(fh.f_symptr);
		mfh.f_nsyms = LONG(fh.f_nsyms);
		mfh.f_opthdr = SHORT(fh.f_opthdr);
		mfh.f_flags = SHORT(fh.f_flags);

		fread(&ah, sizeof(ah), 1, infp);
		mah = ah;
		mah.magic = SHORT(ah.magic);
		mah.vstamp = SHORT(ah.vstamp);
		mah.tsize = LONG(ah.tsize);
		mah.dsize = LONG(ah.dsize);
		mah.bsize = LONG(ah.bsize);
		mah.entry = LONG(ah.entry);
		mah.text_start = LONG(ah.text_start);
		mah.data_start = LONG(ah.data_start);
		mah.bss_start= LONG(ah.bss_start);
		mah.gprmask= LONG(ah.gprmask);
		mah.cprmask[0]= LONG(ah.cprmask[0]);
		mah.cprmask[1]= LONG(ah.cprmask[1]);
		mah.cprmask[2]= LONG(ah.cprmask[2]);
		mah.cprmask[3]= LONG(ah.cprmask[3]);
		mah.gpvalue= LONG(ah.gprmask);
		
		
		for ( i= 0;  i < mfh.f_nscns;  i++ ){
			fread(&sections[i], sizeof(secn),1, infp);
			msections[i] = sections[i];
			msections[i].start = LONG(sections[i].start);
			msections[i].start2 = LONG(sections[i].start2);
			msections[i].length = LONG(sections[i].length);
			msections[i].offset = LONG(sections[i].offset);
			msections[i].unk[0] = LONG(sections[i].unk[0]);
			msections[i].unk[1] = LONG(sections[i].unk[1]);
			msections[i].unk[2] = LONG(sections[i].unk[2]);
			msections[i].unk[3] = LONG(sections[i].unk[3]);
		}




		if ((mfh.f_nscns != 6) && (mfh.f_nscns != 7)) 
		{
			fprintf(stderr, "### %s: Aaugh!  This ecoff file has %d sections.  Tell Joe, I can't do any more.\n", toolName, mfh.f_nscns);
			returnval = -1;
			goto bail;
		}
		
		DumpStats();
						
				
		/* cache the sizes of the sections we're interested in */
		
		textSize = SectionSize(".text");
		dataSize = SectionSize(".data");
		rdataSize = SectionSize(".rdata");
		sdataSize = SectionSize(".sdata");
		lit8Size = SectionSize(".lit8");			/* returns 0 if no lit8 section */
				
						
		/* ROM code size is the sum of the text,data,rdata,sdata, and lit8 (if present) sections */
		
		romCodeSize = textSize + dataSize + rdataSize + sdataSize + lit8Size;
		
			
		if (fakeECOFFSize)											/* add wrapper for loader, if needed */
			workingPtr = romMem + AddFakeECOFFHeader(romMem);		/* if there is no ROMFS, it's the size of the code ONLY */
		else
			workingPtr = romMem;

		
		/* ROM is laid out .text->.rdata->.data->.lit8->.sdata */
		
		rDataStart = (unsigned long)workingPtr + textSize;
		dataStart = rDataStart + rdataSize;								
		if (lit8Size)
		{
			lit8Start = dataStart + dataSize;						
			sDataStart = lit8Start + lit8Size;							
		}
		else
			sDataStart = dataStart + dataSize;							


		/* In the ecoff file, the sections appear text->data->rdata->lit8->sdata->sbss->bss */
		
		if (romCodeSize + romFSSize + sizeof(ROMFSHeader) < availROMSize)
		{
			fseek(infp,SectionOffset(".text"),SEEK_SET);								/* move the file ptr to the text section */	
			fread(workingPtr,sizeof(char),textSize,infp);								/* place the text section FIRST */

			fseek(infp,SectionOffset(".data"),SEEK_SET);								/* move the file ptr to the data section */	
			fread((unsigned char *)dataStart,sizeof(char),dataSize,infp);						

			fseek(infp,SectionOffset(".rdata"),SEEK_SET);								/* move the file ptr to the rdata section */	
			fread((unsigned char *)rDataStart,sizeof(char),rdataSize,infp);

			if (lit8Size)
			{
				fseek(infp,SectionOffset(".lit8"),SEEK_SET);							/* move the file ptr to the lit8 section */	
				fread((unsigned char *)lit8Start,sizeof(char),lit8Size,infp);
				fseek(infp,SectionOffset(".sdata"),SEEK_SET);							/* move the file ptr to the sdata section */	
				fread((unsigned char *)sDataStart,sizeof(char),sdataSize,infp);
			}
			else
			{
				fseek(infp,SectionOffset(".sdata"),SEEK_SET);							/* move the file ptr to the sdata section */	
				fread((unsigned char *)sDataStart,sizeof(char),sdataSize,infp);
			}
			
				
			/* fill in the ROM header 						*/
			/* .data is just beyond .text & .rdata blocks 	*/
			/* lengths & sizes are in WORDS					*/
				
			((ROMHeader *)workingPtr)->rom_size = LONG(romSize >> 2);						
			((ROMHeader *)workingPtr)->code_size = LONG(romCodeSize >> 2);						
			((ROMHeader *)workingPtr)->version = LONG(buildVersion);						
			((ROMHeader *)workingPtr)->data_start = LONG(romBaseAddr + textSize + rdataSize);			
			((ROMHeader *)workingPtr)->data_length = LONG((dataSize + lit8Size + sdataSize) >> 2);				
			((ROMHeader *)workingPtr)->bss_length = LONG((SectionSize(".sbss") + SectionSize(".bss")) >> 2);
			if(romFSSize)
			{
				((ROMHeader *)workingPtr)->romfs_address = LONG(0xa0000000 - 0x4000);
			}
			else
			{
				((ROMHeader *)workingPtr)->romfs_address = LONG(0x9fe00000); // EMAC: hack so the default bfe bootrom doesn't reject this build.
			}
			
			/* copy in the ROM Filesystem, if desired */
				
			if(romFSSize)
			{
				romFSStart = (romMem + romSize + fakeECOFFSize - sizeof(ROMFSHeader)) - romFSSize;		/* place just under FS Header */
				fread(romFSStart,sizeof(char),romFSSize,romfsfp);
				fclose(romfsfp);
				
				if(romFSSize)
				{
					romFSNode = (ROMFSNode *)((unsigned long)((romFSStart+romFSSize) - 0x38));
					romFSNode->emac_cool = LONG(romfsNext);

					romFSHdr = (ROMFSHeader *)((unsigned long)romFSStart+romFSSize);
					romFSHdr->romfs_length = LONG(romFSSize >> 2);
					romFSHdr->romfs_checksum = LONG(ChecksumROM((unsigned long *)romFSStart, (unsigned long *)((unsigned long)romFSHdr-4)));
				}
			}

			{
			unsigned long *startCode;
			unsigned long *endCode;
				
				startCode = (unsigned long *)workingPtr;
				endCode = (unsigned long *)((unsigned long)workingPtr + romCodeSize);		/* includes header */
				checksum = ChecksumROM(startCode, endCode);
				((ROMHeader *)workingPtr)->rom_checksum = LONG(checksum);						
			}
			
			fprintf(stdout, "--- Opening output file: %s\n", outputFileName);
			
			outfp = fopen(outputFileName,"wb");
			if (outfp == 0)
			{
				fprintf(stderr, "### %s: Couldn't open output file: %s\n", toolName, outputFileName);
				returnval = -1;
			}
			else
			{
				if (romFSSize || gPad)
					fwrite(romMem,sizeof(char),romSize+fakeECOFFSize,outfp);			/* there is a ROM FS */
				else
					fwrite(romMem,sizeof(char),romCodeSize+fakeECOFFSize,outfp);		/* just code section */
				fclose(outfp);
					
				fprintf(stdout, "--- ROM Checksum = 0x%x\n", checksum);
				fprintf(stdout, "--- ROM Filesystem occupies 0x%x (%d) bytes\n", romFSSize, romFSSize);
				fprintf(stdout, "--- ROM code occupies 0x%x (%d) bytes\n", romCodeSize, romCodeSize);
				fprintf(stdout, "--- ROM space available 0x%x (%d) bytes\n", availROMSize, availROMSize);
				fprintf(stderr, "--- Free space left in ROM: 0x%x (%d) bytes\n", romSize - (romCodeSize + romFSSize),
					romSize - (romCodeSize + romFSSize));
			}
		}
		else
		{
			fprintf(stderr, "### %s: Can't fit in ROM: code size = %d bytes, filesystem size = %d bytes, rom is %d bytes\n", toolName, romCodeSize, romFSSize,availROMSize);
			returnval = -2;
		}
			
		fclose(infp);
	}
	else
	{
		fprintf(stderr, "### %s: Couldn't open ROM Code file: %s\n", toolName, romFileName);
		returnval = -2;
	}
	
bail:		
#ifdef	__macintosh
	HUnlock(gROMMem);
	DisposeHandle(gROMMem);
#else
	free(gROMMem);
#endif
	return(returnval);
}

