/* MAXPOS reference solution. Plain stdin/stdout, no testlib.
 *
 * Uses iostream rather than scanf: this file does not include testlib.h, so it
 * does not get the _CRT_SECURE_NO_WARNINGS that testlib.h defines, and the
 * Windows UCRT headers mark scanf deprecated. The repo compiles every .cpp
 * outside tests/ with -Wpedantic -Werror, so that would break the build.
 */

#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int t;
    if (!(std::cin >> t))
        return 1;

    while (t--) {
        int n;
        if (!(std::cin >> n))
            return 1;

        int best = 0, bestValue = 0;
        for (int i = 0; i < n; i++) {
            int x;
            if (!(std::cin >> x))
                return 1;
            if (i == 0 || x > bestValue) {
                bestValue = x;
                best = i;
            }
        }

        std::cout << best + 1 << "\n";
    }

    return 0;
}
