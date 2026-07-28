#!/bin/bash
set -eo pipefail

# The end-of-input sentinel used to be 255, a value a real byte can hold. Both
# readers are exercised over the same fixtures and must agree: a 0xFF byte is
# data, and only genuine end of input ends the stream.
#
# eofc-stdin reads stdin  -> FileInputStreamReader
# eofc-file  reads ouf    -> BufferedFileInputStreamReader

bash ../scripts/compile src/eofc-stdin.cpp
bash ../scripts/compile src/eofc-file.cpp

for fixture in plain ff-leading ff-middle ff-only high-bytes; do
  bash ../scripts/test-ref stdin-"$fixture" "$VALGRIND" ./eofc-stdin < files/"$fixture".txt
  bash ../scripts/test-ref file-"$fixture"  "$VALGRIND" ./eofc-file  files/plain.txt files/"$fixture".txt files/plain.txt
done

rm -f eofc-stdin eofc-stdin.exe eofc-file eofc-file.exe
