#ifndef	__BIP__
#define __BIP__

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * 
 * Be careful if you change these. kBufferSize needs to be bigger than WINDOW_SIZE
 * and should be a power of 2.
 *
 */

#define WINDOW_SIZE (tcp_MSS * 7)		/* 3752 */
//#define WINDOW_SIZE (tcp_MSS * 15)

#define kBufferSize 	0x1000
//#define kBufferSize 0x2000

#define kBufferMask		(kBufferSize-1)

Error	bipInit();
Error	bipConnect(long hostAddress, short hostPort);
Error	bipClose();
Error	bipWrite(const void *data, long length, long *count);
Error	bipRead(char* buffer, long length, long *count );


#ifdef __cplusplus
}
#endif

#endif /* __BIP__ */

