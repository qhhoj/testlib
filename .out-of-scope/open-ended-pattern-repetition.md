# Open-ended pattern repetition (`{n,}`)

testlib patterns do not support an open-ended repetition count. `[a-z]{3,}`
is an error, not "three or more letters", and it will stay that way.

## Why this is out of scope

Before 0.9.52, `{3,}` was silently parsed as exactly `{3}`, so a validator
written as

```cpp
inf.readToken("[a-z]{3,}", "s");
```

rejected every token longer than three characters without saying so. When
that was fixed (finding P-01), there were two choices: make `{n,}` mean
"at least n", or make it fail. We chose failing loudly, which is recorded in
`docs/adr/0003-malformed-repetition-counts-fail.md`, for two reasons:

- It matches `{,5}`, which already fails, so the two open-ended forms behave
  the same way.
- A validator exists to enforce the upper bound. Making the author write it
  (`[a-z]{3,100}`) keeps the constraint visible and lets the validator reject
  oversized tokens instead of accepting them.

Giving `{n,}` an unbounded meaning now would also silently change what every
package written against the old behaviour accepts.

If you need "at least n", write the real upper bound from the problem's
constraints.

## Prior requests

None yet.
