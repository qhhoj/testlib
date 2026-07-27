# testlib documentation

Guides for this repository. The root [`README.md`](../README.md) is the
upstream one-page introduction; these documents go further.

| Document | For | Contents |
| --- | --- | --- |
| [usage-guide.md](usage-guide.md) | Problem setters | Writing generators, validators, checkers, interactors and scorers. The `rnd` and `InStream` APIs, patterns, verdicts and exit codes, gotchas. |
| [development-guide.md](development-guide.md) | Maintainers | Repository provenance, a map of `testlib.h`, the `tests/` harness, CI, and the checklist for changing the library. |
| [examples/maxpos/](examples/maxpos/README.md) | Both | A complete runnable problem package — generator, validator, special checker, correct and wrong solutions, and a script that runs the whole pipeline. |
| [../plan.md](../plan.md) | Maintainers | Audit of `testlib.h`: confirmed bugs, ambiguities and security issues, each with a reproducer and a fix, ranked by severity. Blocks are deleted as findings are fixed. |

Start with the usage guide; it links into the example where a concrete
demonstration helps.

## Adding a topic

`docs/` is meant to grow. Conventions:

- One topic per file, kebab-case, `.md`, directly in `docs/`.
- Add a row to the table above.
- Runnable examples go in `docs/examples/<name>/` with a `README.md` and a
  build-and-run script, following `examples/maxpos/`.
- Any `.cpp` you add under `docs/` is picked up by the repo's
  `tests/test-000_compile-all-cpp` sweep, so it must compile warning-free
  under `-std=c++11 … c++23 -Wpedantic -Werror`, **on Windows too**.
- If the file does **not** include `testlib.h`, avoid `scanf`, `fopen`,
  `sprintf`, `strcpy` and friends: the Windows UCRT headers mark them
  deprecated, and `-Werror` turns that into a build failure. `testlib.h`
  defines `_CRT_SECURE_NO_WARNINGS` for everything that includes it, which is
  why the testlib samples are fine and a plain solution file is not. Use
  `<iostream>` / `<fstream>` instead. This is not reproducible on Linux or
  macOS — see the note in the development guide.
- Cite `testlib.h:<line>` for claims about the library, and re-check them when
  the version changes — the line numbers in these guides are for 0.9.50.

## Note

`docs/read.me` is a legacy byte-identical copy of the root `README.md`, kept
for historical reasons. It is not maintained; do not edit it.
