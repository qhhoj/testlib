# testlib development guide

For working **on** testlib itself — modifying `testlib.h`, adding samples, or
maintaining this fork. If you only want to *use* testlib to prepare a problem,
read the [usage guide](usage-guide.md) instead.

- [1. What this repository is](#1-what-this-repository-is)
- [2. Layout](#2-layout)
- [3. Anatomy of testlib.h](#3-anatomy-of-testlibh)
- [4. The test harness](#4-the-test-harness)
- [5. What each test directory covers](#5-what-each-test-directory-covers)
- [6. CI](#6-ci)
- [7. Change checklist](#7-change-checklist)
- [8. Extending these docs](#8-extending-these-docs)

---

## 1. What this repository is

- **testlib 0.9.48**, MIT licensed, copyright Mike Mirzayanov.
- Upstream: `https://github.com/MikeMirzayanov/testlib`. This checkout's
  `origin` is the fork `https://github.com/qhhoj/testlib.git`.
- Forked from upstream at `1e4e8a2` (0.9.45), 457 commits back to 2008-09-14.
  **This fork now diverges:** 0.9.46 fixes the scorer API, and 0.9.47/0.9.48 add
  random generator version 2 — see `plan.md`.
- The library is used by Codeforces, the Russian National Olympiad in
  Informatics, and ICPC regionals; Polygon (the problem-preparation system)
  is built around it.

**Treat `testlib.h` as vendored upstream code.** Before patching it, check
whether the change belongs upstream. If you must carry a local patch, keep it
small and isolated so rebasing onto a new upstream release stays cheap:

```sh
git remote add upstream https://github.com/MikeMirzayanov/testlib.git
git fetch upstream
git log --oneline master..upstream/master
```

## 2. Layout

| Path | What it is |
| --- | --- |
| `testlib.h` | The whole library. One header, 6006 lines, ~200 KB. |
| `checkers/` | 21 ready-made checkers, also serving as samples. |
| `validators/` | 8 sample validators. |
| `generators/` | 11 sample generators. |
| `interactors/` | 1 sample interactor. |
| `tests/` | Test suite and harness. |
| `docs/` | These guides. `docs/read.me` is a stale byte-copy of the root `README.md`. |
| `read.me` | Legacy plain-text readme (points at the dead Google Code URL). |
| `.github/workflows/ci.yml` | The only CI workflow. |

There is **no build system**: no `Makefile`, and `CMakeLists.txt` is
gitignored. Everything is compiled ad hoc with `-I<repo-root>`.

Two `.gitignore` files matter: the root one (`.idea`, `cmake-build-debug`,
`*.exe`, `*.o`, `*.obj`, `CMakeLists.txt`) and `checkers/.gitignore`, which
lists the built binary name of every stock checker.

**`.gitattributes` is a single line: `*     binary`.** Every file in the repo
is treated as binary by git, so line endings are never normalized — which is
essential, because `tests/test-001_run-sval-case-nval/files/` holds `unix/`
and `win/` variants of the same inputs that differ only in EOLN. The side
effect is that `git diff` shows "Binary files differ" for everything,
including markdown. Do not "fix" this.

## 3. Anatomy of `testlib.h`

Approximate map (line numbers as of 0.9.48):

| Lines | Contents |
| --- | --- |
| 30 | `#define VERSION "0.9.48"` |
| 65+ | `const char *latestFeatures[]` — the reverse-chronological changelog |
| 248–307 | Exit-code macros (`OK_EXIT_CODE`, `WA_EXIT_CODE`, …) with `EJUDGE` / `CONTESTER` / `TESTSYS` variants |
| 333–364 | `format` buffer, `__TESTLIB_MAX_TEST_CASE`, test-case globals |
| 431–664 | `upperCase`/`lowerCase`, `doubleCompare`, `doubleDelta`, `vtos`, `toString`, `toHumanReadableString` |
| 716, 1297–1580 | `class pattern` — declaration and implementation |
| 766–1362 | `class random_t` — the whole `rnd` API |
| 1695–1737 | `TMode`, `TResult`, `TTestlibMode`, `_pc()`, `outcomes[]` |
| 1771–2072 | Input readers: `StringInputStreamReader`, `FileInputStreamReader`, `BufferedFileInputStreamReader` |
| 2077–2448 | `class InStream` |
| 2449–2451 | `InStream inf, ouf, ans;` |
| 2461–2844 | `ValidatorBoundsHit`, `ConstantBound(s)`, `class Validator`, global `validator` |
| 2849–2902 | `TestlibFinalizeGuard` |
| 2957–3001 | `setTestCase`, `unsetTestCase`, `resultExitCode` |
| 3125–3283 | `InStream::quit` / `quitf` / `quitif` / `quits` — where verdict rewriting and dirt checking happen |
| 4497–4602 | Global `quit`, `quitp`, `quitpi`, `quitf`, `__testlib_help` |
| 4621 | `__testlib_ensuresPreconditions()` |
| 4669–4979 | `registerGen`, `registerInteraction`, `registerValidation`, `registerTestlibCmd`, `registerTestlib` |
| 4841, 4818–4830 | `class Checker`, global `checker` |
| 4998–5032 | `__testlib_ensure`, `ensure` / `ensure_ext` macros, `ensuref`, `__testlib_fail`, `setName` |
| 5046–5092 | `shuffle`, and the poisoned `random_shuffle` / `rand` / `srand` |
| 5100–5148 | `startTest`, `compress`, `englishEnding`, `join` |
| 5259–5316 | `expectedButFound` and its specializations |
| 5396–5502 | The `println` family |
| 5504–5977 | Command-line options: `TestlibOpt`, `parseOpt`, `prepareOpts`, `has_opt`, `opt<T>` |
| 5980–6284 | Scorer: `TestResultVerdict`, `TestResult`, serialization, `readTestResults` |
| 6286 | `registerScorer` |
| 6313–6358 | `opt<T>(key, default)`, `ensureNoUnusedOpts`, `suppressEnsureNoUnusedOpts` |
| 6364–6403 | `testlib_format_`, `format()` (with a `std::format` path under C++20) |

Two behaviours worth knowing before you change anything in the quit path:

- `_dirt` is rewritten to `_pe` at line 3125, so `DIRT_EXIT_CODE` (4) never
  reaches `resultExitCode` on that path and the observed exit code is 2.
- `__testlib_shouldCheckDirt` (2990) fires for `_ok`, `_points` and
  `_partially`, and is skipped in interactor mode.

**Rule for any user-visible change:** bump `VERSION` (line 28) and prepend a
one-line entry to `latestFeatures[]` (line 65). That array is the only
changelog the project has.

## 4. The test harness

Everything lives under `tests/`. There is no framework dependency; it is bash
plus one compiled comparator.

### Running

```sh
cd tests
bash t.sh                      # shortcut for: run.sh g++ 11 v0
bash run.sh                    # every compiler found, every standard
bash run.sh g++                # g++ only, all standards
bash run.sh g++ 17             # g++ with --std=c++17
bash run.sh g++ 17 test-007_validators   # one test directory
bash run.sh clang++ v14 20     # clang++-14 with --std=c++20
bash run.sh g++ 11 32          # 32-bit build (-m32)
```

Argument grammar (`tests/run.sh:27`), all free-form and combinable:

| Argument | Meaning |
| --- | --- |
| `g++` \| `clang++` \| `msvc` | compiler; at most one |
| `11` \| `14` \| `17` \| `20` \| `23` | C++ standard; repeatable |
| `vNN` | try the `-NN` suffix, e.g. `v14` → `g++-14`; `v0` means no suffix |
| `32` \| `64` | pass `-m32` / `-m64`; at most one |
| `test-*` | restrict to those test directories; repeatable |

Notes:

- **`java -version` must succeed** — `run.sh` checks it up front
  (`tests/run.sh:5`) because `test-006_interactors` needs Java for the
  cross-runner. The check is unconditional: without a JRE, `run.sh` refuses to
  start **even when you filter to a single unrelated test**. On a machine
  without Java, drive one test directory directly instead:

  ```sh
  cd tests/test-007_validators
  TESTS_DIR=/path/to/testlib/tests MACHINE=Linux \
  CPP=g++ CPP_STANDARD=--std=c++17 CPP_INCLUDE_DIR=/path/to/testlib \
  CPP_OPTS="" INVOCATION_ID=$RANDOM VALGRIND="" \
      bash run.sh
  rm -rf ../tester-lcmp        # run.sh normally cleans this up for you
  ```

  Everything except `test-006_interactors` works this way.
- If `valgrind` is on `PATH` it is auto-enabled (`VALGRIND="valgrind -q"`) and
  every referenced invocation runs under it. This slows things down a lot but
  catches memory errors.
- Without a version filter, `run.sh` probes suffixes `0, 6..20` for each
  compiler and runs every one it finds.
- On Windows it discovers MSVC by scanning
  `Program Files\Microsoft Visual Studio\<2000..2100>\{Professional,Enterprise,Community}\VC\Auxiliary\Build\vcvars{32,64}.bat`,
  sourcing the environment through `tests/file-runner.py`, then driving
  `cl.exe`. It also probes `/c/Programs/*/bin/g++.exe`.
- If nothing ran, it exits 1 with `[ERROR] No compilers found`.

Environment exported to each test's `run.sh`: `TESTS_DIR`, `TEST_DIR`,
`CPP`, `CPP_STANDARD`, `CPP_OPTS`, `CPP_INCLUDE_DIR` (the repo root),
`MACHINE` (`Linux`/`Mac`/`Windows`), `VALGRIND`, `INVOCATION_ID`.

### `tests/scripts/compile`

Compiles one `.cpp` into a binary named after it:

```
$CPP $CPP_OPTS $CPP_STANDARD -Wpedantic -Werror -I$CPP_INCLUDE_DIR -o<exe> -O2 <src>
```

- `--check-only` as the second argument uses `-O0` (`-Od` for MSVC) and
  deletes the binary afterwards.
- `TESTLIB_COMPILER_OPTIMIZATION_OPT` overrides the optimization digit.
- Adds `-static` when the compiler lives in an absolute `.../bin` directory or
  on Windows.
- MSVC path: `cl.exe <std> -F268435456 -EHsc -O2 -I<inc> -Fe<exe> <src>`.

`-Wpedantic -Werror` is why every sample in `checkers/`, `validators/`,
`generators/` and `interactors/` must be warning-free on every supported
compiler and standard.

### `tests/scripts/test-ref` — the golden-output harness

```sh
bash ../scripts/test-ref <ref-name> "$VALGRIND" ./program arg1 arg2 …
```

It runs the command, captures stdout, stderr and the exit code, and compares
all three against `refs/<ref-name>/{stdout,stderr,exit_code}`.

- Comparison uses `tests/src/tester-lcmp.cpp` (line-by-line, token-within-line),
  compiled once per `INVOCATION_ID` and cached in `tests/tester-lcmp/`.
- **`tester-lcmp` is deliberately built against `tests/lib/testlib.h`, which is
  pinned at `VERSION "0.9.40-SNAPSHOT"`.** The comparator must not depend on
  the header under test. Never sync that file with the root `testlib.h`.
- If `refs/<ref-name>/` does not exist, it **generates** the reference files
  from the current run — unless `TEST_REF_FORBID_GEN_REFS=true`, which CI
  sets. So: run the test locally to produce refs, inspect them, commit them.
  A missing ref is a hard CI failure with
  `"Run test locally, it will produce ref files and push it into the repo"`.

## 5. What each test directory covers

Note that `005` is used twice; the directory name, not the number, is what
`run.sh` filters on.

| Directory | Covers |
| --- | --- |
| `test-000_compile-all-cpp` | `find`s every `*.cpp` under the repo root except those under `tests/` and compiles each `--check-only`. This is what keeps the sample checkers/validators/generators warning-free. **Any `.cpp` you add anywhere else in the repo — including `docs/examples/` — joins this sweep and must compile warning-free on every supported compiler and standard.** Currently 47 files. |
| `test-001_run-sval-case-nval` | Validators over `files/{unix,win}/` inputs; exercises `--testMarkupFileName`, `--testCase 1..4`, `--testCaseFileName`. |
| `test-002_run-fcmp-wcmp` | `wcmp` and `fcmp` on matching/mismatching files, plus a BOM'd output asserted to yield identical refs. |
| `test-003_run-rnd` | A large smoke test of the whole `rnd` surface (`next`/`wnext` for every type, `perm`, `distinct`, `partition`, `any`/`wany` over vector/set/multiset, `shuffle`, patterns). **This is the reproducibility guarantee for `rnd`** — if a ref changes here, generated tests everywhere in the world change. |
| `test-004_use-test.h` | The unit-test framework. See below. |
| `test-005_no-register` | `println` and `rnd.perm` used *without* `registerGen`. |
| `test-005_opts` | 9 invocations each of `files/test-auto-ensure-no-unused-opts.cpp` and `files/test-suppress-auto-ensure-no-unused-opts.cpp` — typo'd (`-min-val`, `-max-val`, `-bias-value`) and unused option keys. Refs are nested (`refs/auto-ensure-no-unused-opts/r1` …). |
| `test-006_interactors` | Compiles its own `src/interactor-a-plus-b.cpp`, feeds it a canned participant file, then runs a real two-process interaction between it and `src/interactive-a-plus-b.cpp` through `files/crossrun/CrossRun.jar` (`CrossRun.java` and `build-cross-run.sh` included). Output is `tr -d '\r'`-normalized before comparison. `src/interactive_runner.py` is a Google Code Jam-style alternative runner kept alongside. |
| `test-007_validators` | `v1..v4.cpp` differ only in the `~` bound-skip annotations (`"t"`, `"~t"`, `"n~"`, `"~t~"`); each runs 6 inputs with `--testOverviewLogFileName stderr`. |
| `test-008_format` | The `format()` / `std::format` fallback paths added in 0.9.44. |

### `test-004_use-test.h` — where new tests should go

`tests/README.md` says it explicitly: **prefer adding a case here.** The design:

- `test.cpp` defines `TESTLIB_THROW_EXIT_EXCEPTION_INSTEAD_OF_EXIT` before
  including `testlib.h`, so a `quit()` becomes a catchable exception instead of
  terminating the process, then `#include`s each file in `tests/`.
- `test.h` provides a `TEST(name) { … }` macro with static self-registration,
  `ensure_exit(code, lambda)` to assert a program-terminating path, and
  `run_tests()`.
- Existing suites: `test-join`, `test-split`, `test-tokenize`, `test-opts`
  (the largest), `test-instream`, `test-pattern`, `test-stringToLongLong`,
  `test-stringToUnsignedLongLong`.

Adding a case is: write `tests/test-<thing>.cpp` with `TEST(...)` blocks, add
the `#include` to `test.cpp`, run the suite.

### `tests/docker/`

Six images for reproducing failures on compilers you do not have:
`gcc-7`, `gcc-latest`, `clang-3.5`, `clang-7`, `clang-11`, `clang-latest`.
Each has a `Dockerfile` (installs `git default-jre valgrind`), a `startup.sh`
that clones upstream and runs `./run.sh g++ v0 23` on a chosen branch
(default `dev-mikemirzayanov`), and `build.bat`/`run.bat` Windows helpers.

Because `startup.sh` clones from GitHub rather than mounting the working tree,
push your branch first — or edit the script to mount a volume.

## 6. CI

One workflow, `.github/workflows/ci.yml`, named `CI`.

- Triggers: `workflow_dispatch`, `push`, `pull_request`.
- `paths-ignore: docs/**, LICENSE, read.me, README.md` — **documentation-only
  changes do not run CI.**
- Global `env: TEST_REF_FORBID_GEN_REFS: true`.
- Every job is `actions/checkout@v3` then `cd tests && bash ./run.sh <compiler> v<version> [32]`.

Active matrix:

| OS | Jobs |
| --- | --- |
| ubuntu-22.04 | g++ 10, 11, 12 · clang++ 13, 14 |
| ubuntu-22.04 `-m32` | g++ 9, 10, 11 (installs `gcc-N-multilib`) · clang++ 13, 14 (installs `gcc-multilib`) |
| macos-14 | g++ 13, 14, 15 · clang++ (no suffix) |
| macos-15 | g++ 13, 14, 15 · clang++ |
| macos-26 | g++ 13, 14, 15 · clang++ |
| windows-2019 | msvc · g++ · clang++ |
| windows-2022 | msvc · g++ · clang++ |

All ubuntu-20.04 jobs (g++ 9/10, clang++ 10/11/12 and their `-m32` variants)
are commented out rather than deleted. Recent history is mostly matrix pruning
as GitHub retires runner images — expect to keep doing that.

## 7. Change checklist

1. Make the change in `testlib.h`.
2. Add a test. If it can be expressed as a unit test, put it in
   `tests/test-004_use-test.h/tests/`; otherwise create a `test-NNN_*/`
   directory with a `run.sh` that uses `../scripts/compile` and
   `../scripts/test-ref`.
3. Run locally across standards:
   ```sh
   cd tests && bash run.sh g++ 11 14 17 20 23
   cd tests && bash run.sh clang++ 11 17 23
   ```
   The build is `-Wpedantic -Werror`, so a warning on any standard is a
   failure. Watch for MSVC-only issues too (`sscanf_s`, `-EHsc`); if you cannot
   test MSVC, say so in the PR.
4. If you added or changed a ref-based test, the first local run generates
   `refs/*`. **Read them, then commit them** — CI cannot generate them.
5. Bump `VERSION` and prepend to `latestFeatures[]` for any user-visible
   change.
6. If you touched anything a sample demonstrates, update the sample in
   `checkers/` / `validators/` / `generators/` / `interactors/` — they are all
   compiled by `test-000_compile-all-cpp`.
7. Remember that a docs-only commit will show CI as not-run, not as passing.

Things that are load-bearing and easy to break by accident:

- `tests/lib/testlib.h` (pinned 0.9.40-SNAPSHOT) — leave it alone.
- `.gitattributes` (`* binary`) — leave it alone.
- `test-003_run-rnd` refs — a change here means every generator in the world
  produces different tests. If a ref moves, that is a compatibility break and
  needs a `latestFeatures[]` entry saying so.
- Exit-code macros — judges depend on the exact numbers.

## 8. Extending these docs

`docs/` is meant to grow. Conventions:

- One topic per file, kebab-case, `.md`.
- Add a row to the table in [`docs/README.md`](README.md).
- Runnable examples go under `docs/examples/<name>/` with a `README.md` and a
  script that builds and runs everything, following
  [`docs/examples/maxpos/`](examples/maxpos/README.md).
- Keep example sources compiling under the repo's own flags
  (`-std=c++17 -Wpedantic -Werror -O2 -I<repo-root>`).
- Cite `file:line` for claims about `testlib.h`, and verify them — the line
  numbers here are for 0.9.46 and will drift.
