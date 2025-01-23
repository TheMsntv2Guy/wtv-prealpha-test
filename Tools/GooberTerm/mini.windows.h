/*****
 * mini.windows.h
 *
 *	Public interfaces for mini.windows.c
 *
 *****/

void SetUpWindows(void);
void MyGrowWindow(WindowPtr w, Point p);
void DoContent(WindowPtr theWindow, EventRecord *theEvent);
void ShowSelect(void);
void UpdateWindow (WindowPtr theWindow);
void CloseMyWindow(void);
