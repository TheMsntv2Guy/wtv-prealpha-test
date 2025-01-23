/* ELF support for BFD.
   Copyright (C) 1991, 1992 Free Software Foundation, Inc.

   Written by Fred Fish @ Cygnus Support, from information published
   in "UNIX System V Release 4, Programmers Guide: ANSI C and
   Programming Support Tools".

   Modified by Paul Reger, Intel

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Data Representation */

typedef unsigned long  Elf32_Addr;
typedef unsigned short Elf32_Half;
typedef unsigned long  Elf32_Off;
typedef long           Elf32_Sword;
typedef unsigned long  Elf32_Word;
/* typedef unsigned char ... */


/* Fields in e_ident[] */

#define EI_NIDENT       16

#define EI_MAG0		0		/* File identification byte 0 index */
#define ELFMAG0		0x7F		/* Magic number byte 0 */

#define EI_MAG1		1		/* File identification byte 1 index */
#define ELFMAG1		'E'		/* Magic number byte 1 */

#define EI_MAG2		2		/* File identification byte 2 index */
#define ELFMAG2		'L'		/* Magic number byte 2 */

#define EI_MAG3		3		/* File identification byte 3 index */
#define ELFMAG3		'F'		/* Magic number byte 3 */

#define EI_CLASS	4		/* File class */
#define ELFCLASSNONE	0		/* Invalid class */
#define ELFCLASS32	1		/* 32-bit objects */
#define ELFCLASS64	2		/* 64-bit objects */

#define EI_DATA		5		/* Data encoding */
#define ELFDATANONE	0		/* Invalid data encoding */
#define ELFDATA2LSB	1		/* 2's complement, little endian */
#define ELFDATA2MSB	2		/* 2's complement, big endian */

#define EI_VERSION	6		/* File version */

#define EI_PAD		7		/* Start of padding bytes */


/* Values for e_type, which identifies the object file type */

#define ET_NONE		0		/* No file type */
#define ET_REL		1		/* Relocatable file */
#define ET_EXEC		2		/* Executable file */
#define ET_DYN		3		/* Shared object file */
#define ET_CORE		4		/* Core file */
#define ET_LOPROC	0xFF00		/* Processor-specific */
#define ET_HIPROC	0xFFFF		/* Processor-specific */

/* Values for e_machine, which identifies the architecture */

#define EM_NONE		0	/* No machine */
#define EM_M32		1	/* AT&T WE 32100 */
#define EM_SPARC	2	/* SUN SPARC */
#define EM_386		3	/* Intel 80386 */
#define EM_68K		4	/* Motorola m68k family */
#define EM_88K		5	/* Motorola m88k family */
#define EM_860		7	/* Intel 80860 */
#define EM_MIPS		8	/* MIPS R3000 (officially, big-endian only) */
#define EM_HPPA		9	/* HP PA-RISC (not officially assigned?) */
#define EM_MIPS_RS4_BE 10	/* MIPS R4000 big-endian */
#define EM_960	       960   	/* Intel 80960 */

/* Values for e_version */

#define EV_NONE		0		/* Invalid ELF version */
#define EV_CURRENT	1		/* Current version */

/* Values for program header, p_type field */

#define	PT_NULL		0		/* Program header table entry unused */
#define PT_LOAD		1		/* Loadable program segment */
#define PT_DYNAMIC	2		/* Dynamic linking information */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved, unspecified semantics */
#define PT_PHDR		6		/* Entry for header table itself */
#define PT_LOPROC	0x70000000	/* Processor-specific */
#define PT_HIPROC	0x7FFFFFFF	/* Processor-specific */

/* Program segment permissions, in program header p_flags field */

#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */
#define PF_MASKPROC	0xF0000000	/* Processor-specific reserved bits */

/* Values for section header, sh_type field */

