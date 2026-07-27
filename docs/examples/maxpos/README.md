# Worked example: problem MAXPOS

A complete, runnable problem package. Every file here compiles under the same
flags the repo's own test suite uses (`-std=c++17 -Wpedantic -Werror -O2`).

## The problem

> **MAXPOS.** The first line contains `t` (1 ≤ t ≤ 10⁴), the number of test
> cases. Each test case consists of a line with `n` (1 ≤ n ≤ 2·10⁵) and a line
> with `n` integers `a₁ … aₙ` (−10⁹ ≤ aᵢ ≤ 10⁹). The sum of `n` over all test
> cases does not exceed 2·10⁵.
>
> For each test case print **any** 1-based index `i` such that `aᵢ` is maximal.

"Any index" is the point of this example: a plain token comparison (`wcmp`)
would reject correct submissions, so the problem needs a real special checker
that reads the test data itself.

## Files

| File | Role |
| --- | --- |
| `validator.cpp` | Validates an input file. Strict reading, per-test-case context, sum-of-`n` constraint. |
| `gen-random.cpp` | Random tests, fully parameterised via `opt<T>("key")`. |
| `gen-max-n.cpp` | Edge case: one test case at maximum `n` with all values equal — every index is a valid answer. |
| `checker.cpp` | Special checker. Reads the test from `inf`, jury index from `ans`, participant index from `ouf`. |
| `sol-correct.cpp` | Reference solution. Plain stdin/stdout, no testlib. |
| `sol-wrong.cpp` | Always answers `1`; used to demonstrate a wrong-answer verdict. |
| `run-pipeline.sh` | Builds everything and runs the whole loop end to end. |

## Run it

```sh
cd docs/examples/maxpos
bash run-pipeline.sh
```

Or step by step:

```sh
# 1. Build (note -I pointing at the repo root, where testlib.h lives)
g++ -std=c++17 -O2 -I../../.. gen-random.cpp -o gen-random
g++ -std=c++17 -O2 -I../../.. validator.cpp  -o validator
g++ -std=c++17 -O2 -I../../.. checker.cpp    -o checker
g++ -std=c++17 -O2 sol-correct.cpp -o sol-correct
g++ -std=c++17 -O2 sol-wrong.cpp   -o sol-wrong

mkdir -p tests

# 2. Generate. The seed is a hash of the whole command line, so the same
#    arguments always produce a byte-identical file.
./gen-random -test-count 5 -sum-n 20 -min-value 1 -max-value 100 > tests/01

# 3. Validate. Exit 0 means valid; any other exit code is a rejection and the
#    reason goes to stderr.
./validator < tests/01

# 4. Produce the jury answer with the model solution.
./sol-correct < tests/01 > tests/01.a

# 5. Check.  usage: checker <input> <participant-output> <jury-answer>
./checker tests/01 tests/01.a tests/01.a        # ok,  exit 0
./sol-wrong < tests/01 > tests/01.wrong
./checker tests/01 tests/01.wrong tests/01.a    # wrong answer, exit 1
```

## What each verdict looks like

Feeding hand-made participant outputs for the test `1 / 2 / 5 1` with jury
answer `1`:

| Participant output | Checker says | Exit |
| --- | --- | --- |
| `1` | `ok 1 test case(s)` | 0 |
| `2` | `wrong answer test case 1: jury answer 1 has value 5, but participant answer 2 has value 1` | 1 |
| `9` | `wrong answer Integer parameter [name=index] equals to 9, violates the range [1, 2]` | 1 |
| `hello` | `wrong output format Expected integer, but "hello" found` | 2 |
| `1` then `99` | `wrong output format Extra information in the output file` | 2 |
| *(empty)* | `wrong output format Unexpected end of file - int32 expected` | 2 |

Note the last three: `_pe`, `_dirt` and `_unexpected_eof` all surface as exit
code 2 by default. See [the usage guide](../../usage-guide.md#verdicts-and-exit-codes).

And validator rejections:

```
$ printf '1\n0\n\n' | ./validator
FAIL Integer parameter [name=n] equals to 0, violates the range [1, 200000] (test case 1, stdin, line 2)

$ printf '1\n2\n5 2000000000\n' | ./validator
FAIL Integer element a[2] equals to 2000000000, violates the range [-10^9, 10^9] (test case 1, stdin, line 3)

$ ./gen-random -test-count 3 -sum-n 250000 -min-n 80000 -max-value 5 -min-value 1 | ./validator
FAIL sum of n over all test cases is 250000, must be at most 200000
```

The variable names passed to `readInt` / `readInts` (`"t"`, `"n"`, `"a"`) are
what make those messages readable, and they also feed the bounds-hit report:

```
$ ./validator --testOverviewLogFileName /dev/stdout < tests/03
"a": max-value-hit
"n": max-value-hit
"t": min-value-hit
constant-bounds "a": -1000000000 1000000000
constant-bounds "n": 1 200000
constant-bounds "t": 1 10000
variable "a"
variable "n"
variable "t"
```

Run that over your whole test set and any constraint without both a
`min-value-hit` and a `max-value-hit` is a boundary you never tested.
