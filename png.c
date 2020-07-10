#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <png.h>

#include "vpic.h"

void vpicPNGLoad(struct ImageNode *in) {
	printf("\nLoading: %s\n", in->filename);
	FILE *fp = fopen(in->fullname, "rb");
	if (fp == NULL) {
		fprintf(stderr, "vpic error: Cannot open %s: %s\n", in->filename, strerror(errno));
		in->original_width = 100;
		in->original_height = 100;
		in->data_size = 100*100*4;
		in->data = malloc(in->data_size);
		int cnt;
		for (cnt = 0; cnt < in->data_size; cnt++)
			in->data[cnt] = rand()%255;
		in->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
						in->data, in->preview_width, in->preview_height, 32, 400);
		XInitImage(in->ximage);
		return;
	}
	fseek(fp, 0, SEEK_END);
	printf("file size: %lu\n", ftell(fp));
	fseek(fp, 0, SEEK_SET);
	
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
	
	unsigned int bit_depth = png_get_bit_depth(png, info);
	printf("bit_depth: %u\n", bit_depth);

	unsigned int components = 3, color_type = png_get_color_type(png, info);
	switch(color_type) {
	case PNG_COLOR_TYPE_PALETTE:
		printf("color type: palette\n");
		components = 3;
		break;
	case PNG_COLOR_TYPE_RGB:
		printf("color type: RGB\n");
		components = 3;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		printf("color type: RGBA\n");
		components = 4;
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		printf("color type: gray alpha\n");
		components = 2;
		break;
	default:
		components = 3;
		break;
	}
	printf("components: %u\n", components);

	in->original_width = png_get_image_width(png, info);
	in->original_height = png_get_image_height(png, info);
	printf("width: %u height: %u\n", in->original_width, in->original_height);
	printf("image size: %u\n", in->original_width * in->original_height * components);
	in->data_size = in->original_width * in->original_height * 4;
	printf("data size: %u\n", in->data_size);
	unsigned int rowbytes = png_get_rowbytes(png, info);
	printf("row bytes: %u\n", rowbytes);
	in->data = malloc(in->data_size);
	memset(in->data, 0, in->data_size);

	png_bytepp rows = png_get_rows(png, info);
	int x, y, cnt = 0;
	for (y=0; y<in->original_height; y++) {
		png_bytep row = rows[y];
		if (components == 3) {
			for (x=0; x<rowbytes; x+=components, cnt+=4) {
				in->data[cnt] = row[x+2];
				in->data[cnt+1] = row[x+1];
				in->data[cnt+2] = row[x];
				in->data[cnt+3] = 255;
			}
		}
		else if (components == 4) {
			for (x=0; x<rowbytes; x+=components, cnt+=4) {
				in->data[cnt] = row[x+2];
				in->data[cnt+1] = row[x+1];
				in->data[cnt+2] = row[x];
				in->data[cnt+3] = row[x+3];
			}
		}
	}
	
	png_destroy_info_struct(png, &info);
	png_destroy_read_struct(&png, NULL, NULL);
}

