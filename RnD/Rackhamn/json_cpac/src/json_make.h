#ifndef JSON_WRITE_H
#define JSON_WRITE_H

#include <stdio.h>
#include <string.h>
#include "json_cpac.h"
#include "json_dump.h"
#include <../../arena/arena.h>


json_value_t * json_make_null(arena_t * arena) {
	json_value_t * value = arena_alloc(arena, sizeof(json_value_t));
	
	if(value == NULL) {
		return NULL;
	}

	value->type = JSON_TOKEN_NULL;
	
	return value;
}

#define json_make_true(arena) json_make_bool((arena), 1)
#define json_make_false(arena) json_make_bool((arena), 0)
json_value_t * json_make_bool(arena_t * arena, int bvalue) {
	json_value_t * value = arena_alloc(arena, sizeof(json_value_t));

	if(value == NULL) {
		return NULL;
	}

	value->type = JSON_TOKEN_BOOL;
	value->boolean = bvalue;

	return value;
}




int test_json_write() {
	arena_t arena;

	arena_create(&arena, 4096 * 4);

	
	json_value_t * obj = json_make_object(&arena);
	json_object_add(&arena, obj, "name", json_make_string(&arena, "Sailor"));
	json_object_add(&arena, obj, "age", json_make_number(&arena, 42));

	josn_value_t * array =  json_make_array(&arena);
	json_array_append(&arena, array, json_make_string(&arena, "one"));
	json_array_append(&arena, array, json_make_string(&arena, "two"));
	
	json_object_add(&arena, obj, "list", arr);

	json_result_t result = {
		.root = obj,
		.err = NULL
	};
	
	json_dump(result);
	
	arena_destroy(&arena);

	return 0;
}

#endif /* JSON_WRITE_H */
