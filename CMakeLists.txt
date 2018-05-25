cmake_minimum_required(VERSION 3.5)

project(arbito)

set(CMAKE_CXX_STANDARD 11)

#add_subdirectory(lib/aeron-client)
#add_subdirectory(lib/disruptor)


include_directories(include)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.11/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake")
endif ()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(CONANFILE conanfile.txt
        BASIC_SETUP CMAKE_TARGETS
        SETTINGS compiler=gcc
        SETTINGS compiler.libcxx=libstdc++11
        BUILD missing)

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