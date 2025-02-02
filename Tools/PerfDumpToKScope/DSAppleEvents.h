/******************************************************************************
**
**  Project Name:	DropShell
**     File Name:	DSAppleEvents.h
**
**   Description:	Header w/prototypes for the generic AppleEvent handling routines
**
*******************************************************************************
**                       A U T H O R   I D E N T I T Y
*******************************************************************************
**
**	Initials	Name
**	--------	-----------------------------------------------
**	LDR			Leonard Rosenthol
**	MTC			Marshall Clow
**	SCS			Stephan Somogyi
**
*******************************************************************************
**                      R E V I S I O N   H I S T O R Y
*******************************************************************************
**
**	  Date		Author	Description
**	---------	------	---------------------------------------------
**	11 Dec 93	SCS		Universal Headers/UPPs (Phoenix 68k/PPC & PPCC)
**						Skipped System 6 compatible rev of DropShell source
**	11/24/91	LDR		Added new routines to this header
**	10/29/91	SCS		Changes for THINK C 5
**	10/28/91	LDR		Officially renamed DropShell (from QuickShell)
**	10/06/91	MTC		Converted to MPW C
**	04/09/91	LDR		Added to Projector
**
******************************************************************************/

#ifndef __DSAPPLEEVENTS_H__
#define __DSAPPLEEVENTS_H__

#include <AppleEvents.h>

#include "DSGlobals.h"
#include "DSUtils.h"
#include "DSUserProcs.h"

pascal void		InitAEVTStuff(void);
OSErr			GotRequiredParams(AppleEvent *theAppleEvent);
void			FailErr(OSErr err);

pascal OSErr	_HandleDocs ( AppleEvent *theAppleEvent, AppleEvent *reply, Boolean opening );

pascal OSErr	HandleOAPP(AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
pascal OSErr	HandleQuit(AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
pascal OSErr	HandleODOC(AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
pascal OSErr	HandlePDOC(AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
pascal void		DoHighLevelEvent(EventRecord *event);

#endif
