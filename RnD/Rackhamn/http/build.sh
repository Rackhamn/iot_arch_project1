#!/bin/sh

echo "build.sh!"

ARENA_DIR="../../arena/"
ARENA_C="../arena/arena.c"

JSON_DIR="../../json/"
JSON_C="../../json/json.c /../json/json_make.c"

SRC_DIR="src/"
OUT_DIR="build/"

# rmdir $OUT_DIR
mkdir $OUT_DIR

SRC_FILES=""
INC_FILES=""

# hashmap - 4x num of elements (pref pow2), not bucketed, large buffer only

gcc src/watchdog.c -o watchdog
gcc src/http.c src/favicon.c src/sha256.c $ARENA_C -o http -DPORT=8082 -lc -lpthread -lsqlite3
