# Malformed pattern repetition counts fail loudly

Before 0.9.52 a pattern such as `{3,}` silently meant exactly 3, and counts with trailing garbage such as `{1O}` meant 1 (findings P-01 and P-02). We made both an error rather than giving `{n,}` an unbounded meaning such as `{n, INT_MAX}`. Failing is symmetric with how `{,5}` already behaves, and it forces the author to state the upper bound that a validator exists to enforce. Redefining `{n,}` would also have turned every existing exact-3 pattern into an unbounded one without warning. See `.out-of-scope/open-ended-pattern-repetition.md`.
