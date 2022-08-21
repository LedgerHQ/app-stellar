cmake_minimum_required(VERSION 3.10)

if (${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif ()

# project information
project(tests_unit
        VERSION 0.1
        DESCRIPTION "Unit tests for Ledger Nano application"
        LANGUAGES C)


# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

include(CTest)
ENABLE_TESTING()

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -g -O0 --coverage")

set(GCC_COVERAGE_LINK_FLAGS "--coverage -lgcov")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# guard against in-source builds
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif ()

add_compile_definitions(TEST)
add_definitions("-DIO_SEPROXYHAL_BUFFER_SIZE_B=128") # cmake -DIO_SEPROXYHAL_BUFFER_SIZE_B=128
add_definitions("-DTARGET_NANOS=1")

include_directories(../src)
include_directories(mock_includes)

add_executable(test_utils test_utils.c)
add_executable(test_tx_parser test_tx_parser.c)
add_executable(test_tx_formatter test_tx_formatter.c)
add_executable(test_swap test_swap.c)

file(GLOB src_common "../src/common/*.c")

add_library(common STATIC ${src_common})
add_library(utils STATIC ../src/utils.c)
add_library(globals STATIC ../src/globals.c)
add_library(tx_parser STATIC ../src/transaction/transaction_parser.c)
add_library(tx_formatter STATIC ../src/transaction/transaction_formatter.c)
add_library(swap STATIC ../src/swap/swap_lib_calls.c)

target_link_libraries(test_utils PUBLIC cmocka gcov utils common bsd)
target_link_libraries(test_tx_parser PUBLIC cmocka gcov tx_parser utils common bsd)
target_link_libraries(test_tx_formatter PUBLIC cmocka gcov tx_parser tx_formatter utils common globals bsd)
target_link_libraries(test_swap PUBLIC cmocka gcov swap tx_formatter tx_parser utils common bsd)

add_test(test_utils test_utils)
add_test(test_tx_parser test_tx_parser)
add_test(test_tx_formatter test_tx_formatter)
add_test(test_swap test_swap)