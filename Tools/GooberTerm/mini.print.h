/****
 * mini.print.h
 *
 *	Public interface for mini.print.c
 *
 ****/

void DoPageSetUp(void);
void PrintText(char	**hText, long length, GrafPtr gp, int tabPixels);
