# I-03 — `readInts`/`readStrings` allocate before reading

Design for fixing `plan.md` finding **I-03**, targeting testlib **0.9.53**.

Status: approved 2026-07-28, not yet implemented.

## Problem

`__testlib_readMany` (`testlib.h:3670`) is the macro behind all 24 public
`read*s` methods — `readInts`, `readLongs`, `readStrings`, `readWords`,
`readTokens`, `readReals`, `readDoubles`, `readStrictReals`,
`readStrictDoubles`, `readIntegers`, `readUnsignedLongs` and their overloads.
It reads:

```c
if (size < 0)
    quit(_fail, #readMany ": size should be non-negative.");
if (size > 100000000)
    quit(_fail, #readMany ": size should be at most 100000000.");

std::vector<typeName> result(size);
...
for (int i = 0; i < size; i++)
    result[i] = readOne;
```

Two independent defects.

**Eager allocation.** `std::vector<typeName> result(size)` is constructed
before a single token is consumed, and the cap is 10^8 **elements** rather than
bytes. A checker written the obvious way —

```cpp
int n = ouf.readInt();
std::vector<std::string> v = ouf.readStrings(n);
```

— against participant output of `100000000` followed by nothing allocates
10^8 × `sizeof(std::string)` = **3.2 GB** immediately (800 MB for
`long long`, 400 MB for `int`). The vector value-initialises, so those pages
are really touched. The checker is OOM-killed and the verdict becomes
"judgement failed" rather than a wrong-answer.

Per `plan.md`'s threat model, checkers and interactors run on judge machines
against untrusted participant output, so this is a security issue, not merely
a bug.

**Verdict.** A size outside `[0, 10^8]` raises `_fail`, so a participant who
prints `-1` or `200000000` forces a jury-error verdict rather than losing the
test.

## Decisions

| Question | Decision | Why |
| --- | --- | --- |
| Scope | Fix both halves | The verdict half is what stops a participant forcing a jury error; the allocation half is the OOM. |
| Size cap | Keep `[0, 10^8]`, report as `_pe` | Preserves a precise diagnostic instead of a generic unexpected-EOF. Once allocation is lazy the cap is a diagnostic, not a memory guard. |
| Verdict code | `_pe`, not `_wa` | A count that does not match the output contract is a format error. Matches the existing `maxFileSize` violation at `testlib.h:3515`, which is already `_pe`. |
| Reserve budget | 16 MB worth of elements | Same order as the static format buffer testlib already carries, and well under the 128 MB `maxFileSize`. One constant, easy to tune. |
| Pinning method | Replace global `operator new`/`delete` and count bytes | Deterministic and portable to g++/clang/MSVC. A behavioural test alone would still pass if someone reverted to eager allocation. |
| I-07 | Out of scope | `readLine()` ignoring `maxTokenLength` is the same family but a different function with a different fix. Bundling would muddy the changelog and the regression test. |

## The change

Rewrite the macro body:

```c
if (size < 0)
    quit(_pe, #readMany ": size should be non-negative.");
if (size > 100000000)
    quit(_pe, #readMany ": size should be at most 100000000.");

std::vector<typeName> result;
result.reserve(std::min(size_t(size),
                        size_t(16 * 1024 * 1024) / sizeof(typeName)));
readManyIteration = indexBase;

for (int i = 0; i < size; i++)
{
    result.push_back(readOne);
    readManyIteration++;
    if (strict && space && i + 1 < size)
        readSpace();
}

readManyIteration = NO_INDEX;
return result;
```

Two properties this relies on.

**The reserve is capped by bytes, not elements.** 16 MB buys 4M `int`s or
512K `std::string`s. This matters more than it appears: a flat *element* cap
would still let `readStrings` reserve gigabytes. With a byte cap, any read a
real problem performs is allocated exactly once with no growth churn, while a
hostile size costs a bounded 16 MB before the read fails on missing input.

**`quit(_pe, …)` needs no new stream plumbing.** The macro expands inside an
`InStream` member function, so it reaches `InStream::quit`, and the existing
promotion at `testlib.h:3300` —

```cpp
if (mode != _output && result != _fail)
```

