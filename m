#!/bin/sh

CXX="clang"

CXX_FLAGS="-std=c99"
LD_FLAGS="-I/usr/local/include -I/usr/local/lib/include"

ENTRY="day3/main.c"
OUTPUT="build/day3"
$CXX $CXX_FLAGS $LD_FLAGS $ENTRY -o $OUTPUT

