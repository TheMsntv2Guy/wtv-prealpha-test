/*
	These aren't really ANSI things...they really belong elsewhere
*/

#ifndef __CHARIO_H__
#define	__CHARIO_H__

#ifdef __cplusplus
extern "C" {
#endif

extern Boolean gPutCharTV;

void getstring(uchar *s,ulong length,ulong timeout);

void TV_putchar(int c);
void TV_paintchar(uchar c, ulong col, ulong row);

void Parallel_putchar(int c);
void ParallelWrite(uchar byte);
void ParallelAck(uchar assert);
void ParallelWaitStrobe(uchar assert);

#ifdef __cplusplus
}
#endif

#endif /* __CHARIO_H__ */
