#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "favicon.h"

void generate_favicon_data(uint8_t * buf) {
	int width = FAVICON_SIZE;
	int height = FAVICON_SIZE;

	int x, y, idx;

	// clear pixels to white
	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			idx = (y * width + x) * 3;
			buf[idx] = 255;
			buf[idx + 1] = 255;
			buf[idx + 2] = 255;
		}
	}

	// generate a blue 'J'
	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			idx = (y * width + x) * 3;

			// the J
			if((x == 5 && y >= 3) ||
			   (x >= 4 && x <= 8 && y == 13) ||
			   (x == 8 && y >= 9)) {
				// blue
				buf[idx] = 0;
				buf[idx + 1] = 0;
				buf[idx + 2] = 255;
			}
		}
	}
}

void generate_favicon(uint8_t * buf) {
	uint8_t header[6] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x01 }; // ICO fmt
	
	uint8_t image_directory[16] = {
		FAVICON_SIZE,	// width
		FAVICON_SIZE,	// height
		0x00, 		// colors in palette
		0x00, 		// reserved
		// colot planes
		0x00, 
		0x00,
		// bits per pixel
		0x00,
	       	0x18, 
		// size
		0x00, 
		0x00, 
		0x00,
		0x00,
		// offset
		0x00, 
		0x00,
		0x00,
		0x00
	};

	uint32_t dsize = FAVICON_DATA_SIZE;
	image_directory[8] = (dsize) & 0xFF;
	image_directory[9] = (dsize >> 8) & 0xFF;

	uint32_t pdo = 6 + 16; // sizeof(header) + sizeof(image_directory);
	image_directory[12] = (pdo) & 0xFF;
	image_directory[13] = (pdo >> 8) & 0xFF;

	// uint8_t pixels[FAVICON_SIZE * FAVICON_SIZE * 3];
	// generate_favicon_data(pixels);

	size_t offset = 0;
	
	memset(buf, 0, FAVICON_BUF_SIZE);

	memcpy(buf + offset, header, sizeof(header));
	offset += sizeof(header);

	memcpy(buf + offset, image_directory, sizeof(image_directory));
	offset += sizeof(image_directory);

	// memcpy(buf + offset, pixels, sizeof(pixels));
	// offset += sizeof(pixels);
	generate_favicon_data(buf + offset);
}


