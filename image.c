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
	if (verbose)
		printf("vpicImageLoadFromDirectory(): Loading %s\n", dirname);
	
	DIR *dir = opendir(dirname);
	if (dir == NULL) {
		fprintf(stderr, "vpic error: Cannot open %s: %s\n", dirname, strerror(errno));
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
			fprintf(stderr, "vpic error: Cannot open entry #%u from %s: %s\n", 
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
	return 0;
}

void vpicImageAddJPG(char *dirname, char *filename) {
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
	in->preview_width = 100;
	in->preview_height = 100;
	in->original_width = 0;
	in->original_height = 0;
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	in->data_size = 100*100*4;
	in->data = malloc(in->data_size);
	vpicJPGLoad(in);
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, in->preview_width, in->preview_height, 32, 400);
	XInitImage(in->ximage);

	rootImageList.last_image = in;
	++rootImageList.image_total;
	return;
}

void vpicImageAddPNG(char *dirname, char *filename) {
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
	in->preview_width = 100;
	in->preview_height = 100;
	
	vpicPNGLoad(in);
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, in->preview_width, in->preview_height, 32, 400);
	XInitImage(in->ximage);
	
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	
	rootImageList.last_image = in;
	++rootImageList.image_total;
}

void vpicImageAddDirectory(char *dirname, char *filename) {
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
	in->preview_width = 100;
	in->preview_height = 100;
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
	free(in2->fullname);
	free(in2->data);
	free(in2);
	
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, in->preview_width, in->preview_height, 32, 400);
	XInitImage(in->ximage);

	rootImageList.last_image = in;
	++rootImageList.image_total;
	return;
}

void vpicImageAddUnsupported(char *dirname, char *filename) {
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
	in->preview_width = 100;
	in->preview_height = 100;
	in->original_width = 0;
	in->original_height = 0;
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	in->data_size = 100*100*4;
	in->data = malloc(in->data_size);
	int cnt;
	for (cnt = 0; cnt < in->data_size; cnt++)
		in->data[cnt] = rand()%255;
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, in->preview_width, in->preview_height, 32, 400);
	XInitImage(in->ximage);

	rootImageList.last_image = in;
	++rootImageList.image_total;
}



























