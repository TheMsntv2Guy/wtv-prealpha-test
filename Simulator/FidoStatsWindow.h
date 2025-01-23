#ifndef __FIDOSTATSWINDOW_H__
#define __FIDOSTATSWINDOW_H__

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif

#ifndef __FIDO_COMPATIBILITY_H__
#include "FidoCompatibility.h"
#endif

class FidoStatsWindow : public StdWindow{
	class statWindow*		stats;
public:
						FidoStatsWindow();
	virtual				~FidoStatsWindow();			
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			DrawHeader(Rect *r);
	virtual	void			Idle();
};

#else
#error "Attempted to #include FidoStatsWindow.h multiple times"
#endif /* __FIDOSTATSWINDOW_H__ */
