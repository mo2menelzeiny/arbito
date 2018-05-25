cmake_minimum_required(VERSION 3.10)

project(arbito)

set(CMAKE_CXX_STANDARD 11)

#add_subdirectory(lib/aeron-client)
#add_subdirectory(lib/disruptor)


include_directories(include)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

conan_basic_setup()

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
        ${CONAN_LIBS}
        ${FIX8_LIB})

