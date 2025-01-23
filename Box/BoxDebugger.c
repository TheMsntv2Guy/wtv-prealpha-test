#include "Headers.h"
#include "Debug.h"

#include "BoxBoot.h"
#include "Debug.h"
#include "BoxAbsoluteGlobals.h"
#include "BoxDebugger.h"
#include "BoxUtils.h"
#include "Caches.h"
#include "chario.h"
#include "Exceptions.h"
#include "HWDisplay.h"
#include "HWKeyboard.h"
#include "HWModem.h"
#include "HWRegs.h"
#include "HWTimers.h"
#include "Input.h"
#include "Interrupts.h"
#include "IR.h"
#include "iregdef.h"
#include "MemoryManager.h"
#include "ObjectStore.h"
#include "SiliconSerial.h"
#include "Flash.h"
#include "CrashLogC.h"
#include "IIC.h"

#include "Testing.h"

#ifdef APPROM
#include "BoxPrintDebug.h"
#endif

extern Boolean gPrintEnable;
extern Boolean gCrashLogPrintfs;

void ParseCommandLine(char* s);

ulong ParsePossibleReg(char* s);

#undef v1
extern void SetROMTiming(ulong v1, ulong v2,ulong pagemode);

/* --- Common Functions (All ROMs) --- */

void DumpSPOTRegister(ulong argc, char* argv[]);
void SetSPOTRegister(ulong argc, char* argv[]);

void DumpMem(ulong argc, char* argv[]);
void DumpSingleLong(ulong argc, char* argv[]);
void DumpSingleHalfword(ulong argc, char* argv[]);
void DumpSingleByte(ulong argc, char* argv[]);

void SetLongs(ulong argc, char* argv[]);
void SetHalfwords(ulong argc, char* argv[]);
void SetBytes(ulong argc, char* argv[]);

void FillMem(ulong argc, char* argv[]);
void FillByteMem(ulong argc, char* argv[]);
void FillMemCount(ulong argc, char* argv[]);
void FillByteMemCount(ulong argc, char* argv[]);

void DumpExceptionRegs(ulong argc, char* argv[]);
void DumpExceptionRegs2(ulong argc, char* argv[]);

void ReturnFromException(ulong argc, char* argv[]);

void DisassembleBeginning(ulong argc, char* argv[]);
void DisassembleMiddle(ulong argc, char* argv[]);
void DisassembleFunction(ulong argc, char* argv[]);
void DisassembleCommon(ulong argc, char* argv[], ulong middle);
void DisassembleLoop(ulong* addr,ulong nLines);

void doReg(ulong argc, char* argv[], ulong regindex);

void Breakpoint(ulong argc, char* argv[]);
void BreakpointClear(ulong argc, char* argv[]);
void BreakpointDisplay(ulong argc, char* argv[]);

void DataWatchClear(ulong argc, char* argv[]);
void DataWatch(ulong argc, char* argv[]);
void DataWriteWatch(ulong argc, char* argv[]);
void DataReadWatch(ulong argc, char* argv[]);

void Go(ulong argc, char* argv[]);

extern ulong *NextInstrAddr(ulong *addr,uchar over,uchar predict);

void DisplaySSN(ulong argc, char* argv[]);



/* --- App-ROM only functions --- */

#ifdef APPROM


#endif


/* --- Testing-only functions --- */

#ifdef DEBUG
#ifndef BOOTROM

void DumpHistory(ulong argc, char* argv[]);
void DoHistory(ulong num);
void AddToHistory(char* s);

void VerifyMem(ulong argc, char* argv[]);
void VerifyByteMem(ulong argc, char* argv[]);
void VerifyMemCount(ulong argc, char* argv[]);
void VerifyByteMemCount(ulong argc, char* argv[]);
void CompareRange(ulong argc, char* argv[]);

void Step(ulong argc, char* argv[]);
void Trace(ulong argc, char* argv[]);
void StackCrawl(ulong argc, char* argv[]);

void RebootBox(ulong argc, char* argv[]);

void DumpCrashLog(ulong argc, char* argv[]);

void DecodeSysConfig(ulong syscfg);
void DumpSysConfig(ulong argc, char* argv[]);

void How(ulong argc, char* argv[]);
void WhichObj(ulong argc, char* argv[]);
void WhereIs(ulong argc, char* argv[]);
void Where(ulong argc, char* argv[]);

void CreateFileFromMemory(ulong argc, char* argv[]);
void CreateDirectory(ulong argc, char* argv[]);
void DeleteFile(ulong argc, char* argv[]);
void DumpFilesystems(ulong argc, char* argv[]);
void DumpDirectory(ulong argc, char* argv[]);

void DumpHelp(ulong argc, char* argv[]);
void DumpHelp2(ulong argc, char* argv[]);

void DumpHeap(ulong argc, char* argv[]);
void DumpHeapTotals(ulong argc, char* argv[]);
void GetASysHeapBlock(ulong argc, char* argv[]);
void FreeASysHeapBlock(ulong argc, char* argv[]);

void DumpIRBuffer(ulong argc, char* argv[]);

void LastURL(ulong argc, char* argv[]);
void DisplayVariable(ulong argc, char* argv[]);

void SetCaches(ulong argc, char* argv[]);
void ReadIIC(ulong argc, char* argv[]);
void WriteIIC(ulong argc, char* argv[]);
void DoInit7187(ulong argc, char* argv[]);

#endif

#if !defined(BOOTROM) && !defined(APPROM)

void TestIR(ulong argc, char* argv[]);

void PaintDebugLine(ulong argc, char* argv[]);

void PaintPicture(ulong argc, char *argv[]);
void PaintPictureHWInterlaced(ulong argc, char *argv[]);

void PokeHerb(ulong argc, char* argv[]);

void DownloadFlash(ulong argc, char* argv[]);
void EraseFlash(ulong argc, char* argv[]);
void TestFlash(ulong argc, char* argv[]);

void HTest(ulong argc, char* argv[]);

void EnableOverscan(ulong argc, char* argv[]);
void SetOverscanColor(ulong argc, char* argv[]);
void SetScreenPage(ulong argc, char* argv[]);
void RunBenchmarks(ulong argc, char* argv[]);
void FillFreeMem(ulong argc, char* argv[]);

void TestLEDs(ulong argc, char* argv[]);

#endif /* BOOTROM && APPROM */

#endif /* DEBUG */



long stonum(char* s);

static char prompt[32];

typedef struct
{
	const char*	cmd;
	void	(*fn)(ulong argc, char* argv[]);
} cmdvector;


const cmdvector cmdTable[] = 	{
							{ "dr", 		DumpSPOTRegister		},
							{ "sr", 		SetSPOTRegister			},
							{ "dm", 		DumpMem 				},
							{ "dsl", 		DumpSingleLong			},
							{ "dsh", 		DumpSingleHalfword		},
							{ "dsb", 		DumpSingleByte			},
							{ "sb",			SetBytes 				},
							{ "sh",			SetHalfwords			},
							{ "sl",			SetLongs 				},
							{ "f",			FillMem					},
							{ "fb",			FillByteMem				},
							{ "fc",			FillMemCount			},
							{ "fbc",		FillByteMemCount		},
							{ "er",			DumpExceptionRegs		},
							{ "er2",		DumpExceptionRegs2		},
							{ "g",			Go						},
							{ "go",			Go						},
							{ "il",			DisassembleBeginning	},
							{ "ip",			DisassembleMiddle		},
							{ "if",			DisassembleFunction		},
							{ "br",			Breakpoint				},
							{ "brc",		BreakpointClear			},
							{ "brd",		BreakpointDisplay		},
							{ "dw",			DataWatch				},
							{ "dww",		DataWriteWatch			},
							{ "drw",		DataReadWatch			},
							{ "dwc",		DataWatchClear			},
							{ "ssn",		DisplaySSN				},

#ifdef APPROM
#ifdef DEBUG
							{ "hist",		DumpHistory				},

							{ "v",			VerifyMem				},
							{ "vb",			VerifyByteMem			},
							{ "vc",			VerifyMemCount			},
							{ "vbc",		VerifyByteMemCount		},
							{ "cr",			CompareRange			},

							{ "s",			Step					},
							{ "t",			Trace					},
							{ "sc",			StackCrawl				},

							{ "rb",			RebootBox				},

							{ "dc",			DumpCrashLog			},
							{ "sysconfig",	DumpSysConfig			},

							{ "how",		How						},
							{ "whichobj",	WhichObj				},
							{ "whereis",	WhereIs					},
							{ "where",		Where					},

							{ "addfile", 	CreateFileFromMemory	},
							{ "mkdir",		CreateDirectory			},
							{ "delfile", 	DeleteFile 				},
							{ "fs", 		DumpFilesystems 		},
							{ "ls", 		DumpDirectory 			},

							{ "help2",		DumpHelp2				},
							{ "help",		DumpHelp 				},
							{ "?",			DumpHelp 				},

							{ "hd", 		DumpHeap 				},
							{ "ht", 		DumpHeapTotals			},
							{ "alloc", 		GetASysHeapBlock		},
							{ "free", 		FreeASysHeapBlock		},

							{ "dir",		DumpIRBuffer			},

							{ "url",		LastURL					},
							{ "dv",			DisplayVariable			},
							{ "cache",		SetCaches				},
							{ "readiic",	ReadIIC					},
							{ "writeiic",	WriteIIC				},
							{ "init7187",	DoInit7187				},
#endif /* DEBUG */
#endif /* APPROM */

#ifdef DEBUG
#if !defined(BOOTROM) && !defined(APPROM)

							{ "ir",			TestIR					},
							{ "line",		PaintDebugLine			},
							{ "pict",		PaintPicture			},
							{ "picthw",		PaintPictureHWInterlaced},
							{ "herb",		PokeHerb				},
							{ "dlflash",	DownloadFlash			},
							{ "eraseflash",	EraseFlash				},
							{ "testflash",	TestFlash				},
							{ "htest",		HTest					},
							{ "overscan",	EnableOverscan			},
							{ "overcolor",	SetOverscanColor		},
							{ "hgr",		SetScreenPage			},
							{ "bench",		RunBenchmarks			},
							{ "fillfree",	FillFreeMem				},
							{ "led",		TestLEDs				},
							
#endif /* BOOTROM && APPROM */
#endif /* DEBUG */

							{ 0,			0 						}
							};

							
typedef struct
{
	const char*	regname;
	ulong regindex;
} regoffsettbl;

const regoffsettbl	regTable[]=	{
							{ "v0", R_V0 },
							{ "v1", R_V1 },
							
							{ "a0", R_A0 },
							{ "a1", R_A1 },
							{ "a2", R_A2 },
							{ "a3", R_A3 },
							
							{ "t0", R_T0 },
							{ "t1", R_T1 },
							{ "t2", R_T2 },
							{ "t3", R_T3 },
							{ "t4", R_T4 },
							{ "t5", R_T5 },
							{ "t6", R_T6 },
							{ "t7", R_T7 },
							{ "t8", R_T8 },
							{ "t9", R_T9 },

							{ "s0", R_S0 },
							{ "s1", R_S1 },
							{ "s2", R_S2 },
							{ "s3", R_S3 },
							{ "s4", R_S4 },
							{ "s5", R_S5 },
							{ "s6", R_S6 },
							{ "s7", R_S7 },

							{ "k0", R_K0 },
							{ "k1", R_K1 },

							{ "ra", R_RA },
							{ "sp", R_SP },
							{ "fp", R_FP },
							{ "epc", R_EPC },
							
							{ "at", R_AT },
							{ "gp", R_GP },

							{ "ecc", R_ECC },

							{ "sr", R_SR },
							{ "cause", R_CAUSE },
							{ "badvaddr", R_BADVADDR },
							{ 0, 0 }
							};




