#include "WTVTypes.h"
#include "BoxBoot.h"
#include "ObjectStore.h"
#include "Debug.h"
#include "Checksum.h"


#ifndef APPROM
Boolean ROMCodeChecksumOK(ulong base, ulong size)
{
ulong *rom;
ulong romsize, codesize;
ulong sum;
ulong iii;

	/* Checksum Code */
	
	rom = (ulong *)base;
	
	romsize = (((ROMHeader *)base)->rom_size);
	codesize = (((ROMHeader *)base)->code_size);

	Message(("ROM Size = 0x%lx, Code Size = 0x%lx", romsize << 2, codesize << 2));
	
	if(romsize & 0xff000000)
	{
		Message(("ROM Size looks bogus."));
		return false;
	}
	
	if(codesize & 0xff000000)
	{
		Message(("Code Size looks bogus."));
		return false;
	}
	
	sum = *rom++;		/* branch */
	sum += *rom++;		/* nop */
	rom++;				/* skip over the checksum */
	
	codesize -= 3;		/* we've already done 3 of em */
	
	for(iii=0;iii!=codesize;iii++)
		sum += *rom++;
		
	if (sum == (((ROMHeader *)base)->rom_checksum))
	{
		Message(("ROM Code checksum OK: 0x%lx", sum));
		return true;
	}
	else
	{
		Message(("ROM Code checksum FAILED: should be 0x%lx, computed 0x%lx",(((ROMHeader *)base)->rom_checksum),sum));	
		return true;
	}
}
#endif


#ifndef APPROM
Boolean ROMFSChecksumOK(ulong base, ulong size)
{
ulong *rom;
ulong sum;
ulong iii;
ROMFSHeader *romFSHdr;

	/* Checksum ROM FS */
	
	romFSHdr = (ROMFSHeader *)((base + size) - sizeof(ROMFSHeader));
	Message(("ROMFSHdr @ %lx",(ulong)romFSHdr));

	if(romFSHdr->romfs_length & 0xff000000)
	{
		Message(("romfs_length looks bogus."));
		return false;
	}
	
	rom = (ulong *)((ulong)romFSHdr - ((romFSHdr->romfs_length) << 2));
	Message(("ROM FS bottom @ %lx", (ulong)rom));
	
	sum = 0;
	
	for(iii=0;iii!=romFSHdr->romfs_length;iii++)
		sum += *rom++;
		
	if (sum == romFSHdr->romfs_checksum)
	{
		Message(("ROM FS checksum OK: 0x%lx", sum));
		return true;
	}
	else
	{
		Message(("ROM FS checksum FAILED: should be 0x%lx, computed 0x%lx",romFSHdr->romfs_checksum,sum));
		return false;
	}
}
#endif
