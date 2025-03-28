#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../../arena/arena.h"
#include "../src/string8.h"
#include "../src/json_cpac.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE	4096
#endif

uint8_t file_buffer[PAGE_SIZE] = { 0 };
size_t file_buffer_size = 0;

#define FILE_PATH "../resources/syntax_example.json"

int main() {
	
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

	return 0;
}