— rewrites `_pe` to `_fail` on `inf` and `ans` while leaving it `_pe` on
`ouf`. The split plan.md asks for falls out of machinery already present. This
also means the change cannot make a broken *jury* file look like a
participant error.

## Trade-off accepted

Peak memory for a **legitimate** large read gets slightly worse. `push_back`
growth leaves up to ~2× the final size live during the last reallocation,
where `vector(size)` allocated exactly once.

It is bounded. `maxFileSize` caps input at 128 MB, so at most ~64M tokens,
so ~256 MB of `int`s and ~512 MB peak — comparable to the 400 MB the old code
allocated unconditionally whenever `size` was 10^8. Accepted deliberately
against a participant-triggered 3.2 GB.

## Tests — `tests/test-012_readmany/`

Follows `test-010_skipchar` and `test-011_eofc`: a standalone checker under
`src/`, fixtures under `files/`, and stdout/stderr/exit_code compared against
committed refs by `tests/scripts/test-ref`.

The program replaces global `operator new`/`delete` to count allocated bytes,
and is built with `TESTLIB_THROW_EXIT_EXCEPTION_INSTEAD_OF_EXIT` (as
`tests/test-004_use-test.h/test.cpp` already does) so it can catch the quit,
print the allocation verdict, and exit with the real code.

The allocation figure is reported **bucketed**, not exact: allocator internals
differ per platform, so the ref records `alloc: bounded` or `alloc: excessive`
against a 32 MB threshold. That is stable everywhere and still separates the
16 MB reserve cleanly from 400 MB.

| Ref | Call | After fix | Before fix |
| --- | --- | --- | --- |
| `lazy-huge` | `ouf.readInts(100000000)`, 3 ints present | exit 2, `alloc: bounded` | 400 MB allocated |
| `lazy-huge-strings` | `ouf.readStrings(100000000)`, 3 tokens present | exit 2, `alloc: bounded` | 3.2 GB / OOM |
| `over-cap` | `ouf.readInts(100000001)` | exit 2 | exit 3 |
| `negative` | `ouf.readInts(-1)` | exit 2 | exit 3 |
| `inf-promotes` | `inf.readInts(-1)` | exit 3 | exit 3 |
| `normal` | `ouf.readInts(3)`, valid input | exit 0, values correct | same |

`inf-promotes` passes both before and after on purpose. It pins the property
that makes the `_pe` change safe: a bad size on jury data must still be
`_fail`. If someone later removes the promotion at `3300`, this test catches
it.

Refs must be generated locally and committed — CI sets
`TEST_REF_FORBID_GEN_REFS=true` and a missing ref is a hard failure.

## Compatibility

- **Exit codes change** for a checker that passes an out-of-range size while
  reading `ouf`: previously `FAIL` (3), now `wrong output format` (2). This is
  the point of the change, and it requires a `VERSION` bump.
- `inf` and `ans` behaviour is unchanged — still `_fail`, still exit 3.
- No in-repo caller reads from `ouf`; every existing `readInts`/`readLongs`
  call site in `validators/` and `tests/` reads `inf`. Existing refs should
  not move. To be confirmed by running the full suite.
- The macro is shared, so all 24 methods change uniformly. That is intended;
  a per-method split would be worse.

## Bookkeeping

- `VERSION` → `0.9.53` (`testlib.h:30`), prepend to `latestFeatures[]`, and
  add an entry to the fork changelog block above it.
- Delete the I-03 block from `plan.md`; add a *Fixed — 0.9.53* entry
  describing both halves.
- `docs/usage-guide.md`: note the verdict change in §5 and add a gotchas
  entry.
- `docs/development-guide.md`: add the `test-012_readmany` row to the
  test-directory table.

## Out of scope

- **I-07** — `readLine()` ignores `maxTokenLength`.
- **R-08** — the separate eager allocation in `rnd.distinct` at
  `testlib.h:1403`. Same shape, different subsystem, generator-side rather
  than participant-facing.
- Lowering `maxFileSize` or making the cap adaptive to remaining input. That
  was considered and rejected: the byte count is not reliably known for
  stdin-backed streams, which is exactly the interactor case, and `plan.md`
  I-14 notes `maxFileSize` is not enforced on the `FILE*` init path at all.
