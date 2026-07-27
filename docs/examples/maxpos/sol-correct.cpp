/* MAXPOS reference solution. Plain stdin/stdout, no testlib. */

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <vector>

int main() {
    int t;
    if (scanf("%d", &t) != 1)
        return 1;
    while (t--) {
        int n;
        if (scanf("%d", &n) != 1)
            return 1;
        int best = 0, bestValue = 0;
        for (int i = 0; i < n; i++) {
            int x;
            if (scanf("%d", &x) != 1)
                return 1;
            if (i == 0 || x > bestValue) {
                bestValue = x;
                best = i;
            }
        }
        printf("%d\n", best + 1);
    }
    return 0;
}
