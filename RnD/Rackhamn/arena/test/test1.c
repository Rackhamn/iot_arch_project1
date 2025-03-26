#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../arena.h"

#define PAGE_SIZE	4096

int main() {
	printf("Test: Arena 1\n");
	printf("BEGIN\n");

	arena_t arena;

	arena_create(&arena, PAGE_SIZE);

	printf("arena size: %lu, expect: %lu\n", arena.size, (size_t)PAGE_SIZE);

	{
		const char * cstring = "Hello World!";
		const size_t cstring_len = strlen(cstring);
		char * string = arena_alloc(&arena, cstring_len + 1);

		// make sure to copy the '\0'
		for(int i = 0; i <= cstring_len; i++) {
			string[i] = cstring[i];
		}

		printf("cstring: \"%s\", len: %lu\n", cstring, cstring_len);
		printf("string : \"%s\", len: %lu\n", string, strlen(string));
	}

	arena_clear(&arena);

	{
		size_t size = sizeof(char) * 128;
		void * data = arena_alloc(&arena, size);

		printf("arena offset: %lu, alloc size: %lu\n", 
			arena.offset, size);

		size_t size2 = sizeof(char) * 128;
		void * data2 = arena_alloc(&arena, size2);

		printf("arena offset: %lu, alloc size: %lu\n", 
			arena.offset, size2);

	}

	arena_destroy(&arena);

	printf("END\n");
}
