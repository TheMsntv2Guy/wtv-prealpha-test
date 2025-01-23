/* Sejin IR Keyboard */


#define	kIRKBLeftShift		0x4001			
#define	kIRKBRightShift		0x4002
#define	kIRKBLeftCmd		0x4003
#define	kIRKBRightCmd		0x4004
#define	kIRKBLeftControl	0x4005
#define	kIRKBRightControl	0x4006

#define	kIRKBCapsLock		0x4007

#define	kIRKBSpecialKeyFlag	0x4000		/* Special keys have upper nybble == 4 */


#define kIRKeyMapTableSize	128
	
const ushort sej2lc[kIRKeyMapTableSize] = {
				0x0000,			/* 00 */
				0x0000,			/* 01 */
				0x0000,			/* 02 */
				0x0000,			/* 03 */
				kIRKBLeftControl, /* 04 */
				0x0000,			/* 05 */
				0x0000,			/* 06 */
				0x0000,			/* 07 */
				kF5,			/* 08 */
				kIRKBCapsLock,	/* 09 */
				'x',			/* 0a */
				0x0000,			/* 0b */
				kF1,			/* 0c */
				'2',			/* 0d */
				's',			/* 0e */
				'w',			/* 0f */

				0x0000,			/* 10 */
				kF3,			/* 11 */
				'c',			/* 12 */
				kF4,			/* 13 */
				kF2,			/* 14 */
				'3',			/* 15 */
				'd',			/* 16 */
				'e',			/* 17 */
				0x0000,			/* 18 */
				'\t',			/* 19 */		/* TAB */
				'z',			/* 1a */
				0x001b,			/* 1b */		/* ESC */
				'`',			/* 1c */
				'1',			/* 1d */
				'a',			/* 1e */
				'q',			/* 1f */

				'b',			/* 20 */
				't',			/* 21 */
				'v',			/* 22 */
				'g',			/* 23 */
				'5',			/* 24 */
				'4',			/* 25 */
				'f',			/* 26 */
				'r',			/* 27 */
				kIRKBRightCmd,	/* 28 */
				0x0000,			/* 29 */
				0x0000,			/* 2a */
				kIRKBLeftCmd,	/* 2b */
				0x0000,			/* 2c */
				0x0000,			/* 2d */
				0x0000,			/* 2e */
				0x0000,			/* 2f */

				0x0000,			/* 30 */
				kIRKBLeftShift,	/* 31 */
				kIRKBRightShift,/* 32 */
				0x0000,			/* 33 */
				0x0000,			/* 34 */
				0x0000,			/* 35 */
				0x0000,			/* 36 */
				0x0000,			/* 37 */
				' ',			/* 38 */
				'\b',			/* 39 */		/* BS */
				'\r',			/* 3a */		/* CR */
				kF6,			/* 3b */
				kF9,			/* 3c */
				kF10,			/* 3d */
				0x5c,			/* 3e */		/* \ */
				0x0000,			/* 3f */

				0x0000,			/* 40 */
				0x0000,			/* 41 */
				0x0000,			/* 42 */
				0x0000,			/* 43 */
				0x0000,			/* 44 */
				0x0000,			/* 45 */
				0x0000,			/* 46 */
				0x0000,			/* 47 */
				kLeftKey,		/* 48 */
				0x0000,			/* 49 */
				kPowerKey,		/* 4a */
				kUpKey,			/* 4b */
				kHomeKey,		/* 4c */
				0x0000,			/* 4d */
				kBackKey,		/* 4e */
				0x0000,			/* 4f */

				kDownKey,		/* 50 */
				0x0000,			/* 51 */
				0x0000,			/* 52 */
				0x0000,			/* 53 */
				kOptionsKey,	/* 54 */
				kF11,			/* 55 */
				kScrollUpKey,	/* 56 */
				0x0000,			/* 57 */
				kRightKey,		/* 58 */
				0x0000,			/* 59 */
				0x0000,			/* 5a */
				kScrollDownKey,	/* 5b */
				kRecentKey,		/* 5c */
				kF12,			/* 5d */
				0x0000,			/* 5e */
				0x0000,			/* 5f */

				0x0000,			/* 60 */
				kF7,			/* 61 */
				'.',			/* 62 */
				0x0000,			/* 63 */
				kF8,			/* 64 */
				'9',			/* 65 */
				'l',			/* 66 */
				'o',			/* 67 */
				'/',			/* 68 */
				'[',			/* 69 */
				0x0000,			/* 6a */
				0x0027,			/* 6b */		/* ' */
				'-',			/* 6c */
				'0',			/* 6d */
				';',			/* 6e */
				'p',			/* 6f */

				0x0000,			/* 70 */
				']',			/* 71 */
				',',			/* 72 */
				0x0000,			/* 73 */
				'=',			/* 74 */
				'8',			/* 75 */
				'k',			/* 76 */
				'i',			/* 77 */
				'n',			/* 78 */
				'y',			/* 79 */
				'm',			/* 7a */
				'h',			/* 7b */
				'6',			/* 7c */
				'7',			/* 7d */
				'j',			/* 7e */
				'u',			/* 7f */

				};
				

