cmake_minimum_required(VERSION 3.28)

project(LEXER_FM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_executable(lexer_fm main.cpp)

target_sources(lexer_fm
    PUBLIC FILE_SET CXX_MODULES
    FILES token.cppm
)
target_compile_options(lexer_fm PRIVATE -std=c++2c -stdlib=libc++ -Wall -Wextra -Wpedantic)
target_link_options(lexer_fm PRIVATE -stdlib=libc++)