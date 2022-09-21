cmake_minimum_required(VERSION 3.10)

if (${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif ()

# project information
project(tests_unit
        VERSION 0.1
        DESCRIPTION "Fuzzing of transaction parser and foramtter"
        LANGUAGES C)

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif ()

include(CTest)
ENABLE_TESTING()

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -g -O0")


# guard against in-source builds
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif ()

add_compile_definitions(TEST)
add_definitions("-DIO_SEPROXYHAL_BUFFER_SIZE_B=128") # cmake -DIO_SEPROXYHAL_BUFFER_SIZE_B=128
add_definitions("-DTARGET_NANOS=1")

include_directories(../src)
include_directories(../tests_unit/mock_includes)

add_library(bsd STATIC IMPORTED)
set_property(TARGET bsd PROPERTY IMPORTED_LOCATION /lib/x86_64-linux-gnu/libbsd.a)

file(GLOB src_common "../src/common/*.c")

add_library(common STATIC ${src_common})
add_library(utils STATIC ../src/utils.c)
add_library(globals STATIC ../src/globals.c)
add_library(tx_parser STATIC ../src/transaction/transaction_parser.c)
add_library(tx_formatter STATIC ../src/transaction/transaction_formatter.c)

# compatible with ClusterFuzzLite
if (NOT DEFINED ENV{LIB_FUZZING_ENGINE})
    set(COMPILATION_FLAGS_ "-fsanitize=address,fuzzer -g")
else ()
    set(COMPILATION_FLAGS_ "$ENV{LIB_FUZZING_ENGINE} $ENV{CXXFLAGS}")
endif ()
string(REPLACE " " ";" COMPILATION_FLAGS ${COMPILATION_FLAGS_})
message(${COMPILATION_FLAGS})
add_executable(fuzz_tx fuzz_tx.c)
target_compile_options(fuzz_tx PRIVATE ${COMPILATION_FLAGS})
target_link_options(fuzz_tx PRIVATE ${COMPILATION_FLAGS})
target_link_libraries(fuzz_tx PRIVATE tx_parser tx_formatter utils common globals bsd)
