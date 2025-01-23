

#ifndef __CACHES__
#define __CACHES__

#ifdef __cplusplus
extern "C" {
#endif

void FlushDataCache(void);
void FlushDataCacheAddr(void *addr,long len);
void FlushCache(void);
void ClearCache(void);

#ifdef __cplusplus
}
#endif

#endif

