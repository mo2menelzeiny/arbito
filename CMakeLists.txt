cmake_minimum_required(VERSION 3.10)
project(aeron_disruptor_arbitrage)

set(CMAKE_CXX_STANDARD 11)

include_directories(lib/aeron-client)

add_subdirectory(lib/aeron-client)

add_executable(aeron_disruptor_arbitrage main.cpp)

target_link_libraries(aeron_disruptor_arbitrage aeron_client)

install(TARGETS aeron_disruptor_arbitrage DESTINATION bin)
