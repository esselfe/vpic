#ifndef VPIC_H
#define VPIC_H 1

// from vpic.c
extern const char *vpic_version_string;
extern unsigned int loopend, debug, verbose;
extern unsigned int run_rgb2hdr;
extern char *rgb2hdr_filename;
extern char *tmpdir;

// Message types for Msg()
#define MSG_ALL     0
#define MSG_VERBOSE 1
#define MSG_DEBUG   2

#define MSGF(s,...) \
if (debug) { \
	printf("## %s(): ", __FUNCTION__); \
	printf(s, ##__VA_ARGS__); \
	printf("\n"); \
}
#define MSGA(s, ...) Msg(MSG_ALL, s, ##__VA_ARGS__)
#define MSGD(s, ...) Msg(MSG_DEBUG, s, ##__VA_ARGS__)
#define MSGV(s, ...) Msg(MSG_VERBOSE, s, ##__VA_ARGS__)
void Msg(unsigned int type, char *msg, ...);
int vpicHasDirInFilename(char *filename);
void vpicCreateThumbnailParentDir(char *filename);

// from event.c
#include <X11/Xlib.h>
extern XEvent xevent;
void vpicEvent(void);

// from fb.c
void vpic_fb_draw(void);

// from image.c
#define IMAGE_TYPE_JPG	1
#define IMAGE_TYPE_PNG	2
#define IMAGE_TYPE_DIRECTORY	  3
#define IMAGE_TYPE_UNSUPPORTED	4

struct ImageNode {
	unsigned int type;
	unsigned int rank, line_rank;
	struct ImageNode *prev, *next;
	struct Thumbnail *thumbnail;
	char *filename, *fullname, *original_name;
	float ratio;
	unsigned int original_width, original_height;
	int row_bytes, x_row_bytes;
	unsigned int file_size;
	unsigned int data_size;
	char *data;
	struct PageLine *page_line;
	XImage *ximage;
	unsigned int x, y;
};

struct ImageList {
	struct ImageNode *first_image, *last_image;
	unsigned int image_total;
} rootImageList;

int vpicImageLoadFromDirectory(char *dirname);
void vpicImageAddJPG(char *dirname, char *filename);
void vpicImageAddPNG(char *dirname, char *filename);
void vpicImageAddDirectory(char *dirname, char *filename);
void vpicImageAddUnsupported(char *dirname, char *filename);
void vpicImageLoadDataPNG(struct ImageNode *in);

// from jpg.c
#include <jpeglib.h>
#include <setjmp.h>
struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;		/* for return to caller */
};
typedef struct my_error_mgr *my_error_ptr;
void my_error_exit(j_common_ptr cinfo);

void vpicJPGLoad(struct ImageNode *in);

// from page.c
struct PageLine {
	struct PageLine *prev, *next;
	struct ImageNode *first_image, *last_image;
	unsigned int rank, x, y;
};

struct Page {
	struct PageLine *first_line, *last_line;
	unsigned int rank, images_per_line,
		total_lines, total_images;
};
extern struct Page page;

struct PageLine *vpicPageAddLine(void);
void vpicPageLineAddImage(struct ImageNode *in);

// from png.c
void vpicPNGLoad(struct ImageNode *in);
void vpicPNGtoJPG(struct ImageNode *in);

// from render.c
extern unsigned int draw_once;
extern unsigned int fps;
extern char strfps[20];

void vpicRender(void);

// from rgb.c
void vpicRGBtoHeader(char *varname, unsigned int size, char *data);

// from thumbnail.c
struct Thumbnail {
	char *filename, *fullname;
	float ratio;
	unsigned int width, height;
	unsigned int components;
	unsigned int row_bytes, x_row_bytes;
	unsigned int file_size;
	unsigned int data_size;
	char *data;
	XImage *ximage;
};

void vpicThumbnailGenerate(char *src, char *dst);
void vpicThumbnailCreateJPG(struct ImageNode *in);
void vpicThumbnailCreatePNG(struct ImageNode *in);
void vpicThumbnailCreateDirectory(struct ImageNode *in);
void vpicThumbnailCreateUnsupported(struct ImageNode *in);

// from window.c
#include <X11/Xutil.h>
extern Display *display;
extern Screen *screen;
extern int screen_num;
extern int depth;
extern Visual *visual;
extern Window root_window, window;
extern unsigned int winX, winY, winW, winH;
extern GC gc;

void vpicWindowInit(void);

#endif /* VPIC_H */
