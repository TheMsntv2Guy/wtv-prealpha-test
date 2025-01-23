#include "ProgressBar.h"

DialogPtr gProgressDialog;
long gTotalBytes = 0;
long gBytesSoFar = 0;
Rect gProgressRect;
GrafPtr gOldPort;

void InitProgress(char *progressString,long nBytes)
{
	Handle h;
	short type;
	
	GetPort(&gOldPort);
	gProgressDialog = GetNewDialog(130,nil,(WindowPtr)-1);
	SetPort(gProgressDialog);
	ParamText(C2PStr(progressString),"\p","\p","\p");
	ShowWindow(gProgressDialog);
	UpdtDialog(gProgressDialog,gProgressDialog->visRgn);
	GetDItem(gProgressDialog,1,&type,&h,&gProgressRect);
	FrameRect(&gProgressRect);
	InsetRect(&gProgressRect,2,2);
	gTotalBytes = nBytes;
	gBytesSoFar = 0;
}

void UpdateProgress(long count)
{
	float percentDone;
	Rect r;
	
	gBytesSoFar += count;
	percentDone = (float)gBytesSoFar/(float)gTotalBytes;
	r = gProgressRect;
	r.right = percentDone*gProgressRect.right;
	FillRect(&r,&qd.black);
}

void EndProgress(void)
{
	DisposeDialog(gProgressDialog);
	SetPort(gOldPort);
}