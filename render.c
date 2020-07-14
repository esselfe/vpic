#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "vpic.h"

unsigned int draw_once = 10;
unsigned int fps;
char strfps[20];

void vpicRender(void) {
	struct ImageNode *in = rootImageList.first_image;
	int cnt = 0;
	while (1) {
		++cnt;
		XPutImage(display, window, gc, in->ximage, 0, 0, 
//			10*cnt + 100*cnt - 100, 20,
			in->x, in->y, 100, 100);
		XDrawImageString(display, window, gc, 10*cnt + 100*cnt - 100, 140,
			in->original_name, strlen(in->original_name));

		if (in->next == NULL)
			break;
		else
			in = in->next;
	}
}

