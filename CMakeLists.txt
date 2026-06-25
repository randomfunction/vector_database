cmake_minimum_required(VERSION 3.10)
project(VectorDatabase VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -ffast-math -DNDEBUG")

include_directories(${CMAKE_SOURCE_DIR}/include)

# Pybind11 Integration
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# Core library with Position Independent Code for Python shared library
add_library(core_db src/database.cpp src/Save_file.cpp)
set_property(TARGET core_db PROPERTY POSITION_INDEPENDENT_CODE ON)

# Python Module linking
pybind11_add_module(custom_vectordb src/bindings.cpp)
target_link_libraries(custom_vectordb PRIVATE core_db)
# Output the .so file directly to the project root for app.py
set_target_properties(custom_vectordb PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Benchmarks
add_executable(sift_benchmark benchmarks/sift_benchmark.cpp)
target_link_libraries(sift_benchmark PRIVATE core_db)
