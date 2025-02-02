
/* This should eventually be a file in the ROM filesystem */


#define	kCvtTableSize	0x40

#define kEnter			0x1c
#define	kUpArr			0x48
#define	kDownArr		0x50
#define	kRightArr		0x4d
#define	kLeftArr		0x4b
#define	kHome			0x47
#define	kEnd			0x4f
#define	kPageUp			0x49
#define	kPageDown		0x51
#define kInsert			0x52
#define kDelete			0x53

#define	kLeftShift		0x2a
#define	kRightShift		0x36
#define	kCapsLock		0x3a
#define	kControl		0x1d	/* $1d is left ctl, $e0 $1d is right ctl */


const uchar sc2lc[kCvtTableSize] =	{	
					0x00,		/* 00 */
					0x1b,		/* 01 */		/* ESC */
					'1',		/* 02 */
					'2',		/* 03 */
					'3',		/* 04 */
					'4',		/* 05 */
					'5',		/* 06 */
					'6',		/* 07 */
					'7',		/* 08 */
					'8',		/* 09 */
					'9',		/* 0a */
					'0',		/* 0b */
					'-',		/* 0c */
					'=',		/* 0d */
					'\b',		/* 0e */		/* BS */
					'\t',		/* 0f */		/* Tab */

					'q',		/* 10 */
					'w',		/* 11 */
					'e',		/* 12 */
					'r',		/* 13 */
					't',		/* 14 */
					'y',		/* 15 */
					'u',		/* 16 */
					'i',		/* 17 */
					'o',		/* 18 */
					'p',		/* 19 */
					'[',		/* 1a */
					']',		/* 1b */
					'\r',		/* 1c */		/* CR */
					0x00,		/* 1d */
					'a',		/* 1e */
					's',		/* 1f */

					'd',		/* 20 */
					'f',		/* 21 */
					'g',		/* 22 */
					'h',		/* 23 */
					'j',		/* 24 */
					'k',		/* 25 */
					'l',		/* 26 */
					';',		/* 27 */
					0x27,		/* 28 */		/* ' */
					'`',		/* 29 */
					0x00,		/* 2a */
					0x5c,		/* 2b */		/* \ */
					'z',		/* 2c */
					'x',		/* 2d */
					'c',		/* 2e */
					'v',		/* 2f */

					'b',		/* 30 */
					'n',		/* 31 */
					'm',		/* 32 */
					',',		/* 33 */
					'.',		/* 34 */
					'/',		/* 35 */
					0x00,		/* 36 */
					0x00,		/* 37 */
					0x00,		/* 38 */
					' '			/* 39 */
				};
				
				
const uchar sc2uc[kCvtTableSize] =	{	
					0x00,		/* 00 */
					0x1b,		/* 01 */		/* ESC */
					'!',		/* 02 */
					'@',		/* 03 */
					'#',		/* 04 */
					'$',		/* 05 */
					'%',		/* 06 */
					'^',		/* 07 */
					'&',		/* 08 */
					'*',		/* 09 */
					'(',		/* 0a */
					')',		/* 0b */
					'_',		/* 0c */
					'+',		/* 0d */
					'\b',		/* 0e */		/* BS */
					'\t',		/* 0f */		/* Tab */

					'Q',		/* 10 */
					'W',		/* 11 */
					'E',		/* 12 */
					'R',		/* 13 */
					'T',		/* 14 */
					'Y',		/* 15 */
					'U',		/* 16 */
					'I',		/* 17 */
					'O',		/* 18 */
					'P',		/* 19 */
					'{',		/* 1a */
					'}',		/* 1b */
					'\r',		/* 1c */		/* CR */
					0x00,		/* 1d */
					'A',		/* 1e */
					'S',		/* 1f */

					'D',		/* 20 */
					'F',		/* 21 */
					'G',		/* 22 */
					'H',		/* 23 */
					'J',		/* 24 */
					'K',		/* 25 */
					'L',		/* 26 */
					':',		/* 27 */
					'"',		/* 28 */		
					'~',		/* 29 */
					0x00,		/* 2a */
					'|',		/* 2b */
					'Z',		/* 2c */
					'X',		/* 2d */
					'C',		/* 2e */
					'V',		/* 2f */

					'B',		/* 30 */
					'N',		/* 31 */
					'M',		/* 32 */
					'<',		/* 33 */
					'>',		/* 34 */
					'?',		/* 35 */
					0x00,		/* 36 */
					0x00,		/* 37 */
					0x00,		/* 38 */
					' '			/* 39 */
				};
				
