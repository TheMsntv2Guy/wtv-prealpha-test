#ifndef __FENCES__
#define	__FENCES__


/* NOTE:  "bottom" should be a power of 2, and "top" should be ((a power of 2) - 1). */

void SetFence1Range(ulong bottom, ulong top);
void SetFence2Range(ulong bottom, ulong top);

ulong EnableFences(ulong enable);
ulong DisableFences(ulong disable);


#endif /* __FENCES__ */