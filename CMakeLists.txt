cmake_minimum_required(VERSION 3.1)

project(arbito)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

find_library(SSL_LIB ssl)
find_library(DISRUPTOR_LIB Disruptor)
find_library(MONGOC_LIB libmongoc-1.0.so.0.0.0)

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

file(GLOB SPDLOG
        include/spdlog/*.h
        include/spdlog/details/*.h
        include/spdlog/fmt/*.h
        include/spdlog/sinks/*.h)

file(GLOB IBAPI
        include/IBAPI/client/*.h
        include/IBAPI/IBClient/*.h
        include/IBAPI/client/*.cpp
        include/IBAPI/IBClient/*.cpp)

add_executable(arbito
        ${HEADERS}
        ${SOURCES}
        ${SPDLOG}
        ${IBAPI})

#target_include_directories(arbito PRIVATE "${MONGOC_INCLUDE_DIRS}")

target_include_directories(arbito PRIVATE
        /usr/include/libmongoc-1.0
        /usr/include/libbson-1.0)

target_link_libraries(arbito
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        ${SSL_LIB}
        ${MONGOC_LIB})

#target_compile_definitions(arbito PRIVATE "${MONGOC_DEFINITIONS}")

FILE(COPY resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "Copied resources to binary directory")
