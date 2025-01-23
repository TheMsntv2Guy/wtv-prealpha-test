/* CODE EXAMPLE #2 */
/* This code will not work on a 64K ROM Macintosh. */

#include <stdio.h>
#include <Serial.h>	/* THINK C 4.0 users should use SerialDrvr.h> instead */
#include <string.h>

#define SERBUFSIZ 0x5000

#define XONCHAR 0x11
#define XOFFCHAR 0x13
#define BACKSPACECHAR 0x08
#define ESCAPECHAR 0x1B
#define BACKTICCHAR '`'
#define DELETECHAR 0x7f

#define OUTDRIVER "\p.BOut"
#define INDRIVER "\p.BIn"

extern TEHandle		TEH;

extern void ShowSelect(void);

/* prototypes */
OSErr serialinit(void);
void serialwrite(char);
long serialcharsavail(void);
void getserialchars(long );
void displaybuff(char *buf,long count);
void cleanup(void);

char *inbuf;
short inRefNum, outRefNum;

OSErr serialinit(void)
{
	OSErr   err;
	SerShk flags;
	Ptr buf;
	long count;
	
	/* Open Serial Drivers */

	if ((err = OpenDriver(INDRIVER, &inRefNum)))
		return err;
	if ((err = OpenDriver(OUTDRIVER,&outRefNum)))
		return err;
		
	/* Give the serial input driver a SERBUFSIZ buffer */
	if(!(buf = NewPtr(SERBUFSIZ)))
		return MemError();
	if (err = SerSetBuf(inRefNum, buf, SERBUFSIZ))
		return err;

/* Set handshaking for the input driver */
	flags.fXOn = TRUE;
	flags.fInX = TRUE;
	flags.xOn = XONCHAR;
	flags.xOff = XOFFCHAR;
	if (( err = SerHShake(inRefNum,&flags)))
		return err;

	if (!( inbuf = (char *) NewPtr(SERBUFSIZ)))
		return MemError();

	SerGetBuf(inRefNum, &count);
	getserialchars(count);

	if ((err = SerReset(inRefNum,baud19200 + data8 + stop10 + noParity)))
		return err;
	if ((err = SerReset(outRefNum,baud19200 + data8 + stop10 + noParity)))
		return err;

	return noErr;
}

void serialwrite(char ch)
{
	long num = 1;

	(void) FSWrite(outRefNum,  &num, &ch);
}

long serialcharsavail(void)
{
	long count = 0;

	SerGetBuf(inRefNum, &count);
	return count;
}

void getserialchars(long count)
{
	(void) FSRead(inRefNum,&count,inbuf);
}

#define kNukeSize 0x2000

void displaybuff(char *buf,long count)
{
	int i;
	int start=0;
	
	if((**TEH).teLength > 25000) {
		/* remove stuff off top */
		TESetSelect(0,kNukeSize,TEH);
		TEDelete(TEH);
		TESetSelect(32767,32767,TEH);
	}
	for(i=0;i<count;i++) {
		if(buf[i] < 0x20) {
			if(buf[i] == '\n')
			{
				buf[i] = '\r';
				TEInsert(&buf[start],i-start,TEH);
			}
			else
			{
				TEInsert(&buf[start],i-start,TEH);
				TEKey(buf[i],TEH);
			}
			start = i+1;
		}
	}
	TEInsert(&buf[start],i-start,TEH);
	ShowSelect();
}

void cleanup(void)
{
	if(inRefNum)
		CloseDriver(inRefNum);
	if(outRefNum)
		CloseDriver(outRefNum);
	if (inbuf)
		DisposPtr(inbuf);
}

