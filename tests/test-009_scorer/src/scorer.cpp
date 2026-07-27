/*
 * Sample scorer: total points over the tests that passed.
 *
 * Reads serialized TestResult records from stdin, one per line, and prints a
 * single %.3f score. The serialization format is
 *   index;testset;group;VERDICT;points;timeMs;memoryBytes;input;output;answer;exitCode;comment
 */

#include "testlib.h"
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    registerScorer(argc, argv, [](vector<TestResult> results) -> double {
        double total = 0.0;
        for (size_t i = 0; i < results.size(); i++)
            if (results[i].verdict == OK)
                total += results[i].points;
        return total;
    });
}
