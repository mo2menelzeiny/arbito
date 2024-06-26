cmake_minimum_required(VERSION 3.1)

project(arbito)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "--coverage")

include_directories(include)

# Core dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# MongoDB C driver
find_package(libmongoc-1.0 REQUIRED)

# Disruptor-cpp
find_package(Boost REQUIRED COMPONENTS system thread chrono)
find_package(ZLIB REQUIRED)
find_library(DISRUPTOR_LIBRARIES Disruptor)

# C NATS
find_package(cnats REQUIRED)

file(GLOB SPDLOG
        include/spdlog/*.h
        include/spdlog/details/*.h
        include/spdlog/fmt/*.h
        include/spdlog/sinks/*.h)

file(GLOB HEADERS
        include/*.h
        include/libtrading/*.h
        include/libtrading/proto/*.h
        include/catch2/*.hpp)

file(GLOB SOURCES
        src/*.cpp
        src/libtrading/*.c
        src/libtrading/stringencoders/*.c
        src/libtrading/proto/*.c
        src/libtrading/compat/*.c)

file(GLOB TESTS
        tests/*.h
        tests/*.cpp
        tests/unit/*.cpp
        tests/integration/*.cpp)

add_executable(arbito-tests
        ${TESTS}
        src/MongoDBDriver.cpp
        src/NATSSession.cpp
        src/StreamOffice.cpp)

add_executable(arbito
        ${HEADERS}
        ${SOURCES}
        ${SPDLOG})

target_include_directories(arbito-tests PRIVATE "${MONGOC_INCLUDE_DIRS}")
target_include_directories(arbito PRIVATE "${MONGOC_INCLUDE_DIRS}")

target_link_libraries(arbito-tests
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_LIBRARIES}
        ${MONGOC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${DISRUPTOR_LIBRARIES}
        cnats::nats)

target_link_libraries(arbito
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_LIBRARIES}
        ${MONGOC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${DISRUPTOR_LIBRARIES}
        cnats::nats)

FILE(COPY resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})