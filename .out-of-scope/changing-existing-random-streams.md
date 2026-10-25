# Changing the output of an existing random generator version

testlib will not change what random generator versions 0, 1 or 2 produce,
even to fix a real defect in them.

## Why this is out of scope

Problem packages regenerate their tests from the generator and its command
line. If the random stream of an existing version changed, every package
built with that version would silently produce different tests the next time
it was built: tests that may no longer hit the cases the jury chose, and
answers that no longer match. That is a compatibility break across every
judge that uses testlib, which is why `tests/test-003_run-rnd` pins all three
versions byte for byte.

Defects in the stream are real (for example, `rnd.next(0, 1)` repeats every
65536 draws under version 1). They are fixed by adding a **new** random
generator version that generators opt into explicitly:

```cpp
registerGen(argc, argv, 2);   // new generators: use the newest version
```

That is how version 1 fixed version 0 and version 2 fixed version 1. See
`docs/adr/0002-random-streams-are-frozen.md`. A request to fix a stream
defect is in scope as a proposal for the next version, never as an edit to
an existing one.

## Prior requests

None yet.
