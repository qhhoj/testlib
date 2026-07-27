# testlib usage guide

How to use testlib to prepare a competitive programming problem: generators,
validators, checkers, interactors and scorers.

This guide describes **testlib 0.9.49** as vendored in this repository
(`testlib.h`, single header, MIT). Line references point at that file.

- [1. The pieces of a problem package](#1-the-pieces-of-a-problem-package)
- [2. Setup and compiling](#2-setup-and-compiling)
- [3. Generators](#3-generators)
- [4. Validators](#4-validators)
- [5. Checkers](#5-checkers)
- [6. Interactors](#6-interactors)
- [7. Scorers](#7-scorers)
- [8. Patterns](#8-patterns)
- [9. Verdicts and exit codes](#9-verdicts-and-exit-codes)
- [10. Worked example](#10-worked-example)
- [11. Gotchas](#11-gotchas)

---

## 1. The pieces of a problem package

testlib gives you five kinds of small standalone programs. Each one is an
ordinary `main()` that includes `testlib.h` and calls exactly one `register*`
function first.

| Role | Register with | Reads | Writes |
| --- | --- | --- | --- |
| **Generator** | `registerGen(argc, argv, 1)` | command line only | test input on stdout |
| **Validator** | `registerValidation(argc, argv)` | test input on stdin | nothing (exit code is the verdict) |
| **Checker** | `registerTestlibCmd(argc, argv)` | `inf`, `ouf`, `ans` files | verdict message + exit code |
| **Interactor** | `registerInteraction(argc, argv)` | test file + solution's stdout | to the solution's stdin, and to `tout` |
| **Scorer** | `registerScorer(argc, argv, fn)` | serialized per-test results | a single score |

There is **no `registerChecker`** — checkers use `registerTestlibCmd`.

The normal pipeline, the one Polygon automates:

```
generator  --(args)-->  input file  --> validator      (is the test legal?)
input file --> model solution --> jury answer
input file + participant output + jury answer --> checker --> verdict
```

Four global streams exist once you have registered:

| Stream | Generator | Validator | Checker | Interactor |
| --- | --- | --- | --- | --- |
| `inf` | – | stdin (strict) | test input | test input |
| `ouf` | – | – | participant output | **stdin** — the solution's output |
| `ans` | – | – | jury answer | jury answer (if passed) |
| `tout` | – | – | – | output file for a later checker |

The single most important rule: **a read failure on `inf` or `ans` is promoted
to `_fail`, not `_wa`.** Jury data is never "wrong"; if it does not parse, the
problem package is broken. Failures on `ouf` are the participant's fault and
become `_wa` / `_pe`.

## 2. Setup and compiling

testlib is one header with no build system. There is no `Makefile` and no
`CMakeLists.txt` (the latter is gitignored).

```sh
g++ -std=c++17 -O2 -I/path/to/testlib mygen.cpp -o mygen
```

- C++11 is the minimum; the library is CI-tested on C++11/14/17/20/23 with
  g++, clang++ and MSVC.
- The repo's own test suite compiles every sample with `-Wpedantic -Werror -O2`
  (`tests/scripts/compile`). Use the same flags for anything you add here.
- **Never build with `-ffast-math`.** `__testlib_ensuresPreconditions()`
  (`testlib.h:4625`) detects it via a runtime NaN check and aborts, because
  `doubleCompare` relies on real IEEE NaN semantics. It also static-asserts
  `sizeof(int) == 4`, `sizeof(long long) == 8`, `sizeof(double) == 8`.

Useful compile-time switches (all optional, all `-D`):
`ENABLE_UNEXPECTED_EOF`, `EJUDGE`, `CONTESTER`, `TESTSYS`, `PCMS2`, and direct
overrides such as `-DPE_EXIT_CODE=14`. See [§9](#9-verdicts-and-exit-codes).

## 3. Generators

```cpp
#include "testlib.h"

int main(int argc, char *argv[]) {
    registerGen(argc, argv, 1);
    println(rnd.next(1, 10));
    println(rnd.next("[a-zA-Z0-9]{1,1000}"));
}
```

### Seeding: the rule that makes tests reproducible

`registerGen` seeds `rnd` from a hash of the **entire command line**
(`rnd.setSeed(argc, argv)`). Consequences you must internalise:

- Same arguments ⇒ byte-identical output, on every machine and compiler —
  **under version 2**. Versions 0 and 1 read argument bytes as `char`, which is
  signed on x86 and MSVC but unsigned on ARM, so any argument containing a byte
  ≥ 0x80 (a UTF-8 name, say) seeds *differently per architecture*. One more
  reason to use version 2 for new generators.
- Different arguments ⇒ a different test.
- To get 20 different random tests, invoke the same generator 20 times with
  different arguments (`gen 1`, `gen 2`, …), not with a loop inside.
- **Never** call `srand`, `time(0)`, `std::random_device`, or seed anything
  yourself. `rand()`, `srand()` and `std::random_shuffle` are deliberately
  poisoned (`testlib.h:5061`, `5078`, `5096`) — using them is a compile error
  under GCC and a `_fail` otherwise.

The third argument is the random-generator version. **Use `2` for new
generators.** Versions `0` and `1` exist only to reproduce the exact test data
of existing packages and must not be used for new work: their 63-bit draws hand
back the low bits of a 48-bit LCG, so `rnd.next(0, 1)` repeats every 65536 calls
under version 1 and every 131072 under version 0. Version 2 takes only
high-order state bits and has no such period. `rnd.next(2)` — the single-argument
`int` overload — was always sound in every version.

The two-argument `registerGen(argc, argv)` overload is deprecated (it implies
version 0).

All three versions are frozen: once a generator has produced a test package,
its stream must never change. Each is pinned by reference files in
`tests/test-003_run-rnd/`.

### The `rnd` API

```cpp
int    rnd.next(int n);                    // uniform in [0, n-1]
int    rnd.next(int from, int to);         // uniform in [from, to]   INCLUSIVE
double rnd.next();                         // [0, 1)
double rnd.next(double n);                 // [0, n)
double rnd.next(double from, double to);   // [from, to)              EXCLUSIVE
std::string rnd.next(const std::string &pattern);   // e.g. "[a-z]{1,10}"
std::string rnd.next(const char *format, ...);      // printf-formatted pattern
```

Integer overloads exist for `int`, `unsigned int`, `long`, `unsigned long`,
`long long`, `unsigned long long`. Note the asymmetry: integer ranges are
inclusive on both ends, floating-point ranges are half-open.

**Biased draws** — `wnext(..., type)`:

```cpp
int rnd.wnext(int n, int type);
int rnd.wnext(int from, int to, int type);   // and the same for other types
```

`type == 0` behaves exactly like `next`. `type > 0` returns the **maximum** of
`type + 1` independent draws, pulling values toward the top of the range;
`type < 0` returns the **minimum** of `|type| + 1` draws, pulling toward the
bottom. Larger `|type|` means stronger bias. For `|type| >= 25` it switches to
a closed form instead of looping, so huge biases stay cheap.

This is how you get "mostly large values", "a path-like tree", "a star-like
tree" out of the same generator — see `generators/iwgen.cpp` (documented
inline) and `generators/gen-tree-graph.cpp`.

**Picking and sequences:**

```cpp
rnd.any(container);                 // random element
rnd.any(begin, end);
rnd.wany(container, type);          // biased toward the end (type > 0)

std::vector<T> rnd.perm(n);         // permutation of 0..n-1
std::vector<T> rnd.perm(n, first);  // permutation of first..first+n-1
std::vector<T> rnd.distinct(size, from, to);  // distinct, UNSORTED, in [from,to]
std::vector<T> rnd.distinct(size, upper);     // distinct in [0, upper-1]
std::vector<T> rnd.partition(size, sum);           // random composition, parts >= 1
std::vector<T> rnd.partition(size, sum, min_part); // parts >= min_part

shuffle(v.begin(), v.end());        // testlib's shuffle — deterministic, uses rnd
```

`rnd.partition` is the idiomatic way to satisfy a "sum of n over all test cases
≤ X" constraint; see `generators/gen-array-with-opt.cpp`.

`rnd.any` also accepts `std::set` / `std::multiset` (since 0.9.44).

### Output

Use the `println` family rather than `printf` — it handles containers,
iterator pairs, C arrays, and up to seven heterogeneous arguments:

```cpp
println(n);
println(a, b);                 // "a b\n"
println(vec);                  // space-separated, then newline
println(v.begin(), v.end());
std::string s = join(v.begin(), v.end(), ',');
```

### Command-line options

`registerGen` calls `prepareOpts` for you. Two styles:

```cpp
int n = opt<int>(1);              // positional: first non-option argument
int t = opt<int>(2);

int  cnt  = opt<int>("test-count");            // required named option
int  minN = opt<int>("min-n", 1);              // named with default
bool tree = opt<bool>("tree-only", false);
std::string mode = opt("mode", "random");
if (has_opt("sorted")) { /* ... */ }
```

Accepted argument forms (`testlib.h:5542`):

| Form | Example |
| --- | --- |
| `-key=value` / `--key=value` | `-n=10`, `--test-count=20` |
| `-key value` / `--key value` | `-n 10`, `--test-count 20` |
| `-kNumber` (single-char key) | `-n10` |
| `-flag` / `--flag` (means `true`) | `-sorted`, `--tree-only` |

Keys must start with a letter; `---key` is not an option. `bool` accepts
`true`/`false`/`1`/`0`.

**Typo protection:** calling `has_opt(...)` or any `opt(key, default)` overload
switches on an automatic `ensureNoUnusedOpts()` at program exit, which
`_fail`s if you passed an option the generator never read. That turns
`-min-vaule 5` from a silent no-op into a loud error. Turn it off with
`suppressEnsureNoUnusedOpts()` if you genuinely pass extra arguments (for
example, arguments used only to vary the seed).

### Multi-file generators

`startTest(k)` reopens stdout onto the file named `k`:

```cpp
for (int i = 1; i <= 10; i++) {
    startTest(i);          // subsequent output goes to the file "i"
    println(rnd.next(1, i * i), rnd.next(1, i * i));
}
```

See `generators/multigen.cpp`. Single-test stdout generators are preferred —
Polygon supports both, but stdout generators compose better.

### Sample generators in this repo

| File | Shows |
| --- | --- |
| `generators/igen.cpp` | minimal integer generator |
| `generators/iwgen.cpp` | `wnext` bias, with the semantics documented inline |
| `generators/sgen.cpp`, `swgen.cpp` | pattern-based strings, weighted length |
| `generators/bgen.cpp` | biased binary string |
| `generators/gs.cpp` | structured strings from repeat/period arguments |
| `generators/gen-array-with-opt.cpp` | the richest `opt<T>("key", default)` example, `rnd.partition` |
| `generators/gen-tree-graph.cpp`, `gen-rooted-tree-graph.cpp` | random trees, `wnext` for shape control, relabel + shuffle |
| `generators/gen-bipartite-graph.cpp` | positional `opt<int>(1..3)` |
| `generators/multigen.cpp` | `startTest` |

## 4. Validators

A validator reads the test file from **stdin** in strict mode and exits 0 if
and only if the file is legal. Every whitespace character matters: no trailing
spaces, exactly one `\n` between lines, no missing final newline.

```cpp
#include "testlib.h"

int main(int argc, char *argv[]) {
    registerValidation(argc, argv);
    inf.readInt(1, 100, "n");
    inf.readEoln();
    inf.readEof();
}
```

Always use the `registerValidation(argc, argv)` form. The no-argument overload
exists but makes `validator.testset()` / `validator.group()` fail and disables
all the `--test*` command-line features.

`inf.readEof()` is not optional: `TestlibFinalizeGuard` fails a validator that
returns without having consumed the whole file.

### Reading

```cpp
int    inf.readInt(minv, maxv, "n");
long long inf.readLong(minv, maxv, "n");
unsigned long long inf.readUnsignedLong(minv, maxv, "n");
double inf.readDouble(minv, maxv, "x");
double inf.readStrictDouble(minv, maxv, minDigitsAfterPoint, maxDigitsAfterPoint, "x");

std::vector<int>  inf.readInts(n, minv, maxv, "a");        // n ints, single spaces
std::vector<long long> inf.readLongs(n, minv, maxv, "a");

std::string inf.readToken();                     // == readWord()
std::string inf.readToken("[a-z]{1,100}", "s");  // pattern-checked
std::string inf.readLine();                      // == readString()
std::vector<std::string> inf.readTokens(n, "[a-z]+", "s");

inf.readSpace();  inf.readEoln();  inf.readEof();
inf.readChar('#');
bool inf.seekEof();  bool inf.seekEoln();
```

`readStrictDouble` requires the literal form `[-]digits[.digits]` with a
bounded number of fractional digits — no exponent, no `+`, no `.5`. Use it in
validators; use `readDouble` in checkers.

The vector forms (`readInts` and friends) require exactly one space between
elements in strict mode, and report violations as `a[3]` rather than "the 4th
integer", which is why they are preferred over a hand-rolled loop.

### Variable names earn their keep

The third argument is not decoration. It appears in error messages:

```
FAIL Integer element a[2] equals to 2000000000, violates the range [-10^9, 10^9] (test case 1, stdin, line 3)
```

and it drives **bounds-hit analysis**:

```sh
./validator --testOverviewLogFileName /dev/stdout < test01
```

```
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

Run that across your whole test set: any constraint that never reports both
`min-value-hit` and `max-value-hit` is a boundary no test reaches. The name
may not contain digits or control characters or it is not analysable.

Sometimes a bound genuinely cannot be hit and you do not want it flagged.
Prefix or suffix the name with `~`:

| Name | Meaning |
| --- | --- |
| `"t"` | report both bounds |
| `"~t"` | ignore the minimum bound |
| `"t~"` | ignore the maximum bound |
| `"~t~"` | ignore both |
| `"~~"` | a literal name `~` |

`tests/test-007_validators/` exists purely to pin this behaviour; read it if
you need the exact semantics.

Limits: at most 255 tracked variables (`VALIDATOR_MAX_VARIABLE_COUNT`).

### Multi-test-case inputs

```cpp
int t = inf.readInt(1, 10, "t");
inf.readEoln();
for (int tc = 1; tc <= t; tc++) {
    setTestCase(tc);          // messages now say "(test case 3, ...)"
    int n = inf.readInt(1, 10000, "n");
    inf.readEoln();
    inf.readInts(n, -1000, 1000, "a");
    inf.readEoln();
}
unsetTestCase();
inf.readEof();
```

`setTestCase` auto-detects 0-based vs 1-based numbering on the first call. It
also enables three command-line features:

```sh
./validator --testMarkupFileName markup.bin  < input   # annotate case boundaries
./validator --testCase 3 --testCaseFileName case3.txt < input   # extract one case
```

See `validators/case-nval.cpp`.

### Testsets and groups

```cpp
if (validator.testset() == "pretests")
    n = inf.readInt(1, 10, "n");
else
    n = inf.readInt(1, 100, "n");

if (validator.group() == "even-n-and-m")
    ensure(n % 2 == 0);
```

Invoked as `./validator --testset pretests --group even-n-and-m < input`.
See `validators/validate-using-testset-and-group.cpp`.

### Structural constraints

Ranges are not enough for graphs. Use `ensure` / `ensuref`:

```cpp
ensuref(sumN <= 200000, "sum of n is %lld, must be at most %d", sumN, 200000);
ensure(edges.count(std::make_pair(u, v)) == 0);   // no duplicate edge
```

`ensure(cond)` reports the condition text; `ensuref` takes a printf-style
message; `ensure_ext(cond)` adds the line number. See
`validators/undirected-graph-validator.cpp`,
`validators/undirected-tree-validator.cpp` (DSU connectivity check) and
`validators/bipartite-graph-validator.cpp`.

### Features

Declare optional structural properties and record which tests exhibit them:

```cpp
addFeature("has-self-loop");     // declare up front
...
if (u == v) feature("has-self-loop");
```

They show up in the `--testOverviewLogFileName` report. Marking an undeclared
feature is a `_fail`.

## 5. Checkers

Write a checker only when a stock one will not do. Twenty-one ready-made
checkers ship in `checkers/`:

| Checker | Compares |
| --- | --- |
| `wcmp` | sequences of tokens |
| `ncmp` | ordered sequence of `long long` |
| `uncmp` | **un**ordered sequence of `long long` |
| `icmp` | a single `int` |
| `hcmp` | a single huge integer (arbitrary precision, via pattern) |
| `lcmp` | files as sequences of lines, tokens compared within a line |
| `fcmp` | files as sequences of lines, exact string equality |
| `acmp`, `rcmp` | a single double, absolute error 1.5e-6 |
| `dcmp` | a single double, absolute **or** relative error 1e-6 |
| `rncmp` | sequence of doubles, absolute error 1.5e-5 |
| `rcmp4`, `rcmp6`, `rcmp9` | sequence of doubles, abs-or-rel 1e-4 / 1e-6 / 1e-9 |
| `yesno`, `nyesno` | one / many case-insensitive YES-NO answers |
| `caseicmp`, `casencmp`, `casewcmp` | `Case k: ...` multi-test formats |
| `pointscmp` | example of `quitp` partial scoring |
| `pointsinfo` | example of `quitpi` |

### Skeleton

```cpp
#include "testlib.h"

int main(int argc, char *argv[]) {
    setName("compares two signed integers");   // BEFORE registerTestlibCmd
    registerTestlibCmd(argc, argv);

    int ja = ans.readInt();
    int pa = ouf.readInt();
    if (ja != pa)
        quitf(_wa, "expected %d, found %d", ja, pa);
    quitf(_ok, "answer is %d", ja);
}
```

Command line: `checker <input> <output> <answer> [<report> [-appes]]`, plus
optional `--testset X` / `--group Y` (readable via `checker.testset()` /
`checker.group()`). `checker --help` prints usage.

### Reading participant output safely

- Range-check everything: `ouf.readInt(1, n, "index")` gives a clean `_wa`
  instead of an out-of-bounds crash.
- Echo participant tokens through `compress()` — it truncates a 10 MB garbage
  token to something printable.
- `expectedButFound(_wa, expected, found, "on line %d", i)` produces the
  standard "expected X, found Y" message with correct formatting per type.
- You do **not** need a final `seekEof()` check. On `_ok` testlib inspects the
  tail of `ouf` itself and converts to `_dirt` ("Extra information in the
  output file"). See [§9](#9-verdicts-and-exit-codes).

### Comparing doubles

```cpp
if (!doubleCompare(ja, pa, 1e-6))
    quitf(_wa, "expected %.10f, found %.10f, error = %.10f",
          ja, pa, doubleDelta(ja, pa));
```

`doubleCompare(expected, result, eps)` passes if **either** the absolute or the
relative error is within `eps` (plus 1e-15 slack). NaN matches only NaN;
infinities must match in sign. `doubleDelta` returns the smaller of the two
errors, falling back to absolute error when `|expected| <= 1e-9`.

### Sequences of unknown length

The idiom, from `checkers/ncmp.cpp`: compare while **neither** stream is at
EOF, then drain both and count what is left over, so you can say whether the
participant produced too few or too many values.

```cpp
int n = 0;
while (!ans.seekEof() && !ouf.seekEof()) {
    n++;
    long long j = ans.readLong();
    long long p = ouf.readLong();
    if (j != p)
        quitf(_wa, "%d%s numbers differ - expected: '%s', found: '%s'",
              n, englishEnding(n).c_str(), vtos(j).c_str(), vtos(p).c_str());
}

int extraInAns = 0;
while (!ans.seekEof()) { ans.readLong(); extraInAns++; }
int extraInOuf = 0;
while (!ouf.seekEof()) { ouf.readLong(); extraInOuf++; }

if (extraInAns > 0)
    quitf(_wa, "participant output is shorter: expected %d, found %d", n + extraInAns, n);
if (extraInOuf > 0)
    quitf(_wa, "participant output is longer: expected %d, found %d", n, n + extraInOuf);

quitf(_ok, "%d numbers", n);
```

`englishEnding(n)` yields `st`/`nd`/`rd`/`th`; `vtos(x)` stringifies any
value.

### Partial scoring

```cpp
quitp(12.5, "solved %d of %d subtasks", k, m);   // exit 7, score 12.5
quitp(0.75);                                     // exit 7
quitf(_pc(50), "half credit");                   // exit 50
quitpi("subtask1=ok;subtask2=wa", "details");    // exit 7, opaque info string
```

Points must be finite, non-negative and at most 1e6. Which of the two styles
your judge expects is judge-specific — Codeforces/Polygon uses `quitp` with
`_points`; ICPC-style systems using `TESTSYS` expect `_pc(x)` with
`PC_BASE_EXIT_CODE = 50`.

## 6. Interactors

```cpp
#include "testlib.h"
#include <iostream>

int main(int argc, char *argv[]) {
    setName("Interactor A+B");
    registerInteraction(argc, argv);

    int n = inf.readInt();              // the test file
    for (int i = 0; i < n; i++) {
        int a = inf.readInt(), b = inf.readInt();
        std::cout << a << " " << b << std::endl;   // to the solution; endl flushes
        tout << ouf.readInt() << std::endl;        // record for the checker
    }
    quitf(_ok, "%d queries processed", n);
}
```

Command line: `interactor <input> <output> [<answer> [<report> [-appes]]]`.

- `inf` — the test file.
- `ouf` — **stdin**, i.e. whatever the solution writes.
- `tout` — an `std::ofstream` on `argv[2]`; a checker runs afterwards on what
  you put there. If the interactor decides the verdict itself, `quitf(_wa, …)`
  and skip `tout`.
- **Flush after every write** (`std::endl`, or `fflush(stdout)`), or you
  deadlock.
- Dirt checking is skipped in interactor mode, so unread trailing output from
  the solution is not automatically an error.

To run an interactor against a solution locally you need a cross-runner that
wires two processes' pipes together. This repo ships one:
`tests/test-006_interactors/files/crossrun/CrossRun.jar` (Java, source
included), plus `tests/test-006_interactors/src/interactive_runner.py`, the
Google Code Jam-style Python runner. See `interactors/interactor-a-plus-b.cpp`.

## 7. Scorers

A scorer aggregates per-test results into one number, for problems whose score
is not a plain sum of per-test points.

```cpp
#include "testlib.h"
#include <vector>

int main(int argc, char *argv[]) {
    registerScorer(argc, argv, [](std::vector<TestResult> results) -> double {
        double total = 0;
        for (const TestResult &r : results)
            if (r.verdict == OK) total += r.points;
        return total;
    });
}
```

It reads serialized `TestResult` records from **stdin**, one per line, and
prints a single `%.3f` score. The record format is semicolon-separated:

```
index;testset;group;VERDICT;points;timeMs;memoryBytes;input;output;answer;exitCode;comment
```

`;`, `\`, CR and LF inside a text field are backslash-escaped, so the round
trip is lossless. An empty `points` field means "no points" (NaN).

```sh
$ printf '1;tests;;OK;60.000;12;2048;;;;0;\n2;tests;;OK;40.500;11;2048;;;;0;\n' | ./scorer
100.500
```

`TestResult` carries `testIndex`, `testset`, `group`, `verdict`, `points`,
`timeConsumed`, `memoryConsumed`, `input`, `output`, `answer`, `exitCode` and
`checkerComment`. `enum TestResultVerdict` is `SKIPPED, OK, WRONG_ANSWER,
RUNTIME_ERROR, TIME_LIMIT_EXCEEDED, IDLENESS_LIMIT_EXCEEDED,
MEMORY_LIMIT_EXCEEDED, COMPILATION_ERROR, CRASHED, FAILED` — note these are
unscoped names in the global namespace. `readTestResults(fileName)` reads the
same format from a file.

Malformed input is rejected with a `FAIL` verdict and exit code 3, naming the
offending field. Scoring runs inside `registerScorer`, so the score is printed
before `main` returns.

> Scorers were unusable before **0.9.46** — `registerScorer` did not mark
> itself registered, so every scorer aborted and then crashed. See `plan.md`
> for the history.

## 8. Patterns

A small regex-like language used by `rnd.next(pattern)` (generate a matching
string) and by every `read*` overload that takes a pattern (validate a
string). Implemented in `class pattern` (`testlib.h:734`, `1373-1656`).

| Syntax | Meaning |
| --- | --- |
| `a` | the literal character `a` |
| `[abc]` | one of `a`, `b`, `c` |
| `[a-z]` | a character in the range |
| `[^a-z]` | any character (codes 0..254) **not** in the range |
| `X{n}` | exactly `n` repetitions |
| `X{n,m}` | between `n` and `m` repetitions (needs `n <= m`) |
| `X?` | `{0,1}` |
| `X*` | `{0,INT_MAX}` |
| `X+` | `{1,INT_MAX}` |
| `A|B` | alternation |
| `( … )` | grouping |
| `\.` | escape a metacharacter |

Unescaped spaces are stripped from the pattern. A malformed pattern is a
`_fail`, not a silent mismatch.

```cpp
rnd.next("[a-zA-Z0-9]{1,1000}");
rnd.next("[0-9]{%d}", digits);          // printf-formatted pattern
inf.readToken("[a-z]{1,100}", "s");
pattern pnum("0|-?[1-9][0-9]*");        // reusable; see checkers/hcmp.cpp
if (!pnum.matches(token)) quitf(_pe, "…");
```

### It looks like regex and is not — read this before writing one

These are measured behaviours of 0.9.49, each pinned by a test in
`tests/test-004_use-test.h/tests/test-pattern-defects.cpp`. Full detail and
fix plans are in [`plan.md`](../plan.md).

| You write | You might expect | You actually get |
| --- | --- | --- |
| `[a-z]{3,}` | 3 or more | **exactly 3** — the open-ended form is not supported and does not warn |
| `[a-z]{1O}` (letter O) | an error | **`{1}`** — count parsing stops at the first non-digit and ignores the rest |
| `[a-z ]+` | letters and spaces | **`[a-z]+`** — spaces are stripped *everywhere*, including inside `[...]` |
| `No solution` | that literal string | **`Nosolution`** — same cause; escape it as `No\\ solution` |
| `\d+` | digits | **runs of the letter `d`** — there are no `\d`/`\w`/`\s`/`\n`/`\t` escapes |
| `[0-9]*[13579]` | any odd number | matches **nothing** — greedy, with no backtracking |
| `(ab)(cd)` | `abcd` | **construction fails** — a group must be the last element |

The greedy rule is the one that silently rejects correct submissions: the
first part takes the longest run it can and is never retried shorter, so
`[0-9]+0` ("a number ending in zero") can never match. A group may be preceded
by other elements — `x(ab)`, `[0-9](a|b)` are fine — but never followed by
one.

## 9. Verdicts and exit codes

```cpp
enum TResult {
    _ok = 0, _wa = 1, _pe = 2, _fail = 3, _dirt = 4,
    _points = 5, _unexpected_eof = 8, _partially = 16
};
#define _pc(exitCode) (TResult(_partially + (exitCode)))
```

Default process exit codes and messages:

| Verdict | Message prefix | Exit code | Notes |
| --- | --- | --- | --- |
| `_ok` | `ok ` | 0 | |
| `_wa` | `wrong answer ` | 1 | |
| `_pe` | `wrong output format ` | 2 | |
| `_fail` | `FAIL ` | 3 | jury/package error, never the participant's fault |
| `_dirt` | `wrong output format ` | **2** | rewritten to `_pe` at `testlib.h:3201`; `DIRT_EXIT_CODE` (4) is not used on this path |
| `_points` | `points ` | 7 | via `quitp` / `quitpi` |
| `_unexpected_eof` | `wrong output format ` | **2** | becomes 8 only with `-DENABLE_UNEXPECTED_EOF` |
| `_pc(x)` | `partially correct (x) ` | `PC_BASE_EXIT_CODE + x` (0 + x by default, 50 + x under `-DTESTSYS`) | |

All of these are overridable at compile time, e.g. `-DPE_EXIT_CODE=14`, and
they are remapped wholesale under `-DEJUDGE` and `-DCONTESTER`.

Quitting:

```cpp
quitf(_wa, "expected %d, found %d", ja, pa);
quitif(pa < 0, _wa, "negative answer %d", pa);
quit(_ok, "done");
ouf.quitf(_pe, "…");        // stream-scoped: verdict is adjusted per stream
ensuref(cond, "…", args);   // _wa on ouf, _fail elsewhere
__testlib_fail("…");        // always _fail
```

Remember the promotion rule: `inf.quitf(_wa, …)` and `ans.quitf(_wa, …)` both
come out as `_fail`.

## 10. Worked example

[`docs/examples/maxpos/`](examples/maxpos/README.md) is a complete package for
a multi-test-case problem where **any** index of the maximum element is
accepted — so it needs a real special checker. It contains a validator, two
generators, the checker, a correct and a wrong solution, and
`run-pipeline.sh`, which builds everything and demonstrates every verdict.

The validator, showing strict reading, per-case context, and a cross-cutting
`ensuref` constraint:

```cpp
#include "testlib.h"

int main(int argc, char *argv[]) {
    registerValidation(argc, argv);

    int t = inf.readInt(1, 10000, "t");
    inf.readEoln();

    long long sumN = 0;
    for (int testCase = 1; testCase <= t; testCase++) {
        setTestCase(testCase);
        int n = inf.readInt(1, 200000, "n");
        inf.readEoln();
        inf.readInts(n, -1000000000, 1000000000, "a");
        inf.readEoln();
        sumN += n;
    }
    unsetTestCase();

    ensuref(sumN <= 200000, "sum of n over all test cases is %lld, must be at most %d",
            sumN, 200000);
    inf.readEof();
}
```

The checker, showing all three streams working together:

```cpp
#include "testlib.h"
#include <vector>

int main(int argc, char *argv[]) {
    setName("MAXPOS: accepts any index of a maximum element");
    registerTestlibCmd(argc, argv);

    int t = inf.readInt();
    for (int testCase = 1; testCase <= t; testCase++) {
        int n = inf.readInt();
        std::vector<int> a(n);
        for (int i = 0; i < n; i++)
            a[i] = inf.readInt();

        int ja = ans.readInt(1, n, "index");   // out of range here => _fail
        int pa = ouf.readInt(1, n, "index");   // out of range here => _wa

        if (a[ja - 1] != a[pa - 1])
            quitf(_wa, "test case %d: jury answer %d has value %d, "
                       "but participant answer %d has value %d",
                  testCase, ja, a[ja - 1], pa, a[pa - 1]);
    }
    quitf(_ok, "%d test case(s)", t);
}
```

Run the whole thing:

```sh
cd docs/examples/maxpos && bash run-pipeline.sh
```

## 11. Gotchas

- **Line endings.** In strict mode (validators) `readEoln()` expects `\r\n` on
  Windows and `\n` elsewhere. A test file prepared on Windows will be rejected
  by a Linux validator. This repo's `.gitattributes` is a single line
  `*  binary`, precisely so git never rewrites line endings.
- **BOM.** `registerTestlibCmd` calls `ouf.skipBom()`, so a UTF-8 BOM in the
  participant's output is tolerated. `inf`/`ans` are not BOM-skipped
  automatically; call `skipBom()` yourself if you need it.
- **Token size.** `maxTokenLength` is 32 MB and `maxMessageLength` is 32000
  characters; long messages are truncated.
- **`-ffast-math`** is detected and rejected. So is a platform where
  `sizeof(long long) != 8`.
- **`rand`, `srand`, `std::random_shuffle`** are compile errors. Use `rnd` and
  testlib's `shuffle`.
- **Generators must be pure functions of their command line.** No files, no
  clock, no environment.
- **`registerValidation()` with no arguments** silently disables
  `--testset`/`--group`/`--testCase`. Always pass `argc, argv`.
- **`setName` after `registerTestlibCmd`** has no effect on the report header.
  Call it first.
- **Checkers must never `assert`.** Use `quitf(_fail, …)` so the judge gets a
  proper message instead of a signal.

### Known defects

[`plan.md`](../plan.md) in the repo root is an audit of `testlib.h` with
every confirmed bug, its trigger and its fix. The ones most likely to bite a
problem setter:

- **`rnd.next(0, 1)` repeats every 65536 draws under `registerGen(..., 1)`**
  (and `rnd.next(0, 3)` every 131072, and so on). Fixed in 0.9.47: use
  `registerGen(argc, argv, 2)`. Versions 0 and 1 keep the old behaviour on
  purpose, so existing packages still reproduce. *(plan.md R-01, R-02)*
- **The pattern language is not regex** — see the table in
  [§8](#8-patterns). *(P-01, P-02, P-03, I-06, A-01, A-02)*
- **`if (has_opt("flag"))` makes your generator fail at exit** with
  `Opts: unused key`. Use `opt<bool>("flag", false)` instead. *(O-02)*
- **`doubleCompare` accepts anything above 1e300** as equal to any other value
  above 1e300. *(F-03)*
- **`readLine()` silently drops a bare CR**, so `ab\rcd` and `abcd` compare
  equal. *(I-04)*
- **`readDouble()` returns `inf`** for input like `1e999`; range-check it
  yourself or use `readStrictDouble`. *(I-05)*

---

See also: [development guide](development-guide.md) for working on testlib
itself, and the sample sources in `checkers/`, `validators/`, `generators/`,
`interactors/`.
