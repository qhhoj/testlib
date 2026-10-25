# Treat testlib.h as vendored upstream code

This repository forks upstream `MikeMirzayanov/testlib` and now carries its own fixes, but `testlib.h` is still treated as upstream's file: each fork patch is kept small and isolated, so that rebasing onto a new upstream release stays cheap, and fixes are sent upstream where possible. A refactor that makes fork patches harder to rebase needs a reason that outweighs that cost.
