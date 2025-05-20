#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../src/json.h"

int test_json_write() {
        arena_t arena;

        arena_create(&arena, 4096 * 4);

        json_value_t * obj = json_make_object(&arena);
        json_object_add(&arena, obj, "name", json_make_string(&arena, "Sailor"));
        json_object_add(&arena, obj, "age", json_make_number(&arena, 42));

        json_value_t * array =  json_make_array(&arena);
        json_array_append(&arena, array, json_make_string(&arena, "one"));
        json_array_append(&arena, array, json_make_string(&arena, "two"));
        json_object_add(&arena, obj, "list", array);

        json_result_t result = {
                .root = obj,
                .err = NULL
        };


	char * output = NULL;
	size_t output_size = 0;

	output = json_write(&arena, result, &output_size);
	printf("output:\n");
	printf("%.*s\n", (int)output_size, output);

        arena_destroy(&arena);

        return 0;
}

int main() {

	test_json_write();

	return 0;
}

