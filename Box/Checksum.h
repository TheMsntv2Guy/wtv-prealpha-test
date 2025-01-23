#ifndef	__CHECKSUMS__
#define __CHECKSUMS__

#ifdef __cplusplus
extern "C" {
#endif


Boolean ROMCodeChecksumOK(ulong base, ulong size);
Boolean ROMFSChecksumOK(ulong base, ulong size);


#ifdef __cplusplus
}
#endif

#endif /* __CHECKSUMS__ */