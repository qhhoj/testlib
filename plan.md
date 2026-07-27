# testlib.h audit — findings and roadmap

An audit of `testlib.h` for bugs, ambiguities and security issues, with a
prioritised plan for addressing them.

Started against **v0.9.45** (6252 lines, commit `1e4e8a2`). Line numbers are
kept current with the working tree — they now refer to **v0.9.46** (6338
lines). Re-check them after any rebase on upstream.

**Fixed so far — 0.9.47:** R-01 (`rnd.next(0, 1)` repeated every 65536 draws).
Fixed behind an opt-in **`registerGen(argc, argv, 2)`**; versions 0 and 1 stay
byte-identical, as `tests/test-003_run-rnd` proves. Version 2 has its own
reference files there, so it is now a compatibility surface too.

> **Fold R-07 into version 2 before anyone ships a generator on it.**
> `setSeed(argc, argv)` casts `argv[i][j]` (a `char`) to `unsigned`, so the
> same command line seeds differently on x86 and ARM. It is gated by the same
> version switch, and changing version 2's output afterwards would defeat the
> point of versioning it.

**Fixed so far — 0.9.46:** S-01, S-02, S-03, S-04 (the scorer API). It was
completely non-functional: `registerScorer` never marked itself registered, so
every scorer aborted and then segfaulted. Fixed by setting `registered`, moving
the scoring out of the static destructor and into `registerScorer` (so a bad
input file fails cleanly instead of hitting **Q-02**), replacing the raw
`const char*` throw, validating every deserialized field, and making the escape
codec round-trip CR losslessly. Covered by `tests/test-009_scorer/` and
`tests/test-004_use-test.h/tests/test-scorer-serialization.cpp`.

## How to use this file

- Each finding is a self-contained block with an ID, a severity, a status, a
  reproducer and a proposed fix.
- **When a finding is fixed, delete its block from this file.** That is the
  progress mechanism — this file shrinks as the work lands.
- Entries marked **`Pinned by:`** have a regression test in
  `tests/test-004_use-test.h/tests/` that asserts the *current, buggy*
  behaviour, so CI stays green. When you fix the finding, flip its test to
  assert the correct behaviour in the same change, then delete the block here.
- Entries marked **`Reproducer (do not add to the suite)`** trigger undefined
  behaviour. Do not pin them — a test that invokes UB pins a crash.

### Status vocabulary

| Status | Meaning |
| --- | --- |
| `measured` | Reproduced by building and running a program. Numbers quoted are from a run. |
| `traced` | Verified by reading the relevant source paths end to end. Not executed. |
| `reported` | Surfaced by the audit sweep; the cited lines are right, but re-verify before acting. |

### Ground rules for fixes

1. **`rnd` output is a compatibility surface.** Versions 0 and 1 must stay
   byte-identical forever — `tests/test-003_run-rnd` pins them, and every
   existing problem package on every judge depends on them. Any change to the
   random stream lands behind a **new `registerGen(argc, argv, 2)`**, exactly
   as version 1 was introduced to fix version 0.
2. Anything user-visible needs a `VERSION` bump (`testlib.h:28`) and a
   `latestFeatures[]` entry (`testlib.h:65`).
3. Threat model: **checkers and interactors run on judge machines against
   untrusted participant output.** A crash, hang or multi-GB allocation there
   is a security issue, not just a bug. Validators and generators consume
   jury-authored data and are held to a lower bar.

---

---

### O-01 · `-n=-e-5` constructs an iterator before `begin()` · CRITICAL · traced

**Where:** `testlib.h:5627-5676` (`parseExponentialOptValue`), `5718-5727`
(`optValueToLongDouble`)

**What:** traced end to end for the value `"-e-5"`:

1. `pos = 1`, `e = "-5"`, `ne = -5`, `num = s.substr(0, 1) = "-"` (`5645`).
2. `optValueToLongDouble("-")` at `5650` **does not reject it**: `s[0] == '-'`
   sets `sign = -1, pos = 1`, and the digit loop `for (i = pos; i < s.length())`
   never executes, so it returns `-0.0` (`5724-5749`).
3. `num[0] == '-'` strips the sign, leaving `num == ""` (`5652-5654`).
4. The negative-exponent loop runs 5 times; the first iteration finds no `.` and
   evaluates
   `num.insert(num.begin() + int(num.length()) - 1, '.')` =
   `num.insert(num.begin() - 1, '.')` — **an iterator before `begin()`** (`5670`).

The sibling positive-exponent loop at `5656` is safe because it only appends.

**Reproducer (do not add to the suite):** `./gen -n=-e-5` with any numeric
`opt<>` read of `n`.

**Impact:** UB in the option parser, reachable from the command line. On
libstdc++ this is a `memmove` with an underflowed index — heap corruption or a
segfault in a generator.

**Fix:** reject a bare sign in `optValueToLongDouble` (require at least one
digit), and guard the insert loop against an empty `num`. Cover the related
`S-03` cases (`"-e5"`, `".e2"`) at the same time.

---

# High

### R-02 · with `version == 0`, bit 31 of `nextBits(63)` is never set · HIGH · measured

**Where:** `testlib.h:800-805`

**What:** `lowerBitCount` is 31 for version 0, so `right` occupies bits 0..30
and `left` bits 32..62 — bit 31 is always zero. `next(long long n)`'s rejection
sampling assumes `bits` is uniform on [0, 2^63), which it is not.

**Measured:** `rnd.next(0, 1)` under version 0 has period **131072** (2^17)
rather than 65536 — the whole distribution is shifted by one bit, confirming
the different `lowerBitCount` path is live.

**Impact:** for any `n > 2^31`, `bits % n` is biased; `next(0LL, 4294967295LL)`
can never return a value ≥ 2^31 — half the range is unreachable. Affects anyone
compiling pre-0.8.7 generators, including via the deprecated two-argument
`registerGen` (`4668`).

