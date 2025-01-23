
#ifndef __EXCEPTIONS__
#define	__EXCEPTIONS__

#ifndef __IREGDEF_H__
#include "iregdef.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern ulong	except_regs[NREGS];		/* registers at time of exception */
extern ulong 	audioIntRegs[NREGS];	/* audio interrupt handler reg storage */

void otherException(void);				/* system generic exception handler */

ulong FetchConfig(void);
ulong FetchCalg(void);
ulong FetchCounter(void);

void enable_cpu_ints(unsigned long mask);
void disable_cpu_ints(unsigned long mask);
void move_exc_code(void);
void restoreContextAndReturn(void);

#ifdef __cplusplus
}
#endif

#endif