void BoxDebugger(void)
{
	char cmd[66];
	
	gCrashLogPrintfs = false;
	
	strcpy((char*)prompt, "@");
	
	while (1)
	{
		int i;
		
		if(SPOTVersion() == kSPOT3)
			for(i=0;i<1024;i++)
				*(volatile ulong*)kMemCmdReg = 0x48000000;
			
		printf("%s", prompt);	
		getstring((uchar*)cmd, 64, 0);		/* get user command */
		putchar('\n');
		ParseCommandLine(cmd);
	}
}




void ParseCommandLine(char* s)
{
	char*	argv[64];
	char*	args;
	ulong	argc;
	ulong	iii;
	char temp[64];
	
	strcpy(temp, s);			/* make a copy in case we want to add it to the history list */
	
	argc = 0;
	args = s;
	argv[0] = s;

	if ( (*s != '\n') && (*s != '\r') && (*s != 0) )		/* is there SOMETHING there? */
	{
		while (*args == ' ')				/* skip over any leading spaces */
			args++;		

		do
		{
			if ( (*args == ' ') || (*args == '\n') || (*args == '\r') )
			{
				*args = 0;						/* isolate the substring */
				
				if ( (*args != '\n') && (*args != '\r') )		/* EOL? */
				{
					args++;						/* point at next possible substring */

					argc++;						/* we have another arg now */
					argv[argc] = args;

					while (*args == ' ')		/* skip over any leading spaces on the next substring */
						args++;	
				}
			}
			else
				args++;

		}
		while ( *args != 0 );

		
		/* was it a command? */

		iii =0;
		while (cmdTable[iii].cmd)
		{
			if (strcmp((const char*)argv[0], cmdTable[iii].cmd) == 0)
			{
#if defined APPROM && defined DEBUG
				AddToHistory(temp);				/* only add commands to history list */
#endif
				cmdTable[iii].fn(argc, argv);
				return;
			}
			iii++;
		}
		
		/* was it a reg get/set? */
		
		iii =0;
		while (regTable[iii].regname)
		{
			if (strcmp((const char*)argv[0], regTable[iii].regname) == 0)
			{
				doReg(argc, argv, regTable[iii].regindex);
				return;
			}
			iii++;
		}
		

#if defined APPROM && defined DEBUG		
		/* was it a history cmd? */
								
		if ( (argv[0][0] == '!') && (isdigit(argv[0][1])) )
		{
			argv[0][0] = '#';
#ifdef DEBUG
			DoHistory(stonum(argv[0]));
#endif
			return;
		}
		
		if ( (argv[0][0] == '!') && (argv[0][1] == '!') )
		{
#ifdef DEBUG
			DoHistory(0);
#endif
			return;
		}
#endif


		/* was it just a number? */
		
		if ((argv[0][0] == '#') || (isxdigit(argv[0][0])))
		{
		long x;
			x = stonum(argv[0]);
			printf("Num: 0x%x = %u = %d\n", x, x, x);
			return;
		}
		
		printf(">> %s: Unknown command\n", s);
	}
}



/*	--------------------------------------------------------------------------------------------
	REGISTER FUNCTIONS
 
 	<reg>				displays <reg>
	<reg> <value> 		sets <reg> to <value>
*/
 
void doReg(ulong argc, char* argv[], ulong regindex)
{
	if (argc == 1)
		printf("%s = 0x%08lx (%u, %d)\n", argv[0], except_regs[regindex], except_regs[regindex], except_regs[regindex]);
	else
	{
		except_regs[regindex] = stonum(argv[1]);
		printf("%s = 0x%08lx (%u, %d)\n", argv[0], except_regs[regindex], except_regs[regindex], except_regs[regindex]);
	}
}



/*	--------------------------------------------------------------------------------------------
	COMMAND FUNCTIONS
*/


/*	--------------------------------------------------------------------------------------------
	Memory/SPOT manipulation
 */

void DumpSPOTRegister(ulong argc, char* argv[])
{
	ulong*	addr;
	ulong	data;

	if (argc == 2)
	{
		addr = (ulong*)ParsePossibleReg(argv[1]);
		addr = (ulong*)((ulong)addr & 0x0000fffc);
		addr = (ulong*)((ulong)addr + 0xa4000000);			/* SPOT base addr */
		data = *addr;
		printf("0x%x: 0x%x (%d)\n", addr, data, data);
	}
	else
		printf("usage: dr <SPOT Offset>\n");
}


void SetSPOTRegister(ulong argc, char* argv[])
{
	ulong*	addr;

	if (argc == 3)
	{
		addr = (ulong*)ParsePossibleReg(argv[1]);
		addr = (ulong*)((ulong)addr & 0x0000fffc);
		addr = (ulong*)((ulong)addr + 0xa4000000);			/* SPOT base addr */
		*addr = (ulong)stonum(argv[2]);
	}
	else
		printf("usage: sr <SPOT Offset> <val>\n");
}


void SetLongs(ulong argc, char* argv[])
{
	ulong*	addr;
	ulong	iii;

	if (argc >= 3)
	{
		addr = (ulong*)ParsePossibleReg(argv[1]);
		addr = (ulong*)((ulong)addr & 0xfffffffc);
		iii = 2;
		while (iii != argc)
			*addr++ = (ulong)stonum(argv[iii++]);
	}
	else
		printf("usage: sl <start> <data...>\n");
}


void SetBytes(ulong argc, char* argv[])
{
	char*	addr;
	ulong	iii;

	if (argc >= 3)
	{
		addr = (char*)ParsePossibleReg(argv[1]);
		iii = 2;
		while (iii < argc)
			*addr++ = (char)stonum(argv[iii++]);
	}
	else
		printf("usage: sb <start> <data...>\n");
}


void SetHalfwords(ulong argc, char* argv[])
{
	ushort *addr;
	ulong iii;

	if (argc >= 3)
	{
		addr = (ushort *)ParsePossibleReg(argv[1]);
		addr = (ushort *)((ulong)addr & 0xfffffffe);
		iii = 2;
		while (iii < argc)
			*addr++ = (ushort)stonum(argv[iii++]);
	}
	else
		printf("usage: sh <start> <data...>\n");
}




/*
	dm <start> <len> - start & len are optional.  if you leave 'em off, it uses the last 
						len & whatever the current dump offset is set to.
 */
 

ulong		curDumpStart = 0x80000000;
ulong		lastLen = 256;
const char	cvt[17] = "0123456789abcdef";

void DumpMem(ulong argc, char* argv[])
{
	ulong*	addr;
	ulong len;
	ulong iii;
	uchar c;
	uchar tx[17];
	ulong hwsux;

	if (argc <= 3)
	{
		if (argc >= 2)
			addr = (ulong*)ParsePossibleReg(argv[1]);
		else
			addr = (ulong*)curDumpStart;
			
		if (argc == 3)
			lastLen = len = stonum(argv[2]);
		else
			len = lastLen;
			
		len >>= 4;		/* len is in terms of 16 byte chunks */
		if (!len)
			len = 1;
	
		addr = (ulong*)((ulong)addr & 0xfffffffc);
		
		if ( !((ulong)addr & 0x80000000) )			/* kseg0/1 addr? */
		{
			printf(">> %x: Invalid address\n");
			return;
		}
	
		tx[16] = 0;

		while (len--)
		{
			printf("%x: ", addr);

			for(iii=0;iii!=16;iii++)
			{			
				hwsux = *addr++;
				c = (uchar)(hwsux>>24);
				putchar(cvt[(c>>4) & 0xf]);
				putchar(cvt[c&0xf]);
				if (isprint(c))
					tx[iii++]=c;
				else
					tx[iii++]='.';
				
				c = (uchar)(hwsux>>16);
				putchar(cvt[(c>>4) & 0xf]);
				putchar(cvt[c&0xf]);
				if (isprint(c))
					tx[iii++]=c;
				else
					tx[iii++]='.';

				c = (uchar)(hwsux>>8);
				putchar(cvt[(c>>4) & 0xf]);
				putchar(cvt[c&0xf]);
				if (isprint(c))
					tx[iii++]=c;
				else
					tx[iii++]='.';

				c = (uchar)hwsux;
				putchar(cvt[(c>>4) & 0xf]);
				putchar(cvt[c&0xf]);
				if (isprint(c))
					tx[iii]=c;
				else
					tx[iii]='.';
				
				putchar(' ');
			}	
			printf(" %s\n", tx);
		}
		
		curDumpStart = (ulong)addr;
	}
	else
		printf("usage: dm <start> <length>\n");

}


void DumpSingleLong(ulong argc, char* argv[])
{
	ulong*	addr;
	ulong	data;

	if (argc == 2)
	{
		addr = (ulong*)ParsePossibleReg(argv[1]);
		addr = (ulong*)((ulong)addr & 0xfffffffc);
		data = *addr;
		printf("0x%x: 0x%x (%d)\n", addr, data, data);
	}
	else
		printf("usage: dsl <addr>\n");
}


void DumpSingleByte(ulong argc, char* argv[])
{
	char*	addr;
	ulong	data;

	if (argc == 2)
	{
		addr = (char*)ParsePossibleReg(argv[1]);
		data = (ulong)*addr;
		printf("0x%x: 0x%x (%d)\n", addr, data, data);
	}
	else
		printf("usage: dsb <addr>\n");
}


void DumpSingleHalfword(ulong argc, char* argv[])
{
	ushort *addr;
	ulong data;

	if (argc == 2)
	{
		addr = (ushort *)ParsePossibleReg(argv[1]);
		addr = (ushort *)((ulong)addr & 0xfffffffe);
		data = (ushort)*addr;
		printf("0x%x: 0x%x (%d)\n", addr, data, data);
	}
	else
		printf("usage: dsh <addr>\n");
}



/*	--------------------------------------------------------------------------------------------
	Disassembler
*/

#define kMiddle 1
#define kStart  0

extern int disasm(unsigned long *addr, unsigned long *mem_addr, int mode, int regset);
void DisassembleCommon(ulong argc, char* argv[], ulong middle);

ulong linesToDis = 24;

void DisassembleBeginning(ulong argc, char* argv[])
{
	DisassembleCommon(argc, argv, kStart);
}


void DisassembleMiddle(ulong argc, char* argv[])
{
	DisassembleCommon(argc, argv, kMiddle);
}

ulong* FindFunctionEntry(ulong *addr)
{
	do
	{
		addr--;
		if (!((ulong)addr & 0x80000000))
		{
			return 0;
		}
	}
	while (*addr != 0x03e00008);
	
	addr++;		/* jr ra */
	addr++;		/* delay slot */
	return addr;
}

void DisassembleFunction(ulong argc, char* argv[])
{
	ulong*	addr;

	if (argc >= 2)							/* telling us an address? */
	{
		addr = (ulong*)ParsePossibleReg(argv[1]);
		
		if (argc == 3)						/* telling us an instruction count too? */
			linesToDis = stonum(argv[2]);
	}
	else
		addr = (ulong*)curDumpStart;
		
	addr = (ulong*)((ulong)addr & ~3);		/* all instructions are long-aligned */

	addr = FindFunctionEntry(addr);
	if(addr)
		DisassembleLoop(addr,linesToDis);
	else
		printf("Hmm, I couldn't find the function start.  addr = 0x%x.\n", addr);

}


