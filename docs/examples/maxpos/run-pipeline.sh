#!/bin/bash
# End-to-end MAXPOS pipeline: build -> generate -> validate -> answer -> check.
# Run from this directory:  bash run-pipeline.sh
set -uo pipefail

TESTLIB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O2 -Wpedantic -Werror -I$TESTLIB_DIR"

say() { printf '\n\033[1m== %s\033[0m\n' "$*"; }
run() { echo "\$ $*"; "$@"; echo "  -> exit $?"; }

say "1. Build"
for f in gen-random gen-max-n validator checker; do
    $CXX $CXXFLAGS "$f.cpp" -o "$f"
done
# Solutions are ordinary programs; they do not include testlib.h.
$CXX -std=c++17 -O2 sol-correct.cpp -o sol-correct
$CXX -std=c++17 -O2 sol-wrong.cpp -o sol-wrong
echo "built: gen-random gen-max-n validator checker sol-correct sol-wrong"

mkdir -p tests

say "2. Generate tests"
# The seed is a hash of the whole command line: same arguments => same test.
./gen-random -test-count 5  -sum-n 20     -min-value 1 -max-value 100 > tests/01
./gen-random -test-count 10 -sum-n 1000   -min-value -1000000000 -max-value 1000000000 -value-bias 3 > tests/02
./gen-max-n  -n 200000 -value 1000000000 > tests/03
wc -l tests/01 tests/02 tests/03

say "3. Validate every test (exit 0 == valid)"
for t in tests/0*; do
    printf '%s: ' "$t"
    if ./validator < "$t"; then echo "valid"; else echo "INVALID (exit $?)"; fi
done

say "4. Produce jury answers with the model solution"
for t in tests/0*; do
    ./sol-correct < "$t" > "$t.a"
done
echo "wrote tests/*.a"

say "5. Check the correct solution (expect: ok, exit 0)"
for t in tests/0*; do
    [[ "$t" == *.a ]] && continue
    printf '%s: ' "$t"
    ./checker "$t" "$t.a" "$t.a"
    echo "  exit $?"
done

say "6. Check the wrong solution (expect: wrong answer, exit 1)"
./sol-wrong < tests/01 > tests/01.wrong
run ./checker tests/01 tests/01.wrong tests/01.a

say "7. Verdicts a checker can return"
printf '1\n2\n5 1\n' > tests/tiny
printf '1\n'         > tests/tiny.a
printf '2\n'         > tests/tiny.wa      # valid index, smaller value  -> wrong answer (1)
printf 'hello\n'     > tests/tiny.pe      # not an integer              -> wrong output format (2)
printf '9\n'         > tests/tiny.range   # out of [1, n]               -> wrong answer (1)
printf '1\n99\n'     > tests/tiny.dirt    # correct, then junk          -> _dirt, reported as PE (2)
: >                    tests/tiny.eof     # nothing at all              -> unexpected eof, reported as PE (2)
for suffix in wa pe range dirt eof; do
    printf '%-6s: ' "$suffix"
    ./checker tests/tiny "tests/tiny.$suffix" tests/tiny.a
    echo "  exit $?"
done

say "8. Validator bounds-hit overview (which constraints the tests actually reach)"
./validator --testOverviewLogFileName /dev/stdout < tests/03

say "Done. Remove build artifacts with: rm -f gen-random gen-max-n validator checker sol-correct sol-wrong"
