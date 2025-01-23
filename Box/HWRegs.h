#ifndef __HWREGS_H__
#define __HWREGS_H__


/*	CONVENTION:	If the name ends in Reg, it's a complete address.
				Otherwise, it's just an offset off of kSPOTBase.
 */
 

#ifdef SPOT3_REFRESH_HACK
#define	kRefreshCompare				0xa4cb8
#endif


/************************** GOOBER **************************/

#define kGooberControl				0xbf7fff04
#define kGooberSerialRcv			0xbf7fff10
#define	kGooberSerialXmt			0xbf7fff14
#define	kGooberSerialData			kGooberSerialXmt
#define	kGooberSerialStatus			0xbf7fff18
#define	kGooberSerialControl		0xbf7fff1c
#define kGooberParallelIn			0xbf7fff20
#define kGooberParallelOut			0xbf7fff24
#define kGooberParallelCntrlIn		0xbf7fff28
#define kGooberParallelCntrlOut		0xbf7fff2c
#define	kGooberSwitches				0xbf7fff30
#define	kGooberLEDs					0xbf7fff34

/* Goober Parallel Port */

#define kStrobeBit		(1<<0)		/* STROBE - bit 0 of parallel control in  (0x28) */
#if 0
#define kAckBit			(1<<4)		/* FIXME ACK    - bit 4 of parallel control out (0x2c) */
#endif
#define kAckBit			(0x1f)		/* FIXME ACK    - bit 4 of parallel control out (0x2c) */
#define kDirBit			(1<<5)		/* DIR    - bit 5, 1 == output */

/* Goober Serial Port */

#define	kGooberTXHFull				0x00000004
#define	kGooberTXBusy				0x00000002

/* Goober Detection */

#if 1
#define	GOOBER_IS_PRESENT			( ((*(volatile ulong *)kGooberSerialStatus & 0xf0) == 0) && (*(volatile ulong *)kGooberSwitches & 0x40) )
#else
#define	GOOBER_IS_PRESENT			( (*(volatile ulong *)kROMSysConfigReg & 0x01) == 0 )
#endif

/* Goober DIPSwitches */

#define GOOBER_DIPSWITCH(n)			( GOOBER_IS_PRESENT && (*(volatile ulong *)kGooberSwitches & (1<<(n-1))) )

/* 
	Goober DIPSwitch allocation: 	1	
									2	OFF => Boot into pFORTH			ON => Boot into Monitor
									3	OFF => Use TV for output		ON => Use Parallel Port for output
									4	OFF => Boot into Downloader		ON => Boot to Switch2 setting
									5
									6	OFF => Use embedded home page
									7	SOFTWARE GOOBER DISABLE: OFF => Goober ENABLED
									8	HARDWARE GOOBER DISABLE: OFF => Goober ENABLED
*/
			
#define	kMonitorOrForthSwitch		2
#define	kDebugOutputSelectSwitch	3
#define	kBootIntoDownloaderSwitch	4
#define	kUseEmbeddedHomeSwitch		6


/*************************** MISC ***************************/

#define	kSPOTBase					0xa4000000
#define	kSPOTBaseHi					0xa400

#define	kDevLEDs					0x4004
#define	kDevLEDsReg					0xa4004004

#define	kPowerLED					0x0004
#define	kConnectedLED				0x0002
#define	kMessageLED					0x0001

#define	kDevIDChip					0x4008

#define	kDevSmartCard				0x4010
#define	kDevSmartCardReg			0xa4004010

#define	kSmartCardClk				0x00000020
#define	kSmartCardDataOE			0x00000010
#define	kSmartCardDataOut			0x00000008
#define	kSmartCardReset				0x00000002
#define	kSmartCardInsert			0x00000001

#define	kDevExtTime					0xa4004014

#define	kBusChipID					0x0000
#define kRevMask 					0x00F00000

#define	kBusChipCntl				0x0004

/* kBusChipCntl values */

#define	kAudClkDivMask				0xc3ffffff
#define	kExternalAudClk				0x00000000
#define	kAudClkDiv1					0x04000000
#define	kAudClkDiv2					0x08000000
#define	kAudClkDiv3					0x0c000000
#define	kAudClkDiv4					0x10000000
#define	kAudClkDiv5					0x14000000
#define	kAudClkDiv6					0x18000000

/************************** NVRRAM/EEPROM/IIC **************************/

#define	kDevNVCntl					0x400c
#define	kDevNVCntlReg				(kSPOTBase + kDevNVCntl)

