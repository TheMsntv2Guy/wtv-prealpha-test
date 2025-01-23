#ifndef __SMARTCARD__
#define __SMARTCARD__

#ifdef __cplusplus
extern "C" {
#endif

void CheckSmartCard(void);
void SmartCardIdle(void);
void PostSmartCardEvent(ulong event);

#ifdef __cplusplus
}
#endif

#endif
