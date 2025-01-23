
#include <Types.h>
#include <Errors.h>
#include <Memory.h>
#include <Quickdraw.h>
#include <ToolUtils.h>
#include <Files.h>
#include <Resources.h>
#include <Devices.h>
#include <Serial.h>
#include <CursorCtl.h>
#include <CType.h>
#include <Signal.h>
#include <String.h>
#include <Strings.h>
#include <StdIO.h>
#include <StdArg.h>
#include <StdLib.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

#define Private static
#define Public

#pragma parameter DelayTicks(__A0)
void DelayTicks(ulong numTicks) = 0xa03b;
#pragma parameter __D0 FetchTicks
ulong FetchTicks(void) = {0x91c8, 0xa03b};

/* constants for time estimation */
#define kChecksumBytesPerSecond		262144.0
#define kTicksPerSecond				(60.147420147420147419) /* (15667200.0 / 16 / 44 / 370) */
#define kEstimateOverhead			2.0

/* Constants from MPW documentation ... */
enum {success = 0, syntaxerr = 1, failure = 2};

enum {noCheck, slowCheck, fastCheck, dmaCheck};

ParamBlockRec gOutPB;
ParamBlockRec gInPB;

/* option initialization -- also exists elsewhere */
Public Boolean		gDebug = false;
Public Boolean		gShowProgress = false;
Public Boolean		gReportTimeOnly = false;
Public Boolean		gResourceFork = false;
Public Boolean		gPolitely = false;
Public ulong		gBaudRate = 9600;
Public ulong		gPortNumber = 0;
Public uchar		gCheckType = noCheck;
Public long			gWaitTicks;
Public long			gDownloadAddress = 0xA0002000;
Public char			*gDataFile = nil;

Private OSErr
LoadDataFromFile(const char *dataFile, Boolean resourceFork, Handle *dataHandlePtr)
	{
	ParamBlockRec pb;
	Handle dataStorage;
	ulong fileLength;
	ulong roundedLength;
	OSErr error;

	pb.ioParam.ioNamePtr = dataFile;
	pb.ioParam.ioVRefNum = 0;
	pb.ioParam.ioVersNum = 0;
	pb.ioParam.ioPermssn = fsRdPerm;
	pb.ioParam.ioMisc = 0;
	if ((error = (resourceFork ? PBOpenRFSync(&pb) : PBOpenSync(&pb))) == noErr)
		{
		if ((error = PBGetEOFSync(&pb)) == noErr)
			{
			fileLength = (long)pb.ioParam.ioMisc;
			error = fileLength ? (OSErr)noErr : (OSErr)eofErr;
			if (error == noErr)
				{
				roundedLength = ((fileLength + 15) & -16);
				dataStorage = NewHandle(roundedLength);
				if ((error = MemError()) == noErr)
					{
					/* Read the file into the handle */
					pb.ioParam.ioBuffer = *dataStorage;
					pb.ioParam.ioReqCount = fileLength;
					pb.ioParam.ioPosMode = fsFromStart;
					pb.ioParam.ioPosOffset = 0;
					if ((error = PBReadSync(&pb)) == noErr)
						{
						/* Clear extra bytes to get repeatable checksums */
						for (; fileLength < roundedLength; ++fileLength)
							(*dataStorage)[fileLength] = 0;
						*dataHandlePtr = dataStorage;
						}
					}
				}
			}
		PBCloseSync(&pb);
		}
	return error;
	}

Private void
InterruptSerial(int signum)
	{
#pragma unused (signum)
	ParamBlockRec killPB;

	signal(SIGINT, SIG_IGN);
	killPB.ioParam.ioRefNum = gOutPB.ioParam.ioRefNum;
	(void)PBKillIOSync(&killPB);
	killPB.ioParam.ioRefNum = gInPB.ioParam.ioRefNum;
	(void)PBKillIOSync(&killPB);
	(void)PBCloseSync(&gInPB);
	(void)PBCloseSync(&gOutPB);
	exit(-9);
	}

