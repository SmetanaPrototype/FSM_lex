cmake_minimum_required(VERSION 3.28)

project(MyCpp26Project LANGUAGES CXX)

# Создаем исполняемый файл
add_executable(lexer_fm main.cpp)

# Передаем флаг C++26 (C++2c) напрямую в Clang, а также полезные предупреждения
target_compile_options(lexer_fm PRIVATE -std=c++2c -stdlib=libc++ -Wall -Wextra -Wpedantic)
target_link_options(lexer_fm PRIVATE -stdlib=libc++)