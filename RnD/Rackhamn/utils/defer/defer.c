#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// GCC defer like macro as seen in GO
// Jens Gustedt's Blog "Simple defer, ready to use", Jan 6 - 2025
// * https://gustedt.wordpress.com/2020/12/14/a-defer-mechanism-for-c/
// * https://gustedt.wordpress.com/2025/01/06/simple-defer-ready-to-use/
// * https://www.open-std.org/JTC1/SC22/WG14/www/docs/n3434.htm

void defer_printf(int * value) {
	printf("value: %i\n", *value);
}

#define __DEFER__(fn, vn) \
	auto void fn(int*); \
	int vn __attribute__((__cleanup__(fn))); \
	auto void fn(int*)

#define DEFER_ONE(N) __DEFER__(__DEFER_FUNC ## N, __DEFER__VAR ## N)
#define DEFER_TWO(N) DEFER_ONE(N)
#define __defer DEFER_TWO(__COUNTER__)

void my_free(void * p) {
	if(p != NULL) {
		free(p);
	}
}

void my_alloc(void) {
	char * p = malloc(sizeof(char) * 1024);
	// __defer { if(p) { free(p); } };
	__defer { my_free(p); };

	memset(p, 0, 1024);
}

void my_file(FILE * fp, char * path) {
	__defer { 
		if(fp != NULL) { 
			fclose(fp); 
			fp = NULL; 
		} 
	};

	fp = fopen(path, "r");

	char buf[256] = { 0 };
	fread(buf, 1, 256, fp);
	printf("buf: %s\n", buf);
}

int main(void) {
	if(1) {
		int val __attribute__((__cleanup__(defer_printf))) = 5;
		printf("defer\n");
	}

	if(1) {
		my_alloc();
	}

	if(1) {
		FILE * fp = NULL;

		my_file(fp, "example.txt");

		if(fp) {
			printf("BAD\n");
		}
	}

	return 0;
}
