#!/bin/sh

echo "build.sh!"

ARENA_DIR="../../arena/"
ARENA_C="../../arena/arena.c"

SRC_DIR="src/"
OUT_DIR="build/"

# rmdir $OUT_DIR
mkdir $OUT_DIR

SRC_FILES=""
INC_FILES=""

# hashmap - 4x num of elements (pref pow2), not bucketed, large buffer only

gcc src/watchdog.c -o watchdog
gcc src/http.c -o http -DPORT=8082 -lpthread -lc
