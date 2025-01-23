#ifndef __IIC__
#define __IIC__

#ifdef __cplusplus
extern "C" {
#endif

void InitIIC(void);
void IICWrite(uchar comp,uchar addr,uchar val);
uchar IICRead(uchar comp,uchar addr);

#ifdef __cplusplus
}
#endif

#endif
