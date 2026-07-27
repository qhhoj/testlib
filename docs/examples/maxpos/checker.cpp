/**
 * MAXPOS checker.
 *
 * The problem allows ANY index of a maximum element, so a plain token
 * comparison (wcmp) would reject correct submissions. This checker reads the
 * test itself and accepts any index whose value equals the jury's value.
 *
 * Run as:  ./checker <input> <participant-output> <jury-answer>
 *
 * Streams:
 *   inf - the test file       (a failure here means _fail: the test is broken)
 *   ouf - participant output  (a failure here means _wa / _pe)
 *   ans - jury answer         (a failure here means _fail: the jury is broken)
 */

#include "testlib.h"
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    /* setName must be called before registerTestlibCmd. */
    setName("MAXPOS: accepts any index of a maximum element");
    registerTestlibCmd(argc, argv);

    int t = inf.readInt();

    for (int testCase = 1; testCase <= t; testCase++) {
        int n = inf.readInt();
        vector<int> a(n);
        for (int i = 0; i < n; i++)
            a[i] = inf.readInt();

        /* Range-checked reads: on ans an out-of-range value is promoted to
           _fail, on ouf it becomes _wa. */
        int ja = ans.readInt(1, n, "index");
        int pa = ouf.readInt(1, n, "index");

        if (a[ja - 1] != a[pa - 1])
            quitf(_wa, "test case %d: jury answer %d has value %d, "
                       "but participant answer %d has value %d",
                  testCase, ja, a[ja - 1], pa, a[pa - 1]);
    }

    /* No explicit seekEof() needed: on _ok testlib checks the tail of ouf
       itself and returns _dirt (exit code 4) if anything is left over. */
    quitf(_ok, "%d test case(s)", t);
}
