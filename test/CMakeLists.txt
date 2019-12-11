cmake_minimum_required(VERSION 3.10)
project(test_parser C)

set(CMAKE_C_STANDARD 11)

include_directories(../src)
add_compile_definitions(TEST)

add_executable(test_utils
        utilstest.c
        test_utils.c
        ../src/stellar_utils.c)

add_executable(test_parser
        test_parser.c
        test_utils.c
        ../src/stellar_format_blue.c
        ../src/stellar_parser.c
        ../src/stellar_utils.c)
target_link_libraries(test_parser PUBLIC cmocka)
