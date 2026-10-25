# testlib

testlib is a library for preparing competitive programming problems. This
repository is a fork that maintains the library and audits it for defects.

## Problem packages

**Problem package**:
Everything a judge needs to run one problem: its tests, the programs that
produce and check them, and the reference solutions.
_Avoid_: problem, task (when you mean the package)

**Test**:
One input file in a problem package.
_Avoid_: test case (that is a different concept), testcase, input

**Test case**:
One of several independent instances packed into a single multi-case test.
_Avoid_: test (that is the whole file), query

**Testset**:
A named collection of tests within a package, such as `pretests` or `tests`.
_Avoid_: test suite

**Group**:
A named subset of a testset, scored or validated together.
_Avoid_: subtask, batch

**Jury**:
The problem's authors, who write the tests, the reference solution and the
programs in the package.
_Avoid_: setter, author (when you mean the role)

**Participant**:
Whoever submitted the solution being judged.
_Avoid_: contestant, user, solver

**Jury answer**:
The output of the reference solution on a test, used by the checker as a
point of comparison.
_Avoid_: expected output, answer file, correct output

**Participant output**:
What the participant's solution printed for a test.
_Avoid_: contestant output, user output, actual output

**Jury data**:
Anything the jury authored, meaning the test and the jury answer. Jury data
that does not parse means the package is broken, never that the participant
is wrong.

## Roles

**Generator**:
A program that produces one test, determined entirely by its command line.
_Avoid_: test maker

**Validator**:
A program that decides whether a test is legal under the problem's
constraints.
_Avoid_: input checker

**Checker**:
A program that judges participant output against the test and the jury
answer.
_Avoid_: grader, special judge, comparator

**Interactor**:
A program that converses with a running solution in place of a fixed input
file.
_Avoid_: interactive checker

**Scorer**:
A program that combines the results of every test into one score.
_Avoid_: aggregator

## Verdicts

**Verdict**:
The outcome a checker, validator or interactor reports for one test.
_Avoid_: result, status (status is reserved for findings)

**Fail**:
The verdict for an error in the package or the jury data. Never the
participant's fault.
_Avoid_: crash, internal error

**Wrong output format**:
The verdict for participant output that does not have the required shape.
_Avoid_: PE, presentation error (as a verdict separate from this one)

**Points**:
A verdict carrying a numeric partial score.
_Avoid_: partial (that is partially correct)

**Partially correct**:
A verdict carrying an integer grade on a judge-defined scale.

## Randomness

**Random generator version**:
The numbered random stream a generator chooses (0, 1 or 2). Each version is
frozen once published.
_Avoid_: version (alone), rnd version

**testlib version**:
The release number of the library itself, such as 0.9.52.
_Avoid_: version (alone)

**Random stream**:
The exact sequence of values a random generator version produces from a seed.

**Seed**:
The starting state of the random stream, derived from the generator's whole
command line.

**Option**:
A named `-key value` argument passed to a generator.
_Avoid_: flag, parameter

## Validation

**Pattern**:
testlib's own language for describing the allowed shape of a token. It
resembles a regular expression and is not one.
_Avoid_: regex, regular expression

**Variable name**:
The name a validator gives to a value it reads, used in error messages and in
bounds-hit analysis.

**Bounds hit**:
Whether some test reaches a variable's declared minimum or maximum.
_Avoid_: coverage

**Feature**:
A structural property the validator declares and records per test, such as
"has a self-loop".
_Avoid_: tag

**Test markup**:
A record of where each test case begins inside a multi-case test.

## Maintaining the fork

**Upstream**:
The original repository, `MikeMirzayanov/testlib`, from which this fork
diverged at testlib version 0.9.45.
_Avoid_: origin (that is this fork's own remote)

**Fork patch**:
A change carried by this fork and not yet in upstream.

**Finding**:
One audited defect, ambiguity or risk in the library, with an ID such as
`I-03`, recorded in `plan.md` until it is fixed.
_Avoid_: issue (that is a GitHub issue), bug (a finding may be an ambiguity)

**Status**:
How thoroughly a finding has been verified: measured, traced or reported.

**Pinned test**:
A test that asserts a finding's current, defective behaviour, so the suite
stays green until the fix lands.
_Avoid_: known-failure test, xfail

**Ref**:
The stored standard output, standard error and exit code that a test run is
compared against.
_Avoid_: golden file, snapshot, expected output
