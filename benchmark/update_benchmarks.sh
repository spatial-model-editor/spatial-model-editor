#!/usr/bin/env bash

# simple script to run benchmarks & generate plots of the results

BENCH_EXE=${1:-"../build/benchmark/bench"}

$BENCH_EXE --benchmark_out=bench_out.json

python plot.py
