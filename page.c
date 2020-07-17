#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "vpic.h"

struct Page page;

struct PageLine *vpicPageAddLine(void) {
	MSGF("start");
	struct PageLine *pl = malloc(sizeof(struct PageLine));
	if (page.first_line == NULL) {
		page.first_line = pl;
		pl->prev = NULL;
		pl->rank = 1;
	}
	else {
		page.last_line->next = pl;
		pl->prev = page.last_line;
		pl->rank = pl->prev->rank + 1;
	}
	pl->x = 10;
	pl->y = (pl->rank-1) * 120 + 20;
	pl->first_image = NULL;
	pl->last_image = NULL;
	page.last_line = pl;
	page.images_per_line = winW/110;
	++page.total_lines;
	
	MSGD("    total_lines: %u", page.total_lines);
	MSGF("end");
	return pl;
}

void vpicPageLineAddImage(struct ImageNode *in) {
	MSGF("adding %s", in->original_name);
	struct PageLine *pl;
	if (page.first_line == NULL || (in->rank > 0 && 
		((in->rank-1) % (page.images_per_line+1)) == 0))
		pl = vpicPageAddLine();
	else
		pl = page.last_line;
	
	if (pl->first_image == NULL) {
		pl->first_image = in;
		in->line_rank = 1;
	}
	else
		in->line_rank = pl->last_image->line_rank + 1;
	
	MSGD("    images per line: %u", page.images_per_line);
	MSGD("    line_rank: %u", in->line_rank);
	
	pl->last_image = in;
	in->page_line = pl;
	
	++page.total_images;

	MSGF("end");
}

