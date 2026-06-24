#!/bin/bash

echo "==========================================="
echo "COMPILING SCALAR (AUTO-VECTORIZATION) BUILD"
echo "g++ -std=c++17 -O3 -march=native -ffast-math -Isrc benchmark.cpp src/database.cpp -o bench_scalar"
g++ -std=c++17 -O3 -march=native -ffast-math -Isrc benchmark.cpp src/database.cpp -o bench_scalar
echo "RUNNING SCALAR BUILD..."
./bench_scalar

echo ""
echo "==========================================="
echo "COMPILING MANUAL EXPLICIT SIMD BUILD"
g++ -std=c++17 -O3 -march=native -mavx2 -mfma -Isrc benchmark.cpp src/database_manual_simd.cpp -o bench_simd
echo "RUNNING EXPLICIT SIMD BUILD..."
./bench_simd
