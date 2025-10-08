#!/bin/bash
set -eu
cd "$(dirname "$0")"

for arg in "$@"; do declare $arg='1'; done
if [ ! -v clang ];   then gcc=1; fi
if [ ! -v release ]; then debug=1; fi
if [ -v debug ];     then echo "[debug mode]"; fi
if [ -v release ];   then echo "[release mode]"; fi
if [ -v gcc ];       then compiler="${CC:-gcc}"; echo "[gcc compiler]"; fi
if [ -v clang ];     then compiler="${CC:-clang}"; echo "[clang compiler]"; fi

gcc_common="-I../src/ -std=gnu11 -Wall -Wextra -Wno-unused-function -Wno-unused-variable"
gcc_debug="$compiler -O0 -g -DBUILD_DEBUG=1 ${gcc_common}"
gcc_release="$compiler -O2 -DBUILD_DEBUG=0 ${gcc_common}"
gcc_link="-lm"
gcc_out="-o"

clang_common="-I../src/ -std=gnu11 -Wall -Wextra -Wno-unused-function -Wno-unused-variable"
clang_debug="$compiler -O0 -g -DBUILD_DEBUG=1 ${clang_common}"
clang_release="$compiler -O2 -DBUILD_DEBUG=0 ${clang_common}"
clang_link="-lm"
clang_out="-o"

shared="-fPIC -shared"
link_os="-lX11"

if [ -v gcc ]; then compile_debug="$gcc_debug"; fi
if [ -v gcc ]; then compile_release="$gcc_release"; fi
if [ -v gcc ]; then link="$gcc_link"; fi
if [ -v gcc ]; then out="$gcc_out"; fi

if [ -v clang ]; then compile_debug="$clang_debug"; fi
if [ -v clang ]; then compile_release="$clang_release"; fi
if [ -v clang ]; then link="$clang_link"; fi
if [ -v clang ]; then out="$clang_out"; fi

if [ -v debug ]; then compile="$compile_debug"; fi
if [ -v release ]; then compile="$compile_release"; fi

mkdir -p build

cd build
$compile $shared ../src/krueger_shared.c $link          $out libkrueger.so
$compile         ../src/krueger_main.c   $link $link_os $out krueger 
if [ -v run ]; then ./krueger; fi
cd ..
