# Coding standards

These are the rules a reviewer has to judge for themselves. Rules a script can
check are listed at the end with where they are documented; don't re-review
them by eye.

Background: `docs/development-guide.md` (how to change the library) and
`docs/adr/` (decisions not to re-argue).

## Changes to testlib.h

- **Keep a fork patch small and isolated** (ADR 0001). A diff that reformats,
  renames or moves code it did not need to touch makes the next upstream
  rebase harder; flag it even when the result reads better.
- **A random stream must not change** (ADR 0002). Any diff that touches
  `random_t`, `setSeed`, `nextBits` or `registerGen` must leave
  `tests/test-003_run-rnd/refs/` byte-identical. A ref change there is a
  compatibility break, never a test fix.
- **Hold checker and interactor paths to the security bar** (ADR 0008). Ask
  whether participant output can make the change crash, hang or allocate
  without bound. A read failure on jury data must still end in a fail verdict
  and never in wrong answer.
- **Match the error text of the surrounding code.** Judges and the refs
  under `tests/` read testlib's messages, so a message change is
  user-visible.
- **A user-visible change bumps `VERSION`** and adds a `latestFeatures[]`
  entry, plus a row in the fork's own changelog block above it. Behaviour
  breaks (as in 0.9.51 and 0.9.52) say so in the entry.

## Findings and tests

- **Fixing a finding is one change.** It flips the finding's pinned test
  (ADR 0007), deletes its block from `plan.md`, and re-anchors any line
  numbers the change moved. A fix that leaves the pinned test asserting the old
  behaviour, or the block still in `plan.md`, is incomplete.
- **New behaviour is covered by a test**, in
  `tests/test-004_use-test.h/tests/` when it can run in-process, otherwise in a
  new `tests/test-NNN_*/` directory using `../scripts/compile` and
  `../scripts/test-ref`.
- **Refs are read before they are committed.** A regenerated ref that changed
  more than the change explains is a finding in its own right.
- **Do not pin undefined behaviour.** A reproducer that invokes UB stays in
  `plan.md` as "do not add to the suite".

## Docs

- **Claims about `testlib.h` cite `file:line` and are verified** against the
  current header. Line numbers drift; a wrong citation is worse than none.
- **Use the vocabulary in `GLOSSARY.md`**: test versus test case, random
  generator version versus testlib version, finding versus issue.

## Checked mechanically (documented elsewhere)

Each of these is a fixed pattern a script can check. They are not CI checks
yet; each is a candidate for one. Until then, run the commands in
`docs/development-guide.md` §7.

| Rule | Where |
| --- | --- |
| Every `.cpp` outside `tests/` compiles under `-Wpedantic -Werror` on C++11–23 | `tests/test-000_compile-all-cpp` (CI) |
| Missing refs fail CI | `TEST_REF_FORBID_GEN_REFS=true` (CI) |
| No deprecated CRT calls (`scanf`, `fopen`, …) in files that don't include `testlib.h` | dev guide §7 item 7 (grep) |
| No Windows macro names (`small`, `near`, `far`, …) as identifiers | dev guide §7 item 8 |
| The version string is updated in all seven places | dev guide §7 item 5 (grep) |
| `rand`, `srand`, `std::random_shuffle` are not used | poisoned by `testlib.h` itself |
| `tests/lib/testlib.h` and `.gitattributes` are untouched | ADRs 0005, 0006 |
