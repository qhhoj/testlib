# Git treats every file as binary

`.gitattributes` is the single line `* binary`, so git never normalises line endings. The test fixtures include `unix/` and `win/` variants of the same input that differ only in their line endings, and any normalisation would silently make them identical. The cost is that every `git diff` reports "Binary files differ", markdown included (use `git diff --text` or `git show --text`). This is upstream's arrangement; do not "fix" it.
