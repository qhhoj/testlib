/**
 * gen-random  -test-count <num>
 *             -sum-n <num>
 *             [-min-n <num>]
 *             [-min-value <num>] [-max-value <num>]
 *             [-value-bias <num>]
 *
 * Generates a MAXPOS test with `test-count` test cases whose lengths sum to
 * exactly `sum-n`.
 *
 *   -test-count  number of test cases. Required.
 *   -sum-n       sum of n over all test cases. Required.
 *   -min-n       minimum n for a single test case. Default: 1.
 *   -min-value   minimum array element. Default: -10^9.
 *   -max-value   maximum array element. Default: 10^9.
 *   -value-bias  bias passed to rnd.wnext(): positive pulls values toward
 *                max-value, negative toward min-value. Default: 0 (uniform).
 *
 * The seed is derived from the whole command line, so the same arguments
 * always produce byte-identical output and different arguments produce a
 * different test. Never add your own srand()/time()-based seeding.
 *
 * Modelled on generators/gen-array-with-opt.cpp.
 */

#include "testlib.h"
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    registerGen(argc, argv, 1);

    int testCount = opt<int>("test-count");
    int sumN = opt<int>("sum-n");
    int minN = opt<int>("min-n", 1);

    int maxValue = opt<int>("max-value", 1000000000);
    int minValue = opt<int>("min-value", -1000000000);
    int valueBias = opt<int>("value-bias", 0);

    /* Random composition of sumN into testCount parts, each at least minN. */
    vector<int> ns = rnd.partition(testCount, sumN, minN);

    println(testCount);
    for (int testCase = 0; testCase < testCount; testCase++) {
        int n = ns[testCase];
        vector<int> a(n);
        for (int i = 0; i < n; i++)
            a[i] = rnd.wnext(minValue, maxValue, valueBias);
        println(n);
        println(a);
    }
}
