cmake_minimum_required(VERSION 3.7)
project(crone VERSION 1.0.0 DESCRIPTION "main audio DSP process for norns")


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set (SRC src/main.cpp src/JackClient.cpp src/AudioMain.cpp src/Utilities.h )

add_executable(crone ${SRC})

#find_library(lo_lib liblo.so REQUIRED)
#target_link_libraries(crone ${lo_lib}))
target_link_libraries(crone lo)
target_link_libraries(crone jack)
target_link_libraries(crone pthread)
target_link_libraries(crone asound)

set_target_properties(crone PROPERTIES
CXX_STANDARD 14
CXX_STANDARD_REQUIRED YES
CXX_EXTENSIONS YES
)

target_compile_options(crone PRIVATE -Wall -Wextra -pedantic)