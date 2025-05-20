cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico_w)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_subdirectory(src)
pico_enable_stdio_usb(test 1)
pico_enable_stdio_uart(test 1)
target_link_libraries(test pico_cyw43_arch_lwip_threadsafe_background pico_stdlib
pico_lwip
        hardware_spi
        hardware_gpio)
