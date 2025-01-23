
#ifndef __BOX_DEBUGGER__
#define __BOX_DEBUGGER__

#ifdef __cplusplus
extern "C" {
#endif

void BoxDebugger(void);

void debugErrEntry(void);

void returnFromDebugger(void);			/* this is actually in Interrupts.s */

#ifdef __cplusplus
}
#endif

#endif