Private short
SerialSetupValue(void)
	{
	switch (gBaudRate)
		{
		case 9600:		return stop10 + noParity + data8 + baud9600;
		case 19200:		return stop10 + noParity + data8 + baud19200;
		case 38400:		return stop10 + noParity + data8 + baud19200;		// we'll fix this with a csCode=13 call
		}
		
	return 0;
	}
	
Private OSErr
OpenSerial(void)
	{
	char			buffer[512];
	ParamBlockRec	killPB;
	OSErr			error;

	/* handle .AOut, .BOut, .COut, ... format */
	BlockMove("\p.AOut", buffer, 6);
	buffer[2] += gPortNumber;

	signal(SIGINT, SIG_IGN);
	gOutPB.ioParam.ioNamePtr = buffer;
	gOutPB.ioParam.ioVRefNum = 0;
	gOutPB.ioParam.ioVersNum = 0;
	gOutPB.ioParam.ioPermssn = 0;
	gOutPB.ioParam.ioMisc = 0;
	error = PBOpenSync(&gOutPB);
	
	if (error == noErr)
		{
		BlockMove("\p.AIn", buffer, 5);
		buffer[2] += gPortNumber;
	
		gInPB.ioParam.ioNamePtr = buffer;
		gInPB.ioParam.ioVRefNum = 0;
		gInPB.ioParam.ioVersNum = 0;
		gInPB.ioParam.ioPermssn = 0;
		gInPB.ioParam.ioMisc = 0;
		error = PBOpenSync(&gInPB);
		
		if (error == noErr)
			{
			gOutPB.cntrlParam.csCode = 14;
			((SerShk *)&gOutPB.cntrlParam.csParam)->fXOn = false;
			((SerShk *)&gOutPB.cntrlParam.csParam)->fCTS = false;
			((SerShk *)&gOutPB.cntrlParam.csParam)->xOn = 0x11;
			((SerShk *)&gOutPB.cntrlParam.csParam)->xOff = 0x13;
			((SerShk *)&gOutPB.cntrlParam.csParam)->errs = 0;
			((SerShk *)&gOutPB.cntrlParam.csParam)->evts = 0;
			((SerShk *)&gOutPB.cntrlParam.csParam)->fInX = false;
			((SerShk *)&gOutPB.cntrlParam.csParam)->fDTR = true;
			error = PBControlSync(&gOutPB);
			if (error == noErr)
				{
				gOutPB.cntrlParam.csCode = 8;
				*(short *)&gOutPB.cntrlParam.csParam = SerialSetupValue();
				error = PBControlSync(&gOutPB);
				
				if (gBaudRate == 38400)								// total hack
				{
					gOutPB.cntrlParam.csCode = 13;
					*(short *)&gOutPB.cntrlParam.csParam = 38400;
					error = PBControlSync(&gOutPB);
					if (*(unsigned short *)&gOutPB.cntrlParam.csParam != 38400)
						fprintf(stderr, "Error: Set to 38400, got %ud\n",*(unsigned short *)&gOutPB.cntrlParam.csParam); 
					gOutPB.cntrlParam.csCode = 14;
					((SerShk *)&gOutPB.cntrlParam.csParam)->fXOn = false;
					((SerShk *)&gOutPB.cntrlParam.csParam)->fCTS = false;
					((SerShk *)&gOutPB.cntrlParam.csParam)->xOn = 0x11;
					((SerShk *)&gOutPB.cntrlParam.csParam)->xOff = 0x13;
					((SerShk *)&gOutPB.cntrlParam.csParam)->errs = 0;
					((SerShk *)&gOutPB.cntrlParam.csParam)->evts = 0;
					((SerShk *)&gOutPB.cntrlParam.csParam)->fInX = false;
					((SerShk *)&gOutPB.cntrlParam.csParam)->fDTR = true;
					error = PBControlSync(&gOutPB);
				}
				
				if (error == noErr)
					{
					gInPB.ioParam.ioCompletion = nil;
					gInPB.ioParam.ioBuffer = buffer;
					gInPB.ioParam.ioReqCount = 512;
					gInPB.ioParam.ioPosMode = 0;
					gInPB.ioParam.ioPosOffset = 0;
					(void)PBReadAsync(&gInPB);
					DelayTicks(5);
					killPB.ioParam.ioRefNum = gInPB.ioParam.ioRefNum;
					(void)PBKillIOSync(&killPB);
					signal(SIGINT, InterruptSerial);
					return noErr;
					}
				}
			(void)PBCloseSync(&gInPB);
			}
		(void)PBCloseSync(&gOutPB);
		}
	signal(SIGINT, SIG_DFL);
	return error;
	}

