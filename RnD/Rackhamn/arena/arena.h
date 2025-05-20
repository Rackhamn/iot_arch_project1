#ifndef ARENA_H
#define ARENA_H

// compile with -lc (libc)
#include <stdint.h>

struct thread_context_s {
	uint32_t t_id;	
};
typedef struct thread_context_s thread_context_t;

struct arena_s {
	uint8_t * data;
	size_t offset;
	size_t size;
};
typedef struct arena_s arena_t;

int arena_create(arena_t * arena, size_t size);
void arena_clear(arena_t * arena);
void arena_zero(arena_t * arena);
void arena_destroy(arena_t * arena);
void * arena_alloc(arena_t * arena, size_t size);

#endif /* ARENA_H */
