
add_executable(test main.c 
    secrets.c
    network.c
    mfrc522.c
)

pico_add_extra_outputs(test)
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)
