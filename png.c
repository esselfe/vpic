#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <png.h>

#include "vpic.h"

void vpicPNGLoad(struct ImageNode *in) {
	MSGF("loading %s", in->filename);
	
	FILE *fp = fopen(in->fullname, "rb");
	if (fp == NULL) {
		fprintf(stderr, "vpic error: cannot open %s: %s\n", in->filename, strerror(errno));
		in->original_width = 100;
		in->original_height = 100;
		in->row_bytes = 100*3;
		in->x_row_bytes = 100*4;
		in->data_size = 100*100*4;
		in->data = malloc(in->data_size);
		int cnt;
		for (cnt = 0; cnt < in->data_size; cnt++)
			in->data[cnt] = rand()%255;
		return;
	}
	
	unsigned char sig[8];
	fread(sig, 1, 8, fp);
	if (!png_check_sig(sig, 8))
		printf("vpic error: invalid PNG signature: %s\n", in->filename);

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		return;
	}
	png_set_sig_bytes(png, 8);
	png_init_io(png, fp);
	png_read_png(png, info, 0, 0);

	unsigned int components = 3, color_type = png_get_color_type(png, info);
	switch(color_type) {
	case PNG_COLOR_TYPE_GRAY:
		components = 1;
		MSGD("    color type: gray");
	case PNG_COLOR_TYPE_PALETTE:
		components = 3;
		MSGD("    color type: palette");
		break;
	case PNG_COLOR_TYPE_RGB:
		components = 3;
		MSGD("    color type: RGB");
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		components = 4;
		MSGD("    color type: RGBA");
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		components = 2;
		MSGD("    color type: gray alpha");
		break;
	default:
		components = 3;
		break;
	}
	components = png_get_channels(png, info);

	in->original_width = png_get_image_width(png, info);
	in->original_height = png_get_image_height(png, info);
	in->data_size = in->original_width * in->original_height * 4;
	in->row_bytes = png_get_rowbytes(png, info);
	in->x_row_bytes = in->original_width * 4;
	in->data = malloc(in->data_size);
	memset(in->data, 0, in->data_size);
	unsigned int bit_depth = png_get_bit_depth(png, info);
	MSGD("	bit_depth: %u\n", bit_depth);
	MSGD("	components: %u\n", components);
	MSGD("	width: %u height: %u\n", in->original_width, in->original_height);
	MSGD("	image size: %u\n", in->original_width * in->original_height * components);
	MSGD("	data size: %u\n", in->data_size);
	MSGD("	row bytes: %u\n", in->row_bytes);
	MSGD("	x_row bytes: %u\n", in->x_row_bytes);

	png_bytepp rows = png_get_rows(png, info);
	int x, y, cnt = 0;
	for (y=0; y < in->original_height; y++) {
		png_bytep row = rows[y];
		if (components == 3) {
			for (x=0; x < in->row_bytes; x+=components, cnt+=4) {
				in->data[cnt] = row[x+2];
				in->data[cnt+1] = row[x+1];
				in->data[cnt+2] = row[x];
				in->data[cnt+3] = 255;
			}
		}
		else if (components == 4) {
			for (x=0; x < in->row_bytes; x+=components, cnt+=4) {
				in->data[cnt] = row[x+2];
				in->data[cnt+1] = row[x+1];
				in->data[cnt+2] = row[x];
				in->data[cnt+3] = row[x+3];
			}
		}
	}
	
	png_destroy_info_struct(png, &info);
	png_destroy_read_struct(&png, NULL, NULL);

	MSGF("end");
}

void vpicPNGtoJPG(struct ImageNode *in) {
	char *cmd = malloc(1024);
	sprintf(in->filename, "%s/%s", tmpdir, in->original_name);
	int len = strlen(in->filename);
	in->filename[len-1] = 'g';
	in->filename[len-2] = 'p';
	in->filename[len-3] = 'j';
	MSGF("src: %s dst: %s", in->fullname, in->filename);
	
	sprintf(cmd, "convert %s %s", in->fullname, in->filename);
	system(cmd);

	if (debug) {
		sprintf(cmd, "echo \"  output of 'file %s':\"; file %s",
			in->filename, in->filename);
		system(cmd);
	}

	free(cmd);

	MSGF("end");
	return;
}

