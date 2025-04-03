#ifndef JSON_WRITE_H
#define JSON_WRITE_H

#include <stdio.h>
#include <string.h>

#include "../../arena/arena.h"
#include "json_cpac.h"
// #include "json_dump.h"

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

json_value_t * json_make_number(arena_t * arena, double number) {
	json_value_t * value = arena_alloc(arena, sizeof(json_value_t));

	if(value == NULL) {
		return NULL;
	}

	value->type = JSON_TOKEN_NUMBER;
	value->number = number;

	return value;
}

json_value_t * json_make_string(arena_t * arena, char * str) {
	size_t len = strlen(str);
	char * cpy = arena_alloc(arena, len + 1);

	if(cpy == NULL) {
		return NULL; // maybe return a "" instead
	}

	memcpy(cpy, str, len + 1);

	json_value_t * value = arena_alloc(arena, sizeof(json_value_t));

	if(value == NULL) {
		return NULL;
	}

	value->type = JSON_TOKEN_STRING;
	value->string.chars = cpy;
	value->string.length = len;

	return value;
}

json_value_t * json_make_array(arena_t * arena) {
	json_value_t * value = arena_alloc(arena, sizeof(json_value_t));

	if(value == NULL) {
		return NULL;
	}

	value->array.items = NULL;
	value->array.count = 0;

	return value;
}

json_value_t * json_make_object(arena_t * arena) {
	json_value_t * value = arena_alloc(arena, sizeof(json_value_t));
	
	if(value == NULL) {
		return NULL;
	}

	value->type = JSON_TOKEN_OBJECT;
	value->object.members = NULL;
	value->object.count = 0;

	return value;
}

int json_array_append(arena_t * arena, json_value_t * array, json_value_t * item) {
	size_t count = array->array.count;
	// dont like to copy the entire array each damn time we add an item (even if its ptrs)
	json_value_t ** new_items = arena_alloc(arena, sizeof(json_value_t *) * (count + 1));

	if(new_items == NULL) {
		return 0;
	}

	if(count > 0) {
		memcpy(new_items, array->array.items, sizeof(json_value_t *) * (count + 1));
	}

	new_items[count] = item;

	array->array.items = new_items;
	array->array.count = count + 1;

	return 1;
	
}

int json_object_add(arena_t * arena, json_value_t * object, char * key, json_value_t * value) {
	size_t count = object->object.count;
	json_member_t * new_members = arena_alloc(arena, sizeof(json_member_t) * (count + 1));

	if(new_members == NULL) {
		return 0;
	}

	if(count > 0) {
		memcpy(new_members, object->object.members, sizeof(json_member_t) * (count + 1));
	}

	size_t key_len = strlen(key);
	char * key_cpy = arena_alloc(arena, key_len + 1);

	if(key_cpy == NULL) {
		return 0;
	}

	memcpy(key_cpy, key, key_len + 1);

	new_members[count].key = key_cpy;
	new_members[count].value = value;

	object->object.members = new_members;
	object->object.count = count + 1;

	return 1;
}

#endif /* JSON_WRITE_H */