#define kNVDataIn					0x00000001
#define kNVDataOut					0x00000002
#define kNVDataOutEn				0x00000004
#define kNVClock					0x00000008

#define kNVWrite					0x0
#define kNVRead						0x1

/************************** MEMORY **************************/

#define	kMemCntl					0x5000
#define	kMemRefCnt					0x5004
#define	kMemData					0x5008
#define	kMemCmd						0x500c
#define	kMemTiming					0x5010

#define	kMemCntlReg					(kSPOTBase + kMemCntl)
#define	kMemRefCntReg				(kSPOTBase + kMemRefCnt)
#define	kMemDataReg					(kSPOTBase + kMemData)
#define	kMemCmdReg					(kSPOTBase + kMemCmd)
#define	kMemTimingReg				(kSPOTBase + kMemTiming)

#define	kROMSysConfig				0x1000
#define	kROMSysConfigReg			(kSPOTBase + 0x1000)

#define	kROMType0Bit				0x80000000
#define	kROMMode0Bit				0x40000000
#define	kROMSpeed0Bits				0x30000000

#define	kROMType1Bit				0x08000000
#define	kROMMode1Bit				0x04000000
#define	kROMSpeed1Bits				0x03000000

#define	kMemSpeedBits				0x00c00000
#define	kMemVendorBits				0x00300000

#define	kAudDACTypeBits				0x000c0000
#define	kAudDACModeBit				0x00020000

#define	kVidClkSrcBit				0x00010000

#define	kCPUMultBits				0x0000c000
#define	kCPUBuffBit					0x00002000

#define	kNTSCBit					0x00000800
#define	kVidEncTypeBits				0x00000600

#define	kBoardRevBits				0x000000f0
#define	kBoardTypeBits				0x0000000c

#define	kGooberNotPresentBit		0x00000001


/* EPROMs are about 1/2 as fast as the FLASH. */

#ifdef GOOBER
#define	kROMCntl1_Init				0x0a0a0007		/* Bob's safe timing */
#else
#define	kROMCntl1_Init				0x0a0a0007		/* Bob's safe timing */
#endif

#define	kROMCntl0					0x1004
#define	kROMCntl1					0x1008

#define	kROMCntl0Reg				(kSPOTBase + kROMCntl0)
#define	kROMCntl1Reg				(kSPOTBase + kROMCntl1)

#define	kROMProtectMask				0x40000000		/* in each ROMCntl reg.  1 = PROTECTED */

/************************ INTERRUPTS ************************/

#define	kCPUIntMask					0x00000400
#define	kGooberIntMask				0x00004000

#define	kBusIntEnableSet			0x0000000c
#define	kBusIntEnableClear			0x0000010c
#define	kBusIntStatus				0x00000008
#define	kBusIntStatusClear			0x00000108

#define	kBusIntEnableSetReg			(kSPOTBase + kBusIntEnableSet)
#define	kBusIntEnableClearReg		(kSPOTBase + kBusIntEnableClear)
#define	kBusIntStatusReg			(kSPOTBase + kBusIntStatus)
#define	kBusIntStatusClearReg		(kSPOTBase + kBusIntStatusClear)

#define	kVideoIntMask				0x00000080
#define	kKeyboardIntMask			0x00000040
#define	kModemIntMask				0x00000020
#define	kIRIntMask					0x00000010
#define	kSmartCardIntMask			0x00000008
#define	kAudioIntMask				0x00000004

/******************** BUS ERRORS & FENCES *******************/

#define	kBusErrStatus				0x00000010
#define	kBusErrStatusClear			0x00000110
#define	kBusErrEnableSet			0x00000014
#define	kBusErrEnableClear			0x00000114

#define	kBusErrStatusReg			(kSPOTBase + kBusErrStatus)
#define	kBusErrStatusClearReg		(kSPOTBase + kBusErrStatusClear)
#define	kBusErrEnableSetReg			(kSPOTBase + kBusErrEnableSet)
#define	kBusErrEnableClearReg		(kSPOTBase + kBusErrEnableClear)

#define	kFence1ReadIntMask			0x00000040
#define	kFence1WriteIntMask			0x00000020
#define	kFence2ReadIntMask			0x00000010
#define	kFence2WriteIntMask			0x00000008
#define	kBusTimeoutIntMask			0x00000004

#define	kBusErrAddress				0x00000018

#define	kBusErrAddressReg			(kSPOTBase + kBusErrAddress)

#define	kBusFenceAddress1			0x0000001c
#define	kBusFenceMask1				0x00000020
#define	kBusFenceAddress2			0x00000024
#define	kBusFenceMask2				0x00000028

