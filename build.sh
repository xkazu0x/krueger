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

gcc_common="-I../src/ -std=gnu11 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-type-limits -Wno-override-init -Wno-missing-braces -Wno-switch"
gcc_debug="$compiler -O0 -g -DBUILD_DEBUG=1 ${gcc_common}"
gcc_release="$compiler -O2 -DBUILD_DEBUG=0 ${gcc_common}"
gcc_shared="-fPIC -shared"
gcc_link="-lm"
gcc_out="-o"

clang_common="-I../src/ -std=gnu11 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter"
clang_debug="$compiler -O0 -g -DBUILD_DEBUG=1 ${clang_common}"
clang_release="$compiler -O2 -DBUILD_DEBUG=0 ${clang_common}"
clang_shared="-fPIC -shared"
clang_link="-lm"
clang_out="-o"

link_plat="-lX11 -lGL"

if [ -v gcc ]; then compile_debug="$gcc_debug"; fi
if [ -v gcc ]; then compile_release="$gcc_release"; fi
if [ -v gcc ]; then shared="$gcc_shared"; fi
if [ -v gcc ]; then link="$gcc_link"; fi
if [ -v gcc ]; then out="$gcc_out"; fi

if [ -v clang ]; then compile_debug="$clang_debug"; fi
if [ -v clang ]; then compile_release="$clang_release"; fi
if [ -v clang ]; then shared="$clang_shared"; fi
if [ -v clang ]; then link="$clang_link"; fi
if [ -v clang ]; then out="$clang_out"; fi

if [ -v debug ]; then compile="$compile_debug"; fi
if [ -v release ]; then compile="$compile_release"; fi

if [ -v clean ]; then rm -rf build; fi
mkdir -p build

cd build
# $compile ../src/test_linux_main.c $link $link_plat $out krueger 
# if [ -v run ]; then ./krueger; fi

$compile $shared ../src/starfighter.c      $link            $out libstarfighter.so
$compile         ../src/starfighter_main.c $link $link_plat $out starfighter 
if [ -v run ]; then ./starfighter; fi
cd ..
