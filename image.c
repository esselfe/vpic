#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <magic.h>
#include <png.h>
#include <setjmp.h>

#include "vpic.h"

struct ImageList rootImageList;

int vpicImageLoadFromDirectory(char *dirname) {
	if (debug)
		printf("## vpicImageLoadFromDirectory(): loading %s\n", dirname);
	
	DIR *dir = opendir(dirname);
	if (dir == NULL) {
		fprintf(stderr, "vpic error: cannot open %s: %s\n", dirname, strerror(errno));
		return 1;
	}
	
	magic_t mg = magic_open(MAGIC_MIME_TYPE);
	magic_load(mg, NULL);

	struct dirent *de;
	char *mgstr;
	char full_filename[4096];
	int cnt = 0;
	while (1) {
		++cnt;
		errno = 0;
		de = readdir(dir);
		if (de == NULL && errno == 0)
			break;
		else if (de == NULL && errno != 0) {
			fprintf(stderr, "vpic error: cannot open entry #%u from %s: %s\n", 
				cnt, dirname, strerror(errno));
			continue;
		}

		if (strncmp(de->d_name, ".", 1) == 0)
			continue;
		else if (strncmp(de->d_name, "..", 2) == 0)
			continue;
		else if (strncmp(de->d_name, "folder.png", 10) == 0)
			continue;
		
		sprintf(full_filename, "%s/%s", dirname, de->d_name);

		if (de->d_type == DT_DIR)
			vpicImageAddDirectory(dirname, de->d_name);
		else if (de->d_type == DT_REG) {
			mgstr = (char *)magic_file(mg, full_filename);
			if (strncmp(mgstr, "image/png", 9) == 0)
				vpicImageAddPNG(dirname, de->d_name);
			else if (strncmp(mgstr, "image/jpeg", 10) == 0)
				vpicImageAddJPG(dirname, de->d_name);
			else
				vpicImageAddUnsupported(dirname, de->d_name);
		}

	}

	magic_close(mg);

	if (debug)
		printf("## vpicImageLoadFromDirectory(): end\n");
	
	return 0;
}

void vpicImageAddJPG(char *dirname, char *filename) {
	if (debug)
		printf("\n## vpicImageAddJPG(): processing %s\n", filename);
	
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_JPG;
	if (rootImageList.first_image == NULL) {
		rootImageList.first_image = in;
		in->prev = NULL;
	}
	else {
		rootImageList.last_image->next = in;
		in->prev = rootImageList.last_image;
	}
	in->next = NULL;
	in->fullname = malloc(strlen(dirname)+strlen(filename)+2);
	sprintf(in->fullname, "%s/%s", dirname, filename);
	in->filename = malloc(strlen(filename)+1);
	sprintf(in->filename, "%s", filename);
	in->thumbnail_filename = malloc(1024);
	sprintf(in->thumbnail_filename, "%s/%s.tmp", tmpdir, filename);
	in->original_width = 0;
	in->original_height = 0;
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	in->data_thumbnail_size = 100*100*4;
	in->data_thumbnail = malloc(in->data_thumbnail_size);
	vpicJPGLoad(in);

	rootImageList.last_image = in;
	++rootImageList.image_total;

	vpicThumbnailCreateJPG(in);
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->thumbnail->data, 100, 100, 32, in->thumbnail->x_row_bytes);
	XInitImage(in->ximage);

	if (debug)
		printf("## vpicImageAddJPG(): end\n");
}