#define SHT_NULL	0		/* Section header table entry unused */
#define SHT_PROGBITS	1		/* Program specific (private) data */
#define SHT_SYMTAB	2		/* Link editing symbol table */
#define SHT_STRTAB	3		/* A string table */
#define SHT_RELA	4		/* Relocation entries with addends */
#define SHT_HASH	5		/* A symbol hash table */
#define SHT_DYNAMIC	6		/* Information for dynamic linking */
#define SHT_NOTE	7		/* Information that marks file */
#define SHT_NOBITS	8		/* Section occupies no space in file */
#define SHT_REL		9		/* Relocation entries, no addends */
#define SHT_SHLIB	10		/* Reserved, unspecified semantics */
#define SHT_DYNSYM	11		/* Dynamic linking symbol table */
#define SHT_LOPROC	0x70000000	/* Processor-specific semantics, lo */
#define SHT_HIPROC	0x7FFFFFFF	/* Processor-specific semantics, hi */
#define SHT_LOUSER	0x80000000	/* Application-specific semantics */
#define SHT_HIUSER	0x8FFFFFFF	/* Application-specific semantics */

/* Values for section header, sh_flags field */

#define SHF_WRITE	(1 << 0)	/* Writable data during execution */
#define SHF_ALLOC	(1 << 1)	/* Occupies memory during execution */
#define SHF_EXECINSTR	(1 << 2)	/* Executable machine instructions */
#define SHF_BIGENDIAN	(1 << 28)	/* Byte order is big-endian */
#define SHF_MASKPROC	0xF0000000	/* Processor-specific semantics */

/* Values of note segment descriptor types for core files. */

#define NT_PRSTATUS	1		/* Contains copy of prstatus struct */
#define NT_FPREGSET	2		/* Contains copy of fpregset struct */
#define NT_PRPSINFO	3		/* Contains copy of prpsinfo struct */

/* Values of note segment descriptor types for object files.  */
/* (Only for hppa right now.  Should this be moved elsewhere?)  */

#define NT_VERSION	1		/* Contains a version string.  */

/* These three macros disassemble and assemble a symbol table st_info field,
   which contains the symbol binding and symbol type.  The STB_ and STT_
   defines identify the binding and type. */

#define ELF32_ST_BIND(val)		(((unsigned int)(val)) >> 4)
#define ELF32_ST_TYPE(val)		((val) & 0xF)
#define ELF32_ST_INFO(bind,type)	(((bind) << 4) + ((type) & 0xF))

#define STN_UNDEF	0		/* undefined symbol index */

#define STB_LOCAL	0		/* Symbol not visible outside obj */
#define STB_GLOBAL	1		/* Symbol visible outside obj */
#define STB_WEAK	2		/* Like globals, lower precedence */
#define STB_LOPROC	13		/* Application-specific semantics */
#define STB_HIPROC	15		/* Application-specific semantics */

#define STT_NOTYPE	0		/* Symbol type is unspecified */
#define STT_OBJECT	1		/* Symbol is a data object */
#define STT_FUNC	2		/* Symbol is a code object */
#define STT_SECTION	3		/* Symbol associated with a section */
#define STT_FILE	4		/* Symbol gives a file name */
#define STT_LOPROC	13		/* Application-specific semantics */
#define STT_HIPROC	15		/* Application-specific semantics */

/* Special section indices, which may show up in st_shndx fields, among
   other places. */

#define SHN_UNDEF	0		/* Undefined section reference */
#define SHN_LORESERV	0xFF00		/* Begin range of reserved indices */
#define SHN_LOPROC	0xFF00		/* Begin range of appl-specific */
#define SHN_HIPROC	0xFF1F		/* End range of appl-specific */
#define SHN_ABS		0xFFF1		/* Associated symbol is absolute */
#define SHN_COMMON	0xFFF2		/* Associated symbol is in common */
#define SHN_HIRESERVE	0xFFFF		/* End range of reserved indices */

#define SHN_960_LEAF    0xff00
#define SHN_960_SYSPROC 0xff01

/* relocation info handling macros */

#define ELF32_R_SYM(i)		((i)>>8)
#define ELF32_R_TYPE(i)		((unsigned char)(i))
#define ELF32_R_INFO(s,t)	(((s)<<8)+(unsigned char)(t))

#define ELF64_R_SYM(i)		((i)>>32)
#define ELF64_R_TYPE(i)		((Elf64_Word)(i))
#define ELF64_R_INFO(s,t)	(((Elf64_Xword)(s)<<32)+(Elf64_Xword)(t))

