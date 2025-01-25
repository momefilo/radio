file(GLOB FILES *.cpp *.h)
add_library(SI4703 ${FILES})
target_link_libraries(SI4703 pico_stdlib hardware_i2c)
target_include_directories(SI4703 PUBLIC ../)
