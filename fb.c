#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "vpic.h"

void vpicFBDraw(void) {
	FILE *fw = fopen("/dev/fb0", "w");
	if (fw == NULL) {
		fprintf(stderr, "vpic error: Cannot open /dev/fb0: %s\n", strerror(errno));
		exit(1);
	}
	if (verbose)
		printf("opened /dev/fb0\n");

	char buffer[3];
	buffer[0] = 255;
	buffer[1] = 255;
	buffer[2] = 255;
	fwrite(buffer, 1, 4, fw);

	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 255;
	fwrite(buffer, 1, 4, fw);
	
	buffer[0] = 0;
	buffer[1] = 255;
	buffer[2] = 0;
	fwrite(buffer, 1, 4, fw);
	
	buffer[0] = 255;
	buffer[1] = 0;
	buffer[2] = 0;
	fwrite(buffer, 1, 4, fw);

	fclose(fw);

	sleep(3);
}

