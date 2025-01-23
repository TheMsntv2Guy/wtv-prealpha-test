
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "elf.h"

typedef unsigned char	uchar;
typedef unsigned long	ulong;

/* We still need these ECOFF defs for the fake ecoff header */

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
	char		name[8];
	ulong		longs[8];
} secnhdr;

typedef struct
{
	char		name[8];
	ulong		start;
	ulong		start2;
	ulong 		length;
	ulong		offset;
	ulong 		unk[4];
} secn;


typedef struct
{
	filhdr		file_h;
	aouthdr		aout_h;
	secnhdr		text_h;			
	secnhdr		data_h;			
	ulong		pad;
} fakeECOFFHeader;



#define SCNHSZ 40
#define	TXT_OFFSET(f)	((sizeof(filhdr) + f.f_opthdr + f.f_nscns * SCNHSZ + 7) & ~7)


typedef struct 
{
	ulong		branch;				/* don't whack this, it's the reset instruction! */
	ulong 		nop;				/* don't whack this, it's the delay nop! */
	ulong 		rom_checksum;		/* checksum, NOT including the checksum itself */
	ulong 		rom_size;			/* size in bytes */
	ulong		version;			/* passed in as cmd line arg when buildrom is invoked */

	ulong		data_start;			/* offset into ROM to .data section */
	ulong		data_addr;
	ulong		data_length;

	ulong		sdata_addr;			/* sdata follows data */
	ulong		sdata_length;
	
	ulong		sbss_start;			
	ulong		sbss_length;

	ulong		bss_start;			
	ulong		bss_length;

} ROMHeader;



FILE *infp;
FILE *romfsfp;
FILE *outfp;
int size = 0;

Elf32_Ehdr	eh;
Elf32_Phdr	ph[16];

uchar *workingPtr;				/* used as we fill in the rom, increments. */
uchar *romMem;					/* always points at allocated block */

ulong romSize			= 65536;
ulong romOffset;
ulong romFileSize;
ulong romCodeSize;
ulong romFSSize;
ulong buildVersion		= 0x01020304;
ulong fakeECOFFSize 	= 0;
ulong romBaseAddr		= 0x80100000;

char *romFileName 		= "BOXROM";
char *romFSName 		= "ROMFS";
char *outputFileName	= "OUTROM";





ulong AddFakeECOFFHeader(uchar *mem);

ulong AnalyzeElfHeader(void);
ulong AnalyzeProgramHeaders(void);

void 		  ClearROMBuffer(void);

ulong getVal(char *s, ulong *val);
ulong parseCmdLine(ulong argc, char *argv[]);

ulong ChecksumROM(void);


