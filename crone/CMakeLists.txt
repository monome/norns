cmake_minimum_required(VERSION 3.7)
project(crone VERSION 1.0.0)


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set (SRC src/main.cpp
        src/BufDiskWorker.h
        src/Client.h
        src/Utilities.h
        src/OscInterface.cpp
        src/Commands.cpp
        src/Evil.h
        src/softcut/FadeCurves.cpp
        src/softcut/SoftCut.h
        src/softcut/SoftCutHead.cpp
        src/softcut/SoftCutVoice.cpp
        src/softcut/SubHead.cpp
        src/softcut/Svf.cpp
        src/Client.h
        src/MixerClient.h
        src/MixerClient.cpp
        src/SoftCutClient.cpp
        src/SoftCutClient.h
        src/Poll.h
        src/Taper.cpp
        src/Window.cpp
        src/BufDiskWorker.cpp)

add_executable(crone ${SRC})

include_directories(./faust)

if(UNIX)
    if(APPLE)
        find_library(lo_lib liblo.dylib REQUIRED)
        find_library(sndfile_lib libsndfile.a REQUIRED)
        find_library(jack_lib libjack.dylib REQUIRED)
        target_link_libraries(crone ${lo_lib} ${sndfile_lib} ${jack_lib})
        include_directories(/usr/local/include)
    else()
        target_link_libraries(crone lo)
        target_link_libraries(crone jack)
        target_link_libraries(crone pthread)
        target_link_libraries(crone asound)
        target_link_libraries(crone sndfile)
    endif()
else()
    # nope
endif()


set_target_properties(crone PROPERTIES
CXX_STANDARD 14
CXX_STANDARD_REQUIRED YES
CXX_EXTENSIONS YES
)

target_compile_options(crone PRIVATE -Wall -Wextra -pedantic)
