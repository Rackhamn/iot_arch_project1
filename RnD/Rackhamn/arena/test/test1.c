#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../arena.h"

#define PAGE_SIZE	4096

int main() {
	printf("Test: Arena 1\n");
	printf("BEGIN\n");

	arena_t arena;

	arena_init(&arena, PAGE_SIZE);

	printf("arena size: %lu, expect: %lu\n", arena.size, PAGE_SIZE);

	const char * cstring = "Hello World!";
	const size_t cstring_len = strlen(cstring);
	char * string = arena_alloc(&arena, cstring_len);

	for(int i = 0; i < cstring_len; i++) {
		string[i] = cstring[i];
	}

	printf("cstring: \"%s\", len: %lu\n", cstring, cstring_len);
	printf("string : \"%s\", len: %lu\n", string, strlen(string));

	arena_clear(&arena);

	arena_destroy(&arena);

	printf("END\n");
}
