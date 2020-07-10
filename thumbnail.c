#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <png.h>
#include <MagickWand/MagickWand.h>

#include "vpic.h"

void vpicThumbnailGenerate(char *src, char *dst) {
	if (debug)
		printf("## vpicThumbnailGenerate() start: src: %s dst: %s\n", src, dst);

// try with the CLI tool instead
char *cmd = malloc(1024);
sprintf(cmd, "convert -colorspace RGB -resize 100x100 %s %s", src, dst);
system(cmd);
return;

	MagickBooleanType status;
	MagickWandGenesis();
	MagickWand *wand = NewMagickWand();
	//MagickSetImageColorspace(wand, sRGBColorspace);
	//MagickSetColorspace(wand, sRGBColorspace);
	
	status = MagickReadImage(wand, src);
	if (status == MagickFalse) {
		fprintf(stderr, "MagickReadImage() returned false\n");
		return;
	}

	MagickResetIterator(wand);
	while (MagickNextImage(wand) != MagickFalse)
		MagickResizeImage(wand, 100, 100, LanczosFilter);

	status = MagickWriteImages(wand, dst, MagickTrue);
	if (status == MagickFalse) {
		fprintf(stderr, "MagickWriteImages() returned false\n");
		return;
	}

	wand = DestroyMagickWand(wand);
	MagickWandTerminus();

	if (debug)
		printf("## vpicThumbnailGenerate() end\n");
}

void vpicThumbnailCreateJPG(struct ImageNode *in) {
	if (debug)
		printf("## vpicThumbnailCreateJPG(): processing %s\n", in->filename);
	
	struct Thumbnail *tn = malloc(sizeof(struct Thumbnail));
	in->thumbnail = tn;

	tn->filename = malloc(strlen(in->filename)+4);
	sprintf(tn->filename, "%s.tmp", in->filename);
	tn->fullname = malloc(strlen(tmpdir)+1+strlen(tn->filename));
	sprintf(tn->fullname, "%s/%s", tmpdir, tn->filename);

	vpicThumbnailGenerate(in->fullname, tn->fullname);

	// Start reading the new thumbnail
	//////////////////////////////////

	FILE *fp = fopen(tn->fullname, "rb");
    if (fp == NULL) {
        fprintf(stderr, "vpic error: Cannot open %s: %s\n",
            tn->fullname, strerror(errno));
		tn->ratio = 1.0;
        tn->width = 100;
        tn->height = 100;
        tn->row_bytes = 100*3;
        tn->x_row_bytes = 100*4;
		struct stat st;
		stat(tn->fullname, &st);
		tn->file_size = st.st_size;
        tn->data_size = 100*100*4;
        tn->data = malloc(tn->data_size);
        int cnt;
        for (cnt = 0; cnt < tn->data_size; cnt++)
            tn->data[cnt] = rand()%255;
        return;
    }

    struct my_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return;
    }

	JSAMPLE **buffer;
	jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
	tn->components = cinfo.output_components;
    tn->row_bytes = cinfo.output_width * tn->components;
    tn->x_row_bytes = 100*4;
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, tn->row_bytes, 1);
    tn->data_size = cinfo.output_width * cinfo.output_height * 4;
    tn->data = malloc(tn->data_size);
	memset(tn->data, 0, tn->data_size);
    if (debug) {
        printf("##    row_bytes: %d\n", tn->row_bytes);
        printf("##    x_row_bytes: %d\n", tn->row_bytes);
        printf("##    components: %u\n", tn->components);
		printf("##    data_size: %u\n", tn->data_size);
    }

    unsigned int x, cnt = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        for (x=0; x < tn->row_bytes; x += tn->components, cnt+=4) {
            tn->data[cnt] = (char)buffer[0][x];
            tn->data[cnt+1] = (char)buffer[0][x+1];
            tn->data[cnt+2] = (char)buffer[0][x+2];
            tn->data[cnt+3] = 255;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

	if (debug)
		printf("## vpicThumbnailCreateJPG(): end\n");
}

