/* MAXPOS deliberately wrong solution: always answers index 1.
   Used to demonstrate a "wrong answer" verdict from the checker.

   Uses iostream rather than scanf for the reason given in sol-correct.cpp. */

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

        for (int i = 0; i < n; i++) {
            int x;
            if (!(std::cin >> x))
                return 1;
        }

        std::cout << 1 << "\n";
    }

    return 0;
}
