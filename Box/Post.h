
/**************************************/
/**** Prototypes                  *****/
/**************************************/


Boolean TestSPOTReg (short *count, ulong SPOTReg, ulong testval);
ulong GetSPOTRegister (ulong SPOTReg);
Boolean SPOTRegResetTest (void);
void infiniteLoop (void);
void Post(void);
static  Boolean 	KBSelfTestOK(void);
Boolean MemTest(void);
static uchar 	KBReadData(void);
static void 	KBWriteCommand(uchar data);