Private void
CloseSerial(void)
	{
	signal(SIGINT, SIG_IGN);
	(void)PBCloseSync(&gInPB);
	(void)PBCloseSync(&gOutPB);
	signal(SIGINT, SIG_DFL);
	}

#define ShiftKeyDown()		(*(ulong *)0x178 & 0x1)
#define kBytesPerHashCount	(4*1024)

Private OSErr
SendBytes(const void *start, ulong length)
	{
	ulong		lastSpinTicks = 0;
	ulong		lastReportTicks = 0;
	ulong		nextHashCount = kBytesPerHashCount;
	ulong		hashesOnLine = 0;
	
	gOutPB.ioParam.ioBuffer = start;
	gOutPB.ioParam.ioReqCount = length;
	gOutPB.ioParam.ioPosMode = 0;
	gOutPB.ioParam.ioPosOffset = 0;
	
	(void)PBWriteAsync(&gOutPB);
	while (gOutPB.ioParam.ioResult == 1)
		{
		ulong	newSpinTicks = FetchTicks();
		
		if (newSpinTicks - lastSpinTicks > 10)
			{ SpinCursor(32); lastSpinTicks = newSpinTicks; }
			
		if (gShowProgress)
			if (gOutPB.ioParam.ioActCount > nextHashCount)
				{ 
				fprintf(stderr, "#"); fflush(stderr);
				if (++hashesOnLine >= 40)
					{ fprintf(stderr, "\n"); hashesOnLine = 0; }
				nextHashCount += kBytesPerHashCount;
				}
				
		if (gShowProgress && ShiftKeyDown())
			{
			ulong	newReportTicks = FetchTicks();
			
			if (newReportTicks - lastReportTicks > 60)
				{
				fprintf(stderr, "\n### transferred %d of %d bytes\n", gOutPB.ioParam.ioActCount, gOutPB.ioParam.ioReqCount);
				lastReportTicks = newReportTicks;
				}
			}
		}
		
	if (gShowProgress)
		fprintf(stderr, "\n");
	
	return gOutPB.ioParam.ioResult;
	}

Private void
RestorePortStatus(ParamBlockRec *pb)
	{
	pb->cntrlParam.csCode = 14;
	((SerShk *)&pb->cntrlParam.csParam)->fXOn = false;
	((SerShk *)&pb->cntrlParam.csParam)->fCTS = false;
	((SerShk *)&pb->cntrlParam.csParam)->xOn = 0x11;
	((SerShk *)&pb->cntrlParam.csParam)->xOff = 0x13;
	((SerShk *)&pb->cntrlParam.csParam)->errs = 0;
	((SerShk *)&pb->cntrlParam.csParam)->evts = 0;
	((SerShk *)&pb->cntrlParam.csParam)->fInX = false;
	((SerShk *)&pb->cntrlParam.csParam)->fDTR = false;
	PBControlSync(pb);
	}
	
Private int
HandleBefore(void)
	{
	int	tempValue;

	if (OpenSerial() != noErr)
		{
		fprintf(stderr, "### SerialDownload: could not open serial driver\n");
		return failure;
		}

	gInPB.cntrlParam.csCode = 8;
	//if (((tempValue = PBStatusSync(&gInPB)) != noErr) || ((SerStaRec *)&gInPB.cntrlParam.csParam)->ctsHold)
	if ((tempValue = PBStatusSync(&gInPB)) != noErr)
		{
		RestorePortStatus(&gOutPB);
		fprintf(stderr, "### SerialDownload: serial handshake error (is your cable connected?)\n");
		CloseSerial();
		return failure;
		}

	return success;
	}

Private int
HandleAfter(void)
	{
	CloseSerial();
	return success;
	}

