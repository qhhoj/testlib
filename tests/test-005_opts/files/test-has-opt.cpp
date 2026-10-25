#include "testlib.h"

/* has_opt counts as reading a key, and every named read (here opt<int> alone,
   when n <= 1 skips has_opt) turns the automatic unused-key check on. */

int main(int argc, char** argv) {
    registerGen(argc, argv, 2);

    int n = opt<int>("n");
    int sorted = n > 1 && has_opt("sorted") ? 1 : 0;

    println("n =", n);
    println("sorted =", sorted);
}
