cmake_minimum_required(VERSION 3.10)
project(test_parser C)

include(CTest)

set(CMAKE_C_STANDARD 11)

include_directories(../src)
add_compile_definitions(TEST)
add_compile_definitions(TARGET_NANOX)

add_executable(test_printers
        test_printers.c
        test_utils.c
        ../src/stellar_utils.c)

target_link_libraries(test_printers PUBLIC cmocka bsd)

add_executable(test_parser
        test_parser.c
        test_utils.c
        ../src/stellar_format_common.c
        ../src/stellar_format_nanox.c
        ../src/stellar_parser.c
        ../src/stellar_utils.c)

target_link_libraries(test_parser PUBLIC cmocka bsd)

add_test(test_printers test_printers)
add_test(test_parser test_parser)