Private double
TimeEstimate(ulong bytes)
	{
	double		bytesPerSecond = gBaudRate/10.0;
	
	return bytes/bytesPerSecond + bytes/kChecksumBytesPerSecond + kEstimateOverhead;
	}
	
Private void
ReportTime(ulong dataLength)
	{
	fprintf(stderr, "%4.2f\n", TimeEstimate(dataLength));
	}

#define kChunkSize			2048

Private int
DownloadData(Handle dataHandle, ulong dataOffset, ulong dataLength)
/* download the data, assuming it is composed of srecords and the other size knows that */
	{
	Ptr			dataPtr;
	OSErr		err;

	dataLength = (dataLength + 3) & 0xFFFFFFFC;						/* Round-up to long */

	HLock(dataHandle);
	dataPtr = StripAddress(*dataHandle) + dataOffset;
	err = SendBytes(dataPtr, dataLength);
	HUnlock(dataHandle);
	
	if (err != noErr)
		{
		fprintf(stderr, "### SerialDownload: got error %d while writing.\n", (int)err);
		return failure;
		}
	return success;
	}

Private int
HandleDownload(Handle dataHandle, ulong dataOffset, ulong dataLength, ulong address)
	{
	#pragma unused (address)
	ulong elapsedTicks;

	SpinCursor(32);
	
	if (gShowProgress)
		{
		fprintf(stderr, "# writing 0x%X bytes, estimated time: %4.2f seconds\n", dataLength, TimeEstimate(dataLength));
		elapsedTicks = FetchTicks();
		}
		
	DownloadData(dataHandle, dataOffset, dataLength);

	if (gShowProgress)
		{
		elapsedTicks = FetchTicks() - elapsedTicks;
		fprintf(stderr, "# actual time: %4.2f seconds\n", elapsedTicks / kTicksPerSecond);
		}

	return success;
	}


Private int
HandleDownloadPolitely(Handle dataHandle, ulong dataSize) /* takes a while, but lets you read news while it’s going */
	{
	long	chunkSize = 16*1024;
	long	curOffset = 0;
	long	curDownloadAddr = gDownloadAddress;
	long	bytesLeft = (long)dataSize;
	int		result = success;

	for (;;)
		{
		result = HandleDownload(dataHandle, curOffset, chunkSize, curDownloadAddr);
		if ((result != success) || (bytesLeft == 0))
			break;
		curDownloadAddr += chunkSize;
		curOffset += chunkSize;
		bytesLeft -= chunkSize;
		if (bytesLeft < 0)
			chunkSize += bytesLeft, bytesLeft = 0;
		}
	return result;
	}

/* Parse buffer, set number to result, update buffer pointer */
/* Returns true for valid munbers, false for errors */

Private Boolean
ParseNumber(long *number, const char **buffer)
	{
	ulong radix;
	ulong value;
	Boolean negative;
	char digit;
	char *scanner;

	radix = 0;
	value = 0;
	negative = false;

	for (scanner = *buffer; ; ++scanner)
		{
		digit = *scanner;
		if (!radix)
			{
			if (digit == '-')
				{
				if (negative)
					break;
				negative = true;
				}
			else if (digit == '$')
				radix = 16;
			else if (digit == '0')
				radix = 8;
			else if (digit >= '1' && digit <= '9')
				{
				radix = 10;
				digit -= '0';
				value = digit;
				}
			else
				break;
			}
		else if (!value && radix == 8)
			{
			if (digit == 'x' || digit == 'X')
				radix = 16;
			else if (digit == 'b' || digit == 'B')
				radix = 2;
			else if (digit >= '0' && digit <= '7')
				{
				digit -= '0';
				value = digit;
				}
			else
				break;
			}
		else
			{
			if (digit >= 'a' && digit <= 'f')
				digit -= 'a' - 10;
			else if (digit >= 'A' && digit <= 'F')
				digit -= 'A' - 10;
			else if (digit >= '0' && digit <= '9')
				digit -= '0';
			else
				break;
			if (digit >= radix)
				break;
			value *= radix;
			value += digit;
			}
		}

	*buffer = scanner;
	if (!radix)
		return false;
	if (!negative)
		*number = value;
	else
		*number = -value;
	return true;
	}

