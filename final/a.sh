#!/bin/zsh
set -e

cd build
cmake ..
make
cd ..
./build/main

