# Checkers and interactors are held to a security bar

Checkers and interactors run on judge machines and read untrusted participant output, so a crash, hang or multi-gigabyte allocation that participant output can trigger is a security defect, not just a bug, and it is prioritised as one. Generators and validators read only jury data and are held to a lower bar. This is why defects in input reading are ranked by whether they can be reached from participant output.