ulong AddFakeECOFFHeader(uchar *mem)
{
fakeECOFFHeader *ecoff;

	ecoff = (fakeECOFFHeader *)mem;
	
	ecoff->file_h.f_magic  		= 0x0160;
	ecoff->file_h.f_nscns  		= 0x0002;
	ecoff->file_h.f_timdat		= 0x00000000;
	ecoff->file_h.f_symptr 		= 0x00000000;
	ecoff->file_h.f_nsyms  		= 0x00000000;
	ecoff->file_h.f_opthdr 		= sizeof(aouthdr);
	ecoff->file_h.f_flags  		= 0x0207;
	
	ecoff->aout_h.magic			= 0x0107;
	ecoff->aout_h.vstamp		= 0x020b;
	if(romFSSize)
		ecoff->aout_h.tsize		= romSize;
	else
		ecoff->aout_h.tsize		= romCodeSize;		/* code-only builds are built to just the right size */
	ecoff->aout_h.dsize			= 0;
	ecoff->aout_h.bsize			= 0;
	ecoff->aout_h.entry			= 0x80200000;
	ecoff->aout_h.text_start	= 0x80200000;
	ecoff->aout_h.data_start	= 0x80300000;
	ecoff->aout_h.bss_start		= 0;
	ecoff->aout_h.gprmask		= 0xF407FFFE;
	ecoff->aout_h.cprmask[0]	= 0;
	ecoff->aout_h.cprmask[1]	= 0;
	ecoff->aout_h.cprmask[2]	= 0;
	ecoff->aout_h.cprmask[3]	= 0;
	ecoff->aout_h.gpvalue		= 0;			/* we'll fix this up in the ROM loader */
	
	strcpy(ecoff->text_h.name, ".text");
	ecoff->text_h.longs[0] = 0x80200000;
	ecoff->text_h.longs[1] = 0x80200000;
	ecoff->text_h.longs[2] = romSize;
	ecoff->text_h.longs[3] = sizeof(fakeECOFFHeader);
	ecoff->text_h.longs[4] = 0x00000000;
	ecoff->text_h.longs[5] = 0x00000000;
	ecoff->text_h.longs[6] = 0x00000000;
	ecoff->text_h.longs[7] = 0x00000020;
			
	strcpy(ecoff->text_h.name, ".data");
	ecoff->text_h.longs[0] = 0x80300000;
	ecoff->text_h.longs[1] = 0x80300000;
	ecoff->text_h.longs[2] = 0;
	ecoff->text_h.longs[3] = sizeof(fakeECOFFHeader);
	ecoff->text_h.longs[4] = 0x00000000;
	ecoff->text_h.longs[5] = 0x00000000;
	ecoff->text_h.longs[6] = 0x00000000;
	ecoff->text_h.longs[7] = 0x00000020;
			
	ecoff->pad = 0;
	
	return(sizeof(fakeECOFFHeader));
}


ulong AnalyzeElfHeader(void)
{
	if ( (eh.e_ident[EI_MAG0] != ELFMAG0) || 
		 (eh.e_ident[EI_MAG1] != ELFMAG1) || 
		 (eh.e_ident[EI_MAG2] != ELFMAG2) || 
		 (eh.e_ident[EI_MAG3] != ELFMAG3) )
	{
		printf("### Not an ELF file!\n");
		return 0;
	}
		
	if ( eh.e_ident[EI_CLASS] != ELFCLASS32 )
	{
		printf("### ELF file is not 32-bit!\n");
		return 0;
	}
	
	if ( eh.e_ident[EI_DATA] != ELFDATA2MSB )
	{
		printf("### ELF file is not Big Endian!\n");
		return 0;
	}

	if ( eh.e_ident[EI_VERSION] != EV_CURRENT )
	{
		printf("### ELF file has bad version!\n");
		return 0;
	}

	if ( eh.e_machine != EM_MIPS )
	{
		printf("### ELF file is not for MIPS!\n");
		return 0;
	}
	
	if ( eh.e_phoff == 0 || eh.e_phnum == 0 || eh.e_phnum > 16 || eh.e_phentsize != sizeof(Elf32_Phdr) )
	{
		printf("### ELF file Program Header invalid!\n");
		return 0;
	}

	printf("--- ELF: Offset to Program Header: %d\n",eh.e_phoff);
	printf("--- ELF: Number of Program Header entries: %d\n",eh.e_phnum);
	
	printf("--- ROM Base is at 0x%08x\n",romBaseAddr);
}


long	textSect = -1;
long	rdataSect = -1;
long	dataSect1 = -1;
long	dataSect2 = -1;
long	bssSect1 = -1;
long	bssSect2 = -1;


/* assumes that there is always 1 text section, 1 rdata section, 1 or 2 data sections, and 1 or 2 bss sections */

