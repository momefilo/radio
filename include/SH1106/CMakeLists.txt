file(GLOB FILES *.cpp *.h)
add_library(SH1106 ${FILES})
target_link_libraries(SH1106 pico_stdlib hardware_i2c)
target_include_directories(SH1106 PUBLIC ../)
