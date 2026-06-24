#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT" || exit 1

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

if [ ! -d "hnswlib" ]; then
    echo "Downloading hnswlib library (header only)..."
    git clone https://github.com/nmslib/hnswlib.git
fi

echo "==========================================="
echo "COMPILING HNSWLIB BENCHMARK BUILD"

g++ -std=c++17 -O3 -march=native -ffast-math \
    -Ihnswlib \
    benchmarks/hnswlib_benchmark.cpp \
    -o bench_hnswlib -lpthread

echo "RUNNING HNSWLIB BENCHMARK..."
./bench_hnswlib
