/*****************************************************/
/*
**	WhoLicensed.c
**
**		License control and eval fail code
** 
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**
**	History	-
**	6/10/93	Created
**	3/26/95	Changed copyright notice
**	6/4/95	Added __DATE__ parameter into copyright notice
**
*/
/*****************************************************/

#include "WhoLicensed.h"
#include <Types.h>
#include <OSUtils.h>
#include <ShutDown.h>

#if 1
#define LICENSE_RIGHTS	"\
HALES grants to Licensee a non-exclusive, worldwide license to use SoundMusicSys in connection with the development \
of Licensed Products, to incorporate SoundMusicSys in such Licensed Products, and to reproduce, distribute, display, and \
perform Licensed Products containing SoundMusicSys. Licensee acknowledges and agrees that it has no right to and shall not reproduce, distribute, \
display or perform SoundMusicSys except as an object code portion of a Licensed Product."


/*
HALES further grants Licensee the right to sublicense the right to \
reproduce SoundMusicSys as incorporated in the Licensed Products.  Licensee may not alter, augment, adapt, or otherwise \
alter SoundMusicSys in any manner, including without limitation, converting or translating SoundMusicSys to support any \
other operating system.  Licensee acknowledges and agrees that it has no right to and shall not reproduce, distribute, \
display or perform SoundMusicSys except as an object code portion of a Licensed Product. SoundMusicSys contains trade \
secrets and Licensee may not decompile, reverse engineer, disassemble, or otherwise reduce SoundMusicSys to human \
readable form."
*/
#define LICENSE_RIGHTS_EVAL	"\
HALES grants to Licensee a 90 day non-exclusive, worldwide license to use SoundMusicSys in connection with the development \
of Licensed Products, to incorporate SoundMusicSys in such Licensed Products, and to reproduce, distribute, display, and \
perform Licensed Products containing SoundMusicSys. Licensee acknowledges and agrees that it has no right to and shall not reproduce, distribute, \
display or perform SoundMusicSys except as an object code portion of a Licensed Product."


/*
HALES grants to Licensee a 90 day non-exclusive, worldwide license to use SoundMusicSys in connection with the development \
of Licensed Products, to incorporate SoundMusicSys in such Licensed Products, and to reproduce, distribute, display, and \
perform Licensed Products containing SoundMusicSys. HALES further grants Licensee the right to sublicense the right to \
reproduce SoundMusicSys as incorporated in the Licensed Products.  Licensee may not alter, augment, adapt, or otherwise \
alter SoundMusicSys in any manner, including without limitation, converting or translating SoundMusicSys to support any \
other operating system.  Licensee acknowledges and agrees that it has no right to and shall not reproduce, distribute, \
display or perform SoundMusicSys except as an object code portion of a Licensed Product. SoundMusicSys contains trade \
secrets and Licensee may not decompile, reverse engineer, disassemble, or otherwise reduce SoundMusicSys to human \
readable form."
*/
#else
#define LICENSE_RIGHTS	""
#define LICENSE_RIGHTS_EVAL	""
#endif


// This is done to protect the name of the special expire function from linkers, and object code disassembly.
#define TestForExpire	JJ_XX_ZZ


#if EVAL_COPY
static Boolean TestForExpire(void)
{
	DateTimeRec	theTime;
	unsigned long	xTime, rTime;
	Boolean		good;

	good = true;
	theTime.year = EVAL_END_YEAR;
	theTime.month = EVAL_END_MONTH;
	theTime.day = EVAL_END_DAY;
	theTime.hour = 0;
	theTime.minute = 0;
	theTime.second = 0;
	theTime.dayOfWeek = 0;
	
	Date2Secs(&theTime, &xTime);
	GetDateTime(&rTime);

	if (rTime > xTime)
	{
//		DebugStr((void *)"\pExpire");
		good = false;
	}
	return good;
}
#endif


void * CodeSignature(void)
{
#if THINK_C
	register void * pp;
#if EVAL_COPY
	if (TestForExpire() == false)
	{
		return NULL;
	}
#endif
	asm
	{
		bra	@realcode
@realdata:
		dc.b	__DATE__
		dc.b	"SoundMusicSys © Copyright 1989-1996 by Steve Hales, All Rights Reserved"
		dc.b	LICENSEE
#if !  EVAL_COPY
		dc.b	LICENSE_RIGHTS
#else
		dc.b	LICENSE_RIGHTS_EVAL
#endif
		nop
		nop
		nop
		nop
		nop
@realcode:
		lea	@realdata,A0
		move.l	A0, pp;
	}
	return pp;
#else
	char *pp;
	static char *id[] = {	__DATE__, 
					"SoundMusicSys © Copyright 1989-1996 by Steve Hales, All Rights Reserved",
					LICENSEE,
#if !  EVAL_COPY
					LICENSE_RIGHTS
#else
					LICENSE_RIGHTS_EVAL
#endif
					};

#if EVAL_COPY
	if (TestForExpire() == false)
	{
		return NULL;
	}
#endif
	pp = id[0];		// force a link
	pp = id[1];		// force a link
	return id[2];
#endif
}




/* EOF of WhoLicensed.c
*/
