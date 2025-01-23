

#ifndef __SILICON_SERIAL__
#define __SILICON_SERIAL__

#ifdef __cplusplus
extern "C" {
#endif


#define	kMaxSiSerialNumRetries	4

void InitSiliconSerialNumber(void);

Boolean ReadSiliconSerialNumber(ulong countsPerMicro, uchar *databuf);


#ifdef __cplusplus
}
#endif

#endif