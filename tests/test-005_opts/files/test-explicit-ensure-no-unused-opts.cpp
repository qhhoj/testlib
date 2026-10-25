#include "testlib.h"

/* An explicit ensureNoUnusedOpts() call stays fatal and lists every unused
   key, even when the automatic check is suppressed. */

int main(int argc, char** argv) {
    registerGen(argc, argv, 2);

    if (has_opt("suppress"))
        suppressEnsureNoUnusedOpts();

    int n = opt<int>("n");
    println("n =", n);

    ensureNoUnusedOpts();
    println("checked");
}
