#define TESTLIB_THROW_EXIT_EXCEPTION_INSTEAD_OF_EXIT

#include "testlib.h"
#include "test.h"

using namespace std;

#include "tests/test-join.cpp"
#include "tests/test-split.cpp"
#include "tests/test-tokenize.cpp"
#include "tests/test-opts.cpp"
#include "tests/test-instream.cpp"
#include "tests/test-pattern.cpp"
#include "tests/test-stringToLongLong.cpp"
#include "tests/test-stringToUnsignedLongLong.cpp"

/* Tests below pin known-defective behaviour so it cannot change unnoticed.
   Each one names the plan.md entry that will flip it. See ../../plan.md. */
#include "tests/test-rnd-quality.cpp"
#include "tests/test-pattern-defects.cpp"
#include "tests/test-instream-defects.cpp"
#include "tests/test-doublecompare.cpp"
#include "tests/test-opts-defects.cpp"

/* Fixed behaviour: these assert the DESIRED result, not a pinned defect. */
#include "tests/test-scorer-serialization.cpp"

int main() {
    disableFinalizeGuard();
    run_tests();
}
