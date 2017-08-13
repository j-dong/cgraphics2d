cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(graphics)

include(CheckFunctionExists)

add_subdirectory(glad)

add_subdirectory(lodepng)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw-3.2.1)

# get build options for compiler

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(GRAPHICS_DEFAULT_BUILD_OPTIONS "/W4")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang"
    OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    set(GRAPHICS_DEFAULT_BUILD_OPTIONS "-Wall" "-Wextra")
else()
    set(GRAPHICS_DEFAULT_BUILD_OPTIONS "")
endif()

set(GRAPHICS_BUILD_OPTIONS ${GRAPHICS_DEFAULT_BUILD_OPTIONS} CACHE STRING "Build options for programs and tests")

# check for math functions

CHECK_FUNCTION_EXISTS(fmin HAVE_FMIN)
if(NOT HAVE_FMIN)
    CHECK_LIBRARY_EXISTS(m fmin "" HAVE_M_FMIN)
    if(HAVE_M_FMIN)
        set(GRAPHICS_DETECTED_MATH_LIBS "m")
    else()
        set(GRAPHICS_DETECTED_MATH_LIBS "")
        message(WARNING "no fmin() found")
    endif()
else()
    set(GRAPHICS_DETECTED_MATH_LIBS "")
endif()

set(GRAPHICS_MATH_LIBS ${GRAPHICS_DETECTED_MATH_LIBS} CACHE STRING "Library containing functions in <math.h>")

add_subdirectory(src)

# tests

set(GRAPHICS_BUILD_TESTS YES CACHE BOOL "Build tests")

if(GRAPHICS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
