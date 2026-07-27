#!/bin/bash
set -eo pipefail

bash ../scripts/compile src/skipchar.cpp

# Normal skipping still works.
bash ../scripts/test-ref skip-none "$VALGRIND" ./skipchar files/n-none.txt files/small.txt files/small.txt
bash ../scripts/test-ref skip-some "$VALGRIND" ./skipchar files/n-some.txt files/small.txt files/small.txt

# Skipping far past end of file must stay in bounds and report eof, not read
# off the end of the buffer.
bash ../scripts/test-ref skip-past-eof "$VALGRIND" ./skipchar files/n-past-eof.txt files/small.txt files/small.txt
bash ../scripts/test-ref skip-huge     "$VALGRIND" ./skipchar files/n-huge.txt     files/small.txt files/small.txt

# Skipping across the buffer boundary must refill, so the token after the skip
# is the real one rather than stale buffer contents.
bash ../scripts/compile src/gen-large.cpp
./gen-large
bash ../scripts/test-ref skip-refill "$VALGRIND" ./skipchar files/n-refill.txt large.txt files/small.txt
rm -f large.txt gen-large gen-large.exe

rm -f skipchar skipchar.exe
