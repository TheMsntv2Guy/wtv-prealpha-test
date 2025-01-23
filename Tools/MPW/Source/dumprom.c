
#ifdef	__macintosh
#include	<Memory.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned short ushort;


#define kMainRAMDirectoryName	"RAM"

#define kMaxFilenameLen		28


typedef struct FSNode	FSNode;

struct FSNode
{
	FSNode		*next;				/* pointer to next file's FSNode */
	FSNode		*parent;			/* pointer to parent directory */
	FSNode		*first;				/* 0 for object, or pointer to first FSNode in dir */
	char		*data;				/* 0 for directory, or pointer to file data */
	ulong		dataLength;					/* size of object */
	ulong		nodeChecksum;
	ulong		dataChecksum;				/* 0 if data = nil */
	char		name[kMaxFilenameLen];
};



typedef struct						/* only used by downloader & boot code to verify ROMFS */
{
	unsigned long		romfs_length;
	unsigned long		romfs_checksum;
	
} ROMFSHeader;



#define kBadAddress			0xBADDBADD
#define	kDefaultROMTop		0x80110000 - sizeof(ROMFSHeader)			/* 1MB ROM */

#define	Public

char *rom = 0;
ulong romSize = 0;
ulong  romTop = 0;
long	index = 0;


FSNode *
NextNode(FSNode *node)
{
	ulong a =(ulong)node->next;
	if ( a == 0 )
		return 0;
	node = (FSNode *) ( rom + romSize - (long)(romTop - a));
	if ( node-rom > romSize ||   node-rom < 0  ) {
		fprintf(stdout,"bogus rom next entry (%x-%x) = %x %x\n",romTop,a,romTop-a,(ulong)node);
		return 0;
	}
	return node;
}

FSNode *
FirstNode(FSNode *node)
{
	ulong a =(ulong)node->first;
	if ( a == 0 )
		return 0;
	node = (FSNode *) ( rom + romSize - (long)(romTop - a));
	if ( node-rom > romSize || node-rom < 0   ) {
		fprintf(stdout,"bogus rom first entry (%x-%x) = %x %x\n",romTop,a,romTop-a,(ulong)node);
		return 0;
	}
	return node;
}

void
DumpDir(FSNode *node,int goDeep,int goNext)
{
	while ( node ) {	
		fprintf(stdout,"%6d %08x %08x %08x %08x %08x %08x %08x '%28s'\n",index,node->dataLength,node->data,
			node->first,node->next,node->parent,node->nodeChecksum,node->dataChecksum,node->name);
		if (goDeep && node->first )
			DumpDir(FirstNode(node),goDeep,goNext);
		if ( goNext )
			node = NextNode(node);
		else
			break;
		index++;
	}
}




Public int
main(int argc, char *argv[])
{
	FILE *f;
	long i = 0;
	FSNode *node = 0;
#ifdef	__macintosh
	Handle	romH;
#endif
	if ( argc != 2 ) {
		fprintf(stderr,"usage: %s romfile\n",argv[0]);
		exit(-1);
	}


	f = fopen(argv[1],"rb");
	if ( f == 0 ) {
		fprintf(stderr,"cant open file '%s'\n",argv[1]);
		exit(-1);
	}
	fseek(f,0,2);
	romSize = ftell(f);
	fseek(f,0,0);
#ifdef	__macintosh
	romH = TempNewHandle(romSize,&err);
	if ( romH ) {
		HLock(romH);
		rom = *romH;
	} else
		rom = 0;
#else
	rom = malloc(romSize);
#endif
	if ( rom == 0 ) {
		fprintf(stderr,"cant alloc memory (%d bytes)\n",romSize);
		exit(-1);
	}
	fread(rom,1,romSize,f);
	fclose(f);

	fprintf(stdout,"Dumping romfile '%s' %d\n",argv[1],romSize);
	
	node = (FSNode *) ((rom + romSize) );

	node--;
	fprintf(stdout,"node->first = %x, actual %x\n",node->first,(ulong)(node-1)-rom);
	romTop = (ulong)((node-1)->parent);
	romTop += sizeof(FSNode);
	fprintf(stdout,"offset %x\n",romTop);
	
	fprintf(stdout,"num    len       data    first   next     parent   xsum    dxsum        name\n");

	DumpDir( FirstNode(node),1,1);

#ifdef	__macintosh
	DisposeHandle(romH);
#else
	free(rom);
#endif
	return 0;
}