ulong AnalyzeProgramHeaders(void)
{
ulong iii;

	for (iii = 0; iii != eh.e_phnum; iii++)
	{
		if ( ph[iii].p_flags == (PF_X | PF_R) )			/* .text? */
		{
			if (textSect == -1)
				textSect = iii;
			else
			{
				printf("### Error: multiple text sections detected!\n");
				return 0;
			}
		}	

		if ( ph[iii].p_flags == PF_R )					/* .rdata? */
		{
			if (rdataSect == -1)
				rdataSect = iii;
			else
			{
				printf("### Error: multiple rdata sections detected!\n");
				return 0;
			}
		}	
	
		if ( ph[iii].p_flags == (PF_R | PF_W) )			/* .data, .sdata, .bss, or .sbss? */
		{
			if ( ph[iii].p_filesz )						/* bss sects have 0 size */
			{
				if (dataSect1 == -1)
					dataSect1 = iii;
				else
					if (dataSect2 == -1)
						dataSect2 = iii;
					else
					{
						printf("### Error: more than 2 data sections detected!\n");
						return 0;
					}
			}
			else
			{
				if (bssSect1 == -1)
					bssSect1 = iii;
				else
					if (bssSect2 == -1)
						bssSect2 = iii;
					else
					{
						printf("### Error: more than 2 bss sections detected!\n");
						return 0;
					}
			}
		}
	}


	printf("--- text:  offset = %d, vaddr = 0x%x, memsz = 0x%x\n",
						ph[textSect].p_offset, ph[textSect].p_vaddr, ph[textSect].p_memsz);

	printf("--- rdata: offset = %d, vaddr = 0x%x, memsz = 0x%x\n",
						ph[rdataSect].p_offset, ph[rdataSect].p_vaddr, ph[rdataSect].p_memsz);

	if (dataSect1 != -1)
	{
		printf("--- data1:  offset = %d, vaddr = 0x%x, memsz = 0x%x\n",
						ph[dataSect1].p_offset, ph[dataSect1].p_vaddr, ph[dataSect1].p_memsz);
	}
	else
	{
		printf("### Error: no data section found!\n");
		return 0;
	}

	if (dataSect2 != -1)
		printf("--- data2: offset = %d, vaddr = 0x%x, memsz = 0x%x\n",
						ph[dataSect2].p_offset, ph[dataSect2].p_vaddr, ph[dataSect2].p_memsz);

	if (bssSect1 != -1)
	{
		printf("--- bss1:  vaddr = 0x%x, memsz = 0x%x\n",
						ph[bssSect1].p_vaddr, ph[bssSect1].p_memsz);
	}
	else
	{
		printf("### Error: no bss section found!\n");
		return 0;
	}

	if (bssSect2 != -1)
		printf("--- bss2:  vaddr = 0x%x, memsz = 0x%x\n",
						ph[bssSect2].p_vaddr, ph[bssSect2].p_memsz);
	
}


void ClearROMBuffer(void)
{
ulong offset;

	for (offset = 0; offset != romSize+fakeECOFFSize; offset += 4)		/* init the ROM space */
		*(ulong *)((ulong)romMem + offset) = 'joeb';
};



// ----------------------------------------------------
ulong getVal(char *s, ulong *val) 
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

