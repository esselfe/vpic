#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Note that the data will be in RGBA format
void vpicRGBtoHeader(char *varname, unsigned int size, char *data) {
	char *hname = malloc(strlen(varname)+5);
	sprintf(hname, "%s.h", varname);
	FILE *fp = fopen(hname, "w");
	if (fp == NULL) {
		printf("vpic error: cannot open %s: %s\n", hname, strerror(errno));
		return;
	}

	fprintf(fp, "int %s[%u] = {\n\t", varname, size);

	unsigned int cnt;
	for (cnt=0; cnt < size; cnt++) {
		if (cnt == size-1) {
			fprintf(fp, "%3.d };\n", (int)data[cnt]);
			break;
		}
		else
			fprintf(fp, "%3.d,", (int)data[cnt]);

		if (((cnt+1) % 16) == 0)
			fprintf(fp, "\n\t");
	}

	fclose(fp);
}

