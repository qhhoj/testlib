#include "testlib.h"

/* No "using namespace std;" on purpose: this file uses no std name, and under
   C++23 the directive makes println("x =", n) ambiguous with std::println
   from <print>, which libc++ pulls in transitively. See plan.md F-09. */

int main(int argc, char** argv) {
    registerGen(argc, argv, 1);
    suppressEnsureNoUnusedOpts();
    
    int test_count = opt<int>("test-count");
    int sum_n = opt<int>("sum-n");
    int min_n = opt<int>("min-n", 1);
    
    int min_value = opt<int>("min-value", 1);
    int max_value = opt<int>("max-value", 1000 * 1000 * 1000);
    int value_bias = opt<int>("value-bias", 0);
    
    println("test-count =", test_count);
    println("sum-n =", sum_n);
    println("min-n =", min_n);
    println("min-value =", min_value);
    println("max-value =", max_value);
    println("value-bias =", value_bias);
}