void DisassembleCommon(ulong argc, char* argv[], ulong middle)
{
	ulong*	addr;

	if (argc >= 2)							/* telling us an address? */
	{
		addr = (ulong*)ParsePossibleReg(argv[1]);
		
		if (argc == 3)						/* telling us an instruction count too? */
			linesToDis = stonum(argv[2]);
	}
	else
		addr = (ulong*)curDumpStart;

	if (middle)
		addr -= (linesToDis >> 1);
	
	addr = (ulong*)((ulong)addr & ~3);		/* all instructions are long-aligned */
	
	DisassembleLoop(addr,linesToDis);
}


#define DO_DELAY	2	/* disassemble the delay slot inst too */

void DisassembleLoop(ulong* addr,ulong nLines)
{
	ulong n;
	ulong cnt;
	ulong mode = 0;
	
	OpenSymbolFile();
	
	if(nLines == 1)
		mode = DO_DELAY;
	
	printf("Disassembling from %x\n",addr);
	if(gSymbols)
	{
		Symbol *s;
		s = FindSymbol((ulong)addr);
		if(s && ((ulong)addr-s->address))
			printf("  %s\n",s->symbol);
	}
	
	for (n = 0; n < nLines; n++)
	{
		if((ulong)addr == (ulong)except_regs[R_EPC])
			printf("* ");
		else
			printf("  ");
		cnt = disasm(addr, addr, mode, 0);
		printf("\n");
		cnt >>= 2;
		addr += cnt;
	}
	
	curDumpStart = (ulong)addr;
}




/*	--------------------------------------------------------------------------------------------
	UTILS
 */

/* front end for strtol, makes it more like macsbug.
   	#num = dec num
   	num = hex num
	0xnum = hex num
 
 	can have +/- sign on num (after # for dec nums)
 */

long stonum(char* s)
{
	if (*s == '#')
	{
		s++;
		return(strtol(s, 0, 10));
	}
	else
		return(strtoul(s, 0, 16));
}


ulong ParsePossibleReg(char* s)
{
	ulong iii = 0;
	Symbol *sym;
	
	while (regTable[iii].regname)
	{
		if (strcmp((const char*)s, regTable[iii].regname) == 0)
			return(except_regs[regTable[iii].regindex]);

		iii++;
	}
	
	if((sym = FindSymbolByName(s)))
		return sym->address;

	return(stonum(s));
}


void DisplaySSN(ulong argc, char* argv[])
{
	printf("SSN = %08lx %08lx\n", READ_AG(agSerialNumberHi), READ_AG(agSerialNumberLo));
}




/*	--------------------------------------------------------------------------------------------
	Fillers & Verifiers
*/

void FillMem(ulong argc, char* argv[])
{
	ulong*	testStart;
	ulong*	testEnd;
	ulong pattern;
	ulong*	iii;

	if (argc == 4)
	{
		testStart = (ulong*)ParsePossibleReg(argv[1]);
		testStart = (ulong*)((ulong)testStart & 0xfffffffc);
		
		testEnd = (ulong*)ParsePossibleReg(argv[2]);
		testEnd = (ulong*)((ulong)testEnd & 0xfffffffc);
		
		pattern = stonum(argv[3]);
		
		printf("Filling...\n");
		
		for (iii=testStart;iii<=testEnd;iii++)		/* fill 'em up */
				*iii = pattern;
	}
	else
		printf("usage: f <start WORD> <end WORD> <WORD pat>\n");
}


void FillByteMem(ulong argc, char* argv[])
{
	uchar*	testStart;
	uchar*	testEnd;
	ulong 	pattern;
	uchar*	iii;

	if (argc == 4)
	{
		testStart = (uchar*)ParsePossibleReg(argv[1]);
		testEnd = (uchar*)ParsePossibleReg(argv[2]);
		
		pattern = stonum(argv[3]);
		
		printf("Filling...\n");
		
		iii = testStart;
	
		while (iii<=testEnd)		/* fill 'em up */
		{
				*iii = (pattern>>24);
				iii++;
				if (iii<=testEnd)
				{
					*iii = (pattern>>16);
					iii++;
				}
				if (iii<=testEnd)
				{
					*iii = (pattern>>8);
					iii++;
				}
				if (iii<=testEnd)
				{
					*iii = pattern;
					iii++;
				}
		}
	}
	else
		printf("usage: fb <start BYTE> <end BYTE> <WORD pat>\n");
}


void FillMemCount(ulong argc, char* argv[])
{
	ulong*	testStart;
	ulong testCount;
	ulong pattern;
	ulong*	iii;
	ulong*	end;

	if (argc == 4)
	{
		testStart = (ulong*)ParsePossibleReg(argv[1]);
		testStart = (ulong*)((ulong)testStart & 0xfffffffc);
		
		testCount = ParsePossibleReg(argv[2]);

		pattern = stonum(argv[3]);
		
		end = testStart + testCount;
		
		printf("Filling...\n");
		
		for (iii=testStart;iii!=end;iii++)		/* fill 'em up */
				*iii = pattern;
	}
	else
		printf("usage: fc <start WORD> <WORD cnt> <WORD pat>\n");
}


void FillByteMemCount(ulong argc, char* argv[])
{
	uchar*	testStart;
	uchar testCount;
	ulong pattern;
	uchar*	iii;
	uchar*	end;

	if (argc == 4)
	{
		testStart = (uchar*)ParsePossibleReg(argv[1]);
		
		testCount = ParsePossibleReg(argv[2]);

		pattern = stonum(argv[3]);
		
		end = testStart + testCount;
		
		printf("Filling...\n");
						
		iii = testStart;
						
		while (iii<end)		/* fill 'em up */
		{
				*iii = (pattern>>24);
				iii++;
				if (iii!=end)
				{
					*iii = (pattern>>16);
					iii++;
				}
				if (iii!=end)
				{
					*iii = (pattern>>8);
					iii++;
				}
				if (iii!=end)
				{
					*iii = pattern;
					iii++;
				}
		}
	}
	else
		printf("usage: fbc <start BYTE> <BYTE cnt> <WORD pat>\n");
}


/*	--------------------------------------------------------------------------------------------
	Exceptions
*/
 
#define kIWatchEnable	(1<<0)
#define kWriteWatchBit  (1<<1)
#define kReadWatchBit   (1<<2)

ulong	gBreakPoint = 0;
Boolean gBreakContinue = false;

/*
	v0 = 00000000    a0 = 00000000    ra = 00000000
	v1 = 00000000    a1 = 00000000    sp = 00000000
					 a2 = 00000000    fp = 00000000
					 a3 = 00000000   epc = 00000000 
					 
	t0 = 00000000    s0 = 00000000    at = 00000000
	t1 = 00000000    s1 = 00000000    gp = 00000000
	t2 = 00000000    s2 = 00000000   
	t3 = 00000000    s3 = 00000000
	t4 = 00000000    s4 = 00000000     
	t5 = 00000000    s5 = 00000000   
	t6 = 00000000    s6 = 00000000
	t7 = 00000000    s7 = 00000000
	t8 = 00000000    
	t9 = 00000000

          sr = 00000000
	   cause = 00000000
	badvaddr = 00000000
*/


void
printhex(long xx)
{
	short i, v;
	long x = xx;
	for ( i=0; i < 8; i++ ) {
		v = (x>>28) & 0xf;
		if ( v < 0xa ) 
			putchar('0'+v);
		else
			putchar ('A'+(v-0xa));
		x<<= 4;
	}
	x = xx;
	putchar('\n');
	for ( i=0; i < 8; i++ ) {
		v = (x>>28) & 0xf;
		while (v--)
			putchar('*');
		putchar('\n');
		x<<= 4;
	}
}


void DumpExceptionRegs(ulong argc, char* argv[])
{
	printf("\n");
	printf("v0 = %08lx    a0 = %08lx    ra = %08lx\n",	except_regs[R_V0], except_regs[R_A0], except_regs[R_RA]);
	printf("v1 = %08lx    a1 = %08lx    sp = %08lx\n",	except_regs[R_V1], except_regs[R_A1], except_regs[R_SP]);
	printf("                 a2 = %08lx    fp = %08lx\n",	                   except_regs[R_A2], except_regs[R_FP]);
	printf("                 a3 = %08lx   epc = %08lx\n",	                   except_regs[R_A3], except_regs[R_EPC]);




	printf("\n");
	printf("t0 = %08lx    s0 = %08lx    at = %08lx\n",	except_regs[R_T0], except_regs[R_S0], except_regs[R_AT]);
	printf("t1 = %08lx    s1 = %08lx    gp = %08lx\n",	except_regs[R_T1], except_regs[R_S1], except_regs[R_GP]);
	printf("t2 = %08lx    s2 = %08lx\n",	            except_regs[R_T2], except_regs[R_S2]);
	printf("t3 = %08lx    s3 = %08lx    k0 = %08lx\n",  except_regs[R_T3], except_regs[R_S3], except_regs[R_K0]);
	printf("t4 = %08lx    s4 = %08lx    k1 = %08lx\n",  except_regs[R_T4], except_regs[R_S4], except_regs[R_K1]);
	printf("t5 = %08lx    s5 = %08lx\n",	            except_regs[R_T5], except_regs[R_S5]);
	printf("t6 = %08lx    s6 = %08lx\n",	            except_regs[R_T6], except_regs[R_S6]);
	printf("t7 = %08lx    s7 = %08lx\n",	            except_regs[R_T7], except_regs[R_S7]);
	printf("t8 = %08lx\n",	                        except_regs[R_T8]);
	printf("t9 = %08lx\n",	                        except_regs[R_T9]);
	printf("\n");
	printf("      sr = %08lx\n",                     except_regs[R_SR]);
	printf("   cause = %08lx\n",                     except_regs[R_CAUSE]);
	printf("badvaddr = %08lx\n",                     except_regs[R_BADVADDR]);
	printf("\n");	
}

void DumpExceptionRegs2(ulong argc, char* argv[])
{
#ifdef CPU_R4640

	printf("   ibase = %08lx    ibound = %08lx\n", except_regs[R_IBASE], except_regs[R_IBOUND]);    
	printf("   dbase = %08lx    dbound = %08lx\n", except_regs[R_DBASE], except_regs[R_DBOUND]);    
	printf("  iwatch = %08lx    dwatch = %08lx\n", except_regs[R_IWATCH], except_regs[R_DWATCH]);
	printf("   count = %08lx   compare = %08lx\n", except_regs[R_COUNT], except_regs[R_COMPARE]);
	printf("    prid = %08lx    config = %08lx\n", except_regs[R_PRID], except_regs[R_CONFIG]);
	printf("    calg = %08lx       ecc = %08lx\n", except_regs[R_CALG], except_regs[R_ECC]);
	printf("cacheerr = %08lx     taglo = %08lx\n", except_regs[R_CACHEERR], except_regs[R_TAGLO]);
	printf("errorepc = %08lx\n", except_regs[R_ERROREPC]);
	printf("\n");
	
#endif /* CPU_R4640 */
}


char	gHowString[64];

ulong gOldBlankColor,gOldVidFCntl;