**Fix:** versions 0 and 1 must stay byte-identical, so this is
**documentation only**. `registerGen(..., 0)` and `(..., 1)` exist to reproduce
existing packages; new generators should use version 2, which never reads
low-order state bits. Documented in `docs/usage-guide.md` §3.

---

### P-01 · `{n,}` silently means `{n,n}` · HIGH · measured

**Where:** `testlib.h:1437-1445`, `1470-1471`

**What:** the scan loop pushes `part` when it hits `,`, then the tail is pushed
only `if (part != "")`. For `"{3,}"` the tail is empty, so `parts == {"3"}`,
`parts.size() == 1`, and `from = to = 3`.

**Trigger:** `inf.readToken("[a-z]{3,}", "s")` — intended as "3 or more
letters" — accepts **only** length-3 tokens and rejects every valid longer one.
`rnd.next("[a-z]{3,}")` always produces exactly 3 characters.

**Impact:** silent inversion of standard regex semantics inside a validator. No
diagnostic. Note that the symmetric `{,5}` *does* fail loudly (`1458-1459`),
which makes the asymmetry look accidental rather than designed.

**Pinned by:** `tests/test-004_use-test.h/tests/test-pattern-defects.cpp`

**Fix:** either support `{n,}` as `{n, INT_MAX}` (and then `pattern::next` must
reject it like `*`, see `1408-1410`), or `__testlib_fail` on it. Failing loudly
is the safer choice and matches `{,5}`.

---

### P-02 · pattern counts accept trailing garbage · HIGH · measured

**Where:** `testlib.h:1460-1467`

**What:** `sscanf(parts[i].c_str(), "%d", &number) != 1` only checks that *a*
number was parsed; it never verifies the whole part was consumed.

**Trigger:** all of these silently become `{1}`:
`[a-z]{1O}` (capital O for zero) · `[a-z]{1e9}` · `a{3x}` · `[0-9]{1;5}`.
Additionally `{99999999999}` is `sscanf` UB — the value is not representable in
`int`.

**Impact:** a typo in a length bound silently weakens a validator, which then
accepts short or malformed tokens.

**Pinned by:** `tests/test-004_use-test.h/tests/test-pattern-defects.cpp`

**Fix:** parse with `strtol` and require the entire part to be consumed and in
range; `__testlib_fail` otherwise.

---

### P-03 · a group may only be the trailing element, and the error is misleading · MEDIUM · measured

**Where:** `testlib.h:1574-1599`, falling through to `1558`

> **Correction.** The audit sweep reported this as a *silent* misparse —
> `pattern("(ab)(cd)")` supposedly becoming the literal string `(ab)(cd)`.
> That is wrong; measurement shows it **fails loudly** at construction. The
> real defect is narrower: the restriction is undocumented and the diagnostic
> quotes a string the user never wrote. Severity lowered from HIGH.

**What:** the group branch at `1597` requires `firstClose + 1 == s.length()`,
so a `(` that does not open a whole-pattern group falls through to
`__pattern_scanCharSet` and is consumed as a literal. The *remainder* of the
pattern then has unbalanced parentheses, and the recursive construction hits
`__testlib_fail` at `1588` or `1595` a level or two down.

**Measured** — a group must be the last element; a prefix before it is fine:

| Pattern | Result |
| --- | --- |
| `(ab\|cd)` | works |
| `x(ab)` | works, matches `xab` |
| `xy(a\|b)` | works, matches `xya` |
| `[0-9](a\|b)` | works, matches `1a` |
| `((a))` | works |
| `(ab)(cd)` | **fails at construction** |
| `(ab)x` | **fails at construction** |
| `(a\|b)(c\|d)` | **fails at construction** |
| `(a\|b)xy` | **fails at construction** |

**Impact:** groups cannot be concatenated or followed by anything — a real
expressiveness limit that nothing documents. And the message names a mangled
substring: `pattern("(a|b)xy")` reports
`Illegal pattern (or part) "a|b)xy"`, sending the author looking for a pattern
they never wrote.

**Pinned by:** `tests/test-004_use-test.h/tests/test-pattern-defects.cpp::pattern_group_must_be_trailing`

**Fix:** support concatenation of groups. Failing that, detect the case in the
top-level constructor and fail with a message quoting the **original** pattern
and saying plainly that a group must be the final element. Document the
restriction in `docs/usage-guide.md` §8.

---

### I-01 · `isEof(inf.curChar())` is always false on x86 · HIGH · traced

**Where:** `testlib.h:242` (`#define EOFC (255)`), `1621-1623` (`isEof`),
`3370-3377` (`InStream::curChar` / `nextChar`)

**What:** the reader classes signal EOF as `int` 255, but `InStream::curChar()`
and `nextChar()` narrow to `char`. Where `char` is signed (x86, x86-64, MSVC),
`char(255) == -1` and `isEof(c)` evaluates `-1 == 255` → **false, always**.
Where `char` is unsigned (ARM, PowerPC) it works. Same source, opposite
behaviour per architecture.

`isEof` has **zero uses inside the library** — the internal readers correctly
keep everything in `int`. It exists purely as a public helper for user code.

**Trigger:** `while (!isEof(ouf.curChar())) { ...; ouf.skipChar(); }` — the
documented low-level idiom.

**Impact:** the loop never terminates on x86, so the checker spins until the
judge's time limit kills it, producing "judgement failed" rather than a verdict.
Participant-triggerable.

**Fix:** make `EOFC` `(-1)` and widen `InStream::curChar`/`nextChar`/`readChar`
to `int`, or delete `isEof` from the public surface. Both are API breaks, so
pair with a `VERSION` bump and a `latestFeatures[]` note.

---

### I-02 · `skipChar()` has no bounds check and no refill · HIGH · traced

**Where:** `testlib.h:1947-1952` (`increment`), `1995-1997` (`skipChar`),
`3406-3408` (`InStream::skipChar`), `1999-2007` (`unreadChar`)

