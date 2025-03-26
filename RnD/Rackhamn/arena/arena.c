#include <stdlib.h>
#include "arena.h"

// this is the simplest and kind-of 'worst' arena allocator one can make.
// its still on the heap (only), not aligned, no pages and no virtual allocs.
// if you want to learn more about arenas, read this link:
// https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator

int arena_create(arena_t * arena, size_t size) {
	arena->data = malloc(size);
	if(arena->data == NULL) return ENOMEM;
	arena->offset = 0;
	arena->size = size;
}

void arena_clear(arena_t * arena) {
	arena->offset = 0;
}

void arena_zero(arena_t * arena) {
	memset(arena->data, 0, arena->size);
	arena->offset = 0;
}

void arena_destroy(arena_t * arena) {
	if(arena == NULL) return;
	if(arena->data != NULL) free(arena->data);
	arena->offset = 0;
	arena->size = 0;
}

void * arena_alloc(arena_t * arena, size_t size) {
	size_t noffset = arena->offset + size;
	if(noffset > arena->size) {
		// cant allocate more space in arena
		return NULL;
	}

	void * ptr = (void*)(arena->data + arena->offset);
	arena->offset = noffset;

	return ptr;
}
