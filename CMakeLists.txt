cmake_minimum_required(VERSION 3.10)
project(arbito)

set(CMAKE_CXX_STANDARD 11)
set(HEADERS include/Configuration.h)

include_directories(lib/aeron-client)
include_directories(lib/fix8/LMAX-market-data)
include_directories(lib/fix8/LMAX-trading)
include_directories(include)

add_subdirectory(lib/aeron-client)

add_executable(arbito src/main.cpp)

target_link_libraries(arbito aeron_client)

install(TARGETS arbito DESTINATION bin)