void debugErrEntry(void)			/* assumes that the serial port is still set up */
{	
	Boolean fullDump = true;

	gCrashLogPrintfs = false;	
	gPrintEnable = true;			/* make sure we can see output! */
	
	gOldBlankColor = *(volatile ulong *)kVidBlnkColReg;
	*(volatile ulong *)kVidBlnkColReg = 0x2fcffd;			/* red border */
	gOldVidFCntl = *(volatile ulong *)kVidFCntlReg;
	*(volatile ulong *)kVidFCntlReg = (gOldVidFCntl | kBlankColorEnable);

	switch((except_regs[R_CAUSE] & 0x7c) >> 2)
	{
		case 0: 
		{
				ulong err = *(volatile ulong*)kBusErrStatusReg;
				if(err & ( kFence1ReadIntMask | kFence2ReadIntMask ))
				{
					*(volatile ulong*)kBusErrStatusClearReg = (kFence1ReadIntMask | kFence2ReadIntMask);
					sprintf(gHowString,"Fence read violation @ %08lx\n",*(volatile ulong*)kBusErrAddressReg);
				} 
				else if(err & ( kFence1WriteIntMask | kFence2WriteIntMask ))
				{
					*(volatile ulong*)kBusErrStatusClearReg = ( kFence1WriteIntMask | kFence2WriteIntMask );
					sprintf(gHowString,"Fence write violation @ %08lx\n",*(volatile ulong*)kBusErrAddressReg);
				} 
				else if(err & kBusTimeoutIntMask)
				{
					*(volatile ulong*)kBusErrStatusClearReg = kBusTimeoutIntMask;
					sprintf(gHowString,"Bus Timeout (perhaps an attempt to write to flash)");
				}
				else
				{
					sprintf(gHowString,"Interrupt");
				}
				break;
		}
		case 1:
				sprintf(gHowString,"Exception: Reserved");
				break;
		case 2:
				sprintf(gHowString,"Exception: I Bound");
				break;
		case 3:
				sprintf(gHowString,"Exception: D Bound");
				break;
		case 4:
				sprintf(gHowString,"Exception: Addr Err on Load");
				break;
		case 5:
				sprintf(gHowString,"Exception: Addr Err on Store");
				break;
		case 6:
				sprintf(gHowString,"Exception: Bus Err on I Fetch");
				break;
		case 7:
				sprintf(gHowString,"Exception: Bus Err on D Load");
				break;
		case 8:
				sprintf(gHowString,"Exception: Syscall");
				break;
		case 9:
			{
				ulong code;
				code = (*(ulong*)except_regs[R_EPC] >> 16) & 0xFFFF;
				switch(code)
				{
					case 0x7:
						sprintf(gHowString,"Exception: Divide by zero");
						break;
					default:
						sprintf(gHowString,"Exception: Breakpoint");
						break;
				}
				break;
			}
		case 10:
				sprintf(gHowString,"Exception: Reserved Instruction");
				break;
		case 11:
				sprintf(gHowString,"Exception: Coprocessor Unusable");
				break;
		case 12:
				sprintf(gHowString,"Exception: Overflow");
				break;
		case 13:
				sprintf(gHowString,"Trap Exception");
				break;
		case 15:
				sprintf(gHowString,"Floating Point Exception");
				break;
		case 23:
				if( except_regs[R_CAUSE] & (1<<24) )
				{
					if( (except_regs[R_EPC] & ~0x3) != gBreakPoint )
					{
						/* must be stepping or trying to continuing after breakpoint */
						if(gBreakContinue)
						{
							/* continuing from breakpoint */
							gBreakContinue = false;
							except_regs[R_IWATCH] = (gBreakPoint | kIWatchEnable);
							ReturnFromException(0,0);
						}
						else
						{
							/* stepping */
							fullDump = false;
							NextInstrAddr((ulong*)except_regs[R_EPC],0,1);
						}
					}
					else
					{
						sprintf(gHowString,"IWatch Exception");
					}
				}
				else if( except_regs[R_CAUSE] & (1<<25) )
					sprintf(gHowString,"DWatch Exception");
				else
					sprintf(gHowString,"Watch Exception?");
				break;
		default:
				sprintf(gHowString,"Unknown exception type (Weird!)");
				break;
	}
	
	if(fullDump)
	{
		printf("\n%s\n",gHowString);
		DumpExceptionRegs(0, 0);
	}
	DisassembleLoop((ulong*)(except_regs[R_EPC] & ~3),1);
	BoxDebugger();	
}

/* Special.  Never returns.  Restores exception context, whatever it is. */

void ReturnFromException(ulong argc, char* argv[])
{
	*(volatile ulong *)kVidBlnkColReg = gOldBlankColor;			/* red border */
	*(volatile ulong *)kVidFCntlReg = gOldVidFCntl;

	gCrashLogPrintfs = true;	

	returnFromDebugger();
}

void Breakpoint(ulong argc, char* argv[])
{
	ulong addr;
	ulong mask = 0x3;
	
	if(argc == 2)
	{
		addr = ParsePossibleReg(argv[1]);
		addr &= ~mask;
		printf("Breakpoint set at %08lx\n", addr);
		except_regs[R_IWATCH] = (addr | kIWatchEnable);
		gBreakPoint = addr;
	}
	else
	{
		printf("usage: br <address>\n");
	}
}

void BreakpointClear(ulong argc, char* argv[])
{
	except_regs[R_IWATCH] = 0;
	gBreakPoint = 0;
}

void BreakpointDisplay(ulong argc, char* argv[])
{
	printf("Breakpoint set at %08lx\n",gBreakPoint);
}

void Go(ulong argc, char* argv[])
{
	ulong next;

	if( ( except_regs[R_EPC] & ~0x3 ) == ( except_regs[R_IWATCH] & ~0x3 ) )
	{
		/* 
			The IWatch is pointing at the current instruction.
			We need to bump the IWatch register to point to next
			instr. when we take the subsequent exception, we'll
			restore IWatch back to the original break point value
			and continue as if nothing happened.
		*/
		gBreakContinue = true;
		next = (ulong)NextInstrAddr((ulong*)except_regs[R_EPC],0,0);
		except_regs[R_IWATCH] =  next | kIWatchEnable;
	} 
	else if ((*(ulong*)except_regs[R_EPC]  & 0xFC00003F) == 0xD)
	{
		/* it's a BREAK instruction, so skip over it */
		except_regs[R_EPC] += 4;
	}

	gCrashLogPrintfs = true;	

	ReturnFromException(0,0);
}

void DataWatchClear(ulong argc, char* argv[])
{
	except_regs[R_DWATCH] = 0;
}

void DataWatch(ulong argc, char* argv[])
{
	ulong addr;
	ulong mask = 0x7;
	
	if(argc == 2)
	{
		addr = ParsePossibleReg(argv[1]);
		addr &= ~mask;
		printf("Data read/write watch set for %x\n", addr);
		except_regs[R_DWATCH] = ( addr | kWriteWatchBit | kReadWatchBit );	/* enable DWatch for reads/writes */
	}
	else
	{
		printf("usage: dww <address>\n");
	}
}

void DataWriteWatch(ulong argc, char* argv[])
{
	ulong addr;
	ulong mask = 0x7;
	
	if(argc == 2)
	{
		addr = ParsePossibleReg(argv[1]);
		addr &= ~mask;
		printf("Data write watch set for %x\n", addr);
		except_regs[R_DWATCH] = ( addr | kWriteWatchBit );	/* enable DWatch for writes */
	}
	else
	{
		printf("usage: dww <address>\n");
	}
}

void DataReadWatch(ulong argc, char* argv[])
{
	ulong addr;
	ulong mask = 0x7;
	
	if(argc == 2)
	{
		addr = ParsePossibleReg(argv[1]);
		addr &= ~mask;
		printf("Data read watch set for %x\n", addr);
		except_regs[R_DWATCH] = ( addr | kReadWatchBit);	/* enable DWatch for reads */
	}
	else
	{
		printf("usage: drw <address>\n");
	}
}




/*	############################################################################################
	APPROM ONLY
*/

#ifdef APPROM
#ifdef DEBUG

/*	--------------------------------------------------------------------------------------------
	FILES
 */

void CreateFileFromMemory(ulong argc, char* argv[])
{
	if (argc == 4)
	{
		Create(argv[1], (char*)stonum(argv[2]), stonum(argv[3]) + 1);
	}
	else
		printf("usage: addfile <name> <data addr> <size>\n");
}


void CreateDirectory(ulong argc, char* argv[])
{
	if (argc == 2)
	{
		Create(argv[1], nil, 0);
	}
	else
		printf("usage: mkdir <pathname>\n");
}

void DeleteFile(ulong argc, char* argv[])
{
	if (argc == 2)
	{
		Remove(argv[1]);
	}
	else
		printf("usage: delfile <path>\n");
}


/*	
	print a list of the available filesystems
 */

void DumpFilesystems(ulong argc, char* argv[])
{
	char*	fsName = GetFirstFilesystemName();
	
	while (fsName != nil)
	{
		printf("%s\n", fsName);
		fsName = GetNextFilesystemName((uchar*)fsName);
	}
}


/*	
	print a list of files in the specified filesystem or starting at the file.
 */

char curDir[32] = "/RAM";

void listfiles(FSNode *fsn)
{
	if (fsn != nil)
	{
		if (!fsn->data)		
			printf("%s\t\t- directory -\n", fsn->name);
		else
			printf("%s\t\t@ 0x%x, %d bytes\n", fsn->name, fsn->data, fsn->dataLength);

		listfiles(fsn->next);		
	}
}

void DumpDirectory(ulong argc, char* argv[])
{
	FSNode*		fsn;

	if (argc == 2)
		strcpy(curDir, argv[1]);

	fsn = Resolve(curDir, 1);
	
	if (fsn->first)
	{
		printf("%s:\n", curDir);
		listfiles(fsn->first);
	}
	else
		printf(">> not a directory\n");
}


