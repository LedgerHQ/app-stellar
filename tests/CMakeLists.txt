cmake_minimum_required(VERSION 3.10)
project(stellar_tests C)

include(CTest)

set(CMAKE_C_STANDARD 99)

enable_testing()

add_compile_options(-g -ggdb2)
add_definitions(-DTEST)

add_library(stellar
    ../src/stellar_format.c
    ../src/stellar_utils.c
    ../src/stellar_nvram.c
    ../src/stellar_parser.c
)

target_include_directories(stellar PUBLIC ../src include)
target_link_libraries(stellar PRIVATE bsd)

add_executable(test_printers src/test_printers.c)

target_link_libraries(test_printers PRIVATE cmocka stellar)

add_executable(test_swap src/test_swap.c ../src/swap/swap_lib_calls.c)

target_link_libraries(test_swap PRIVATE cmocka stellar)

add_executable(test_tx src/test_tx.c)

target_link_libraries(test_tx PRIVATE cmocka stellar)

add_test(test_printers test_printers)
add_test(test_tx test_tx)
add_test(test_swap test_swap)

if (FUZZ)
    if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Fuzzer needs to be built with Clang")
    endif()

    add_executable(fuzz_tx src/fuzz_tx.c)
    target_compile_options(fuzz_tx PRIVATE -fsanitize=address,fuzzer -g -ggdb2)
    target_link_options(fuzz_tx PRIVATE -fsanitize=address,fuzzer)
    target_link_libraries(fuzz_tx PRIVATE stellar bsd crypto)
endif()
