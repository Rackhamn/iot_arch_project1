#ifndef FAVICON_H
#define FAVICON_H

#include <stdio.h>
#include <stdint.h>

#define FAVICON_SIZE	16
#define ICON_SIZE	16

#define FAVICON_HEADER_SIZE		6
#define FAVICON_IMAGE_DIRECTORY_SIZE	16
#define FAVICON_BUFFER_SIZE (\
		ICON_SIZE + \
		FAVICON_IMAGE_DIRECTORY_SIZE + \
		(FAVICON_SIZE * FAVICON_SIZE * 3) \
		)

#define FAVICON_DATA_SIZE (FAVICON_SIZE * FAVICON_SIZE * 3)
// 16*16*3 = 768 bytes
// 6 + 16 + 768 = 790 bytes
#define FAVICON_BUF_SIZE (\
		FAVICON_HEADER_SIZE + \
		FAVICON_IMAGE_DIRECTORY_SIZE + \
		FAVICON_DATA_SIZE)

// uint8_t favicon_ico[FAVICON_BUFFER_SIZE];

void generate_favicon_data(uint8_t * buffer);
void generate_favicon(uint8_t * buffer);



#endif /* FAVICON_H */
