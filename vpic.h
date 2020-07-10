#ifndef VPIC_H
#define VPIC_H 1

extern const char *vpic_version_string;
extern unsigned int loopend, verbose;

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
    struct ImageNode *prev, *next;
	char *fullname;
    char *filename;
	float ratio;
	unsigned int preview_width, preview_height;
	unsigned int original_width, original_height;
    unsigned int file_size;
    unsigned int data_size;
    char *data;
	XImage *ximage;
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
void vpicJPGLoad(struct ImageNode *in);

// from png.c
void vpicPNGLoad(struct ImageNode *in);

// from render.c
extern unsigned int draw_once;
extern unsigned int fps;
extern char strfps[20];

void vpicRender(void);

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