// ----------------------------------------------------
ulong parseCmdLine(ulong argc, char *argv[])
{
char *s;
ulong cnt;
ulong err;

  err = 0;										// assume no err

	if (argc != 1)								
	{
		for (cnt = 1; cnt != argc; cnt++)
		{
			s = argv[cnt];
			switch (s[1])
			{
			case 's':
			  			cnt++;
						err = getVal(argv[cnt], &romSize);		
						break;
			case 'v':
			  			cnt++;
						err = getVal(argv[cnt], &buildVersion);	
						break;
			case 'b':
			  			cnt++;
						err = getVal(argv[cnt], &romBaseAddr);	
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
		printf("BuildROM [options]\n");
		printf("            -s <romsize>	ROM size in bytes\n");
		printf("            -v <version>	ROM version\n");
		printf("            -b <rombase>	ROM base address\n");
		printf("            -c <codefile>	ROM code filename\n");
		printf("            -f <romfsfile>	ROM filesystem filename\n");
		printf("            -o <outfile>	Output filename\n");
		printf("			-e				If present, generate fake ecoff wrapper\n");
		printf("			-h				Display this message\n");
	}
	
	
	return(err);
}



ulong ChecksumROM(void)
{
ulong sum;
ulong *mem;
ulong iii;

	mem = (ulong *)workingPtr;
	sum = 0;
	
	for(iii = 0; iii != (romSize>>2); iii++)
	{
		sum += *mem;
		mem++;
	}
	
	return sum;
}


main(int argc,char *argv[])
{
ulong dataStart;
ulong rDataStart;
ulong sDataStart;

ulong rDataOffset;

ulong textSize;
ulong dataSize;
ulong rdataSize;
ulong sdataSize;

uchar *romFSStart;


#ifdef applec
	if (parseCmdLine(argc, argv) != 0)			/* try to parse cmd line */
		return;
#else
	romFileName = "MIPS.exe";
	romFSName = "NO_ROM_FS";
	outputFileName = "OUTROM";
	fakeECOFFSize = sizeof(fakeECOFFHeader);	
	romSize = 0x00180000;
	buildVersion = 0x12345678;
	romBaseAddr = 0xA0200000;
#endif
	
	printf("BuildROM tool start.  Target ROM size is %d bytes.\n",romSize);


	romMem = (uchar *)malloc(romSize+fakeECOFFSize);			/* alloc the buf we'll fill in */
	if (!romMem)
	{
		printf("### Couldn't malloc ROM buffer!\n");
		return;
	}

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
				printf("### Couldn't open ROM Filesystem file: %s\n",romFSName);
				return(0);
			}
		}
		else
			romFSSize = 0;
			

		fseek(infp,0,SEEK_END);
		romFileSize = ftell(infp);
		fseek(infp,0,SEEK_SET);

		/* read Elf Header, analyze */

		fread(&eh, sizeof(eh), 1, infp);
				
		if (AnalyzeElfHeader())
		{			
			/* read the Program Headers, analyze */
			
			fseek(infp, eh.e_phoff, SEEK_SET);
			fread(&ph, eh.e_phentsize, eh.e_phnum, infp);	
			
			if (AnalyzeProgramHeaders())
			{	
				/* AnalyzeProgramHeaders() will have set up the section indexes for us */
				
				textSize 	= ph[textSect].p_memsz;	
				textSize 	= ((textSize + 3) & ~3);		/* round to 4 bytes */
				
				rdataSize	= ph[rdataSect].p_memsz;		
				rdataSize 	= ((rdataSize + 3) & ~3);		/* round to 4 bytes */

				dataSize 	= ph[dataSect1].p_memsz;		
				dataSize 	= ((dataSize + 3) & ~3);		/* round to 4 bytes */

				if (dataSect2 != -1)
				{
					sdataSize 	= ph[dataSect2].p_memsz;		
					sdataSize 	= ((sdataSize + 3) & ~3);	/* round to 4 bytes */
				}
				else
					sdataSize = 0;						
						
				/* ROM code size is the sum of the text, data, rdata, and sdata sections */

				rDataOffset = ph[rdataSect].p_vaddr - ph[textSect].p_vaddr;		/* can be a gap between text and rdata */
			
				printf("--- text->rdata gap = 0x%x (%d) bytes\n", rDataOffset - textSize, rDataOffset - textSize);
			
				romCodeSize = textSize + rdataSize + dataSize + sdataSize + (rDataOffset - textSize);
		
			
				if (fakeECOFFSize)											/* add wrapper for loader, if needed */
					workingPtr = romMem + AddFakeECOFFHeader(romMem);		/* if there is no ROMFS, it's the size of the code ONLY */
				else
					workingPtr = romMem;
		
		
				/* ROM is laid out .text->.rdata->.data->->.sdata */

				rDataStart = (ulong)workingPtr + rDataOffset;			
				dataStart = rDataStart + rdataSize;								
				sDataStart = dataStart + dataSize;							
		
				if (romCodeSize + romFSSize < romSize)
				{
					fseek(infp, ph[textSect].p_offset, SEEK_SET);				/* move the file ptr to the text section */	
					fread(workingPtr,sizeof(char),textSize,infp);				/* place the text section FIRST */
					
					fseek(infp, ph[rdataSect].p_offset, SEEK_SET);				/* move the file ptr to the rdata section */	
					fread((uchar *)rDataStart,sizeof(char),rdataSize,infp);		/* rdata comes next */

					fseek(infp, ph[dataSect1].p_offset, SEEK_SET);				/* move the file ptr to the data section */	
					fread((uchar *)dataStart,sizeof(char),dataSize,infp);		/* data comes next */
						
					if (dataSect2 != -1)
					{
						fseek(infp, ph[dataSect2].p_offset, SEEK_SET);				/* move the file ptr to the sdata section */	
						fread((uchar *)sDataStart,sizeof(char),sdataSize,infp);		/* sdata comes next */
					}

					/* fill in the ROM header 						*/
					/* .data is just beyond .text & .rdata blocks 	*/
					/* lengths are in WORDS 						*/
				
					((ROMHeader *)workingPtr)->rom_size = romSize;						
					((ROMHeader *)workingPtr)->version = buildVersion;						
					
					((ROMHeader *)workingPtr)->data_start = romBaseAddr + rDataOffset + rdataSize;			

					((ROMHeader *)workingPtr)->data_addr = ph[dataSect1].p_vaddr;			
					((ROMHeader *)workingPtr)->data_length = dataSize >> 2;					/* already rounded */		
							
					if (dataSect2 != -1)
					{
						((ROMHeader *)workingPtr)->sdata_addr = ph[dataSect2].p_vaddr;			
						((ROMHeader *)workingPtr)->sdata_length = sdataSize >> 2;			/* already rounded */		
					}
					else
					{
						((ROMHeader *)workingPtr)->sdata_addr = 0;			
						((ROMHeader *)workingPtr)->sdata_length = 0;						
					}
					
					((ROMHeader *)workingPtr)->bss_start = ph[bssSect1].p_vaddr;					
					((ROMHeader *)workingPtr)->bss_length = ((ph[bssSect1].p_memsz + 3) & ~3) >> 2;		/* round, div by 4 */				

					if (bssSect2 != -1)
					{
						((ROMHeader *)workingPtr)->sbss_start = ph[bssSect2].p_vaddr;					
						((ROMHeader *)workingPtr)->sbss_length = ((ph[bssSect2].p_memsz + 3) & ~3) >> 2;	/* round, div by 4 */				
					}
					else
					{
						((ROMHeader *)workingPtr)->sbss_start = 0;					
						((ROMHeader *)workingPtr)->sbss_length = 0;				
					}
					
					/* copy in the ROM Filesystem, if desired */
				
					if(romFSSize)
					{
						romFSStart = (romMem + romSize + fakeECOFFSize) - romFSSize;
						fread(romFSStart,sizeof(char),romFSSize,romfsfp);				
						fclose(romfsfp);
					}

					((ROMHeader *)workingPtr)->rom_checksum = ChecksumROM();						
			
					outfp = fopen(outputFileName,"wb");
					fwrite(romMem,sizeof(char),romSize+fakeECOFFSize,outfp);
					fclose(outfp);
				
					printf("--- ROM Checksum = 0x%x\n", ((ROMHeader *)workingPtr)->rom_checksum);
					printf("--- ROM Filesystem occupies 0x%x (%d) bytes\n", romFSSize, romFSSize);
					printf("--- ROM code occupies 0x%x (%d) bytes\n", romCodeSize, romCodeSize);
					printf("--- Free space left in ROM: 0x%x (%d) bytes\n", romSize - (romCodeSize + romFSSize), 
																			romSize - (romCodeSize + romFSSize));
				}
				else
					printf("### ROM is TOO SMALL!  code size = %d bytes, filesystem size = %d bytes\n", romCodeSize, romFSSize);
			
			}
			else
				printf("### Error parsing program headers.  Tell Joe you saw this.\n");
		}
		
		fclose(infp);
	}
	else
		printf("### Couldn't open ROM Code file: %s\n",romFileName);
		
		

	free(romMem);
	
	return(0);
}