#define R_960_NONE     0
#define R_960_12       1
#define R_960_32       2
#define R_960_IP24     3
#define R_960_SUB      4
#define R_960_OPTCALL  5
#define R_960_OPTCALLX 6

/* Dynamic section tags */

#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH	15
#define DT_SYMBOLIC	16
#define DT_REL		17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff


typedef struct Elf32_Ehdr
{
    unsigned char	e_ident[EI_NIDENT];	/* ELF "magic number" */
    Elf32_Half 	e_type;			/* Identifies object file type */
    Elf32_Half	e_machine;		/* Specifies required architecture */
    Elf32_Word	e_version;		/* Identifies object file version */
    Elf32_Addr	e_entry;		/* Entry point virtual address */
    Elf32_Off	e_phoff;		/* Program header table file offset */
    Elf32_Off	e_shoff;		/* Section header table file offset */
    Elf32_Word	e_flags;		/* Processor-specific flags */
    Elf32_Half	e_ehsize;		/* ELF header size in bytes */
    Elf32_Half	e_phentsize;		/* Program header table entry size */
    Elf32_Half	e_phnum;		/* Program header table entry count */
    Elf32_Half	e_shentsize;		/* Section header table entry size */
    Elf32_Half	e_shnum;		/* Section header table entry count */
    Elf32_Half	e_shstrndx;		/* Section header string table index */
} Elf32_Ehdr;

/* Section header */

typedef struct {
    Elf32_Word	sh_name;	/* Section name, index in string tbl */
    Elf32_Word	sh_type;	/* Type of section */
    Elf32_Word	sh_flags;	/* Miscellaneous section attributes */
    Elf32_Addr	sh_addr;	/* Section virtual addr at execution */
    Elf32_Off	sh_offset;	/* Section file offset */
    Elf32_Word	sh_size;	/* Size of section in bytes */
    Elf32_Word	sh_link;	/* Index of another section */
    Elf32_Word	sh_info;	/* Additional section information */
    Elf32_Word	sh_addralign;	/* Section alignment */
    Elf32_Word	sh_entsize;	/* Entry size if section holds table */
} Elf32_Shdr;

/* Symbol table entry */

typedef struct {
    Elf32_Word	st_name;	/* Symbol name, index in string tbl */
    Elf32_Addr	st_value;	/* Value of the symbol */
    Elf32_Word	st_size;	/* Associated symbol size */
    unsigned char	st_info;	/* Type and binding attributes */
    unsigned char	st_other;	/* No defined meaning, 0 */
    Elf32_Half	st_shndx;	/* Associated section index */
} Elf32_Sym;

/* Relocation Entries */
typedef struct {
  Elf32_Addr	r_offset;	/* Location at which to apply the action */
  Elf32_Word	r_info;		/* index and type of relocation */
} Elf32_Rel;

#if 0

Intel 960 does not use these types of relocations.

typedef struct {
  Elf32_Word r_offset;	/* Location at which to apply the action */
  Elf32_Word r_info;	/* index and type of relocation */
  long		 r_addend;	/* Constant addend used to compute value */
} Elf32_Rela;

#endif

/* Program header */

typedef struct {
    Elf32_Word	p_type;		/* Identifies program segment type */
    Elf32_Off	p_offset;	/* Segment file offset */
    Elf32_Addr	p_vaddr;	/* Segment virtual address */
    Elf32_Addr	p_paddr;	/* Segment physical address */
    Elf32_Word	p_filesz;	/* Segment size in file */
    Elf32_Word	p_memsz;	/* Segment size in memory */
    Elf32_Word	p_flags;	/* Segment flags */
    Elf32_Word	p_align;	/* Segment alignment, file & memory */
} Elf32_Phdr;

/* Note segments */

typedef struct {
    Elf32_Word	namesz;		/* Size of entry's owner string */
    Elf32_Word	descsz;		/* Size of the note descriptor */
    Elf32_Word	type;		/* Interpretation of the descriptor */
    char	name[1];	/* Start of the name+desc data */
} Elf32_Note;

/* dynamic section structure */

typedef struct {
    Elf32_Sword		d_tag;		/* entry tag value */
    union 
    {
	Elf32_Word	d_val;
	Elf32_Addr	d_ptr;
    } d_un;
} Elf32_Dyn;
