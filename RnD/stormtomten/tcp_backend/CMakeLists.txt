cmake_minimum_required(VERSION 3.16)

project(tcp_fetch LANGUAGES C CXX ASM)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
    src/main.c
)
add_executable(${PROJECT_NAME}_test
    tests/test.c
)
target_link_libraries(${PROJECT_NAME}
    sqlite3
)
