#ifndef VPIC_H
#define VPIC_H 1

extern const char *vpic_version_string;
extern unsigned int loopend, debug, verbose;
extern char *tmpdir;

// from event.c
#include <X11/Xlib.h>
extern XEvent xevent;
void vpicEvent(void);

// from fb.c
void vpic_fb_draw(void);

// from image.c
#define IMAGE_TYPE_JPG    1
#define IMAGE_TYPE_PNG    2
#define IMAGE_TYPE_DIRECTORY      3
#define IMAGE_TYPE_UNSUPPORTED    4

struct ImageNode {
	unsigned int type;
	unsigned int rank;
    struct ImageNode *prev, *next;
	struct Thumbnail *thumbnail;
    char *filename, *fullname, *original_name;
	char *thumbnail_filename;
	float ratio;
	unsigned int original_width, original_height;
	int row_bytes, xrow_bytes;
    unsigned int file_size;
    unsigned int data_size;
    char *data;
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
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
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
		total_lines;
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
