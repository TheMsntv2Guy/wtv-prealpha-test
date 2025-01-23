

#ifndef __FRAMES__
#define	__FRAMES__

#define FRAME(name,frm_reg,offset,ret_reg)      \
	.globl  name;                           	\
	.ent    name;                           	\
name:;                                          \
	.frame  frm_reg,offset,ret_reg

#define ENDFRAME(name)                          \
	.end name


#endif

