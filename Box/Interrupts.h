
#ifndef __INTERRUPTS__
#define __INTERRUPTS__

#ifdef __cplusplus
extern "C" {
#endif

extern ulong 	SystemIntHook;

void 	InterruptDecode(void);			/* main system interrupt decoder/dispatcher */

ulong 	EnableInts(ulong ints);			/* returns int enable state AFTER enable */
ulong 	DisableInts(ulong ints);		/* returns int enable state BEFORE disable */

ulong 	GetIntEnables(void);

void 	ClearAllInts(void);

void 	DisableAllInts(void);



#ifdef __cplusplus
}
#endif


#endif