void DumpHelp(ulong argc, char* argv[])
{
	printf("\nCommands:\n\n");
	
	printf("   dr <SPOT Offset>               read & display 0xa4000000+<SPOT Offset>\n");
	printf("   sr <SPOT Offset> <val>         set 0xa4000000+<SPOT Offset> to <val>\n");
	printf("\n");
	printf("   dm <start> <length>            dump a range of memory\n");
	printf("   dsl <addr>                     read & display a single longword\n");
	printf("   dsh <addr>                     read & display a single halfword\n");
	printf("   dsb <addr>                     read & display a single byte\n");
	printf("\n");
	printf("   sl <start> <data...>           set long(s)\n");
	printf("   sh <start> <data...>           set halfword(s)\n");
	printf("   sb <start> <data...>           set byte(s)\n");
	printf("\n");
	printf("   f <start> <end> <pat>          fill WORD mem range with WORD <pat>\n");
	printf("   fb <start> <end> <pat>         fill BYTE mem range with WORD <pat>\n");
	printf("   fc <start> <cnt> <pat>         fill <cnt> WORDS from <start> with WORD <pat>\n");
	printf("   fbc <start> <cnt> <pat>        fill <cnt> BYTES from <start> with WORD <pat>\n");
	printf("   v <start> <end> <pat>          verify WORD mem range with WORD <pat>\n");
	printf("   vb <start> <end> <pat>         verify BYTE mem range with WORD <pat>\n");
	printf("   vc <start> <cnt> <pat>         verify <cnt> WORDS from <start> with WORD <pat>\n");
	printf("   vbc <start> <cnt> <pat>        verify <cnt> BYTES from <start> with WORD <pat>\n");
	printf("\n");
	printf("   il <addr> <cnt>                disassemble instructions\n");
	printf("   ip <addr> <cnt>                disassemble half page around addr\n");
	printf("   if <addr> <cnt>                disassemble fn starting before <addr>\n");
	printf("                                    (<cnt> is optional)\n");
	printf("   er                             dump exception reg snapshot\n");
	printf("   er2                            dump special regs napshot\n");
	printf("   <regname>                      displays register <regname>\n");
	printf("   <regname> <value>              set register <regname> to <value>\n");
	printf("   g (or go)                      restore context & resume execution.\n");
	printf("\n");
	printf("   hist                           display history buf\n");
	printf("   !!                             repeat last command\n");
	printf("   !<num>                         repeat command <num>\n");
	printf("\n");
	printf("   cache <on | wthru | wback>     set cache mode\n");
	printf("\n");
	printf("   br <addr>                      set breakpoint at  <addr>\n");
	printf("   brc                            clear breakpoint\n");
	printf("   brd                            display current breakpoint\n");
	printf("\n");
	printf("   s                              step into\n");
	printf("   t                              trace (step over)\n");
	printf("   sc                             stack crawl\n");
	printf("   dw <addr>                      set read/write watch at  <addr>\n");
	printf("   dww <addr>                     set write watch at <addr>\n");
	printf("   dwr <addr>                     set read watch at  <addr>\n");
	printf("   dwc                            clear watch point\n");
	printf("\n");
	printf("   hd                             heap dump\n");
	printf("   ht                             heap totals\n");
	printf("\n");
	printf("   rb                             hard reset box\n");
	printf("   ssn                            display box silicon serial number\n");
	printf("   how                            show how we got into debugger\n");
	printf("   dv <system global>             display fields of a browser system global\n");
	printf("   whereis  <symbol>              display address of a symbol\n");
	printf("   where 	<addr>                display symbol of addr\n");
	printf("   whichobj	<addr>                display name of object at location\n");
	printf("\n");
	printf("   help2                          show help for more obscure commands\n");

	printf("\n\n");
}



void DumpHelp2(ulong argc, char* argv[])
{
	printf("\nCommands:\n\n");
	
	printf("   addfile <name> <data addr> <size>    add a file, specify complete path\n");
	printf("   alloc <size>                         allocate a block from the system heap\n");
	printf("   mkdir <pathname>                     create a new directory\n");
	printf("   ls                                   display files in filesystems\n");
	printf("   fs                                   display available filesystems\n");
	printf("\n");

#ifdef DEBUG
#ifndef BOOTROM
	printf("   ir                                   test IR receiver\n");
	printf("   led                                  test front panel leds\n");
#endif /* BOOTROM */
#endif /* DEBUG */

	printf("   help                                 show main help text\n");

	printf("\n\n");
}


/*	--------------------------------------------------------------------------------------------
	HEAPS
 */

/* allocate a chunk of memory from the system heap */
 
void GetASysHeapBlock(ulong argc, char* argv[])
{
	ulong	size;
	void*	start;

	if (argc == 2)
	{
		size = stonum(argv[1]);
		start = AllocateTaggedZero(size, 0x69);
		printf("your 0x%x (%d) byte block starts at 0x%x.\n", size, size, start);
	}
	else
		printf("usage: alloc <size>\n");
}


void FreeASysHeapBlock(ulong argc, char* argv[])
{
	if (argc == 2)
		FreeTaggedMemory((void *)stonum(argv[1]), nil);
	else
		printf("usage: free <addr>\n");
}


void DumpHeap(ulong argc, char* argv[])
{
#ifdef MEMORY_TRACKING
	void*	block; 
	Boolean	used; 
	ulong	length; 
	char	name[32];
	char	ownerURL[32];
	Boolean	isCPlus;
	char	sourceFile[32];
	ulong	lineNumber;

	block = nil; 

	printf(" %s\t        %s %s   %s\t\t%s %s %s %s\n", "block", "used", "length", "CPlus", "name", "ownerURL", "file", "line");

	do
	{
		block = NextMemoryBlock(block, &used, &length, name, ownerURL, &isCPlus, sourceFile, &lineNumber);
		if (block != nil)
			printf("%x\t%x    %x\t\t%x\t%s %s %s %d\n", ((ulong)block)+8, used, length, isCPlus, name, ownerURL, sourceFile, lineNumber);
	}
	while (block != nil);
#else
	DumpHeapBlocks(1);
#endif
}

void DumpHeapTotals(ulong argc, char* argv[])
{
	DumpHeapBlocks(0);
}


const char *buttonText[0xf] = {

						"kUpKey",				/* 0 */
						"kLeftKey",				/* 1 */
						"kRightKey",			/* 2 */
						"kDownKey",				/* 3 */
						"kOptionsKey",			/* 4 */
						"kExecuteKey",			/* 5 */
						"kPowerKey",			/* 6 */
						"kForwardKey",			/* 7 */
						"kBackKey",				/* 8 */
						"kScrollUpKey",			/* 9 */
						"kScrollDownKey",		/* a */
						"kRecentKey",			/* b */
						"kHomeKey",				/* c */
						"kFavoriteKey",			/* d */
						"kRemoteEnterKey"		/* e */
	
						};

extern rawIR irData[kIRBufSize];
extern ulong irHead;
extern ulong irTail;
extern ushort *sej2lcp;

void DumpIRBuffer(ulong argc, char* argv[])
{
ulong iii;
uchar kbd;

	for(iii=0;iii!=kIRBufSize;iii++)
	{
		if ((irHead == irTail) && (iii==irHead))
			printf("H T ");
		else
			if(iii==irHead)
				printf("H   ");
			else
				if(iii==irTail)
					printf("  T ");
				else
					printf("    ");
	
		if(irData[iii].kbData)			/* Sejin KB or Sony remote? */
		{
			printf("KB: %02x %02x @ %08lx --> ", irData[iii].kbData, irData[iii].category1, irData[iii].time);

			if (isprint(sej2lcp[irData[iii].category1]))
			{
				kbd = sej2lcp[irData[iii].category1];
				printf("'%c'", kbd);
			}
			else
				if(sej2lcp[irData[iii].category1] & 0x8000)		/* remote emulation key? */
					printf("%s", buttonText[sej2lcp[irData[iii].category1] & 0x000F]);
				else
					printf("%02x ", sej2lcp[irData[iii].category1]);

			if(irData[iii].kbData == 0x90)
				printf("  Make");
			else
				if(irData[iii].kbData == 0xd0)
					printf("  Break");
				else
					printf("  UNKNOWN!");

			if(sej2lcp[irData[iii].category1] == 0)
				printf("  (Sleep)");
				
			printf("\n");
		}
		else
		{
			printf("RM: %02x %02x %02x @ %08lx --> ", irData[iii].category1, irData[iii].category2, irData[iii].button, irData[iii].time);
			printf("\n");
		
		}
	
			
	}
}


void RebootBox(ulong argc, char* argv[])
{
	Reboot(kColdBoot);
}

#undef sp

void DumpCrashLog(ulong argc, char* argv[])
{
CrashLogStruct *cls = (CrashLogStruct *)kCrashLogBase;
uchar tx[17];
ulong len;
ulong iii,jjj;
ulong werd,werdcnt,sp;
uchar c;

	if(cls->crashSig == kValidCrashLogSig)
	{
		printf("Crash Log Version = %d, ROM Version = %08lx, SPOT Version = %d\n",cls->crashVersion,cls->romVersion,cls->spotVersion);
		printf("PhysicalRAM = %08lx, CPU Speed = %d, Bus Speed = %d\n", cls->physicalRAM, cls->cpuSpeed, cls->busSpeed);
		printf("\n");
		
		printf("Sys Config = %08lx:\n",cls->sysConfig);
		DecodeSysConfig(cls->sysConfig);
		printf("\n");

		printf("Int Enable = %08lx, Int Status = %08lx\n", cls->intEnable, cls->intStatus);
		printf("Err Enable = %08lx, Err Status = %08lx, Err Addr = %08lx\n", cls->errEnable, cls->errStatus, cls->errAddress);
		printf("\n");
		
		printf("v0 = %08lx    a0 = %08lx    ra = %08lx\n",	cls->cpuRegs[R_V0], cls->cpuRegs[R_A0], cls->cpuRegs[R_RA]);
		printf("v1 = %08lx    a1 = %08lx    sp = %08lx\n",	cls->cpuRegs[R_V1], cls->cpuRegs[R_A1], cls->cpuRegs[R_SP]);
		printf("                 a2 = %08lx    fp = %08lx\n",	                   cls->cpuRegs[R_A2], cls->cpuRegs[R_FP]);
		printf("                 a3 = %08lx   epc = %08lx\n",	                   cls->cpuRegs[R_A3], cls->cpuRegs[R_EPC]);
	
		printf("\n");
		printf("t0 = %08lx    s0 = %08lx    at = %08lx\n",	cls->cpuRegs[R_T0], cls->cpuRegs[R_S0], cls->cpuRegs[R_AT]);
		printf("t1 = %08lx    s1 = %08lx    gp = %08lx\n",	cls->cpuRegs[R_T1], cls->cpuRegs[R_S1], cls->cpuRegs[R_GP]);
		printf("t2 = %08lx    s2 = %08lx\n",	            cls->cpuRegs[R_T2], cls->cpuRegs[R_S2]);
		printf("t3 = %08lx    s3 = %08lx\n",	            cls->cpuRegs[R_T3], cls->cpuRegs[R_S3]);
		printf("t4 = %08lx    s4 = %08lx\n",	            cls->cpuRegs[R_T4], cls->cpuRegs[R_S4]);
		printf("t5 = %08lx    s5 = %08lx\n",	            cls->cpuRegs[R_T5], cls->cpuRegs[R_S5]);
		printf("t6 = %08lx    s6 = %08lx\n",	            cls->cpuRegs[R_T6], cls->cpuRegs[R_S6]);
		printf("t7 = %08lx    s7 = %08lx\n",	            cls->cpuRegs[R_T7], cls->cpuRegs[R_S7]);
		printf("t8 = %08lx\n",	                        cls->cpuRegs[R_T8]);
		printf("t9 = %08lx\n",	                        cls->cpuRegs[R_T9]);
		printf("\n");
		printf("      sr = %08lx\n",                     cls->cpuRegs[R_SR]);
		printf("   cause = %08lx\n",                     cls->cpuRegs[R_CAUSE]);
		printf("badvaddr = %08lx\n",                     cls->cpuRegs[R_BADVADDR]);
		printf("\n");	
	
		printf("   ibase = %08lx    ibound = %08lx\n", cls->cpuRegs[R_IBASE], cls->cpuRegs[R_IBOUND]);    
		printf("   dbase = %08lx    dbound = %08lx\n", cls->cpuRegs[R_DBASE], cls->cpuRegs[R_DBOUND]);    
		printf("  iwatch = %08lx    dwatch = %08lx\n", cls->cpuRegs[R_IWATCH], cls->cpuRegs[R_DWATCH]);
		printf("   count = %08lx   compare = %08lx\n", cls->cpuRegs[R_COUNT], cls->cpuRegs[R_COMPARE]);
		printf("    prid = %08lx    config = %08lx\n", cls->cpuRegs[R_PRID], cls->cpuRegs[R_CONFIG]);
		printf("    calg = %08lx       ecc = %08lx\n", cls->cpuRegs[R_CALG], cls->cpuRegs[R_ECC]);
		printf("cacheerr = %08lx     taglo = %08lx\n", cls->cpuRegs[R_CACHEERR], cls->cpuRegs[R_TAGLO]);
		printf("errorepc = %08lx\n", cls->cpuRegs[R_ERROREPC]);
		printf("\n\n");
	
		
		sp = cls->cpuRegs[R_SP];
		len = kSaveStackWords>>2;
		werdcnt = 0;
		tx[16] = 0;

		while (len--)
		{
			printf("%x: ", sp);

			for(iii=0;iii!=16;iii++)
			{			
				werd = cls->stackSave[werdcnt++];
				c = (uchar)(werd>>24);
				printf("%02x",c);
				if (isprint(c))
					tx[iii++]=c;
				else
					tx[iii++]='.';
				
				c = (uchar)(werd>>16);
				printf("%02x",c);
				if (isprint(c))
					tx[iii++]=c;
				else
					tx[iii++]='.';

				c = (uchar)(werd>>8);
				printf("%02x",c);
				if (isprint(c))
					tx[iii++]=c;
				else
					tx[iii++]='.';

				c = (uchar)werd;
				printf("%02x ",c);
				if (isprint(c))
					tx[iii]=c;
				else
					tx[iii]='.';				
			}	
			printf(" %s\n", tx);
			sp += 16;
		}
		
		printf("\n\n");
		
		jjj= cls->printfBufTail;
		printf("printf log:\n");
		for(iii=0;iii!=kPrintfBufSize;iii++)
		{
			if( (isprint(cls->printfBuf[jjj])) || (iscntrl(cls->printfBuf[jjj])) ) 
				putchar(cls->printfBuf[jjj]);
			jjj++;
			jjj &= kPrintfBufMask;
		}

		printf("\n\n");		
	}
	else
		printf("No valid Crash Log present!\n");
}


