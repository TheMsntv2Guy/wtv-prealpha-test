
/* Forth Stuff */

//#define PF_NO_INIT		1

//#define	PF_NO_SHELL		1
//#define	PF_MEM_POOL_SIZE

#define PF_NO_MALLOC		1
#define	PF_NO_FILEIO		1

#ifdef PF_SUPPORT_FP
#undef PF_SUPPORT_FP
#endif

