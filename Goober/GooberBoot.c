#include "Headers.h"
#include "BoxAbsoluteGlobals.h"

#include "HWExpansion.h"

#ifdef BOX_FORTH
#include "pforth.h"
#endif


#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

#define MSG(cs) { pfMessage(cs); }

#define DEFAULT_RETURN_DEPTH (512)
#define DEFAULT_USER_DEPTH (512)
#define DEFAULT_HEADER_SIZE (60000)
#define DEFAULT_CODE_SIZE (150000)

int DoForth( char *DicName, char *SourceName, int IfInit );


#ifdef SIMULATOR
AbsoluteGlobals	gSimAbsoluteGlobals;
#else
extern "C" void BoxDebugger(void);
#endif


#ifndef BOX_FORTH

void LateBoot(void)
{
	printf("Entering debugger...\n");
	BoxDebugger();
}

#else /* BOX_FORTH */

void LateBoot(void)
{
	char *DicName = "pforth.dic";
	char *SourceName = NULL;
	char IfInit;

	// Check dipswitch 0, call either downloader or pForth

	IfInit = FALSE;

	printf("Cranking up FORTH...\n");

	DoForth( DicName, SourceName, IfInit);
	printf("Forth exited!\n");
	printf("Entering debugger...\n");
	BoxDebugger();

}


/**************************************************************************
** Conveniance routine to execute PForth
*/
int DoForth( char *DicName, char *SourceName, int IfInit )
{
	cfTaskData *cftd;
	cfDictionary *dic;
	int Result = 0;
	ExecToken  EntryPoint = 0;
	
/* Allocate Task structure. */

	cftd = pfCreateTask( DEFAULT_USER_DEPTH, DEFAULT_RETURN_DEPTH );

	if( cftd )
	{
		pfSetCurrentTask( cftd );
		
#if 1
/* Don't use MSG before task set. */
	MSG("DoForth: \n");
	if( IfInit ) MSG("IfInit TRUE\n");
	
	if( DicName )
	{
		MSG("DicName = "); MSG(DicName); MSG("\n");
	}
	if( SourceName )
	{
		MSG("SourceName = "); MSG(SourceName); MSG("\n");
	}
#endif

		dic = pfLoadDictionary( DicName, &EntryPoint );

		if( dic == NULL ) goto error;
		
		if( EntryPoint != 0 )
		{
			pfExecuteToken( EntryPoint );
		}
#ifndef PF_NO_SHELL
		else
		{
			if( SourceName == NULL )
			{
				Result = pfRunForth();
			}
			else
			{
				MSG("Including: ");
				MSG(SourceName);
				MSG("\n");
				Result = pfIncludeFile( SourceName );
			}
		}
#endif /* PF_NO_SHELL */
		
		pfDeleteDictionary( dic );
		pfDeleteTask( cftd );
	}
	return Result;
	
error:
	MSG("DoForth: Error occured.\n");
	pfDeleteTask( cftd );
	return -1;
}

#endif /* BOX_FORTH */
