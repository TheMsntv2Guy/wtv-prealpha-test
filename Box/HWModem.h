
#ifndef __HW_MODEM__
#define __HW_MODEM__

#ifdef __cplusplus
extern "C" {
#endif

Error InitModem(void);

ulong putchar_modem(int c);
ulong getbuf_modem(char *buf, ulong cnt);
void ModemDrain(void);
long ModemFIFOCount(void);
void ModemFlush(void);
Boolean ModemGetCD(void);
Boolean ModemGetDTR(void);
void ModemSetDTR(Boolean state);
void MonitorCD(void);

#ifdef __cplusplus
}
#endif

#endif