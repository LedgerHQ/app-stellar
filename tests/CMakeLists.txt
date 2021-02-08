cmake_minimum_required(VERSION 3.10)
project(test_parser C)

include(CTest)

set(CMAKE_C_STANDARD 99)

# required for fuzzing
set(CMAKE_C_COMPILER /usr/bin/clang)

enable_testing()

include_directories(include/ ../src)
add_compile_definitions(TEST)

add_library(stellar
../src/stellar_format.c
../src/stellar_utils.c
../src/stellar_nvram.c
../src/stellar_parser.c
)

add_executable(test_printers
  src/test_printers.c
  src/cx.c
  include/bolos_target.h
  include/cx.h
  include/os.h
)

add_executable(test_swap
  src/test_swap.c
  src/cx.c
  ../src/swap/swap_lib_calls.c
  ../src/swap/swap_lib_calls.h
  ../src/stellar_api.h
  ../src/stellar_types.h
  include/bolos_target.h
  include/cx.h
  include/os.h
  )

add_executable(test_tx
  src/test_tx.c
  src/cx.c
  include/bolos_target.h
  include/cx.h
  include/os.h
)

target_link_libraries(test_printers PRIVATE cmocka crypto ssl bsd stellar)
target_link_libraries(test_tx PRIVATE cmocka crypto ssl bsd stellar)
target_link_libraries(test_swap PRIVATE cmocka crypto ssl bsd stellar)

add_test(test_printers test_printers)
add_test(test_tx test_tx)
add_test(test_swap test_swap)
