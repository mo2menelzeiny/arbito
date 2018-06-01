cmake_minimum_required(VERSION 3.1)

project(arbito)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(lib/aeron-client)
add_subdirectory(lib/aeron-driver)
add_subdirectory(lib/disruptor)

include_directories(
        include
        include/fix8
        include/LMAX_FIXM_SBE)

find_library(PTHREAD_LIB pthread)
find_library(TBB_LIB tbb)
find_library(POCO_FOUNDATION_LIB PocoFoundation)
find_library(POCO_NET_LIB PocoNet)
find_library(POCO_SSL_LIB PocoNetSSL)
find_library(POCO_CRYPTO_LIB PocoCrypto)
find_library(FIX8_LIB fix8)
find_library(LIBTRADING_LIB libtrading)

FILE(COPY resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "Copied resources to binary directory")

file(GLOB HEADERS
        "include/*.h"
        "include/fix8/*.hpp")

file(GLOB SOURCES "src/*.cpp")

add_executable(arbito ${HEADERS} ${SOURCES})

target_link_libraries(arbito
        ${PTHREAD_LIB}
        ${TBB_LIB}
        ${POCO_FOUNDATION_LIB}
        ${POCO_NET_LIB}
        ${POCO_SSL_LIB}
        ${POCO_CRYPTO_LIB}
        ${FIX8_LIB}
        ${LIBTRADING_LIB})
