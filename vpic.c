#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "vpic.h"

const char *vpic_version_string = "0.1.8";
unsigned int loopend;
unsigned int use_framebuffer;
unsigned int debug, verbose;
unsigned int run_rgb2hdr;
char *rgb2hdr_filename;
char *tmpdir;

static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'V'},
	{"verbose", no_argument, NULL, 'v'},
	{"debug", no_argument, NULL, 'D'},
	{"fb", no_argument, NULL, 'F'},
	{"rgb2hdr", required_argument, NULL, 'r'},
	{NULL, 0, NULL, 0}
};
static const char *short_options = "hVvDFr:";

void ShowHelp(void) {
	printf("vpic options:\n"
		"\t-h, --help	   Show this help message\n"
		"\t-V, --version	Show program version and exit\n"
		"\t-v, --verbose	Show more detailed informations\n"
		"\t-D, --debug	  Show intrisict information to detect errors and bugs\n"
		"\t-F, --fb		 Draw image on framebuffer (/dev/fb0) [undeveloped]\n");
}

void vpicExit(void) {
	char cmd[1024];
	if (debug)
		sprintf(cmd, "rm -rfv %s", tmpdir);
	else
		sprintf(cmd, "rm -rf %s", tmpdir);
	system(cmd);

	if (debug) {
		time_t t0 = time(NULL);
		struct tm *tm0 = localtime(&t0);
		struct timeval tv0;
		gettimeofday(&tv0, NULL);
		printf("Exiting: %02d%02d%02d-%02d%02d%02d.%06ld\n", tm0->tm_year-100,
			tm0->tm_mon+1, tm0->tm_mday, tm0->tm_hour, tm0->tm_min, tm0->tm_sec,
			tv0.tv_usec);
	}
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
		case 'D':
			debug = 1;
			verbose = 1;
			printf("## debug enabled\n");
			break;
		case 'F':
			use_framebuffer = 1;
			break;
		case 'r':
			run_rgb2hdr = 1;
			rgb2hdr_filename = malloc(strlen(optarg)+1);
			sprintf(rgb2hdr_filename, "%s", optarg);
			break;
		}
	}
	if (verbose || debug)
		printf("vpic %s\n", vpic_version_string);

	atexit(vpicExit);

	time_t t0 = time(NULL);
	struct tm *tm0 = localtime(&t0);
	struct timeval tv0;
	gettimeofday(&tv0, NULL);
	tmpdir = malloc(strlen("/tmp/vpic-201231-235959.999999"));
	sprintf(tmpdir, "/tmp/vpic-%02d%02d%02d-%02d%02d%02d.%06ld", tm0->tm_year-100,
		tm0->tm_mon+1, tm0->tm_mday, tm0->tm_hour, tm0->tm_min, tm0->tm_sec,
		tv0.tv_usec);
	if (mkdir(tmpdir, 0755) == -1) {
		fprintf(stderr, "vpic error: Cannot open %s: %s\n", tmpdir, strerror(errno));
		return 1;
	}
	if (debug)
		printf("## tmpdir: %s\n", tmpdir);

	if (use_framebuffer)
		vpic_fb_draw();
	else { // render using X11
		if (debug)
			printf("## Initializing X11 window\n");
		vpicWindowInit();
		
		vpicImageLoadFromDirectory("images");

		if (verbose && !debug)
			printf("%u items total\n", page.total_images);
		else if (debug) {
			printf("## main(): %u items total\n", page.total_images);
			printf("## starting mainloop\n");
		}
		time_t tp = time(NULL), tc;
		while (!loopend) {
			++fps;
			tc = time(NULL);
			if (tc > tp) {
				tp = tc;
				fps = 0;
				if (debug) {
					sprintf(strfps, "%u fps", fps);
					XClearArea(display, window, 10, 9, 200, 3, False);
					XDrawImageString(display, window, gc, 10, 10, strfps, strlen(strfps));
				}
			}

			vpicRender();

			if (debug)
				XDrawLine(display, window, gc, 50, 10, 50+fps, 10);
			
			vpicEvent();

			usleep(250000);
		}
		XDestroyWindow(display, window);
		XCloseDisplay(display);
	}

	return 0;
}