#define	kBusFenceAddress1Reg		(kSPOTBase + kBusFenceAddress1)
#define	kBusFenceMask1Reg			(kSPOTBase + kBusFenceMask1)
#define	kBusFenceAddress2Reg		(kSPOTBase + kBusFenceAddress2)
#define	kBusFenceMask2Reg			(kSPOTBase + kBusFenceMask2)

/**************************** DMA ***************************/

#define	kDMA_NVF					0x00000001
#define	kDMA_NV						0x00000002
#define	kDMA_Enable					0x00000004

/* SPOT2-specific */

#define	kDMA_Interlace_Enable		0x00000008

/************************** DISPLAY *************************/

/* SPOT1 Vid Hack H Interrupt lines */

#define kSPOT1TopIntLine			30		/* assumes video from line 32 to line 242 */
#define kSPOT1BotIntLine			246

/* ### Make these consistent with the other regs (offset vs. absolute addr) */

#define	kVidCStart					0x3000
#define	kVidCSize					0x3004
#define	kVidCCnt					0x3008
#define	kVidNStart					0x300c
#define	kVidNSize					0x3010
#define	kVidDMACntl					0x3014
#define	kVidFCntl					0x3018
#define	kVidBlnkCol					0x301c
#define	kVidHStart					0x3020
#define	kVidHSize					0x3024
#define	kVidVStart					0x3028
#define	kVidVSize					0x302c
#define	kVidHIntLine				0x3030
#define	kVidCLine					0x3034
#define	kVidIntStatus				0x3038
#define	kVidIntEnableSet			0x303c
#define kVidIntStatusClear			0x3138
#define	kVidIntEnableClear			0x313c

#define	kVidCStartReg				(kSPOTBase + kVidCStart)
#define	kVidCSizeReg				(kSPOTBase + kVidCSize)
#define	kVidCCntReg					(kSPOTBase + kVidCCnt)
#define	kVidNStartReg				(kSPOTBase + kVidNStart)
#define	kVidNSizeReg				(kSPOTBase + kVidNSize)
#define	kVidDMACntlReg				(kSPOTBase + kVidDMACntl)
#define	kVidFCntlReg				(kSPOTBase + kVidFCntl)
#define	kVidBlnkColReg				(kSPOTBase + kVidBlnkCol)
#define	kVidHStartReg				(kSPOTBase + kVidHStart)
#define	kVidHSizeReg				(kSPOTBase + kVidHSize)
#define	kVidVStartReg				(kSPOTBase + kVidVStart)
#define	kVidVSizeReg				(kSPOTBase + kVidVSize)
#define	kVidHIntLineReg				(kSPOTBase + kVidHIntLine)
#define	kVidCLineReg				(kSPOTBase + kVidCLine)
#define	kVidIntStatusReg			(kSPOTBase + kVidIntStatus)
#define	kVidIntEnableSetReg			(kSPOTBase + kVidIntEnableSet)
#define kVidIntStatusClearReg		(kSPOTBase + kVidIntStatusClear)
#define	kVidIntEnableClearReg		(kSPOTBase + kVidIntEnableClear)


/* kVidFCntl bits */

#define	kVidEnable					0x00000001
#define	kPALMode					0x00000002
#define	kInterlaceMode				0x00000004
#define	kBlankColorEnable			0x00000008
#define	kGammaEnable				0x00000010
#define	kFIDOEnable					0x00000020
#define	kCrCbInvert					0x00000040
#define kUVSelSwap					0x00000080
#define kCrCbSwap					0x00000100
#define kBlankInvert				0x00000200
#define kClkSync					0x00000400
#define kPowerDown					0x00000800
#define kBT852Mode					0x00001000

/* SPOT2-specific kVidFCntl bits */

#define	kUVSelSwap					0x00000080

/* Vid Int bits */

#define	kVidFIDOIntMask				0x00000040
#define	kVidVSyncEvenIntMask		0x00000020
#define	kVidVSyncOddIntMask			0x00000010
#define	kVidHSyncIntMask			0x00000008
#define	kVidDMAIntMask				0x00000004

/*************************** AUDIO **************************/

#define	kAudCStart					0xa4002000
#define	kAudCSize					0xa4002004
#define	kAudCConfig					0xa4002008
#define	kAudCCnt					0xa400200c
#define	kAudNStart					0xa4002010
#define	kAudNSize					0xa4002014
#define	kAudNConfig					0xa4002018
#define	kAudDMACntl					0xa400201c

/* kAudFCntl bits */

