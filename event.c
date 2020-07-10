#include <stdio.h>
#include <X11/Xlib.h>

#include "vpic.h"

XEvent xevent;

void vpicEvent(void) {
	if (XPending(display)) {
        XNextEvent(display, &xevent);
        switch (xevent.type) {
        case KeyPress:
            if (xevent.xkey.keycode == 9)
                loopend = 1;
			else if (xevent.xkey.keycode == 32) // o
				draw_once = 1;
            break;
        }
    }
}

