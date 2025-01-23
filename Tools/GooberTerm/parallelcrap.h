#define	kDefaultDownloadAddress	0x9fc00000

#define	kDefaultROMFileName	"TS1ROM"

#define swap(x) ( ( (x & 0x00FF0000) << 8 ) | ( (x & 0xFF000000) >> 8 ) | \
				((x & 0x000000FF) << 8 ) | ( (x & 0x0000FF00) >> 8 ) )

#define kCardSig	0x10950640

#define kReadMode 	1
#define kWriteMode 	2

#define kStrobeBit	(1)
#define kDIRBit		(1<<5)
#define kACKBit		(1<<6)
#define kBusyBit	(1<<7)

#define kDefaultControl	0xff

#define kRWPortReg		0x1f0
#define kReadStatReg	0x1f1
#define kRWControlReg	0x1f2

typedef unsigned char uchar;
typedef unsigned long ulong;

enum 	{ 
			kSquirtCmd,
			kSlurpCmd,
			kGoCmd,
			kDummyCmd,
			kMemTestCmd,
			kHelpCmd,
			kQuitCmd,
			kSquirtAndRunCmd,
			kNumCmds=kQuitCmd 
		};

int 	FindCard(void);
ulong 	ReadConfig( long bridge, long slot, long reg);
void 	WriteConfig( long bridge, long slot, long reg, ulong data);
uchar	ReadIOByte(long reg);
void 	WriteIOByte(long reg, unsigned char data);
void	SquirtByte(unsigned char x);
ulong Squirt4Bytes(ulong val);
uchar 	SlurpByte(void);
void 	SetMode(unsigned char mode);
int 	GetVal(char *s, unsigned char *val) ;
int 	DoIt(char *s);
void 	SquirtIt(char *s);
void 	SlurpIt(char *s);
void 	Go(char *s);
void 	Help(void);
void 	Strobe(char assert);
void	InitCard(void);
void 	WaitForACK(char assert);
void	Dummy(void);



