#ifndef STRING8_H
#define STRING8_H

#include <stdint.h>
#include <string.h> // strlen
/*
struct string8_s {
	char * data;
	size_t size;
	size_t num_chars;
};
typedef struct string8_s string8_t;
*/

/* TODO(@Rackhamn):
 *	get num valid codepoints from ptr
 *	get num invalid codepoints from ptr
 *	search substring in ptr+size (from left and from right)
 *	compare utf8 string
 *	write utf8 string into buffer
 *	trim utf8 buffer (remove ' ' or something)
 *	concat utf8 buffers (with delimiter)
 *	split utf8 buffer (with delimiter)
 *	utf8 codepoint to upper / lower
 *	copy utf8 string to buffer
 *	get size of codepoint
 *
 *	+ auto remove emoji stuff?
 *	+ use arena plz
 *
 *	utf8_has_bom(ptr):
 *		find utf8 BOM (Byte Order Mark)
 *		BOM = 0xEF 0xBB 0xBF
 *		its optional and often dismissed.
 *
*/

// maybe: 
// 	get current codepoint and return ptr to next codepoint in buffer
// uint8_t * utf8_codepoint(uint8_t * ptr, uint32_t * codepoint);
// pointer to pointer
uint32_t decode_utf8(uint8_t ** ptr) {
	const uint8_t * p = *ptr;
	uint32_t codepoint = 0;

	if(p[0] < 0x80) {
		// ASCII
		// 1 byte utf8 codepoint
		codepoint = p[0];
		*ptr += 1;
	} else if((p[0] & 0xE0) == 0xC0) {
		// 2 byte utf8 codepoint
		codepoint = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
		*ptr += 2;
	} else if((p[0] & 0xF0) == 0xE0) {
		// 3 byte utf8 codepoint
		codepoint = ((p[0] & 0x0F) << 12) |
				((p[1] & 0x3F) << 6) |
				(p[2] & 0x3F);
		*ptr += 3;
	} else if((p[0] & 0xF8) == 0xF0) {
		// 4 byte utf8 codepoint
		codepoint = ((p[0] & 0x07) << 18) |
				((p[1] & 0x3F) << 12) |
				((p[2] & 0x3F) << 6) |
				(p[3] & 0x3F);
		*ptr += 4;
	}

	// maybe just return the current ptr?
	return codepoint;
}

void test_decode_utf8(void) {
	uint8_t * data = (uint8_t *)"hello ö 😐 world";
	uint8_t data_size = strlen(data);
	uint8_t num_codepoints = 0;

	printf("input: \"%s\"\n", data);

	printf("output:\n");
	while(*data) {
		uint32_t codepoint = decode_utf8(&data);
		printf("U+%04X\n", codepoint);
		num_codepoints++;
	}

	printf("data size: %i\n", data_size);
	printf("num codepoints: %i\n", num_codepoints);
}

#endif /* STRING8_H */
