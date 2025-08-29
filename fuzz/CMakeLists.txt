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

# Build with code coverage generation
if(CODE_COVERAGE)
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
endif()

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
add_compile_definitions(PRINTF=)

add_definitions("-DIO_SEPROXYHAL_BUFFER_SIZE_B=128") # cmake -DIO_SEPROXYHAL_BUFFER_SIZE_B=128
add_definitions("-DTARGET_NANOSP=1")

add_library(bsd STATIC IMPORTED)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set_property(TARGET bsd PROPERTY IMPORTED_LOCATION /lib/x86_64-linux-gnu/libbsd.a)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set_property(TARGET bsd PROPERTY IMPORTED_LOCATION /lib/aarch64-linux-gnu/libbsd.a)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

include_directories(../libstellar/include)
include_directories(${BOLOS_SDK}/lib_standard_app)

add_library(base58 STATIC ${BOLOS_SDK}/lib_standard_app/base58.c)
add_library(bip32 STATIC ${BOLOS_SDK}/lib_standard_app/bip32.c)
add_library(buffer STATIC ${BOLOS_SDK}/lib_standard_app/buffer.c)
add_library(read STATIC ${BOLOS_SDK}/lib_standard_app/read.c)
add_library(write STATIC ${BOLOS_SDK}/lib_standard_app/write.c)
add_library(format STATIC ${BOLOS_SDK}/lib_standard_app/format.c)
add_library(varint STATIC ${BOLOS_SDK}/lib_standard_app/varint.c)
add_library(apdu_parser STATIC ${BOLOS_SDK}/lib_standard_app/parser.c)
add_library(parser STATIC ../libstellar/parser.c)
add_library(printer STATIC ../libstellar/printer.c)
add_library(formatter STATIC ../libstellar/formatter.c)
add_library(base32 STATIC ../libstellar/base32.c)
add_library(base64 STATIC ../libstellar/base64.c)

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
target_link_libraries(fuzz_tx PRIVATE formatter parser printer format base64 base32 buffer bip32 varint write read bsd)
