#include <stdio.h>
#include <X11/Xlib.h>

#include "vpic.h"

XEvent xevent;

void vpicEvent(void) {
	if (XPending(display)) {
		XNextEvent(display, &xevent);
		switch (xevent.type) {
		case KeyPress:
			if (debug)
				printf("key: type %d, code %d\n", xevent.type, xevent.xkey.keycode);
			if (xevent.xkey.keycode == 9)
				loopend = 1;
			else if (xevent.xkey.keycode == 32) // o
				draw_once = 1;
			else if(xevent.xkey.keycode == 40) // d
				debug = !debug;
			else if (xevent.xkey.keycode == 55) // v
				verbose = !verbose;
			break;
		}
	}
}

