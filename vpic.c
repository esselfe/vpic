#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "vpic.h"

const char *vpic_version_string = "0.1.6";
unsigned int loopend;
unsigned int use_framebuffer;
unsigned int verbose;

static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'V'},
	{"verbose", no_argument, NULL, 'v'},
	{"fb", no_argument, NULL, 'F'},
	{NULL, 0, NULL, 0}
};
static const char *short_options = "hVvF";

void ShowHelp(void) {
	printf("vpic options:\n"
		"\t-h, --help       Show this help message\n"
		"\t-V, --version    Show program version and exit\n"
		"\t-v, --verbose    Show more detailed informations\n"
		"\t-F, --fb         Draw image on framebuffer (/dev/fb0)\n");
}

int main(int argc, char **argv) {
	int c;
	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1) break;
		switch (c) {
		case 'h':
			ShowHelp();
			exit(0);
		case 'V':
			printf("vpic %s\n", vpic_version_string);
			exit(0);
		case 'v':
			verbose = 1;
			break;
		case 'F':
			use_framebuffer = 1;
			break;
		}
	}
	if (verbose)
		printf("vpic %s\n", vpic_version_string);

	if (use_framebuffer)
		vpic_fb_draw();
	else { // render using X11
		if (verbose)
			printf("Initializing X11 window\n");
		vpicWindowInit();
		
		vpicImageLoadFromDirectory("images");

		time_t tp = time(NULL), tc;
		while (!loopend) {
			++fps;
			tc = time(NULL);
			if (tc > tp) {
				tp = tc;
				sprintf(strfps, "%u fps", fps);
				fps = 0;
				XClearArea(display, window, 10, 9, 200, 3, False);
			    XDrawImageString(display, window, gc, 10, 10, strfps, strlen(strfps));		    
				vpicRender();
			}
			XDrawLine(display, window, gc, 50, 10, 50+fps, 10);
			vpicEvent();

			usleep(250000);
		}
		XDestroyWindow(display, window);
		XCloseDisplay(display);
	}
	if (verbose)
		printf("Exiting\n");

	return 0;
}

