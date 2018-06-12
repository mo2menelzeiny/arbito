cmake_minimum_required(VERSION 3.1)

project(arbito)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

find_library(DISRUPTOR_LIBRARIES Disruptor)
find_library(AERON_CLIENT_LIB aeron_client)
find_library(AERON_DRIVER_LIB aeron_driver)

file(GLOB HEADERS
        include/*.h
        include/libtrading/*.h
        include/libtrading/proto/*.h)

file(GLOB SOURCES
        src/*.cpp
        src/libtrading/*.c
        src/libtrading/stringencoders/*.c
        src/libtrading/proto/*.c
        src/libtrading/compat/*.c)

add_executable(arbito
        ${HEADERS}
        ${SOURCES})

target_link_libraries(arbito
        ${ZLIB_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${DISRUPTOR_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        ${AERON_DRIVER_LIB}
        ${AERON_CLIENT_LIB})

FILE(COPY resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "Copied resources to binary directory")
