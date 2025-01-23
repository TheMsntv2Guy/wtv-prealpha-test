/*
 * Compression routines.
 */
#ifndef __Compression__
#define __Compression__

#ifdef __cplusplus
extern "C" {
#endif

/* identifies contents of a compressed block */
typedef struct CompressHeader {
    ulong       compressedSize;         /* how many bytes to read */
    ulong       expandedSize;           /* how big it's going to get */
    ulong       crc32;                  /* CRC-32 on *uncompressed* data */
    uchar       type;                   /* compression method used */
    uchar       flags;                  /* flag bits, see below */
    uchar       pad[2];                 /* (explicit 32-bit alignment) */
} CompressHeader;

/* values for compressType, must fit in uchar */
typedef enum {
    kCompressNone = 0,
    kCompressLzss
} CompressTypes;

/* values for flags, must fit in uchar */
#define kCompressHasCrc ((uchar)(1))

long CompressLzss(const uchar *in_buf, uchar *out_buf, ulong uncr_length);
long ExpandLzss(const uchar *in_buf, uchar *out_buf, ulong length, ulong uncr_length);

#ifdef __cplusplus
}
#endif

#endif /*__Compression__*/