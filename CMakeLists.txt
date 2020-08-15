cmake_minimum_required(VERSION 3.17)

# By default, use the vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")

project(miniweb LANGUAGES C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

if (DEFINED MINIWEB_COVERAGE_BUILD)
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()

set(THREADS_PREFER_PTHREAD_FLAG)
find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
