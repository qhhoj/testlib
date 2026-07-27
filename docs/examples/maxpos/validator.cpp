/**
 * MAXPOS validator.
 *
 * Format:
 *   t                      (1 <= t <= 10^4)
 *   for each test case:
 *     n                    (1 <= n <= 2*10^5)
 *     a_1 ... a_n          (-10^9 <= a_i <= 10^9)
 *   sum of n over all test cases <= 2*10^5
 *
 * Run as:  ./validator < input
 * Exit code 0 means the file is valid; anything else is a validation failure
 * and the reason is printed to stderr.
 */

#include "testlib.h"

using namespace std;

const int MAX_T = 10000;
const int MAX_N = 200000;
const int MAX_SUM_N = 200000;
const int MAX_VALUE = 1000000000;

int main(int argc, char *argv[]) {
    registerValidation(argc, argv);

    int t = inf.readInt(1, MAX_T, "t");
    inf.readEoln();

    long long sumN = 0;
    for (int testCase = 1; testCase <= t; testCase++) {
        /* Makes every message below say "[test case k]" and drives the
           --testCase / --testMarkupFileName machinery. */
        setTestCase(testCase);

        int n = inf.readInt(1, MAX_N, "n");
        inf.readEoln();

        /* Reads exactly n integers separated by single spaces. Out-of-range
           values are reported as a[index]. */
        inf.readInts(n, -MAX_VALUE, MAX_VALUE, "a");
        inf.readEoln();

        sumN += n;
    }
    unsetTestCase();

    ensuref(sumN <= MAX_SUM_N, "sum of n over all test cases is %lld, must be at most %d",
            sumN, MAX_SUM_N);

    /* Required: TestlibFinalizeGuard fails the validator if readEof() is missing. */
    inf.readEof();
}
