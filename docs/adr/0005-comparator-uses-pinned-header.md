# The reference comparator is built against a pinned testlib.h

`tests/src/tester-lcmp.cpp` compares every test run with its ref, and it is compiled against `tests/lib/testlib.h`, which is pinned at `0.9.40-SNAPSHOT`, not against the root header under test. A regression in the root header must not be able to break the tool that detects regressions, so the pinned copy is never synced with the root one, even though it looks stale. This is upstream's arrangement.
