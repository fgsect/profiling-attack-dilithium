#!/bin/bash
set +e
rm -rf build
mkdir build
cd build
export CC=clang
export CXX=clang++
export CFLAGS="-O3"
export CXXFLAGS="-std=c++17 -Wall -g -O3"
export CMAKE_BUILD_PARALLEL_LEVEL=40
cmake ../
cmake --build .
cd ../
#cd ../ && ./build/main
#clang++ -g -std=c++11 -pthread main.cpp -o main -lntl -lgmp -lm
#./main
