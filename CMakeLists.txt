cmake_minimum_required(VERSION 3.28)
project(FSM_lex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_library(fsm)
target_sources(fsm
    PUBLIC FILE_SET CXX_MODULES
    FILES 
        src/token.cppm
        src/lexer.cppm
        src/fsm/regex.cppm
        src/fsm/nfa.cppm
        src/fsm/dfa.cppm
        src/fsm/parallel.cppm
)

# C++26
target_compile_options(fsm
    PUBLIC  -stdlib=libc++ -pthread
    PRIVATE -Wall -Wextra -Wpedantic
)
target_link_options(fsm PUBLIC -stdlib=libc++ -pthread)

add_executable(lexer_fm src/main.cpp)
target_link_libraries(lexer_fm PRIVATE fsm)

enable_testing()
add_subdirectory(tests)