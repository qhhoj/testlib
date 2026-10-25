#!/bin/bash
set -eo pipefail

bash ../scripts/compile files/test-auto-ensure-no-unused-opts.cpp
bash ../scripts/test-ref auto-ensure-no-unused-opts/r1 "$VALGRIND" ./test-auto-ensure-no-unused-opts
bash ../scripts/test-ref auto-ensure-no-unused-opts/r2 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 3
bash ../scripts/test-ref auto-ensure-no-unused-opts/r3 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-val 1
bash ../scripts/test-ref auto-ensure-no-unused-opts/r4 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-value 10 -max-val 20
bash ../scripts/test-ref auto-ensure-no-unused-opts/r5 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-length 3 -min-value 10 -max-value 20
bash ../scripts/test-ref auto-ensure-no-unused-opts/r6 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-n 3 -min-value 10 -max-value 20 -bias-value 3
bash ../scripts/test-ref auto-ensure-no-unused-opts/r7 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 3 -sum-n 10
bash ../scripts/test-ref auto-ensure-no-unused-opts/r8 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-value 10
bash ../scripts/test-ref auto-ensure-no-unused-opts/r9 "$VALGRIND" ./test-auto-ensure-no-unused-opts -test-count 5 -sum-n 100 -min-n 3 -min-value 10 -max-value 20 -value-bias -3
rm -f test-auto-ensure-no-unused-opts test-auto-ensure-no-unused-opts.exe

bash ../scripts/compile files/test-suppress-auto-ensure-no-unused-opts.cpp
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r1 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r2 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 3
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r3 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-val 1
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r4 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-value 10 -max-val 20
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r5 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-length 3 -min-value 10 -max-value 20
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r6 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-n 3 -min-value 10 -max-value 20 -bias-value 3
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r7 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 3 -sum-n 10
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r8 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 3 -sum-n 10 -min-value 10
bash ../scripts/test-ref suppress-auto-ensure-no-unused-opts/r9 "$VALGRIND" ./test-suppress-auto-ensure-no-unused-opts -test-count 5 -sum-n 100 -min-n 3 -min-value 10 -max-value 20 -value-bias -3
rm -f test-suppress-auto-ensure-no-unused-opts test-suppress-auto-ensure-no-unused-opts.exe

bash ../scripts/compile files/test-explicit-ensure-no-unused-opts.cpp
bash ../scripts/test-ref explicit-ensure-no-unused-opts/r1 "$VALGRIND" ./test-explicit-ensure-no-unused-opts -n 5
bash ../scripts/test-ref explicit-ensure-no-unused-opts/r2 "$VALGRIND" ./test-explicit-ensure-no-unused-opts -n 5 -x 1
bash ../scripts/test-ref explicit-ensure-no-unused-opts/r3 "$VALGRIND" ./test-explicit-ensure-no-unused-opts -n 5 -y 1 -x 2
bash ../scripts/test-ref explicit-ensure-no-unused-opts/r4 "$VALGRIND" ./test-explicit-ensure-no-unused-opts -suppress -n 5 -x 1 -y 2
rm -f test-explicit-ensure-no-unused-opts test-explicit-ensure-no-unused-opts.exe

bash ../scripts/compile files/test-has-opt.cpp
bash ../scripts/test-ref has-opt/r1 "$VALGRIND" ./test-has-opt -n 3 -sorted
bash ../scripts/test-ref has-opt/r2 "$VALGRIND" ./test-has-opt -n 3
bash ../scripts/test-ref has-opt/r3 "$VALGRIND" ./test-has-opt -n 3 -sortd
bash ../scripts/test-ref has-opt/r4 "$VALGRIND" ./test-has-opt -n 3 -sortd -m 2
bash ../scripts/test-ref has-opt/r5 "$VALGRIND" ./test-has-opt -n 1 -sortd
rm -f test-has-opt test-has-opt.exe

bash ../scripts/compile files/test-positional-opts.cpp
bash ../scripts/test-ref positional-opts/r1 "$VALGRIND" ./test-positional-opts 5 7
bash ../scripts/test-ref positional-opts/r2 "$VALGRIND" ./test-positional-opts 1 2 -extra
rm -f test-positional-opts test-positional-opts.exe

bash ../scripts/compile files/test-positional-after-named-opts.cpp
bash ../scripts/test-ref positional-after-named-opts/r1 "$VALGRIND" ./test-positional-after-named-opts -n 5 -extra 1
rm -f test-positional-after-named-opts test-positional-after-named-opts.exe
