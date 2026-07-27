/*
 * Pins the version 2 random stream (registerGen(argc, argv, 2)).
 *
 * Version 2 is a compatibility surface exactly like versions 0 and 1: once
 * problem packages are generated with it, its output must never change. If a
 * ref here moves, that is a breaking change and needs a latestFeatures entry.
 *
 * Deliberately a separate program from gen.cpp so that the existing version 1
 * invocations, and their reference files, are untouched.
 */

#include "testlib.h"

#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    registerGen(argc, argv, 2);

    int n = opt<int>(1);

    /* The call that was periodic under version 1. */
    string bits;
    for (int i = 0; i < n; i++)
        bits += char('0' + rnd.next(0, 1));
    println(bits);

    /* Small ranges: these read the low bits of the 63-bit draw. */
    vector<int> small(n);
    for (int i = 0; i < n; i++)
        small[i] = rnd.next(0, 3);
    println(small);

    /* The int overload, which uses a different code path. */
    vector<int> viaInt(n);
    for (int i = 0; i < n; i++)
        viaInt[i] = rnd.next(4);
    println(viaInt);

    /* Wide and 64-bit ranges. */
    println(rnd.next(-1000000000, 1000000000));
    println(rnd.next(1LL, 1000000000000000000LL));
    println(rnd.wnext(0, 1000, 5));
    println(rnd.wnext(0, 1000, -5));

    /* Sequence helpers. */
    println(rnd.perm(n));
    println(rnd.distinct(n, 0, 10 * n));
    println(rnd.partition(n, 10 * n));

    /* Patterns and shuffle. */
    println(rnd.next("[a-z]{20}"));
    vector<int> toShuffle(n);
    for (int i = 0; i < n; i++)
        toShuffle[i] = i;
    shuffle(toShuffle.begin(), toShuffle.end());
    println(toShuffle);

    /* Doubles, printed with a fixed precision so the ref is stable. */
    printf("%.6f\n", rnd.next());
    printf("%.6f\n", rnd.next(1.0, 2.0));
}
