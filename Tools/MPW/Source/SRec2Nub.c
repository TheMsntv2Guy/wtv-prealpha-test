#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CursorCtl.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

#define Private static
#define Public

#define true	1
#define false	0

Private ulong	last_addr;
Private ulong	last_range;
Private int		last_acc_type;
Private int		csum;		/* checksum during load operation */

Private ulong	gEntryPoint = 0;
Private int		gProgressFlag = false;
Private FILE	*gInputFile;
Private char	*gInputFilename;
Private FILE	*gOutfile;
Private char	*gOutBuffer;
Private ulong	lowAddr = 0x000fffff;
Private ulong	hiAddr  = 0x00000000;
Private ulong	segment = 0x00000000;

Private int 
a_con_bin(char c)
{
	if (isupper(c))
		c = tolower(c);
	if (isdigit(c))
		return((int)(c-'0'));
	if (isxdigit(c))
		return((int)(c-'a'+10));
	else
		return(10000);
}


Private int
GetPair(FILE *fp)
{
	register byte;

	byte = a_con_bin(fgetc(fp)) << 4;
	byte |= a_con_bin(fgetc(fp)); 
	csum += byte;
	return(byte);
}

/*
 * load -- download motorola s-records from host
 */
Private int
Load(FILE *fp, char *buf)
{
	register int	length, address;
	int				i, first, done, reccount, type, save_csum;
	ulong			bufOffset;

	reccount = 1;

	for (first = 1, done = 0; !done; first = 0, reccount++) {
		while (fgetc(fp) != 'S')
			continue;
		csum = 0;
		type = fgetc(fp);
		length = GetPair(fp);
		if (length < 0 || length >= 256) {
			fprintf(stderr, "### record %d: invalid length\n", reccount);
			goto bad;
		}
		length--;	/* save checksum for later */
		switch (type) {

		case '0':	/* initial record, ignored */
			while (length-- > 0)
				GetPair(fp);
			break;

		case '3':	/* data with 32 bit address */
			address = 0;
			for (i = 0; i < 4; i++) {
				address <<= 8;
				address |= GetPair(fp);
				length--;
			}

			bufOffset = (address & 0x000fffff);		/* ASSUMES 1MB BUF */

			if (bufOffset < lowAddr)
				lowAddr = bufOffset;			

			while (length-- > 0)
				{
				*(char *)(buf+bufOffset) = GetPair(fp);
				bufOffset++;	/* this could write over the end of the buffer */
				}

			if (bufOffset > hiAddr)
				hiAddr = bufOffset;		/* update the highest addr we need to write out */

			break;

		case '7':	/* end of data, 32 bit initial pc */
			address = 0;
			for (i = 0; i < 4; i++) {
				address <<= 8;
				address |= GetPair(fp);
				length--;
			}
			gEntryPoint = address;
			if (length)
			    fprintf(stderr, "### record %d: type 7 record bad\n",reccount);
			done = 1;
			break;
		
		default:
			fprintf(stderr, "### record %d: unknown record type, %c\n",
			    reccount, type);
			break;
		}
		save_csum = (~csum) & 0xff;
		csum = GetPair(fp);
		if (csum != save_csum) 
			fprintf(stderr, "### record %d: checksum error, calculated 0x%x, received 0x%x\n", reccount, save_csum, csum);
		
		SpinCursor(2);
	}
	if (gProgressFlag)
		fprintf(stderr, "# Conversion done: %d records, initial pc: 0x%x\n#\n", reccount-1, gEntryPoint);

bad:
	return 1;
}

#define FixOneLong(ul)		(ul)

#if 0
Private ulong
FixOneLong(ulong ul)
{
	return( ul );
	
/*
	return( (ul<<24) | ((ul&0x0000ff00)<<8) | ((ul&0x00ff0000)>>8)
				| ((ul&0xff000000)>>24) );
*/
}
#endif

Private int
ParseCommandLine(ulong argc, char *argv[])
{
	char		*s;
	ulong		cnt;
	ulong		err;

  err = 0;										// assume no err

  if (argc > 1)									
  	{	
  	for (cnt = 1; cnt <= argc; cnt++)
	  {
	  s = argv[cnt];
	  if (s[0] != '-')							// if no dash, then treat it like a filename & stop parsing the line
	    {
		gInputFilename = argv[cnt];
		cnt = argc;								// this will make us drop out of the for loop
		}
		else
			{
			switch (s[1])
			  {
			  case 'p': case 'P':
			  			gProgressFlag = true; break;
			  default:
			  			err = 1;						// bogus switch, dump usage info
						cnt = argc;
						break;
			  }
			  
			if (err != 0)
			  cnt = argc;								// stop parsing if we get an error
			}
	  }
	}
	else
	  err = 1;
	
  if (err == 1)			// a cmd line error occurred, dump usage info
	{
	printf("Usage: scvt <fn>\n");
	}
	
  return(err);
}


Public int
main(int argc, char **argv)
{
	ulong		x,checksum,addr;

	InitCursorCtl(0);
	
	gOutBuffer = calloc(0x00100000,1);
	if (gOutBuffer)
	{
		if (ParseCommandLine(argc,argv) == 0)
		{
			if (gProgressFlag)
			{
				fprintf(stderr, "# SRec2Nub - converts s record file to Britt nub file format.\n");
				fprintf(stderr, "# 	converting file \"%s\".\n", gInputFilename);
			}
			
			if ((gInputFile = fopen(gInputFilename, "r")) != 0)
			{
				gOutfile = stdout;

				Load(gInputFile, gOutBuffer);

				checksum=0;
				for(x = lowAddr; x!= hiAddr; x += 4)
				{
					addr = (ulong)gOutBuffer + x;
					checksum += FixOneLong(*(ulong *)addr); 
				}

				/* sync flag */
				x = FixOneLong(0x000000aa);
				fwrite(&x, 4, 1, gOutfile);

				/* start addr */
				x = FixOneLong(0x80000000 | lowAddr);
				fwrite(&x, 4, 1, gOutfile);

				/* # words in this block */
				x = FixOneLong((hiAddr-lowAddr)>>2);
				fwrite(&x, 4, 1, gOutfile);

				/* output the checksum */
				x = FixOneLong(checksum);
				fwrite(&x, 4, 1, gOutfile);

				/* write the data */
				fwrite((gOutBuffer + lowAddr), 1, hiAddr-lowAddr, gOutfile);		

				fclose(gInputFile);
				if (gOutfile != stdout)
					fclose(gOutfile);
				free(gOutBuffer);

				if (gProgressFlag)
				{
					fprintf(stderr, "# lowAddr: 0x%08lx\n", lowAddr);
					fprintf(stderr, "# hiAddr: 0x%08lx\n", hiAddr);
				}
			}
			else
			{
				fprintf(stderr, "### Can't open input file: %s\n", gInputFilename);
				return 1;
			}
		}
	}
	else
	{
		fprintf(stderr, "### couldn't alloc buf\n");
		return 2;
	}

	return 0;		
}
