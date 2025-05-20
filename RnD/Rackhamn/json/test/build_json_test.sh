ARENA_C="../../arena/arena.c"

gcc json_test.c $ARENA_C -o json_test
gcc json_make_test.c $ARENA_C -o json_make_test
gcc json_write_test.c $ARENA_C -o json_write_test
gcc json_json_test.c $ARENA_C -o json_json_test
gcc json_find_test.c $ARENA_C -o json_find_test
