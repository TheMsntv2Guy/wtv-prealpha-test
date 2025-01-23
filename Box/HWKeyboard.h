

#ifndef __HW_KEYBOARD__
#define __HW_KEYBOARD__

#ifdef __cplusplus
extern "C" {
#endif

/* User stuff */

/* flags */

#define	kHWKeyboardPresent		0x00000001

#define	kShiftDown				0x00000010
#define	kControlDown			0x00000020
#define	kAltDown				0x00000040
#define	kShiftLock				0x00000080


/* Other useful constants */

#define	kNumLED					0x02
#define	kCapLED					0x04
#define	kScrLED					0x01


void 	InitHWKeyboard(void);
void 	HWKeyboardIdle(void);

uchar 	GetHWKBModifiers(void);

void 	SetHWKBLEDs(uchar newleds);
uchar	GetHWKBLEDs(void);

uchar 	PollHWKeyboard(void);		/* just for debugger */


/* Driver stuff */

/* 	Keyboard Controller Commands
	Written to kKybdControlReg
 */

#define	kReadCommandByte			0x20		/* read controller command */
#define	kWriteCommandByte			0x60		/* write controller command */
#define	kTestController				0xaa		/* test controller */
#define	kDisableKeyboard			0xad		/* disable the keyboard */
#define	kEnableKeyboard				0xae		/* enable the keyboard */
#define	kAssertA20					0xdf		/* assert the A20 output line */
#define	kDeassertA20				0xdd		/* deassert the A20 output line */


/* 	Keyboard Status Bits
	Read from kKybdControlReg
 */
 
#define	kKStat_ParityError			0x80		/* kybd serial parity err */
#define kKStat_RecTimeout			0x40		/* timeout during kybd message */
#define kKStat_TransTimeout			0x20		/* timeout after control message */
#define	kKStat_NoKeyLock			0x10		/* 0 = keylock switch ON */
#define	kKStat_CmdReceived			0x08		/* 1 = last write to kKybdControlReg */
#define	kKStat_SysFlag				0x04		/* keyboard OK */
#define	kKStat_InputFull			0x02		/* 1 = full */
#define kKStat_OutputFull			0x01		/* 1 = read data available */


/*	Keyboard Command Bits
	Written to kKybdControlReg after writing kCCmd_Write to kKybdControlReg
 */

#define	kBCmd_Reserved				0x80		/* reserved, must be 0 */
#define kBCmd_Translate				0x40		/* translate AT -> PC */
#define kBCmd_PCMode				0x20		/* use PC serial interface */
#define	kBCmd_DisableKybd			0x10		/* force kybd clk low */
#define kBCmd_DisableInhibit		0x08		/* override keylock */
#define	kBCmd_SysFlag				0x04		/* 1 after BIOS setup */
#define	kBCmd_Reserved1				0x02		/* reserved, must be 0 */
#define	kBCmd_EnableInterrupt		0x01		/* enable interrupt */




#ifdef __cplusplus
}
#endif


#endif

