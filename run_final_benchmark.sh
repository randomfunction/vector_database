#!/bin/bash
echo "==========================================="
echo "COMPILING SCALAR BUILD"
echo "g++ -std=c++17 -O3 -march=native -ffast-math -Isrc final_benchmark.cpp src/database.cpp -o bench_final"
g++ -std=c++17 -O3 -march=native -ffast-math -Isrc final_benchmark.cpp src/database.cpp -o bench_final

echo ""
echo "RUNNING BENCHMARK..."
./bench_final
