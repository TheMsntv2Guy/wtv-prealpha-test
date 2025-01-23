/* This tool converts a Mac HFS directory and sub-directories
 * into a format that is suitable for the WebTV ROM Filesystem.
 * The output goes to stdout.
 */





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#ifndef	_SYS_TYPES_H
typedef	unsigned long ulong;			/* need for BSD, but not Solaris */
#endif

#define	COULD_BE_LITTLE_DICK_ENDIAN		/* for pee cees */

#ifdef	COULD_BE_LITTLE_DICK_ENDIAN

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


#else

#define	LONG(v)		(v)
#define	SHORT(v)	(v)

#endif


typedef unsigned char uchar;


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

Public int			gDebug = 0;
Public int			gProgressLevel = 0;

Public ulong		gROMTop = kDefaultROMTop;
Public ulong		gROMOffset = kDefaultROMTop;
Public char			*gDirectoryName = 0;
Public char*		gData = 0;
Public char*		gMetaData = 0;
Public ulong		gDataOffset = 0;
Public ulong		gMetaDataOffset = 0;
Public ulong		gLastDirOffset = 0;
Public ulong		gDataSize = 0;
Public ulong		gMetaDataSize = 0;
Public FILE			*gOutputFile;
Public FILE			*gErrorFile;

Public ulong		gTotalDataSize = 0;
Public ulong		gTotalMetaDataSize = 0;



typedef struct
	{
	FSNode					*parents[32];
	FSNode					*parentAddresses[32];
	ulong					index;
	} RomifyOneParameters;
	
	
int 
IsTextFile(const char *path)
{
	return 0;		/* fix this !!! */
}

void
WalkDirectory(const char *path,const char *fileName,RomifyOneParameters *parameters)
{
	struct stat fnode;
	int fd;
	ulong size;
	FILE	*fp;
	DIR		*dp;
	char	*dataPtr;
	struct dirent *dnode;
	FSNode		*node, *nodeAddress;
	int		isDirectory;
	char	subFile[256];
	
	if ( stat(path,&fnode) == -1 ) {
		fprintf(stderr,"error stating file '%s'\n",path);
		exit(-1);
	}
	isDirectory = S_ISDIR(fnode.st_mode);
	
	size = isDirectory ? 0 : fnode.st_size;
	
	if (parameters == 0 ) {


		/* just get size */
		
		gTotalMetaDataSize += sizeof(FSNode);
		
		if ( isDirectory ) {
			long startSize = gTotalDataSize;
			dp = opendir(path);
			if ( dp ) {
				while ( (dnode=readdir(dp)) != 0 )  {
					if ( strcmp(dnode->d_name,".") == 0  || strcmp(dnode->d_name,"..") == 0 )
						continue;
					subFile[0] = 0;
					strcat(subFile,path);
					strcat(subFile,"/");
					strcat(subFile,dnode->d_name);
					WalkDirectory(subFile,dnode->d_name,parameters);
				}
				closedir(dp);
			}
			if ( gProgressLevel )
				fprintf(stderr,"directory '%s' contains %d bytes\n",path,gTotalDataSize-startSize);
		} else {

			if ((size & 3) == 0)			/* If there is no zero padding then pad it */
			{
				size += 4;
			}
			if ( gProgressLevel > 4 ) 
				fprintf(stderr,"measure '%s' = %d\n",path,size);
			gTotalDataSize += (size + 0x3) & ~0x3;
		}



	} else {
		
		
		gMetaDataOffset -= sizeof(FSNode);										/* make a hole for the FSNode */
		node = (FSNode *)(gMetaData + gMetaDataOffset);							/* and fill it in */
		if ( parameters->index > 0 )
			node->parent = (FSNode *)LONG((ulong)(parameters->parentAddresses[parameters->index-1]));
		else
			node->parent = 0;
			
		nodeAddress = (FSNode *)(gROMTop - (gMetaDataSize - gMetaDataOffset));

			
		if (isDirectory) {
			if ( gProgressLevel > 1 ) 
				fprintf(stderr, "'%s'/\n", path);
			if ( gProgressLevel > 2 ) 
				fprintf(stderr, "at node %x (depth %d)\n",(ulong)nodeAddress,parameters->index);
			gLastDirOffset = gMetaDataOffset;
			node->first = (FSNode *)LONG((ulong)(nodeAddress - 1));
			parameters->parents[parameters->index] = node;
			parameters->parentAddresses[parameters->index] = nodeAddress;
		}
		else {
		
			if ( gProgressLevel > 1 ) 
				fprintf(stderr, "'%s' %8d\n", path,size);
			if ( gProgressLevel > 2 ) 
				fprintf(stderr, "at node %x (depth %d)\n",(ulong)nodeAddress,parameters->index);
			node->dataLength = LONG(size);
			gDataOffset -= ((size + 3) & ~3);							/* make a hole for the data, rounded to 4 byte boundary */
			
			if ((size & 3) == 0)										/* If there is no zero padding then pad it */
			{
				gDataOffset -= 4;
			}
			
			node->data = (char *)LONG((gROMTop - gTotalMetaDataSize - (gDataSize - gDataOffset)));	/* and make the FSNode ref it */
			
			if ( gProgressLevel > 2 ) {
				fprintf(stderr, "node->data = %x\n", node->data);
				fprintf(stderr, "node->dataLength = %x\n", node->dataLength);
			}
			
			
			dataPtr = gData + gDataOffset;									/* make an offset into our block */

			fp = fopen(path,"rb");
			if ( fread(dataPtr,1,size,fp) != size ) {
				fprintf(stderr, "read file failed '%s'\n",path);
				exit(1);
			}
			fclose(fp);
			node->next = (FSNode *)LONG((ulong)(nodeAddress - 1));
		}

	
		if ( gProgressLevel > 2 ) {
			fprintf(stderr,"node->first = %x\n", node->first);
			fprintf(stderr, "node->next = %x\n", node->next);
		}
		
		node->name[0] = 0;
		if ( fileName ) {
			strncpy(node->name, fileName,kMaxFilenameLen);	/* and finally, give it a name */
			if ( strlen(fileName) < kMaxFilenameLen )
				node->name[strlen(fileName)] = 0;
			if ( gProgressLevel > 1) {
				fprintf(stderr, "node->name = %s\n", node->name);
			}
		}
		if ( isDirectory  ) {
			int wereChildren = 0;
			parameters->index++;
			dp = opendir(path);
			if ( dp ) {
				while ( (dnode=readdir( dp) ) != 0 ) { 
					if ( strcmp(dnode->d_name,".") == 0  || strcmp(dnode->d_name,"..") == 0 )
						continue;
					subFile[0] = 0;
					strcat(subFile,path);
					strcat(subFile,"/");
					strcat(subFile,dnode->d_name);
					WalkDirectory(subFile,dnode->d_name,parameters);
					wereChildren = 1;
				}
				closedir(dp);
			}
			--parameters->index;
			
			if ( gProgressLevel > 1 )
				fprintf(stderr, "Completed romifying directory of depth %d\n", parameters->index);
			
			if (wereChildren)
			{
				((FSNode *)(gMetaData + gMetaDataOffset))[0].next = 0;
				if ( gProgressLevel > 1)
					fprintf(stderr, "This node had children.  Setting node->next for %s to %x.\n",
											((FSNode *)(gMetaData + gMetaDataOffset))[0].name,
											((FSNode *)(gMetaData + gMetaDataOffset))[0].next);	
			}
			else
			{
				parameters->parents[parameters->index]->first = 0;
				if ( gProgressLevel > 1)
					fprintf(stderr,"Empty directory, setting node->first for %s to 0\n", parameters->parents[parameters->index]->name);	
			}
			
			
			parameters->parents[parameters->index]->next = (parameters->index == 0 ) ? 0 : (FSNode *)LONG((ulong)((gROMTop - (gMetaDataSize - gMetaDataOffset) - sizeof(FSNode))));
			if ( gProgressLevel  > 1)
				fprintf(stderr,"Setting node->next for %s to %x\n", parameters->parents[parameters->index]->name, parameters->parents[parameters->index]->next);	
		}
	}
}


