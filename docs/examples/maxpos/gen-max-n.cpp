/**
 * gen-max-n  [-n <num>] [-value <num>]
 *
 * Edge-case generator: a single test case at the maximum allowed n where every
 * element is equal, so every index is a valid answer. This is the test that
 * catches a checker which compares indices instead of values.
 *
 *   -n      array length. Default: 200000 (the constraint maximum).
 *   -value  the repeated value. Default: 10^9.
 */

#include "testlib.h"
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    registerGen(argc, argv, 1);

    int n = opt<int>("n", 200000);
    int value = opt<int>("value", 1000000000);

    println(1);
    println(n);
    println(vector<int>(n, value));
}
