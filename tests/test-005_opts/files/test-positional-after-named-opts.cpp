#include "testlib.h"

/* A positional read turns the unused-key warning off for the whole run, even
   when it comes after a named read has already turned the check on. */

int main(int argc, char** argv) {
    registerGen(argc, argv, 2);

    int n = opt<int>("n");
    int m = opt<int>(2);

    println(n, m);
}
