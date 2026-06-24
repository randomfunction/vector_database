#!/bin/bash

if [ -d "sift" ]; then
    echo "SIFT dataset already extracted."
else
    if [ -f "sift.tar.gz" ]; then
        echo "Found existing sift.tar.gz. Extracting..."
    else
        echo "Downloading SIFT1M dataset..."
        wget ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz
    fi

    tar -xzf sift.tar.gz
fi

echo "==========================================="
echo "COMPILING SIFT BENCHMARK BUILD"

g++ -std=c++17 -O3 -march=native -ffast-math \
    -Isrc sift_benchmark.cpp src/database.cpp \
    -o bench_sift

echo "RUNNING SIFT BENCHMARK..."
./bench_sift