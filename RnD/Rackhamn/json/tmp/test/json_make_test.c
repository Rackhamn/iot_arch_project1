#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../../arena/arena.h"
#include "../src/string8.h"
#include "../src/json_cpac.h"
#include "../src/json_dump.h"
#include "../src/json_make.h"

int test_json_make() {
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

        json_dump(result);

        arena_destroy(&arena);

        return 0;
}

int main() {

	test_json_make();

	return 0;
}