/*  Typical calling sequence: */
/*      if (MatchOption(userarg, "-someparam")) */
/*          HandleSomeParam(); */
/*  Note:   opt[] MUST be all lower case or the compare will fail */
/*          No side effects */

Private Boolean
MatchOption(const char *arg, const char *opt)
	{
	char lastChar;

	do
		{
		lastChar = *opt++;
		if (tolower(lastChar) != tolower(*arg++))
			return false;
		}
	while (lastChar);
	return true;
	}

Private Boolean
ParseOptions(char **argv)
	{
	char	*argument;

	*argv++;
	while ((argument = *argv++) != nil)
		{
		if (MatchOption(argument, "-p"))
			gShowProgress = true;
		else if (MatchOption(argument, "-porta"))
			gPortNumber = 0;
		else if (MatchOption(argument, "-portb"))
			gPortNumber = 1;
		else if (MatchOption(argument, "-portc"))
			gPortNumber = 2;
		else if (MatchOption(argument, "-portd"))
			gPortNumber = 3;
		else if (MatchOption(argument, "-porte"))
			gPortNumber = 4;
		else if (MatchOption(argument, "-portf"))
			gPortNumber = 5;
		else if (MatchOption(argument, "-debug"))
			gDebug = true;
		else if (MatchOption(argument, "-rf"))
			gResourceFork = true;
		else if (MatchOption(argument, "-check"))
			gCheckType = slowCheck;
		else if (MatchOption(argument, "-politely"))
			gPolitely = true;
		else if (MatchOption(argument, "-reporttimeonly"))
			gReportTimeOnly = true;
		else if (strncmp(argument, "-9600", 5) == 0)
			gBaudRate = 9600;
		else if (strncmp(argument, "-19200", 6) == 0)
			gBaudRate = 19200;
		else if (strncmp(argument, "-38400", 6) == 0)
			gBaudRate = 38400;
		else if (MatchOption(argument, "-base"))
			{
			fprintf(stderr, "-base option invalid with srecord downloads.\n");
			goto badOption;
#if 0		
			if (!(argument = *argv++))
				{
				fprintf(stderr, "-base address missing.\n");
				goto badOption;
				}
			if (!ParseNumber(&gDownloadAddress, &argument) || *argument)
				{
				fprintf(stderr, "-base address is invalid.\n");
				goto badOption;
				}
#endif
			}
		else
			{
			if (gDataFile != nil)
				{
				fprintf(stderr, "More than one data file (you passed %P and %s).\n", gDataFile, argument);
				goto badOption;
				}
				
			gDataFile = c2pstr(argument);
			}
		}

	if (gDataFile != nil)
		return true;

	fprintf(stderr, "Data file name problem.\n");

  badOption:
	return false;
	}

Public int
main(int argc, char *argv[])
	{
	#pragma unused (argc)
	int			result;
	OSErr		error;
	ulong		dataSize;
	Handle		dataHandle;

	InitGraf(&qd.thePort);

	if (!ParseOptions(argv))
		{
		fprintf(stderr, "Usage: SDownload [-base <address>] [-p] [-porta] [-portb] [-reporttimeonly] <file name>\n");
		return syntaxerr;
		}

	SetCursor(*GetCursor(watchCursor));

	if ((error = LoadDataFromFile(gDataFile, gResourceFork, &dataHandle)) != noErr)
		{
		fprintf(stderr, "### SerialDownload: could not read data file '%P' (error %d)\n", gDataFile, error);
		return failure;
		}

	dataSize = GetHandleSize(dataHandle);

	if (gReportTimeOnly)
		{
		ReportTime(dataSize);
		return success;
		}

	result = HandleBefore();
	if (result == success)
		{
		if (gPolitely)
			result = HandleDownloadPolitely(dataHandle, dataSize);
		else
			result = HandleDownload(dataHandle, 0, dataSize, gDownloadAddress);

		if (result == success)
			result = HandleAfter();
		}
	return result;
	}
