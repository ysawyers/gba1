#!/usr/bin/env bash

cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-16
cmake --build build