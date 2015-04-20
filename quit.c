#include <stdio.h>
#include <stdlib.h>

void quit(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}