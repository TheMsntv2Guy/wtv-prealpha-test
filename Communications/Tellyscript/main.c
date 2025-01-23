#include "TellyDefs.h"
#include "TellyIO.h"

/* note: to be used to build a standalone version for testing */

main()
{
	char s[80];

/* get the name of the script to execute */

	printf("Script name: ");
	gets(s);
	printf("\n");
	RunScript((Byte *) s);		/* load the script into ram and begin interpreting */
}
