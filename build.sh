#!/bin/bash
set -eu
cd "$(dirname "$0")"

for arg in "$@"; do declare $arg='1'; done
if [ ! -v gcc ];     then clang=1; fi
if [ ! -v release ]; then debug=1; fi
if [ -v debug ];     then echo "[debug mode]"; fi
if [ -v release ];   then echo "[release mode]"; fi
if [ -v clang ];     then compiler="${CC:-clang}"; echo "[clang compiler]"; fi
if [ -v gcc ];       then compiler="${CC:-gcc}"; echo "[gcc compiler]"; fi

compile_common="-I../src/ -std=c99 -pedantic -Wall -Wextra -Wno-unused-function -Wno-zero-length-array"
compile_debug="$compiler -O0 -g -DBUILD_DEBUG=1 ${compile_common}"
compile_release="$compiler -O2 -DBUILD_DEBUG=0 ${compile_common}"
link=""
out="-o"

if [ -v debug ]; then compile="$compile_debug"; fi
if [ -v release ]; then compile="$compile_release"; fi

mkdir -p build

cd build
$compile ../src/krueger.c $link $out krueger 
if [ -v run ]; then ./krueger; fi
cd ..

