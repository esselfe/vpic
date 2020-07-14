#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <jpeglib.h>
#include <setjmp.h>

#include "vpic.h"

void my_error_exit(j_common_ptr cinfo) {
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr)cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

void vpicJPGLoad(struct ImageNode *in) {
	if (debug)
		printf("## vpicJPGLoad(): loading %s\n", in->filename);
	
	JSAMPLE **buffer;
	struct my_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;
	FILE *fp = fopen(in->fullname, "rb");
	if (fp == NULL) {
		fprintf(stderr, "vpic error: cannot open %s: %s\n", 
			in->fullname, strerror(errno));
		in->original_width = 100;
		in->original_height = 100;
		in->row_bytes = 100*3;
		in->xrow_bytes = 100*4;
		in->data_size = 100*100*4;
		in->data = malloc(in->data_size);
		int cnt;
		for (cnt = 0; cnt < in->data_size; cnt++)
			in->data[cnt] = rand()%255;
		return;
	}
	
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(fp);
		return;
	}
	
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	in->row_bytes = cinfo.output_width * cinfo.output_components;
	in->xrow_bytes = 100*4;
	buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, in->row_bytes, 1);
	if (verbose) {
		printf("	row bytes: %d\n", in->row_bytes);
		printf("	components: %u\n", cinfo.output_components);
	}
	
	in->data_size = cinfo.output_width * cinfo.output_height * 4;
	in->data = malloc(in->data_size);
	
	unsigned int x, cnt = 0;
	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);
		for (x=0; x < in->row_bytes; x+=3, cnt+=4) {
			in->data[cnt] = (char)buffer[0][x+2];
			in->data[cnt+1] = (char)buffer[0][x+1];
			in->data[cnt+2] = (char)buffer[0][x];
			in->data[cnt+3] = 255;
		}
	}
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(fp);

	if (debug)
		printf("## vpicJPGLoad(): end\n");
}