#define	k8Bit						0x00000002
#define	kMono						0x00000001

/**************************** IR ****************************/

#define	kIRBufSize					64
#define	kIRBufMask					0x3f

#define	kIRDataReg					0xa4004000

/************************* KEYBOARD *************************/

#define	kKybdBufSize				16
#define	kKybdBufMask				0xf

#define	kKBDataOffset				0x4020
#define	kKBCtlOffset				0x4024

/* absolute addresses */

#define	kKybdDataReg				(kSPOTBase + kKBDataOffset)	
#define	kKybdControlReg				(kSPOTBase + kKBCtlOffset)	

/************************** MODEM ***************************/

/* Rx & Tx Buffer sizes */

#define kModemRcvBufSize			(1024*16)
#define	kModemRcvBufMask			0x3fff
#define	kModemRcvRTSEnableCnt		64			/* # bytes to drain b4 re-enabling Rx ints */

#define kModemXmtBufSize			256			
#define	kModemXmtBufMask			0xff


/* Registers & Bits */

#define	kDivLSB						0x4040
#define	kDivMSB						0x4044

#define	kRxTx						0x4040		/* R = Rx, W = Tx */
#define	kIER						0x4044		/* RW */
#define	kIIR						0x4048		/* RO */
#define	kFCR						0x4048		/* WO */
#define	kLCR						0x404c		/* RW */
#define	kMCR						0x4050		/* RW */
#define	kLSR						0x4054		/* RW */
#define	kMSR						0x4058		/* RW */
#define	kSCR						0x405c		/* RW */


/* absolute addresses */

#define	kModemDivLSB				(kSPOTBase + kDivLSB)	/* all are in LOW BYTE of WORD */
#define	kModemDivMSB				(kSPOTBase + kDivMSB)

#define	kModemRxTx					(kSPOTBase + kRxTx)		/* R = Rx, W = Tx */
#define	kModemIER					(kSPOTBase + kIER)		/* RW */
#define	kModemIIR					(kSPOTBase + kIIR)		/* RO */
#define	kModemFCR					(kSPOTBase + kFCR)		/* WO */
#define	kModemLCR					(kSPOTBase + kLCR)		/* RW */
#define	kModemMCR					(kSPOTBase + kMCR)		/* RW */
#define	kModemLSR					(kSPOTBase + kLSR)		/* RW */
#define	kModemMSR					(kSPOTBase + kMSR)		/* RW */
#define	kModemSCR					(kSPOTBase + kSCR)		/* RW */

/* LCR bits */

#define	kLCR_DLAB					0x80

#define	kLCR_NoParity				0x00
#define	kLCR_OddParity				0x08
#define	kLCR_EvenParity				0x18
#define	kLCR_MarkParity				0x28
#define	kLCR_SpaceParity			0x38

#define	kLCR_5Bits					0x00
#define	kLCR_6Bits					0x01
#define	kLCR_7Bits					0x02
#define	kLCR_8Bits					0x03

#define	kLCR_1StopBit				0x00
#define	kLCR_2StopBits				0x04

/* LSR bits */

#define	kLSR_DataReady				0x01
#define	kLSR_OverrunErr				0x02
#define	kLSR_ParityErr				0x04
#define	kLSR_FramingErr				0x08
#define	kLSR_BreakInd				0x10
#define	kLSR_THxEmpty				0x20
#define	kLSR_TxEmpty				0x40

/* IER bits */

#define	kIER_MSRInt					0x08
#define	kIER_LSRInt					0x04
#define	kIER_TxEmptyInt				0x02
#define	kIER_RxFullInt				0x01

/* FCR bits */

#define	k1ByteFIFO					0x00
#define	k4ByteFIFO					0x40
#define	k8ByteFIFO					0x80
#define	k14ByteFIFO					0xC0

#define	kDMAMode					0x08	/* DON'T USE!! */
#define	kTxFIFOReset				0x04
#define	kRxFIFOReset				0x02
#define	kFIFOEnable					0x01

/* MCR bits */

#define	kMCR_IntEnable				0x08	/* PC baggage */
#define	kMCR_RTS					0x02
#define	kMCR_DTR					0x01

/* MSR bits */

#define	kMSR_DCD					0x80
#define	kMSR_RI						0x40
#define	kMSR_DSR					0x20
#define	kMSR_CTS					0x10
#define	kMSR_DCD_Changed			0x08
#define	kMSR_RI_Changed				0x04
#define	kMSR_DSR_Changed			0x02
#define	kMSR_CTS_Changed			0x01



#endif /* __HWREGS_H__ */
