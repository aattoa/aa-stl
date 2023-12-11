cmake_minimum_required(VERSION 3.26)
project(aa-stl CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME}
    PRIVATE include/aa/utility.hpp
    PRIVATE include/aa/utility.cpp
    PRIVATE include/aa/maybe.hpp)
target_include_directories(${PROJECT_NAME}
    PUBLIC include)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/W4")
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra" "-Wpedantic")
endif ()

option(AA_STL_BUILD_TESTS "Build aa-stl tests" OFF)
if (${AA_STL_BUILD_TESTS})
    enable_testing()
    add_subdirectory(tests)
endif ()