void vpicThumbnailCreatePNG(struct ImageNode *in) {
	if (debug)
		printf("## vpicThumbnailCreatePNG(): processing %s\n", in->filename);

	struct Thumbnail *tn = malloc(sizeof(struct Thumbnail));
	in->thumbnail = tn;

	tn->filename = malloc(strlen(in->filename)+4);
    sprintf(tn->filename, "%s.tmp", in->filename);
    tn->fullname = malloc(strlen(tmpdir)+1+strlen(tn->filename));
    sprintf(tn->fullname, "%s/%s", tmpdir, tn->filename);

    vpicThumbnailGenerate(in->fullname, tn->fullname);
	
	FILE *fp = fopen(tn->fullname, "rb");
	if (fp == NULL) {
		fprintf(stderr, "vpic error: Cannot open %s: %s\n",
            tn->fullname, strerror(errno));
        tn->ratio = 1.0;
        tn->width = 100;
        tn->height = 100;
        tn->row_bytes = 100*3;
        tn->x_row_bytes = 100*4;
        struct stat st;
        stat(tn->fullname, &st);
        tn->file_size = st.st_size;
        tn->data_size = 100*100*4;
        tn->data = malloc(tn->data_size);
        int cnt; 
        for (cnt = 0; cnt < tn->data_size; cnt++)
            tn->data[cnt] = rand()%255;
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

	unsigned int color_type = png_get_color_type(png, info);
	png_sPLT_tp palettes = NULL;
	switch(color_type) {
	case PNG_COLOR_TYPE_PALETTE:
		if (verbose)
			printf("    color type: palette\n");

		tn->components = 3;
		printf("    palette max: %d\n", png_get_palette_max(png, info));
		printf("    png_get_sPLT(): %d\n", png_get_sPLT(png, info, &palettes));
		//printf("    palette name: %s\n", palettes->name);
		break;
	case PNG_COLOR_TYPE_RGB:
		tn->components = 3;
		if (verbose)
			printf("    color type: RGB\n");
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		tn->components = 4;
		if (verbose)
			printf("    color type: RGBA\n");
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		tn->components = 2;
		if (verbose)
			printf("    color type: gray alpha\n");
		break;
	default:
		tn->components = 3;
		break;
	}

	tn->ratio = 1.0;
	tn->width = png_get_image_width(png, info);
	tn->height = png_get_image_height(png, info);
	tn->row_bytes = png_get_rowbytes(png, info) * tn->components;
	tn->x_row_bytes = 100*4;
	struct stat st;
	stat(tn->fullname, &st);
	tn->file_size = st.st_size;
	tn->data_size = tn->width * tn->height * 4;
	tn->data = malloc(tn->data_size);
	memset(tn->data, 0, tn->data_size);
	if (debug) {
		unsigned int bit_depth = png_get_bit_depth(png, info);
		printf("    bit_depth: %u\n", bit_depth);
		printf("    width: %u height: %u\n", tn->width, tn->height);
		printf("    components: %u\n", tn->components);
		printf("    row bytes: %u\n", tn->row_bytes);
		printf("    xrow bytes: %u\n", tn->x_row_bytes);
		printf("    image size: %u\n", tn->width * tn->height * tn->components);
		printf("    file size: %u\n", tn->file_size);
		printf("    data size: %u\n", tn->data_size);
	}

	png_bytepp rows = png_get_rows(png, info);
	int x, y, cnt = 0;
	for (y=0; y < tn->height; y++) {
		png_bytep row = rows[y];
		if (tn->components == 3) {
			for (x=0; x < tn->row_bytes; x += tn->components, cnt+=4) {
				tn->data[cnt] = row[x+2];
				tn->data[cnt+1] = row[x+1];
				tn->data[cnt+2] = row[x];
				tn->data[cnt+3] = 255;
			}
		}
		else if (tn->components == 4) {
			for (x=0; x < tn->row_bytes; x += tn->components, cnt+=4) {
				tn->data[cnt] = row[x+2];
				tn->data[cnt+1] = row[x+1];
				tn->data[cnt+2] = row[x];
				tn->data[cnt+3] = row[x+3];
			}
		}
	}
	
	png_destroy_info_struct(png, &info);
	png_destroy_read_struct(&png, NULL, NULL);

	if (debug)
		printf("## vpicThumbnailCreatePNG(): end\n");
}

void vpicThumbnailCreateDirectory(struct ImageNode *in) {

}

void vpicThumbnailCreateUnsupported(struct ImageNode *in) {

}