const ushort sej2uc[kIRKeyMapTableSize] = {

				0x0000,			/* 00 */
				0x0000,			/* 01 */
				0x0000,			/* 02 */
				0x0000,			/* 03 */
				kIRKBLeftControl, /* 04 */
				0x0000,			/* 05 */
				0x0000,			/* 06 */
				0x0000,			/* 07 */
				kF5,			/* 08 */
				kIRKBCapsLock,	/* 09 */
				'X',			/* 0a */
				0x0000,			/* 0b */
				kF1,			/* 0c */
				'@',			/* 0d */
				'S',			/* 0e */
				'W',			/* 0f */

				0x0000,			/* 10 */
				kF3,			/* 11 */
				'C',			/* 12 */
				kF4,			/* 13 */
				kF2,			/* 14 */
				'#',			/* 15 */
				'D',			/* 16 */
				'E',			/* 17 */
				0x0000,			/* 18 */
				'\t',			/* 19 */		/* TAB */
				'Z',			/* 1a */
				0x001b,			/* 1b */		/* ESC */
				'~',			/* 1c */
				'!',			/* 1d */
				'A',			/* 1e */
				'Q',			/* 1f */

				'B',			/* 20 */
				'T',			/* 21 */
				'V',			/* 22 */
				'G',			/* 23 */
				'%',			/* 24 */
				'$',			/* 25 */
				'F',			/* 26 */
				'R',			/* 27 */
				kIRKBRightCmd,	/* 28 */
				0x0000,			/* 29 */
				0x0000,			/* 2a */
				kIRKBLeftCmd,	/* 2b */
				0x0000,			/* 2c */
				0x0000,			/* 2d */
				0x0000,			/* 2e */
				0x0000,			/* 2f */

				0x0000,			/* 30 */
				kIRKBLeftShift,	/* 31 */
				kIRKBRightShift,/* 32 */
				0x0000,			/* 33 */
				0x0000,			/* 34 */
				0x0000,			/* 35 */
				0x0000,			/* 36 */
				0x0000,			/* 37 */
				' ',			/* 38 */
				'\b',			/* 39 */		/* BS */
				'\r',			/* 3a */		/* CR */
				kF6,			/* 3b */
				kF9,			/* 3c */
				kF10,			/* 3d */
				'|',			/* 3e */
				0x0000,			/* 3f */

				0x0000,			/* 40 */
				0x0000,			/* 41 */
				0x0000,			/* 42 */
				0x0000,			/* 43 */
				0x0000,			/* 44 */
				0x0000,			/* 45 */
				0x0000,			/* 46 */
				0x0000,			/* 47 */
				kLeftKey,		/* 48 */
				0x0000,			/* 49 */
				kPowerKey,		/* 4a */
				kUpKey,			/* 4b */
				kHomeKey,		/* 4c */
				0x0000,			/* 4d */
				kBackKey,		/* 4e */
				0x0000,			/* 4f */

				kDownKey,		/* 50 */
				0x0000,			/* 51 */
				0x0000,			/* 52 */
				0x0000,			/* 53 */
				kOptionsKey,	/* 54 */
				kF11,			/* 55 */
				kScrollUpKey,	/* 56 */
				0x0000,			/* 57 */
				kRightKey,		/* 58 */
				0x0000,			/* 59 */
				0x0000,			/* 5a */
				kScrollDownKey,	/* 5b */
				kRecentKey,		/* 5c */
				kF12,			/* 5d */
				0x0000,			/* 5e */
				0x0000,			/* 5f */

				0x0000,			/* 60 */
				kF7,			/* 61 */
				'>',			/* 62 */
				0x0000,			/* 63 */
				kF8,			/* 64 */
				'(',			/* 65 */
				'L',			/* 66 */
				'O',			/* 67 */
				'?',			/* 68 */
				'{',			/* 69 */
				0x0000,			/* 6a */
				'"',			/* 6b */	
				'_',			/* 6c */
				')',			/* 6d */
				':',			/* 6e */
				'P',			/* 6f */

				0x0000,			/* 70 */
				'}',			/* 71 */
				'<',			/* 72 */
				0x0000,			/* 73 */
				'+',			/* 74 */
				'*',			/* 75 */
				'K',			/* 76 */
				'I',			/* 77 */
				'N',			/* 78 */
				'Y',			/* 79 */
				'M',			/* 7a */
				'H',			/* 7b */
				'^',			/* 7c */
				'&',			/* 7d */
				'J',			/* 7e */
				'U',			/* 7f */

				};
