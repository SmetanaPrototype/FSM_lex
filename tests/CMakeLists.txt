add_executable(lexer_tests main.cpp)

target_sources(lexer_tests
    PRIVATE FILE_SET CXX_MODULES
    FILES
        test.cppm
)

target_link_libraries(lexer_tests PRIVATE fsm)

target_compile_options(lexer_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME lexer_tests COMMAND lexer_tests)