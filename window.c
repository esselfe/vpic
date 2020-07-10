#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "vpic.h"

Display *display;
Screen *screen;
int screen_number;
int depth;
Visual *visual;
Window root_window, window;
unsigned int winX = 100, winY = 100, winW = 800, winH = 600;
GC gc;

void vpicWindowInit(void) {
	display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "vpic error: Cannot open an X display!\n");
        exit(1);
    }

    screen = XDefaultScreenOfDisplay(display);
    screen_number = XScreenNumberOfScreen(screen);
    depth = XDefaultDepthOfScreen(screen);
	visual = XDefaultVisualOfScreen(screen);

	XSetWindowAttributes wattr;
    wattr.background_pixel = 0x040810;
    wattr.event_mask = KeyPressMask;
	root_window = XDefaultRootWindow(display);
    window = XCreateWindow(display, root_window,
                (int)winX, (int)winY, winW, winH, 2,
                depth, InputOutput,
                visual,
                CWBackPixel | CWEventMask,
                &wattr);

	XSizeHints wmsize;
    wmsize.flags = USPosition | USSize;
    XSetWMNormalHints(display, window, &wmsize);

	XWMHints wmhint;
    wmhint.initial_state = NormalState;
    wmhint.flags = StateHint;
    XSetWMHints(display, window, &wmhint);

	char *icon_name = "vpic icon";
	XTextProperty iname;
    XStringListToTextProperty(&icon_name, 1, &iname);
    XSetWMIconName(display, window, &iname);

	char *window_name = malloc(128);
	sprintf(window_name, "vpic %s", vpic_version_string);
	XTextProperty wname;
    XStringListToTextProperty(&window_name, 1, &wname);
    XSetWMName(display, window, &wname);
	free(window_name);

    XMapWindow(display, window);

	XGCValues gcv;
    gcv.foreground = 0x304050;
    gcv.background = 0x101010;
    gcv.line_width = 3;
    gc = XCreateGC(display, window, GCForeground | GCBackground | GCLineWidth, &gcv);
}

