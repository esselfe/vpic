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
#include "folder_data.h"

struct ImageList root_image_list;

int vpicImageLoadFromDirectory(char *dirname) {
	MSGF("loading %s", dirname);
	
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

	MSGF("end");
	return 0;
}

void vpicImageAddJPG(char *dirname, char *filename) {
	MSGF("processing %s/%s", dirname, filename);
	
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_JPG;
	if (root_image_list.first_image == NULL) {
		root_image_list.first_image = in;
		in->rank = 1;
		in->prev = NULL;
	}
	else {
		root_image_list.last_image->next = in;
		in->rank = root_image_list.last_image->rank + 1;
		in->prev = root_image_list.last_image;
	}
	MSGD("    rank: %u", in->rank);
	
	in->next = NULL;
	in->original_name = malloc(strlen(filename)+1);
	 sprintf(in->original_name, "%s", filename);
	in->fullname = malloc(strlen(dirname)+strlen(filename)+2);
	 sprintf(in->fullname, "%s/%s", dirname, filename);
	in->filename = malloc(strlen(filename)+1);
	 sprintf(in->filename, "%s", filename);
	in->original_width = 0;
	in->original_height = 0;
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;

	vpicJPGLoad(in);

	if (run_rgb2hdr && strcmp(in->filename, rgb2hdr_filename) == 0) {
		MSGD("saving rgb to header");
		char *varname = malloc(strlen(rgb2hdr_filename)+1);
		memset(varname, ' ', strlen(rgb2hdr_filename));
		varname[strlen(rgb2hdr_filename)] = '\0';
		unsigned int i, once = 1;
		for (i=strlen(rgb2hdr_filename); i >= 1; i--) {
			printf("%u ", i-1); fflush(stdout);
			if (rgb2hdr_filename[i-1] == '.') {
				if (once) {
					once = 0;
					varname[i-1] = '\0';
					strcat(varname, "_data");
				}
			}
			else
				varname[i-1] = rgb2hdr_filename[i-1];
		}
		MSGD("    rgb2hdr_filename: %s", rgb2hdr_filename);
		MSGD("    varname: %s", varname);
		vpicRGBtoHeader(varname, in->data_size, in->data);
	}

	vpicThumbnailCreateJPG(in);
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
		in->thumbnail->data, in->thumbnail->width, in->thumbnail->height, 
		32, in->thumbnail->x_row_bytes);
	XInitImage(in->ximage);

	root_image_list.last_image = in;
	++root_image_list.image_total;

	vpicPageLineAddImage(in);

	in->x = in->page_line->x + (in->line_rank-1) * 110;
	in->y = in->page_line->y;

	MSGF("end\n");
}

void vpicImageAddPNG(char *dirname, char *filename) {
	MSGF("processing %s/%s", dirname, filename);
	
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_PNG;
	if (root_image_list.first_image == NULL) {
		root_image_list.first_image = in;
		in->rank = 1;
		in->prev = NULL;
	}
	else {
		root_image_list.last_image->next = in;
		in->rank = root_image_list.last_image->rank + 1;
		in->prev = root_image_list.last_image;
	}
	MSGD("    rank: %u", in->rank);

	in->next = NULL;
	in->original_name = malloc(1024);
	 sprintf(in->original_name, "%s", filename);
	in->fullname = malloc(1024);
	 sprintf(in->fullname, "%s/%s", dirname, filename);
	in->filename = malloc(1024);
	 sprintf(in->filename, "%s", filename);
	
	vpicPNGtoJPG(in);

	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	
	vpicThumbnailCreateJPG(in);
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
		in->thumbnail->data, in->thumbnail->width, in->thumbnail->height,
		32, in->thumbnail->x_row_bytes);
	XInitImage(in->ximage);

	root_image_list.last_image = in;
	++root_image_list.image_total;

	vpicPageLineAddImage(in);

	in->x = in->page_line->x + (in->line_rank-1) * 110;
	in->y = in->page_line->y;

	MSGF("end\n");
}

void vpicImageAddDirectory(char *dirname, char *filename) {
	MSGF("processing %s/%s", dirname, filename);
	
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_DIRECTORY;
	if (root_image_list.first_image == NULL) {
		root_image_list.first_image = in;
		in->rank = 1;
		in->prev = NULL;
	}
	else {
		root_image_list.last_image->next = in;
		in->rank = root_image_list.last_image->rank + 1;
		in->prev = root_image_list.last_image;
	}
	MSGD("    rank: %u", in->rank);
	
	in->next = NULL;
	in->original_name = malloc(strlen(filename)+1);
	 sprintf(in->original_name, "%s", filename);
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
	snprintf(in->data, 100*100*4, "%s", folder_data);
	in->row_bytes = 0;
	in->x_row_bytes = 100*4; // 100 pixel * RGBA channels
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					(char *)folder_data, 100, 100, 32, 100*4);
	
	root_image_list.last_image = in;
	++root_image_list.image_total;

	vpicPageLineAddImage(in);

	in->x = in->page_line->x + (in->line_rank-1) * 110;
	in->y = in->page_line->y;

	MSGF("end\n");
}

void vpicImageAddUnsupported(char *dirname, char *filename) {
	MSGF("processing %s/%s", dirname, filename);
	struct ImageNode *in = malloc(sizeof(struct ImageNode));
	in->type = IMAGE_TYPE_UNSUPPORTED;
	if (root_image_list.first_image == NULL) {
		root_image_list.first_image = in;
		in->rank = 1;
		in->prev = NULL;
	}
	else {
		root_image_list.last_image->next = in;
		in->rank = root_image_list.last_image->rank + 1;
		in->prev = root_image_list.last_image;
	}
	MSGD("    rank: %u", in->rank);
	
	root_image_list.last_image = in;
	++root_image_list.image_total;
	in->next = NULL;
	in->original_name = malloc(strlen(filename)+1);
	 sprintf(in->original_name, "%s", filename);
	in->fullname = malloc(strlen(dirname)+strlen(filename)+2);
	 sprintf(in->fullname, "%s/%s", dirname, filename);
	in->filename = malloc(strlen(filename)+1);
	 sprintf(in->filename, "%s", filename);
	in->original_width = 0;
	in->original_height = 0;
	in->row_bytes = 100*3;
	in->x_row_bytes = 100*4;
	struct stat st;
	stat(filename, &st);
	in->file_size = st.st_size;
	in->data_size = 100*100*4;
	in->data = malloc(in->data_size);

	int cnt;
	for (cnt = 0; cnt < in->data_size; cnt++)
		in->data[cnt] = rand()%255;
	in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
					in->data, 100, 100, 32, in->x_row_bytes);
	XInitImage(in->ximage);

	vpicThumbnailCreateUnsupported(in);

	vpicPageLineAddImage(in);

	in->x = in->page_line->x + (in->line_rank-1) * 110;
	in->y = in->page_line->y;

	MSGF("end\n");
}

