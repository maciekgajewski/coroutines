#!/bin/bash -e

mkdir -p .asan
clang++ -g -O1 -fsanitize=address -fno-omit-frame-pointer -std=c++11 -o .asan/coroutines main.cpp -L/usr/local/boost_1_54/lib -Wl,-Bstatic -lboost_context -Wl,-Bdynamic
cd .asan
./coroutines 2> log
/home/maciek/dev/llvm/llvm/projects/compiler-rt/lib/asan/scripts/asan_symbolize.py / < log | c++filt
