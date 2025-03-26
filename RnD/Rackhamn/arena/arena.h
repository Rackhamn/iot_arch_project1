#ifndef ARENA_H
#define ARENA_H

// compile with -lc (libc)
#include <stdint.h>

typedef struct thread_context_t {
	uint32_t t_id;	
};

typedef struct arena_t {
	uint8_t * data;
	size_t offset;
	size_t size;
};

int arena_create(arena_t * arena, size_t size);
void arena_clear(arena_t * arena);
void arena_zero(arena_t * arena);
void arena_destroy(arena_t * arena);
void * arena_alloc(arena_t * arena, size_t size);

#endif /* ARENA_H */
