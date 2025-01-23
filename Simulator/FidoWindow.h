#ifndef __FIDOWINDOW_H__
#define __FIDOWINDOW_H__

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif

#ifndef __FIDO_COMPATIBILITY_H__
#include "FidoCompatibility.h"
#endif

class FidoWindow : public StdWindow{
public:
							FidoWindow();
	virtual					~FidoWindow();			
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			DrawHeader(Rect *r);
	virtual	void			Idle();
private:
	class FidoStatsWindow* 	stats;
	FidoCompatibilityState*	state;
};

#else
#error "Attempted to #include FidoWindow.h multiple times"
#endif /* __FIDOWINDOW_H__ */
