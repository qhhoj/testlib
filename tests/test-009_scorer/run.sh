#!/bin/bash
set -eo pipefail

bash ../scripts/compile src/scorer.cpp

# Well-formed input: the scorer must actually produce a score.
bash ../scripts/test-ref ok-empty    "$VALGRIND" ./scorer < files/empty.txt
bash ../scripts/test-ref ok-single   "$VALGRIND" ./scorer < files/single.txt
bash ../scripts/test-ref ok-mixed    "$VALGRIND" ./scorer < files/mixed.txt
bash ../scripts/test-ref ok-noPoints "$VALGRIND" ./scorer < files/no-points.txt
bash ../scripts/test-ref ok-escapes  "$VALGRIND" ./scorer < files/escapes.txt

# Malformed input must FAIL cleanly (exit 3) rather than terminate on a signal.
bash ../scripts/test-ref bad-index      "$VALGRIND" ./scorer < files/bad-index.txt
bash ../scripts/test-ref bad-verdict    "$VALGRIND" ./scorer < files/bad-verdict.txt
bash ../scripts/test-ref bad-points-nan "$VALGRIND" ./scorer < files/bad-points-nan.txt
bash ../scripts/test-ref bad-points-inf "$VALGRIND" ./scorer < files/bad-points-inf.txt
bash ../scripts/test-ref bad-points-hex "$VALGRIND" ./scorer < files/bad-points-hex.txt
bash ../scripts/test-ref bad-points-big "$VALGRIND" ./scorer < files/bad-points-big.txt
bash ../scripts/test-ref bad-time       "$VALGRIND" ./scorer < files/bad-time.txt
bash ../scripts/test-ref bad-fields     "$VALGRIND" ./scorer < files/bad-fields.txt
bash ../scripts/test-ref bad-overflow   "$VALGRIND" ./scorer < files/bad-overflow.txt

rm -f scorer scorer.exe
