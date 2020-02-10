cmake_minimum_required(VERSION 3.7)
project(maiden-repl VERSION 1.0.0)

set (CMAKE_CC_FLAGS "${CMAKE_CC_FLAGS} -Wall")
set (CMAKE_CC_FLAGS "${CMAKE_CC_FLAGS_DEBUG} -O0")
set (CMAKE_CC_FLAGS "${CMAKE_CC_FLAGS_RELEASE} -O2")

set (SRC src/main.c
         src/io.c
         src/page.c
         src/pages.c
         src/ui.c)

add_executable(maiden-repl ${SRC})

if(UNIX)
    if(APPLE)
        find_library(pthread libpthread.dylib REQUIRED)
        find_library(readline libreadline.dylib
            NO_DEFAULT_PATH
            HINTS
                /usr/local/opt/readline/lib
            REQUIRED)
        find_library(nanomsg libnanomsg.dylib REQUIRED)
        find_library(ncurses libcurses.dylib REQUIRED)
        find_library(panel libpanel.dylib REQUIRED)
        target_link_libraries(maiden-repl
            ${pthread}
            ${readline}
            ${ncurses}
            ${panel}
            ${nanomsg})
        include_directories(/usr/local/include /usr/local/opt/readline/include)
    else()
        target_link_libraries(maiden-repl pthread)
        target_link_libraries(maiden-repl readline)
        target_link_libraries(maiden-repl ncursesw)
        target_link_libraries(maiden-repl panel)
        target_link_libraries(maiden-repl nanomsg)
    endif()
else()
    # nope
endif()

target_compile_options(maiden-repl PRIVATE -Wall -Wextra -Werror -pedantic)