void DumpSysConfig(ulong argc, char* argv[])
{
	DecodeSysConfig(*(ulong *)kROMSysConfigReg);
}


void DecodeSysConfig(ulong syscfg)
{
	printf("CPU Clk Mult = ");
	switch(syscfg & kCPUMultBits)
	{
		case 0x00000000:
						printf("5x");
						break;
		case 0x00004000:
						printf("4x");
						break;
		case 0x00008000:
						printf("3x");
						break;
		case 0x0000c000:
						printf("2x");
						break;
	}
	
	printf(" Bus Clk, CPU output bufs @ ");
	
	if(syscfg & kCPUBuffBit)
		printf("50%%\n");
	else
		printf("83%%\n");

	/* ---- */

	printf("ROM Bank 0:  ");
	
	if(syscfg & kROMType0Bit)
		printf("Mask, ");
	else
		printf("Flash, ");

	if(syscfg & kROMMode0Bit)
		printf("PageMode, ");
	else
		printf("Normal, ");
	
	switch(syscfg & kROMSpeed0Bits)
	{
		case 0x00000000:
						printf("200ns/100ns\n");
						break;
		case 0x10000000:
						printf("100ns/50ns\n");
						break;
		case 0x20000000:
						printf("90ns/45ns\n");
						break;
		case 0x30000000:
						printf("120ns/60ns\n");
						break;
	}

	/* ---- */

	printf("ROM Bank 1:  ");
	
	if(syscfg & kROMType1Bit)
		printf("Mask, ");
	else
		printf("Flash, ");

	if(syscfg & kROMMode1Bit)
		printf("PageMode, ");
	else
		printf("Normal, ");
	
	switch(syscfg & kROMSpeed1Bits)
	{
		case 0x00000000:
						printf("200ns/100ns\n");
						break;
		case 0x01000000:
						printf("100ns/50ns\n");
						break;
		case 0x02000000:
						printf("90ns/45ns\n");
						break;
		case 0x03000000:
						printf("120ns/60ns\n");
						break;
	}

	/* ---- */

	printf("SGRAM:  ");

	switch(syscfg & kMemVendorBits)
	{
		case 0x00000000:
						printf("Other, ");
						break;
		case 0x00100000:
						printf("Samsung, ");
						break;
		case 0x00200000:
						printf("Fujitsu, ");
						break;
		case 0x00300000:
						printf("NEC, ");
						break;
	}
	
	switch(syscfg & kMemSpeedBits)
	{
		case 0x00000000:
						printf("100MHz\n");
						break;
		case 0x00400000:
						printf("66MHz\n");
						break;
		case 0x00800000:
						printf("77MHz\n");
						break;
		case 0x00c00000:
						printf("83MHz\n");
						break;
	}

	/* ---- */

	printf("Audio:  ");
	
	switch(syscfg & kAudDACTypeBits)
	{
		case 0x000c0000:
						printf("AKM4310/4309, ");
						break;
		default:
						printf("Unknown, ");
						break;
	}
	
	if(syscfg & kAudDACModeBit)
		printf("External Clk\n");
	else
		printf("SPOT Clk\n");
		
	/* ---- */

	printf("Video:  ");
	
	switch(syscfg & kVidEncTypeBits)
	{
		case 0x00000000:
						printf("Unknown, ");
						break;
		case 0x00000200:
						printf("Bt851, ");
						break;
		case 0x00000400:
						printf("Bt852, ");
						break;
		case 0x00000600:
						printf("Philips7187/Bt866, ");
						break;
	}
	
	if(syscfg & kNTSCBit)
		printf("NTSC, ");
	else
		printf("PAL, ");
		
	if(syscfg & kVidClkSrcBit)
		printf("SPOT Clk\n");
	else
		printf("External Clk\n");
	
	/* ---- */

	printf("Board:  ");	
		
	switch(syscfg & kBoardTypeBits)
	{
		case 0x00000008:
						printf("Trial, ");
						break;
		case 0x0000000c:
						printf("FCS, ");
						break;
		default:
						printf("Unknown Type, ");
						break;
	}
	
	printf("Rev = %d (%x)\n", (15-((syscfg & kBoardRevBits)>>4)), ((syscfg & kBoardRevBits)>>4));
		
}


void How(ulong argc, char* argv[])
{
	printf("%s\n",gHowString);
}

void Lookup(ulong addr)
{
	Symbol *sym;
	sym = FindSymbol(addr);
	if(sym)
		printf("%s + %04x\n",sym->symbol,addr - sym->address);
	else
		printf("Can't find %#x in symbol table\n",addr);
}

void WhichObj(ulong argc, char* argv[])
{
	ulong*  addr;

	addr = (ulong*)ParsePossibleReg(argv[1]);
	Lookup((ulong)(*addr));
}

void Where(ulong argc, char* argv[])
{
	ulong  addr;

	addr = ParsePossibleReg(argv[1]);
	Lookup(addr);
}

void WhereIs(ulong argc, char* argv[])
{
	Symbol *sym;

	if((sym = FindSymbolByName(argv[1])))
		printf("%s is at %#x\n",argv[1], sym->address);
	else
		printf("Can't find %s in symbol table\n",argv[1]);
}


void Step(ulong argc, char* argv[])
{
	ulong next;

	printf("Step (into)\n");
	next = (ulong)NextInstrAddr((ulong*)except_regs[R_EPC],0,0);
	except_regs[R_IWATCH] =  next | kIWatchEnable;
	ReturnFromException(0,0);
}

void Trace(ulong argc, char* argv[])
{
	ulong next;
	
	printf("Step (over)\n");
	next = (ulong)NextInstrAddr((ulong*)except_regs[R_EPC],1,0);
	except_regs[R_IWATCH] =  next | kIWatchEnable;
	ReturnFromException(0,0);
}

void StackCrawl(ulong argc, char* argv[])
{
	ulong 	*p;
	ulong	*base;
	ulong	*addr;
	ulong	sp;
	ulong	op;
	int		coolAddr = 0;
	Symbol *s	 = 0;
	
	/* start from base of stack */
	base = (ulong*)READ_AG(agSysStackBase);	
	
	if( ((ulong)base & 0xff000000) != 0x80000000 )
		base = (ulong *)0x80000000;						/* should be dynamic when we have RAM expansion */ 
	
	sp = (ulong)except_regs[R_SP];
	
	if( (sp & 0xffff0000) != 0x801f0000 )
		sp = 0x801f8000;								/* same, do 32KB */
	
	if((ulong)sp < (ulong)base)
	{
		OpenSymbolFile();
		
		p = base;
		printf("Crawling from %08lx to %08lx\n",base,sp);
		printf("Stack Addr  Caller\n");
		while((ulong)p > (ulong)sp)
		{
			if((*p & 0x9fc00003) == 0x9fc00000)
			{
				/* 
					it looks like a ROM address, check if it's a candidate
					for being a return address.
					we'll look two instructions back, and if it's JAL then
					we'll assume it's what we'ere looking at is a return
					address. 
				 */
				addr = (ulong*)*p;
				addr -= 2;
				op = (*addr & 0xfc000000)  >> 26;
				switch(op)
				{
					case 0x0:
						/* check if JALR */
						op = *addr & 0x0000003f;
						if(op == 0x09)
							coolAddr = 1;
					case 0x1:
						/* check if BLTZAL or BGEZAL */
						op = (*addr & 0x001f0000) >> 16;
						if( (op == 0x10) || (op == 0x11) )
							coolAddr = 1;
					case 0x3: 
						/* JAL */
						coolAddr = 1;
						break;
					default:
						coolAddr = 0;
						break;
				}
				if(coolAddr)
				{
					printf("%08lx    %08lx   ",p,*p);
					if(gSymbols)
					{
						s = FindSymbol((ulong)*p);
						printf("<%s+%x>",s->symbol,(ulong)*p - s->address);
					}
					printf("\n");
				}
			}
			p--;
		}
	}
	else
	{
		printf("something is wacky: sp (%#08x) > agSysStackBase (%#08x)\n",sp,base); 
	}
}



/* 	--------------------------
	Memory Verifiers
*/

void VerifyMem(ulong argc, char* argv[])
{
	ulong*	testStart;
	ulong*	testEnd;
	ulong pattern;
	ulong*	iii;

	if (argc == 4)
	{
		testStart = (ulong*)ParsePossibleReg(argv[1]);
		testStart = (ulong*)((ulong)testStart & 0xfffffffc);
		testEnd = (ulong*)ParsePossibleReg(argv[2]);
		testEnd = (ulong*)((ulong)testEnd & 0xfffffffc);
		pattern = stonum(argv[3]);
		
		printf("Verifying...\n");
		
		for (iii=testStart;iii<=testEnd;iii++)		
		{
			if (*iii != pattern)
				printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
		}
	}
	else
		printf("usage: v <start WORD> <end WORD> <WORD pat>\n");
}


void VerifyByteMem(ulong argc, char* argv[])
{
	uchar*	testStart;
	uchar*	testEnd;
	ulong pattern;
	uchar*	iii;

	if (argc == 4)
	{
		testStart = (uchar*)ParsePossibleReg(argv[1]);
		testEnd = (uchar*)ParsePossibleReg(argv[2]);
		
		pattern = stonum(argv[3]);
		
		printf("Verifying...\n");
		
		iii = testStart;
	
		while (iii<=testEnd)		
		{
				if (*iii != (pattern>>24))
					printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
				iii++;
				
				if (iii<=testEnd)
				{
					if (*iii != (pattern>>16))
						printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
					iii++;
				}
				
				if (iii<=testEnd)
				{
					if (*iii != (pattern>>8))
						printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
					iii++;
				}
				
				if (iii<=testEnd)
				{
					if (*iii != pattern)
						printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
					iii++;
				}
		}
	}
	else
		printf("usage: vb <start BYTE> <end BYTE> <WORD pat>\n");
}



