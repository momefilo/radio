cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(radio C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_subdirectory(./include/SH1106)
add_subdirectory(./include/SI4703)
add_executable(radio
radio.cpp
)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(radio pico_stdlib hardware_gpio hardware_flash SH1106 SI4703)

# create map/bin/hex file etc.
#pico_add_extra_outputs(oled_i2c)

# add url via pico_set_program_url
#example_auto_set_url(oled_i2c)
