cmake_minimum_required(VERSION 3.1)

project(arbito)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#add_subdirectory(lib/aeron-client)
#add_subdirectory(lib/disruptor)

include_directories(include)

find_library(PTHREAD_LIB pthread)
find_library(TBB_LIB tbb)
find_library(POCO_FOUNDATION_LIB PocoFoundation)
find_library(POCO_NET_LIB PocoNet)
find_library(POCO_SSL_LIB PocoNetSSL)
find_library(POCO_CRYPTO_LIB PocoCrypto)
find_library(FIX8_LIB fix8)


if (NOT FIX8_LIB)
    message(FATAL_ERROR "${FIX8_LIB}")
endif ()

FILE(COPY resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_executable(arbito
        src/main.cpp
        src/LMAX_FIXM_types.cpp
        src/LMAX_FIXM_traits.cpp
        src/LMAX_FIXM_classes.cpp
        include/LMAX_FIXM_types.hpp
        include/LMAX_FIXM_classes.hpp
        include/LMAX_FIXM_router.hpp
        include/LMAX_FIXM_session.hpp)

target_link_libraries(arbito
        ${PTHREAD_LIB}
        ${TBB_LIB}
        ${POCO_FOUNDATION_LIB}
        ${POCO_NET_LIB}
        ${POCO_SSL_LIB}
        ${POCO_CRYPTO_LIB}
        ${FIX8_LIB})
