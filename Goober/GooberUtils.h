

#ifndef __GOOBER_UTILITIES__
#define __GOOBER_UTILITIES__

#ifdef __cplusplus
extern "C" {
#endif

void	SetGooberLEDs(ulong bits);
ulong	GetGooberLEDs(void);

void	GooberHeartbeat(void);

#ifdef __cplusplus
}
#endif

#endif