void
BuildData()
{
	int i;
	RomifyOneParameters parameters;
	
	/* fprintf(stderr,"Building rom from '%s'\n",gDirectoryName); */
	
	WalkDirectory(gDirectoryName,0,0);
	
	if ( gProgressLevel ) {
		fprintf(stderr, "directory size: %d bytes\n", gTotalDataSize);
		fprintf(stderr, "data size: %d bytes\n", gTotalMetaDataSize);
	}
	gData = malloc(gTotalDataSize);
	gMetaData = malloc(gTotalMetaDataSize);
	
	if ( gData == 0 || gMetaData == 0 ) {
		fprintf(stderr,"out of memory\n");
		exit(-1);
	}
	parameters.index = 0;
	for ( i=0; i < 32; i++ ) {
		parameters.parents[i] = 0;
		parameters.parentAddresses[i] = 0;
	}
	gDataOffset = gDataSize = gTotalDataSize;
	gMetaDataOffset = gMetaDataSize = gTotalMetaDataSize;
	WalkDirectory(gDirectoryName,"ROM",&parameters);
	
}

void
EndData()
{
	free(gData);
	free(gMetaData);
}

void
WriteData()
{	
	if ( gData ) 
		fwrite(gData,1,gDataSize,gOutputFile);
	if ( gMetaData ) 
		fwrite(gMetaData,1,gMetaDataSize,gOutputFile);
}
	
	
	
int
ParseOptions(char **argv)
	{
	char	*argument;
	char	*pch;
	int	gotFileName = 0;

	*argv++;
	while ((argument = *argv++) != 0)
		{
		if (*argument != '-')
			{
			if (gDirectoryName != 0)
				return 0;
			gDirectoryName = argument;
			continue;
			}
			
		switch (*++argument)
			{
			case 'R': case 'r':
				if (strncmp(argument + 1, "omtop=", 6) != 0)
					return 0;
				(void)sscanf(argument + 7, "%x", &gROMTop);
				gROMTop -= sizeof(ROMFSHeader);
				break;
				
			case 'D': case 'd':
				if (strcmp(argument + 1, "ebug") != 0)
					return 0;
				gDebug = 1;
				break;

			case 'P': case 'p':
				gProgressLevel = 1;
				if ((pch = strrchr(argument, '=')) != 0)
					gProgressLevel = atoi(pch+1);
				break;
				
			default:
				return 0;
			}
		}
		
	return gDirectoryName != 0;
	}




Public int
main(int argc, char *argv[])
{
	#pragma unused (argc)

	gOutputFile = stdout;
	gErrorFile = stderr;
	
	if (!ParseOptions(argv)) {
		fprintf(stderr, "Usage: Romify -romtop=<top addr + 1 (hex)> [-progress] <directory name>\n");
		exit(-1);
	}
	
	/* fprintf(stderr, "ROM Top = %x\n",gROMTop); */
	


	BuildData();
	WriteData();
	EndData();
	
	return 0;
}