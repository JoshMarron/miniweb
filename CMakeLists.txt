cmake_minimum_required(VERSION 3.17)

project(miniweb LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# By default, use the vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

find_package(cmocka CONFIG REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
