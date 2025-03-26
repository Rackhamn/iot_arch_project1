#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef struct thread_context_t {
	uint32_t t_id;	
};

typedef struct arena_t {
	uint8_t * data;
	size_t offset;
	size_t size;
};

int create_arena(arena_t * arena, size_t size);
void clear_arena(arena_t * arena);
void desotry_arena(arena_t * arena);
void * arena_alloc(arena_t * arena, size_t size);

#endif /* ARENA_H */
