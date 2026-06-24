cmake_minimum_required(VERSION 3.10)
project(VectorDatabase VERSION 1.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set aggressive optimization flags for release builds
# -march=native is critical for AVX2 instructions
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -ffast-math -DNDEBUG")

# Include directory for header files
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create the core database library
add_library(core_db src/database.cpp src/Save_file.cpp)

# ---------------------------------------------------------
# Executable: SIFT Benchmark
# ---------------------------------------------------------
add_executable(sift_benchmark benchmarks/sift_benchmark.cpp)
target_link_libraries(sift_benchmark PRIVATE core_db)