**What:** `BufferedFileInputStreamReader::increment()` does
`buffer[bufferPos++]` with **neither `refill()` nor a `bufferPos < bufferSize`
test**. Every other accessor (`curChar`, `nextChar`, `eof`) refills first.
Internally `skipChar` is always paired with a refilling `curChar` (e.g.
`skipBlanks`, `3410-3413`), so the library itself is safe — but
`InStream::skipChar()` is public and documented ("Moves stream pointer one
character forward", `2076-2077`) and forwards straight through.

**Trigger:**
```cpp
int k = ouf.readInt();                     // participant-controlled
for (int i = 0; i < k; i++) ouf.skipChar(); // k > 2000000 walks off the buffer
```

**Impact:** out-of-bounds heap read; and once `bufferPos` exceeds
`BUFFER_SIZE`, a subsequent `unreadChar` — which guards only `bufferPos < 0`,
never the upper bound — performs an out-of-bounds heap **write** at an
attacker-influenced offset.

**Fix:** one line — make `skipChar()` call `refill()` and bounds-check, the same
as `nextChar()`. Add an upper-bound guard to `unreadChar`.

---

### I-03 · `readInts`/`readStrings` allocate before reading · HIGH · traced

**Where:** `testlib.h:3472-3478` (`__testlib_readMany`)

**What:** `std::vector<typeName> result(size);` is allocated up front, before a
single token is consumed, with the cap at 10^8 **elements** rather than bytes.

**Trigger:** participant output `100000000` followed by nothing, against a
checker doing `int n = ouf.readInt(); auto v = ouf.readStrings(n);` — 10^8 ×
`sizeof(std::string)` = **3.2 GB** immediately (800 MB for `long long`,
400 MB for `int`).

**Impact:** the checker is OOM-killed → "judgement failed" instead of a verdict.
Separately, a size outside `[0, 1e8]` raises `_fail`, letting a participant
force a jury-error verdict rather than a WA.

**Fix:** `reserve` a bounded amount and `push_back` as tokens are read, so the
allocation tracks real input. Change the out-of-range verdict from `_fail` to
`_pe`/`_wa` when the size came from `ouf`.

---

### O-02 · `has_opt` arms the unused-opts check but never marks the opt used · HIGH · measured

**Where:** `testlib.h:5759-5762` (`has_opt`), `5618` (`__testlib_keyToOpts`,
the only place `used` is set), `6312-6316` (`autoEnsureNoUnusedOpts`)

**What:** `has_opt` sets `__testlib_ensureNoUnusedOptsFlag = true` and returns
`__testlib_opts.count(key) != 0` — without touching `used`. So the very call
that enables the check guarantees the check will fail.

**Trigger:**
```cpp
registerGen(argc, argv, 1);
if (has_opt("sorted")) { /* ... */ }     // ./gen -sorted
```

**Impact:** the generator writes its entire test to stdout, then dies at exit
with `FAIL Opts: unused key 'sorted'`, exit 3 — leaving a plausible-looking but
officially-failed test file behind (see **A-11**). The documented "is this flag
present" idiom is self-defeating. Workaround today: `suppressEnsureNoUnusedOpts()`
or a dummy `opt<bool>("sorted")`.

**Pinned by:** `tests/test-004_use-test.h/tests/test-opts-defects.cpp`

**Fix:** mark the opt used in `has_opt`, or split into `has_opt` (marks used) and
a separate non-arming query. See also **O-03**, **O-06**.

---

### O-03 · positional `opt(i)` never marks keys used · HIGH · reported

**Where:** `testlib.h:5812-5818`, `5868-5871` (`__testlib_indexToArgv`)

**What:** index-based access reads `__testlib_argv` and never touches
`__testlib_opts`. If any code path also calls `has_opt` or `opt(key, default)`
— which arms the auto-check — every positionally-consumed option is reported as
unused.

**Trigger:** `./gen -n 5 --mode fast` with
`int n = opt<int>(2); bool f = opt<bool>("flag", false);` → `FAIL Opts: unused
key 'n'` at exit, after the test was already emitted.

**Impact:** mixing the two access styles is silently unsupported and fails late.

**Fix:** mark the corresponding key used when an index resolves to one, or
reject mixing the two styles explicitly and early.

---

### O-04 · `-k10` parses three different ways depending on what follows · HIGH · measured

**Where:** `testlib.h:5539-5549` (`parseOpt`)

**What:** the two-token lookahead branch is tried first, so form 3 (single-char
key with an inline numeric value) applies only when the next argv is itself an
option, or absent:

| Command line | Parsed as |
| --- | --- |
| `./gen -n10 -m20` | `n = 10`, `m = 20` |
| `./gen -n10 20` | key **`"n10"`** `= "20"` |
| `./gen -n10` | `n = 10` |

**Impact:** `opt<int>("n")` fails with `unknown key 'n'` in the middle case; if
the generator uses `opt<int>("n10", default)` it silently takes the default.

**Pinned by:** `tests/test-004_use-test.h/tests/test-opts-defects.cpp`

**Fix:** the grammar needs to be statable in one sentence. Either drop form 3 or
make it unconditional; add an explicit `--` end-of-options terminator (there is
none today). This is an API break — pair with a version bump.

---

### Q-01 · `_pc(x)` exit codes collide with OK/WA/PE/FAIL · HIGH · traced

**Where:** `testlib.h:298-304` (`PC_BASE_EXIT_CODE`), `1675` (`_pc`),
`2958-2959` (`resultExitCode`)

**What:** `PC_BASE_EXIT_CODE` is 0 unless `TESTSYS` is defined, and
`resultExitCode` returns `PC_BASE_EXIT_CODE + (r - _partially)`.

| Call | Exit code | Collides with |
| --- | --- | --- |
| `quitf(_pc(0), …)` | 0 | `OK_EXIT_CODE` |
| `quitf(_pc(1), …)` | 1 | `WA_EXIT_CODE` |
| `quitf(_pc(2), …)` | 2 | `PE_EXIT_CODE` |
| `quitf(_pc(3), …)` | 3 | `FAIL_EXIT_CODE` |

`_pc(256)` truncates to exit status 0 = OK. `_pc(-1)` yields `TResult(15)`,
below `_partially`, and hits `quit(_fail, "What is the code ??? ")` (`3173`).

**Impact:** a partial-score checker run outside Codeforces (no `-DTESTSYS`, no
appes result file) reports "accepted" for `_pc(0)` and "judge failure" for
`_pc(3)`. Only the appes XML (`3184-3186`) carries the real value.

**Fix:** default `PC_BASE_EXIT_CODE` to 50 as under `TESTSYS`, or reject `x`
values whose exit code collides. Until then, document it — already noted in
`docs/usage-guide.md` §9.

---

### V-01 · validator bounds analysis silently stops at 255 variables · HIGH · reported

**Where:** `testlib.h:2586-2593`, and the same shape at `2596-2597`
(`addVariable`) and `2612-2613` (`adjustConstantBounds`)

**What:** the guard reads

```cpp
if (isVariableNameBoundsAnalyzable(variableName)
        && _boundsHitByVariableName.size() < VALIDATOR_MAX_VARIABLE_COUNT) {
```

The size test is evaluated **before** the lookup, so once the map holds 255
entries the condition is false for *every* subsequent call — including updates
to variables that are already being tracked.

**Impact:** past 255 variables, bounds silently stop being recorded, and
Polygon's test-overview log reports bounds as un-hit for variables that were in
fact hit. The three caps are independent, so the three logs can disagree about
which variables even exist. No warning anywhere.

**Fix:** check membership first and only apply the cap to *new* entries; emit an
explicit `truncated` line in the overview log when the cap is reached, so the
consumer knows the report is incomplete.

---


---

# Medium

### R-03 · signed overflow in `next(long long, long long)` · traced
`testlib.h:925-927`, `937-939`. `to - from + 1` is computed in `long long` and
overflows for wide ranges: `rnd.next(LLONG_MIN, LLONG_MAX)`. `next(int, int)` at
`915` widens first, so the fix pattern is already known and simply not applied
here. **Reproducer (do not add to the suite).** Fix: detect the overflow and
`__testlib_fail` with a message naming the overload the user actually called.

### R-04 · `wnext(int, int, int)` computes the span in `int` · traced
`testlib.h:1122-1125`, and the same at `1136`, `1151`. `rnd.wnext(-15e8, 15e8, 1)`
is UB, while the plain `rnd.next(-15e8, 15e8)` works — an asymmetry between
sibling functions. **Reproducer (do not add to the suite).**

### R-05 · `wnext(unsigned, unsigned, int)` is declared returning `int` · traced
`testlib.h:1129-1133`. `next(unsigned, unsigned)` at `920` correctly returns
`unsigned int`. Values above `INT_MAX` come back negative:
`rnd.wnext(3000000000u, 4000000000u, 1)`. Also `to - from + 1` is `unsigned`
arithmetic, so `rnd.wnext(0u, UINT_MAX, 1)` wraps to 0 and fails. Fix: correct
the return type — source-compatible for every in-range use.

### R-06 · signed overflow in `distinct` and `partition` · reported
`testlib.h:1230` (`to - from + 1` evaluated in `T`, then widened),
`1278` (`min_part * size` overflows *before* the guard can fire), `1287`
(`sum + size - 1`). Some cases wrap into a state the post-hoc self-checks at
`1303`/`1306` do not catch, so the corruption is silent.
**Reproducer (do not add to the suite).**

### R-07 · `setSeed(argc, argv)` is not reproducible across architectures · traced
`testlib.h:818-830`. `(unsigned int)(argv[i][j])` converts a **`char`**: byte
`0xC3` becomes 4294967235 where `char` is signed and 195 where it is unsigned.
**The same command line therefore produces different tests on x86 and ARM** for
any argument containing a byte ≥ 0x80 — directly contradicting testlib's core
promise. Additionally the mixing is one LCG step per byte with no avalanche, so
command lines differing only in the last character produce seeds differing only
in low bits — which is exactly what **R-01** reads, making `gen 1` and `gen 2`
produce correlated coin-flip streams. Dead `random_t p;` at `819`.
Fix: cast through `unsigned char` and mix properly, behind version 2.

### R-08 · `distinct` can spend O(size) divisions then allocate gigabytes · reported
`testlib.h:1234-1250`. The strategy heuristic costs O(size) divisions just to
*choose* a strategy, then `rnd.distinct(100000000, 0, 999999999)` allocates
~10^8 red-black nodes (4–5 GB). Fix: `unordered_set` or a sparse Fisher–Yates map.

### I-04 · `readLine()` silently swallows a bare CR · measured
`testlib.h:4302-4318`. When a CR is not followed by LF it is consumed by
`nextChar()` and the fall-through appends the character *after* it. So
`ouf.readLine()` maps both `ab\rcd` and `abcd` to `"abcd"` — two distinct
participant outputs compare equal in any line-based checker (`fcmp`, `lcmp`).
`\r\r\n` loses one CR. **Pinned by:** `test-instream-defects.cpp`.

### I-05 · `readDouble()` returns ±inf · measured
`testlib.h:3706`. `stringToDouble` checks `__testlib_isNaN` but not
`__testlib_isInfinite`; `stringToStrictDouble` **does** check it (`3787`) — an
explicit inconsistency between siblings. A participant printing `1e999` makes
`ouf.readDouble()` return `inf`, and the usual checker arithmetic
(`fabs(ja - pa)`, `pa / ja`) then yields NaN, so every comparison silently takes
the false branch. **Pinned by:** `test-instream-defects.cpp`.

### I-06 · pattern matching is greedy with no backtracking · measured
`testlib.h:1364-1377`, `1383-1399`. `__pattern_greedyMatch` takes the longest
run and `matches` never retries a shorter one. Verified by hand:
`pattern("[0-9]*[13579]").matches("13")` is **false**; so are
`readToken("[0-9]+0")` ("a number ending in 0") and `[a-z]*[abc]` against
anything. Documented at `707-708`, but the consequence is a checker that rejects
correct answers, so the practical severity is high. **Pinned by:**
`test-pattern-defects.cpp`. Fix: implement backtracking, or detect the
unsatisfiable shape at construction and `__testlib_fail`.

### I-07 · `readLine()` ignores `maxTokenLength` · reported
`testlib.h:4298-4324` grows `result` unbounded, unlike `readWordTo` (`3438-3440`)
which enforces the cap. A 128 MB single-line participant output costs ~400 MB
across geometric growth plus the by-value return at `4327-4328`.

### I-08 · report text is unsanitised outside appes mode · reported
`testlib.h:3196-3197`, `3218` write the message with a bare `%s`; only the XML
branch runs it through `xmlSafeWrite` (`3269-3272`). Participant-controlled ANSI
escape sequences therefore reach judge logs and operator terminals. Separately,
`xmlSafeWrite` tests `0 <= msg[i] && msg[i] <= 31` on a **signed** `char`, so
bytes `0x80..0xFF` fall through to a raw `%c` write and can make the XML report
ill-formed under the declared encoding — a participant-triggerable "judgement
failed". Fix: sanitise in both branches; validate bytes against the encoding.

### I-09 · testlib's own message delimiters are forgeable · reported
`testlib.h:2384-2385` define `OPEN_BRACKET = char(11)` and
`CLOSE_BRACKET = char(17)`. `isBlanks` (`1632-1634`) accepts neither, so both
are legal inside a token read by `readWord`. A participant emitting
`\x0b…\x11` makes `__testlib_appendMessage` (`3048-3051`) splice
`", test case N"` into attacker-controlled text, or suppress the annotation
entirely. The verdict and exit code are unaffected; the human-readable report is
spoofable. Fix: strip these bytes from participant text before embedding it.

### I-10 · `FileInputStreamReader` cannot distinguish `0xFF` from EOF · reported
`testlib.h:1802-1807` maps `EOF` to `EOFC (255)`, which is exactly what `getc`
returns for a literal `0xFF` byte. This reader backs `stdin` — i.e. **interactors
reading participant output**. One `0xFF` byte fakes end-of-input. The buffered
reader returns a signed `char` instead, so the two readers disagree on identical
data. Same root cause as **I-01**; fix together.

### P-04 · `[^…]` is a complement over 0..254 and is architecture-dependent · traced
`testlib.h:1545-1553`. Three problems: the loop runs `code < 255`, so byte
`0xFF` matches no negated set; code 0 is included, so `rnd.next("[^ ]{10}")` can
write NUL, LF and CR into a test file; and `char c = char(code)` is signed on
x86 and unsigned on ARM, so `std::sort` orders the set differently and
`pattern::next` picks **different bytes per architecture from the same seed**.
Fix: use a 256-bit bitmap indexed by `unsigned char` — this kills the
signedness dependence and speeds up `__pattern_greedyMatch` (**P-09**).

### P-05 · `pattern::next` accepts an explicit huge count · traced
`testlib.h:1408-1415`. The guard catches `to == INT_MAX` (so `*` and `+` fail
cleanly) but not `rnd.next("[a-z]{0,2147483646}")`, which appends ~2×10^9
characters one at a time. Fix: cap the generated length and fail loudly.

### P-06 · pattern construction recurses one level per token · reported
`testlib.h:1599`, `1614`. `children.push_back(pattern(s.substr(pos)))` makes the
tail a nested `pattern`, so depth equals the token count, with an O(n²) `substr`
chain and an O(n²) space-stripping loop (`1565-1567`). A ~100k-character pattern
stack-overflows; `next()` and `matches()` recurse to the same depth at runtime.

### O-05 · an option expecting a value silently becomes boolean `true` · measured
`testlib.h:5542-5548`. `./gen -count -verbose` yields `count = "true"`.
`opt<int>("count")` then fails loudly (fine), but `opt<bool>("count", false)`
returns `true` — silently wrong. Negative numbers *are* handled correctly
(`getOptType("-5")` returns 0, `5485-5495`), but the rule "a value may not start
with `-` followed by a letter" is undocumented.
**Pinned by:** `test-opts-defects.cpp`.

### O-06 · `opt<bool>(key)` alone defaults silently · measured
`testlib.h:5911-5913`. `opt<int>("x")` on a missing key FAILs with
`Opts: unknown key 'x'`; `opt<bool>("x")` silently returns `false`. It also
calls `has_opt`, so it arms the global unused-opts check as a **type-dependent
side effect** — a single `opt<bool>` can make an unrelated typo fatal at exit
where `opt<int>` would not. **Pinned by:** `test-opts-defects.cpp`.

### O-07 · `isalpha` on a possibly-negative `char` · reported
`testlib.h:5489`, `5492`. Passing a negative value to `isalpha` is UB — only
`unsigned char` values and `EOF` are permitted. Trigger: `./gen -é`. glibc
tolerates it; the MSVC debug CRT asserts. **Reproducer (do not add to the suite).**

### O-08 · integer overflow happens before the overflow check · reported
`testlib.h:5707`. `value = T(value * 10 + s[i] - '0')` overflows for signed `T`
*before* the `about` sanity check at `5712` can detect it — the guard is a
post-hoc detector of UB that already occurred. Under `-fsanitize=undefined` the
generator aborts instead of reporting "Opts: integer overflow".
**Reproducer (do not add to the suite).**

### O-09 · opts are silently generator-only · reported
`testlib.h:4642` is the only call site of `prepareOpts`. In checkers,
validators, interactors and scorers `__testlib_opts` stays empty, so
`opt<int>("x", 5)` returns `5` forever and `opt(1)` fails with
`index '1' is out of range [0,0)`. Nothing documents this. Fix: call
`prepareOpts` from the other registration functions, or fail loudly when `opt`
is used outside a generator.

### V-02 · test-case marker 256 is mis-encoded · reported
`testlib.h:2706` and `2747` use `if (c <= 256)` against the encoding at `1848`
(`readChars.push_back(testCase + 256)`), so marker 256 is treated as a character
and `char(256) == '\0'`. Reachable via `setTestCase(0)` after a first
`setTestCase(1)` (the global at `2917-2934` only latches `zero_based` on the
first call). Fix: `c < 256`.

### V-03 · markup and test-case files truncate at the first NUL · reported
`testlib.h:2728`, `2775` use `fprintf(f, "%s", …)` on `std::string`s built from
raw input bytes, which may legitimately contain `\0`. Everything after the first
NUL is silently dropped from the file Polygon consumes. Fix: `fwrite(s.data(),
1, s.size(), f)`.

### V-04 · validator logs have no escaping · reported
`testlib.h:2625`, `2641`, `2661`, `2606`. `isVariableNameBoundsAnalyzable`
(`2492-2497`) rejects digits and control characters but not `"` or `\`, so a
variable name can forge arbitrary lines in the test-overview log. More
realistically, an accidental `"` makes the log unparseable.

### V-05 · log write errors are ignored · reported
`testlib.h:2685-2690`, `2728-2729`, `2775-2776` check only `fclose`. On a full
disk `fprintf` fails, `fclose` succeeds, and the validator exits 0 with a
silently truncated log. When the target is `stdout`/`stderr` (`2676`, `2719`,
`2766`) `fclose` is deliberately skipped, so those paths have **no** error
detection at all.

### V-06 · `writeTestCase` writes no file for an empty case · reported
`testlib.h:2763` guards on `!testCaseContent.empty()`, so the caller cannot
distinguish "empty test case" from "case not found" from "validator does not
support extraction". Related: at `2766`, if `--testCaseFileName` is omitted the
extracted case is dumped to **stdout**, mixing with any other validator output.

### V-07 · `registerValidation`'s argument parser consumes tokens twice · reported
`testlib.h:4784-4825` uses a chain of independent `if`s (not `else if`) while
mutating `i` inside them, so after `setTestset(argv[++i])` the following `if`s
in the same iteration re-test the just-consumed token. `./val --testset
--testMarkupFileName markup.txt` sets the testset to the literal
`"--testMarkupFileName"` **and** consumes `markup.txt` as the markup file name.
`registerTestlibCmd` (`4886-4899`) got this right with `else if`. Also `4812`'s
range check and its message disagree by one, and the reader's own check (`1846`)
allows a third range.

### Q-02 · `std::exit` from a destructor invoked by `exit` · traced
`testlib.h:2819-2842` (`~TestlibFinalizeGuard`), `3019` (`halt`). The guard
can call `__testlib_fail` → `quitf` → `InStream::quit` → `halt` → `std::exit`
during static destruction, which is UB ([basic.start.term]). **Measured:** this
turned the scorer's clean exit 3 into a **segfault (exit 139)** before 0.9.46.
The scorer no longer finalizes from a destructor, but the hazard remains for
any `__testlib_fail` raised from `~TestlibFinalizeGuard` itself. Under
`-DTESTLIB_THROW_EXIT_EXCEPTION_INSTEAD_OF_EXIT` (`3017`) it instead *throws*
from a `noexcept` destructor → guaranteed `std::terminate`.
Fix: an explicit `__testlib_finalize()` called at the end of each `register*`
program's flow, with the destructor kept only as a diagnostic fallback that
never exits — the pattern `registerScorer` already uses since 0.9.46.

### Q-03 · `registerTestlib(int, ...)` crashes on call · traced
`testlib.h:4948` sets `argv[0] = NULL`, then `registerTestlibCmd` constructs
`std::vector<std::string> args(1, argv[0])` at `4883` — `std::string(nullptr)`
is UB and segfaults on libstdc++. The API is documented as legacy and has no
test. **Reproducer (do not add to the suite).** Fix: delete the function, or
have it build a proper `argv` array.

### Q-04 · points are validated only on the appes path · reported
`testlib.h:3191-3192`. `__testlib_preparePoints` (`4465-4480`) is reached only
through `quitp`/`__testlib_quitp`, so `quit(_points, "banana")` writes `banana`
to the result file and exits 7 with no numeric validation. `quitpi`
(`4524-4531`) rejects only spaces, accepting `""` and embedded newlines.


### F-02 · `FMT_TO_RESULT`'s reentrancy guard recurses infinitely · traced
`testlib.h:331-342`, `4986-4990`. The guard reports its own violation via
`__testlib_fail`, which calls `quitf`, which re-enters `FMT_TO_RESULT` with the
counter still non-zero → unbounded recursion → stack overflow instead of a
clean `_fail`. The counter is also not exception-safe: a `bad_alloc` at `340`
(plausible near the 16 MB buffer limit on a memory-capped judge) leaks it and
poisons **every** later format call. Fix: RAII the counter and report the
violation without re-entering the formatter.

### F-03 · `doubleCompare` treats any `|x| > 1e300` as infinite · measured
`testlib.h:465-468`, `477-482`. `__testlib_isInfinite` is a magnitude test, not
`std::isinf`. When `expected` is a finite 1e301 the function returns
`result > 0 && __testlib_isInfinite(result)` — i.e. **any** same-signed value
above 1e300 is accepted. `doubleCompare(1e301, 5e305, 1e-6)` is `true`. Affects
`dcmp`, `rcmp4/6/9` and `rncmp`. **Pinned by:** `test-doublecompare.cpp`.
Fix: use `std::isinf`/`std::isnan` (guarding the `-ffast-math` case that
`__testlib_ensuresPreconditions` already rejects).

### F-05 · `vtos` truncates at the first whitespace · traced
`testlib.h:577-584`. The non-integral overload does `ss << t; ss >> s;`, so
`vtos(std::string("a b"))` is `"a"` and any double is rendered at 6 significant
digits. `std::string` has a specialization in `expectedButFound`, but `join`,
`println` and the generic `expectedButFound<T>` do not — so
`println(3.14159265358979)` silently prints `3.14159`, a live hazard for
generators of real-valued tests. The overload also uses a function-local
`static std::stringstream` (non-reentrant). Fix: use `std::to_string` /
`ostringstream` with full precision, and document the precision contract.

---

# Ambiguities

Surprising but arguably intended. These need **documentation**, not
necessarily code changes — but each one silently produces a wrong validator or
checker, so they belong in `docs/usage-guide.md`.

### A-01 · patterns strip spaces, even inside `[...]` · measured
`testlib.h:1563-1567`. The constructor removes every unescaped space from the
whole pattern before parsing, so `[a-z ]+` silently becomes `[a-z]+` and
`pattern("No solution")` matches nothing. Pinned as intended behaviour by
`tests/test-004_use-test.h/tests/test-pattern.cpp:24-25`. This is the root cause
that makes **P-01**, **P-02**, **P-03** and **I-06** dangerous rather than merely
surprising: the pattern language looks like regex and is not.

### A-02 · no escape sequences · measured
`testlib.h:1353-1360`. `__pattern_getChar` returns the raw character after a
backslash: `\n`, `\t`, `\r`, `\xNN`, `\d`, `\w`, `\s` are **not** interpreted.
`readToken("\\d+")` matches runs of the letter `d`, not digits. The header
documents that spaces need escaping but never says escapes are *only* for
metacharacters.

### A-03 · `_dirt` destroys the original verdict · traced
`testlib.h:3153-3158`. `_dirt` is rewritten to `_pe` before `resultExitCode`, so
`DIRT_EXIT_CODE` (4) is unreachable and dirt surfaces as exit 2. Worse, the
original verdict **and its message are discarded**: `quitp(50, "half right")` on
a solution that leaves trailing output reports `wrong output format … Extra
information in the output file`, exit 2, and the 50 points are silently thrown
away. Fix: preserve the original message, and either honour `DIRT_EXIT_CODE` or
delete it.

### A-04 · verdict promotion on `inf`/`ans` · traced
`testlib.h:3119-3124`. Any non-`_fail` verdict raised on a stream whose
`mode != _output` becomes `_fail`. This is correct design — never blame the
contestant for a broken answer file — but it means `ans.quitf(_wa, …)` silently
becomes a judge failure, and a validator can never emit anything but FAIL.

### A-05 · `setSeed(long long)` ignores the top 16 bits · traced
`testlib.h:833-836` masks to 48 bits, so `rnd.setSeed(1)` and
`rnd.setSeed(1 + (1LL << 48))` produce byte-identical streams.

### A-06 · `shuffle` hard-codes the global `rnd` · traced
`testlib.h:5006-5010`. There is no `shuffle(first, last, random_t&)`, so a
generator using a private `random_t` cannot shuffle with it.

### A-07 · variable names containing a digit are invisible to bounds analysis · reported
`testlib.h:2494`. `readInt(1, n, "a1")` is completely absent from the bounds-hit
log, the constant-bounds log and the variables log — silently. This is a Polygon
convention (digits mean "indexed variable") but nothing warns.

### A-08 · `~` decorations are OR-ed into a shared record · reported
`testlib.h:2566-2576`, `2431-2436`. `n`, `~n`, `n~` and `~n~` all collapse to the
same prepared name, and the flags are merged, so **one** careless
`readInt(1, 100, "~n")` anywhere permanently marks `n`'s lower bound as hit for
the whole validator run.

### A-09 · calling `register*` twice is not detected · reported
`registered` is a plain `static bool`; `testlibMode`, `inf`, `ouf` and `ans` are
simply overwritten. `registerValidation()` after `registerTestlibCmd()` silently
reinitialises `inf` to stdin and changes the mode.

### A-10 · the finalize guard does not cover interactors or generators · reported
`testlib.h:2824-2828` checks only `_checker` (must quit) and `_validator` (must
`readEof`). An interactor that falls off the end of `main` exits 0 = OK with an
unwritten result file.

### A-11 · the guard fires after all output is produced · traced
`tests/test-005_no-register/refs/r1` shows exit code 3 alongside a complete,
valid-looking test on stdout. Any pipeline that trusts the file rather than the
exit code picks up an artifact from a failed run. Compounds **O-02**.

---

# Low and nits

**random:** `R-09` `next(double,double)` can return `to` (`963-967`), and
`wnext(double,int)` crops inconsistently across the `lim = 25` boundary (`1075`
vs `1084`) · `R-10` `abs(INT_MIN)` and `type + 1` are UB in `wnext` (`1012`,
`1026-1028`), with `wnext(n, INT_MAX)` degrading **silently** to a constant rather
than failing · `R-11` the four signed `next(from, to)` overloads have no
`from > to` check while every `wnext` and every unsigned form does, so the error
message names an overload the user never called · `R-12` `rnd.any(arr, arr + n)`
does not compile — it uses `Iter::value_type` instead of
`std::iterator_traits` (`982`) · `R-13` `rnd.any` on a `std::set`/`std::list` is
O(n) per call via `std::advance`.

**pattern:** `P-07` `{-5,3}` is accepted and behaves like `{0,3}` with a skewed
length distribution (`1460-1476`) · `P-08` `rnd.next("[]")` fails with a
`random_t` message rather than a pattern one (`1414`) · `P-09`
`__pattern_greedyMatch` takes its `std::vector<char>` **by value** and is marked
`__attribute__((pure))` while allocating (`1364`) · `P-10`
`__pattern_isCommandChar` walks back over all preceding backslashes on every
call, making construction O(n²) on backslash-heavy patterns (`1346-1348`).

**input:** `I-11` `stringToUnsignedLongLong(InStream&, const std::string&)` is
declared returning `long long` while the `const char*` overload returns
`unsigned long long` (`3871` vs `3846`) · `I-12` `maxMessageLength` underflows
when set below the warning-prefix length, making the message *longer* after
truncation (`3105-3109`) · `I-13` a missing input file leaves `reader == NULL`
and `readWordTo` dereferences it, unlike the guarded `eof()` (`3287-3314` vs
`4200`) · `I-14` `maxFileSize` is not enforced by the `FILE*` `init` overload
(`3339-3352`), so the 128 MB guarantee does not hold for interactors ·
`I-15` `int(size_t)` casts print a negative size in the file-too-big message
(`3334`) · `I-16` NUL bytes in a participant token truncate the report at
`.c_str()` · `I-17` `StringInputStreamReader::unreadChar` writes into the source
string (`1772-1773`) · `I-18` the 2 MB buffer and its `isEof` array are never
zeroed (`1955-1958`) · `I-19` `refill()` issues a fresh `fread` on every
`curChar`/`eof` once the file is exhausted (`1914-1940`) · `I-20`
`__testlib_part` builds a full copy of a 32 MB token to produce a 64-character
excerpt, on the error path (`3459-3470`), and `t.substr(s.length() - 31, 31)` at
`3469` should use `t.length()` — provably equal today, wrong by intent.

**opts / validator / verdict:** `O-10` duplicate keys silently last-win
(`5551`) · `O-11` `prepareOpts` fails when `argc <= 0` but prints
"expected argc>=0" (`5589`) · `V-08` `writeTestMarkup`'s error message names
`_testCaseFileName` (`2732`) · `V-09` `ConstantBound` has no constructor
(`2439-2462`) and `std::to_string(double)` gives 6 decimals, so
`readDouble(1e-9, 1, "x")` reports its lower bound as `0.000000` and collides
with a genuine `0` · `Q-05` the guard tests `__testlib_exitCode == 0` rather
than `== OK_EXIT_CODE` (`2837`), so under `-DCONTESTER` (`OK_EXIT_CODE 0xAC`)
the validator logs are silently never written · `Q-06` `startTest(-3)` creates a
file literally named `-3` (`5060-5064`), and the `"wt"` mode string is an MSVC
extension · `Q-07` `__testlib_ensuresPreconditions` runs before `resultName` is
known, so an `-ffast-math` rejection produces exit 3 with **no result file**
(`4581`).

**misc:** `F-04` the C++20 `format()` computes `size_t(snprintf(…) + 1)`, which is 0 when
`snprintf` returns −1, then reads `buffer.data()` of an empty vector
(`6335-6349`); the C++20 and fallback implementations also differ in truncation
behaviour (unbounded vs 16 MB) for the same call · `F-06`
`quitf(_wa, msg.c_str())` is a format-string bug with **no** compiler warning,
because the `std::string` overloads of `format`/`testlib_format_` cannot carry
`__attribute__((format))` (`6329-6332`, `6358-6361`); every in-tree call site
passes a literal — checked across `checkers/`, `validators/`, `interactors/` —
so this is an API footgun, not a live vulnerability · `F-07` `englishEnding`
returns `"th"` for all negative input because `x %= 100` keeps the sign
(`5070`) · `F-08` `__testlib_format_buffer` is a **16 MB static array**
(`328`), so every testlib binary carries 16 MB of BSS and is inherently
non-reentrant.

---

# Suspicions — verify before acting

- **B-20 · C++03 is broken at link.** `TestlibFinalizeGuard testlibFinalizeGuard;`
  is *defined* at `6318`, inside the `#if __cplusplus > 199711L || defined(_MSC_VER)`
  block that opens at `5277`, while the `extern` declaration (`2851`) and its
  uses in `InStream::quit` (`3087`) and `InStream::readEof` (`4276`) are outside
  it. `g++ -std=c++03` should fail with "undefined reference". CI only covers
  c++11…c++23 (`tests/run.sh:93`), so this is untested, yet `latestFeatures`
  still advertises c++03 support. **Verify by compiling.**
- **S-05 · `opt<long long>` above 2^53.** `testlib.h:5712` uses
  `fabsl(value - about) > 0.1` with `long double` as the reference. Where
  `long double == double` (MSVC, most ARM64 ABIs) a *valid* value such as
  `9007199254740993` should round in `about` and trip "integer overflow".
  **Verify on an affected platform.**
- **O-12 · `-n=1e2000000000`.** The exponent is bounded only by `int` range, so
  the positive loop at `5656` appends ~2×10^9 characters. A hang/OOM reachable
  straight from the command line, but bounded — confirm the actual cost.

---

# Future work

Beyond fixing the individual findings:

1. **`registerGen(argc, argv, 2)`** — a new random version carrying the R-01
   `nextBits` fix, the R-07 seeding fix, and the R-03/R-04/R-05 overload
   corrections together, so problem setters take one deliberate step.
2. **Replace the character-class `std::vector<char>` with a 256-bit bitmap**
   indexed by `unsigned char` — removes P-04's architecture dependence and
   makes `__pattern_greedyMatch` allocation-free (P-09).
3. **An explicit `__testlib_finalize()`** instead of destructor-driven
   finalisation — structurally removes S-01, S-02 and Q-02 rather than patching
   each.
4. **A statable opts grammar** with a `--` terminator, uniform `used` tracking,
   and consistent missing-key behaviour across types (O-02, O-03, O-04, O-06).
5. **Fuzz `InStream`** against hostile participant output — most of the `I-*`
   findings are the kind a fuzzer surfaces in minutes.
6. **Build the samples with `-Wformat-security`** in CI to catch F-06-class
   mistakes in contributed checkers.
7. **Add c++03 to the CI matrix** or drop the claim of c++03 support (B-20).
