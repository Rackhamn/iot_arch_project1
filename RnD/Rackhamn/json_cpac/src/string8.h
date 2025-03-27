#ifndef STRING8_H
#define STRING8_H

struct string8_s {
	char * data;
	size_t size;
	size_t num_chars;
};
typedef struct string8_s string8_t;

#endif /* STRING8_H */
