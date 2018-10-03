cmake_minimum_required(VERSION 3.1)

project(arbito)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(/usr/local/include/aeronmd/)
include_directories(include)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(ZLIB REQUIRED)
find_package (libmongoc-1.0 1.7 REQUIRED)

find_library(SSL_LIB ssl)
find_library(DISRUPTOR_LIBRARIES Disruptor)
find_library(AERON_CLIENT_LIB aeron_client)
find_library(AERON_DRIVER_LIB aeron_driver)
find_library(AERON_AGENT_LIB aeron_driver_agent)

message ("--   mongoc found version \"${MONGOC_VERSION}\"")
message ("--   mongoc include path \"${MONGOC_INCLUDE_DIRS}\"")
message ("--   mongoc libraries \"${MONGOC_LIBRARIES}\"")

file(GLOB HEADERS
        include/*.h
        include/libtrading/*.h
        include/libtrading/proto/*.h
        include/sbe/*.h
        include/sbe/otf/*.h
        include/lmax/*.h
        include/swissquote/*.h)

file(GLOB SOURCES
        src/*.cpp
        src/lmax/*.cpp
        src/swissquote/*.cpp
        src/libtrading/*.c
        src/libtrading/stringencoders/*.c
        src/libtrading/proto/*.c
        src/libtrading/compat/*.c)

add_executable(arbito
        ${HEADERS}
        ${SOURCES})

target_include_directories (arbito PRIVATE "${MONGOC_INCLUDE_DIRS}")

target_link_libraries(arbito
        ${CMAKE_THREAD_LIBS_INIT}
        ${DISRUPTOR_LIBRARIES}
        ${Boost_LIBRARIES}
        ${SSL_LIB}
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${AERON_DRIVER_LIB}
        ${AERON_AGENT_LIB}
        ${AERON_CLIENT_LIB}
        ${MONGOC_LIBRARIES})

target_compile_definitions (arbito PRIVATE "${MONGOC_DEFINITIONS}")

FILE(COPY resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "Copied resources to binary directory")
