/* MAXPOS deliberately wrong solution: always answers index 1.
   Used to demonstrate a "wrong answer" verdict from the checker. */

#include <cstdio>

int main() {
    int t;
    if (scanf("%d", &t) != 1)
        return 1;
    while (t--) {
        int n;
        if (scanf("%d", &n) != 1)
            return 1;
        for (int i = 0; i < n; i++) {
            int x;
            if (scanf("%d", &x) != 1)
                return 1;
        }
        printf("1\n");
    }
    return 0;
}
