#!/bin/bash
if [ ! -d "sift" ]; then
    echo "Downloading SIFT1M dataset (this may take a few minutes depending on your internet connection)..."
    wget ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz
    tar -xzf sift.tar.gz
    rm sift.tar.gz
fi

echo "==========================================="
echo "COMPILING SIFT BENCHMARK BUILD"
g++ -std=c++17 -O3 -march=native -ffast-math -Isrc sift_benchmark.cpp src/database.cpp -o bench_sift
echo "RUNNING SIFT BENCHMARK..."
./bench_sift