void VerifyMemCount(ulong argc, char* argv[])
{
	ulong*	testStart;
	ulong testCount;
	ulong pattern;
	ulong*	iii;
	ulong*	end;

	if (argc == 4)
	{
		testStart = (ulong*)ParsePossibleReg(argv[1]);
		testStart = (ulong*)((ulong)testStart & 0xfffffffc);
		
		testCount = ParsePossibleReg(argv[2]);

		pattern = stonum(argv[3]);
		
		end = testStart + testCount;
		
		printf("Verifying...\n");
		
		for (iii=testStart;iii!=end;iii++)		
		{
			if (*iii != pattern)
				printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
		}
	}
	else
		printf("usage: vc <start WORD> <WORD cnt> <WORD pat>\n");
}



void VerifyByteMemCount(ulong argc, char* argv[])
{
	uchar*	testStart;
	uchar testCount;
	ulong pattern;
	uchar*	iii;
	uchar*	end;

	if (argc == 4)
	{
		testStart = (uchar*)ParsePossibleReg(argv[1]);
		
		testCount = ParsePossibleReg(argv[2]);

		pattern = stonum(argv[3]);
		
		end = testStart + testCount;
		
		printf("Verifying...\n");
						
		iii = testStart;
						
		while (iii<end)		
		{
				if (*iii != (pattern>>24))
					printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
				iii++;

				if (iii!=end)
				{
					if (*iii != (pattern>>16))
						printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
					iii++;
				}
				
				if (iii!=end)
				{
					if (*iii != (pattern>>8))
						printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
					iii++;
				}
				
				if (iii!=end)
				{
					if (*iii != pattern)
						printf("### Verify: 0x%x: 0x%x\n", iii, *iii);
					iii++;
				}
		}
	}
	else
		printf("usage: vbc <start BYTE> <BYTE cnt> <WORD pat>\n");
}


void CompareRange(ulong argc, char* argv[])
{
	ulong*	range1;
	ulong*	range2;
	ulong	cnt;

	if (argc == 4)
	{
		range1 = (ulong*)ParsePossibleReg(argv[1]);
		range2 = (ulong*)ParsePossibleReg(argv[2]);
		cnt = ParsePossibleReg(argv[3]);
		
		cnt >>= 2;		/* bytes -> words */
		
		while (cnt--)
		{
			if ( *range1 != *range2 )
				printf("### Compare: 0x%x: 0x%x != 0x%x\n", range1, *range1, *range2);
				
			range1++; range2++;
		}
		
	}
	else
		printf("usage: cr <range1> <range2> <bytecnt>\n");
		
}


/*	--------------------------------------------------------------------------------------------
	Command History 
*/

#define	kHistoryDepth		8
#define	kHistoryMask		7

char	history[kHistoryDepth][66] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ulong	historyBase = 0;

void DumpHistory(ulong argc, char* argv[])
{
	ulong	iii, index = historyBase;
	
	for(iii=kHistoryDepth;iii!=0;iii--)
	{
		index++;
		index &= kHistoryMask;
		
		printf("%d: %s\n", iii-1, history[index]);
	}
}

void DoHistory(ulong num)
{	
	num &= kHistoryMask;
	num = kHistoryDepth - num;
	num += historyBase;
	num &= kHistoryMask;
	
	if (history[num][0] != 0)
		ParseCommandLine(history[num]);
}

void AddToHistory(char* s)
{
	historyBase++;
	historyBase &= kHistoryMask;
	
	strcpy(history[historyBase], s);
}




void LastURL(ulong argc, char* argv[])
{
	printf("%s\n",gLastRequestedURL);
}

void DisplayVariable(ulong argc, char* argv[])
{
	if(argc != 2)
		printf("Usage: dv <variable_name>\n");
	else
		BoxPrintDebugGlobal(nil,argv[1]);
}

void SetCaches(ulong argc, char* argv[])
{
	ulong calg = 0;
	ulong ocalg = 0;
	
	if (argc == 2)
	{
		ocalg = FetchCalg();
		
		if(strcmp("off",argv[1]) == 0)
			calg = 0x22222222;
		else if(strcmp("wthru",argv[1]) == 0)
			calg = 0x22211111;
		else if(strcmp("wback",argv[1]) == 0)
			calg = 0x22233333;
		else
			printf("usage: cache <off | wthru | wback>\n");

		if( ( calg ) && ( calg != ocalg ) )
		{
			FlushDataCache();
			printf("setting C0_CALG to %x\n", calg);
			SetCalg(calg);
		}
	}
	else
	{
		printf("usage: cache <off | wthru | wback>\n");
	}
}

void ReadIIC(ulong argc, char* argv[])
{
	uchar byte = 0xff;
	ulong comp = 0;
	ulong addr = 0;
	
	comp = ParsePossibleReg(argv[1]);
	addr = ParsePossibleReg(argv[2]);
	byte = IICRead((uchar)comp,(uchar)addr);
	printf("ReadIIC : read %x\n",byte);
}

void WriteIIC(ulong argc, char* argv[])
{
	ulong comp = 0;
	ulong addr = 0;
	ulong value = 0;
	
	comp = ParsePossibleReg(argv[1]);
	addr = ParsePossibleReg(argv[2]);
	value = ParsePossibleReg(argv[3]);
	printf("WriteIIC : writing  comp = %x,addr = %x,val = %x\n",comp,addr,value);
	IICWrite((uchar)comp,(uchar)addr,(uchar)value);
}

extern void Init7187(void);

void DoInit7187(ulong argc, char* argv[])
{
	Init7187();
}

#endif
#endif




/*	############################################################################################
	DEBUG/GOOBER ONLY 
*/
 

#ifdef DEBUG

#if !defined(BOOTROM) && !defined(APPROM)
 

void TestIR(ulong argc, char* argv[])
{
	printf("IR Testing Start.\n");
	printf("hit any key to end.\n");

	*(ulong*)kBusIntEnableSetReg = kIRIntMask;

	while (!getchar())
	{
		ulong	data = PollIR();
		
		if (data)				
			printf("IR: %x\n", *(ulong*)kIRDataReg);
	}
}



void PokeHerb(ulong argc, char* argv[])
{
ulong data1, data2, data3;

	if (argc == 4)
	{
		data1 = (ulong)stonum(argv[1]);
		data2 = (ulong)stonum(argv[2]);
		data3 = (ulong)stonum(argv[3]);
		
		*(ulong*)(kSPOTBase + kMemCntl) 	= data1;
		*(ulong*)(kSPOTBase + kMemTiming) 	= data2;
		*(ulong*)(kSPOTBase + kMemCmd) 	= data3;
	}
	else
		printf("usage: herb <MemCntl> <MemTiming> <MemCmd>\n");

}


#define	SMART_CARD_ACK	1
#define DEBOUNCE 1
#define kStrobes 2

ulong gChecksum = 0;

ulong ParallelGetByte(void)
{
	ulong	result, smcreg;
	int n;
	
	/* wait for strobe to be asserted */
#ifdef DEBOUNCE
	/* this hack is to ensure we don't get any fake strobes */
	n = 0;
	while(n < kStrobes)
	{
		if((*(volatile ulong*)kGooberParallelCntrlIn & kStrobeBit) == 0)
			n = 0;
		else
			n++;
	}
#else
	while ((*(volatile ulong*)kGooberParallelCntrlIn & kStrobeBit) == 0)
		;
#endif		

	/* get the data */
	result = *(volatile ulong*)kGooberParallelIn;
	
	/* ACK the Mac */
#ifdef SMART_CARD_ACK
	smcreg = *(volatile ulong*)kDevSmartCardReg;
	smcreg |= kSmartCardReset;
	*(volatile ulong*)kDevSmartCardReg = smcreg;			/* Ack */
#else
	*(volatile ulong*)kGooberParallelCntrlOut = kAckBit;	/* Ack, dir = INPUT */
#endif

#ifdef DEBOUNCE
	/* this hack is to ensure we don't get any fake strobes */
	n = 0;
	while(n < kStrobes)
	{
		if((*(volatile ulong*)kGooberParallelCntrlIn & kStrobeBit) == kStrobeBit)
			n = 0;
		else
			n++;
	}
#else
	while ( (*(volatile ulong*)kGooberParallelCntrlIn & kStrobeBit) == kStrobeBit)
		;
#endif		

	/* de-ACK the Mac */
#ifdef SMART_CARD_ACK
	smcreg = *(volatile ulong*)kDevSmartCardReg;
	smcreg &= ~kSmartCardReset;
	*(volatile ulong*)kDevSmartCardReg = smcreg;			/* NAck */
#else
	*(volatile ulong*)kGooberParallelCntrlOut = 0; 			/* NAck, dir = INPUT */
#endif
	
	gChecksum += (result & 0xff);
	return result & 0xff;
}


ulong ParallelGet4Bytes(void)
{
	ulong	result;

	result = ParallelGetByte();
	result = ((result << 8) | ParallelGetByte());
	result = ((result << 8) | ParallelGetByte());
	result = ((result << 8) | ParallelGetByte());

	return result;
}


void DownloadFlash(ulong argc, char* argv[])
{
	ulong cmd;
	ulong cnt, addr;
	ulong data;
	ulong checksum = 0;
	ulong i;
	ulong n;
	ulong oldROMTiming;
	ulong len;
	ulong	err;
	
	oldROMTiming = *(volatile ulong*)(kSPOTBase+kROMCntl1);
	
	*(volatile ulong*)(kSPOTBase+kROMCntl1) = kROMCntl1_Init;
	
	*(volatile ulong*)kGooberParallelCntrlOut = 0; 			/* NAck, dir = INPUT */

	do
	{
		printf("Awaiting Command.\n");
		cmd = ParallelGetByte();
		printf("Got Command = %x\n", cmd);
	}
	while (cmd != 0);

	n = cnt = ParallelGet4Bytes();
	printf("count = 0x%x, ", cnt);
	addr = ParallelGet4Bytes();
	printf("addr = %#x\n", addr);
	
	/* erase just enough flash */

	addr -= 0x400000;
	len = (cnt+0x1ffff) & ~0x1ffff;	/* round up to next 128k chunk */
	printf("Erasing FLASH from %x to %x (%x bytes)...\n", addr, addr+len, len);
	err = EraseBank((ulong*)addr, len);
	if(err == 0)
		printf("Flash erased\n");
	else
		printf("Error erasing flash = %x\n", err);

	printf("Downloading to FLASH...\n");

	SetupFlash((ulong*)addr);

	gChecksum = 0;

	while (cnt)
	{
		data = ParallelGet4Bytes();
		FlashLong(data);
		cnt -= 4;
	}

	printf("download checksum = 0x%x\n", gChecksum);

	FinishFlash();

	for(i=0;i<n;i++)
	{
		checksum += *((uchar*)addr)++;
	}

	printf("flash checksum =  0x%x\n", checksum);
	
	if(gChecksum != checksum)
		printf("\n#### DOWNLOAD ERROR - checksums did not match!!! ####\n");

	printf("Flash complete.\n");
	
	*(volatile ulong*)(kSPOTBase+kROMCntl1) = oldROMTiming;
}



void EraseFlash(ulong argc, char* argv[])
{
	ulong	err;
	ulong	start = 0xbf800000;
	ulong	len = 0x400000;
	ulong	end;

	if (argc == 2)
	{
		len = (ulong)stonum(argv[1]);
	}
	else if (argc == 3)
	{
		start = (ulong)stonum(argv[1]);
		len = (ulong)stonum(argv[2]);
	}

	end = start + len;

	printf("Erasing FLASH from %x to %x (%x bytes)...\n", start, end, len);
	err = EraseBank((ulong*)start, len);
	printf("result = %x\n", err);
}


