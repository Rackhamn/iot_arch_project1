#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../../arena/arena.h"
#include "../src/string8.h"
#include "../src/json_cpac.h"
#include "../src/json_dump.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE	4096
#endif

// JSON "maximum" file size is 4MB~ in the IBM Parser btw
uint8_t file_buffer[PAGE_SIZE] = { 0 };
size_t file_buffer_size = 0;

#define FILE_PATH "../resources/syntax_example.json"

int main() {
	
	// Load JSON File
	printf("Load JSON File\n");
	FILE * fp = fopen(FILE_PATH, "rb");
	if(fp == NULL) {
		printf("Error: File '%s' Could Not Be Opened.\n", FILE_PATH);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(file_size > PAGE_SIZE) {
		printf("Error: File '%s' Too Big For Data Buffer.\n", FILE_PATH);
		fclose(fp);
		exit(1);
	}

	while(file_buffer_size < file_size) {
		size_t read = fread(file_buffer + file_buffer_size, sizeof(uint8_t), PAGE_SIZE, fp);
		if(read == 0) break;
		file_buffer_size += read;
	}

	printf("Read %lu / %lu bytes\n", file_buffer_size, file_size);

	fclose(fp);

	printf("File Data:\n%s\n", file_buffer);
	printf("\n");

	// Parse JSON File
	arena_t arena;
	
	arena_create(&arena, PAGE_SIZE * 16); // guesstimate size
	
	char * ptr = &file_buffer;
	json_result_t result;
	result = json_parse(&arena, ptr);

	// json_dump_results(result, file_buffer);
	printf("Result:\n");
	json_dump(result);

	arena_destroy(&arena);

	return 0;
}