void vpicImageAddPNG(char *dirname, char *filename) {
	if (debug)
		printf("\n## vpicImageAddPNG(): processing %s\n", filename);
	
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_PNG;
	if (rootImageList.first_image == NULL) {
		rootImageList.first_image = in;
		in->prev = NULL;
	}
	else {
		rootImageList.last_image->next = in;
		in->prev = rootImageList.last_image;
	}
	in->next = NULL;
	in->fullname = malloc(strlen(dirname)+strlen(filename)+2);
	sprintf(in->fullname, "%s/%s", dirname, filename);
	in->filename = malloc(strlen(filename)+1);
	sprintf(in->filename, "%s", filename);
	in->thumbnail_filename = malloc(1024);
	sprintf(in->thumbnail_filename, "%s/%s.tmp", tmpdir, filename);
	in->data_thumbnail_size = 100*100*4;
	in->data_thumbnail = malloc(in->data_thumbnail_size);
	
	vpicPNGLoad(in);
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, 100, 100, 32, in->xrow_bytes);
	XInitImage(in->ximage);
	
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	
	rootImageList.last_image = in;
	++rootImageList.image_total;

	vpicThumbnailCreatePNG(in);
//	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
//					in->thumbnail->data, 100, 100, 32, in->thumbnail->x_row_bytes);
//	XInitImage(in->ximage);

	if (debug)
		printf("## vpicImageAddPNG(): end\n");
}

void vpicImageAddDirectory(char *dirname, char *filename) {
	if (debug)
		printf("\n## vpicImageAddDirectory(): processing %s\n", filename);
	
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_DIRECTORY;
	if (rootImageList.first_image == NULL) {
		rootImageList.first_image = in;
		in->prev = NULL;
	}
	else {
		rootImageList.last_image->next = in;
		in->prev = rootImageList.last_image;
	}
	in->next = NULL;
	in->fullname = malloc(strlen(dirname)+strlen(filename)+2);
	sprintf(in->fullname, "%s/%s", dirname, filename);
	in->filename = malloc(strlen(filename)+1);
	sprintf(in->filename, "%s", filename);
	in->original_width = 0;
	in->original_height = 0;
	struct stat st;
	stat(dirname, &st);
	in->file_size = st.st_size;
	in->data_size = 100*100*4;
	in->data = malloc(in->data_size);

	struct ImageNode *in2 = malloc(sizeof(struct ImageNode));
	in2->fullname = malloc(1024);
	sprintf(in2->fullname, "images/folder.png");
	in2->filename = malloc(1024);
	sprintf(in2->filename, "folder.png");
	in2->data_size = 100*100*4;
	in2->data = malloc(in2->data_size);
	vpicPNGLoad(in2);
	snprintf(in->data, in2->data_size, "%s", in2->data);
	in->row_bytes = in2->row_bytes;
	in->xrow_bytes = 100*4;
	free(in2->fullname);
	free(in2->data);
	free(in2);
	
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, 100, 100, 32, in->xrow_bytes);
	XInitImage(in->ximage);

	rootImageList.last_image = in;
	++rootImageList.image_total;

	vpicThumbnailCreateDirectory(in);

	if (debug)
		printf("## vpicImageAddDirectory(): end\n");
}

void vpicImageAddUnsupported(char *dirname, char *filename) {
	if (debug)
		printf("\n## vpicImageAddUnsupported(): processing %s\n", filename);
	
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_UNSUPPORTED;
	if (rootImageList.first_image == NULL) {
		rootImageList.first_image = in;
		in->prev = NULL;
	}
	else {
		rootImageList.last_image->next = in;
		in->prev = rootImageList.last_image;
	}
	in->next = NULL;
	in->fullname = malloc(strlen(dirname)+strlen(filename)+2);
	sprintf(in->fullname, "%s/%s", dirname, filename);
	in->filename = malloc(strlen(filename)+1);
	sprintf(in->filename, "%s", filename);
	in->original_width = 0;
	in->original_height = 0;
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	in->data_size = 100*100*4;
	in->data = malloc(in->data_size);
	in->row_bytes = 100*3;
	in->xrow_bytes = 100*4;

	int cnt;
	for (cnt = 0; cnt < in->data_size; cnt++)
		in->data[cnt] = rand()%255;
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, 100, 100, 32, in->xrow_bytes);
	XInitImage(in->ximage);

	rootImageList.last_image = in;
	++rootImageList.image_total;

	vpicThumbnailCreateUnsupported(in);

	if (debug)
		printf("## vpicImageAddUnsupported(): end\n");
}

