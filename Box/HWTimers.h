

#ifndef __HW_TIMERS__
#define __HW_TIMERS__

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------
   	For Big-endian config on 3081 board: 32-bit access, data in lo byte.

	The 2681 (uart/timer) is connected to cpu interrupt 5.

	With an input clock frequency of 3.6864MHz, a downcount value
	of 1843 generates a 1000.1085 Hz (999.89151 ms) clock interrupt.
 */

#define kTimerCount		1843		


void InitTimerHardware(void);
void EnableTimerHardwareInterrupts(int on);


#ifdef __cplusplus
}
#endif

#endif

