/*********************************************************************

	mini.print.c
	
	printing functions for Miniedit
	
*********************************************************************/

#include "mini.print.h"
#include "pleasewait.h"

#define topMargin 20
#define leftMargin 20
#define bottomMargin 20
#define tabChar	((char)'\t')

static	THPrint	hPrint = NULL;
static	int		tabWidth;



	/**
	 **		Prototypes for private functions.
	 **		(They really should be static.)
	 **
	 **/
 
void CheckPrintHandle(void);
void MyDrawText(char *p, int count);
void PrDoc(char **hText, long count, THPrint hPrint, int font, int size);
int HowMany(void);


void CheckPrintHandle()
{
	if (hPrint==NULL) 
		PrintDefault(hPrint = (TPrint **) NewHandle(sizeof( TPrint)));
}

void DoPageSetUp()
{
	PrOpen();
	CheckPrintHandle();
	if (PrStlDialog(hPrint)) ;
	PrClose();
}


void MyDrawText(char	*p, int count)
{
	register char	*p1, *p2;
	int				len;
	Point			pt;

	p1 = p;
	p2 = p+count;
	while (p<p2) {
		while ((p1<p2) && (*p1 !=tabChar)) *p1++;
		if ((len=p1-p)>0) DrawText(p, 0, p1-p);
		if (*p1==tabChar) {
			GetPen(&pt);
			Move((tabWidth-(pt.h-leftMargin)%tabWidth), 0);
			*p1++;
		}
		p = p1;
	}
}

void PrDoc (char **hText, long count, THPrint hPrint, int font, int size)
{
	register int 	line = 0;
	register int 	lastLineOnPage = 0;
	int				length;
	Rect 			printRect;
	int 			linesPerPage;
	int 			lineBase;
	int 			lineHeight;
	register char 	*ptr, *p1;
	FontInfo		info;
	TPPrPort		printPort;

	printPort = PrOpenDoc(hPrint, 0L, 0L);
	SetPort((GrafPtr)printPort);
	TextFont(font);
	TextSize(size);
	printRect = (**hPrint).prInfo.rPage;
	GetFontInfo(&info);
	lineHeight = info.leading+info.ascent+info.descent;
	linesPerPage = 
		(printRect.bottom-printRect.top-topMargin-bottomMargin)/lineHeight;
	HLock(hText);
	ptr = p1 = (*hText);
	do {
		PrOpenPage(printPort, 0L);
		lastLineOnPage += linesPerPage;
		MoveTo( printRect.left+leftMargin, 
			(lineBase = printRect.top+lineHeight) );
		do {
			/* PrintLine: */
			while ((ptr<=(*hText)+count) && (*ptr++ != (char)'\r')) ;
			if ((length=(int)(ptr-p1)-1)>0)
				MyDrawText(p1, length);
			MoveTo( printRect.left+leftMargin, (lineBase += lineHeight));
			p1 = ptr;
		} while ((++line != lastLineOnPage) && (ptr<(*hText)+count));
		PrClosePage(printPort);
	} while (ptr<(*hText)+count);
	HUnlock(hText);
	PrCloseDoc(printPort);
}

void PrintText(char	**hText, long length, GrafPtr gp, int tabPixels)

{
	TPPrPort	printPort;
	GrafPtr		savePort;
	TPrStatus	prStatus;
	int			copies;
	
    PrOpen();
	CheckPrintHandle();
	tabWidth = tabPixels;
	SetCursor(&qd.arrow);
	if (PrJobDialog(hPrint) != 0) {
		PleaseWait();
		GetPort(&savePort);
		for (copies=HowMany(); copies>0; copies--) {
			PrDoc (hText, length, hPrint, (*gp).txFont, (*gp).txSize);
			PrPicFile(hPrint, 0L, 0L, 0L, &prStatus);
		}
		SetPort(savePort);
	}
	PrClose();
}

int HowMany(void)
{
	return( ((**hPrint).prJob.bJDocLoop==bDraftLoop) ? 
				(**hPrint).prJob.iCopies : 1 );
}


