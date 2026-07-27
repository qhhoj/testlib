#!/bin/bash
set -eo pipefail

bash ../scripts/compile src/gen.cpp
bash ../scripts/test-ref r1 $VALGRIND ./gen 10 100
bash ../scripts/test-ref r2 $VALGRIND ./gen 100 1000
rm -f gen gen.exe

# registerGen(argc, argv, 2) is its own compatibility surface: pin it too.
bash ../scripts/compile src/gen-v2.cpp
bash ../scripts/test-ref v2-r1 $VALGRIND ./gen-v2 10
bash ../scripts/test-ref v2-r2 $VALGRIND ./gen-v2 100
rm -f gen-v2 gen-v2.exe
