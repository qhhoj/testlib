#include "testlib.h"

/* Reading by position is deprecated: one warning per run however many
   positional reads there are, and no unused-key warning for that run. */

int main(int argc, char** argv) {
    registerGen(argc, argv, 2);

    int a = opt<int>(1);
    int b = opt<int>(2);
    std::string c = opt(3, std::string("none"));
    int n = opt<int>("n", 0);

    println(a, b, c, n);
}
