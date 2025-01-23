//
// DisplayablesWindow.h
//

#ifndef __DISPLAYABLESWINDOW_H__
#define __DISPLAYABLESWINDOW_H__

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

class DisplayablesWindow : public StdWindow{
public:
					
							DisplayablesWindow();
	virtual					~DisplayablesWindow();			
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			DrawHeader(Rect *r);
	virtual	void			Idle();
	
protected:
			Resource			fLastResource;
};

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include DisplayablesWindow.h multiple times"
	#endif
#endif /* __DISPLAYABLESWINDOW_H__ */