void TestFlash(ulong argc, char* argv[])
{
	ulong	err;
	
	printf("Copying Goober ROM to Flash...\n");
	err = CopyToFlash((ulong*)0xbfc00000, (ulong*)0xbf800000, 0x400000);
	printf("result = %x\n", err);
}

void HTest(ulong argc, char* argv[])
{
ulong iii;

	printf("Filling TV screen with H.  Hit any key when done.\n");

	for (iii=0;iii!=(70*26);iii++)
		TV_putchar('H');
	
	while (!getchar());
}

void EnableOverscan(ulong argc, char* argv[])
{
ulong enable;

	if (argc == 2)
	{
		enable = ParsePossibleReg(argv[1]);
		if (enable)
			SetDisplayOptions(GetDisplayOptions() | kBlankColorEnable);
		else
			SetDisplayOptions(GetDisplayOptions() & ~kBlankColorEnable);
	}
	else
		printf("usage: overscan <1/0>\n");
}


void SetOverscanColor(ulong argc, char* argv[])
{
ulong color;

	if (argc == 2)
	{
		color = ParsePossibleReg(argv[1]);
		SetDisplayOverscanColor(color);
	}
	else
		printf("usage: overcolor <color>\n");
}

void SetScreenPage(ulong argc, char* argv[])
{
ulong page = 0;

	if (argc == 2)
		page = ParsePossibleReg(argv[1]);

	page &= 1;
	
	DisplayPage(page);
}

void RunBenchmarks(ulong argc, char* argv[])
{
	benchmarks();
}

extern Boolean gFillFreeMem;

void FillFreeMem(ulong argc, char* argv[])
{
	if(argc == 2)
	{
		gFillFreeMem = ( ParsePossibleReg(argv[1]) == 1 );
	}
	else
	{
		printf("usage: fillfree < 0 | 1 >\n");
	}
}


void TestLEDs(ulong argc, char* argv[])
{
	printf("Green\n");
	SetBoxLEDs(kBoxLEDPower);
	while(!getchar());
	
	printf("Yellow\n");
	SetBoxLEDs(kBoxLEDConnect);
	while(!getchar());

	printf("Red\n");
	SetBoxLEDs(kBoxLEDMessage);
	while(!getchar());

	printf("All Off\n");
	SetBoxLEDs(0);
}



typedef ushort 	UInt16;
typedef	short	Int16;
typedef	uchar	UInt8;
typedef	char	Int8;
typedef	ulong	UInt32;
typedef long	Int32;

typedef struct Point {
	ushort	h;
	ushort	v;
	} Point;


UInt8 lastcolor = 0xce;
Point lastpoint = { 0, 0 };

void BLine(Point start, Point end, UInt8 pixVal);

/* line x1 y1 [x2 y2] [color] */

void PaintDebugLine(ulong argc, char* argv[])
{
	Point start, end;

	if (argc == 6)
	{
		start.h = (ushort)ParsePossibleReg(argv[1]);
		start.v = (ushort)ParsePossibleReg(argv[2]);
		
		end.h = (ushort)ParsePossibleReg(argv[3]);
		end.v = (ushort)ParsePossibleReg(argv[4]);
			
		lastcolor = (UInt8)ParsePossibleReg(argv[5]);
	}
	else
		if (argc == 3)
		{
			end.h = (ushort)ParsePossibleReg(argv[1]);
			end.v = (ushort)ParsePossibleReg(argv[2]);
		
			start.h = lastpoint.h;
			start.v = lastpoint.v;
		}
		else
		{
			printf("usage: line startx starty endx enxy color\n");
			printf("usage: line endx enxy (starts at last end)\n");
			return;
		}


	lastpoint.h = end.h;
	lastpoint.v = end.v;
		
	printf("Drawing...\n");
		
	BLine(start, end, lastcolor);
}

/* this sucks mightily, but it's just for those hardware guys. */

#define		PutPixel(x, y, pixVal) { \
				*(UInt8 *)(GetDisplayPageBase(0) + (x<<1) + (y * GetDisplayRowBytes())) = pixVal; \
				}
				
Int16 abs(Int16 x) {
  if (x < 0) 
  	return(-x);
  	else
  		return(x);
}

/* ----------------------------------------------------
 * Bresenham
 */
void BLine(Point start, Point end, UInt8 pixVal) {
 Int16		dx, dy;
 Int16		incrE, incrNE;
 Int16		d, x, y;
 Int16		temp;
 
 /* decide which case we are */
 
 /* ========================================================================================
  * Horizontal
  */
 if (end.v == start.v) {								/* horizontal? */
 	y = end.v;
 	if (start.h > end.h) {								/* need to go backward? */
 		for (x = end.h; x <= start.h; x++)
 			{
 			PutPixel(x, y, pixVal);
 			}
 		}
 		else {											/* draw 'em forward (left to right) */
 			 for (x = start.h; x <= end.h; x++)
 				{
 				PutPixel(x, y, pixVal);
 				}
 			 }
 	}
 /* ========================================================================================
  * Vertical
  */
 	else if (end.h == start.h) {						/* vertical? */
 		x = end.h;
 		if (start.v > end.v) {							/* need to go backward? */
 			for (y = end.v; y <= start.v; y++)
 				{
 				PutPixel(x, y, pixVal);
 				}
 			}
 			else {										/* draw 'em forward (top to bottom) */
 				for (y = start.v; y <= end.v; y++)
 					{
 					PutPixel(x, y, pixVal);
 					}
 				 }
 		}
 		
 		else											/* see if end.v > start.v */
 			{
 			
 /* ========================================================================================
  * Is end.v < start.v?  If so, swap to reduce # possibilities to 2.
  */

 if ( start.v > end.v ) {
 	temp = start.h;
 	start.h = end.h;
 	end.h = temp;
 	
 	temp = start.v;
 	start.v = end.v;
 	end.v = temp;
 	}

 /* ========================================================================================
  * Possibility #1: Positive Slope
  */
 if (end.h > start.h) {						/* x1 > x0, y1 > y0 */	
	dx = abs(end.h - start.h);
   	dy = abs(end.v - start.v);
   	x = start.h;
   	y = start.v;
 
 	PutPixel(x, y, pixVal);
 
 	if (dx > dy) {							/* x-dominant */
   		d = (dy << 1) - dx;					/* initial d */
   		incrE = dy << 1;					/* incr used to move to E */
   		incrNE = (dy - dx) << 1;			/* incr used to move to NE */
   		while (x < end.h) {
     		if (d <= 0) {
       		d = d + incrE;
       		x++; 
     		}
     		else {
   		  		d = d + incrNE;
   		  		x++;
   		  		y++;
          		}
   
 			PutPixel(x, y, pixVal);
   			}
   		}
   		else {								/* y-dominant */
   				d = -dy + (dx << 1);		/* initial d */
   				incrE = dx << 1;			/* incr used to move to E */
   				incrNE = (-dy + dx) << 1;	/* incr used to move to NE */
   				while (y < end.v) {
     				if (d <= 0) {
       					d = d + incrE;
       					y++; 
    					}
    					else {
     						d = d + incrNE;
     			 			x++;
     			 			y++;
       			 			}
  
 					PutPixel(x, y, pixVal);
   					}
   				}
   	}
   	
   	else												

 /* ========================================================================================
  * Possibility 2: Negative Slope.
  */
 		{													/* x1 < x0, y1 > y0 */	
		dx = abs(end.h - start.h);
   		dy = abs(end.v - start.v);
   		x = start.h;
   		y = start.v;
 
 		PutPixel(x, y, pixVal);
 
 		if (dx > dy) {							/* x-dominant */
   			d = (dy << 1) - dx;					/* initial d */
   			incrE = dy << 1;					/* incr used to move to E */
   			incrNE = (dy - dx) << 1;			/* incr used to move to NE */
   			while (x > end.h) {
     			if (d <= 0) {
       				d = d + incrE;
       				x--; 
    				}
    				else {
     				 	d = d + incrNE;
     				 	x--;
     				 	y++;
       				 	}
   
 				PutPixel(x, y, pixVal);
   				}
   			}
   		else {								/* y-dominant */
   				d = -dy + (dx << 1);		/* initial d */
   				incrE = dx << 1;			/* incr used to move to E */
   				incrNE = (-dy + dx) << 1;	/* incr used to move to NE */
   				while (y < end.v) {
     				if (d <= 0) {
       					d = d + incrE;
       					y++; 
    					}
     					else {
    		  				d = d + incrNE;
   		  					x--;
   		  					y++;
          					}
   
 					PutPixel(x, y, pixVal);
   					}
   				}
   		}
   		
   	} 	/* else */

}

/* FIXME - nuke once goober rom is trimmed down */
void PaintPicture(ulong argc, char *argv[])
{
ulong *pic;
ulong *fbEven;
ulong *fbOdd;
ulong *pix;
ulong x,y;
ulong rowWords;
ulong xSize,ySize;
ulong xStart,yStart;

	xStart = yStart = 0;

	pic = (ulong *)Resolve("/ROM/lum.yuv", false)->data;
	fbOdd = (ulong *)GetDisplayPageBase(0);
	fbEven = (ulong *)GetDisplayPageBase(2);
	
	rowWords = (GetDisplayRowBytes() >> 2);
	
	printf("image @ %x\n",pic);
	printf("ODD Field @ %x\n",fbOdd);
	printf("EVEN Field @ %x\n",fbEven);
	
	xSize = *pic;
	pic++;
	ySize = *pic;
	pic++;
	
	for(y=0;y!=(ySize>>1);y++)
	{
		pix = fbEven;
		
		for(x=0;x!=(xSize>>1);x++)
		{
			*pix = *pic;
			pix++;
			pic++;
		}

		fbEven += rowWords;

		pix = fbOdd;
		
		for(x=0;x!=(xSize>>1);x++)
		{
			*pix = *pic;
			pix++;
			pic++;
		}
		
		fbOdd += rowWords;
	}
}

void PaintPictureHWInterlaced(ulong argc, char *argv[])
{
ulong *pic;
ulong *fb;
ulong *pix;
ulong x,y;
ulong rowWords;
ulong xSize,ySize;
ulong xStart,yStart;
FSNode *fsn;
	
	xStart = yStart = 0;

	fsn = Resolve("/ROM/lum.yuv", false);
	if(!fsn) {
		printf("Can't find /ROM/lum.yuv\n");
		return;
	}
	pic = (ulong *)fsn->data;
	fb = (ulong *)GetDisplayPageBase(0);
	
	rowWords = (GetDisplayRowBytes() >> 2);
	
	printf("image @ %x\n",pic);
	printf("fb @ %x\n",fb);
	
	xSize = *pic;
	pic++;
	ySize = *pic;
	pic++;
	
	for(y=0;y<ySize;y++)
	{
		pix = fb;
		for(x=0;x!=(xSize>>1);x++)
		{
			*pix = *pic;
			pix++;
			pic++;
		}
		fb += rowWords;
	}
	
	if(SPOTVersion() != kSPOT3) {
		printf("HW interlacing only works on SPOT3\n");
		return;
	}

	printf("Enabling HW interlacing...\n");
	EnableInterlacedDisplay();
}


#endif /* BOOTROM */

#endif /* DEBUG */


