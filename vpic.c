#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <getopt.h>
#include <magic.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "vpic.h"

const char *vpic_version_string = "0.1.9";
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

void Msg(unsigned int type, char *msg, ...) {
	va_list va;
	int i;
	unsigned int u;
	long l;
	unsigned long lu;
	char c, *s;
	float f;

	void ProcessMsg() {
		while (*msg != '\0') {
			if (*msg == '%') {
				++msg;
				switch (*msg) {
				case 'd':
					i = va_arg(va, int);
					printf("%d", i);
					break;
				case 'u':
					u = (unsigned int)va_arg(va, unsigned);
					printf("%u", u);
					break;
				case 'l':
					++msg;
					if (*msg == 'u') {
						lu = (unsigned long)va_arg(va, long);
						printf("%lu", lu);
					}
					else if (*msg == 'd') {
						l = (long)va_arg(va, long);
						printf("%ld", l);
					}
					break;
				case 'c':
					c = (char)va_arg(va, int);
					printf("%c", c);
					break;
				case 's':
					s = va_arg(va, char *);
					printf("%s", s);
					break;
				case 'f':
					f = (float)va_arg(va, double);
					printf("%f", f);
					break;
				}
			}
			else
				fputc(*msg, stdout);

			fflush(stdout);
			++msg;
		}
		printf("\n");
	}

	va_start(va, msg);
	if (type == MSG_ALL) {
		ProcessMsg();
	}
	else if (type == MSG_VERBOSE) {
		if (verbose) {
			ProcessMsg();
		}
	}
	else if (type == MSG_DEBUG) {
		if (debug) {
			ProcessMsg();
		}
	}
	va_end(va);
}

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

int vpicHasDirInFilename(char *filename) {
	char *c = filename;
	while (1) {
		if (*c == '\0')
			return 0;
		else if (*c == '/' && c != filename)
			return 1;

		++c;
	}
}

void vpicCreateThumbnailParentDir(char *filename) {
	unsigned int len = strlen(filename), cnt;
	char *dirname = malloc(1024);
	memset(dirname, 0, 1024);
	char *fullname = malloc(1024);
	int ret;
	for (cnt=0; cnt<len; cnt++) {
		if (cnt > 0 && filename[cnt] == '/') {
			sprintf(fullname, "%s/%s", tmpdir, dirname);
			if (debug)
				printf("## vpicCreateThumbnailParentDir(): Creating %s\n", fullname);
			ret = mkdir(fullname, 0755);
			if (ret < 0)
				printf("vpic error: cannot create %s: %s\n", fullname, strerror(errno));
		}
		dirname[cnt] = filename[cnt];
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
		
		if (argc == 1)
			vpicImageLoadFromDirectory(".");
		else {
			struct stat st;
			unsigned int cnt;
			magic_t mg = magic_open(MAGIC_MIME_TYPE);
			magic_load(mg, NULL);
			for (cnt=1; cnt < argc; cnt++) {
				if (argv[cnt][0] == '-') {
					if (cnt >= argc-1) {
						vpicImageLoadFromDirectory(".");
						break;
					}
					continue;
				}

				stat(argv[cnt], &st);
				if (st.st_mode & S_IFDIR)
					vpicImageLoadFromDirectory(argv[cnt]);
				else if (st.st_mode & S_IFREG) {
					if (vpicHasDirInFilename(argv[cnt]))
						vpicCreateThumbnailParentDir(argv[cnt]);
		        	char *mgstr = (char *)magic_file(mg, argv[cnt]);
		            if (strncmp(mgstr, "image/png", 9) == 0)
		                vpicImageAddPNG("./", argv[cnt]);
		            else if (strncmp(mgstr, "image/jpeg", 10) == 0)
		                vpicImageAddJPG("./", argv[cnt]);
		            else
		                vpicImageAddUnsupported("./", argv[cnt]);
				}
			}
			magic_close(mg);
		}

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

