#ifndef ENDIAN_H
#define ENDIAN_H

int is_big_endian() {
	int i = 1;
	return !*((char *)&i);
}

int is_little_endian() {
	int i = 1;
	return *((char *)&i);
}

// TODO: add a compiler & target-platform specific const endian int (gcc, mingw, ppc, x86 etc)
#define IS_LITTLE_ENDIAN	(0)
#define IS_BIG_ENDIAN		(1)

#define get_endian() (is_little_endian() ? IS_LITTLE_ENDIAN : IS_BIG_ENDIAN)
#define get_endian_string() (is_little_endian() ? "little_endian" : "big_endian")
#define get_endian_string_short() (is_little_endian() ? "LE" : "BE")

#endif /* ENDIAN_H */
