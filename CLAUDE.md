# CLAUDE.md

## What this repository is

**testlib 0.9.47** — a single-header C++ library (`testlib.h`, ~6000 lines,
MIT) for preparing competitive programming problems: generators, validators,
checkers, interactors and scorers.

This checkout is a mirror of upstream `MikeMirzayanov/testlib` via the fork
`qhhoj/testlib`, currently with **zero local divergence**. Treat `testlib.h`
as vendored upstream code — do not modify it casually, and prefer sending
fixes upstream.

## Where to look

| Question | File |
| --- | --- |
| How do I write a generator / validator / checker / interactor? | `docs/usage-guide.md` |
| How do I change testlib itself, run its tests, read its CI? | `docs/development-guide.md` |
| Show me a complete working problem package | `docs/examples/maxpos/` |
| Known bugs in testlib.h, and the roadmap for fixing them | `plan.md` (repo root) |
| Index of all docs | `docs/README.md` |

`plan.md` is a living audit: each finding is a block with a reproducer and a
fix, and **a block is deleted when its finding is fixed**. Entries marked
`Pinned by:` have a regression test in `tests/test-004_use-test.h/tests/` that
asserts the *current, buggy* behaviour so CI stays green — flip the test in the
same change that fixes the bug.

## Layout

```
testlib.h        the entire library
checkers/        21 stock checkers (wcmp, ncmp, rcmp6, yesno, …), also samples
validators/      8 sample validators
generators/      11 sample generators
interactors/     1 sample interactor
tests/           bash test harness + 10 test directories
docs/            guides and worked examples
```

## Building

There is no build system. No `Makefile`; `CMakeLists.txt` is gitignored.

```sh
g++ -std=c++17 -O2 -I/home/lam_n/Projects/testlib file.cpp -o file
```

The repo's own harness compiles with `-Wpedantic -Werror -O2`. Match it.

## Running the tests

```sh
cd tests
bash t.sh                                # shortcut for run.sh g++ 11 v0
bash run.sh g++ 17                       # one compiler + standard
bash run.sh g++ 17 test-004_use-test.h   # one test directory
```

Arguments are free-form: compiler (`g++`/`clang++`/`msvc`), standard
(`11 14 17 20 23`), version suffix (`v14` → `g++-14`, `v0` → none), bitness
(`32`/`64`), and `test-*` filters.

**`run.sh` refuses to start without a JRE** — it runs `java -version` up front
for `test-006_interactors`, unconditionally, even when you filter to an
unrelated test. There is no JRE on this machine. Workaround for any test
directory except `test-006`: set the environment yourself and run its
`run.sh` directly — see "The test harness" in `docs/development-guide.md`.

## Rules that are easy to violate by accident

- **New `.cpp` anywhere outside `tests/` joins the CI compile sweep**
  (`tests/test-000_compile-all-cpp`), including files under `docs/`. It must
  compile warning-free under `-Wpedantic -Werror` on C++11 through C++23.
- **Ref-based tests:** `tests/scripts/test-ref` generates
  `refs/<name>/{stdout,stderr,exit_code}` when they are missing locally, but
  CI sets `TEST_REF_FORBID_GEN_REFS=true`. Generate them locally, read them,
  and commit them — a missing ref is a hard CI failure.
- **`tests/lib/testlib.h` is intentionally pinned at `0.9.40-SNAPSHOT`.** It
  builds the reference comparator, which must not depend on the header under
  test. Never sync it with the root header.
- **`.gitattributes` is `*  binary`**, so git never normalizes line endings
  (the suite has `unix/` and `win/` fixtures that differ only in EOLN). The
  side effect is that every `git diff` reads "Binary files differ", including
  for markdown. Do not change it.
- **`test-003_run-rnd` refs pin `rnd` reproducibility.** If one moves, every
  generator in the world produces different tests — that is a compatibility
  break, not a test fix.
- **Any user-visible library change** must bump `VERSION` (`testlib.h:28`) and
  prepend an entry to `latestFeatures[]` (`testlib.h:65`).
- **Docs-only commits do not run CI** — `.github/workflows/ci.yml` has
  `paths-ignore: docs/**, LICENSE, read.me, README.md`.
- New library features should be covered by a test, preferably in
  `tests/test-004_use-test.h/tests/`.

## Quick API reminders

- Checkers register with `registerTestlibCmd` — there is **no**
  `registerChecker`.
- **Use `registerGen(argc, argv, 2)` for new generators.** Versions 0 and 1
  repeat `rnd.next(0, 1)` every 65536 draws. All three streams are frozen
  compatibility surfaces pinned by `tests/test-003_run-rnd/`; never change the
  output of an existing version.
- A read failure on `inf` or `ans` is promoted to `_fail`, never `_wa`.
- `rand`, `srand` and `std::random_shuffle` are deliberately poisoned; use
  `rnd` and testlib's `shuffle`.
- Default exit codes: `_ok` 0, `_wa` 1, `_pe` 2, `_fail` 3, `_points` 7.
  `_dirt` and `_unexpected_eof` both surface as 2 by default.
